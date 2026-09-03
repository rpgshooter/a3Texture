#include "paa.h"
#include "utils.h"
#include "image_loader.h"

#include <squish.h>
#include <lzo/lzo1x.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>
#include <thread>
#include <atomic>

namespace a3tex {

using namespace utils;

namespace {

void ensureLzoInit() {
    static std::once_flag flag;
    static int result = LZO_E_OK;
    std::call_once(flag, [] { result = lzo_init(); });
    if (result != LZO_E_OK) {
        throw std::runtime_error("LZO initialization failed");
    }
}

size_t dxtDataSize(PAAFormat format, uint32_t width, uint32_t height) {
    const size_t blocks = size_t((width + 3) / 4) * ((height + 3) / 4);
    return blocks * (format == PAAFormat::DXT1 ? 8 : 16);
}

// BIS compresses mipmaps wider than 128px and stores the rest raw.
constexpr uint32_t kLzoMinWidth = 128;

// Colour is stored gamma encoded, so averaging it directly darkens each level.
// Retail mip chains match a linear average far more closely, by a wide margin
// on high contrast areas, so the colour channels are converted before
// averaging and back afterwards. Alpha carries data rather than colour and is
// averaged as it is.
const std::array<float, 256>& srgbToLinear() {
    static const std::array<float, 256> table = [] {
        std::array<float, 256> values{};
        for (int i = 0; i < 256; i++) {
            values[i] = std::pow(i / 255.0f, 2.2f);
        }
        return values;
    }();
    return table;
}

uint8_t linearToSrgb(float value) {
    const float encoded = std::pow(std::clamp(value, 0.0f, 1.0f), 1.0f / 2.2f);
    return static_cast<uint8_t>(encoded * 255.0f + 0.5f);
}

template <typename Fn>
void parallelFor(size_t count, unsigned limit, Fn&& fn) {
    const size_t hw = limit ? limit : std::max(1u, std::thread::hardware_concurrency());
    const size_t workers = std::min(hw, count);

    if (workers <= 1) {
        for (size_t i = 0; i < count; i++) fn(i);
        return;
    }

    std::atomic<size_t> next{0};
    std::vector<std::thread> threads;
    threads.reserve(workers);
    for (size_t t = 0; t < workers; t++) {
        threads.emplace_back([&] {
            for (size_t i = next++; i < count; i = next++) fn(i);
        });
    }
    for (auto& thread : threads) thread.join();
}

int squishFlags(PAAFormat format, Quality quality) {
    int flags = (format == PAAFormat::DXT1) ? squish::kDxt1 : squish::kDxt5;
    switch (quality) {
        case Quality::Fast:   flags |= squish::kColourRangeFit; break;
        case Quality::Normal: flags |= squish::kColourClusterFit; break;
        case Quality::High:   flags |= squish::kColourIterativeClusterFit; break;
    }
    return flags;
}

// Swizzle bytes select, per output channel in A,R,G,B order, which source
// channel supplies it: 0-3 pass through A,R,G,B, 4-7 invert them, 8 is a
// constant 1. Verified against retail a3 textures.
enum class FlagPolicy { Auto, Never, Always };

struct SwizzlePreset {
    std::vector<uint8_t> bytes;
    PAAFormat format;
    FlagPolicy flag;
};

const SwizzlePreset* swizzlePreset(SwizzleType type) {
    static const SwizzlePreset nohq{{0x05, 0x04, 0x02, 0x03}, PAAFormat::DXT5, FlagPolicy::Never};
    static const SwizzlePreset no{{0x08, 0x01, 0x02, 0x03}, PAAFormat::DXT1, FlagPolicy::Never};
    static const SwizzlePreset smdi{{0x08, 0x08, 0x02, 0x03}, PAAFormat::DXT1, FlagPolicy::Never};
    static const SwizzlePreset as{{0x08, 0x08, 0x02, 0x08}, PAAFormat::DXT1, FlagPolicy::Never};
    static const SwizzlePreset dt{{0x08, 0x00, 0x00, 0x00}, PAAFormat::DXT1, FlagPolicy::Always};

    switch (type) {
        case SwizzleType::NOHQ: return &nohq;
        case SwizzleType::NO:   return &no;
        case SwizzleType::SMDI: return &smdi;
        case SwizzleType::AS:   return &as;
        case SwizzleType::DT:   return &dt;
        case SwizzleType::NONE: break;
    }
    return nullptr;
}

const std::vector<uint8_t>* swizzleBytes(SwizzleType type) {
    const SwizzlePreset* preset = swizzlePreset(type);
    return preset ? &preset->bytes : nullptr;
}

uint8_t sampleSwizzle(const uint8_t* pixel, uint8_t selector) {
    if (selector == 0x08) {
        return 255;
    }
    // Selector indexes A,R,G,B; the buffer is RGBA.
    static const int order[4] = {3, 0, 1, 2};
    const uint8_t value = pixel[order[selector & 0x03]];
    return (selector >= 0x04) ? static_cast<uint8_t>(255 - value) : value;
}

void applySwizzle(MipMap& mipmap, const std::vector<uint8_t>& bytes, unsigned threads) {
    const size_t pixels = mipmap.data.size() / 4;
    parallelFor(pixels, threads, [&](size_t i) {
        uint8_t* pixel = mipmap.data.data() + i * 4;
        const uint8_t source[4] = {pixel[0], pixel[1], pixel[2], pixel[3]};
        pixel[3] = sampleSwizzle(source, bytes[0]);
        pixel[0] = sampleSwizzle(source, bytes[1]);
        pixel[1] = sampleSwizzle(source, bytes[2]);
        pixel[2] = sampleSwizzle(source, bytes[3]);
    });
}

SwizzleType swizzleFromBytes(const std::vector<uint8_t>& data) {
    for (auto type : {SwizzleType::NOHQ, SwizzleType::NO, SwizzleType::SMDI,
                      SwizzleType::AS, SwizzleType::DT}) {
        const auto* bytes = swizzleBytes(type);
        if (bytes && *bytes == data) {
            return type;
        }
    }
    return SwizzleType::NONE;
}

}

PAA::PAA() : format(PAAFormat::DXT5), magicNumber(0xFF05) {}

PAA::PAA(const std::string& filename) {
    inputStream = std::make_shared<std::ifstream>(filename, std::ios::binary);
}

PAA::PAA(const std::vector<uint8_t>& data) {
    inputStream = std::make_shared<std::stringstream>(
        std::string(data.begin(), data.end())
    );
}

void PAA::readPAA() {
    if (!inputStream) {
        throw std::runtime_error("No input stream available");
    }

    auto& stream = *inputStream;

    // Read magic number
    magicNumber = readBytes<uint16_t>(stream);

    switch (magicNumber) {
        case 0xFF01: format = PAAFormat::DXT1; break;
        case 0xFF02: format = PAAFormat::DXT2; break;
        case 0xFF03: format = PAAFormat::DXT3; break;
        case 0xFF04: format = PAAFormat::DXT4; break;
        case 0xFF05: format = PAAFormat::DXT5; break;
        case 0x4444: format = PAAFormat::RGBA4444; break;
        case 0x1555: format = PAAFormat::RGBA5551; break;
        case 0x8888: format = PAAFormat::RGBA8888; break;
        case 0x8080: format = PAAFormat::GRAY_ALPHA; break;
        default:
            throw std::runtime_error("Invalid PAA magic number: " + std::to_string(magicNumber));
    }

    // Read tags
    while (stream.peek() != 0) {
        Tagg tagg;
        tagg.signature = readString(stream, 8);
        tagg.dataLength = readBytes<uint32_t>(stream);
        tagg.data = readBytes<uint8_t>(stream, tagg.dataLength);
        taggs.push_back(tagg);

        if (tagg.signature == "GGATGALF") {
            hasTransparency = true;
        } else if (tagg.signature == "GGATZIWS") {
            swizzle = swizzleFromBytes(tagg.data);
            swizzleMode = SwizzleMode::TagOnly;
        }
    }

    // Read palette
    palette.dataLength = readBytes<uint16_t>(stream);
    if (palette.dataLength > 0) {
        palette.data = readBytes<uint8_t>(stream, palette.dataLength);
    }

    // Read mipmaps
    while (peekBytes<uint16_t>(stream) != 0) {
        MipMap mipmap;
        mipmap.width = readBytes<uint16_t>(stream);
        mipmap.height = readBytes<uint16_t>(stream);
        mipmap.dataLength = readBytesAsArmaUShort(stream);
        mipmap.data = readBytes<uint8_t>(stream, mipmap.dataLength);

        if ((mipmap.width & 0x8000) != 0) {
            mipmap.width &= 0x7FFF;
            mipmap.lzoCompressed = true;
            decompressLZO(mipmap);
        }

        // Decompress DXT
        if (format == PAAFormat::DXT1) {
            decompressDXT1(mipmap);
        } else if (format == PAAFormat::DXT5) {
            decompressDXT5(mipmap);
        }

        mipMaps.push_back(mipmap);
    }
}

void PAA::loadImage(const std::string& filename) {
    setImage(ImageLoader::load(filename));
}

void PAA::setImage(const ImageData& img) {
    mipMaps.clear();

    MipMap mipmap;
    mipmap.width = img.width;
    mipmap.height = img.height;
    mipmap.data = img.data;
    mipmap.dataLength = img.data.size();

    mipMaps.push_back(mipmap);
    calculateMipmapsAndTaggs();
}

void PAA::calculateMipmapsAndTaggs() {
    if (mipMaps.empty()) {
        throw std::runtime_error("No mipmaps to calculate from");
    }

    uint32_t curWidth = mipMaps[0].width;
    uint32_t curHeight = mipMaps[0].height;

    // Generate mipmaps
    std::vector<MipMap> generatedMips;
    generatedMips.push_back(mipMaps[0]);

    while (std::min(curWidth, curHeight) > 4) {
        uint32_t newWidth = curWidth / 2;
        uint32_t newHeight = curHeight / 2;

        MipMap mipmap;
        mipmap.width = newWidth;
        mipmap.height = newHeight;
        mipmap.data.resize(newWidth * newHeight * 4);

        // Box filter, averaging colour in linear space.
        const auto& srcData = generatedMips.back().data;
        const auto& toLinear = srgbToLinear();

        for (uint32_t y = 0; y < newHeight; y++) {
            for (uint32_t x = 0; x < newWidth; x++) {
                const uint32_t sx = x * 2;
                const uint32_t sy = y * 2;

                const size_t p1 = (size_t(sy) * curWidth + sx) * 4;
                const size_t p2 = (size_t(sy) * curWidth + sx + 1) * 4;
                const size_t p3 = (size_t(sy + 1) * curWidth + sx) * 4;
                const size_t p4 = (size_t(sy + 1) * curWidth + sx + 1) * 4;
                const size_t out = (size_t(y) * newWidth + x) * 4;

                for (int c = 0; c < 3; c++) {
                    const float sum = toLinear[srcData[p1 + c]] + toLinear[srcData[p2 + c]] +
                                      toLinear[srcData[p3 + c]] + toLinear[srcData[p4 + c]];
                    mipmap.data[out + c] = linearToSrgb(sum * 0.25f);
                }

                const uint32_t alpha = srcData[p1 + 3] + srcData[p2 + 3] +
                                       srcData[p3 + 3] + srcData[p4 + 3];
                mipmap.data[out + 3] = static_cast<uint8_t>(alpha / 4);
            }
        }

        mipmap.dataLength = mipmap.data.size();
        generatedMips.push_back(mipmap);

        curWidth = newWidth;
        curHeight = newHeight;
    }

    mipMaps = generatedMips;

    // 64 bit, since 4096x4096 of pure white already fills a uint32_t.
    uint64_t sumRed = 0;
    uint64_t sumGreen = 0;
    uint64_t sumBlue = 0;
    uint64_t sumAlpha = 0;

    for (size_t i = 0; i < mipMaps[0].data.size(); i += 4) {
        sumRed += mipMaps[0].data[i];
        sumGreen += mipMaps[0].data[i + 1];
        sumBlue += mipMaps[0].data[i + 2];
        sumAlpha += mipMaps[0].data[i + 3];
    }

    const uint64_t pixelCount =
        uint64_t(mipMaps[0].width) * mipMaps[0].height;

    averageRed = uint32_t(sumRed / pixelCount);
    averageGreen = uint32_t(sumGreen / pixelCount);
    averageBlue = uint32_t(sumBlue / pixelCount);
    averageAlpha = uint32_t(sumAlpha / pixelCount);

    // Create tags
    taggs.clear();

    // Average color tag
    Tagg taggAvg;
    taggAvg.signature = "GGATCGVA";
    taggAvg.data = {
        static_cast<uint8_t>(averageBlue),
        static_cast<uint8_t>(averageGreen),
        static_cast<uint8_t>(averageRed),
        static_cast<uint8_t>(averageAlpha)
    };
    taggAvg.dataLength = 4;
    taggs.push_back(taggAvg);

    // Max color tag
    Tagg taggMax;
    taggMax.signature = "GGATCXAM";
    taggMax.data = {0xFF, 0xFF, 0xFF, 0xFF};
    taggMax.dataLength = 4;
    taggs.push_back(taggMax);

    hasTransparency = (averageAlpha != 255);
}

const char* PAA::swizzleName(SwizzleType type) {
    switch (type) {
        case SwizzleType::NOHQ: return "nohq";
        case SwizzleType::NO:   return "no";
        case SwizzleType::SMDI: return "smdi";
        case SwizzleType::AS:   return "as";
        case SwizzleType::DT:   return "dt";
        case SwizzleType::NONE: break;
    }
    return "none";
}

PAAFormat PAA::swizzleFormat(SwizzleType type) {
    const SwizzlePreset* preset = swizzlePreset(type);
    return preset ? preset->format : PAAFormat::UNKNOWN;
}

SwizzleType PAA::swizzleFromFilename(const std::string& filename) {
    std::string stem = filename;
    const size_t slash = stem.find_last_of("/\\");
    if (slash != std::string::npos) stem = stem.substr(slash + 1);
    const size_t dot = stem.find_last_of('.');
    if (dot != std::string::npos) stem = stem.substr(0, dot);
    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);

    const std::pair<const char*, SwizzleType> suffixes[] = {
        {"_nohq", SwizzleType::NOHQ},
        {"_no",   SwizzleType::NO},
        {"_smdi", SwizzleType::SMDI},
        {"_as",   SwizzleType::AS},
        {"_dt",   SwizzleType::DT}
    };

    for (const auto& entry : suffixes) {
        const std::string suffix = entry.first;
        if (stem.size() >= suffix.size() &&
            stem.compare(stem.size() - suffix.size(), suffix.size(), suffix) == 0) {
            return entry.second;
        }
    }
    return SwizzleType::NONE;
}

void PAA::writePAA(const std::string& filename, PAAFormat targetFormat) {
    if (mipMaps.size() <= 1) {
        calculateMipmapsAndTaggs();
    }

    const SwizzlePreset* preset = swizzlePreset(swizzle);

    // Determine format. A packed texture's format is fixed by its type, since
    // alpha carries data rather than opacity.
    if (targetFormat != PAAFormat::UNKNOWN) {
        format = targetFormat;
    } else if (preset) {
        format = preset->format;
    } else {
        format = hasTransparency ? PAAFormat::DXT5 : PAAFormat::DXT1;
    }

    taggs.erase(std::remove_if(taggs.begin(), taggs.end(),
                               [](const Tagg& t) {
                                   return t.signature == "GGATZIWS" ||
                                          t.signature == "GGATGALF";
                               }),
                taggs.end());

    if (preset) {
        Tagg taggSwiz;
        taggSwiz.signature = "GGATZIWS";
        taggSwiz.data = preset->bytes;
        taggSwiz.dataLength = static_cast<uint32_t>(preset->bytes.size());
        taggs.push_back(taggSwiz);
    }

    bool writeTransparency = hasTransparency;
    if (preset && preset->flag != FlagPolicy::Auto) {
        writeTransparency = (preset->flag == FlagPolicy::Always);
    }

    if (writeTransparency) {
        Tagg taggFlag;
        taggFlag.signature = "GGATGALF";
        taggFlag.data = {0x01, 0x00, 0x00, 0x00};
        taggFlag.dataLength = 4;
        taggs.push_back(taggFlag);
    }

    // Copy mipmaps for encoding
    std::vector<MipMap> encodedMips = mipMaps;

    const std::vector<uint8_t> transform = swizzleTransformBytes();
    if (!transform.empty()) {
        for (auto& mip : encodedMips) {
            applySwizzle(mip, transform, threadCount);
        }
    }

    // Compress with DXT
    if (format == PAAFormat::DXT5 || format == PAAFormat::DXT1) {
        magicNumber = static_cast<uint16_t>(format);
        for (auto& mip : encodedMips) {
            compressDXT(mip);
        }
    }

    for (auto& mip : encodedMips) {
        if (mip.width > kLzoMinWidth) {
            compressLZO(mip);
        }
    }

    // Calculate offsets tag
    Tagg taggOffs;
    taggOffs.signature = "GGATSFFO";

    uint32_t offset = 2; // magic number

    for (const auto& tagg : taggs) {
        offset += 8 + 4 + tagg.dataLength;
    }

    offset += 8 + 4 + (encodedMips.size() * 4); // OFFSTAGG itself (signature + length + data)
    offset += 2; // palette length

    for (const auto& mip : encodedMips) {
        uint32_t mipOffset = offset;
        taggOffs.data.push_back(mipOffset & 0xFF);
        taggOffs.data.push_back((mipOffset >> 8) & 0xFF);
        taggOffs.data.push_back((mipOffset >> 16) & 0xFF);
        taggOffs.data.push_back((mipOffset >> 24) & 0xFF);

        offset += 2 + 2 + 3 + mip.dataLength;
    }

    taggOffs.dataLength = taggOffs.data.size();

    // Write file
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Failed to open output file: " + filename);
    }

    writeBytes(ofs, magicNumber);

    for (const auto& tagg : taggs) {
        writeString(ofs, tagg.signature);
        writeBytes(ofs, tagg.dataLength);
        writeBytes(ofs, tagg.data);
    }

    writeString(ofs, taggOffs.signature);
    writeBytes(ofs, taggOffs.dataLength);
    writeBytes(ofs, taggOffs.data);

    writeBytes(ofs, palette.dataLength);

    for (const auto& mip : encodedMips) {
        uint16_t width = mip.width;
        if (mip.lzoCompressed) {
            width |= 0x8000;
        }
        writeBytes(ofs, width);
        writeBytes(ofs, mip.height);
        writeBytesAsArmaUShort(ofs, mip.dataLength);
        writeBytes(ofs, mip.data);
    }

    writeBytes<uint16_t>(ofs, 0);
    writeBytes<uint16_t>(ofs, 0);

    ofs.close();
}

void PAA::compressDXT(MipMap& mipmap) {
    const int flags = squishFlags(format, quality);
    const size_t blockBytes = (format == PAAFormat::DXT1) ? 8 : 16;
    const size_t width = mipmap.width;
    const size_t height = mipmap.height;
    const size_t blocksPerRow = (width + 3) / 4;
    const size_t blockRows = (height + 3) / 4;

    std::vector<uint8_t> compressed(blocksPerRow * blockRows * blockBytes);

    // DXT blocks are independent, so bands of 4 rows compress in parallel.
    const size_t rowsPerBand = std::max<size_t>(1, blockRows / 64);
    const size_t bands = (blockRows + rowsPerBand - 1) / rowsPerBand;

    parallelFor(bands, threadCount, [&](size_t band) {
        const size_t firstBlockRow = band * rowsPerBand;
        const size_t y = firstBlockRow * 4;
        const size_t bandHeight = std::min(rowsPerBand * 4, height - y);

        squish::CompressImage(
            mipmap.data.data() + y * width * 4,
            static_cast<int>(width),
            static_cast<int>(bandHeight),
            compressed.data() + firstBlockRow * blocksPerRow * blockBytes,
            flags
        );
    });

    mipmap.dataLength = static_cast<uint32_t>(compressed.size());
    mipmap.data = std::move(compressed);
}

void PAA::decompressDXT1(MipMap& mipmap) {
    size_t uncompressedSize = mipmap.dataLength * 8;
    std::vector<uint8_t> uncompressed(uncompressedSize);

    squish::DecompressImage(
        uncompressed.data(),
        mipmap.width,
        mipmap.height,
        mipmap.data.data(),
        squish::kDxt1
    );

    mipmap.data = uncompressed;
    mipmap.dataLength = uncompressedSize;
}

void PAA::decompressDXT5(MipMap& mipmap) {
    size_t uncompressedSize = mipmap.dataLength * 4;
    std::vector<uint8_t> uncompressed(uncompressedSize);

    squish::DecompressImage(
        uncompressed.data(),
        mipmap.width,
        mipmap.height,
        mipmap.data.data(),
        squish::kDxt5
    );

    mipmap.data = uncompressed;
    mipmap.dataLength = uncompressedSize;
}

void PAA::compressLZO(MipMap& mipmap) {
    ensureLzoInit();

    const size_t srcLen = mipmap.data.size();
    std::vector<uint8_t> work(LZO1X_999_MEM_COMPRESS);
    std::vector<uint8_t> compressed(srcLen + srcLen / 16 + 64 + 3);

    lzo_uint compressedLen = compressed.size();
    if (lzo1x_999_compress(mipmap.data.data(), srcLen,
                           compressed.data(), &compressedLen, work.data()) != LZO_E_OK) {
        throw std::runtime_error("LZO compression failed");
    }

    if (compressedLen >= srcLen) {
        return;
    }

    compressed.resize(compressedLen);
    mipmap.data = std::move(compressed);
    mipmap.dataLength = compressedLen;
    mipmap.lzoCompressed = true;
}

void PAA::decompressLZO(MipMap& mipmap) {
    ensureLzoInit();

    const size_t expected = dxtDataSize(format, mipmap.width, mipmap.height);
    std::vector<uint8_t> decompressed(expected);

    lzo_uint decompressedLen = expected;
    if (lzo1x_decompress_safe(mipmap.data.data(), mipmap.data.size(),
                              decompressed.data(), &decompressedLen, nullptr) != LZO_E_OK) {
        throw std::runtime_error("LZO decompression failed");
    }

    decompressed.resize(decompressedLen);
    mipmap.data = std::move(decompressed);
    mipmap.dataLength = decompressedLen;
    mipmap.lzoCompressed = false;
}

std::vector<uint8_t> PAA::swizzleTransformBytes() const {
    const SwizzlePreset* preset = swizzlePreset(swizzle);
    if (!preset || swizzleMode != SwizzleMode::Apply) {
        return {};
    }

    // BIS authored _dt detail in the source alpha, but exporters put a
    // greyscale in RGB. Read red instead when alpha carries nothing; the
    // stored result is the same either way.
    if (swizzle == SwizzleType::DT && !hasTransparency) {
        return {0x08, 0x01, 0x01, 0x01};
    }

    return preset->bytes;
}

std::vector<uint8_t> PAA::getPackedPixelData(uint8_t level) const {
    if (level >= mipMaps.size()) {
        return {};
    }

    const std::vector<uint8_t> transform = swizzleTransformBytes();
    if (transform.empty()) {
        return mipMaps[level].data;
    }

    MipMap copy = mipMaps[level];
    applySwizzle(copy, transform, threadCount);
    return copy.data;
}

std::vector<uint8_t> PAA::getRawPixelData(uint8_t level) {
    if (level >= mipMaps.size()) {
        return {};
    }
    return mipMaps[level].data;
}

void PAA::setRawPixelData(const std::vector<uint8_t>& data, uint8_t level) {
    if (level < mipMaps.size()) {
        mipMaps[level].data = data;
        mipMaps[level].dataLength = data.size();
    }
}

void PAA::writeImage(const std::string& filename, int mipLevel) {
    if (mipLevel >= mipMaps.size()) {
        throw std::out_of_range("Mipmap level out of range");
    }

    ImageData img;
    img.width = mipMaps[mipLevel].width;
    img.height = mipMaps[mipLevel].height;
    img.data = mipMaps[mipLevel].data;

    ImageLoader::savePNG(filename, img);
}

} // namespace a3tex
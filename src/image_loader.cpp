#include "../include/image_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <tiffio.h>

#include <stdexcept>
#include <algorithm>
#include <fstream>

namespace a3tex {

ImageData ImageLoader::loadPNG(const std::string& filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);

    if (!data) {
        throw std::runtime_error("Failed to load PNG: " + filename);
    }

    ImageData img;
    img.width = width;
    img.height = height;
    img.data = std::vector<uint8_t>(data, data + (width * height * 4));

    stbi_image_free(data);
    return img;
}

ImageData ImageLoader::loadTGA(const std::string& filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);

    if (!data) {
        throw std::runtime_error("Failed to load TGA: " + filename);
    }

    ImageData img;
    img.width = width;
    img.height = height;
    img.data = std::vector<uint8_t>(data, data + (width * height * 4));

    stbi_image_free(data);
    return img;
}

namespace {

ImageData loadTIFFViaRGBA(TIFF* tif, uint32_t width, uint32_t height) {
    std::vector<uint32_t> raster(size_t(width) * height);
    if (!TIFFReadRGBAImageOriented(tif, width, height, raster.data(),
                                   ORIENTATION_TOPLEFT, 0)) {
        throw std::runtime_error("Failed to decode TIFF");
    }

    ImageData img;
    img.width = width;
    img.height = height;
    img.data.resize(raster.size() * 4);

    for (size_t i = 0; i < raster.size(); i++) {
        const uint32_t pixel = raster[i];
        img.data[i * 4 + 0] = static_cast<uint8_t>(TIFFGetR(pixel));
        img.data[i * 4 + 1] = static_cast<uint8_t>(TIFFGetG(pixel));
        img.data[i * 4 + 2] = static_cast<uint8_t>(TIFFGetB(pixel));
        img.data[i * 4 + 3] = static_cast<uint8_t>(TIFFGetA(pixel));
    }
    return img;
}

}

ImageData ImageLoader::loadTIFF(const std::string& filename) {
    TIFF* tif = TIFFOpen(filename.c_str(), "r");
    if (!tif) {
        throw std::runtime_error("Failed to open TIFF: " + filename);
    }

    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t bits = 0;
    uint16_t samples = 0;
    uint16_t planar = PLANARCONFIG_CONTIG;
    uint16_t photometric = PHOTOMETRIC_MINISBLACK;
    uint16_t orientation = ORIENTATION_TOPLEFT;
    uint16_t sampleFormat = SAMPLEFORMAT_UINT;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bits);
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);
    TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planar);
    TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC, &photometric);
    TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &orientation);
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

    if (width == 0 || height == 0) {
        TIFFClose(tif);
        throw std::runtime_error("TIFF has no usable dimensions: " + filename);
    }

    // TIFFReadRGBAImage premultiplies by alpha, which destroys colour wherever
    // alpha is 0. Packed maps carry data in alpha, so decode scanlines directly
    // whenever the layout is one we understand and fall back only for the rest.
    const bool directDecode =
        (bits == 8 || bits == 16) &&
        sampleFormat == SAMPLEFORMAT_UINT &&
        planar == PLANARCONFIG_CONTIG &&
        (photometric == PHOTOMETRIC_MINISBLACK || photometric == PHOTOMETRIC_RGB) &&
        samples >= 1 && samples <= 4 &&
        (orientation == ORIENTATION_TOPLEFT || orientation == ORIENTATION_BOTLEFT) &&
        !TIFFIsTiled(tif);

    if (!directDecode) {
        try {
            ImageData img = loadTIFFViaRGBA(tif, width, height);
            TIFFClose(tif);
            return img;
        } catch (...) {
            TIFFClose(tif);
            throw std::runtime_error("Failed to decode TIFF: " + filename);
        }
    }

    uint16_t extraCount = 0;
    uint16_t* extraTypes = nullptr;
    TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &extraCount, &extraTypes);
    const bool premultiplied = extraCount > 0 && extraTypes != nullptr &&
                               extraTypes[0] == EXTRASAMPLE_ASSOCALPHA;

    const bool hasAlpha = (samples == 2 || samples == 4);
    const bool isGray = (samples <= 2);
    const uint16_t sampleCount = std::min<uint16_t>(samples, 4);

    std::vector<uint8_t> scanline(TIFFScanlineSize(tif));

    ImageData img;
    img.width = width;
    img.height = height;
    img.data.assign(size_t(width) * height * 4, 255);

    for (uint32_t row = 0; row < height; row++) {
        if (TIFFReadScanline(tif, scanline.data(), row) < 0) {
            TIFFClose(tif);
            throw std::runtime_error("Failed to read TIFF scanline: " + filename);
        }

        const uint32_t dstRow = (orientation == ORIENTATION_BOTLEFT) ? height - 1 - row : row;
        uint8_t* dst = img.data.data() + size_t(dstRow) * width * 4;

        for (uint32_t x = 0; x < width; x++) {
            uint8_t value[4] = {0, 0, 0, 255};
            for (uint16_t sample = 0; sample < sampleCount; sample++) {
                const size_t index = size_t(x) * samples + sample;
                value[sample] = (bits == 8)
                    ? scanline[index]
                    : static_cast<uint8_t>(
                          reinterpret_cast<const uint16_t*>(scanline.data())[index] >> 8);
            }

            uint8_t rgba[4];
            if (isGray) {
                rgba[0] = rgba[1] = rgba[2] = value[0];
                rgba[3] = hasAlpha ? value[1] : 255;
            } else {
                rgba[0] = value[0];
                rgba[1] = value[1];
                rgba[2] = value[2];
                rgba[3] = hasAlpha ? value[3] : 255;
            }

            if (premultiplied && rgba[3] > 0 && rgba[3] < 255) {
                for (int c = 0; c < 3; c++) {
                    rgba[c] = static_cast<uint8_t>(
                        std::min(255, (rgba[c] * 255 + rgba[3] / 2) / rgba[3]));
                }
            }

            std::copy(rgba, rgba + 4, dst + size_t(x) * 4);
        }
    }

    TIFFClose(tif);
    return img;
}

ImageData ImageLoader::load(const std::string& filename) {
    if (isTIFF(filename)) {
        return loadTIFF(filename);
    }

    // stb_image auto-detects format
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);

    if (!data) {
        throw std::runtime_error("Failed to load image: " + filename + " - " + stbi_failure_reason());
    }

    ImageData img;
    img.width = width;
    img.height = height;
    img.data = std::vector<uint8_t>(data, data + (width * height * 4));

    stbi_image_free(data);
    return img;
}

void ImageLoader::savePNG(const std::string& filename, const ImageData& image) {
    int result = stbi_write_png(
        filename.c_str(),
        image.width,
        image.height,
        4,
        image.data.data(),
        image.width * 4
    );

    if (!result) {
        throw std::runtime_error("Failed to save PNG: " + filename);
    }
}

bool ImageLoader::isPNG(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of('.'));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png";
}

bool ImageLoader::isTGA(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of('.'));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".tga";
}

bool ImageLoader::isTIFF(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    char magic[4] = {0};
    file.read(magic, 4);
    if (file.gcount() < 4) {
        return false;
    }

    const bool littleEndian = magic[0] == 'I' && magic[1] == 'I' &&
                              magic[2] == 42 && magic[3] == 0;
    const bool bigEndian = magic[0] == 'M' && magic[1] == 'M' &&
                           magic[2] == 0 && magic[3] == 42;
    return littleEndian || bigEndian;
}

} // namespace a3tex
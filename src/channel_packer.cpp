#include "../include/channel_packer.h"

#include <stdexcept>
#include <algorithm>
#include <map>

namespace arma3 {

namespace {

constexpr ChannelMapping kConst(uint8_t value) {
    return ChannelMapping{-1, PackChannel::R, value, false};
}

constexpr ChannelMapping kFrom(int source, PackChannel channel) {
    return ChannelMapping{source, channel, 0, false};
}

const PackPreset kPresets[] = {
    {
        "smdi", SwizzleType::SMDI, 2,
        {"Metallic / specular", "Roughness", nullptr, nullptr},
        {kConst(255), kFrom(0, PackChannel::R), kFrom(1, PackChannel::R), kConst(255)}
    },
    {
        "nohq", SwizzleType::NOHQ, 1,
        {"Normal map", nullptr, nullptr, nullptr},
        {kFrom(0, PackChannel::R), kFrom(0, PackChannel::G),
         kFrom(0, PackChannel::B), kConst(255)}
    },
    {
        "as", SwizzleType::AS, 1,
        {"Ambient occlusion", nullptr, nullptr, nullptr},
        {kFrom(0, PackChannel::R), kFrom(0, PackChannel::R),
         kFrom(0, PackChannel::R), kConst(255)}
    },
    {
        "dt", SwizzleType::DT, 1,
        {"Detail", nullptr, nullptr, nullptr},
        {kFrom(0, PackChannel::R), kFrom(0, PackChannel::R),
         kFrom(0, PackChannel::R), kConst(255)}
    }
};

ImageData resize(const ImageData& src, uint32_t width, uint32_t height) {
    if (src.width == width && src.height == height) {
        return src;
    }

    ImageData out;
    out.width = width;
    out.height = height;
    out.data.resize(size_t(width) * height * 4);

    const double sx = double(src.width) / width;
    const double sy = double(src.height) / height;

    for (uint32_t y = 0; y < height; y++) {
        const double fy = (y + 0.5) * sy - 0.5;
        const int y0 = std::max(0, int(fy));
        const int y1 = std::min<int>(src.height - 1, y0 + 1);
        const double wy = std::max(0.0, fy - y0);

        for (uint32_t x = 0; x < width; x++) {
            const double fx = (x + 0.5) * sx - 0.5;
            const int x0 = std::max(0, int(fx));
            const int x1 = std::min<int>(src.width - 1, x0 + 1);
            const double wx = std::max(0.0, fx - x0);

            for (int c = 0; c < 4; c++) {
                const double a = src.data[(size_t(y0) * src.width + x0) * 4 + c];
                const double b = src.data[(size_t(y0) * src.width + x1) * 4 + c];
                const double d = src.data[(size_t(y1) * src.width + x0) * 4 + c];
                const double e = src.data[(size_t(y1) * src.width + x1) * 4 + c];

                const double top = a + (b - a) * wx;
                const double bottom = d + (e - d) * wx;
                out.data[(size_t(y) * width + x) * 4 + c] =
                    static_cast<uint8_t>(top + (bottom - top) * wy + 0.5);
            }
        }
    }

    return out;
}

} // namespace

ChannelPacker::ChannelPacker(const PackPreset& p) : preset(p) {
    sources.resize(p.sourceCount);
}

ChannelPacker::ChannelPacker()
    : preset{"custom", SwizzleType::NONE, 0,
             {nullptr, nullptr, nullptr, nullptr},
             {kConst(0), kConst(0), kConst(0), kConst(255)}} {}

int ChannelPacker::addSource(const ImageData& image) {
    sources.push_back(image);
    return static_cast<int>(sources.size()) - 1;
}

void ChannelPacker::setSlot(PackChannel output, const ChannelMapping& mapping) {
    preset.slots[static_cast<int>(output)] = mapping;
}

void ChannelPacker::setSource(int index, const ImageData& image) {
    if (index < 0 || index >= int(sources.size())) {
        throw std::out_of_range("Source index out of range");
    }
    sources[index] = image;
}

void ChannelPacker::setSourceChannel(int index, PackChannel channel) {
    for (auto& slot : preset.slots) {
        if (slot.source == index) {
            slot.channel = channel;
        }
    }
}

int ChannelPacker::sourceSlotCount(int index) const {
    int count = 0;
    for (const auto& slot : preset.slots) {
        if (slot.source == index) count++;
    }
    return count;
}

void ChannelPacker::setSourceInvert(int index, bool invert) {
    for (auto& slot : preset.slots) {
        if (slot.source == index) {
            slot.invert = invert;
        }
    }
}

void ChannelPacker::setTargetSize(uint32_t width, uint32_t height) {
    targetWidth = width;
    targetHeight = height;
}

void ChannelPacker::resolveSize(uint32_t& width, uint32_t& height) const {
    if (targetWidth && targetHeight) {
        width = targetWidth;
        height = targetHeight;
        return;
    }

    std::map<std::pair<uint32_t, uint32_t>, int> counts;
    for (const auto& source : sources) {
        if (source.width && source.height) {
            counts[{source.width, source.height}]++;
        }
    }

    if (counts.empty()) {
        throw std::runtime_error("No sources supplied");
    }

    // Majority wins; ties go to the largest, and only an exact area tie between
    // different shapes is genuinely ambiguous.
    auto best = counts.begin();
    for (auto it = std::next(counts.begin()); it != counts.end(); ++it) {
        const size_t area = size_t(it->first.first) * it->first.second;
        const size_t bestArea = size_t(best->first.first) * best->first.second;

        if (it->second > best->second ||
            (it->second == best->second && area > bestArea)) {
            best = it;
        }
    }

    const size_t bestArea = size_t(best->first.first) * best->first.second;
    for (const auto& entry : counts) {
        const size_t area = size_t(entry.first.first) * entry.first.second;
        if (entry.first != best->first && entry.second == best->second && area == bestArea) {
            std::string sizes;
            for (const auto& option : counts) {
                if (!sizes.empty()) sizes += ", ";
                sizes += std::to_string(option.first.first) + "x" +
                         std::to_string(option.first.second);
            }
            throw std::runtime_error(
                "Sources have equally sized but differently shaped resolutions (" +
                sizes + "). Pick one with --resolution WxH.");
        }
    }

    width = best->first.first;
    height = best->first.second;
}

ImageData ChannelPacker::pack() const {
    for (size_t i = 0; i < sources.size(); i++) {
        bool used = false;
        for (const auto& slot : preset.slots) {
            if (slot.source == int(i)) used = true;
        }
        if (used && sources[i].data.empty()) {
            const char* label = preset.sourceLabels[i];
            throw std::runtime_error(
                std::string("Missing source: ") + (label ? label : "unnamed"));
        }
    }

    uint32_t width = 0;
    uint32_t height = 0;
    resolveSize(width, height);

    std::vector<ImageData> scaled;
    scaled.reserve(sources.size());
    for (const auto& source : sources) {
        scaled.push_back(source.data.empty() ? source : resize(source, width, height));
    }

    ImageData out;
    out.width = width;
    out.height = height;
    out.data.resize(size_t(width) * height * 4);

    const size_t pixels = size_t(width) * height;
    for (int c = 0; c < 4; c++) {
        const ChannelMapping& slot = preset.slots[c];

        if (slot.source < 0) {
            const uint8_t value = slot.invert ? uint8_t(255 - slot.constant) : slot.constant;
            for (size_t i = 0; i < pixels; i++) {
                out.data[i * 4 + c] = value;
            }
            continue;
        }

        const ImageData& source = scaled[slot.source];
        const int channel = static_cast<int>(slot.channel);
        for (size_t i = 0; i < pixels; i++) {
            const uint8_t value = source.data[i * 4 + channel];
            out.data[i * 4 + c] = slot.invert ? uint8_t(255 - value) : value;
        }
    }

    return out;
}

const PackPreset* ChannelPacker::findPreset(const std::string& name) {
    for (const auto& preset : kPresets) {
        if (name == preset.name) {
            return &preset;
        }
    }
    return nullptr;
}

std::vector<std::string> ChannelPacker::presetNames() {
    std::vector<std::string> names;
    for (const auto& preset : kPresets) {
        names.push_back(preset.name);
    }
    return names;
}

} // namespace arma3

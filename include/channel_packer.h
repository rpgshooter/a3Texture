#pragma once

#include "image_loader.h"
#include "paa.h"

#include <vector>
#include <string>
#include <cstdint>

namespace arma3 {

enum class PackChannel { R = 0, G = 1, B = 2, A = 3 };

struct ChannelMapping {
    int source = -1;                        // -1 supplies constant
    PackChannel channel = PackChannel::R;
    uint8_t constant = 0;
    bool invert = false;
};

// Assembles the source layout a swizzle expects; PAA::writePAA then applies
// the swizzle itself.
struct PackPreset {
    const char* name;
    SwizzleType swizzle;
    int sourceCount;
    const char* sourceLabels[4];
    ChannelMapping slots[4];
};

class ChannelPacker {
public:
    explicit ChannelPacker(const PackPreset& preset);

    // Free-form: slots are set directly and sources added as needed.
    ChannelPacker();

    int addSource(const ImageData& image);
    void setSlot(PackChannel output, const ChannelMapping& mapping);
    void setSwizzle(SwizzleType type) { preset.swizzle = type; }

    void setSource(int index, const ImageData& image);
    // Collapses every slot fed by this source onto one channel, so it only
    // makes sense where the source feeds a single slot.
    void setSourceChannel(int index, PackChannel channel);
    int sourceSlotCount(int index) const;
    void setSourceInvert(int index, bool invert);
    void setTargetSize(uint32_t width, uint32_t height);
    void setThreadCount(unsigned) {}   // reserved; packing is already cheap

    SwizzleType getSwizzle() const { return preset.swizzle; }
    int getSourceCount() const { return preset.sourceCount; }
    const char* getSourceLabel(int index) const { return preset.sourceLabels[index]; }

    ImageData pack() const;

    static const PackPreset* findPreset(const std::string& name);
    static std::vector<std::string> presetNames();

private:
    void resolveSize(uint32_t& width, uint32_t& height) const;

    PackPreset preset;
    std::vector<ImageData> sources;
    uint32_t targetWidth = 0;
    uint32_t targetHeight = 0;
};

} // namespace arma3

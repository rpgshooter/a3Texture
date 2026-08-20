#pragma once

#include "channel_packer.h"
#include "paa.h"

#include <string>
#include <vector>

namespace arma3 {

// What a source file holds. Exporters either write Arma's own names, in which
// case the file converts as-is, or PBR names that have to be assembled first.
enum class TextureRole {
    Ignore,
    ArmaMap,            // already named _co/_nohq/_smdi/..., convert directly
    BaseColor,
    Normal,             // -> _nohq
    NormalPlain,        // -> _no
    AmbientOcclusion,   // -> _as
    Metallic,           // -> _smdi green
    Roughness,          // -> _smdi blue, inverted, since the engine wants gloss
    Gloss,              // -> _smdi blue directly
    Detail              // -> _dt
};

struct RoleOption {
    TextureRole role;
    const char* label;
};

struct SourceFile {
    std::string path;
    TextureRole role = TextureRole::Ignore;
    bool invert = false;
    std::string baseName;              // filename with the role token removed
    int group = 0;                     // 0 groups by name, otherwise by this number
    SwizzleType armaType = SwizzleType::NONE;  // when role is ArmaMap
};

// Also the unit of work every entry point ends up producing.
struct PlannedOutput {
    std::string name;                  // e.g. hull_smdi.paa
    SwizzleType swizzle = SwizzleType::NONE;
    std::vector<std::string> sources;  // files, in slot order
    ChannelMapping slots[4];
    std::string note;                  // shown when something is assumed
    PAAFormat format = PAAFormat::UNKNOWN;
    SwizzleMode mode = SwizzleMode::Apply;
    uint32_t width = 0;                // 0 resolves from the sources
    uint32_t height = 0;
};

// Fills in role, baseName and invert from the filename.
SourceFile describeSource(const std::string& path);

// Groups sources by base name and works out what each group produces.
std::vector<PlannedOutput> planOutputs(const std::vector<SourceFile>& sources);

std::vector<RoleOption> roleOptions();
const char* roleLabel(TextureRole role);
TextureRole roleFromName(const std::string& name);

} // namespace arma3

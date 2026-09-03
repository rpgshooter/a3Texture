#include "../include/texture_role.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

namespace a3tex {

namespace {

struct Token {
    const char* text;
    TextureRole role;
    SwizzleType arma;
    bool invert;
};

// Longest first, so metallic wins over metal and normalgl over normal.
const Token kTokens[] = {
    {"ambient_occlusion", TextureRole::AmbientOcclusion, SwizzleType::NONE, false},
    {"ambientocclusion", TextureRole::AmbientOcclusion, SwizzleType::NONE, false},
    {"normaldirectx",    TextureRole::Normal,           SwizzleType::NONE, false},
    {"displacement",     TextureRole::Ignore,           SwizzleType::NONE, false},
    {"normalopengl",     TextureRole::Normal,           SwizzleType::NONE, false},
    {"base_color",       TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"basecolour",       TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"glossiness",            TextureRole::Gloss,            SwizzleType::NONE, false},
    {"basecolor",        TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"metalness",        TextureRole::Metallic,         SwizzleType::NONE, false},
    {"normalmap",        TextureRole::Normal,           SwizzleType::NONE, false},
    {"occlusion",        TextureRole::AmbientOcclusion, SwizzleType::NONE, false},
    {"roughness",             TextureRole::Roughness,        SwizzleType::NONE, true},
    {"emissive",         TextureRole::Ignore,           SwizzleType::NONE, false},
    {"metallic",         TextureRole::Metallic,         SwizzleType::NONE, false},
    {"normaldx",         TextureRole::Normal,           SwizzleType::NONE, false},
    {"normalgl",         TextureRole::Normal,           SwizzleType::NONE, false},
    {"specular",         TextureRole::Metallic,         SwizzleType::NONE, false},
    {"diffuse",          TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"opacity",          TextureRole::Ignore,           SwizzleType::NONE, false},
    {"albedo",           TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"colour",           TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"detail",           TextureRole::Detail,           SwizzleType::NONE, false},
    {"height",           TextureRole::Ignore,           SwizzleType::NONE, false},
    {"normal",           TextureRole::Normal,           SwizzleType::NONE, false},
    {"color",            TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"gloss",                 TextureRole::Gloss,            SwizzleType::NONE, false},
    {"metal",            TextureRole::Metallic,         SwizzleType::NONE, false},
    {"rough",                 TextureRole::Roughness,        SwizzleType::NONE, true},
    {"diff",             TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"disp",             TextureRole::Ignore,           SwizzleType::NONE, false},
    {"glos",                  TextureRole::Gloss,            SwizzleType::NONE, false},
    {"norm",             TextureRole::Normal,           SwizzleType::NONE, false},
    {"spec",             TextureRole::Metallic,         SwizzleType::NONE, false},
    {"alb",              TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"col",              TextureRole::BaseColor,        SwizzleType::NONE, false},
    {"mtl",              TextureRole::Metallic,         SwizzleType::NONE, false},
    {"nrm",              TextureRole::Normal,           SwizzleType::NONE, false},
    {"occ",              TextureRole::AmbientOcclusion, SwizzleType::NONE, false},
    {"rgh",                   TextureRole::Roughness,        SwizzleType::NONE, true},
    {"ao",               TextureRole::AmbientOcclusion, SwizzleType::NONE, false},

    // Arma's own names convert straight through.
    {"nohq",             TextureRole::ArmaMap, SwizzleType::NOHQ, false},
    {"smdi",             TextureRole::ArmaMap, SwizzleType::SMDI, false},
    {"as",               TextureRole::ArmaMap, SwizzleType::AS,   false},
    {"ca",               TextureRole::ArmaMap, SwizzleType::NONE, false},
    {"co",               TextureRole::ArmaMap, SwizzleType::NONE, false},
    {"dt",               TextureRole::ArmaMap, SwizzleType::DT,   false},
    {"mc",               TextureRole::ArmaMap, SwizzleType::NONE, false},
    {"no",               TextureRole::ArmaMap, SwizzleType::NO,   false}
};

std::string lowered(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return value;
}

void trimSeparators(std::string& value) {
    while (!value.empty() && (value.back() == '_' || value.back() == '-' ||
                             value.back() == ' ' || value.back() == '.')) {
        value.pop_back();
    }
}

ChannelMapping constant(uint8_t value) {
    ChannelMapping mapping;
    mapping.source = -1;
    mapping.constant = value;
    return mapping;
}

ChannelMapping from(int source, PackChannel channel, bool invert = false) {
    ChannelMapping mapping;
    mapping.source = source;
    mapping.channel = channel;
    mapping.invert = invert;
    return mapping;
}

const SourceFile* find(const std::vector<const SourceFile*>& group, TextureRole role) {
    for (const auto* source : group) {
        if (source->role == role) return source;
    }
    return nullptr;
}

} // namespace

SourceFile describeSource(const std::string& path) {
    SourceFile source;
    source.path = path;

    std::string stem = fs::path(path).stem().string();
    const std::string key = lowered(stem);

    // A trailing token is the reliable case and also tells us the base name.
    for (const auto& token : kTokens) {
        const size_t at = key.rfind(token.text);
        if (at == std::string::npos) continue;
        if (at + std::strlen(token.text) != key.size()) continue;

        source.role = token.role;
        source.armaType = token.arma;
        source.invert = token.invert;
        source.baseName = stem.substr(0, at);
        trimSeparators(source.baseName);

        if (source.baseName.empty()) source.baseName = stem;
        return source;
    }

    // Otherwise take a token from anywhere, which catches names like
    // metal_v2. The base name is unreliable then, so grouping falls to the
    // whole stem and the user can override it.
    for (const auto& token : kTokens) {
        if (token.role == TextureRole::ArmaMap) continue;
        if (key.find(token.text) == std::string::npos) continue;

        source.role = token.role;
        source.invert = token.invert;
        break;
    }

    source.baseName = stem;
    return source;
}

std::vector<PlannedOutput> planOutputs(const std::vector<SourceFile>& sources) {
    std::vector<std::string> order;
    std::map<std::string, std::vector<const SourceFile*>> groups;
    std::map<std::string, std::string> baseFor;

    for (const auto& source : sources) {
        if (source.role == TextureRole::Ignore) continue;

        // A number groups files that do not share a name; otherwise the name does.
        const std::string key = source.group > 0
            ? "#" + std::to_string(source.group)
            : source.baseName;

        if (!groups.count(key)) {
            order.push_back(key);
            baseFor[key] = source.baseName;
        }
        groups[key].push_back(&source);
    }

    std::vector<PlannedOutput> outputs;

    for (const auto& key : order) {
        const auto& group = groups[key];
        const std::string base = lowered(baseFor[key]);

        for (const auto* source : group) {
            if (source->role != TextureRole::ArmaMap) continue;

            PlannedOutput output;
            output.name = lowered(fs::path(source->path).stem().string()) + ".paa";
            output.swizzle = source->armaType;
            output.sources = {source->path};
            output.slots[0] = from(0, PackChannel::R);
            output.slots[1] = from(0, PackChannel::G);
            output.slots[2] = from(0, PackChannel::B);
            output.slots[3] = from(0, PackChannel::A);
            outputs.push_back(std::move(output));
        }

        if (const auto* colour = find(group, TextureRole::BaseColor)) {
            PlannedOutput output;
            output.name = base + "_co.paa";
            output.swizzle = SwizzleType::NONE;
            output.sources = {colour->path};
            output.slots[0] = from(0, PackChannel::R);
            output.slots[1] = from(0, PackChannel::G);
            output.slots[2] = from(0, PackChannel::B);
            output.slots[3] = from(0, PackChannel::A);
            outputs.push_back(std::move(output));
        }

        for (auto role : {TextureRole::Normal, TextureRole::NormalPlain}) {
            const auto* normal = find(group, role);
            if (!normal) continue;

            PlannedOutput output;
            const bool plain = role == TextureRole::NormalPlain;
            output.name = base + (plain ? "_no.paa" : "_nohq.paa");
            output.swizzle = plain ? SwizzleType::NO : SwizzleType::NOHQ;
            output.sources = {normal->path};
            output.slots[0] = from(0, PackChannel::R);
            output.slots[1] = from(0, PackChannel::G);
            output.slots[2] = from(0, PackChannel::B);
            output.slots[3] = constant(255);
            outputs.push_back(std::move(output));
        }

        if (const auto* occlusion = find(group, TextureRole::AmbientOcclusion)) {
            PlannedOutput output;
            output.name = base + "_as.paa";
            output.swizzle = SwizzleType::AS;
            output.sources = {occlusion->path};
            output.slots[0] = from(0, PackChannel::R, occlusion->invert);
            output.slots[1] = from(0, PackChannel::R, occlusion->invert);
            output.slots[2] = from(0, PackChannel::R, occlusion->invert);
            output.slots[3] = constant(255);
            outputs.push_back(std::move(output));
        }

        if (const auto* detail = find(group, TextureRole::Detail)) {
            PlannedOutput output;
            output.name = base + "_dt.paa";
            output.swizzle = SwizzleType::DT;
            output.sources = {detail->path};
            output.slots[0] = from(0, PackChannel::R, detail->invert);
            output.slots[1] = from(0, PackChannel::R, detail->invert);
            output.slots[2] = from(0, PackChannel::R, detail->invert);
            output.slots[3] = constant(255);
            outputs.push_back(std::move(output));
        }

        const auto* metallic = find(group, TextureRole::Metallic);
        const auto* roughness = find(group, TextureRole::Roughness);
        if (!roughness) roughness = find(group, TextureRole::Gloss);

        if (metallic || roughness) {
            PlannedOutput output;
            output.name = base + "_smdi.paa";
            output.swizzle = SwizzleType::SMDI;
            output.slots[0] = constant(255);
            output.slots[3] = constant(255);

            if (metallic) {
                output.sources.push_back(metallic->path);
                output.slots[1] = from(int(output.sources.size()) - 1,
                                       PackChannel::R, metallic->invert);
            } else {
                output.slots[1] = constant(0);
                output.note = "no metallic map, green left at 0";
            }

            if (roughness) {
                output.sources.push_back(roughness->path);
                output.slots[2] = from(int(output.sources.size()) - 1,
                                       PackChannel::R, roughness->invert);
            } else {
                output.slots[2] = constant(255);
                output.note = "no roughness map, blue left at 255";
            }

            outputs.push_back(std::move(output));
        }
    }

    return outputs;
}

std::vector<RoleOption> roleOptions() {
    return {
        {TextureRole::Ignore,           "Ignore"},
        {TextureRole::ArmaMap,          "Already an Arma map"},
        {TextureRole::BaseColor,        "Base color"},
        {TextureRole::Normal,           "Normal (high quality)"},
        {TextureRole::NormalPlain,      "Normal (plain)"},
        {TextureRole::AmbientOcclusion, "Ambient occlusion"},
        {TextureRole::Metallic,         "Metallic / specular"},
        {TextureRole::Gloss,            "Gloss / specular power"},
        {TextureRole::Roughness,        "Roughness (inverted to gloss)"},
        {TextureRole::Detail,           "Detail"}
    };
}

const char* roleLabel(TextureRole role) {
    for (const auto& option : roleOptions()) {
        if (option.role == role) return option.label;
    }
    return "Ignore";
}

TextureRole roleFromName(const std::string& name) {
    const std::string key = lowered(name);
    if (key == "basecolor" || key == "color") return TextureRole::BaseColor;
    if (key == "normal") return TextureRole::Normal;
    if (key == "normalplain" || key == "no") return TextureRole::NormalPlain;
    if (key == "ao" || key == "occlusion") return TextureRole::AmbientOcclusion;
    if (key == "metallic" || key == "specular") return TextureRole::Metallic;
    if (key == "roughness") return TextureRole::Roughness;
    if (key == "gloss" || key == "glossiness") return TextureRole::Gloss;
    if (key == "detail") return TextureRole::Detail;
    if (key == "arma" || key == "packed") return TextureRole::ArmaMap;
    return TextureRole::Ignore;
}

} // namespace a3tex

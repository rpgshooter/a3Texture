#pragma once

#include <array>
#include <vector>
#include <map>
#include <string>

namespace a3tex {

struct RvmatStage {
    std::string texture;               // file path or a procedural like #(argb,8,8,3)color(...)
    std::string uvSource = "tex";
    float uvScale = 1.0f;              // tiling, written into aside and up
};

// Mirrors what retail materials contain, in the order they write it.
struct RvmatMaterial {
    std::array<float, 4> ambient{1, 1, 1, 1};
    std::array<float, 4> diffuse{1, 1, 1, 1};
    std::array<float, 4> forcedDiffuse{0, 0, 0, 0};
    std::array<float, 4> emissive{0, 0, 0, 1};
    std::array<float, 4> specular{0.2f, 0.2f, 0.2f, 1};
    float specularPower = 100.0f;

    std::string pixelShaderID = "Super";
    std::string vertexShaderID = "Super";

    std::string thermalTexture;        // StageTI, written first when set
    std::map<int, RvmatStage> stages;  // Stage1..Stage7
};

std::string writeRvmat(const RvmatMaterial& material);

// Well-known starting points, named as an artist would think of them.
std::vector<std::string> rvmatTemplates();
RvmatMaterial rvmatTemplate(const std::string& name);

// A Super material with every stage on its placeholder, for starting from
// nothing rather than from a texture set.
RvmatMaterial blankMaterial();

// Builds a Super material wired to whichever maps of a set actually exist,
// falling back to the procedural placeholders retail files use.
// The engine wants backslashed paths rooted at the P drive, not filesystem
// ones, so a root to make them relative to is passed in.
RvmatMaterial materialForTextureSet(const std::string& anyTexturePath,
                                    const std::string& driveRoot = {});

std::string enginePath(const std::string& path, const std::string& driveRoot);

} // namespace a3tex

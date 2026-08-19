#include "../include/rvmat_writer.h"
#include "../include/texture_role.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace arma3 {

namespace {

// Retail files carry float32 artefacts like 0.30000001. Write the shortest
// form that reads back the same, which is easier to edit by hand.
std::string number(float value) {
    if (std::fabs(value - std::round(value)) < 1e-6f) {
        return std::to_string(static_cast<long long>(std::lround(value)));
    }

    std::ostringstream out;
    out.precision(8);
    out << value;
    return out.str();
}

std::string vector4(const std::array<float, 4>& values) {
    std::ostringstream out;
    out << "{";
    for (size_t i = 0; i < values.size(); i++) {
        if (i) out << ",";
        out << number(values[i]);
    }
    out << "}";
    return out.str();
}

void writeStage(std::ostringstream& out, const std::string& name, const RvmatStage& stage,
                bool withTransform) {
    out << "class " << name << "\n{\n";
    out << "\ttexture=\"" << stage.texture << "\";\n";

    if (withTransform) {
        out << "\tuvSource=\"" << stage.uvSource << "\";\n";
        if (stage.uvSource != "none") {
            out << "\tclass uvTransform\n\t{\n";
            out << "\t\taside[]={" << number(stage.uvScale) << ",0,0};\n";
            out << "\t\tup[]={0," << number(stage.uvScale) << ",0};\n";
            out << "\t\tdir[]={0,0,0};\n";
            out << "\t\tpos[]={0,0,0};\n";
            out << "\t};\n";
        }
    }
    out << "};\n";
}

} // namespace

std::string enginePath(const std::string& path, const std::string& driveRoot) {
    fs::path relative(path);

    if (!driveRoot.empty()) {
        std::error_code ec;
        const fs::path made = fs::relative(path, driveRoot, ec);
        // native() is wide on Windows, so compare through a narrow string.
        const std::string text = made.generic_string();
        if (!ec && !text.empty() && text.rfind("..", 0) != 0) {
            relative = made;
        }
    } else {
        relative = relative.filename();
    }

    std::string text = relative.generic_string();
    std::replace(text.begin(), text.end(), '/', '\\');
    return text;
}

std::string writeRvmat(const RvmatMaterial& material) {
    std::ostringstream out;

    if (!material.thermalTexture.empty()) {
        RvmatStage thermal;
        thermal.texture = material.thermalTexture;
        writeStage(out, "StageTI", thermal, false);
    }

    out << "ambient[]=" << vector4(material.ambient) << ";\n";
    out << "diffuse[]=" << vector4(material.diffuse) << ";\n";
    out << "forcedDiffuse[]=" << vector4(material.forcedDiffuse) << ";\n";
    // Spelled the way the engine reads it.
    out << "emmisive[]=" << vector4(material.emissive) << ";\n";
    out << "specular[]=" << vector4(material.specular) << ";\n";
    out << "specularPower=" << number(material.specularPower) << ";\n";
    out << "PixelShaderID=\"" << material.pixelShaderID << "\";\n";
    out << "VertexShaderID=\"" << material.vertexShaderID << "\";\n";

    for (const auto& [index, stage] : material.stages) {
        writeStage(out, "Stage" + std::to_string(index), stage, true);
    }

    return out.str();
}

std::vector<std::string> rvmatTemplates() {
    return {"super", "metal", "matte", "glass", "emissive"};
}

RvmatMaterial rvmatTemplate(const std::string& name) {
    RvmatMaterial material;

    if (name == "metal") {
        material.specular = {1, 1, 1, 1};
        material.specularPower = 200.0f;
    } else if (name == "matte") {
        material.specular = {0.1f, 0.1f, 0.1f, 1};
        material.specularPower = 10.0f;
    } else if (name == "glass") {
        material.specular = {1, 1, 1, 1};
        material.specularPower = 500.0f;
        material.pixelShaderID = "Glass";
    } else if (name == "emissive") {
        material.emissive = {1, 1, 1, 1};
        material.specular = {0, 0, 0, 1};
        material.specularPower = 1.0f;
        material.pixelShaderID = "Normal";
        material.vertexShaderID = "Basic";
    }

    return material;
}

RvmatMaterial materialForTextureSet(const std::string& anyTexturePath,
                                    const std::string& driveRoot) {
    RvmatMaterial material = rvmatTemplate("super");

    const fs::path path(anyTexturePath);
    const SourceFile described = describeSource(anyTexturePath);
    const fs::path dir = path.parent_path();

    const auto mapPath = [&](const char* suffix) -> std::string {
        const fs::path candidate = dir / (described.baseName + suffix + ".paa");
        std::error_code ec;
        return fs::exists(candidate, ec) ? enginePath(candidate.string(), driveRoot)
                                         : std::string{};
    };

    // An asset usually names its own thermal texture; the shared one is what
    // retail falls back to.
    material.thermalTexture = mapPath("_ti_ca");
    if (material.thermalTexture.empty()) {
        material.thermalTexture = "a3\\data_f\\default_vehicle_ti_ca.paa";
    }

    // Stages follow the layout retail Super materials use, with the same
    // procedural placeholders where a map is absent.
    RvmatStage normal;
    normal.texture = mapPath("_nohq");
    if (normal.texture.empty()) normal.texture = "#(argb,8,8,3)color(0.5,0.5,1,1)";
    material.stages[1] = normal;

    RvmatStage detail;
    detail.texture = "#(argb,8,8,3)color(0.5,0.5,0.5,1,DT)";
    material.stages[2] = detail;

    RvmatStage macro;
    macro.texture = "#(argb,8,8,3)color(0,0,0,0,MC)";
    material.stages[3] = macro;

    RvmatStage ambientShadow;
    ambientShadow.texture = mapPath("_as");
    if (ambientShadow.texture.empty()) {
        ambientShadow.texture = "#(argb,8,8,3)color(1,1,1,1,AS)";
    }
    material.stages[4] = ambientShadow;

    RvmatStage specular;
    specular.texture = mapPath("_smdi");
    if (specular.texture.empty()) specular.texture = "#(argb,8,8,3)color(0,0,0,0,SMDI)";
    material.stages[5] = specular;

    RvmatStage fresnel;
    fresnel.texture = "#(ai,64,64,1)fresnel(1,0.3)";
    fresnel.uvSource = "none";
    material.stages[6] = fresnel;

    RvmatStage environment;
    environment.texture = "a3\\data_f\\env_co.paa";
    material.stages[7] = environment;

    return material;
}

} // namespace arma3

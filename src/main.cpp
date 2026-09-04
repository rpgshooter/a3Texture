#include "paa.h"
#include "image_loader.h"
#include "channel_packer.h"
#include "texture_role.h"
#include "job_runner.h"
#include "p3d_reader.h"
#include "rvmat_writer.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>

namespace fs = std::filesystem;
//TODO: main.cpp, line 24, should probably rework this stupid std::cout spam
void printUsage(const char* programName) {
    std::cout << "A3Texture \n";
    std::cout << "==========================================\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << programName << " <input> <output> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --format <DXT1|DXT5>    Compression format (default: auto-detect)\n";
    std::cout << "  --quality <fast|normal|high>  Compression quality (default: normal)\n";
    std::cout << "  --jobs <n>              Parallel jobs in batch mode (default: cores)\n";
    std::cout << "\nChannel packing:\n";
    std::cout << "  " << programName << " pack --preset <name> --source <spec> ... <output.paa>\n";
    std::cout << "  --preset <name>         One of: ";
    {
        auto names = a3tex::ChannelPacker::presetNames();
        for (size_t i = 0; i < names.size(); i++) {
            std::cout << (i ? ", " : "") << names[i];
        }
        std::cout << "\n";
    }
    std::cout << "  --source <file[:c][~]>  Source image; :c picks a channel, ~ inverts\n";
    std::cout << "  --resolution <WxH>      Override the majority source resolution\n";
    std::cout << "  --r/--g/--b/--a <spec>  Free-form slot; =N for a constant\n";
    std::cout << "  --no-swizzle            Input is already packed; tag only\n";
    std::cout << "  --batch <pattern>       Batch convert files matching pattern\n";
    std::cout << "  --output-dir <dir>      Output directory for batch mode\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " texture.png texture.paa\n";
    std::cout << "  " << programName << " texture.png texture.paa --format DXT5\n";
    std::cout << "  " << programName << " --batch \"*.png\" --output-dir ./paa/\n";
    std::cout << "  " << programName << " pack --preset smdi --source spec.tif"
              << " --source gloss.tif~ hull_smdi.paa\n";
    std::cout << "  " << programName << " spec material.json\n";
    std::cout << "\nWhole material:\n";
    std::cout << "  " << programName << " plan <files...>            Show what would be produced\n";
    std::cout << "  " << programName << " auto <files...> [--output-dir <dir>]\n";
    std::cout << "\nModels:\n";
    std::cout << "  " << programName << " model <file.p3d>       Inspect a model\n";
    std::cout << "\nMaterials:\n";
    std::cout << "  " << programName << " rvmat <texture> [-o <out.rvmat>] [--template <name>]\n";
    std::cout << "  --root <dir>            P drive root, so texture paths suit the engine\n";
    std::cout << "  --blank                 Start from placeholders, with no texture set\n";
    std::cout << "  --group <n>             Group following files into one texture\n";
    std::cout << "  --role <name>           Force the role of following files\n";
}

a3tex::Quality parseQuality(const std::string& value) {
    if (value == "fast") return a3tex::Quality::Fast;
    if (value == "high") return a3tex::Quality::High;
    return a3tex::Quality::Normal;
}

int runRvmat(int argc, char** argv) {
    std::string source;
    std::string output;
    std::string templateName;
    std::string driveRoot;
    bool blank = false;

    for (int i = 2; i < argc; i++) {
        const std::string arg = argv[i];
        if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--template" && i + 1 < argc) {
            templateName = argv[++i];
        } else if (arg == "--root" && i + 1 < argc) {
            driveRoot = argv[++i];
        } else if (arg == "--blank") {
            blank = true;
        } else if (source.empty()) {
            source = arg;
        }
    }

    if (source.empty() && !blank) {
        std::cerr << "rvmat needs a texture from the set, or --blank\n";
        return 1;
    }

    a3tex::RvmatMaterial material = blank
        ? a3tex::blankMaterial()
        : a3tex::materialForTextureSet(source, driveRoot);

    if (!templateName.empty()) {
        const a3tex::RvmatMaterial preset = a3tex::rvmatTemplate(templateName);
        material.ambient = preset.ambient;
        material.diffuse = preset.diffuse;
        material.forcedDiffuse = preset.forcedDiffuse;
        material.emissive = preset.emissive;
        material.specular = preset.specular;
        material.specularPower = preset.specularPower;
        material.pixelShaderID = preset.pixelShaderID;
        material.vertexShaderID = preset.vertexShaderID;
    }

    const std::string text = a3tex::writeRvmat(material);

    if (output.empty()) {
        std::cout << text;
        return 0;
    }

    std::ofstream file(output, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot write " << output << "\n";
        return 1;
    }
    file << text;
    std::cout << "\u2713 " << output << "\n";
    return 0;
}

int runModel(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "model requires a .p3d file\n";
        return 1;
    }

    const std::string path = argv[2];
    const a3tex::P3DInfo info = a3tex::ReadP3DInfo(path.c_str());

    if (!info.valid) {
        std::cerr << "Could not read " << path << "\n";
        for (const auto& warning : info.warnings) {
            std::cerr << "  " << warning << "\n";
        }
        return 1;
    }

    std::cout << fs::path(path).filename().string() << "\n";
    std::cout << "  format     " << info.type << " v" << info.version << "\n";
    std::cout << "  lods       " << info.lodCount << "\n";
    std::cout << "  vertices   " << info.totalVertices << "\n";
    std::cout << "  faces      " << info.totalFaces << "\n";

    if (!info.lods.empty()) {
        std::cout << "\nLODs:\n";
        for (const auto& lod : info.lods) {
            std::cout << "  " << a3tex::GetLODTypeName(lod.resolution)
                      << "  (" << lod.points.size() << " points, "
                      << lod.faces.size() << " faces)\n";
        }
    }

    if (!info.allTextures.empty()) {
        std::cout << "\nTextures (" << info.allTextures.size() << "):\n";
        for (const auto& texture : info.allTextures) {
            std::cout << "  " << texture << "\n";
        }
    }

    if (!info.allSelections.empty()) {
        std::cout << "\nSelections (" << info.allSelections.size() << "):\n";
        size_t shown = 0;
        for (const auto& selection : info.allSelections) {
            if (shown++ == 20) {
                std::cout << "  ... and " << (info.allSelections.size() - 20) << " more\n";
                break;
            }
            std::cout << "  " << selection << "\n";
        }
    }

    if (!info.namedProperties.empty()) {
        std::cout << "\nProperties:\n";
        for (const auto& property : info.namedProperties) {
            std::cout << "  " << property.key << " = " << property.value << "\n";
        }
    }

    if (!info.warnings.empty()) {
        std::cout << "\nNotes:\n";
        for (const auto& warning : info.warnings) {
            std::cout << "  " << warning << "\n";
        }
    }

    return 0;
}

int runPlan(int argc, char** argv, bool execute) {
    std::vector<a3tex::SourceFile> sources;
    std::string outputDir;
    int currentGroup = 0;
    a3tex::TextureRole currentRole = a3tex::TextureRole::Ignore;
    bool roleSet = false;
    a3tex::Quality quality = a3tex::Quality::Normal;

    for (int i = 2; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--output-dir" && i + 1 < argc) {
            outputDir = argv[++i];
        } else if (arg == "--quality" && i + 1 < argc) {
            quality = parseQuality(argv[++i]);
        } else if (arg == "--group" && i + 1 < argc) {
            currentGroup = std::stoi(argv[++i]);
        } else if (arg == "--role" && i + 1 < argc) {
            currentRole = a3tex::roleFromName(argv[++i]);
            roleSet = true;
        } else {
            a3tex::SourceFile source = a3tex::describeSource(arg);
            source.group = currentGroup;
            if (roleSet) source.role = currentRole;
            sources.push_back(source);
        }
    }

    if (sources.empty()) {
        std::cerr << "Give it some image files\n";
        return 1;
    }

    std::cout << "Inputs:\n";
    for (const auto& source : sources) {
        std::cout << "  " << fs::path(source.path).filename().string()
                  << "  ->  " << a3tex::roleLabel(source.role);
        if (source.invert) std::cout << " (inverted)";
        std::cout << "\n";
    }

    const auto outputs = a3tex::planOutputs(sources);
    if (outputs.empty()) {
        std::cout << "\nNothing to write.\n";
        return 0;
    }

    std::cout << "\nOutputs:\n";
    for (const auto& output : outputs) {
        std::cout << "  " << output.name;
        if (!output.note.empty()) std::cout << "   (" << output.note << ")";
        std::cout << "\n";
        for (const auto& file : output.sources) {
            std::cout << "        <- " << fs::path(file).filename().string() << "\n";
        }
    }

    if (!execute) {
        std::cout << "\nDry run. Use 'auto' to write these.\n";
        return 0;
    }

    std::cout << "\n";

    a3tex::JobOptions options;
    options.outputDir = outputDir;
    options.quality = quality;

    const auto counts = a3tex::runJobs(outputs, options, [](const a3tex::JobResult& r) {
        if (r.success) std::cout << "\u2713 " << r.name << "\n";
        else std::cerr << "\u2717 " << r.name << " - " << r.error << "\n";
    });

    std::cout << "\n" << counts.first << " written, " << counts.second
              << " failed\n";
    return counts.second ? 1 : 0;
}

a3tex::PackChannel channelFromName(const std::string& name) {
    if (name == "g") return a3tex::PackChannel::G;
    if (name == "b") return a3tex::PackChannel::B;
    if (name == "a") return a3tex::PackChannel::A;
    return a3tex::PackChannel::R;
}

struct SlotSpec {
    bool set = false;
    bool constant = false;
    uint8_t value = 0;
    std::string file;
    a3tex::PackChannel channel = a3tex::PackChannel::R;
    bool invert = false;
};

// file[:c][~] or =N
bool parseSlotSpec(const std::string& text, SlotSpec& out) {
    out.set = true;

    if (!text.empty() && text[0] == '=') {
        out.constant = true;
        out.value = static_cast<uint8_t>(std::stoul(text.substr(1)));
        return true;
    }

    std::string spec = text;
    if (!spec.empty() && spec.back() == '~') {
        out.invert = true;
        spec.pop_back();
    }

    const size_t colon = spec.find_last_of(':');
    if (colon != std::string::npos && colon + 2 == spec.size()) {
        switch (spec[colon + 1]) {
            case 'r': out.channel = a3tex::PackChannel::R; break;
            case 'g': out.channel = a3tex::PackChannel::G; break;
            case 'b': out.channel = a3tex::PackChannel::B; break;
            case 'a': out.channel = a3tex::PackChannel::A; break;
            default: return false;
        }
        spec = spec.substr(0, colon);
    }

    out.file = spec;
    return !spec.empty();
}


a3tex::PAAFormat parseFormat(std::string value) {
    std::ranges::transform(value, value.begin(), ::toupper);
    if (value == "DXT1") return a3tex::PAAFormat::DXT1;
    if (value == "DXT5") return a3tex::PAAFormat::DXT5;
    return a3tex::PAAFormat::UNKNOWN;
}

struct SpecEntry {
    std::string output;
    std::string input;
    std::string presetName;
    std::vector<std::string> files;
    std::vector<a3tex::PackChannel> channels;
    std::vector<bool> hasChannel;
    std::vector<bool> inverts;
    uint32_t width = 0;
    uint32_t height = 0;
    a3tex::PAAFormat format = a3tex::PAAFormat::UNKNOWN;
};

int runSpec(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "spec requires a json file\n";
        return 1;
    }

    const fs::path specPath(argv[2]);
    std::ifstream file(specPath);
    if (!file) {
        std::cerr << "Cannot open " << specPath << "\n";
        return 1;
    }

    nlohmann::json doc;
    try {
        file >> doc;
    }
    catch (const std::exception& e) {
        std::cerr << "Invalid JSON in " << specPath.string() << ": " << e.what() << "\n";
        return 1;
    }

    // Paths are relative to the spec, so a material folder stays portable.
    const fs::path base = specPath.has_parent_path() ? specPath.parent_path() : fs::path(".");
    const auto resolve = [&base](const std::string& value) {
        const fs::path path(value);
        return path.is_absolute() ? path : base / path;
    };

    const a3tex::Quality quality =
        parseQuality(doc.value("quality", std::string("normal")));
    const fs::path outputDir = resolve(doc.value("outputDir", std::string(".")));

    if (!doc.contains("textures") || !doc["textures"].is_array()) {
        std::cerr << "Spec needs a \"textures\" array\n";
        return 1;
    }

    std::vector<SpecEntry> entries;
    for (const auto& item : doc["textures"]) {
        SpecEntry entry;
        entry.output = item.value("output", std::string());
        if (entry.output.empty()) {
            std::cerr << "Every texture needs an \"output\"\n";
            return 1;
        }

        entry.input = item.value("input", std::string());
        entry.presetName = item.value("preset", std::string());

        if (entry.input.empty() == entry.presetName.empty()) {
            std::cerr << entry.output << ": needs either \"input\" or \"preset\"\n";
            return 1;
        }

        if (item.contains("format")) {
            entry.format = parseFormat(item["format"].get<std::string>());
        }

        if (item.contains("resolution")) {
            const std::string value = item["resolution"].get<std::string>();
            const size_t x = value.find_first_of("xX");
            if (x != std::string::npos) {
                entry.width = static_cast<uint32_t>(std::stoul(value.substr(0, x)));
                entry.height = static_cast<uint32_t>(std::stoul(value.substr(x + 1)));
            }
        }

        for (const auto& source : item.value("sources", nlohmann::json::array())) {
            if (source.is_string()) {
                entry.files.push_back(source.get<std::string>());
                entry.channels.push_back(a3tex::PackChannel::R);
                entry.hasChannel.push_back(false);
                entry.inverts.push_back(false);
            } else {
                entry.files.push_back(source.value("file", std::string()));
                entry.channels.push_back(
                    channelFromName(source.value("channel", std::string("r"))));
                entry.hasChannel.push_back(source.contains("channel"));
                entry.inverts.push_back(source.value("invert", false));
            }
        }

        entries.push_back(std::move(entry));
    }

    std::vector<a3tex::PlannedOutput> jobs;
    for (const auto& entry : entries) {
        a3tex::PlannedOutput job;
        job.name = entry.output;
        job.format = entry.format;
        job.width = entry.width;
        job.height = entry.height;

        if (!entry.presetName.empty()) {
            const a3tex::PackPreset* preset =
                a3tex::ChannelPacker::findPreset(entry.presetName);
            if (!preset) {
                std::cerr << entry.output << ": unknown preset " << entry.presetName << "\n";
                return 1;
            }
            if (static_cast<int>(entry.files.size()) != preset->sourceCount) {
                std::cerr << entry.output << ": preset " << entry.presetName << " needs "
                          << preset->sourceCount << " source(s)\n";
                return 1;
            }

            job.swizzle = preset->swizzle;
            for (int c = 0; c < 4; c++) job.slots[c] = preset->slots[c];

            for (size_t i = 0; i < entry.files.size(); i++) {
                job.sources.push_back(resolve(entry.files[i]).string());
                for (auto& slot : job.slots) {
                    if (slot.source != static_cast<int>(i)) continue;
                    if (entry.hasChannel[i]) slot.channel = entry.channels[i];
                    if (entry.inverts[i]) slot.invert = true;
                }
            }
        } else {
            job.swizzle = a3tex::PAA::swizzleFromFilename(entry.output);
            job.sources.push_back(resolve(entry.input).string());
            job.slots[0] = {0, a3tex::PackChannel::R, 0, false};
            job.slots[1] = {0, a3tex::PackChannel::G, 0, false};
            job.slots[2] = {0, a3tex::PackChannel::B, 0, false};
            job.slots[3] = {0, a3tex::PackChannel::A, 0, false};
        }

        jobs.push_back(std::move(job));
    }

    std::cout << "Spec: " << jobs.size() << " texture(s)\n";

    a3tex::JobOptions options;
    options.outputDir = outputDir.string();
    options.quality = quality;
    options.jobs = doc.value("jobs", 0u);

    const auto start = std::chrono::high_resolution_clock::now();
    const auto counts = a3tex::runJobs(jobs, options, [](const a3tex::JobResult& r) {
        if (r.success) std::cout << "\u2713 " << r.name << "\n";
        else std::cerr << "\u2717 " << r.name << " - " << r.error << "\n";
    });
    const auto end = std::chrono::high_resolution_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\n" << counts.first << " written, " << counts.second
              << " failed in " << ms.count() << "ms\n";
    return counts.second ? 1 : 0;
}

int runPack(int argc, char** argv) {
    const a3tex::PackPreset* preset = nullptr;
    std::vector<std::string> sourceSpecs;
    SlotSpec slots[4];
    std::string output;
    uint32_t width = 0;
    uint32_t height = 0;
    bool applySwizzle = true;
    a3tex::Quality quality = a3tex::Quality::Normal;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--preset" && i + 1 < argc) {
            const std::string name = argv[++i];
            preset = a3tex::ChannelPacker::findPreset(name);
            if (!preset) {
                std::cerr << "Unknown preset: " << name << "\n";
                return 1;
            }
        }
        else if (arg == "--source" && i + 1 < argc) {
            sourceSpecs.emplace_back(argv[++i]);
        }
        else if (arg == "--resolution" && i + 1 < argc) {
            std::string value = argv[++i];
            const size_t x = value.find_first_of("xX");
            if (x == std::string::npos) {
                std::cerr << "Expected --resolution WxH\n";
                return 1;
            }
            width = static_cast<uint32_t>(std::stoul(value.substr(0, x)));
            height = static_cast<uint32_t>(std::stoul(value.substr(x + 1)));
        }
        else if (arg == "--quality" && i + 1 < argc) {
            quality = parseQuality(argv[++i]);
        }
        else if (arg == "--no-swizzle") {
            applySwizzle = false;
        }
        else if ((arg == "--r" || arg == "--g" || arg == "--b" || arg == "--a") &&
                 i + 1 < argc) {
            const int index = (arg == "--r") ? 0 : (arg == "--g") ? 1 : (arg == "--b") ? 2 : 3;
            if (!parseSlotSpec(argv[++i], slots[index])) {
                std::cerr << "Could not parse " << arg << " " << argv[i] << "\n";
                return 1;
            }
        }
        else if (output.empty()) {
            output = arg;
        }
    }

    const bool freeForm = !preset;
    if (freeForm) {
        bool any = false;
        for (const auto& slot : slots) any = any || slot.set;
        if (!any) {
            std::cerr << "pack requires --preset, or free-form --r/--g/--b/--a slots\n";
            return 1;
        }
    }
    if (output.empty()) {
        std::cerr << "pack requires an output file\n";
        return 1;
    }
    if (freeForm) {
        a3tex::ChannelMapping freeSlots[4];
        std::vector<std::string> ordered;
        std::map<std::string, int> loaded;

        for (int c = 0; c < 4; c++) {
            const SlotSpec& spec = slots[c];
            const auto channel = static_cast<a3tex::PackChannel>(c);

            (void)channel;

            if (!spec.set) {
                freeSlots[c] = {-1, a3tex::PackChannel::R, static_cast<uint8_t>(c == 3 ? 255 : 0), false};
                continue;
            }

            if (spec.constant) {
                freeSlots[c] = {-1, a3tex::PackChannel::R, spec.value, spec.invert};
                continue;
            }

            auto it = loaded.find(spec.file);
            if (it == loaded.end()) {
                ordered.push_back(spec.file);
                it = loaded.emplace(spec.file, static_cast<int>(ordered.size()) - 1).first;
            }
            freeSlots[c] = {it->second, spec.channel, 0, spec.invert};
        }

        a3tex::PlannedOutput job;
        job.name = fs::path(output).filename().string();
        job.swizzle = a3tex::PAA::swizzleFromFilename(output);
        job.width = width;
        job.height = height;
        if (!applySwizzle) job.mode = a3tex::SwizzleMode::TagOnly;

        for (const auto& file : ordered) job.sources.push_back(file);
        for (int c = 0; c < 4; c++) job.slots[c] = freeSlots[c];

        a3tex::JobOptions options;
        options.quality = quality;
        options.outputDir = fs::path(output).parent_path().string();

        const a3tex::JobResult result = a3tex::runJob(job, options);
        if (!result.success) {
            std::cerr << "Error: " << result.error << "\n";
            return 1;
        }
        std::cout << "\u2713 Packed " << output << " (" << result.durationMs << "ms)\n";
        return 0;
    }

    if (static_cast<int>(sourceSpecs.size()) != preset->sourceCount) {
        std::cerr << "Preset " << preset->name << " needs " << preset->sourceCount
                  << " source(s), got " << sourceSpecs.size() << "\n";
        for (int i = 0; i < preset->sourceCount; i++) {
            std::cerr << "  " << (i + 1) << ". " << preset->sourceLabels[i] << "\n";
        }
        return 1;
    }

    a3tex::ChannelPacker packer(*preset);

    for (size_t i = 0; i < sourceSpecs.size(); i++) {
        std::string spec = sourceSpecs[i];

        bool invert = false;
        if (!spec.empty() && spec.back() == '~') {
            invert = true;
            spec.pop_back();
        }

        a3tex::PackChannel channel = a3tex::PackChannel::R;
        bool hasChannel = false;
        const size_t colon = spec.find_last_of(':');
        if (colon != std::string::npos && colon + 2 == spec.size()) {
            hasChannel = true;
            switch (spec[colon + 1]) {
                case 'r': channel = a3tex::PackChannel::R; break;
                case 'g': channel = a3tex::PackChannel::G; break;
                case 'b': channel = a3tex::PackChannel::B; break;
                case 'a': channel = a3tex::PackChannel::A; break;
                default:
                    std::cerr << "Unknown channel in: " << sourceSpecs[i] << "\n";
                    return 1;
            }
            spec = spec.substr(0, colon);
        }

        packer.setSource(static_cast<int>(i), a3tex::ImageLoader::load(spec));
        if (hasChannel) {
            packer.setSourceChannel(static_cast<int>(i), channel);
        }
        packer.setSourceInvert(static_cast<int>(i), invert);

        std::cout << "  " << preset->sourceLabels[i] << ": " << spec
                  << (invert ? " (inverted)" : "") << "\n";
    }

    if (width && height) {
        packer.setTargetSize(width, height);
    }

    auto start = std::chrono::high_resolution_clock::now();

    a3tex::PAA paa;
    paa.setQuality(quality);
    paa.setImage(packer.pack());
    paa.setSwizzle(packer.getSwizzle());
    if (!applySwizzle) {
        paa.setSwizzleMode(a3tex::SwizzleMode::TagOnly);
    }
    paa.writePAA(output);

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\u2713 Packed " << output << " (" << ms.count() << "ms)\n";
    return 0;
}

std::string getOutputFilename(const std::string& input, const std::string& outputDir = "") {
    fs::path inputPath(input);
    std::string outputName = inputPath.stem().string() + ".paa";

    if (!outputDir.empty()) {
        return (fs::path(outputDir) / outputName).string();
    }
    return outputName;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    if (std::string(argv[1]) == "rvmat") {
        try {
            return runRvmat(argc, argv);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    if (std::string(argv[1]) == "model") {
        try {
            return runModel(argc, argv);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    if (std::string(argv[1]) == "plan" || std::string(argv[1]) == "auto") {
        try {
            return runPlan(argc, argv, std::string(argv[1]) == "auto");
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    if (std::string(argv[1]) == "spec") {
        try {
            return runSpec(argc, argv);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    if (std::string(argv[1]) == "pack") {
        try {
            return runPack(argc, argv);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    try {
        std::string input;
        std::string output;
        std::string batchPattern;
        std::string outputDir;
        a3tex::PAAFormat format = a3tex::PAAFormat::UNKNOWN;
        a3tex::Quality quality = a3tex::Quality::Normal;
        unsigned jobs = 0;
        bool batchMode = false;

        // Parse arguments
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--format" && i + 1 < argc) {
                format = parseFormat(argv[++i]);
            }
            else if (arg == "--batch" && i + 1 < argc) {
                batchPattern = argv[++i];
                batchMode = true;
            }
            else if (arg == "--output-dir" && i + 1 < argc) {
                outputDir = argv[++i];
            }
            else if (arg == "--quality" && i + 1 < argc) {
                quality = parseQuality(argv[++i]);
            }
            else if (arg == "--jobs" && i + 1 < argc) {
                jobs = static_cast<unsigned>(std::stoul(argv[++i]));
            }
            else if (arg == "--help" || arg == "-h") {
                printUsage(argv[0]);
                return 0;
            }
            else if (input.empty()) {
                input = arg;
            }
            else if (output.empty()) {
                output = arg;
            }
        }

        if (batchMode) {
            // Batch conversion
            std::cout << "Batch mode: " << batchPattern << "\n";

            if (!outputDir.empty() && !fs::exists(outputDir)) {
                fs::create_directories(outputDir);
            }

            std::vector<std::string> files;
            for (const auto& entry : fs::directory_iterator(".")) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().string();
                    if (filename.find(".png") != std::string::npos ||
                        filename.find(".tga") != std::string::npos) {
                        files.push_back(filename);
                    }
                }
            }

            std::cout << "Found " << files.size() << " files\n";

            std::atomic<int> successCount{0};
            std::atomic<int> failCount{0};
            std::atomic<size_t> nextFile{0};
            std::mutex outputMutex;

            unsigned workers = jobs ? jobs : std::thread::hardware_concurrency();
            if (workers == 0) workers = 1;
            workers = std::min<unsigned>(workers, static_cast<unsigned>(files.size()));

            auto convert = [&] {
                for (size_t i = nextFile++; i < files.size(); i = nextFile++) {
                    const std::string& file = files[i];
                    try {
                        auto start = std::chrono::high_resolution_clock::now();

                        a3tex::PAA paa;
                        paa.setQuality(quality);
                        // Files already run in parallel, so keep each one serial.
                        paa.setThreadCount(1);
                        paa.loadImage(file);

                        std::string outFile = getOutputFilename(file, outputDir);
                        paa.setSwizzle(a3tex::PAA::swizzleFromFilename(outFile));
                        paa.writePAA(outFile, format);

                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                        std::lock_guard<std::mutex> lock(outputMutex);
                        std::cout << "✓ " << file << " → " << outFile
                                  << " (" << duration.count() << "ms)\n";
                        ++successCount;
                    }
                    catch (const std::exception& e) {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        std::cerr << "✗ " << file << " - Error: " << e.what() << "\n";
                        ++failCount;
                    }
                }
            };

            auto batchStart = std::chrono::high_resolution_clock::now();

            if (workers <= 1) {
                convert();
            } else {
                std::vector<std::thread> threads;
                threads.reserve(workers);
                for (unsigned t = 0; t < workers; t++) threads.emplace_back(convert);
                for (auto& thread : threads) thread.join();
            }

            auto batchEnd = std::chrono::high_resolution_clock::now();
            auto batchMs = std::chrono::duration_cast<std::chrono::milliseconds>(batchEnd - batchStart);

            std::cout << "\nBatch complete: " << successCount << " successful, "
                      << failCount << " failed in " << batchMs.count() << "ms"
                      << " (" << workers << " jobs)\n";
        }
        else {
            // Single file conversion
            if (input.empty() || output.empty()) {
                std::cerr << "Error: Both input and output files required\n";
                printUsage(argv[0]);
                return 1;
            }

            std::cout << "Converting: " << input << " → " << output << "\n";

            auto start = std::chrono::high_resolution_clock::now();

            a3tex::PAA paa;
            paa.setQuality(quality);
            paa.loadImage(input);
            paa.setSwizzle(a3tex::PAA::swizzleFromFilename(output));
            paa.writePAA(output, format);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            std::cout << "Conversion complete in " << duration.count() << "ms\n";
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
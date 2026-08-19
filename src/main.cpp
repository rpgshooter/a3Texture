#include "../include/paa.h"
#include "../include/image_loader.h"
#include "../include/channel_packer.h"
#include "../include/texture_role.h"

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

void printUsage(const char* programName) {
    std::cout << "Arma 3 PAA Converter - Native C++ Edition\n";
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
        auto names = arma3::ChannelPacker::presetNames();
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
    std::cout << "  --group <n>             Group following files into one texture\n";
    std::cout << "  --role <name>           Force the role of following files\n";
}

arma3::Quality parseQuality(const std::string& value) {
    if (value == "fast") return arma3::Quality::Fast;
    if (value == "high") return arma3::Quality::High;
    return arma3::Quality::Normal;
}

int runPlan(int argc, char** argv, bool execute) {
    std::vector<arma3::SourceFile> sources;
    std::string outputDir;
    int currentGroup = 0;
    arma3::TextureRole currentRole = arma3::TextureRole::Ignore;
    bool roleSet = false;
    arma3::Quality quality = arma3::Quality::Normal;

    for (int i = 2; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--output-dir" && i + 1 < argc) {
            outputDir = argv[++i];
        } else if (arg == "--quality" && i + 1 < argc) {
            quality = parseQuality(argv[++i]);
        } else if (arg == "--group" && i + 1 < argc) {
            currentGroup = std::stoi(argv[++i]);
        } else if (arg == "--role" && i + 1 < argc) {
            currentRole = arma3::roleFromName(argv[++i]);
            roleSet = true;
        } else {
            arma3::SourceFile source = arma3::describeSource(arg);
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
                  << "  ->  " << arma3::roleLabel(source.role);
        if (source.invert) std::cout << " (inverted)";
        std::cout << "\n";
    }

    const auto outputs = arma3::planOutputs(sources);
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

    if (!outputDir.empty()) {
        std::error_code ec;
        fs::create_directories(outputDir, ec);
    }

    std::cout << "\n";

    std::atomic<size_t> next{0};
    std::atomic<int> okCount{0};
    std::atomic<int> failCount{0};
    std::mutex outputMutex;

    unsigned workers = std::max(1u, std::thread::hardware_concurrency());
    workers = std::min<unsigned>(workers, static_cast<unsigned>(outputs.size()));

    auto worker = [&] {
        for (size_t i = next++; i < outputs.size(); i = next++) {
            const auto& plan = outputs[i];
            const std::string path = outputDir.empty()
                ? plan.name
                : (fs::path(outputDir) / plan.name).string();

            try {
                arma3::ChannelPacker packer;
                packer.setThreadCount(1);
                for (const auto& file : plan.sources) {
                    packer.addSource(arma3::ImageLoader::load(file));
                }
                for (int c = 0; c < 4; c++) {
                    packer.setSlot(static_cast<arma3::PackChannel>(c), plan.slots[c]);
                }

                arma3::PAA paa;
                paa.setQuality(quality);
                paa.setThreadCount(1);
                paa.setImage(packer.pack());
                paa.setSwizzle(plan.swizzle);
                paa.writePAA(path);

                std::lock_guard<std::mutex> lock(outputMutex);
                std::cout << "\u2713 " << plan.name << "\n";
                okCount++;
            }
            catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cerr << "\u2717 " << plan.name << " - " << e.what() << "\n";
                failCount++;
            }
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(workers);
    for (unsigned t = 0; t < workers; t++) pool.emplace_back(worker);
    for (auto& thread : pool) thread.join();

    std::cout << "\n" << okCount.load() << " written, " << failCount.load() << " failed\n";
    return failCount.load() ? 1 : 0;
}

arma3::PackChannel channelFromName(const std::string& name) {
    if (name == "g") return arma3::PackChannel::G;
    if (name == "b") return arma3::PackChannel::B;
    if (name == "a") return arma3::PackChannel::A;
    return arma3::PackChannel::R;
}

struct SlotSpec {
    bool set = false;
    bool constant = false;
    uint8_t value = 0;
    std::string file;
    arma3::PackChannel channel = arma3::PackChannel::R;
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
            case 'r': out.channel = arma3::PackChannel::R; break;
            case 'g': out.channel = arma3::PackChannel::G; break;
            case 'b': out.channel = arma3::PackChannel::B; break;
            case 'a': out.channel = arma3::PackChannel::A; break;
            default: return false;
        }
        spec = spec.substr(0, colon);
    }

    out.file = spec;
    return !spec.empty();
}


arma3::PAAFormat parseFormat(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), ::toupper);
    if (value == "DXT1") return arma3::PAAFormat::DXT1;
    if (value == "DXT5") return arma3::PAAFormat::DXT5;
    return arma3::PAAFormat::UNKNOWN;
}

struct SpecEntry {
    std::string output;
    std::string input;
    std::string presetName;
    std::vector<std::string> files;
    std::vector<arma3::PackChannel> channels;
    std::vector<bool> hasChannel;
    std::vector<bool> inverts;
    uint32_t width = 0;
    uint32_t height = 0;
    arma3::PAAFormat format = arma3::PAAFormat::UNKNOWN;
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

    const arma3::Quality quality =
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
                entry.channels.push_back(arma3::PackChannel::R);
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

    std::error_code ec;
    fs::create_directories(outputDir, ec);

    std::cout << "Spec: " << entries.size() << " texture(s)\n";

    std::atomic<size_t> next{0};
    std::atomic<int> okCount{0};
    std::atomic<int> failCount{0};
    std::mutex outputMutex;

    unsigned workers = doc.value("jobs", 0u);
    if (workers == 0) workers = std::thread::hardware_concurrency();
    if (workers == 0) workers = 1;
    workers = std::min<unsigned>(workers, static_cast<unsigned>(entries.size()));

    const auto start = std::chrono::high_resolution_clock::now();

    auto worker = [&] {
        for (size_t i = next++; i < entries.size(); i = next++) {
            const SpecEntry& entry = entries[i];
            const std::string outPath = (outputDir / entry.output).string();

            try {
                arma3::PAA paa;
                paa.setQuality(quality);
                paa.setThreadCount(1);

                if (!entry.presetName.empty()) {
                    const arma3::PackPreset* preset =
                        arma3::ChannelPacker::findPreset(entry.presetName);
                    if (!preset) {
                        throw std::runtime_error("unknown preset " + entry.presetName);
                    }
                    if (int(entry.files.size()) != preset->sourceCount) {
                        throw std::runtime_error(
                            "preset " + entry.presetName + " needs " +
                            std::to_string(preset->sourceCount) + " source(s)");
                    }

                    arma3::ChannelPacker packer(*preset);
                    for (size_t sourceIndex = 0; sourceIndex < entry.files.size(); sourceIndex++) {
                        packer.setSource(int(sourceIndex),
                                         arma3::ImageLoader::load(
                                             resolve(entry.files[sourceIndex]).string()));
                        if (entry.hasChannel[sourceIndex]) {
                            packer.setSourceChannel(int(sourceIndex),
                                                    entry.channels[sourceIndex]);
                        }
                        packer.setSourceInvert(int(sourceIndex), entry.inverts[sourceIndex]);
                    }
                    if (entry.width && entry.height) {
                        packer.setTargetSize(entry.width, entry.height);
                    }

                    paa.setImage(packer.pack());
                    paa.setSwizzle(preset->swizzle);
                } else {
                    paa.loadImage(resolve(entry.input).string());
                    paa.setSwizzle(arma3::PAA::swizzleFromFilename(entry.output));
                }

                paa.writePAA(outPath, entry.format);

                std::lock_guard<std::mutex> lock(outputMutex);
                std::cout << "\u2713 " << entry.output << "\n";
                okCount++;
            }
            catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cerr << "\u2717 " << entry.output << " - " << e.what() << "\n";
                failCount++;
            }
        }
    };

    if (workers <= 1) {
        worker();
    } else {
        std::vector<std::thread> pool;
        pool.reserve(workers);
        for (unsigned t = 0; t < workers; t++) pool.emplace_back(worker);
        for (auto& thread : pool) thread.join();
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\n" << okCount.load() << " written, " << failCount.load()
              << " failed in " << ms.count() << "ms\n";
    return failCount.load() ? 1 : 0;
}

int runPack(int argc, char** argv) {
    const arma3::PackPreset* preset = nullptr;
    std::vector<std::string> sourceSpecs;
    SlotSpec slots[4];
    std::string output;
    uint32_t width = 0;
    uint32_t height = 0;
    bool applySwizzle = true;
    arma3::Quality quality = arma3::Quality::Normal;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--preset" && i + 1 < argc) {
            const std::string name = argv[++i];
            preset = arma3::ChannelPacker::findPreset(name);
            if (!preset) {
                std::cerr << "Unknown preset: " << name << "\n";
                return 1;
            }
        }
        else if (arg == "--source" && i + 1 < argc) {
            sourceSpecs.push_back(argv[++i]);
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
        arma3::ChannelPacker packer;
        std::map<std::string, int> loaded;

        for (int c = 0; c < 4; c++) {
            const SlotSpec& spec = slots[c];
            const auto channel = static_cast<arma3::PackChannel>(c);

            if (!spec.set) {
                packer.setSlot(channel, arma3::ChannelMapping{-1, arma3::PackChannel::R,
                                                              uint8_t(c == 3 ? 255 : 0), false});
                continue;
            }

            if (spec.constant) {
                packer.setSlot(channel, arma3::ChannelMapping{-1, arma3::PackChannel::R,
                                                              spec.value, spec.invert});
                continue;
            }

            auto it = loaded.find(spec.file);
            if (it == loaded.end()) {
                it = loaded.emplace(spec.file,
                                    packer.addSource(arma3::ImageLoader::load(spec.file))).first;
            }
            packer.setSlot(channel, arma3::ChannelMapping{it->second, spec.channel,
                                                          0, spec.invert});
        }

        if (width && height) {
            packer.setTargetSize(width, height);
        }

        const arma3::SwizzleType swizzle = arma3::PAA::swizzleFromFilename(output);

        auto start = std::chrono::high_resolution_clock::now();

        arma3::PAA paa;
        paa.setQuality(quality);
        paa.setImage(packer.pack());
        paa.setSwizzle(swizzle);
        if (!applySwizzle) {
            paa.setSwizzleMode(arma3::SwizzleMode::TagOnly);
        }
        paa.writePAA(output);

        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "\u2713 Packed " << output << " (" << ms.count() << "ms)\n";
        return 0;
    }

    if (int(sourceSpecs.size()) != preset->sourceCount) {
        std::cerr << "Preset " << preset->name << " needs " << preset->sourceCount
                  << " source(s), got " << sourceSpecs.size() << "\n";
        for (int i = 0; i < preset->sourceCount; i++) {
            std::cerr << "  " << (i + 1) << ". " << preset->sourceLabels[i] << "\n";
        }
        return 1;
    }

    arma3::ChannelPacker packer(*preset);

    for (size_t i = 0; i < sourceSpecs.size(); i++) {
        std::string spec = sourceSpecs[i];

        bool invert = false;
        if (!spec.empty() && spec.back() == '~') {
            invert = true;
            spec.pop_back();
        }

        arma3::PackChannel channel = arma3::PackChannel::R;
        bool hasChannel = false;
        const size_t colon = spec.find_last_of(':');
        if (colon != std::string::npos && colon + 2 == spec.size()) {
            hasChannel = true;
            switch (spec[colon + 1]) {
                case 'r': channel = arma3::PackChannel::R; break;
                case 'g': channel = arma3::PackChannel::G; break;
                case 'b': channel = arma3::PackChannel::B; break;
                case 'a': channel = arma3::PackChannel::A; break;
                default:
                    std::cerr << "Unknown channel in: " << sourceSpecs[i] << "\n";
                    return 1;
            }
            spec = spec.substr(0, colon);
        }

        packer.setSource(static_cast<int>(i), arma3::ImageLoader::load(spec));
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

    arma3::PAA paa;
    paa.setQuality(quality);
    paa.setImage(packer.pack());
    paa.setSwizzle(packer.getSwizzle());
    if (!applySwizzle) {
        paa.setSwizzleMode(arma3::SwizzleMode::TagOnly);
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
        arma3::PAAFormat format = arma3::PAAFormat::UNKNOWN;
        arma3::Quality quality = arma3::Quality::Normal;
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

                        arma3::PAA paa;
                        paa.setQuality(quality);
                        // Files already run in parallel, so keep each one serial.
                        paa.setThreadCount(1);
                        paa.loadImage(file);

                        std::string outFile = getOutputFilename(file, outputDir);
                        paa.setSwizzle(arma3::PAA::swizzleFromFilename(outFile));
                        paa.writePAA(outFile, format);

                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                        std::lock_guard<std::mutex> lock(outputMutex);
                        std::cout << "✓ " << file << " → " << outFile
                                  << " (" << duration.count() << "ms)\n";
                        successCount++;
                    }
                    catch (const std::exception& e) {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        std::cerr << "✗ " << file << " - Error: " << e.what() << "\n";
                        failCount++;
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

            arma3::PAA paa;
            paa.setQuality(quality);
            paa.loadImage(input);
            paa.setSwizzle(arma3::PAA::swizzleFromFilename(output));
            paa.writePAA(output, format);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            std::cout << "✓ Conversion complete in " << duration.count() << "ms\n";
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
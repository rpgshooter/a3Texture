#include "../include/paa.h"
#include "../include/image_loader.h"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

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
    std::cout << "  --batch <pattern>       Batch convert files matching pattern\n";
    std::cout << "  --output-dir <dir>      Output directory for batch mode\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " texture.png texture.paa\n";
    std::cout << "  " << programName << " texture.png texture.paa --format DXT5\n";
    std::cout << "  " << programName << " --batch \"*.png\" --output-dir ./paa/\n";
}

std::string getOutputFilename(const std::string& input, const std::string& outputDir = "") {
    fs::path inputPath(input);
    std::string outputName = inputPath.stem().string() + ".paa";

    if (!outputDir.empty()) {
        return (fs::path(outputDir) / outputName).string();
    }
    return outputName;
}

arma3::Quality parseQuality(const std::string& value) {
    if (value == "fast") return arma3::Quality::Fast;
    if (value == "high") return arma3::Quality::High;
    return arma3::Quality::Normal;
}

arma3::PAAFormat parseFormat(const std::string& formatStr) {
    if (formatStr == "DXT1") return arma3::PAAFormat::DXT1;
    if (formatStr == "DXT5") return arma3::PAAFormat::DXT5;
    return arma3::PAAFormat::UNKNOWN;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
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
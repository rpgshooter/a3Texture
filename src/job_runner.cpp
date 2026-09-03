#include "job_runner.h"
#include "channel_packer.h"
#include "image_loader.h"
#include "paa.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <thread>

namespace fs = std::filesystem;

namespace a3tex {

JobResult runJob(const PlannedOutput& job, const JobOptions& options) {
    JobResult result;
    result.name = job.name;

    const auto start = std::chrono::high_resolution_clock::now();

    try {
        if (job.sources.empty()) {
            throw std::runtime_error("no sources");
        }

        ChannelPacker packer;
        for (const auto& file : job.sources) {
            packer.addSource(ImageLoader::load(file));
        }
        for (int c = 0; c < 4; c++) {
            packer.setSlot(static_cast<PackChannel>(c), job.slots[c]);
        }
        if (job.width && job.height) {
            packer.setTargetSize(job.width, job.height);
        }

        ImageData packed = packer.pack();
        result.width = packed.width;
        result.height = packed.height;

        const std::string dir = options.outputDir.empty()
            ? fs::path(job.sources.front()).parent_path().string()
            : options.outputDir;

        PAA paa;
        paa.setQuality(options.quality);
        paa.setThreadCount(1);
        paa.setImage(packed);
        paa.setSwizzle(job.swizzle);
        paa.setSwizzleMode(job.mode);
        paa.writePAA((fs::path(dir) / job.name).string(), job.format);

        result.success = true;
    }
    catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }

    const auto end = std::chrono::high_resolution_clock::now();
    result.durationMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return result;
}

std::pair<int, int> runJobs(const std::vector<PlannedOutput>& jobs,
                            const JobOptions& options,
                            const std::function<void(const JobResult&)>& onResult) {
    if (jobs.empty()) return {0, 0};

    if (!options.outputDir.empty()) {
        std::error_code ec;
        fs::create_directories(options.outputDir, ec);
    }

    std::atomic<size_t> next{0};
    std::atomic<int> okCount{0};
    std::atomic<int> failCount{0};
    std::mutex reportMutex;

    unsigned workers = options.jobs ? options.jobs : std::thread::hardware_concurrency();
    if (workers == 0) workers = 1;
    workers = std::min<unsigned>(workers, static_cast<unsigned>(jobs.size()));

    auto worker = [&] {
        for (size_t i = next++; i < jobs.size(); i = next++) {
            const JobResult result = runJob(jobs[i], options);

            if (result.success) okCount++; else failCount++;

            if (onResult) {
                std::lock_guard<std::mutex> lock(reportMutex);
                onResult(result);
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

    return {okCount.load(), failCount.load()};
}

} // namespace a3tex

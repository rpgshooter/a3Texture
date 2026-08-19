#pragma once

#include "texture_role.h"

#include <functional>
#include <string>
#include <vector>

namespace arma3 {

struct JobOptions {
    std::string outputDir;             // empty writes beside the first source
    Quality quality = Quality::Normal;
    unsigned jobs = 0;                 // 0 uses all cores
};

struct JobResult {
    std::string name;
    bool success = false;
    std::string error;
    uint32_t width = 0;
    uint32_t height = 0;
    int64_t durationMs = 0;
};

// Assembles, swizzles and writes one texture. Shared by every entry point so
// they cannot drift apart.
JobResult runJob(const PlannedOutput& job, const JobOptions& options);

// Runs jobs across cores, reporting each as it finishes.
std::pair<int, int> runJobs(const std::vector<PlannedOutput>& jobs,
                            const JobOptions& options,
                            const std::function<void(const JobResult&)>& onResult);

} // namespace arma3

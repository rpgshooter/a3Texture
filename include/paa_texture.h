#pragma once

// Adapts the renderer's texture loading onto this project's PAA reader.

#include "paa.h"

#include <glad/glad.h>

#include <cstdio>
#include <string>
#include <vector>

namespace arma3 {

struct PAATexture {
    bool valid = false;
    uint32_t width = 0;
    uint32_t height = 0;
    PAAFormat type = PAAFormat::UNKNOWN;
    GLuint textureId = 0;
    std::vector<uint8_t> pixels;   // RGBA of the largest mip
};

struct PAALoader {
    static PAATexture Load(const std::string& path);
    static bool Upload(PAATexture& texture);
    static const char* GetTypeName(PAAFormat format);
};

} // namespace arma3

// The renderer logs through the host application's console; here it goes to
// stderr, which keeps shader and texture failures visible.
#define LOG_ERROR(msg)   fprintf(stderr, "[renderer] %s\n", std::string(msg).c_str())
#define LOG_WARNING(msg) fprintf(stderr, "[renderer] %s\n", std::string(msg).c_str())
#define LOG_INFO(msg)    ((void)0)
#define LOG_DEBUG(msg)   ((void)0)

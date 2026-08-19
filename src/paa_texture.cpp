#include "../include/paa_texture.h"

namespace arma3 {

PAATexture PAALoader::Load(const std::string& path) {
    PAATexture texture;

    try {
        PAA paa(path);
        paa.readPAA();

        if (paa.getMipMaps().empty()) {
            return texture;
        }

        const MipMap& mip = paa.getMipMaps().front();
        if (mip.data.size() < size_t(mip.width) * mip.height * 4) {
            return texture;
        }

        texture.width = mip.width;
        texture.height = mip.height;
        texture.type = paa.getFormat();
        texture.pixels = mip.data;
        texture.valid = true;
    }
    catch (const std::exception&) {
        texture.valid = false;
    }

    return texture;
}

bool PAALoader::Upload(PAATexture& texture) {
    if (!texture.valid || texture.pixels.empty()) {
        return false;
    }

    glGenTextures(1, &texture.textureId);
    glBindTexture(GL_TEXTURE_2D, texture.textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 GLsizei(texture.width), GLsizei(texture.height), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, texture.pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture.textureId != 0;
}

const char* PAALoader::GetTypeName(PAAFormat format) {
    switch (format) {
        case PAAFormat::DXT1: return "DXT1";
        case PAAFormat::DXT5: return "DXT5";
        default: return "unknown";
    }
}

} // namespace arma3

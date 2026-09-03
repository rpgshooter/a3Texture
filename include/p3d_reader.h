#pragma once

#include "binary_reader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>


namespace arma3 {
//=============================================================================
// P3D Structures
//=============================================================================

struct P3DPoint {
	float x, y, z;
	uint32_t flags;
};

struct P3DFaceVertex {
	uint32_t pointIndex;
	uint32_t normalIndex;
	float u, v;
};

struct P3DFace {
	std::string texture;
	std::string material;
	uint32_t type;	// 3=triangle, 4=quad
	P3DFaceVertex verts[4];
	uint32_t flags;
};

struct P3DSelection {
	std::string name;
	std::vector<uint8_t> pointWeights;
	std::vector<uint8_t> faceWeights;
};

struct P3DUV {
	float u, v;
};

struct P3DLOD {
	std::string signature;	// "P3DM" or "SP3X"
	uint32_t majorVersion;
	uint32_t minorVersion;
	float resolution;

	std::vector<P3DPoint> points;
	std::vector<float> normals;
	std::vector<P3DUV> uvs;	 // Per-vertex UV coordinates
	std::vector<P3DFace> faces;
	std::vector<P3DSelection> selections;
	std::vector<std::string> namedProperties;
};

// Named property (key=value pair)
struct P3DNamedProperty {
	std::string key;
	std::string value;
};

struct P3DInfo {
	std::vector<std::string> warnings;
	bool valid = false;
	std::string path;
	std::string type;  // "MLOD" or "ODOL"
	uint32_t version = 0;
	uint32_t lodCount = 0;
	std::vector<P3DLOD> lods;
	float boundingBox[6] = {0};

	std::vector<std::string> allSelections;
	std::vector<std::string> allTextures;
	std::vector<std::string> proxySelections;
	std::vector<P3DNamedProperty> namedProperties;	// All named properties from all LODs
	int totalVertices = 0;
	int totalFaces = 0;
};

//=============================================================================
// LOD Type Names
//=============================================================================

inline std::string GetLODTypeName(float resolution) {
	// Visual LODs (0-999)
	if (resolution < 1000.0f) {
		if (resolution < 1.0f)
			return "Visual";
		int lodNum = (int)resolution;
		char buf[32];
		snprintf(buf, 32, "Visual %d", lodNum);
		return buf;
	}

	// Special LODs by resolution value
	if (resolution >= 1000.0f && resolution < 1100.0f)
		return "View Geo";
	if (resolution >= 1100.0f && resolution < 1200.0f)
		return "Fire Geo";
	if (resolution >= 1200.0f && resolution < 2000.0f)
		return "View Cargo";
	if (resolution >= 2000.0f && resolution < 10000.0f)
		return "View Pilot";

	// Shadow volumes
	if (resolution >= 10000.0f && resolution < 11000.0f)
		return "Shadow 0";
	if (resolution >= 11000.0f && resolution < 12000.0f)
		return "Shadow 10";

	// Very high resolution values (special LODs)
	if (resolution > 1e12f && resolution < 2e13f)
		return "Geometry";
	if (resolution > 1e14f && resolution < 2e15f)
		return "Memory";
	if (resolution > 2e14f && resolution < 3e15f)
		return "LandContact";
	if (resolution > 3e14f && resolution < 4e15f)
		return "Roadway";
	if (resolution > 4e14f && resolution < 5e15f)
		return "Paths";
	if (resolution > 5e14f && resolution < 6e15f)
		return "HitPoints";
	if (resolution > 6e14f && resolution < 7e15f)
		return "View Geo Pilot";
	if (resolution > 7e14f && resolution < 8e15f)
		return "Fire Geo Pilot";

	// Fallback - show resolution
	if (resolution > 1e10f) {
		return "Special";
	}

	char buf[32];
	snprintf(buf, 32, "LOD %.0f", resolution);
	return buf;
}

//=============================================================================
// Inline Helper Functions (kept in header for use by other headers)
//=============================================================================

inline std::string ReadAsciiz(std::ifstream& f) {
	std::string result;
	char c;
	while (f.get(c) && c != '\0') {
		result += c;
	}
	return result;
}

inline std::vector<uint8_t> DecompressLZSS(const uint8_t* data, size_t packedSize, size_t unpackedSize) {
	std::vector<uint8_t> output;
	output.reserve(unpackedSize);

	const int RING_SIZE = 4096;
	const int THRESHOLD = 2;
	uint8_t ring[RING_SIZE];
	memset(ring, 0x20, RING_SIZE);
	int ringPos = RING_SIZE - 18;

	size_t srcPos = 0;

	while (output.size() < unpackedSize && srcPos < packedSize) {
		uint8_t flags = data[srcPos++];

		for (int bit = 0; bit < 8 && output.size() < unpackedSize && srcPos < packedSize; bit++) {
			if (flags & (1 << bit)) {
				uint8_t c = data[srcPos++];
				output.push_back(c);
				ring[ringPos] = c;
				ringPos = (ringPos + 1) % RING_SIZE;
			} else {
				if (srcPos + 1 >= packedSize)
					break;

				uint8_t lo = data[srcPos++];
				uint8_t hi = data[srcPos++];

				int offset = lo | ((hi & 0xF0) << 4);
				int length = (hi & 0x0F) + THRESHOLD + 1;

				for (int i = 0; i < length && output.size() < unpackedSize; i++) {
					uint8_t c = ring[(offset + i) % RING_SIZE];
					output.push_back(c);
					ring[ringPos] = c;
					ringPos = (ringPos + 1) % RING_SIZE;
				}
			}
		}
	}

	return output;
}

inline std::vector<uint8_t> ReadODOLCompressedBlock(std::ifstream& file, [[maybe_unused]] const char* blockName = "block") {
	uint32_t packedSize, unpackedSize;
	file.read(reinterpret_cast<char*>(&packedSize), 4);
	file.read(reinterpret_cast<char*>(&unpackedSize), 4);

	if (packedSize == 0 || packedSize > 50000000) {
		return {};
	}

	std::vector<uint8_t> packed(packedSize);
	file.read(reinterpret_cast<char*>(packed.data()), packedSize);

	if (packedSize != unpackedSize) {
		return DecompressLZSS(packed.data(), packedSize, unpackedSize);
	}
	return packed;
}

inline void ExtractPathsFromData(const std::vector<uint8_t>& data, std::vector<std::string>& paths,
								 const std::string& extension) {
	std::string dataStr(data.begin(), data.end());
	size_t pos = 0;

	while ((pos = dataStr.find(extension, pos)) != std::string::npos) {
		size_t start = pos;
		while (start > 0 && dataStr[start - 1] != '\0' && dataStr[start - 1] >= 32 && dataStr[start - 1] < 127) {
			start--;
		}

		if (pos > start) {
			std::string path = dataStr.substr(start, pos - start + extension.length());
			if ((path.find('\\') != std::string::npos || path.find('/') != std::string::npos) &&
				std::find(paths.begin(), paths.end(), path) == paths.end()) {
				paths.push_back(path);
			}
		}
		pos += extension.length();
	}
}

inline size_t ReadAsciizFromBuffer(const uint8_t* data, size_t offset, size_t maxSize, std::string& out) {
	out.clear();
	while (offset < maxSize && data[offset] != 0) {
		out += (char)data[offset];
		offset++;
	}
	return offset + 1;	// Skip null terminator
}

//=============================================================================
// P3D Reading Functions (implemented in p3d_reader.cpp)
//=============================================================================

void ParseODOLLodGeometry(const std::vector<uint8_t>& data, P3DLOD& lod, P3DInfo& info);
P3DInfo ReadP3DInfo(const char* path);

} // namespace arma3

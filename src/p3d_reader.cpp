#include "../include/p3d_reader.h"

#include <cstring>
#include <fstream>
#include <algorithm>


namespace a3tex {
//=============================================================================
// ODOL LOD Geometry Parser
//=============================================================================

void ParseODOLLodGeometry(const std::vector<uint8_t>& data, P3DLOD& lod, P3DInfo& info) {
	if (data.size() < 20) {
		return;
	}

	BinaryReader reader(data);

	// ODOL LOD structure (v40+):
	// 1. Proxies
	uint32_t proxyCount = reader.readOr<uint32_t>(0);
	if (proxyCount > 1000)
		return;
	for (uint32_t i = 0; i < proxyCount; i++) {
		reader.skipAsciiz();  // proxy model path
		reader.skip(48);	  // 12 floats transform
		reader.skip(4);		  // sequence ID
		reader.skip(4);		  // named selection ID
		reader.skip(4);		  // bone ID
		if (reader.eof())
			return;
	}

	// 2. Bones/Items
	uint32_t boneCount = reader.readOr<uint32_t>(0);
	if (boneCount > 500)
		return;
	for (uint32_t i = 0; i < boneCount; i++) {
		reader.skipAsciiz();  // bone name
		if (reader.eof())
			return;
	}

	// 3. Sections
	uint32_t sectionCount = reader.readOr<uint32_t>(0);
	if (sectionCount > 200)
		return;
	for (uint32_t i = 0; i < sectionCount; i++) {
		reader.skip(4);	 // faceLowerIndex
		reader.skip(4);	 // faceUpperIndex
		reader.skip(4);	 // minBoneIndex
		reader.skip(4);	 // boneCountUsed
		reader.skip(4);	 // commonPointFlags
		reader.skip(2);	 // commonTexU (short)
		reader.skip(2);	 // commonTexV (short)
		reader.skip(4);	 // materialIndex
		if (info.version >= 68) {
			reader.skip(1);	 // extra byte in newer versions
		}
		if (reader.eof())
			return;
	}

	// 4. Selections (skip for now - we need vertex data first)
	uint32_t selectionCount = reader.readOr<uint32_t>(0);
	if (selectionCount > 500)
		return;
	for (uint32_t i = 0; i < selectionCount; i++) {
		reader.skipAsciiz();  // selection name
		uint32_t vertInSel = reader.readOr<uint32_t>(0);
		reader.skip(vertInSel);	 // vertex weights (bytes)
		uint32_t facesInSel = reader.readOr<uint32_t>(0);
		reader.skip(facesInSel);  // face weights (bytes)
		if (reader.eof())
			return;
	}

	// 5. Properties
	uint32_t propCount = reader.readOr<uint32_t>(0);
	if (propCount > 100)
		return;
	for (uint32_t i = 0; i < propCount; i++) {
		reader.skipAsciiz();  // property name
		reader.skipAsciiz();  // property value
		if (reader.eof())
			return;
	}

	// 6. Frames (animation frames, skip)
	uint32_t frameCount = reader.readOr<uint32_t>(0);
	if (frameCount > 1000)
		return;
	// Each frame has time(float) + boneCount transforms
	reader.skip(frameCount * (4 + boneCount * 48));
	if (reader.eof())
		return;

	// 7. Icon color & selected color
	reader.skip(4);	 // iconColor
	reader.skip(4);	 // selectedColor

	// 8. Special properties flags
	reader.skip(4);	 // flags

	// 9. Bounding info
	float bboxMinX = reader.readOr<float>(0.0f);
	float bboxMinY = reader.readOr<float>(0.0f);
	float bboxMinZ = reader.readOr<float>(0.0f);
	float bboxMaxX = reader.readOr<float>(0.0f);
	float bboxMaxY = reader.readOr<float>(0.0f);
	float bboxMaxZ = reader.readOr<float>(0.0f);
	reader.skip(4);	  // autocenter flag
	reader.skip(12);  // sphere (x, y, z)
	reader.skip(4);	  // sphere radius

	// 10. Vertex table
	uint32_t vtxFlags = reader.readOr<uint32_t>(0);
	uint32_t vertexCount = reader.readOr<uint32_t>(0);

	if (vertexCount == 0 || vertexCount > 500000)
		return;

	info.totalVertices += vertexCount;
	lod.points.resize(vertexCount);

	// Check if positions are compressed (flag bit 0)
	bool compressedPositions = (vtxFlags & 1) != 0;

	if (compressedPositions) {
		// Read XYZ scale factors (bounding box for decompression)
		[[maybe_unused]] float scaleX = reader.readOr<float>(0.0f);
		[[maybe_unused]] float scaleY = reader.readOr<float>(0.0f);
		[[maybe_unused]] float scaleZ = reader.readOr<float>(0.0f);

		// Read positions as signed shorts and decompress
		for (uint32_t i = 0; i < vertexCount; i++) {
			int16_t sx = reader.readOr<int16_t>(0);
			int16_t sy = reader.readOr<int16_t>(0);
			int16_t sz = reader.readOr<int16_t>(0);

			// Decompress: value = (short / 32767.0) * scale
			lod.points[i].x = bboxMinX + (static_cast<float>(sx + 32768) / 65535.0f) * (bboxMaxX - bboxMinX);
			lod.points[i].y = bboxMinY + (static_cast<float>(sy + 32768) / 65535.0f) * (bboxMaxY - bboxMinY);
			lod.points[i].z = bboxMinZ + (static_cast<float>(sz + 32768) / 65535.0f) * (bboxMaxZ - bboxMinZ);
			lod.points[i].flags = 0;

			if (reader.eof())
				break;
		}
	} else {
		// Read positions as floats
		for (uint32_t i = 0; i < vertexCount; i++) {
			lod.points[i].x = reader.readOr<float>(0.0f);
			lod.points[i].y = reader.readOr<float>(0.0f);
			lod.points[i].z = reader.readOr<float>(0.0f);
			lod.points[i].flags = 0;
			if (reader.eof())
				break;
		}
	}

	// Update bounding box from actual points
	for (const auto& pt : lod.points) {
		info.boundingBox[0] = std::min(info.boundingBox[0], pt.x);
		info.boundingBox[1] = std::min(info.boundingBox[1], pt.y);
		info.boundingBox[2] = std::min(info.boundingBox[2], pt.z);
		info.boundingBox[3] = std::max(info.boundingBox[3], pt.x);
		info.boundingBox[4] = std::max(info.boundingBox[4], pt.y);
		info.boundingBox[5] = std::max(info.boundingBox[5], pt.z);
	}

	// 11. Normals (skip for now, we'll generate them)
	reader.skip(vertexCount * 12);	// Assume floats for now

	// 12. UV coordinates - read first UV set, skip others
	uint32_t uvSetCount = 1;
	if (info.version >= 45) {
		uvSetCount = reader.readOr<uint32_t>(1);
	}

	lod.uvs.resize(vertexCount);

	for (uint32_t uvSet = 0; uvSet < uvSetCount; uvSet++) {
		if (info.version >= 45) {
			reader.skip(4);	 // uv set flags
		}

		if (uvSet == 0) {
			// Read first UV set
			for (uint32_t i = 0; i < vertexCount; i++) {
				if (!reader.hasBytes(8))
					break;
				float u = reader.readOr<float>(0.0f);
				float v = reader.readOr<float>(0.0f);
				lod.uvs[i].u = u;
				lod.uvs[i].v = 1.0f - v;  // Flip V coordinate for OpenGL
			}
		} else {
			// Skip additional UV sets
			reader.skip(vertexCount * 8);
		}
		if (reader.eof())
			break;
	}

	// 13. Faces
	uint32_t faceCount = reader.readOr<uint32_t>(0);
	if (faceCount == 0 || faceCount > 500000) {
		return;
	}

	info.totalFaces += faceCount;
	lod.faces.reserve(faceCount);

	for (uint32_t i = 0; i < faceCount; i++) {
		if (!reader.hasBytes(8))
			break;

		P3DFace face;

		// Face type (1 byte in newer ODOL): 3=triangle, 4=quad
		uint8_t faceType = reader.readOr<uint8_t>(3);
		face.type = (faceType == 4) ? 4 : 3;

		int numVerts = (face.type == 4) ? 4 : 3;

		// Read vertex indices (2 bytes each for models < 65535 vertices)
		for (int v = 0; v < numVerts; v++) {
			if (!reader.hasBytes(2))
				break;
			uint16_t idx = reader.readOr<uint16_t>(0);
			face.verts[v].pointIndex = idx;
			face.verts[v].normalIndex = idx;
			// Apply UV from per-vertex UV array
			if (idx < lod.uvs.size()) {
				face.verts[v].u = lod.uvs[idx].u;
				face.verts[v].v = lod.uvs[idx].v;
			} else {
				face.verts[v].u = 0;
				face.verts[v].v = 0;
			}
		}

		// Skip remaining vertex slots for triangles
		if (numVerts == 3) {
			face.verts[3].pointIndex = 0;
			face.verts[3].normalIndex = 0;
		}

		face.flags = 0;

		// Validate indices
		bool valid = true;
		for (int v = 0; v < numVerts; v++) {
			if (face.verts[v].pointIndex >= vertexCount) {
				valid = false;
				break;
			}
		}

		if (valid) {
			lod.faces.push_back(face);
		}
	}

	// Generate flat normals since we didn't parse them properly
	lod.normals.resize(vertexCount * 3, 0.0f);
	for (size_t i = 0; i < lod.faces.size(); i++) {
		const auto& face = lod.faces[i];
		int nv = (face.type == 4) ? 4 : 3;

		// Calculate face normal
		if (nv >= 3 && face.verts[0].pointIndex < vertexCount && face.verts[1].pointIndex < vertexCount &&
			face.verts[2].pointIndex < vertexCount) {

			const auto& p0 = lod.points[face.verts[0].pointIndex];
			const auto& p1 = lod.points[face.verts[1].pointIndex];
			const auto& p2 = lod.points[face.verts[2].pointIndex];

			float ax = p1.x - p0.x, ay = p1.y - p0.y, az = p1.z - p0.z;
			float bx = p2.x - p0.x, by = p2.y - p0.y, bz = p2.z - p0.z;
			float nx = ay * bz - az * by;
			float ny = az * bx - ax * bz;
			float nz = ax * by - ay * bx;
			float len = sqrtf(nx * nx + ny * ny + nz * nz);
			if (len > 0.0001f) {
				nx /= len;
				ny /= len;
				nz /= len;
			}

			// Assign to all vertices of this face
			for (int v = 0; v < nv; v++) {
				uint32_t idx = face.verts[v].pointIndex;
				if (idx < vertexCount) {
					lod.normals[idx * 3] = nx;
					lod.normals[idx * 3 + 1] = ny;
					lod.normals[idx * 3 + 2] = nz;
				}
			}
		}
	}
}

//=============================================================================
// Main P3D Reader
//=============================================================================

P3DInfo ReadP3DInfo(const char* path) {
	P3DInfo info;
	info.path = path;

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
		return info;

	char sig[4];
	file.read(sig, 4);

	if (strncmp(sig, "MLOD", 4) == 0) {
		info.type = "MLOD";
		info.valid = true;

		file.read((char*)&info.version, 4);
		file.read((char*)&info.lodCount, 4);

		info.boundingBox[0] = info.boundingBox[1] = info.boundingBox[2] = 1e9f;
		info.boundingBox[3] = info.boundingBox[4] = info.boundingBox[5] = -1e9f;

		for (uint32_t lodIdx = 0; lodIdx < info.lodCount; lodIdx++) {
			P3DLOD lod;

			char lodSig[4];
			file.read(lodSig, 4);
			lod.signature = std::string(lodSig, 4);

			bool isP3DM = (lod.signature == "P3DM");

			file.read((char*)&lod.majorVersion, 4);
			file.read((char*)&lod.minorVersion, 4);

			uint32_t numPoints, numNormals, numFaces, flags;
			file.read((char*)&numPoints, 4);
			file.read((char*)&numNormals, 4);
			file.read((char*)&numFaces, 4);
			file.read((char*)&flags, 4);

			info.totalVertices += numPoints;
			info.totalFaces += numFaces;

			lod.points.resize(numPoints);
			for (uint32_t i = 0; i < numPoints; i++) {
				file.read((char*)&lod.points[i].x, 4);
				file.read((char*)&lod.points[i].y, 4);
				file.read((char*)&lod.points[i].z, 4);
				file.read((char*)&lod.points[i].flags, 4);

				info.boundingBox[0] = std::min(info.boundingBox[0], lod.points[i].x);
				info.boundingBox[1] = std::min(info.boundingBox[1], lod.points[i].y);
				info.boundingBox[2] = std::min(info.boundingBox[2], lod.points[i].z);
				info.boundingBox[3] = std::max(info.boundingBox[3], lod.points[i].x);
				info.boundingBox[4] = std::max(info.boundingBox[4], lod.points[i].y);
				info.boundingBox[5] = std::max(info.boundingBox[5], lod.points[i].z);
			}

			lod.normals.resize(numNormals * 3);
			file.read((char*)lod.normals.data(), numNormals * 12);

			lod.faces.resize(numFaces);
			for (uint32_t i = 0; i < numFaces; i++) {
				P3DFace& face = lod.faces[i];

				if (!isP3DM) {
					char texName[32];
					file.read(texName, 32);
					face.texture = texName;
				}

				file.read((char*)&face.type, 4);

				for (int v = 0; v < 4; v++) {
					file.read((char*)&face.verts[v].pointIndex, 4);
					file.read((char*)&face.verts[v].normalIndex, 4);
					file.read((char*)&face.verts[v].u, 4);
					file.read((char*)&face.verts[v].v, 4);
				}

				file.read((char*)&face.flags, 4);

				if (isP3DM) {
					face.texture = ReadAsciiz(file);
					face.material = ReadAsciiz(file);
				}

				if (!face.texture.empty()) {
					if (std::find(info.allTextures.begin(), info.allTextures.end(), face.texture) == info.allTextures.end()) {
						info.allTextures.push_back(face.texture);
					}
				}
			}

			char taggSig[4];
			file.read(taggSig, 4);

			if (strncmp(taggSig, "TAGG", 4) == 0) {
				bool endOfTags = false;
				while (!endOfTags && file.good()) {
					std::string tagName;

					if (isP3DM) {
						uint8_t active;
						file.read((char*)&active, 1);
						tagName = ReadAsciiz(file);
					} else {
						char name[64];
						file.read(name, 64);
						tagName = name;
					}

					uint32_t dataSize;
					file.read((char*)&dataSize, 4);

					if (tagName == "#EndOfFile#") {
						endOfTags = true;
					} else if (tagName == "#Property#") {
						// Property tag: 64 bytes key + 64 bytes value (128
						// total)
						if (dataSize == 128) {
							char propKey[64] = {0};
							char propValue[64] = {0};
							file.read(propKey, 64);
							file.read(propValue, 64);

							P3DNamedProperty prop;
							prop.key = propKey;
							prop.value = propValue;

							// Add to list if not already present
							bool found = false;
							for (const auto& existing : info.namedProperties) {
								if (existing.key == prop.key) {
									found = true;
									break;
								}
							}
							if (!found && !prop.key.empty()) {
								info.namedProperties.push_back(prop);
							}

							// Also store in LOD-specific list
							lod.namedProperties.push_back(prop.key + "=" + prop.value);
						} else {
							file.seekg(dataSize, std::ios::cur);
						}
					} else if (tagName == "#SharpEdges#" || tagName == "#Mass#" || tagName == "#UVSet#" ||
							   tagName == "#Lock#" || tagName == "#Animation#") {
						file.seekg(dataSize, std::ios::cur);
					} else {
						P3DSelection sel;
						sel.name = tagName;

						if (dataSize == numPoints + numFaces) {
							sel.pointWeights.resize(numPoints);
							sel.faceWeights.resize(numFaces);
							file.read((char*)sel.pointWeights.data(), numPoints);
							file.read((char*)sel.faceWeights.data(), numFaces);
						} else {
							file.seekg(dataSize, std::ios::cur);
						}

						lod.selections.push_back(sel);

						if (std::find(info.allSelections.begin(), info.allSelections.end(), tagName) == info.allSelections.end()) {
							info.allSelections.push_back(tagName);

							if (tagName.find("proxy:") == 0) {
								info.proxySelections.push_back(tagName);
							}
						}
					}
				}
			}

			file.read((char*)&lod.resolution, 4);
			info.lods.push_back(lod);
		}

	} else if (strncmp(sig, "ODOL", 4) == 0) {
		info.type = "ODOL";
		info.valid = true;
		file.read((char*)&info.version, 4);

		// Skip appID for newer versions
		if (info.version >= 58) {
			uint32_t appID;
			file.read((char*)&appID, 4);
		}

		// Search for lodCount by finding where the LOD resolutions start
		info.lodCount = 0;

		// Scan header looking for valid pattern: lodCount (1-50) followed by
		// valid float resolutions
		for (int scanOffset = 8; scanOffset <= 64 && info.lodCount == 0; scanOffset++) {
			file.seekg(scanOffset, std::ios::beg);
			uint32_t potential;
			file.read((char*)&potential, 4);

			if (potential >= 1 && potential <= 50) {
				// Check if followed by valid LOD resolutions
				bool allValid = true;
				std::vector<float> testRes(std::min(potential, 5u));

				for (uint32_t i = 0; i < testRes.size() && allValid; i++) {
					file.read((char*)&testRes[i], 4);
					if (testRes[i] < 0.0f || (testRes[i] > 20000.0f && testRes[i] < 1e10f) || testRes[i] > 1e16f) {
						allValid = false;
					}
				}

				// Check progression - visual LODs should be increasing
				if (allValid && testRes.size() >= 2) {
					for (size_t i = 1; i < testRes.size(); i++) {
						if (testRes[i] < testRes[i - 1] && testRes[i] < 1e10f) {
							allValid = false;
							break;
						}
					}
				}

				if (allValid) {
					info.lodCount = potential;
					file.seekg(scanOffset + 4, std::ios::beg);
					break;
				}
			}
		}

		if (info.lodCount == 0) {
			info.warnings.push_back("  Could not find valid lodCount pattern!");
			info.warnings.push_back("  ODOL v" + std::to_string(info.version) +
									" parsing failed - binarized models not yet supported");
			return info;
		}

		// Read LOD resolutions
		std::vector<float> lodResolutions(info.lodCount);
		for (uint32_t i = 0; i < info.lodCount; i++) {
			file.read((char*)&lodResolutions[i], 4);
		}

		// Initialize bounding box
		info.boundingBox[0] = info.boundingBox[1] = info.boundingBox[2] = 1e9f;
		info.boundingBox[3] = info.boundingBox[4] = info.boundingBox[5] = -1e9f;

		// For ODOL v73+, model info is NOT compressed - it's inline
		if (info.version >= 73) {
			size_t infoStart = file.tellg();

			std::vector<uint8_t> modelInfoChunk(65536);
			file.read((char*)modelInfoChunk.data(), modelInfoChunk.size());
			size_t bytesRead = file.gcount();
			modelInfoChunk.resize(bytesRead);

			ExtractPathsFromData(modelInfoChunk, info.allTextures, ".paa");
			ExtractPathsFromData(modelInfoChunk, info.allTextures, ".rvmat");

			// Find the start of compressed LOD data
			size_t lodDataOffset = 0;
			for (size_t i = 0; i + 8 < bytesRead; i++) {
				uint32_t packed = *reinterpret_cast<uint32_t*>(&modelInfoChunk[i]);
				uint32_t unpacked = *reinterpret_cast<uint32_t*>(&modelInfoChunk[i + 4]);

				if (packed > 100 && packed < 10000000 && unpacked >= packed && unpacked < 50000000) {
					lodDataOffset = infoStart + i;
					break;
				}
			}

			if (lodDataOffset > 0) {
				file.seekg(lodDataOffset, std::ios::beg);
			}
		} else if (info.version >= 68) {
			auto modelInfo = ReadODOLCompressedBlock(file, "modelInfo");

			ExtractPathsFromData(modelInfo, info.allTextures, ".paa");
			ExtractPathsFromData(modelInfo, info.allTextures, ".rvmat");
		}

		// Read each LOD's compressed data
		for (uint32_t lodIdx = 0; lodIdx < info.lodCount; lodIdx++) {
			P3DLOD lod;
			lod.resolution = lodResolutions[lodIdx];
			lod.signature = "ODOL";
			lod.majorVersion = info.version;
			lod.minorVersion = 0;

			if (info.version >= 68) {
				auto lodData = ReadODOLCompressedBlock(file, ("LOD" + std::to_string(lodIdx)).c_str());
				if (!lodData.empty()) {
					ParseODOLLodGeometry(lodData, lod, info);
				}
			}

			info.lods.push_back(lod);
		}

		// If no geometry was extracted, note that ODOL geometry parsing is limited
		if (info.totalVertices == 0 && info.lodCount > 0) {
			info.warnings.push_back("ODOL: Binarized model - use unbinarized (MLOD) for preview");
		}

	} else {
		info.valid = false;
	}

	return info;
}

} // namespace a3tex

#include "../include/viewer_3d.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>


namespace a3tex {

//=============================================================================
// OBJ Loading
//=============================================================================

bool LoadOBJ(const char* path, Mesh& mesh) {
	std::ifstream file(path);
	if (!file.is_open())
		return false;

	std::vector<float> positions;
	std::vector<float> normals;
	std::vector<Vertex> vertices;

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;

		if (prefix == "v") {
			float x, y, z;
			iss >> x >> y >> z;
			positions.push_back(x);
			positions.push_back(y);
			positions.push_back(z);
		} else if (prefix == "vn") {
			float x, y, z;
			iss >> x >> y >> z;
			normals.push_back(x);
			normals.push_back(y);
			normals.push_back(z);
		} else if (prefix == "f") {
			std::string v1, v2, v3, v4;
			iss >> v1 >> v2 >> v3;

			auto parseVertex = [&](const std::string& s) -> Vertex {
				Vertex v = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0};
				int vi = 0, ti = 0, ni = 0;
				if (sscanf(s.c_str(), "%d/%d/%d", &vi, &ti, &ni) >= 1 || sscanf(s.c_str(), "%d//%d", &vi, &ni) >= 1 ||
					sscanf(s.c_str(), "%d", &vi) >= 1) {
					if (vi > 0 && (vi - 1) * 3 + 2 < (int)positions.size()) {
						v.x = positions[(vi - 1) * 3];
						v.y = positions[(vi - 1) * 3 + 1];
						v.z = positions[(vi - 1) * 3 + 2];
					}
					if (ni > 0 && (ni - 1) * 3 + 2 < (int)normals.size()) {
						v.nx = normals[(ni - 1) * 3];
						v.ny = normals[(ni - 1) * 3 + 1];
						v.nz = normals[(ni - 1) * 3 + 2];
					}
				}
				return v;
			};

			Vertex verts[4];
			verts[0] = parseVertex(v1);
			verts[1] = parseVertex(v2);
			verts[2] = parseVertex(v3);

			vertices.push_back(verts[0]);
			vertices.push_back(verts[1]);
			vertices.push_back(verts[2]);

			if (iss >> v4) {
				verts[3] = parseVertex(v4);
				vertices.push_back(verts[0]);
				vertices.push_back(verts[2]);
				vertices.push_back(verts[3]);
			}
		}
	}

	if (vertices.empty())
		return false;

	// Calculate bounds
	mesh.boundsMin[0] = mesh.boundsMin[1] = mesh.boundsMin[2] = 1e9f;
	mesh.boundsMax[0] = mesh.boundsMax[1] = mesh.boundsMax[2] = -1e9f;

	for (const auto& v : vertices) {
		mesh.boundsMin[0] = std::min(mesh.boundsMin[0], v.x);
		mesh.boundsMin[1] = std::min(mesh.boundsMin[1], v.y);
		mesh.boundsMin[2] = std::min(mesh.boundsMin[2], v.z);
		mesh.boundsMax[0] = std::max(mesh.boundsMax[0], v.x);
		mesh.boundsMax[1] = std::max(mesh.boundsMax[1], v.y);
		mesh.boundsMax[2] = std::max(mesh.boundsMax[2], v.z);
	}

	mesh.center[0] = (mesh.boundsMin[0] + mesh.boundsMax[0]) * 0.5f;
	mesh.center[1] = (mesh.boundsMin[1] + mesh.boundsMax[1]) * 0.5f;
	mesh.center[2] = (mesh.boundsMin[2] + mesh.boundsMax[2]) * 0.5f;

	float size = std::max({mesh.boundsMax[0] - mesh.boundsMin[0], mesh.boundsMax[1] - mesh.boundsMin[1],
						   mesh.boundsMax[2] - mesh.boundsMin[2]});
	mesh.scale = 2.0f / (size > 0 ? size : 1.0f);

	// Generate normals if missing
	for (size_t i = 0; i < vertices.size(); i += 3) {
		float len =
			sqrtf(vertices[i].nx * vertices[i].nx + vertices[i].ny * vertices[i].ny + vertices[i].nz * vertices[i].nz);
		if (len < 0.001f) {
			float ax = vertices[i + 1].x - vertices[i].x;
			float ay = vertices[i + 1].y - vertices[i].y;
			float az = vertices[i + 1].z - vertices[i].z;
			float bx = vertices[i + 2].x - vertices[i].x;
			float by = vertices[i + 2].y - vertices[i].y;
			float bz = vertices[i + 2].z - vertices[i].z;
			float nx = ay * bz - az * by;
			float ny = az * bx - ax * bz;
			float nz = ax * by - ay * bx;
			len = sqrtf(nx * nx + ny * ny + nz * nz);
			if (len > 0) {
				nx /= len;
				ny /= len;
				nz /= len;
			}
			vertices[i].nx = vertices[i + 1].nx = vertices[i + 2].nx = nx;
			vertices[i].ny = vertices[i + 1].ny = vertices[i + 2].ny = ny;
			vertices[i].nz = vertices[i + 1].nz = vertices[i + 2].nz = nz;
		}
	}

	mesh.vertices = vertices;

	// Upload to GPU
	if (mesh.vao)
		glDeleteVertexArrays(1, &mesh.vao);
	if (mesh.vbo)
		glDeleteBuffers(1, &mesh.vbo);

	glGenVertexArrays(1, &mesh.vao);
	glGenBuffers(1, &mesh.vbo);

	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	mesh.loaded = true;

	return true;
}

//=============================================================================
// Selection Helpers
//=============================================================================

bool IsFaceHidden(const P3DLOD& lod, size_t faceIndex, const std::vector<std::string>& hiddenSelections) {
	for (const auto& sel : lod.selections) {
		bool isHidden = false;
		for (const auto& hidden : hiddenSelections) {
			if (sel.name == hidden) {
				isHidden = true;
				break;
			}
		}
		if (!isHidden)
			continue;

		if (faceIndex < sel.faceWeights.size() && sel.faceWeights[faceIndex] > 0) {
			return true;
		}
	}
	return false;
}

bool IsFaceInSelection(const P3DLOD& lod, size_t faceIndex, const std::string& selectionName) {
	for (const auto& sel : lod.selections) {
		if (sel.name == selectionName) {
			if (faceIndex < sel.faceWeights.size() && sel.faceWeights[faceIndex] > 0) {
				return true;
			}
		}
	}
	return false;
}

//=============================================================================
// Animation Transform Support
//=============================================================================

uint8_t GetPointWeight(const P3DLOD& lod, size_t pointIndex, const std::string& selectionName) {
	for (const auto& sel : lod.selections) {
		if (sel.name == selectionName) {
			if (pointIndex < sel.pointWeights.size()) {
				return sel.pointWeights[pointIndex];
			}
		}
	}
	return 0;
}

void ApplyRotation(float& x, float& y, float& z, const float* axisPos, const float* axisDir, float angle,
					  float weight) {
	if (std::abs(angle) < 0.0001f || weight < 0.01f)
		return;

	angle *= weight;

	float px = x - axisPos[0];
	float py = y - axisPos[1];
	float pz = z - axisPos[2];

	float axLen = std::sqrt(axisDir[0] * axisDir[0] + axisDir[1] * axisDir[1] + axisDir[2] * axisDir[2]);
	if (axLen < 0.0001f)
		return;
	float ax = axisDir[0] / axLen;
	float ay = axisDir[1] / axLen;
	float az = axisDir[2] / axLen;

	// Rodrigues' rotation formula
	float c = std::cos(angle);
	float s = std::sin(angle);
	float dot = px * ax + py * ay + pz * az;
	float cx = ay * pz - az * py;
	float cy = az * px - ax * pz;
	float cz = ax * py - ay * px;

	float rx = px * c + cx * s + ax * dot * (1 - c);
	float ry = py * c + cy * s + ay * dot * (1 - c);
	float rz = pz * c + cz * s + az * dot * (1 - c);

	x = rx + axisPos[0];
	y = ry + axisPos[1];
	z = rz + axisPos[2];
}

void ApplyTranslation(float& x, float& y, float& z, const float* offset0, const float* offset1, float value,
						 float weight) {
	if (weight < 0.01f)
		return;

	float ox = offset0[0] + (offset1[0] - offset0[0]) * value;
	float oy = offset0[1] + (offset1[1] - offset0[1]) * value;
	float oz = offset0[2] + (offset1[2] - offset0[2]) * value;

	x += ox * weight;
	y += oy * weight;
	z += oz * weight;
}

bool IsFaceHiddenByAnimation(const P3DLOD& lod, size_t faceIndex,
								const std::vector<AnimationTransform>& transforms) {
	for (const auto& t : transforms) {
		if (t.type != 2)
			continue;

		if (t.value >= t.hideValue) {
			if (IsFaceInSelection(lod, faceIndex, t.selection)) {
				return true;
			}
		}
	}
	return false;
}

//=============================================================================
// P3D to Mesh Conversion
//=============================================================================

bool ConvertP3DToMesh(const P3DLOD& lod, Mesh& mesh, const std::vector<std::string>& hiddenSelections,
						 const std::string& highlightedSelection,
						 std::map<std::string, int>* textureMap) {
	if (lod.points.empty() || lod.faces.empty())
		return false;

	mesh.vertices.clear();

	// Build texture-to-index map (max 16 textures)
	std::map<std::string, int> texMap;
	int nextTexIndex = 0;
	for (const auto& face : lod.faces) {
		if (!face.texture.empty() && texMap.find(face.texture) == texMap.end()) {
			if (nextTexIndex < 16) {
				texMap[face.texture] = nextTexIndex++;
			}
		}
	}
	if (textureMap) {
		*textureMap = texMap;
	}

	for (size_t faceIdx = 0; faceIdx < lod.faces.size(); faceIdx++) {
		if (!hiddenSelections.empty() && IsFaceHidden(lod, faceIdx, hiddenSelections)) {
			continue;
		}

		float highlight = 0.0f;
		if (!highlightedSelection.empty() && IsFaceInSelection(lod, faceIdx, highlightedSelection)) {
			highlight = 1.0f;
		}

		const auto& face = lod.faces[faceIdx];
		int numVerts = (face.type == 4) ? 4 : 3;

		float texIndex = 0.0f;
		if (!face.texture.empty()) {
			auto it = texMap.find(face.texture);
			if (it != texMap.end()) {
				texIndex = (float)it->second;
			}
		}

		int indices[6];
		int numIndices;
		if (numVerts == 3) {
			indices[0] = 0;
			indices[1] = 1;
			indices[2] = 2;
			numIndices = 3;
		} else {
			indices[0] = 0;
			indices[1] = 1;
			indices[2] = 2;
			indices[3] = 0;
			indices[4] = 2;
			indices[5] = 3;
			numIndices = 6;
		}

		for (int i = 0; i < numIndices; i++) {
			int vi = indices[i];
			const P3DFaceVertex& fv = face.verts[vi];

			if (fv.pointIndex >= lod.points.size())
				continue;

			Vertex v;
			v.x = lod.points[fv.pointIndex].x;
			v.y = lod.points[fv.pointIndex].y;
			v.z = lod.points[fv.pointIndex].z;
			v.u = fv.u;
			v.v = fv.v;
			v.highlight = highlight;
			v.texIndex = texIndex;

			if (fv.normalIndex * 3 + 2 < lod.normals.size()) {
				v.nx = lod.normals[fv.normalIndex * 3];
				v.ny = lod.normals[fv.normalIndex * 3 + 1];
				v.nz = lod.normals[fv.normalIndex * 3 + 2];
			} else {
				v.nx = 0;
				v.ny = 1;
				v.nz = 0;
			}

			mesh.vertices.push_back(v);
		}
	}

	if (mesh.vertices.empty())
		return false;

	mesh.boundsMin[0] = mesh.boundsMin[1] = mesh.boundsMin[2] = 1e9f;
	mesh.boundsMax[0] = mesh.boundsMax[1] = mesh.boundsMax[2] = -1e9f;

	for (const auto& v : mesh.vertices) {
		mesh.boundsMin[0] = std::min(mesh.boundsMin[0], v.x);
		mesh.boundsMin[1] = std::min(mesh.boundsMin[1], v.y);
		mesh.boundsMin[2] = std::min(mesh.boundsMin[2], v.z);
		mesh.boundsMax[0] = std::max(mesh.boundsMax[0], v.x);
		mesh.boundsMax[1] = std::max(mesh.boundsMax[1], v.y);
		mesh.boundsMax[2] = std::max(mesh.boundsMax[2], v.z);
	}

	mesh.center[0] = (mesh.boundsMin[0] + mesh.boundsMax[0]) * 0.5f;
	mesh.center[1] = (mesh.boundsMin[1] + mesh.boundsMax[1]) * 0.5f;
	mesh.center[2] = (mesh.boundsMin[2] + mesh.boundsMax[2]) * 0.5f;

	float size = std::max({mesh.boundsMax[0] - mesh.boundsMin[0], mesh.boundsMax[1] - mesh.boundsMin[1],
						   mesh.boundsMax[2] - mesh.boundsMin[2]});
	mesh.scale = 2.0f / (size > 0 ? size : 1.0f);

	if (mesh.vao)
		glDeleteVertexArrays(1, &mesh.vao);
	if (mesh.vbo)
		glDeleteBuffers(1, &mesh.vbo);

	glGenVertexArrays(1, &mesh.vao);
	glGenBuffers(1, &mesh.vbo);

	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	mesh.loaded = true;

	return true;
}

bool ConvertP3DToMeshAnimated(const P3DLOD& lod, Mesh& mesh, const std::vector<std::string>& hiddenSelections,
								 const std::string& highlightedSelection,
								 const std::vector<AnimationTransform>& transforms,
								 std::map<std::string, int>* textureMap) {
	if (lod.points.empty() || lod.faces.empty())
		return false;

	mesh.vertices.clear();

	// Build texture-to-index map (max 16 textures)
	std::map<std::string, int> texMap;
	int nextTexIndex = 0;
	for (const auto& face : lod.faces) {
		if (!face.texture.empty() && texMap.find(face.texture) == texMap.end()) {
			if (nextTexIndex < 16) {
				texMap[face.texture] = nextTexIndex++;
			}
		}
	}
	if (textureMap) {
		*textureMap = texMap;
	}

	// Pre-calculate which points need transforms and their weights
	std::vector<std::vector<std::pair<const AnimationTransform*, float>>> pointTransforms(lod.points.size());

	for (size_t pi = 0; pi < lod.points.size(); pi++) {
		for (const auto& t : transforms) {
			if (t.type == 2)
				continue;

			uint8_t weight = GetPointWeight(lod, pi, t.selection);
			if (weight > 0) {
				pointTransforms[pi].push_back({&t, weight / 255.0f});
			}
		}
	}

	for (size_t faceIdx = 0; faceIdx < lod.faces.size(); faceIdx++) {
		if (!hiddenSelections.empty() && IsFaceHidden(lod, faceIdx, hiddenSelections)) {
			continue;
		}

		if (!transforms.empty() && IsFaceHiddenByAnimation(lod, faceIdx, transforms)) {
			continue;
		}

		float highlight = 0.0f;
		if (!highlightedSelection.empty() && IsFaceInSelection(lod, faceIdx, highlightedSelection)) {
			highlight = 1.0f;
		}

		const auto& face = lod.faces[faceIdx];
		int numVerts = (face.type == 4) ? 4 : 3;

		float texIndex = 0.0f;
		if (!face.texture.empty()) {
			auto it = texMap.find(face.texture);
			if (it != texMap.end()) {
				texIndex = (float)it->second;
			}
		}

		int indices[6];
		int numIndices;
		if (numVerts == 3) {
			indices[0] = 0;
			indices[1] = 1;
			indices[2] = 2;
			numIndices = 3;
		} else {
			indices[0] = 0;
			indices[1] = 1;
			indices[2] = 2;
			indices[3] = 0;
			indices[4] = 2;
			indices[5] = 3;
			numIndices = 6;
		}

		for (int i = 0; i < numIndices; i++) {
			int vi = indices[i];
			const P3DFaceVertex& fv = face.verts[vi];

			if (fv.pointIndex >= lod.points.size())
				continue;

			Vertex v;
			v.x = lod.points[fv.pointIndex].x;
			v.y = lod.points[fv.pointIndex].y;
			v.z = lod.points[fv.pointIndex].z;
			v.u = fv.u;
			v.v = fv.v;
			v.highlight = highlight;
			v.texIndex = texIndex;

			if (fv.normalIndex * 3 + 2 < lod.normals.size()) {
				v.nx = lod.normals[fv.normalIndex * 3];
				v.ny = lod.normals[fv.normalIndex * 3 + 1];
				v.nz = lod.normals[fv.normalIndex * 3 + 2];
			} else {
				v.nx = 0;
				v.ny = 1;
				v.nz = 0;
			}

			// Apply transforms to this vertex
			for (const auto& [t, weight] : pointTransforms[fv.pointIndex]) {
				if (t->type == 0) {
					float angle = t->angle0 + (t->angle1 - t->angle0) * t->value;
					ApplyRotation(v.x, v.y, v.z, t->axisPos, t->axisDir, angle, weight);
					float zeroPos[3] = {0, 0, 0};
					ApplyRotation(v.nx, v.ny, v.nz, zeroPos, t->axisDir, angle, weight);
				} else if (t->type == 1) {
					ApplyTranslation(v.x, v.y, v.z, t->offset0, t->offset1, t->value, weight);
				}
			}

			mesh.vertices.push_back(v);
		}
	}

	if (mesh.vertices.empty())
		return false;

	mesh.boundsMin[0] = mesh.boundsMin[1] = mesh.boundsMin[2] = 1e9f;
	mesh.boundsMax[0] = mesh.boundsMax[1] = mesh.boundsMax[2] = -1e9f;

	for (const auto& v : mesh.vertices) {
		mesh.boundsMin[0] = std::min(mesh.boundsMin[0], v.x);
		mesh.boundsMin[1] = std::min(mesh.boundsMin[1], v.y);
		mesh.boundsMin[2] = std::min(mesh.boundsMin[2], v.z);
		mesh.boundsMax[0] = std::max(mesh.boundsMax[0], v.x);
		mesh.boundsMax[1] = std::max(mesh.boundsMax[1], v.y);
		mesh.boundsMax[2] = std::max(mesh.boundsMax[2], v.z);
	}

	mesh.center[0] = (mesh.boundsMin[0] + mesh.boundsMax[0]) * 0.5f;
	mesh.center[1] = (mesh.boundsMin[1] + mesh.boundsMax[1]) * 0.5f;
	mesh.center[2] = (mesh.boundsMin[2] + mesh.boundsMax[2]) * 0.5f;

	float size = std::max({mesh.boundsMax[0] - mesh.boundsMin[0], mesh.boundsMax[1] - mesh.boundsMin[1],
						   mesh.boundsMax[2] - mesh.boundsMin[2]});
	mesh.scale = 2.0f / (size > 0 ? size : 1.0f);

	if (mesh.vao)
		glDeleteVertexArrays(1, &mesh.vao);
	if (mesh.vbo)
		glDeleteBuffers(1, &mesh.vbo);

	glGenVertexArrays(1, &mesh.vao);
	glGenBuffers(1, &mesh.vbo);

	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	mesh.loaded = true;

	return true;
}

} // namespace a3tex

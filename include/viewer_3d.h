#pragma once

#include "p3d_reader.h"

#include <glad/glad.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>


namespace arma3 {

//=============================================================================
// Structures
//=============================================================================

struct Vertex {
	float x, y, z;
	float nx, ny, nz;
	float u, v;		  // Texture coordinates
	float highlight;  // 0.0 = normal, 1.0 = highlighted
	float texIndex;	  // Texture slot index (0-15 for multi-texture support)
};

struct Mesh {
	std::vector<Vertex> vertices;
	GLuint vao = 0;
	GLuint vbo = 0;
	bool loaded = false;
	float boundsMin[3] = {0, 0, 0};
	float boundsMax[3] = {0, 0, 0};
	float center[3] = {0, 0, 0};
	float scale = 1.0f;
};

struct Camera {
	float distance = 5.0f;
	float yaw = 45.0f;
	float pitch = 30.0f;
	float targetX = 0, targetY = 0, targetZ = 0;
};

//=============================================================================
// Animation Transform Support
//=============================================================================

struct AnimationTransform {
	std::string selection;	// Target selection name
	int type = 0;			// 0=rotation, 1=translation, 2=hide
	float value = 0.0f;	// Current interpolated value (0.0-1.0)

	// For rotation
	float axisPos[3] = {0, 0, 0};  // Axis origin point
	float axisDir[3] = {0, 0, 1};  // Axis direction
	float angle0 = 0.0f;		    // Angle at value=0 (radians)
	float angle1 = 0.0f;		    // Angle at value=1 (radians)

	// For translation
	float offset0[3] = {0, 0, 0};  // Offset at value=0
	float offset1[3] = {0, 0, 0};  // Offset at value=1

	// For hide
	float hideValue = 0.5f;  // Hide when value >= this
};

//=============================================================================
// Functions (implemented in viewer_3d.cpp)
//=============================================================================

bool LoadOBJ(const char* path, Mesh& mesh);

bool IsFaceHidden(const P3DLOD& lod, size_t faceIndex, const std::vector<std::string>& hiddenSelections);
bool IsFaceInSelection(const P3DLOD& lod, size_t faceIndex, const std::string& selectionName);

uint8_t GetPointWeight(const P3DLOD& lod, size_t pointIndex, const std::string& selectionName);
void ApplyRotation(float& x, float& y, float& z, const float* axisPos, const float* axisDir, float angle,
				   float weight);
void ApplyTranslation(float& x, float& y, float& z, const float* offset0, const float* offset1, float value,
					  float weight);
bool IsFaceHiddenByAnimation(const P3DLOD& lod, size_t faceIndex,
							 const std::vector<AnimationTransform>& transforms);

bool ConvertP3DToMesh(const P3DLOD& lod, Mesh& mesh, const std::vector<std::string>& hiddenSelections = {},
					  const std::string& highlightedSelection = "",
					  std::map<std::string, int>* textureMap = nullptr);

bool ConvertP3DToMeshAnimated(const P3DLOD& lod, Mesh& mesh, const std::vector<std::string>& hiddenSelections,
							  const std::string& highlightedSelection,
							  const std::vector<AnimationTransform>& transforms,
							  std::map<std::string, int>* textureMap = nullptr);

} // namespace arma3

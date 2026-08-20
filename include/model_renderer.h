#pragma once
/**
 * @file renderer.hpp
 * @brief 3D model renderer with proper depth buffer handling
 * @author Eathan McLeod-Lucas
 *
 * Part of A3 Studio - Arma 3 Modding IDE
 */

#include <glad/glad.h>

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>


namespace arma3 {

// Material properties from RVMAT
struct MaterialProperties {
	float ambient[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float specular[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	float emissive[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	float specularPower = 50.0f;
	std::string rvmatPath;	// Source RVMAT file path
	bool hasRvmat = false;
};

struct RendererVertex {
	float x, y, z;	   // Position
	float nx, ny, nz;  // Normal
	float u, v;		   // Texture coordinates
	float highlight;   // 0.0 = normal, 1.0 = highlighted
	float texIndex;	   // Texture slot index (0-15 for multi-texture support)
};

struct RendererMesh {
	std::vector<RendererVertex> vertices;
	float boundsMin[3] = {0, 0, 0};
	float boundsMax[3] = {0, 0, 0};
	float center[3] = {0, 0, 0};
	float size = 1.0f;	// Largest dimension
	bool valid = false;
};

// Texture slot for multi-texture support
struct TextureSlot {
	GLuint textureId = 0;
	GLuint normalId = 0;	// Associated normal map
	GLuint specularId = 0;	// Associated specular map
	std::string path;
	std::string name;  // Display name (filename)
	std::string normalPath;
	std::string specularPath;
	int width = 0;
	int height = 0;
	bool active = true;			  // Whether this texture is currently displayed
	MaterialProperties material;  // Material properties from RVMAT
};

class ModelRenderer {
public:
	ModelRenderer();
	~ModelRenderer();

	// Initialize OpenGL resources
	bool Initialize();

	// Cleanup OpenGL resources
	void Shutdown();

	// Set viewport dimensions
	void SetViewportSize(int width, int height);

	// Camera controls
	void SetCameraDistance(float distance);
	void SetCameraAngles(float yaw, float pitch);
	void SetCameraTarget(float x, float y, float z);
	void AdjustCameraDistance(float delta);	 // For scroll zoom

	// Arcball camera rotation (smooth 3D rotation using quaternions)
	void RotateArcball(float deltaX, float deltaY, float viewportWidth, float viewportHeight);
	void ResetCamera();	 // Reset to default view
	void FrameObject();	 // Frame camera to fit the loaded mesh

	// Load mesh data
	void LoadMesh(const RendererMesh& mesh);
	void ClearMesh();
	void LoadTestCube();  // Debug: load a simple cube to test renderer

	// Render the scene
	void Render();

	// Get output texture for ImGui display
	GLuint GetOutputTexture() const { return m_ColorTexture; }

	// Check if initialized
	bool IsInitialized() const { return m_Initialized; }
	bool HasMesh() const { return m_HasMesh; }
	int GetVertexCount() const { return m_VertexCount; }

	// Texture management - single texture (legacy)
	bool LoadTexture(const std::string& path);
	void ClearTexture();
	bool HasTexture() const { return m_DiffuseTexture != 0; }
	std::string GetTexturePath() const { return m_DiffusePath; }

	// Multi-texture support (Arma 3 material system)
	// Texture types: _co (diffuse), _nohq (normal), _smdi (specular/metallic),
	// _as (ambient shadow)
	bool LoadAllTextures(const std::string& basePath);	// Load all related textures from base name
	bool LoadDiffuseTexture(const std::string& path);

	// Used for sections with no material of their own, and for a model that
	// carries no textures at all.
	void SetDefaultMaterial(const MaterialProperties& material) {
		m_DefaultMaterial = material;
	}

	// Approximates the family a PixelShaderID belongs to. The engine's shaders
	// are not available here, so this only makes the families look different.
	enum class ShaderStyle { Super = 0, Basic = 1, Glass = 2, Foliage = 3, Unlit = 4 };
	void SetShaderStyle(ShaderStyle style) { m_ShaderStyle = static_cast<int>(style); }
	static ShaderStyle StyleForPixelShader(const std::string& pixelShaderID);
	bool LoadNormalTexture(const std::string& path);
	bool LoadSpecularTexture(const std::string& path);
	bool LoadDetailTexture(const std::string& path);
	bool LoadMacroTexture(const std::string& path);
	void ClearAllTextures();

	bool HasDiffuseTexture() const { return m_DiffuseTexture != 0; }
	bool HasNormalTexture() const { return m_NormalTexture != 0; }
	bool HasSpecularTexture() const { return m_SpecularTexture != 0; }

	std::string GetDiffusePath() const { return m_DiffusePath; }
	std::string GetNormalPath() const { return m_NormalPath; }
	std::string GetSpecularPath() const { return m_SpecularPath; }

	// Multi-texture slot system (for loading multiple _co textures)
	int AddTextureSlot(const std::string& path);			  // Returns slot index, -1 on failure
	int AddTextureSlotWithMaterial(const std::string& path);  // Load texture + RVMAT + normal/specular
	void RemoveTextureSlot(int index);
	void SetActiveTextureSlot(int index);  // -1 = no texture
	int GetActiveTextureSlot() const { return m_ActiveTextureSlot; }
	const std::vector<TextureSlot>& GetTextureSlots() const { return m_TextureSlots; }
	std::vector<TextureSlot>& GetTextureSlotsMutable() { return m_TextureSlots; }
	void ClearTextureSlots();

	// Get current material properties (from active slot or default)
	const MaterialProperties& GetActiveMaterial() const;

	// Camera getters
	float GetCameraDistance() const { return m_CameraDistance; }
	float GetCameraYaw() const { return m_CameraYaw; }
	float GetCameraPitch() const { return m_CameraPitch; }
	void GetCameraTarget(float& x, float& y, float& z) const {
		x = m_TargetX;
		y = m_TargetY;
		z = m_TargetZ;
	}

	// Pan camera (move target in screen space)
	void PanCamera(float deltaX, float deltaY);

	// Lighting presets
	enum class LightingPreset { Day, Night, Custom };
	void SetLightingPreset(LightingPreset preset);
	LightingPreset GetLightingPreset() const { return m_LightingPreset; }

	// Custom lighting control
	void SetLightDirection(float x, float y, float z);
	void SetLightDirectionFromAngles(float yaw, float pitch);  // Degrees
	void GetLightDirection(float& x, float& y, float& z) const {
		x = m_LightDirX;
		y = m_LightDirY;
		z = m_LightDirZ;
	}
	void SetAmbientIntensity(float intensity) { m_AmbientIntensity = intensity; }
	float GetAmbientIntensity() const { return m_AmbientIntensity; }
	void SetLightColor(float r, float g, float b) {
		m_LightColorR = r;
		m_LightColorG = g;
		m_LightColorB = b;
	}
	void GetLightColor(float& r, float& g, float& b) const {
		r = m_LightColorR;
		g = m_LightColorG;
		b = m_LightColorB;
	}

	// Display options
	void SetBackfaceCulling(bool enabled) { m_BackfaceCulling = enabled; }
	bool GetBackfaceCulling() const { return m_BackfaceCulling; }
	void SetShowGrid(bool show) { m_ShowGrid = show; }
	bool GetShowGrid() const { return m_ShowGrid; }

	// Debug: Raycast from camera to scene center, returns true if ray hits mesh
	// bounds
	bool RaycastTest(float* hitDistance = nullptr) const;

private:
	bool m_Initialized = false;

	// Viewport
	int m_Width = 800;
	int m_Height = 600;

	// Framebuffer
	GLuint m_Framebuffer = 0;
	GLuint m_ColorTexture = 0;
	GLuint m_DepthTexture = 0;

	// Shader
	GLuint m_ShaderProgram = 0;
	GLint m_LocMVP = -1;
	GLint m_LocModel = -1;
	GLint m_LocLightDir = -1;
	GLint m_LocLightColor = -1;
	GLint m_LocAmbientIntensity = -1;
	GLint m_LocViewPos = -1;
	GLint m_LocTexture = -1;
	GLint m_LocHasTexture = -1;

	// Model textures (Arma 3 material system)
	GLuint m_DiffuseTexture = 0;   // _co.paa - color/diffuse
	GLuint m_NormalTexture = 0;	   // _nohq.paa - normal map
	GLuint m_SpecularTexture = 0;  // _smdi.paa - specular/metallic
	GLuint m_DetailTexture = 0;    // _dt.paa
	GLuint m_MacroTexture = 0;     // _mc.paa
	bool LoadStageTexture(const std::string& path, GLuint& target);
	std::string m_DiffusePath;
	std::string m_NormalPath;
	std::string m_SpecularPath;

	// Shader uniform locations for multi-texture
	GLint m_LocNormalMap = -1;
	GLint m_LocSpecularMap = -1;
	GLint m_LocDetailMap = -1;
	GLint m_LocMacroMap = -1;
	GLint m_LocHasDetailMap = -1;
	GLint m_LocHasMacroMap = -1;
	GLint m_LocHasNormalMap = -1;
	GLint m_LocHasSpecularMap = -1;

	// Texture array uniforms (for per-vertex texture indexing)
	GLint m_LocTextures[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
	GLint m_LocTextureCount = -1;

	// Material property uniforms
	GLint m_LocMatAmbient = -1;
	GLint m_LocMatDiffuse = -1;
	GLint m_LocMatSpecular = -1;
	GLint m_LocMatEmissive = -1;
	GLint m_LocMatSpecularPower = -1;

	// Default material properties
	MaterialProperties m_DefaultMaterial;
	int m_ShaderStyle = 0;
	GLint m_LocShaderStyle = -1;

	// Multi-texture slot system
	std::vector<TextureSlot> m_TextureSlots;
	int m_ActiveTextureSlot = -1;  // Index into m_TextureSlots, -1 = none

	// Mesh
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	int m_VertexCount = 0;
	float m_MeshCenter[3] = {0, 0, 0};
	float m_MeshSize = 1.0f;
	bool m_HasMesh = false;

	// Camera (spherical coordinates around target)
	float m_CameraDistance = 5.0f;
	float m_CameraYaw = 45.0f;	  // Degrees
	float m_CameraPitch = 20.0f;  // Degrees
	float m_TargetX = 0.0f;
	float m_TargetY = 0.0f;
	float m_TargetZ = 0.0f;

	// Arcball camera state (quaternion-based rotation)
	float m_Quaternion[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // x, y, z, w
	bool m_UseArcball = true;						   // Use quaternion rotation vs yaw/pitch

	// Display options
	bool m_BackfaceCulling = true;
	bool m_ShowGrid = true;

	// Grid rendering
	GLuint m_GridShader = 0;
	GLint m_GridMVPLoc = -1;
	GLuint m_GridVAO = 0;
	GLuint m_GridVBO = 0;
	int m_GridVertexCount = 0;
	bool CreateGridShader();
	void BuildGrid();
	void RenderGrid(const float* mvp);

	// Lighting
	LightingPreset m_LightingPreset = LightingPreset::Day;
	float m_LightDirX = 0.5f;
	float m_LightDirY = 0.8f;
	float m_LightDirZ = 0.3f;
	float m_AmbientIntensity = 0.4f;
	float m_LightColorR = 1.0f;
	float m_LightColorG = 0.95f;
	float m_LightColorB = 0.9f;

	// Internal methods
	bool CreateShaders();
	bool CreateFramebuffer();
	void ResizeFramebuffer();
	void ComputeMatrices(float* mvp, float* model, float* eyePos);

	// Matrix helpers
	static void Mat4Identity(float* m);
	static void Mat4Multiply(float* result, const float* a, const float* b);
	static void Mat4Perspective(float* m, float fovY, float aspect, float nearZ, float farZ);
	static void Mat4LookAt(float* m, float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ,
						   float upX, float upY, float upZ);
	static void Mat4Translation(float* m, float x, float y, float z);

	// Quaternion helpers for arcball rotation
	static void QuatMultiply(float* result, const float* a, const float* b);
	static void QuatNormalize(float* q);
	static void QuatFromAxisAngle(float* q, float axisX, float axisY, float axisZ, float angle);
	static void QuatToMatrix(float* m, const float* q);
	void UpdateAnglesFromQuaternion();	// Sync yaw/pitch from quaternion
	void UpdateQuaternionFromAngles();	// Sync quaternion from yaw/pitch
};

// Global renderer instance
extern ModelRenderer g_ModelRenderer;

} // namespace arma3

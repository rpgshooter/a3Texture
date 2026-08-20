/**
 * @file renderer.cpp
 * @brief OpenGL-based 3D model renderer for P3D visualization
 * @author Eathan McLeod-Lucas
 *
 * Part of A3 Studio - Arma 3 Modding IDE
 */

#include "../include/model_renderer.h"

#include "../include/paa_texture.h"
#include "../include/rvmat_parser.h"

#include <algorithm>
#include <cstring>
#include <filesystem>


namespace arma3 {

static void CheckGLError(const char* context) {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		const char* errStr = "UNKNOWN";
		switch (err) {
			case GL_INVALID_ENUM:
				errStr = "INVALID_ENUM";
				break;
			case GL_INVALID_VALUE:
				errStr = "INVALID_VALUE";
				break;
			case GL_INVALID_OPERATION:
				errStr = "INVALID_OPERATION";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				errStr = "INVALID_FRAMEBUFFER_OPERATION";
				break;
			case GL_OUT_OF_MEMORY:
				errStr = "OUT_OF_MEMORY";
				break;
		}
		LOG_ERROR(std::string("GL Error in ") + context + ": " + errStr);
	}
}

ModelRenderer g_ModelRenderer;

#define MAX_TEXTURE_SLOTS 16

// Vertex shader: transforms vertices and passes attributes to fragment shader.
// Uses per-vertex texture index to support multi-material P3D models.
static const char* s_VertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in float aHighlight;   // Selection highlight (0-1)
layout(location = 4) in float aTexIndex;    // Which texture slot this vertex uses

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vNormal;
out vec3 vWorldPos;
out vec3 vModelPos;
out vec2 vTexCoord;
out float vHighlight;
flat out int vTexIndex;     // 'flat' = no interpolation, keeps integer index intact

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vNormal = mat3(uModel) * aNormal;
    vWorldPos = vec3(uModel * vec4(aPos, 1.0));
    vModelPos = aPos;  // For derivative-based TBN calculation
    vTexCoord = aTexCoord;
    vHighlight = aHighlight;
    vTexIndex = int(aTexIndex);
}
)";

// Fragment shader: PBR-lite rendering with Arma 3 material support.
// Implements normal mapping, specular highlights, and environment reflection.
static const char* s_FragmentShaderSrc = R"(
#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec3 vModelPos;
in vec2 vTexCoord;
in float vHighlight;
flat in int vTexIndex;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uAmbientIntensity;
uniform vec3 uViewPos;
uniform sampler2D uTextures[16];  // Array of diffuse textures (per-material)
uniform sampler2D uNormalMap;     // Normal map (_nohq suffix in Arma 3)
uniform sampler2D uSpecularMap;   // SMDI map (_smdi suffix): R=spec, G=gloss, A=emissive
uniform int uTextureCount;        // Number of active textures
uniform int uHasNormalMap;
uniform int uHasSpecularMap;

// Which family of Arma shader this material asks for. The engine's own
// shaders are not available here, so these are approximations that make the
// families read differently rather than match the game.
#define STYLE_SUPER 0
#define STYLE_BASIC 1
#define STYLE_GLASS 2
#define STYLE_FOLIAGE 3
#define STYLE_UNLIT 4
uniform int uShaderStyle;

// Material properties loaded from RVMAT files, one entry per texture slot so
// a model carrying several materials shades each section with its own.
uniform vec4 uMatAmbient[16];
uniform vec4 uMatDiffuse[16];
uniform vec4 uMatSpecular[16];
uniform vec4 uMatEmissive[16];
uniform float uMatSpecularPower[16];

out vec4 FragColor;

// Compute cotangent frame for normal mapping without per-vertex tangents.
// This technique reconstructs TBN matrix using screen-space derivatives,
// avoiding the need to store tangent data in the vertex buffer.
// Reference: http://www.thetenthplanet.de/archives/1180
mat3 cotangentFrame(vec3 N, vec3 p, vec2 uv) {
    // Get edge vectors of the pixel triangle using screen-space derivatives
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    // Solve the linear system to find tangent and bitangent
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // Construct a scale-invariant frame
    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax, B * invmax, N);
}

// Perturb normal using normal map and TBN frame
vec3 perturbNormal(vec3 N, vec3 V, vec2 texcoord) {
    // Sample normal map (stored as RGB, convert to [-1,1] range)
    vec3 map = texture(uNormalMap, texcoord).rgb * 2.0 - 1.0;

    // IMPORTANT: Arma 3 normal maps use DirectX convention (Y-up flipped)
    // Must flip Y (green channel) to convert to OpenGL convention
    map.y = -map.y;

    // Build cotangent frame and transform normal
    mat3 TBN = cotangentFrame(N, -V, texcoord);
    return normalize(TBN * map);
}

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);

    // P3D models exported from Object Builder have inverted winding order.
    // We cull front faces (see glCullFace(GL_FRONT) in Render()),
    // so visible faces are "back" faces from OpenGL's perspective.
    // Flip normal for fragments that are front-facing to maintain correct lighting.
    if (gl_FrontFacing) normal = -normal;

    // Apply normal map if available using proper tangent-space transformation
    // The simpler families do not carry a normal map at all.
    if (uHasNormalMap != 0 && uShaderStyle != STYLE_BASIC && uShaderStyle != STYLE_UNLIT) {
        vec3 viewDir = normalize(vWorldPos - uViewPos);
        normal = perturbNormal(normal, viewDir, vTexCoord);
    }

    // Untextured faces fall back to the first material.
    int matIndex = (vTexIndex >= 0 && vTexIndex < 16) ? vTexIndex : 0;

    // Lighting using material properties and scene lighting
    vec3 ambientLight = uMatAmbient[matIndex].rgb * uAmbientIntensity;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuseLight = uMatDiffuse[matIndex].rgb * diff * uLightColor * 0.8;

    // Specular using material specular power
    vec3 viewDir = normalize(uViewPos - vWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float specPower = uMatSpecularPower[matIndex];
    vec3 specColor = uMatSpecular[matIndex].rgb;

    // SMDI, as the engine reads it: green scales the specular term and blue is
    // the exponent, spec = green * pow(NdotH, blue * coefficient). Red and
    // alpha are never sampled and sit at 255 in shipped textures, so taking
    // emissive from alpha lit everything carrying a specular map.
    float textureEmissive = 0.0;
    if (uHasSpecularMap != 0) {
        vec4 specTex = texture(uSpecularMap, vTexCoord);
        specColor *= specTex.g;
        specPower = max(1.0, specTex.b * specPower);
    }

    float spec = pow(max(dot(normal, halfDir), 0.0), specPower);
    vec3 specularLight = specColor * spec;

    // Environment reflection (fake sky gradient)
    vec3 reflectDir = reflect(-viewDir, normal);
    float skyGradient = reflectDir.y * 0.5 + 0.5;  // Map to 0-1
    vec3 skyColor = mix(vec3(0.3, 0.35, 0.4), vec3(0.6, 0.7, 0.9), skyGradient);  // Ground to sky
    vec3 groundColor = vec3(0.2, 0.18, 0.15);  // Brown ground
    vec3 envColor = reflectDir.y > 0.0 ? skyColor : groundColor;

    // Fresnel - more reflective at glancing angles
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);
    fresnel *= 0.3;  // Scale down for subtlety

    // Blend environment reflection based on fresnel and material specularity
    vec3 envReflection = envColor * fresnel * length(specColor);

    // Base color - select from texture array based on vertex texture index.
    // P3D models can have multiple materials, each with its own texture.
    vec4 baseColor = vec4(0.6, 0.6, 0.6, 1.0);  // Neutral gray when no texture
    if (uTextureCount > 0 && vTexIndex >= 0 && vTexIndex < uTextureCount) {
        // Sample from the appropriate texture based on vertex's material index.
        // GLSL requires constant index for sampler arrays - dynamic indexing not allowed.
        // This switch statement is the standard workaround for this GLSL limitation.
        switch (vTexIndex) {
            case 0:  baseColor = texture(uTextures[0], vTexCoord); break;
            case 1:  baseColor = texture(uTextures[1], vTexCoord); break;
            case 2:  baseColor = texture(uTextures[2], vTexCoord); break;
            case 3:  baseColor = texture(uTextures[3], vTexCoord); break;
            case 4:  baseColor = texture(uTextures[4], vTexCoord); break;
            case 5:  baseColor = texture(uTextures[5], vTexCoord); break;
            case 6:  baseColor = texture(uTextures[6], vTexCoord); break;
            case 7:  baseColor = texture(uTextures[7], vTexCoord); break;
            case 8:  baseColor = texture(uTextures[8], vTexCoord); break;
            case 9:  baseColor = texture(uTextures[9], vTexCoord); break;
            case 10: baseColor = texture(uTextures[10], vTexCoord); break;
            case 11: baseColor = texture(uTextures[11], vTexCoord); break;
            case 12: baseColor = texture(uTextures[12], vTexCoord); break;
            case 13: baseColor = texture(uTextures[13], vTexCoord); break;
            case 14: baseColor = texture(uTextures[14], vTexCoord); break;
            case 15: baseColor = texture(uTextures[15], vTexCoord); break;
        }
    }

    // Discard fully transparent pixels (alpha test)
    if (baseColor.a < 0.01) discard;

    // Orange tint for highlighted faces
    vec3 highlightColor = vec3(1.0, 0.6, 0.2);
    vec3 color = mix(baseColor.rgb, highlightColor, vHighlight);

    // Combine lighting with emissive and environment reflection
    // Emissive comes from: RVMAT emissive property + SMDI texture alpha (self-illumination)
    // Boost emissive for more visible glow effect
    float emissiveStrength = textureEmissive * 3.0;  // Amplify SMDI alpha glow
    vec3 emissiveColor = uMatEmissive[matIndex].rgb + baseColor.rgb * emissiveStrength;

    // Add slight HDR bloom effect for strong emissives
    float emissiveIntensity = max(max(emissiveColor.r, emissiveColor.g), emissiveColor.b);
    if (emissiveIntensity > 0.5) {
        emissiveColor *= 1.0 + (emissiveIntensity - 0.5) * 0.5;  // Subtle bloom
    }

    float alpha = baseColor.a;

    if (uShaderStyle == STYLE_BASIC) {
        // Diffuse modulate, no environment term.
        envReflection = vec3(0.0);
        specularLight *= 0.35;
    } else if (uShaderStyle == STYLE_GLASS) {
        // Thin and reflective: the rim carries it rather than the surface.
        float rim = pow(1.0 - abs(dot(normal, normalize(uViewPos - vWorldPos))), 3.0);
        envReflection *= 2.5;
        specularLight *= 2.0;
        diffuseLight *= 0.25;
        alpha = clamp(alpha * (0.25 + rim), 0.0, 1.0);
    } else if (uShaderStyle == STYLE_FOLIAGE) {
        // Lit largely by ambient, barely shiny, and cut out rather than blended.
        ambientLight *= 1.6;
        specularLight *= 0.15;
        envReflection = vec3(0.0);
        if (alpha < 0.5) discard;
        alpha = 1.0;
    } else if (uShaderStyle == STYLE_UNLIT) {
        // Reticles and screens ignore scene lighting entirely.
        ambientLight = vec3(1.0);
        diffuseLight = vec3(0.0);
        specularLight = vec3(0.0);
        envReflection = vec3(0.0);
    }

    vec3 result = (ambientLight + diffuseLight + specularLight) * color + emissiveColor + envReflection;

    FragColor = vec4(result, alpha);
}
)";

// Simple grid/axis shader - just transforms position and passes through color
static const char* s_GridVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uMVP;

out vec4 vColor;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char* s_GridFragSrc = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    FragColor = vColor;
}
)";

ModelRenderer::ModelRenderer() {}

ModelRenderer::~ModelRenderer() {
	Shutdown();
}

bool ModelRenderer::Initialize() {
	if (m_Initialized)
		return true;

	CheckGLError("Before Initialize");

	if (!CreateShaders()) {
		LOG_ERROR("Renderer: Failed to create shaders");
		return false;
	}
	CheckGLError("After CreateShaders");

	if (!CreateFramebuffer()) {
		LOG_ERROR("Renderer: Failed to create framebuffer");
		return false;
	}
	CheckGLError("After CreateFramebuffer");

	// Create VAO/VBO for mesh
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	CheckGLError("After VAO/VBO creation");

	// Create grid shader and geometry
	if (!CreateGridShader()) {
		LOG_ERROR("Renderer: Failed to create grid shader (non-fatal)");
	} else {
		BuildGrid();
	}
	CheckGLError("After grid creation");

	m_Initialized = true;
	return true;
}

void ModelRenderer::Shutdown() {
	if (m_VAO) {
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
	}
	if (m_VBO) {
		glDeleteBuffers(1, &m_VBO);
		m_VBO = 0;
	}
	if (m_Framebuffer) {
		glDeleteFramebuffers(1, &m_Framebuffer);
		m_Framebuffer = 0;
	}
	if (m_ColorTexture) {
		glDeleteTextures(1, &m_ColorTexture);
		m_ColorTexture = 0;
	}
	if (m_DepthTexture) {
		glDeleteTextures(1, &m_DepthTexture);
		m_DepthTexture = 0;
	}
	if (m_DiffuseTexture) {
		glDeleteTextures(1, &m_DiffuseTexture);
		m_DiffuseTexture = 0;
	}
	if (m_NormalTexture) {
		glDeleteTextures(1, &m_NormalTexture);
		m_NormalTexture = 0;
	}
	if (m_SpecularTexture) {
		glDeleteTextures(1, &m_SpecularTexture);
		m_SpecularTexture = 0;
	}
	if (m_ShaderProgram) {
		glDeleteProgram(m_ShaderProgram);
		m_ShaderProgram = 0;
	}
	if (m_GridShader) {
		glDeleteProgram(m_GridShader);
		m_GridShader = 0;
	}
	if (m_GridVAO) {
		glDeleteVertexArrays(1, &m_GridVAO);
		m_GridVAO = 0;
	}
	if (m_GridVBO) {
		glDeleteBuffers(1, &m_GridVBO);
		m_GridVBO = 0;
	}
	m_DiffusePath.clear();
	m_NormalPath.clear();
	m_SpecularPath.clear();
	m_Initialized = false;
}

bool ModelRenderer::CreateShaders() {
	// Compile vertex shader
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &s_VertexShaderSrc, nullptr);
	glCompileShader(vs);

	GLint success;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[1024];
		glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
		LOG_ERROR(std::string("Vertex shader error: ") + log);
		glDeleteShader(vs);
		return false;
	}

	// Compile fragment shader
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &s_FragmentShaderSrc, nullptr);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[1024];
		glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
		LOG_ERROR(std::string("Fragment shader error: ") + log);
		glDeleteShader(vs);
		glDeleteShader(fs);
		return false;
	}

	// Link program
	m_ShaderProgram = glCreateProgram();
	glAttachShader(m_ShaderProgram, vs);
	glAttachShader(m_ShaderProgram, fs);
	glLinkProgram(m_ShaderProgram);

	glGetProgramiv(m_ShaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		char log[1024];
		glGetProgramInfoLog(m_ShaderProgram, sizeof(log), nullptr, log);
		LOG_ERROR(std::string("Shader link error: ") + log);
		glDeleteShader(vs);
		glDeleteShader(fs);
		glDeleteProgram(m_ShaderProgram);
		m_ShaderProgram = 0;
		return false;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	// Get uniform locations
	m_LocMVP = glGetUniformLocation(m_ShaderProgram, "uMVP");
	m_LocModel = glGetUniformLocation(m_ShaderProgram, "uModel");
	m_LocLightDir = glGetUniformLocation(m_ShaderProgram, "uLightDir");
	m_LocLightColor = glGetUniformLocation(m_ShaderProgram, "uLightColor");
	m_LocAmbientIntensity = glGetUniformLocation(m_ShaderProgram, "uAmbientIntensity");
	m_LocViewPos = glGetUniformLocation(m_ShaderProgram, "uViewPos");
	m_LocTexture = glGetUniformLocation(m_ShaderProgram, "uTexture");
	m_LocHasTexture = glGetUniformLocation(m_ShaderProgram, "uHasTexture");
	m_LocNormalMap = glGetUniformLocation(m_ShaderProgram, "uNormalMap");
	m_LocHasNormalMap = glGetUniformLocation(m_ShaderProgram, "uHasNormalMap");
	m_LocSpecularMap = glGetUniformLocation(m_ShaderProgram, "uSpecularMap");
	m_LocHasSpecularMap = glGetUniformLocation(m_ShaderProgram, "uHasSpecularMap");

	// Get texture array uniform locations
	for (int i = 0; i < MAX_TEXTURE_SLOTS; i++) {
		char uniformName[32];
		snprintf(uniformName, sizeof(uniformName), "uTextures[%d]", i);
		m_LocTextures[i] = glGetUniformLocation(m_ShaderProgram, uniformName);
	}
	m_LocTextureCount = glGetUniformLocation(m_ShaderProgram, "uTextureCount");

	// Get material property uniform locations
	m_LocMatAmbient = glGetUniformLocation(m_ShaderProgram, "uMatAmbient");
	m_LocMatDiffuse = glGetUniformLocation(m_ShaderProgram, "uMatDiffuse");
	m_LocMatSpecular = glGetUniformLocation(m_ShaderProgram, "uMatSpecular");
	m_LocMatEmissive = glGetUniformLocation(m_ShaderProgram, "uMatEmissive");
	m_LocMatSpecularPower = glGetUniformLocation(m_ShaderProgram, "uMatSpecularPower");
	m_LocShaderStyle = glGetUniformLocation(m_ShaderProgram, "uShaderStyle");

	return true;
}

bool ModelRenderer::CreateFramebuffer() {
	// Create color texture
	glGenTextures(1, &m_ColorTexture);
	glBindTexture(GL_TEXTURE_2D, m_ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create depth texture (32-bit float for maximum precision)
	glGenTextures(1, &m_DepthTexture);
	glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create framebuffer
	glGenFramebuffers(1, &m_Framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOG_ERROR("Framebuffer incomplete: " + std::to_string(status));
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

void ModelRenderer::ResizeFramebuffer() {
	if (!m_Initialized)
		return;

	// Resize color texture
	glBindTexture(GL_TEXTURE_2D, m_ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Resize depth texture
	glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
}

bool ModelRenderer::CreateGridShader() {
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &s_GridVertSrc, nullptr);
	glCompileShader(vs);
	GLint ok;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(vs, 512, nullptr, log);
		LOG_ERROR(std::string("Grid vertex shader error: ") + log);
		glDeleteShader(vs);
		return false;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &s_GridFragSrc, nullptr);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(fs, 512, nullptr, log);
		LOG_ERROR(std::string("Grid fragment shader error: ") + log);
		glDeleteShader(vs);
		glDeleteShader(fs);
		return false;
	}

	m_GridShader = glCreateProgram();
	glAttachShader(m_GridShader, vs);
	glAttachShader(m_GridShader, fs);
	glLinkProgram(m_GridShader);
	glDeleteShader(vs);
	glDeleteShader(fs);

	glGetProgramiv(m_GridShader, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetProgramInfoLog(m_GridShader, 512, nullptr, log);
		LOG_ERROR(std::string("Grid shader link error: ") + log);
		glDeleteProgram(m_GridShader);
		m_GridShader = 0;
		return false;
	}

	m_GridMVPLoc = glGetUniformLocation(m_GridShader, "uMVP");
	return true;
}

// Grid vertex: position (3) + color (4)
struct GridVertex {
	float x, y, z;
	float r, g, b, a;
};

void ModelRenderer::BuildGrid() {
	std::vector<GridVertex> verts;
	verts.reserve(200);

	float gridSize = 10.0f;
	float step = 1.0f;
	float y = 0.0f;

	// Grid lines on XZ plane
	for (float i = -gridSize; i <= gridSize; i += step) {
		float intensity = (i == 0.0f) ? 0.35f : 0.15f;

		// Lines along Z
		verts.push_back({i, y, -gridSize, intensity, intensity, intensity, 1.0f});
		verts.push_back({i, y,  gridSize, intensity, intensity, intensity, 1.0f});

		// Lines along X
		verts.push_back({-gridSize, y, i, intensity, intensity, intensity, 1.0f});
		verts.push_back({ gridSize, y, i, intensity, intensity, intensity, 1.0f});
	}

	// Axis indicators (slightly above grid)
	float ay = 0.001f;
	float axisLen = 2.0f;

	// X axis - red
	verts.push_back({0, ay, 0, 0.8f, 0.2f, 0.2f, 1.0f});
	verts.push_back({axisLen, ay, 0, 0.8f, 0.2f, 0.2f, 1.0f});

	// Y axis - green (vertical)
	verts.push_back({0, 0, 0, 0.2f, 0.8f, 0.2f, 1.0f});
	verts.push_back({0, axisLen, 0, 0.2f, 0.8f, 0.2f, 1.0f});

	// Z axis - blue
	verts.push_back({0, ay, 0, 0.2f, 0.2f, 0.8f, 1.0f});
	verts.push_back({0, ay, axisLen, 0.2f, 0.2f, 0.8f, 1.0f});

	m_GridVertexCount = static_cast<int>(verts.size());

	glGenVertexArrays(1, &m_GridVAO);
	glGenBuffers(1, &m_GridVBO);
	glBindVertexArray(m_GridVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);
	glBufferData(GL_ARRAY_BUFFER,
				 static_cast<GLsizeiptr>(verts.size() * sizeof(GridVertex)),
				 verts.data(), GL_STATIC_DRAW);
	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)0);
	glEnableVertexAttribArray(0);
	// Color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

void ModelRenderer::RenderGrid(const float* mvp) {
	if (!m_GridShader || m_GridVertexCount == 0) return;

	glUseProgram(m_GridShader);
	glUniformMatrix4fv(m_GridMVPLoc, 1, GL_FALSE, mvp);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindVertexArray(m_GridVAO);
	glDrawArrays(GL_LINES, 0, m_GridVertexCount);
	glBindVertexArray(0);
}

void ModelRenderer::SetViewportSize(int width, int height) {
	if (width <= 0 || height <= 0)
		return;
	if (width == m_Width && height == m_Height)
		return;

	m_Width = width;
	m_Height = height;

	if (m_Initialized) {
		ResizeFramebuffer();
	}
}

void ModelRenderer::SetCameraDistance(float distance) {
	m_CameraDistance = std::max(0.1f, distance);
}

void ModelRenderer::SetCameraAngles(float yaw, float pitch) {
	m_CameraYaw = yaw;
	m_CameraPitch = std::max(-89.0f, std::min(89.0f, pitch));
}

void ModelRenderer::SetCameraTarget(float x, float y, float z) {
	m_TargetX = x;
	m_TargetY = y;
	m_TargetZ = z;
}

void ModelRenderer::AdjustCameraDistance(float delta) {
	// Zoom speed proportional to distance
	float speed = m_CameraDistance * 0.1f;
	m_CameraDistance -= delta * speed;
	m_CameraDistance = std::max(0.1f, std::min(10000.0f, m_CameraDistance));
}

void ModelRenderer::PanCamera(float deltaX, float deltaY) {
	// Convert yaw/pitch to radians
	const float PI = 3.14159265f;
	float yawRad = m_CameraYaw * PI / 180.0f;
	float pitchRad = m_CameraPitch * PI / 180.0f;

	// Calculate camera right vector (perpendicular to view in XZ plane)
	float rightX = std::cos(yawRad);
	float rightZ = std::sin(yawRad);

	// Calculate camera up vector (perpendicular to view and right)
	// For a simple orbit camera, up is mostly world Y adjusted by pitch
	float upX = -std::sin(pitchRad) * std::sin(yawRad);
	float upY = std::cos(pitchRad);
	float upZ = std::sin(pitchRad) * std::cos(yawRad);

	// Pan speed proportional to distance
	float speed = m_CameraDistance * 0.002f;

	// Move target in screen space
	m_TargetX -= (deltaX * rightX + deltaY * upX) * speed;
	m_TargetY -= deltaY * upY * speed;
	m_TargetZ -= (-deltaX * rightZ + deltaY * upZ) * speed;
}

void ModelRenderer::SetLightingPreset(LightingPreset preset) {
	m_LightingPreset = preset;
	switch (preset) {
		case LightingPreset::Day:
			// Bright sun from upper-right
			m_LightDirX = 0.5f;
			m_LightDirY = 0.8f;
			m_LightDirZ = 0.3f;
			m_AmbientIntensity = 0.4f;
			m_LightColorR = 1.0f;
			m_LightColorG = 0.95f;
			m_LightColorB = 0.9f;
			break;
		case LightingPreset::Night:
			// Dim blue moonlight from above
			m_LightDirX = -0.2f;
			m_LightDirY = 0.9f;
			m_LightDirZ = 0.3f;
			m_AmbientIntensity = 0.15f;
			m_LightColorR = 0.6f;
			m_LightColorG = 0.7f;
			m_LightColorB = 0.9f;
			break;
		case LightingPreset::Custom:
			// Keep current values
			break;
	}
}

void ModelRenderer::SetLightDirection(float x, float y, float z) {
	// Normalize the direction
	float len = std::sqrt(x * x + y * y + z * z);
	if (len > 0.001f) {
		m_LightDirX = x / len;
		m_LightDirY = y / len;
		m_LightDirZ = z / len;
	}
	m_LightingPreset = LightingPreset::Custom;
}

void ModelRenderer::SetLightDirectionFromAngles(float yaw, float pitch) {
	const float PI = 3.14159265f;
	float yawRad = yaw * PI / 180.0f;
	float pitchRad = pitch * PI / 180.0f;

	// Convert spherical to cartesian
	// Yaw: 0 = +X, 90 = +Z (horizontal rotation)
	// Pitch: 0 = horizontal, 90 = straight up
	m_LightDirX = std::cos(pitchRad) * std::cos(yawRad);
	m_LightDirY = std::sin(pitchRad);
	m_LightDirZ = std::cos(pitchRad) * std::sin(yawRad);
	m_LightingPreset = LightingPreset::Custom;
}

void ModelRenderer::LoadMesh(const RendererMesh& mesh) {
	if (!m_Initialized)
		return;
	if (mesh.vertices.empty()) {
		ClearMesh();
		return;
	}

	// Store mesh info
	m_MeshCenter[0] = mesh.center[0];
	m_MeshCenter[1] = mesh.center[1];
	m_MeshCenter[2] = mesh.center[2];
	m_MeshSize = mesh.size;
	m_VertexCount = static_cast<int>(mesh.vertices.size());
	m_HasMesh = true;

	// Upload to GPU
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(RendererVertex), mesh.vertices.data(), GL_STATIC_DRAW);

	// Position attribute (location 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RendererVertex), (void*)0);

	// Normal attribute (location 1)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RendererVertex), (void*)(3 * sizeof(float)));

	// UV attribute (location 2)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RendererVertex), (void*)(6 * sizeof(float)));

	// Highlight attribute (location 3)
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(RendererVertex), (void*)(8 * sizeof(float)));

	// Texture index attribute (location 4)
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(RendererVertex), (void*)(9 * sizeof(float)));

	glBindVertexArray(0);

	// Set camera to frame the model
	m_CameraDistance = m_MeshSize * 2.5f;
	if (m_CameraDistance < 1.0f)
		m_CameraDistance = 3.0f;
	m_TargetX = 0.0f;  // Model is centered at origin
	m_TargetY = 0.0f;
	m_TargetZ = 0.0f;
}

void ModelRenderer::ClearMesh() {
	m_VertexCount = 0;
	m_HasMesh = false;
}

void ModelRenderer::LoadTestCube() {
	if (!m_Initialized)
		return;

	// Simple cube vertices with normals, UVs, and highlight (9 floats per
	// vertex) Format: x, y, z, nx, ny, nz, u, v, highlight
	float cubeData[] = {
		// Front face
		-0.5f,
		-0.5f,
		0.5f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.5f,
		-0.5f,
		0.5f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f,
		0.0f,
		0.5f,
		0.5f,
		0.5f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		1.0f,
		0.0f,
		-0.5f,
		-0.5f,
		0.5f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.5f,
		0.5f,
		0.5f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		1.0f,
		0.0f,
		-0.5f,
		0.5f,
		0.5f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		1.0f,
		0.0f,
		// Back face
		-0.5f,
		-0.5f,
		-0.5f,
		0.0f,
		0.0f,
		-1.0f,
		1.0f,
		0.0f,
		0.0f,
		-0.5f,
		0.5f,
		-0.5f,
		0.0f,
		0.0f,
		-1.0f,
		1.0f,
		1.0f,
		0.0f,
		0.5f,
		0.5f,
		-0.5f,
		0.0f,
		0.0f,
		-1.0f,
		0.0f,
		1.0f,
		0.0f,
		-0.5f,
		-0.5f,
		-0.5f,
		0.0f,
		0.0f,
		-1.0f,
		1.0f,
		0.0f,
		0.0f,
		0.5f,
		0.5f,
		-0.5f,
		0.0f,
		0.0f,
		-1.0f,
		0.0f,
		1.0f,
		0.0f,
		0.5f,
		-0.5f,
		-0.5f,
		0.0f,
		0.0f,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		// Top face
		-0.5f,
		0.5f,
		-0.5f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		-0.5f,
		0.5f,
		0.5f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.5f,
		0.5f,
		0.5f,
		0.0f,
		1.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f,
		-0.5f,
		0.5f,
		-0.5f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0.5f,
		0.5f,
		0.5f,
		0.0f,
		1.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f,
		0.5f,
		0.5f,
		-0.5f,
		0.0f,
		1.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		// Bottom face
		-0.5f,
		-0.5f,
		-0.5f,
		0.0f,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.5f,
		-0.5f,
		-0.5f,
		0.0f,
		-1.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f,
		0.5f,
		-0.5f,
		0.5f,
		0.0f,
		-1.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		-0.5f,
		-0.5f,
		-0.5f,
		0.0f,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.5f,
		-0.5f,
		0.5f,
		0.0f,
		-1.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		-0.5f,
		-0.5f,
		0.5f,
		0.0f,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		// Right face
		0.5f,
		-0.5f,
		-0.5f,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0.5f,
		0.5f,
		-0.5f,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.5f,
		0.5f,
		0.5f,
		1.0f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f,
		0.5f,
		-0.5f,
		-0.5f,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0.5f,
		0.5f,
		0.5f,
		1.0f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f,
		0.5f,
		-0.5f,
		0.5f,
		1.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		// Left face
		-0.5f,
		-0.5f,
		-0.5f,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		-0.5f,
		-0.5f,
		0.5f,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		-0.5f,
		0.5f,
		0.5f,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		-0.5f,
		-0.5f,
		-0.5f,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		-0.5f,
		0.5f,
		0.5f,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		-0.5f,
		0.5f,
		-0.5f,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f,
	};

	m_MeshCenter[0] = 0.0f;
	m_MeshCenter[1] = 0.0f;
	m_MeshCenter[2] = 0.0f;
	m_MeshSize = 1.0f;
	m_VertexCount = 36;
	m_HasMesh = true;

	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeData), cubeData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));

	glBindVertexArray(0);

	m_CameraDistance = 3.0f;
	m_TargetX = m_TargetY = m_TargetZ = 0.0f;
}

/// Main render function - renders the loaded mesh to the offscreen framebuffer.
/// The rendered image is then displayed in ImGui via GetColorTexture().
void ModelRenderer::Render() {
	if (!m_Initialized)
		return;

	// Save OpenGL state that ImGui modifies during its rendering.
	// Must restore this after our render pass to avoid breaking ImGui.
	GLint lastProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
	GLint lastFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFBO);
	GLint lastViewport[4];
	glGetIntegerv(GL_VIEWPORT, lastViewport);

	// Bind our framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

	// Check framebuffer completeness
	GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
		LOG_ERROR("Framebuffer incomplete during render: " + std::to_string(fbStatus));
		glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
		return;
	}

	glViewport(0, 0, m_Width, m_Height);

	glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Compute matrices (needed for grid even without mesh)
	float mvp[16], model[16], eyePos[3];
	ComputeMatrices(mvp, model, eyePos);

	// Draw ground grid and axes
	if (m_ShowGrid) {
		RenderGrid(mvp);
	}

	// Only draw mesh if we have one
	if (!m_HasMesh || m_VertexCount == 0) {
		glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
		glViewport(lastViewport[0], lastViewport[1], lastViewport[2], lastViewport[3]);
		glUseProgram(lastProgram);
		return;
	}

	// Configure OpenGL state for 3D rendering.
	// ImGui uses different state, so we must set everything explicitly.
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	if (m_BackfaceCulling) {
		glEnable(GL_CULL_FACE);
		// IMPORTANT: Cull front faces, not back. P3D models from Object Builder
		// have inverted winding order compared to standard OpenGL convention.
		glCullFace(GL_FRONT);
	} else {
		glDisable(GL_CULL_FACE);
	}
	// Enable alpha blending for transparent textures (glass, scopes, etc.)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Use our shader
	glUseProgram(m_ShaderProgram);

	// Set uniforms
	glUniformMatrix4fv(m_LocMVP, 1, GL_FALSE, mvp);
	glUniformMatrix4fv(m_LocModel, 1, GL_FALSE, model);
	glUniform3f(m_LocLightDir, m_LightDirX, m_LightDirY, m_LightDirZ);
	glUniform3f(m_LocLightColor, m_LightColorR, m_LightColorG, m_LightColorB);
	glUniform1f(m_LocAmbientIntensity, m_AmbientIntensity);
	glUniform3f(m_LocViewPos, eyePos[0], eyePos[1], eyePos[2]);

	// One material per slot, defaulting anything unset so the array is whole.
	float ambient[MAX_TEXTURE_SLOTS * 4];
	float diffuse[MAX_TEXTURE_SLOTS * 4];
	float specular[MAX_TEXTURE_SLOTS * 4];
	float emissive[MAX_TEXTURE_SLOTS * 4];
	float specularPower[MAX_TEXTURE_SLOTS];

	for (int i = 0; i < MAX_TEXTURE_SLOTS; i++) {
		const MaterialProperties& mat =
			(i < static_cast<int>(m_TextureSlots.size()) && m_TextureSlots[i].material.hasRvmat)
				? m_TextureSlots[i].material
				: m_DefaultMaterial;

		for (int c = 0; c < 4; c++) {
			ambient[i * 4 + c] = mat.ambient[c];
			diffuse[i * 4 + c] = mat.diffuse[c];
			specular[i * 4 + c] = mat.specular[c];
			emissive[i * 4 + c] = mat.emissive[c];
		}
		specularPower[i] = mat.specularPower;
	}

	glUniform4fv(m_LocMatAmbient, MAX_TEXTURE_SLOTS, ambient);
	glUniform4fv(m_LocMatDiffuse, MAX_TEXTURE_SLOTS, diffuse);
	glUniform4fv(m_LocMatSpecular, MAX_TEXTURE_SLOTS, specular);
	glUniform4fv(m_LocMatEmissive, MAX_TEXTURE_SLOTS, emissive);
	glUniform1fv(m_LocMatSpecularPower, MAX_TEXTURE_SLOTS, specularPower);
	glUniform1i(m_LocShaderStyle, m_ShaderStyle);

	// Bind all active texture slots to texture units 0-15
	// Each slot gets its own texture unit for per-vertex texture indexing
	int activeTextureCount = 0;
	for (size_t i = 0; i < m_TextureSlots.size() && i < MAX_TEXTURE_SLOTS; i++) {
		const auto& slot = m_TextureSlots[i];
		if (slot.active && slot.textureId != 0) {
			glActiveTexture(GL_TEXTURE0 + (GLenum)i);
			glBindTexture(GL_TEXTURE_2D, slot.textureId);
			if (m_LocTextures[i] >= 0) {
				glUniform1i(m_LocTextures[i], static_cast<int>(i));
			}
			activeTextureCount = static_cast<int>(i) + 1;  // Track highest used slot + 1
		}
	}

	// Fall back to legacy diffuse texture if no slots loaded
	if (activeTextureCount == 0 && m_DiffuseTexture != 0) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_DiffuseTexture);
		if (m_LocTextures[0] >= 0) {
			glUniform1i(m_LocTextures[0], 0);
		}
		activeTextureCount = 1;
	}

	// Set texture count uniform
	if (m_LocTextureCount >= 0) {
		glUniform1i(m_LocTextureCount, activeTextureCount);
	}

	// Bind normal map to texture unit 16 (units 0-15 are for diffuse textures)
	if (m_NormalTexture != 0) {
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, m_NormalTexture);
		glUniform1i(m_LocNormalMap, 16);
		glUniform1i(m_LocHasNormalMap, 1);
	} else {
		glUniform1i(m_LocHasNormalMap, 0);
	}

	// Bind specular/SMDI map to texture unit 17
	if (m_SpecularTexture != 0) {
		glActiveTexture(GL_TEXTURE17);
		glBindTexture(GL_TEXTURE_2D, m_SpecularTexture);
		glUniform1i(m_LocSpecularMap, 17);
		glUniform1i(m_LocHasSpecularMap, 1);
	} else {
		glUniform1i(m_LocHasSpecularMap, 0);
	}

	// Draw
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLES, 0, m_VertexCount);
	glBindVertexArray(0);

	// Restore OpenGL state
	glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
	glViewport(lastViewport[0], lastViewport[1], lastViewport[2], lastViewport[3]);
	glUseProgram(lastProgram);
}

/// Compute MVP, model matrix, and eye position for the current camera state.
/// Uses an orbit camera model: camera orbits around target point at fixed distance.
void ModelRenderer::ComputeMatrices(float* mvp, float* model, float* eyePos) {
	const float PI = 3.14159265359f;
	float yawRad = m_CameraYaw * PI / 180.0f;
	float pitchRad = m_CameraPitch * PI / 180.0f;

	// Convert spherical coordinates (yaw, pitch, distance) to Cartesian camera position
	eyePos[0] = m_TargetX + m_CameraDistance * cosf(pitchRad) * sinf(yawRad);
	eyePos[1] = m_TargetY + m_CameraDistance * sinf(pitchRad);
	eyePos[2] = m_TargetZ + m_CameraDistance * cosf(pitchRad) * cosf(yawRad);

	// Scene radius must encompass both the model and the grid (±10 units).
	// Using only m_MeshSize caused the grid (and model edges) to be clipped
	// when viewed from the side, since the far plane was too close.
	float gridRadius = m_ShowGrid ? 15.0f : 0.0f;
	float sceneRadius = std::max(m_MeshSize * 1.5f, gridRadius);
	float nearPlane = 0.1f;
	float farPlane = m_CameraDistance + sceneRadius * 2.0f;

	// Projection matrix
	float proj[16];
	float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
	Mat4Perspective(proj, 0.8f, aspect, nearPlane, farPlane);  // ~45 degree FOV

	// View matrix
	float view[16];
	Mat4LookAt(view, eyePos[0], eyePos[1], eyePos[2], m_TargetX, m_TargetY, m_TargetZ, 0.0f, 1.0f, 0.0f);

	// Model matrix: translate mesh so its center is at origin
	Mat4Translation(model, -m_MeshCenter[0], -m_MeshCenter[1], -m_MeshCenter[2]);

	// MVP = Projection * View * Model
	float temp[16];
	Mat4Multiply(temp, view, model);
	Mat4Multiply(mvp, proj, temp);
}

// Matrix helper implementations
void ModelRenderer::Mat4Identity(float* m) {
	memset(m, 0, 16 * sizeof(float));
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void ModelRenderer::Mat4Multiply(float* result, const float* a, const float* b) {
	// Column-major multiplication: result = a * b
	// For column-major: element at (row, col) is at index col*4 + row
	float temp[16];
	for (int col = 0; col < 4; col++) {
		for (int row = 0; row < 4; row++) {
			float sum = 0.0f;
			for (int k = 0; k < 4; k++) {
				// a[row, k] * b[k, col]
				// a[row, k] is at a[k*4 + row]
				// b[k, col] is at b[col*4 + k]
				sum += a[k * 4 + row] * b[col * 4 + k];
			}
			temp[col * 4 + row] = sum;
		}
	}
	memcpy(result, temp, 16 * sizeof(float));
}

void ModelRenderer::Mat4Perspective(float* m, float fovY, float aspect, float nearZ, float farZ) {
	float f = 1.0f / tanf(fovY * 0.5f);
	memset(m, 0, 16 * sizeof(float));
	m[0] = f / aspect;
	m[5] = f;
	m[10] = (farZ + nearZ) / (nearZ - farZ);
	m[11] = -1.0f;
	m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
}

void ModelRenderer::Mat4LookAt(float* m, float eyeX, float eyeY, float eyeZ, float centerX, float centerY,
							   float centerZ, float upX, float upY, float upZ) {
	// Forward vector (from eye to center)
	float fx = centerX - eyeX;
	float fy = centerY - eyeY;
	float fz = centerZ - eyeZ;
	float flen = sqrtf(fx * fx + fy * fy + fz * fz);
	fx /= flen;
	fy /= flen;
	fz /= flen;

	// Right vector (cross product of forward and up)
	float sx = fy * upZ - fz * upY;
	float sy = fz * upX - fx * upZ;
	float sz = fx * upY - fy * upX;
	float slen = sqrtf(sx * sx + sy * sy + sz * sz);
	sx /= slen;
	sy /= slen;
	sz /= slen;

	// Recompute up vector (cross product of right and forward)
	float ux = sy * fz - sz * fy;
	float uy = sz * fx - sx * fz;
	float uz = sx * fy - sy * fx;

	// Build view matrix (column-major)
	m[0] = sx;
	m[4] = sy;
	m[8] = sz;
	m[12] = -(sx * eyeX + sy * eyeY + sz * eyeZ);
	m[1] = ux;
	m[5] = uy;
	m[9] = uz;
	m[13] = -(ux * eyeX + uy * eyeY + uz * eyeZ);
	m[2] = -fx;
	m[6] = -fy;
	m[10] = -fz;
	m[14] = (fx * eyeX + fy * eyeY + fz * eyeZ);
	m[3] = 0;
	m[7] = 0;
	m[11] = 0;
	m[15] = 1;
}

void ModelRenderer::Mat4Translation(float* m, float x, float y, float z) {
	Mat4Identity(m);
	m[12] = x;
	m[13] = y;
	m[14] = z;
}

//=============================================================================
// Quaternion helpers for arcball camera
// Quaternions avoid gimbal lock that occurs with Euler angles (yaw/pitch).
// Used for smooth, intuitive camera rotation in the 3D viewport.
//=============================================================================

/// Hamilton product: result = a * b
void ModelRenderer::QuatMultiply(float* result, const float* a, const float* b) {
	// Hamilton product: result = a * b
	// q = (x, y, z, w)
	float x = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
	float y = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
	float z = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
	float w = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
	result[0] = x;
	result[1] = y;
	result[2] = z;
	result[3] = w;
}

void ModelRenderer::QuatNormalize(float* q) {
	float len = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
	if (len > 0.0001f) {
		q[0] /= len;
		q[1] /= len;
		q[2] /= len;
		q[3] /= len;
	}
}

void ModelRenderer::QuatFromAxisAngle(float* q, float axisX, float axisY, float axisZ, float angle) {
	// Normalize axis
	float len = sqrtf(axisX * axisX + axisY * axisY + axisZ * axisZ);
	if (len < 0.0001f) {
		q[0] = 0;
		q[1] = 0;
		q[2] = 0;
		q[3] = 1;
		return;
	}
	axisX /= len;
	axisY /= len;
	axisZ /= len;

	float halfAngle = angle * 0.5f;
	float sinHalf = sinf(halfAngle);
	q[0] = axisX * sinHalf;
	q[1] = axisY * sinHalf;
	q[2] = axisZ * sinHalf;
	q[3] = cosf(halfAngle);
}

void ModelRenderer::QuatToMatrix(float* m, const float* q) {
	// Convert quaternion to 4x4 rotation matrix (column-major)
	float x = q[0], y = q[1], z = q[2], w = q[3];

	float x2 = x + x, y2 = y + y, z2 = z + z;
	float xx = x * x2, xy = x * y2, xz = x * z2;
	float yy = y * y2, yz = y * z2, zz = z * z2;
	float wx = w * x2, wy = w * y2, wz = w * z2;

	m[0] = 1.0f - (yy + zz);
	m[4] = xy - wz;
	m[8] = xz + wy;
	m[12] = 0.0f;
	m[1] = xy + wz;
	m[5] = 1.0f - (xx + zz);
	m[9] = yz - wx;
	m[13] = 0.0f;
	m[2] = xz - wy;
	m[6] = yz + wx;
	m[10] = 1.0f - (xx + yy);
	m[14] = 0.0f;
	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = 0.0f;
	m[15] = 1.0f;
}

void ModelRenderer::UpdateAnglesFromQuaternion() {
	// Extract yaw/pitch from quaternion for UI display
	// This is approximate but good enough for display purposes
	float x = m_Quaternion[0], y = m_Quaternion[1], z = m_Quaternion[2], w = m_Quaternion[3];

	// Calculate pitch (rotation around X axis)
	float sinp = 2.0f * (w * x - z * y);
	if (fabsf(sinp) >= 1.0f) {
		m_CameraPitch = copysignf(90.0f, sinp);	 // Gimbal lock
	} else {
		m_CameraPitch = asinf(sinp) * 180.0f / 3.14159265f;
	}

	// Calculate yaw (rotation around Y axis)
	float siny_cosp = 2.0f * (w * y + x * z);
	float cosy_cosp = 1.0f - 2.0f * (x * x + y * y);
	m_CameraYaw = atan2f(siny_cosp, cosy_cosp) * 180.0f / 3.14159265f;
}

void ModelRenderer::UpdateQuaternionFromAngles() {
	// Build quaternion from yaw/pitch angles
	const float PI = 3.14159265f;
	float yawRad = m_CameraYaw * PI / 180.0f;
	float pitchRad = m_CameraPitch * PI / 180.0f;

	// Create rotation quaternions for yaw (Y axis) and pitch (X axis)
	float qYaw[4], qPitch[4];
	QuatFromAxisAngle(qYaw, 0, 1, 0, yawRad);
	QuatFromAxisAngle(qPitch, 1, 0, 0, pitchRad);

	// Combine: first pitch, then yaw
	QuatMultiply(m_Quaternion, qYaw, qPitch);
	QuatNormalize(m_Quaternion);
}

//=============================================================================
// Arcball camera rotation
// Maps mouse drag to intuitive 3D rotation like rotating a trackball.
//=============================================================================

/// Rotate camera based on mouse drag delta.
/// Horizontal drag rotates around world Y axis, vertical around camera's right axis.
void ModelRenderer::RotateArcball(float deltaX, float deltaY, float viewportWidth, float viewportHeight) {
	const float PI = 3.14159265f;
	float sensitivity = 0.005f;

	// Convert mouse pixels to rotation angles
	float angleY = -deltaX * sensitivity;  // Horizontal = yaw (around world Y)
	float angleX = -deltaY * sensitivity;  // Vertical = pitch (around camera right)

	// Create rotation quaternions
	float qYaw[4], qPitch[4];

	// Yaw: always rotate around world Y axis
	QuatFromAxisAngle(qYaw, 0, 1, 0, angleY);

	// Pitch: rotate around camera's right vector
	// First, get the camera's right vector from current orientation
	float rotMatrix[16];
	QuatToMatrix(rotMatrix, m_Quaternion);

	// Right vector is the first column of the rotation matrix
	float rightX = rotMatrix[0];
	float rightY = rotMatrix[1];
	float rightZ = rotMatrix[2];

	QuatFromAxisAngle(qPitch, rightX, rightY, rightZ, angleX);

	// Combine rotations: new_rotation = yaw * pitch * current
	float temp[4];
	QuatMultiply(temp, qYaw, qPitch);
	QuatMultiply(m_Quaternion, temp, m_Quaternion);
	QuatNormalize(m_Quaternion);

	// Update yaw/pitch for UI display (approximate values)
	UpdateAnglesFromQuaternion();
}

void ModelRenderer::ResetCamera() {
	// Reset to default isometric-ish view
	m_CameraYaw = 45.0f;
	m_CameraPitch = 20.0f;
	m_TargetX = 0.0f;
	m_TargetY = 0.0f;
	m_TargetZ = 0.0f;

	// Update quaternion to match
	UpdateQuaternionFromAngles();

	// Reset distance if we have a mesh
	if (m_HasMesh) {
		m_CameraDistance = m_MeshSize * 2.5f;
		if (m_CameraDistance < 1.0f)
			m_CameraDistance = 3.0f;
	} else {
		m_CameraDistance = 5.0f;
	}
}

void ModelRenderer::FrameObject() {
	// Frame the camera to fit the loaded mesh
	if (!m_HasMesh)
		return;

	// Reset target to mesh center (model transform centers it at origin)
	m_TargetX = 0.0f;
	m_TargetY = 0.0f;
	m_TargetZ = 0.0f;

	// Calculate distance to fit object in view
	// For a 45-degree FOV, distance = size / (2 * tan(fov/2))
	float fovY = 0.8f;	// ~45 degrees in radians
	float fitDistance = (m_MeshSize * 1.2f) / (2.0f * tanf(fovY * 0.5f));
	m_CameraDistance = std::max(1.0f, fitDistance);
}

bool ModelRenderer::RaycastTest(float* hitDistance) const {
	if (!m_HasMesh)
		return false;

	const float PI = 3.14159265359f;
	float yawRad = m_CameraYaw * PI / 180.0f;
	float pitchRad = m_CameraPitch * PI / 180.0f;

	// Camera position
	float eyeX = m_TargetX + m_CameraDistance * cosf(pitchRad) * sinf(yawRad);
	float eyeY = m_TargetY + m_CameraDistance * sinf(pitchRad);
	float eyeZ = m_TargetZ + m_CameraDistance * cosf(pitchRad) * cosf(yawRad);

	// Ray direction (from camera to target)
	float dirX = m_TargetX - eyeX;
	float dirY = m_TargetY - eyeY;
	float dirZ = m_TargetZ - eyeZ;
	float dirLen = sqrtf(dirX * dirX + dirY * dirY + dirZ * dirZ);
	dirX /= dirLen;
	dirY /= dirLen;
	dirZ /= dirLen;

	// Mesh bounds (centered at origin after model transform)
	float halfSize = m_MeshSize * 0.5f;
	float minX = -halfSize, maxX = halfSize;
	float minY = -halfSize, maxY = halfSize;
	float minZ = -halfSize, maxZ = halfSize;

	// Ray-AABB intersection (slab method)
	float tmin = -1e30f, tmax = 1e30f;

	// X slab
	if (fabsf(dirX) > 1e-6f) {
		float t1 = (minX - eyeX) / dirX;
		float t2 = (maxX - eyeX) / dirX;
		if (t1 > t2) {
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		tmin = std::max(tmin, t1);
		tmax = std::min(tmax, t2);
	}

	// Y slab
	if (fabsf(dirY) > 1e-6f) {
		float t1 = (minY - eyeY) / dirY;
		float t2 = (maxY - eyeY) / dirY;
		if (t1 > t2) {
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		tmin = std::max(tmin, t1);
		tmax = std::min(tmax, t2);
	}

	// Z slab
	if (fabsf(dirZ) > 1e-6f) {
		float t1 = (minZ - eyeZ) / dirZ;
		float t2 = (maxZ - eyeZ) / dirZ;
		if (t1 > t2) {
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		tmin = std::max(tmin, t1);
		tmax = std::min(tmax, t2);
	}

	if (tmin > tmax || tmax < 0) {
		return false;  // No intersection
	}

	if (hitDistance) {
		*hitDistance = tmin > 0 ? tmin : tmax;
	}
	return true;
}

bool ModelRenderer::LoadTexture(const std::string& path) {
	if (!m_Initialized)
		return false;

	// Clear existing texture
	ClearTexture();

	// Check file extension
	std::filesystem::path filePath(path);
	std::string ext = filePath.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext != ".paa") {
		LOG_ERROR("Only PAA textures are supported: " + path);
		return false;
	}

	// Check if file exists
	if (!std::filesystem::exists(filePath)) {
		LOG_ERROR("Texture file not found: " + path);
		return false;
	}

	// Load PAA
	PAATexture paa = PAALoader::Load(path);
	if (!paa.valid) {
		LOG_ERROR("Failed to load PAA: " + path);
		return false;
	}

	// Upload to OpenGL
	if (!PAALoader::Upload(paa)) {
		LOG_ERROR("Failed to upload texture to GPU");
		return false;
	}

	m_DiffuseTexture = paa.textureId;
	m_DiffusePath = path;

	LOG_INFO("Texture loaded: " + std::to_string(paa.width) + "x" + std::to_string(paa.height) + " " +
			 PAALoader::GetTypeName(paa.type));

	return true;
}

void ModelRenderer::ClearTexture() {
	if (m_DiffuseTexture != 0) {
		glDeleteTextures(1, &m_DiffuseTexture);
		m_DiffuseTexture = 0;
	}
	m_DiffusePath.clear();
}

bool ModelRenderer::LoadDiffuseTexture(const std::string& path) {
	return LoadTexture(path);  // Same as LoadTexture
}

bool ModelRenderer::LoadNormalTexture(const std::string& path) {
	if (!m_Initialized)
		return false;

	// Clear existing normal texture
	if (m_NormalTexture != 0) {
		glDeleteTextures(1, &m_NormalTexture);
		m_NormalTexture = 0;
	}
	m_NormalPath.clear();

	// Check file extension
	std::filesystem::path filePath(path);
	std::string ext = filePath.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext != ".paa") {
		LOG_ERROR("Only PAA textures are supported: " + path);
		return false;
	}

	if (!std::filesystem::exists(filePath)) {
		LOG_ERROR("Normal map not found: " + path);
		return false;
	}

	PAATexture paa = PAALoader::Load(path);
	if (!paa.valid) {
		LOG_ERROR("Failed to load normal map: " + path);
		return false;
	}

	if (!PAALoader::Upload(paa)) {
		LOG_ERROR("Failed to upload normal map to GPU");
		return false;
	}

	m_NormalTexture = paa.textureId;
	m_NormalPath = path;
	LOG_INFO("Normal map loaded: " + std::to_string(paa.width) + "x" + std::to_string(paa.height));
	return true;
}

bool ModelRenderer::LoadSpecularTexture(const std::string& path) {
	if (!m_Initialized)
		return false;

	// Clear existing specular texture
	if (m_SpecularTexture != 0) {
		glDeleteTextures(1, &m_SpecularTexture);
		m_SpecularTexture = 0;
	}
	m_SpecularPath.clear();

	std::filesystem::path filePath(path);
	std::string ext = filePath.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext != ".paa") {
		LOG_ERROR("Only PAA textures are supported: " + path);
		return false;
	}

	if (!std::filesystem::exists(filePath)) {
		LOG_ERROR("Specular map not found: " + path);
		return false;
	}

	PAATexture paa = PAALoader::Load(path);
	if (!paa.valid) {
		LOG_ERROR("Failed to load specular map: " + path);
		return false;
	}

	if (!PAALoader::Upload(paa)) {
		LOG_ERROR("Failed to upload specular map to GPU");
		return false;
	}

	m_SpecularTexture = paa.textureId;
	m_SpecularPath = path;
	LOG_INFO("Specular map loaded: " + std::to_string(paa.width) + "x" + std::to_string(paa.height));
	return true;
}

void ModelRenderer::ClearAllTextures() {
	ClearTexture();
	if (m_NormalTexture != 0) {
		glDeleteTextures(1, &m_NormalTexture);
		m_NormalTexture = 0;
	}
	m_NormalPath.clear();
	if (m_SpecularTexture != 0) {
		glDeleteTextures(1, &m_SpecularTexture);
		m_SpecularTexture = 0;
	}
	m_SpecularPath.clear();
}

bool ModelRenderer::LoadAllTextures(const std::string& basePath) {
	if (!m_Initialized)
		return false;

	// Clear any existing textures
	ClearAllTextures();

	// basePath could be a full texture path like "model_co.paa" or just a base
	// name
	std::filesystem::path texPath(basePath);
	std::string stem = texPath.stem().string();
	std::string dir = texPath.parent_path().string();

	// Remove common suffixes to get base name
	std::vector<std::string> suffixes = {"_co", "_nohq", "_smdi", "_as", "_ca", "_nopx", "_dt", "_mc"};
	std::string baseName = stem;
	for (const auto& suffix : suffixes) {
		if (stem.length() > suffix.length()) {
			std::string ending = stem.substr(stem.length() - suffix.length());
			std::string endingLower = ending;
			std::transform(endingLower.begin(), endingLower.end(), endingLower.begin(), ::tolower);
			if (endingLower == suffix) {
				baseName = stem.substr(0, stem.length() - suffix.length());
				break;
			}
		}
	}

	LOG_INFO("Loading all textures for base: " + baseName);

	int loadedCount = 0;

	// Try to load diffuse (_co)
	for (const auto& suffix : {"_co", "_CO", "_Co"}) {
		std::string coPath = dir + "/" + baseName + suffix + ".paa";
		if (std::filesystem::exists(coPath)) {
			if (LoadTexture(coPath))
				loadedCount++;
			break;
		}
	}

	// Try to load normal map (_nohq)
	for (const auto& suffix : {"_nohq", "_NOHQ", "_Nohq", "_nopx", "_NOPX"}) {
		std::string nohqPath = dir + "/" + baseName + suffix + ".paa";
		if (std::filesystem::exists(nohqPath)) {
			if (LoadNormalTexture(nohqPath))
				loadedCount++;
			break;
		}
	}

	// Try to load specular/SMDI map (_smdi)
	for (const auto& suffix : {"_smdi", "_SMDI", "_Smdi", "_as", "_AS"}) {
		std::string smdiPath = dir + "/" + baseName + suffix + ".paa";
		if (std::filesystem::exists(smdiPath)) {
			if (LoadSpecularTexture(smdiPath))
				loadedCount++;
			break;
		}
	}

	LOG_INFO("Loaded " + std::to_string(loadedCount) + " texture(s)");
	return loadedCount > 0;
}

//=============================================================================
// Multi-texture slot system
// P3D models can have multiple materials, each with its own texture set.
// This system manages up to 16 texture slots (GPU texture unit limit).
//=============================================================================

/// Add a texture to the next available slot. Returns slot index or -1 on failure.
int ModelRenderer::AddTextureSlot(const std::string& path) {
	if (!m_Initialized)
		return -1;

	// Check if already loaded
	for (size_t i = 0; i < m_TextureSlots.size(); i++) {
		if (m_TextureSlots[i].path == path) {
			LOG_INFO("Texture already loaded in slot " + std::to_string(i));
			return static_cast<int>(i);
		}
	}

	// Check file extension
	std::filesystem::path filePath(path);
	std::string ext = filePath.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext != ".paa") {
		LOG_ERROR("Only PAA textures are supported: " + path);
		return -1;
	}

	if (!std::filesystem::exists(filePath)) {
		LOG_ERROR("Texture not found: " + path);
		return -1;
	}

	// Load PAA
	PAATexture paa = PAALoader::Load(path);
	if (!paa.valid) {
		LOG_ERROR("Failed to load texture: " + path);
		return -1;
	}

	if (!PAALoader::Upload(paa)) {
		LOG_ERROR("Failed to upload texture to GPU: " + path);
		return -1;
	}

	// Create slot
	TextureSlot slot;
	slot.textureId = paa.textureId;
	slot.path = path;
	slot.name = filePath.filename().string();
	slot.width = paa.width;
	slot.height = paa.height;
	slot.active = true;

	m_TextureSlots.push_back(slot);
	int index = static_cast<int>(m_TextureSlots.size()) - 1;

	// Auto-activate if first texture
	if (m_ActiveTextureSlot < 0) {
		m_ActiveTextureSlot = index;
	}

	LOG_INFO("Added texture slot " + std::to_string(index) + ": " + slot.name + " (" + std::to_string(slot.width) +
			 "x" + std::to_string(slot.height) + ")");

	return index;
}

void ModelRenderer::RemoveTextureSlot(int index) {
	if (index < 0 || index >= static_cast<int>(m_TextureSlots.size()))
		return;

	// Delete GL textures (diffuse, normal, specular)
	if (m_TextureSlots[index].textureId != 0) {
		glDeleteTextures(1, &m_TextureSlots[index].textureId);
	}
	if (m_TextureSlots[index].normalId != 0) {
		glDeleteTextures(1, &m_TextureSlots[index].normalId);
	}
	if (m_TextureSlots[index].specularId != 0) {
		glDeleteTextures(1, &m_TextureSlots[index].specularId);
	}

	LOG_INFO("Removed texture slot: " + m_TextureSlots[index].name);

	m_TextureSlots.erase(m_TextureSlots.begin() + index);

	// Adjust active slot
	if (m_ActiveTextureSlot == index) {
		m_ActiveTextureSlot = m_TextureSlots.empty() ? -1 : 0;
	} else if (m_ActiveTextureSlot > index) {
		m_ActiveTextureSlot--;
	}
}

void ModelRenderer::SetActiveTextureSlot(int index) {
	if (index < -1 || index >= static_cast<int>(m_TextureSlots.size()))
		return;
	m_ActiveTextureSlot = index;
}

void ModelRenderer::ClearTextureSlots() {
	for (auto& slot : m_TextureSlots) {
		if (slot.textureId != 0) {
			glDeleteTextures(1, &slot.textureId);
		}
		if (slot.normalId != 0) {
			glDeleteTextures(1, &slot.normalId);
		}
		if (slot.specularId != 0) {
			glDeleteTextures(1, &slot.specularId);
		}
	}
	m_TextureSlots.clear();
	m_ActiveTextureSlot = -1;
	LOG_INFO("Cleared all texture slots");
}

//=============================================================================
// Material system
// Integrates with Arma 3's RVMAT material definitions.
//=============================================================================

/// Returns the material properties for the currently active texture slot.
/// Falls back to default material if no RVMAT is loaded.
// Grouped by what the wiki says each shader does, since the families differ
// far more from each other than the members differ within one.
ModelRenderer::ShaderStyle ModelRenderer::StyleForPixelShader(const std::string& pixelShaderID) {
	std::string id = pixelShaderID;
	std::transform(id.begin(), id.end(), id.begin(), ::tolower);

	const auto has = [&id](const char* needle) {
		return id.find(needle) != std::string::npos;
	};

	if (has("collimator") || has("alphaonly") || has("point") || has("star")) {
		return ShaderStyle::Unlit;
	}
	if (has("glass") || has("refract") || has("water")) {
		return ShaderStyle::Glass;
	}
	if (has("tree") || has("grass") || has("crown") || has("leaf")) {
		return ShaderStyle::Foliage;
	}
	// The families with no normal map stage of their own.
	if (id == "normal" || id == "normaldxta" || id == "white" || id == "whitealpha" ||
		id == "detail" || id == "interpolation" || id == "dummy0") {
		return ShaderStyle::Basic;
	}

	return ShaderStyle::Super;
}

const MaterialProperties& ModelRenderer::GetActiveMaterial() const {
	// Return material from first active texture slot that has RVMAT
	for (const auto& slot : m_TextureSlots) {
		if (slot.active && slot.material.hasRvmat) {
			return slot.material;
		}
	}
	// Return default material
	return m_DefaultMaterial;
}

/// Find the RVMAT file associated with a texture by naming convention.
/// Arma 3 convention: texture_co.paa has material texture.rvmat
static std::string FindRvmatPath(const std::string& texturePath) {
	std::filesystem::path texPath(texturePath);
	std::string stem = texPath.stem().string();
	std::string dir = texPath.parent_path().string();

	// Remove texture suffix to get base name
	std::vector<std::string> suffixes = {"_co", "_CO", "_Co", "_ca", "_CA", "_dt", "_DT"};
	std::string baseName = stem;
	for (const auto& suffix : suffixes) {
		if (stem.length() > suffix.length()) {
			std::string ending = stem.substr(stem.length() - suffix.length());
			if (ending == suffix) {
				baseName = stem.substr(0, stem.length() - suffix.length());
				break;
			}
		}
	}

	// Try common RVMAT naming patterns
	std::vector<std::string> rvmatPatterns = {
		dir + "/" + baseName + ".rvmat",
		dir + "/" + stem + ".rvmat",
		dir + "/" + baseName + "_default.rvmat",
	};

	for (const auto& pattern : rvmatPatterns) {
		if (std::filesystem::exists(pattern)) {
			return pattern;
		}
	}

	return "";	// Not found
}

/// Find related texture (normal _nohq, specular _smdi) from diffuse texture path.
/// Uses Arma 3 naming conventions to locate texture variants.
static std::string FindRelatedTexture(const std::string& basePath, const std::vector<std::string>& suffixes) {
	std::filesystem::path texPath(basePath);
	std::string stem = texPath.stem().string();
	std::string dir = texPath.parent_path().string();

	// Remove _co suffix if present
	std::string baseName = stem;
	std::vector<std::string> coSuffixes = {"_co", "_CO", "_Co"};
	for (const auto& suffix : coSuffixes) {
		if (stem.length() > suffix.length()) {
			std::string ending = stem.substr(stem.length() - suffix.length());
			if (ending == suffix) {
				baseName = stem.substr(0, stem.length() - suffix.length());
				break;
			}
		}
	}

	// Try each suffix
	for (const auto& suffix : suffixes) {
		std::string path = dir + "/" + baseName + suffix + ".paa";
		if (std::filesystem::exists(path)) {
			return path;
		}
	}

	return "";	// Not found
}

/// Add texture with full material support: loads diffuse, normal, specular, and RVMAT.
/// This is the preferred method for loading P3D model textures.
int ModelRenderer::AddTextureSlotWithMaterial(const std::string& path) {
	if (!m_Initialized)
		return -1;

	// Avoid duplicate loading
	for (size_t i = 0; i < m_TextureSlots.size(); i++) {
		if (m_TextureSlots[i].path == path) {
			LOG_INFO("Texture already loaded in slot " + std::to_string(i));
			return static_cast<int>(i);
		}
	}

	// Check file extension
	std::filesystem::path filePath(path);
	std::string ext = filePath.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext != ".paa") {
		LOG_ERROR("Only PAA textures are supported: " + path);
		return -1;
	}

	if (!std::filesystem::exists(filePath)) {
		LOG_ERROR("Texture not found: " + path);
		return -1;
	}

	// Load diffuse PAA
	PAATexture paa = PAALoader::Load(path);
	if (!paa.valid) {
		LOG_ERROR("Failed to load texture: " + path);
		return -1;
	}

	if (!PAALoader::Upload(paa)) {
		LOG_ERROR("Failed to upload texture to GPU: " + path);
		return -1;
	}

	// Create slot
	TextureSlot slot;
	slot.textureId = paa.textureId;
	slot.path = path;
	slot.name = filePath.filename().string();
	slot.width = paa.width;
	slot.height = paa.height;
	slot.active = true;

	// Try to find and load RVMAT
	try {
		std::string rvmatPath = FindRvmatPath(path);
		if (!rvmatPath.empty()) {
			LOG_INFO("Found RVMAT: " + rvmatPath);
			auto material = rvmat::Parser::parseFile(rvmatPath);
			if (material) {
				slot.material.hasRvmat = true;
				slot.material.rvmatPath = rvmatPath;
				slot.material.ambient[0] = material->ambient.r;
				slot.material.ambient[1] = material->ambient.g;
				slot.material.ambient[2] = material->ambient.b;
				slot.material.ambient[3] = material->ambient.a;
				slot.material.diffuse[0] = material->diffuse.r;
				slot.material.diffuse[1] = material->diffuse.g;
				slot.material.diffuse[2] = material->diffuse.b;
				slot.material.diffuse[3] = material->diffuse.a;
				slot.material.specular[0] = material->specular.r;
				slot.material.specular[1] = material->specular.g;
				slot.material.specular[2] = material->specular.b;
				slot.material.specular[3] = material->specular.a;
				slot.material.emissive[0] = material->emissive.r;
				slot.material.emissive[1] = material->emissive.g;
				slot.material.emissive[2] = material->emissive.b;
				slot.material.emissive[3] = material->emissive.a;
				slot.material.specularPower = material->specularPower;
				LOG_INFO("  Specular power: " + std::to_string(material->specularPower));

				// Get texture paths from RVMAT stages
				for (const auto& [stageNum, stage] : material->stages) {
					try {
						if (stageNum == 1 && !stage.texture.empty()) {
							// Stage 1 is typically normal map
							std::filesystem::path resolved = rvmat::resolveTexturePath(stage.texture, rvmatPath);
							if (!resolved.empty() && std::filesystem::exists(resolved)) {
								slot.normalPath = resolved.string();
							}
						} else if (stageNum == 2 && !stage.texture.empty()) {
							// Stage 2 is typically specular/SMDI
							std::filesystem::path resolved = rvmat::resolveTexturePath(stage.texture, rvmatPath);
							if (!resolved.empty() && std::filesystem::exists(resolved)) {
								slot.specularPath = resolved.string();
							}
						}
					} catch (const std::exception& e) {
						LOG_ERROR("Error resolving stage texture: " + std::string(e.what()));
					}
				}
			}
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Error loading RVMAT: " + std::string(e.what()));
	}

	// Try to find normal map by naming convention if not in RVMAT
	try {
		if (slot.normalPath.empty()) {
			slot.normalPath = FindRelatedTexture(path, {"_nohq", "_NOHQ", "_nopx", "_NOPX"});
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Error finding normal map: " + std::string(e.what()));
	}

	// Try to find specular map by naming convention if not in RVMAT
	try {
		if (slot.specularPath.empty()) {
			slot.specularPath = FindRelatedTexture(path, {"_smdi", "_SMDI", "_as", "_AS"});
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Error finding specular map: " + std::string(e.what()));
	}

	// Load normal map if found
	try {
		if (!slot.normalPath.empty() && std::filesystem::exists(slot.normalPath)) {
			PAATexture normalPaa = PAALoader::Load(slot.normalPath);
			if (normalPaa.valid && PAALoader::Upload(normalPaa)) {
				slot.normalId = normalPaa.textureId;
				LOG_INFO("  Loaded normal map: " + std::filesystem::path(slot.normalPath).filename().string());
			}
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Error loading normal map: " + std::string(e.what()));
	}

	// Load specular map if found
	try {
		if (!slot.specularPath.empty() && std::filesystem::exists(slot.specularPath)) {
			PAATexture specPaa = PAALoader::Load(slot.specularPath);
			if (specPaa.valid && PAALoader::Upload(specPaa)) {
				slot.specularId = specPaa.textureId;
				LOG_INFO("  Loaded specular map: " + std::filesystem::path(slot.specularPath).filename().string());
			}
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Error loading specular map: " + std::string(e.what()));
	}

	m_TextureSlots.push_back(slot);
	int index = static_cast<int>(m_TextureSlots.size()) - 1;

	// Auto-activate if first texture
	if (m_ActiveTextureSlot < 0) {
		m_ActiveTextureSlot = index;
	}

	LOG_INFO("Added texture slot " + std::to_string(index) + ": " + slot.name + " (" + std::to_string(slot.width) +
			 "x" + std::to_string(slot.height) + ")" + (slot.material.hasRvmat ? " [RVMAT]" : ""));

	return index;
}

} // namespace arma3

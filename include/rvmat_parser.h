#pragma once
/**
 * @file rvmat_parser.h
 * @brief RVMAT material file parser and serializer
 * @author Eathan McLeod-Lucas
 *
 * Part of A3 Studio - Arma 3 Modding IDE
 */

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace rvmat {

// Color with alpha (RGBA)
struct Color4 {
	float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

	Color4() = default;
	Color4(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {}

	float* data() { return &r; }
	const float* data() const { return &r; }

	// Conversion to std::array
	std::array<float, 4> toArray() const { return {r, g, b, a}; }

	// Conversion from std::array
	static Color4 fromArray(const std::array<float, 4>& arr) { return Color4(arr[0], arr[1], arr[2], arr[3]); }
};

// Texture stage configuration
struct Stage {
	std::string texture;
	std::string filter = "Anizotropic";	 // Anizotropic, Point, Linear, Trilinear
	std::string uvSource = "tex";		 // tex, tex1, pos, norm, worldPos, worldNorm, etc.

	// UV Transform
	struct UVTransform {
		std::array<float, 2> offset = {0.0f, 0.0f};
		std::array<float, 2> scale = {1.0f, 1.0f};
		float rotation = 0.0f;
	} uvTransform;
};

// Render flags
enum class RenderFlag {
	None = 0,
	NoZWrite = 1 << 0,
	NoColorWrite = 1 << 1,
	NoAlphaWrite = 1 << 2,
	AddBlend = 1 << 3,
	LandShadow = 1 << 4,
	AlphaTest32 = 1 << 5,
	AlphaTest64 = 1 << 6,
	AlphaTest128 = 1 << 7,
	Road = 1 << 8,
	NoTiWrite = 1 << 9,
	NoReceiveShadow = 1 << 10,
	NoLODBlend = 1 << 11,
};

inline RenderFlag operator|(RenderFlag a, RenderFlag b) {
	return static_cast<RenderFlag>(static_cast<int>(a) | static_cast<int>(b));
}

inline RenderFlag operator&(RenderFlag a, RenderFlag b) {
	return static_cast<RenderFlag>(static_cast<int>(a) & static_cast<int>(b));
}

// Main material definition
struct Material {
	// Basic lighting properties
	Color4 ambient = {1.0f, 1.0f, 1.0f, 1.0f};
	Color4 diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
	Color4 forcedDiffuse = {0.0f, 0.0f, 0.0f, 0.0f};
	Color4 emissive = {0.0f, 0.0f, 0.0f, 0.0f};
	Color4 specular = {0.0f, 0.0f, 0.0f, 1.0f};
	float specularPower = 1.0f;

	// Shader selection
	std::string pixelShaderID;
	std::string vertexShaderID;

	// Render settings
	std::string mainLight = "Sun";	// None, Sun, Sky, Horizon, Stars, etc.
	std::string fogMode = "fog";	// none, fog, alpha, fogAlpha, fogSky
	RenderFlag renderFlags = RenderFlag::None;

	// Texture stages (up to 8 typically)
	std::unordered_map<int, Stage> stages;

	// Source file path (for relative texture resolution)
	std::filesystem::path sourcePath;
};

class Parser {
public:
	// Parse RVMAT from file
	static std::optional<Material> parseFile(const std::filesystem::path& path);

	// Parse RVMAT from string
	static std::optional<Material> parseString(const std::string& content, const std::filesystem::path& basePath = "");

	// Serialize material to RVMAT format
	static std::string serialize(const Material& material);

	// Save material to file
	static bool saveFile(const Material& material, const std::filesystem::path& path);

private:
	struct Token {
		enum Type {
			Identifier,
			Number,
			String,
			LBrace,
			RBrace,
			LBracket,
			RBracket,
			Equals,
			Semicolon,
			Comma,
			EndOfFile,
			Error
		};
		Type type;
		std::string value;
		int line;
	};

	class Lexer {
	public:
		Lexer(const std::string& source);
		Token nextToken();
		Token peekToken();

	private:
		std::string source;
		size_t pos = 0;
		int line = 1;
		std::optional<Token> peeked;

		void skipWhitespaceAndComments();
		Token readIdentifier();
		Token readNumber();
		Token readString();
	};

	static bool parseProperty(Lexer& lexer, Material& material);
	static bool parseStage(Lexer& lexer, int stageNum, Material& material);
	static std::optional<Color4> parseColor(Lexer& lexer);
	static std::optional<std::vector<std::string>> parseStringArray(Lexer& lexer);
	static std::optional<float> parseFloat(Lexer& lexer);
};

// Helper to resolve texture paths (handles both absolute and relative)
std::filesystem::path resolveTexturePath(const std::string& texturePath, const std::filesystem::path& materialPath);

// MaterialProperties for undo/redo system (copyable subset of Material)
struct MaterialProperties {
	std::array<float, 4> ambient = {1, 1, 1, 1};
	std::array<float, 4> diffuse = {1, 1, 1, 1};
	std::array<float, 4> forcedDiffuse = {0, 0, 0, 0};
	std::array<float, 4> emissive = {0, 0, 0, 0};
	std::array<float, 4> specular = {0.5f, 0.5f, 0.5f, 1};
	float specularPower = 50.0f;
	std::string pixelShaderID = "NormalMap";
	std::string vertexShaderID = "NormalMap";
};

// Common shader presets
namespace Presets {
Material basicOpaque();
Material normalMapped();
Material glass();
Material terrain();
Material character();
Material vehicleMetal();
Material weaponMetal();
Material weaponPlastic();
Material staticObject();
}  // namespace Presets

// Object type for auto-generation
enum class ObjectType { Vehicle, Weapon, Character, Static, Unknown };

// Create material from template with auto-populated texture paths
Material createFromTemplate(ObjectType type, const std::string& texturePath = "");

// Generate RVMAT file for a texture
bool generateRvmatForTexture(const std::string& texturePath, ObjectType type, const std::string& outputPath);

}  // namespace rvmat

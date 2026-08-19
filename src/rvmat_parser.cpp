/**
 * @file rvmat_parser.cpp
 * @brief Parser and serializer for RVMAT material definition files
 * @author Eathan McLeod-Lucas
 *
 * Part of A3 Studio - Arma 3 Modding IDE
 */

#include "../include/rvmat_parser.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

namespace rvmat {

Parser::Lexer::Lexer(const std::string& source) : source(source) {}

void Parser::Lexer::skipWhitespaceAndComments() {
	while (pos < source.size()) {
		char c = source[pos];

		if (std::isspace(c)) {
			if (c == '\n')
				line++;
			pos++;
			continue;
		}

		if (c == '/' && pos + 1 < source.size() && source[pos + 1] == '/') {
			while (pos < source.size() && source[pos] != '\n')
				pos++;
			continue;
		}

		if (c == '/' && pos + 1 < source.size() && source[pos + 1] == '*') {
			pos += 2;
			while (pos + 1 < source.size() && !(source[pos] == '*' && source[pos + 1] == '/')) {
				if (source[pos] == '\n')
					line++;
				pos++;
			}
			pos += 2;
			continue;
		}

		break;
	}
}

Parser::Token Parser::Lexer::readIdentifier() {
	size_t start = pos;
	while (pos < source.size() && (std::isalnum(source[pos]) || source[pos] == '_')) {
		pos++;
	}
	return {Token::Identifier, source.substr(start, pos - start), line};
}

Parser::Token Parser::Lexer::readNumber() {
	size_t start = pos;
	bool hasDecimal = false;
	bool hasExponent = false;

	if (source[pos] == '-' || source[pos] == '+')
		pos++;

	while (pos < source.size()) {
		char c = source[pos];
		if (std::isdigit(c)) {
			pos++;
		} else if (c == '.' && !hasDecimal && !hasExponent) {
			hasDecimal = true;
			pos++;
		} else if ((c == 'e' || c == 'E') && !hasExponent) {
			hasExponent = true;
			pos++;
			if (pos < source.size() && (source[pos] == '+' || source[pos] == '-')) {
				pos++;
			}
		} else {
			break;
		}
	}
	return {Token::Number, source.substr(start, pos - start), line};
}

Parser::Token Parser::Lexer::readString() {
	pos++;	// skip opening quote
	size_t start = pos;
	while (pos < source.size() && source[pos] != '"') {
		if (source[pos] == '\\' && pos + 1 < source.size()) {
			pos += 2;
		} else {
			if (source[pos] == '\n')
				line++;
			pos++;
		}
	}
	std::string value = source.substr(start, pos - start);
	if (pos < source.size())
		pos++;	// skip closing quote
	return {Token::String, value, line};
}

Parser::Token Parser::Lexer::nextToken() {
	if (peeked) {
		Token t = *peeked;
		peeked.reset();
		return t;
	}

	skipWhitespaceAndComments();

	if (pos >= source.size()) {
		return {Token::EndOfFile, "", line};
	}

	char c = source[pos];

	if (std::isalpha(c) || c == '_') {
		return readIdentifier();
	}

	if (std::isdigit(c) || c == '-' || c == '+' || c == '.') {
		if (c == '.' && (pos + 1 >= source.size() || !std::isdigit(source[pos + 1]))) {
			pos++;
			return {Token::Error, ".", line};
		}
		if ((c == '-' || c == '+') &&
			(pos + 1 >= source.size() || (!std::isdigit(source[pos + 1]) && source[pos + 1] != '.'))) {
			pos++;
			return {Token::Error, std::string(1, c), line};
		}
		return readNumber();
	}

	if (c == '"') {
		return readString();
	}

	pos++;
	switch (c) {
		case '{':
			return {Token::LBrace, "{", line};
		case '}':
			return {Token::RBrace, "}", line};
		case '[':
			return {Token::LBracket, "[", line};
		case ']':
			return {Token::RBracket, "]", line};
		case '=':
			return {Token::Equals, "=", line};
		case ';':
			return {Token::Semicolon, ";", line};
		case ',':
			return {Token::Comma, ",", line};
		default:
			return {Token::Error, std::string(1, c), line};
	}
}

Parser::Token Parser::Lexer::peekToken() {
	if (!peeked) {
		peeked = nextToken();
	}
	return *peeked;
}

std::optional<Color4> Parser::parseColor(Lexer& lexer) {
	Token t = lexer.nextToken();
	if (t.type != Token::LBrace)
		return std::nullopt;

	Color4 color;
	float* values[] = {&color.r, &color.g, &color.b, &color.a};

	for (int i = 0; i < 4; i++) {
		t = lexer.nextToken();
		if (t.type != Token::Number)
			return std::nullopt;
		*values[i] = std::stof(t.value);

		if (i < 3) {
			t = lexer.nextToken();
			if (t.type != Token::Comma)
				return std::nullopt;
		}
	}

	t = lexer.nextToken();
	if (t.type != Token::RBrace)
		return std::nullopt;

	return color;
}

std::optional<std::vector<std::string>> Parser::parseStringArray(Lexer& lexer) {
	Token t = lexer.nextToken();
	if (t.type != Token::LBrace)
		return std::nullopt;

	std::vector<std::string> result;

	while (true) {
		t = lexer.peekToken();
		if (t.type == Token::RBrace) {
			lexer.nextToken();
			break;
		}

		t = lexer.nextToken();
		if (t.type == Token::String || t.type == Token::Identifier) {
			result.push_back(t.value);
		} else {
			return std::nullopt;
		}

		t = lexer.peekToken();
		if (t.type == Token::Comma) {
			lexer.nextToken();
		}
	}

	return result;
}

std::optional<float> Parser::parseFloat(Lexer& lexer) {
	Token t = lexer.nextToken();
	if (t.type != Token::Number)
		return std::nullopt;
	return std::stof(t.value);
}

bool Parser::parseStage(Lexer& lexer, int stageNum, Material& material) {
	Token t = lexer.nextToken();
	if (t.type != Token::LBrace)
		return false;

	Stage stage;

	while (true) {
		t = lexer.peekToken();
		if (t.type == Token::RBrace) {
			lexer.nextToken();
			break;
		}
		if (t.type == Token::EndOfFile)
			return false;

		t = lexer.nextToken();
		if (t.type != Token::Identifier)
			continue;

		std::string propName = t.value;
		std::transform(propName.begin(), propName.end(), propName.begin(), ::tolower);

		t = lexer.peekToken();
		if (t.type == Token::LBracket) {
			lexer.nextToken();
			t = lexer.nextToken();
			if (t.type != Token::RBracket)
				return false;
		}

		t = lexer.nextToken();
		if (t.type != Token::Equals)
			continue;

		if (propName == "texture") {
			t = lexer.nextToken();
			if (t.type == Token::String) {
				stage.texture = t.value;
			}
		} else if (propName == "filter") {
			t = lexer.nextToken();
			if (t.type == Token::String || t.type == Token::Identifier) {
				stage.filter = t.value;
			}
		} else if (propName == "uvsource") {
			t = lexer.nextToken();
			if (t.type == Token::String || t.type == Token::Identifier) {
				stage.uvSource = t.value;
			}
		} else if (propName == "class" || propName == "uvtransform") {
			t = lexer.nextToken();
			if (t.type == Token::LBrace) {
				while (true) {
					t = lexer.peekToken();
					if (t.type == Token::RBrace) {
						lexer.nextToken();
						break;
					}
					t = lexer.nextToken();
					while (t.type != Token::Semicolon && t.type != Token::EndOfFile) {
						t = lexer.nextToken();
					}
				}
			}
		} else {
			t = lexer.nextToken();
		}

		t = lexer.peekToken();
		if (t.type == Token::Semicolon) {
			lexer.nextToken();
		}
	}

	material.stages[stageNum] = stage;
	return true;
}

bool Parser::parseProperty(Lexer& lexer, Material& material) {
	Token t = lexer.nextToken();

	if (t.type == Token::EndOfFile)
		return false;
	if (t.type != Token::Identifier)
		return true;

	std::string propName = t.value;
	std::string propNameLower = propName;
	std::transform(propNameLower.begin(), propNameLower.end(), propNameLower.begin(), ::tolower);

	if (propNameLower == "class") {
		t = lexer.nextToken();
		if (t.type != Token::Identifier)
			return true;

		std::string className = t.value;
		std::transform(className.begin(), className.end(), className.begin(), ::tolower);

		if (className.rfind("stage", 0) == 0) {
			int stageNum = 0;
			if (className.size() > 5) {
				std::string suffix = className.substr(5);
				if (!suffix.empty() && std::all_of(suffix.begin(), suffix.end(), ::isdigit)) {
					stageNum = std::stoi(suffix);
					return parseStage(lexer, stageNum, material);
				}
			} else {
				return parseStage(lexer, stageNum, material);
			}
		}

		t = lexer.nextToken();
		if (t.type == Token::LBrace) {
			int depth = 1;
			while (depth > 0) {
				t = lexer.nextToken();
				if (t.type == Token::LBrace)
					depth++;
				else if (t.type == Token::RBrace)
					depth--;
				else if (t.type == Token::EndOfFile)
					return false;
			}
		}
		return true;
	}

	t = lexer.peekToken();
	if (t.type == Token::LBracket) {
		lexer.nextToken();
		t = lexer.nextToken();
		if (t.type != Token::RBracket)
			return true;
	}

	t = lexer.nextToken();
	if (t.type != Token::Equals)
		return true;

	if (propNameLower == "ambient") {
		auto color = parseColor(lexer);
		if (color)
			material.ambient = *color;
	} else if (propNameLower == "diffuse") {
		auto color = parseColor(lexer);
		if (color)
			material.diffuse = *color;
	} else if (propNameLower == "forceddiffuse") {
		auto color = parseColor(lexer);
		if (color)
			material.forcedDiffuse = *color;
	} else if (propNameLower == "emmisive" || propNameLower == "emissive") {
		auto color = parseColor(lexer);
		if (color)
			material.emissive = *color;
	} else if (propNameLower == "specular") {
		auto color = parseColor(lexer);
		if (color)
			material.specular = *color;
	} else if (propNameLower == "specularpower") {
		auto val = parseFloat(lexer);
		if (val)
			material.specularPower = *val;
	} else if (propNameLower == "pixelshaderid") {
		t = lexer.nextToken();
		if (t.type == Token::String || t.type == Token::Identifier) {
			material.pixelShaderID = t.value;
		}
	} else if (propNameLower == "vertexshaderid") {
		t = lexer.nextToken();
		if (t.type == Token::String || t.type == Token::Identifier) {
			material.vertexShaderID = t.value;
		}
	} else if (propNameLower == "mainlight") {
		t = lexer.nextToken();
		if (t.type == Token::String || t.type == Token::Identifier) {
			material.mainLight = t.value;
		}
	} else if (propNameLower == "fogmode") {
		t = lexer.nextToken();
		if (t.type == Token::String || t.type == Token::Identifier) {
			material.fogMode = t.value;
		}
	} else if (propNameLower == "renderflags") {
		auto flags = parseStringArray(lexer);
		if (flags) {
			material.renderFlags = RenderFlag::None;
			for (const auto& flag : *flags) {
				std::string f = flag;
				std::transform(f.begin(), f.end(), f.begin(), ::tolower);
				if (f == "nozwrite")
					material.renderFlags = material.renderFlags | RenderFlag::NoZWrite;
				else if (f == "nocolorwrite")
					material.renderFlags = material.renderFlags | RenderFlag::NoColorWrite;
				else if (f == "noalphawrite")
					material.renderFlags = material.renderFlags | RenderFlag::NoAlphaWrite;
				else if (f == "addblend")
					material.renderFlags = material.renderFlags | RenderFlag::AddBlend;
				else if (f == "landshadow")
					material.renderFlags = material.renderFlags | RenderFlag::LandShadow;
				else if (f == "alphatest32")
					material.renderFlags = material.renderFlags | RenderFlag::AlphaTest32;
				else if (f == "alphatest64")
					material.renderFlags = material.renderFlags | RenderFlag::AlphaTest64;
				else if (f == "alphatest128")
					material.renderFlags = material.renderFlags | RenderFlag::AlphaTest128;
				else if (f == "road")
					material.renderFlags = material.renderFlags | RenderFlag::Road;
				else if (f == "notiwrite")
					material.renderFlags = material.renderFlags | RenderFlag::NoTiWrite;
				else if (f == "noreceiveshadow")
					material.renderFlags = material.renderFlags | RenderFlag::NoReceiveShadow;
				else if (f == "nolodblend")
					material.renderFlags = material.renderFlags | RenderFlag::NoLODBlend;
			}
		}
	} else {
		// Skip unknown property value
		int braceDepth = 0;
		while (true) {
			t = lexer.nextToken();
			if (t.type == Token::LBrace)
				braceDepth++;
			else if (t.type == Token::RBrace) {
				braceDepth--;
				if (braceDepth < 0)
					break;
			} else if (t.type == Token::Semicolon && braceDepth == 0)
				break;
			else if (t.type == Token::EndOfFile)
				break;
		}
		return true;
	}

	t = lexer.peekToken();
	if (t.type == Token::Semicolon) {
		lexer.nextToken();
	}

	return true;
}

std::optional<Material> Parser::parseFile(const std::filesystem::path& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << path << std::endl;
		return std::nullopt;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	auto material = parseString(buffer.str(), path.parent_path());
	if (material) {
		material->sourcePath = path;
	}
	return material;
}

std::optional<Material> Parser::parseString(const std::string& content, const std::filesystem::path& basePath) {
	Material material;
	material.sourcePath = basePath;

	Lexer lexer(content);

	while (parseProperty(lexer, material)) {
		// Continue parsing
	}

	return material;
}

std::string Parser::serialize(const Material& material) {
	std::ostringstream out;

	out << "ambient[] = { " << material.ambient.r << ", " << material.ambient.g << ", " << material.ambient.b << ", "
		<< material.ambient.a << " };\n";
	out << "diffuse[] = { " << material.diffuse.r << ", " << material.diffuse.g << ", " << material.diffuse.b << ", "
		<< material.diffuse.a << " };\n";
	out << "forcedDiffuse[] = { " << material.forcedDiffuse.r << ", " << material.forcedDiffuse.g << ", "
		<< material.forcedDiffuse.b << ", " << material.forcedDiffuse.a << " };\n";
	out << "emmisive[] = { " << material.emissive.r << ", " << material.emissive.g << ", " << material.emissive.b
		<< ", " << material.emissive.a << " };\n";
	out << "specular[] = { " << material.specular.r << ", " << material.specular.g << ", " << material.specular.b
		<< ", " << material.specular.a << " };\n";
	out << "specularPower = " << material.specularPower << ";\n";

	if (!material.pixelShaderID.empty()) {
		out << "PixelShaderID = \"" << material.pixelShaderID << "\";\n";
	}
	if (!material.vertexShaderID.empty()) {
		out << "VertexShaderID = \"" << material.vertexShaderID << "\";\n";
	}

	if (material.mainLight != "Sun") {
		out << "mainLight = \"" << material.mainLight << "\";\n";
	}
	if (material.fogMode != "fog") {
		out << "fogMode = \"" << material.fogMode << "\";\n";
	}

	if (static_cast<int>(material.renderFlags) != 0) {
		out << "renderFlags[] = { ";
		bool first = true;
		auto addFlag = [&](RenderFlag flag, const char* name) {
			if ((material.renderFlags & flag) != RenderFlag::None) {
				if (!first)
					out << ", ";
				out << "\"" << name << "\"";
				first = false;
			}
		};
		addFlag(RenderFlag::NoZWrite, "NoZWrite");
		addFlag(RenderFlag::NoColorWrite, "NoColorWrite");
		addFlag(RenderFlag::NoAlphaWrite, "NoAlphaWrite");
		addFlag(RenderFlag::AddBlend, "AddBlend");
		addFlag(RenderFlag::LandShadow, "LandShadow");
		addFlag(RenderFlag::AlphaTest32, "AlphaTest32");
		addFlag(RenderFlag::AlphaTest64, "AlphaTest64");
		addFlag(RenderFlag::AlphaTest128, "AlphaTest128");
		addFlag(RenderFlag::Road, "Road");
		addFlag(RenderFlag::NoTiWrite, "NoTiWrite");
		addFlag(RenderFlag::NoReceiveShadow, "NoReceiveShadow");
		addFlag(RenderFlag::NoLODBlend, "NoLODBlend");
		out << " };\n";
	}

	for (const auto& [stageNum, stage] : material.stages) {
		out << "\nclass Stage" << stageNum << "\n{\n";
		out << "\ttexture = \"" << stage.texture << "\";\n";
		if (stage.filter != "Anizotropic") {
			out << "\tfilter = \"" << stage.filter << "\";\n";
		}
		if (stage.uvSource != "tex") {
			out << "\tuvSource = \"" << stage.uvSource << "\";\n";
		}
		out << "};\n";
	}

	return out.str();
}

bool Parser::saveFile(const Material& material, const std::filesystem::path& path) {
	std::ofstream file(path);
	if (!file.is_open()) {
		return false;
	}
	file << serialize(material);
	return true;
}

std::filesystem::path resolveTexturePath(const std::string& texturePath, const std::filesystem::path& materialPath) {
	if (texturePath.empty())
		return {};

	std::filesystem::path texPath = texturePath;

	std::string normalized = texturePath;
	std::replace(normalized.begin(), normalized.end(), '\\', '/');
	texPath = normalized;

	if (texPath.is_absolute()) {
		return texPath;
	}

	if (!normalized.empty() && (normalized[0] == '/' || normalized[0] == '\\')) {
		return texPath;
	}

	if (!materialPath.empty()) {
		return materialPath.parent_path() / texPath;
	}

	return texPath;
}

namespace Presets {

Material basicOpaque() {
	Material m;
	m.ambient = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.diffuse = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.forcedDiffuse = Color4(0.0f, 0.0f, 0.0f, 0.0f);
	m.emissive = Color4(0.0f, 0.0f, 0.0f, 1.0f);
	m.specular = Color4(0.2f, 0.2f, 0.2f, 1.0f);
	m.specularPower = 30.0f;
	m.pixelShaderID = "Normal";
	m.vertexShaderID = "Basic";
	return m;
}

Material normalMapped() {
	Material m = basicOpaque();
	m.pixelShaderID = "NormalMap";
	m.vertexShaderID = "NormalMap";

	Stage s0;
	s0.texture = "";
	s0.uvSource = "tex";
	m.stages[0] = s0;

	Stage s1;
	s1.texture = "";
	s1.uvSource = "tex";
	m.stages[1] = s1;

	Stage s2;
	s2.texture = "";
	s2.uvSource = "tex";
	m.stages[2] = s2;

	return m;
}

Material glass() {
	Material m;
	m.ambient = Color4(1.0f, 1.0f, 1.0f, 0.5f);
	m.diffuse = Color4(1.0f, 1.0f, 1.0f, 0.5f);
	m.forcedDiffuse = Color4(0.0f, 0.0f, 0.0f, 0.0f);
	m.emissive = Color4(0.0f, 0.0f, 0.0f, 1.0f);
	m.specular = Color4(0.8f, 0.8f, 0.8f, 1.0f);
	m.specularPower = 80.0f;
	m.pixelShaderID = "Glass";
	m.vertexShaderID = "Glass";
	m.renderFlags = RenderFlag::NoAlphaWrite;
	return m;
}

Material terrain() {
	Material m = basicOpaque();
	m.pixelShaderID = "Terrain";
	m.vertexShaderID = "Terrain";
	m.renderFlags = RenderFlag::LandShadow;
	return m;
}

Material character() {
	Material m = normalMapped();
	m.pixelShaderID = "Super";
	m.vertexShaderID = "Super";
	m.specularPower = 20.0f;
	return m;
}

Material vehicleMetal() {
	Material m;
	m.ambient = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.diffuse = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.forcedDiffuse = Color4(0.0f, 0.0f, 0.0f, 0.0f);
	m.emissive = Color4(0.0f, 0.0f, 0.0f, 1.0f);
	m.specular = Color4(0.14f, 0.14f, 0.14f, 1.0f);
	m.specularPower = 70.0f;
	m.pixelShaderID = "Super";
	m.vertexShaderID = "Super";
	return m;
}

Material weaponMetal() {
	Material m;
	m.ambient = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.diffuse = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.forcedDiffuse = Color4(0.0f, 0.0f, 0.0f, 0.0f);
	m.emissive = Color4(0.0f, 0.0f, 0.0f, 1.0f);
	m.specular = Color4(0.16f, 0.16f, 0.16f, 1.0f);
	m.specularPower = 100.0f;
	m.pixelShaderID = "Super";
	m.vertexShaderID = "Super";
	return m;
}

Material weaponPlastic() {
	Material m;
	m.ambient = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.diffuse = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.forcedDiffuse = Color4(0.0f, 0.0f, 0.0f, 0.0f);
	m.emissive = Color4(0.0f, 0.0f, 0.0f, 1.0f);
	m.specular = Color4(0.09f, 0.09f, 0.09f, 1.0f);
	m.specularPower = 35.0f;
	m.pixelShaderID = "Super";
	m.vertexShaderID = "Super";
	return m;
}

Material staticObject() {
	Material m;
	m.ambient = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.diffuse = Color4(1.0f, 1.0f, 1.0f, 1.0f);
	m.forcedDiffuse = Color4(0.0f, 0.0f, 0.0f, 0.0f);
	m.emissive = Color4(0.0f, 0.0f, 0.0f, 1.0f);
	m.specular = Color4(0.1f, 0.1f, 0.1f, 1.0f);
	m.specularPower = 40.0f;
	m.pixelShaderID = "Super";
	m.vertexShaderID = "Super";
	return m;
}

}  // namespace Presets

Material createFromTemplate(ObjectType type, const std::string& texturePath) {
	Material m;

	switch (type) {
		case ObjectType::Vehicle:
			m = Presets::vehicleMetal();
			break;
		case ObjectType::Weapon:
			m = Presets::weaponMetal();
			break;
		case ObjectType::Character:
			m = Presets::character();
			break;
		case ObjectType::Static:
		default:
			m = Presets::staticObject();
			break;
	}

	if (!texturePath.empty()) {
		std::filesystem::path texPath(texturePath);
		std::string baseName = texPath.stem().string();
		std::string parentPath = texPath.parent_path().string();

		for (const char* suffix : {"_co", "_ca", "_nohq", "_smdi", "_as", "_dt", "_mc"}) {
			size_t pos = baseName.rfind(suffix);
			if (pos != std::string::npos && pos == baseName.length() - strlen(suffix)) {
				baseName = baseName.substr(0, pos);
				break;
			}
		}

		Stage s1;
		s1.texture = parentPath + "/" + baseName + "_nohq.paa";
		s1.uvSource = "tex";
		m.stages[1] = s1;

		Stage s2;
		s2.texture = "#(argb,8,8,3)color(0.5,0.5,0.5,0)";
		s2.uvSource = "tex";
		m.stages[2] = s2;

		Stage s3;
		s3.texture = "#(argb,8,8,3)color(0,0,0,0)";
		s3.uvSource = "tex";
		m.stages[3] = s3;

		Stage s4;
		s4.texture = parentPath + "/" + baseName + "_as.paa";
		s4.uvSource = "tex";
		m.stages[4] = s4;

		Stage s5;
		s5.texture = parentPath + "/" + baseName + "_smdi.paa";
		s5.uvSource = "tex";
		m.stages[5] = s5;

		Stage s6;
		if (type == ObjectType::Vehicle) {
			s6.texture = "#(ai,64,64,1)fresnel(0.45,0.35)";
		} else if (type == ObjectType::Weapon) {
			s6.texture = "#(ai,64,64,1)fresnel(4.01,2.86)";
		} else {
			s6.texture = "#(ai,64,64,1)fresnel(1.3,7)";
		}
		s6.uvSource = "none";
		m.stages[6] = s6;

		Stage s7;
		s7.texture = "a3\\data_f\\env_land_co.paa";
		s7.uvSource = "none";
		m.stages[7] = s7;
	}

	return m;
}

bool generateRvmatForTexture(const std::string& texturePath, ObjectType type, const std::string& outputPath) {
	Material m = createFromTemplate(type, texturePath);
	m.sourcePath = outputPath;
	return Parser::saveFile(m, outputPath);
}

}  // namespace rvmat

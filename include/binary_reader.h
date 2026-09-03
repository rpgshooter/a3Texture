#pragma once

#include <concepts>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <vector>


namespace a3tex {
// Safe binary data reader that tracks position and bounds.
// Uses std::memcpy for type-safe reads instead of reinterpret_cast.
class BinaryReader {
public:
	explicit BinaryReader(std::span<const uint8_t> data) : m_data(data) {}

	explicit BinaryReader(const std::vector<uint8_t>& data)
		: m_data(std::span<const uint8_t>(data.data(), data.size())) {}

	BinaryReader(const uint8_t* data, size_t size) : m_data(std::span<const uint8_t>(data, size)) {}

	// Read an integral or floating-point type from current position.
	// Returns std::nullopt if not enough data remains.
	template <typename T>
		requires std::is_trivially_copyable_v<T>
	[[nodiscard]] std::optional<T> read() {
		if (m_offset + sizeof(T) > m_data.size()) {
			return std::nullopt;
		}
		T value;
		std::memcpy(&value, m_data.data() + m_offset, sizeof(T));
		m_offset += sizeof(T);
		return value;
	}

	// Read an integral or floating-point type, returning default value on failure.
	template <typename T>
		requires std::is_trivially_copyable_v<T>
	[[nodiscard]] T readOr(T defaultValue = T{}) {
		return read<T>().value_or(defaultValue);
	}

	// Read uint8_t
	[[nodiscard]] std::optional<uint8_t> readU8() { return read<uint8_t>(); }

	// Read uint16_t (little-endian)
	[[nodiscard]] std::optional<uint16_t> readU16() { return read<uint16_t>(); }

	// Read uint32_t (little-endian)
	[[nodiscard]] std::optional<uint32_t> readU32() { return read<uint32_t>(); }

	// Read uint64_t (little-endian)
	[[nodiscard]] std::optional<uint64_t> readU64() { return read<uint64_t>(); }

	// Read int8_t
	[[nodiscard]] std::optional<int8_t> readI8() { return read<int8_t>(); }

	// Read int16_t (little-endian)
	[[nodiscard]] std::optional<int16_t> readI16() { return read<int16_t>(); }

	// Read int32_t (little-endian)
	[[nodiscard]] std::optional<int32_t> readI32() { return read<int32_t>(); }

	// Read int64_t (little-endian)
	[[nodiscard]] std::optional<int64_t> readI64() { return read<int64_t>(); }

	// Read float (IEEE 754 single precision)
	[[nodiscard]] std::optional<float> readFloat() { return read<float>(); }

	// Read double (IEEE 754 double precision)
	[[nodiscard]] std::optional<double> readDouble() { return read<double>(); }

	// Read a null-terminated string (ASCIIZ)
	[[nodiscard]] std::optional<std::string> readAsciiz() {
		std::string result;
		while (m_offset < m_data.size()) {
			uint8_t c = m_data[m_offset++];
			if (c == 0) {
				return result;
			}
			result += static_cast<char>(c);
		}
		return std::nullopt;	// Hit end of data without null terminator
	}

	// Read a length-prefixed string (uint32_t length followed by chars)
	[[nodiscard]] std::optional<std::string> readLengthPrefixedString() {
		auto length = readU32();
		if (!length || m_offset + *length > m_data.size()) {
			return std::nullopt;
		}
		std::string result(reinterpret_cast<const char*>(m_data.data() + m_offset), *length);
		m_offset += *length;
		return result;
	}

	// Read raw bytes into a vector
	[[nodiscard]] std::optional<std::vector<uint8_t>> readBytes(size_t count) {
		if (m_offset + count > m_data.size()) {
			return std::nullopt;
		}
		std::vector<uint8_t> result(m_data.data() + m_offset, m_data.data() + m_offset + count);
		m_offset += count;
		return result;
	}

	// Skip n bytes forward
	bool skip(size_t count) {
		if (m_offset + count > m_data.size()) {
			m_offset = m_data.size();
			return false;
		}
		m_offset += count;
		return true;
	}

	// Skip a null-terminated string
	bool skipAsciiz() {
		while (m_offset < m_data.size()) {
			if (m_data[m_offset++] == 0) {
				return true;
			}
		}
		return false;
	}

	// Get current read position
	[[nodiscard]] size_t position() const { return m_offset; }

	// Set read position (with bounds checking)
	bool seek(size_t pos) {
		if (pos > m_data.size()) {
			return false;
		}
		m_offset = pos;
		return true;
	}

	// Get remaining bytes count
	[[nodiscard]] size_t remaining() const { return m_data.size() > m_offset ? m_data.size() - m_offset : 0; }

	// Check if we've reached end of data
	[[nodiscard]] bool eof() const { return m_offset >= m_data.size(); }

	// Check if n bytes are available
	[[nodiscard]] bool hasBytes(size_t n) const { return m_offset + n <= m_data.size(); }

	// Get total data size
	[[nodiscard]] size_t size() const { return m_data.size(); }

	// Get pointer to current position (for compatibility with legacy code)
	[[nodiscard]] const uint8_t* currentPtr() const {
		return m_offset < m_data.size() ? m_data.data() + m_offset : nullptr;
	}

	// Get span of remaining data
	[[nodiscard]] std::span<const uint8_t> remainingSpan() const {
		if (m_offset >= m_data.size()) {
			return {};
		}
		return m_data.subspan(m_offset);
	}

private:
	std::span<const uint8_t> m_data;
	size_t m_offset = 0;
};

} // namespace a3tex

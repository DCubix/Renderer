#pragma once

#include <cstdint>

#include "glad.h"
#include "linalg.h"
#include "data_type.h"

using namespace linalg::aliases;

enum class BufferType {
	ArrayBuffer = GL_ARRAY_BUFFER,
	ElementBuffer = GL_ELEMENT_ARRAY_BUFFER,
	UniformBuffer = GL_UNIFORM_BUFFER,
	ShaderStorageBuffer = GL_SHADER_STORAGE_BUFFER
};

enum class BufferUsage {
	StaticDraw = GL_STATIC_DRAW,
	DynamicDraw = GL_DYNAMIC_DRAW,
	StreamDraw = GL_STREAM_DRAW
};

struct BufferLayoutEntry {
	uint32_t size;
	DataType type;
	bool normalized;
};

class Buffer {
public:
	Buffer() = default;
	~Buffer() = default;

	void create(BufferType type, BufferUsage usage, size_t initialSize = 0);

	void setLayout(BufferLayoutEntry* layout, size_t layoutSize, size_t stride, size_t indexStart = 0);

	template <typename V>
	void update(V* data, size_t n, size_t offset = 0) {
		size_t sz = n * sizeof(V);
		size_t off = offset * sizeof(V);

		bind();
		if (sz > m_prevSize) {
			m_prevSize = sz;
			glBufferData((GLenum)m_type, sz, data, (GLenum)m_usage);
		} else {
			glBufferSubData((GLenum)m_type, off, sz, data);
		}
	}

	template <typename T>
	T* map(size_t offset = 0, size_t length = 0) {
		length = length == 0 ? sizeof(T) : length;
		bind();
		return (T*)glMapBufferRange((GLenum)m_type, offset, length, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	}
	void unmap() { glUnmapBuffer((GLenum)m_type); }
	void bind() { glBindBuffer((GLenum)m_type, m_object); }
	void unbind() { glBindBuffer((GLenum)m_type, 0); }

	void destroy() { if (valid()) { glDeleteBuffers(1, &m_object); m_object = 0; } }
	bool valid() const { return m_object > 0; }

	void attributeDivisor(uint32_t index, uint32_t divisor) { glVertexAttribDivisor(index, divisor); }

	GLuint object() const { return m_object; }

private:
	GLuint m_object{ 0 };
	BufferType m_type{ BufferType::ArrayBuffer };
	BufferUsage m_usage{ BufferUsage::StaticDraw };

	size_t m_prevSize{ 0 };
};

class VertexArray {
public:
	void create() { glGenVertexArrays(1, &m_object); }

	void bind() { glBindVertexArray(m_object); }
	void unbind() { glBindVertexArray(0); }

	void destroy() { if (valid()) { glDeleteVertexArrays(1, &m_object); m_object = 0; } }
	bool valid() const { return m_object > 0; }

private:
	GLuint m_object{ 0 };
};
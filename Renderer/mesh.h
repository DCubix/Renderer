#pragma once

#include "buffer.h"

struct Vertex {
	float3 position, normal, tangent;
	float2 texCoord;
};

static BufferLayoutEntry VertexLayout[] = {
	{ 3, DataType::Float, false },
	{ 3, DataType::Float, false },
	{ 3, DataType::Float, false },
	{ 2, DataType::Float, false }
};

enum class PrimitiveType {
	Points = GL_POINTS,
	Lines = GL_LINES,
	LineLoop = GL_LINE_LOOP,
	LineStrip = GL_LINE_STRIP,
	Triangles = GL_TRIANGLES,
	TriangleFan = GL_TRIANGLE_FAN,
	TriangleStrip = GL_TRIANGLE_STRIP
};

class Mesh {
	friend class Renderer;
public:

	void load(const std::string& fileName);

	void create(Vertex* vertices, size_t count, uint32_t* indices, size_t icount);
	void destroy();

	VertexArray& vao() { return m_vertexArray;  }
	Buffer& vbo() { return m_vertexBuffer; }
	Buffer& ibo() { return m_indexBuffer; }
	uint32_t indexCount() const { return m_indexCount; }

protected:
	Buffer m_vertexBuffer{}, m_indexBuffer{};
	VertexArray m_vertexArray{};
	uint32_t m_indexCount{ 0 };
	bool m_hasInstanceBuffer{ false };
};


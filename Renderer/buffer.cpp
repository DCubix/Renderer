#include "buffer.h"

void Buffer::create(BufferType type, BufferUsage usage, size_t initialSize) {
	m_type = type;
	m_usage = usage;

	glGenBuffers(1, &m_object);
	if (initialSize > 0) {
		glBindBuffer((GLenum)type, m_object);
		glBufferData((GLenum)type, initialSize, nullptr, (GLenum)usage);
		m_prevSize = initialSize;
	}
}

void Buffer::setLayout(BufferLayoutEntry* layout, size_t layoutSize, size_t stride, size_t indexStart) {
	bind();
	size_t off = 0;
	for (size_t i = 0; i < layoutSize; i++) {
		size_t idx = indexStart + i;
		BufferLayoutEntry e = layout[i];
		glEnableVertexAttribArray(idx);
		glVertexAttribPointer(idx, e.size, DataOpenGLMap[(size_t)e.type], e.normalized, stride, (void*)(off));
		off += e.size * DataSizes[(size_t)e.type];
	}
}

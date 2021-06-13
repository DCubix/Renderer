#pragma once

#include "texture.h"
#include <vector>

enum class FrameBufferTarget {
	Framebuffer = GL_FRAMEBUFFER,
	DrawFramebuffer = GL_DRAW_FRAMEBUFFER,
	ReadFramebuffer = GL_READ_FRAMEBUFFER
};

enum class Attachment {
	ColorAttachment = GL_COLOR_ATTACHMENT0,
	DepthAttachment = GL_DEPTH_ATTACHMENT,
	StencilAttachment = GL_STENCIL_ATTACHMENT,
	DepthStencilAttachment = GL_DEPTH_STENCIL_ATTACHMENT,
	NoAttachment = GL_NONE
};

class Framebuffer {
public:

	void destroy();
	bool valid() const { return m_object > 0; }

	void create(uint32_t width, uint32_t height);

	void bind(FrameBufferTarget target = FrameBufferTarget::Framebuffer, Attachment readBuffer = Attachment::NoAttachment);
	void unbind(bool resetViewport = true);

	void addColorAttachment(TextureFormat format, TextureTarget target);
	void addDepthAttachment();
	void addStencilAttachment();
	void addRenderBuffer(TextureFormat storage, Attachment attachment);

	void setDrawBuffer(uint32_t index) { glDrawBuffer(GL_COLOR_ATTACHMENT0 + index); }
	void resetDrawBuffers();

	const Texture& depthAttachment() { return m_depthAttachment; }
	const Texture& stencilAttachment() { return m_stencilAttachment; }
	std::vector<Texture> colorAttachments() { return m_colorAttachments; }

	uint32_t width() const { return m_width; }
	uint32_t height() const { return m_height; }

private:
	GLuint m_object, m_renderBuffer{ 0 };

	int m_previousViewport[4];
	uint32_t m_width, m_height;

	Texture m_depthAttachment{}, m_stencilAttachment{};
	std::vector<Texture> m_colorAttachments;

	FrameBufferTarget m_boundTarget;
};


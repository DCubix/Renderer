#include "framebuffer.h"

void Framebuffer::destroy() {
	if (valid()) {
		glDeleteFramebuffers(1, &m_object);
		m_object = 0;

		if (m_renderBuffer) {
			glDeleteRenderbuffers(1, &m_renderBuffer);
		}

		if (m_depthAttachment.valid()) m_depthAttachment.destroy();
		if (m_stencilAttachment.valid()) m_stencilAttachment.destroy();

		for (auto& a : m_colorAttachments) {
			if (a.valid()) a.destroy();
		}
	}
}

void Framebuffer::create(uint32_t width, uint32_t height) {
	m_width = width;
	m_height = height;

	glGenFramebuffers(1, &m_object);
	glBindFramebuffer(GL_FRAMEBUFFER, m_object);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::bind(FrameBufferTarget target, Attachment readBuffer, uint32_t slot) {
	m_boundTarget = target;
	glGetIntegerv(GL_VIEWPORT, m_previousViewport);
	glBindFramebuffer((GLenum)target, m_object);
	glViewport(0, 0, m_width, m_height);
	if (target == FrameBufferTarget::ReadFramebuffer)
		glReadBuffer((GLenum)readBuffer + slot);
}

void Framebuffer::unbind(bool resetViewport) {
	glBindFramebuffer((GLenum)m_boundTarget, 0);
	if (resetViewport) {
		glViewport(
			m_previousViewport[0],
			m_previousViewport[1],
			m_previousViewport[2],
			m_previousViewport[3]
		);
	}
}

void Framebuffer::addColorAttachment(TextureFormat format, TextureTarget target) {
	glBindFramebuffer(GL_FRAMEBUFFER, m_object);

	Texture tex{};
	tex.create(target);
	tex.bind();
	if (target != TextureTarget::Texture2DMS) {
		tex.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear);
		tex.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	}
	tex.update(format, nullptr, m_width, m_height);
	if (target != TextureTarget::Texture2DMS) tex.generateMipmaps();

	std::vector<GLenum> db;
	uint32_t att = m_colorAttachments.size();
	for (uint32_t i = 0; i < att + 1; i++) {
		db.push_back(GL_COLOR_ATTACHMENT0 + i);
	}

	if (target == TextureTarget::Texture2D) {
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + att,
			(GLenum)target,
			tex.object(),
			0
		);
	} else {
		glFramebufferTexture(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + att,
			tex.object(),
			0
		);
	}

	glDrawBuffers(db.size(), db.data());

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	m_colorAttachments.push_back(tex);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::addDepthAttachment() {
	if (m_depthAttachment.object() != 0) {
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, m_object);

	Texture tex{};
	tex.create(TextureTarget::Texture2D);
	tex.setFilter(TextureFilter::Linear, TextureFilter::Linear);
	tex.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	tex.update(TextureFormat::Depthf, nullptr, m_width, m_height);

	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D,
		tex.object(),
		0
	);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_depthAttachment = tex;
}

void Framebuffer::addStencilAttachment() {
	if (m_stencilAttachment.object() != 0) {
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, m_object);

	Texture tex;
	tex.create(TextureTarget::Texture2D);
	tex.setFilter(TextureFilter::Linear, TextureFilter::Linear);
	tex.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	tex.update(TextureFormat::Rf, nullptr, m_width, m_height);

	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D,
		tex.object(),
		0
	);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_stencilAttachment = tex;
}

void Framebuffer::addRenderBuffer(TextureFormat storage, Attachment attachment, bool multisample) {
	if (m_renderBuffer != 0) {
		return;
	}

	auto [internalFormat, fmt, type] = getTextureFormat(storage);
	
	glGenRenderbuffers(1, &m_renderBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_object);
	glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffer);

	if (!multisample) glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, m_width, m_height);
	else glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, internalFormat, m_width, m_height);

	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		(GLenum)attachment,
		GL_RENDERBUFFER,
		m_renderBuffer
	);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resetDrawBuffers() {
	std::vector<GLenum> db;
	for (int i = 0; i < m_colorAttachments.size(); i++) {
		db.push_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers(db.size(), db.data());
}

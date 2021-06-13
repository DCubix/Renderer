#include "texture.h"

void Texture::create(TextureTarget target) {
	m_target = target;
	glGenTextures(1, &m_object);
}

void Texture::setWrap(TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR) {
	glTexParameteri((GLenum)m_target, GL_TEXTURE_WRAP_S, (GLint)wrapS);
	glTexParameteri((GLenum)m_target, GL_TEXTURE_WRAP_T, (GLint)wrapT);
	glTexParameteri((GLenum)m_target, GL_TEXTURE_WRAP_R, (GLint)wrapR);
}

void Texture::setFilter(TextureFilter minFilter, TextureFilter magFilter) {
	glTexParameteri((GLenum)m_target, GL_TEXTURE_MIN_FILTER, (GLint)minFilter);
	glTexParameteri((GLenum)m_target, GL_TEXTURE_MAG_FILTER, (GLint)magFilter);
}

void Texture::update(TextureFormat format, const void* pixels, uint32_t width, uint32_t height, uint32_t depth) {
	auto [internalFormat, fmt, type] = getTextureFormat(format);
	switch (m_target) {
		case TextureTarget::Texture1D: glTexImage1D((GLenum)m_target, 0, internalFormat, width, 0, fmt, type, pixels); break;
		case TextureTarget::Texture2D: glTexImage2D((GLenum)m_target, 0, internalFormat, width, height, 0, fmt, type, pixels); break;
			// TODO: custom sample count
		case TextureTarget::Texture2DMS: glTexImage2DMultisample((GLenum)m_target, 4, internalFormat, width, height, GL_TRUE); break;
		case TextureTarget::Texture3D: glTexImage3D((GLenum)m_target, 0, internalFormat, width, height, depth, 0, fmt, type, pixels); break;
		default: break;
	}
	m_width = width;
	m_height = height;
}

void Texture::updateCubemapFace(uint8_t* pixels, uint32_t width, uint32_t height, TextureFormat format, CubemapFace face) {
	auto [internalFormat, fmt, type] = getTextureFormat(format);
	glTexImage2D((GLenum)face, 0, internalFormat, width, height, 0, fmt, type, pixels);
}

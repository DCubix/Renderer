#pragma once

#include <tuple>

#include "glad.h"
#include "data_type.h"

enum class TextureTarget {
	Texture1D = GL_TEXTURE_1D,
	Texture2D = GL_TEXTURE_2D,
	Texture3D = GL_TEXTURE_3D,
	CubeMap = GL_TEXTURE_CUBE_MAP
};

enum class CubemapFace {
	CubeMapPX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	CubeMapNX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	CubeMapPY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	CubeMapNY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	CubeMapPZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	CubeMapNZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

enum class TextureWrap {
	None = 0,
	Repeat = GL_REPEAT,
	ClampToEdge = GL_CLAMP_TO_EDGE
};

enum class TextureFilter {
	Nearest = GL_NEAREST,
	Linear = GL_LINEAR,
	NearestMipNearest = GL_NEAREST_MIPMAP_NEAREST,
	NearestMipLinear = GL_NEAREST_MIPMAP_LINEAR,
	LinearMipLinear = GL_LINEAR_MIPMAP_LINEAR,
	LinearMipNearest = GL_LINEAR_MIPMAP_NEAREST
};

enum class TextureFormat {
	R = 0,
	RG,
	RGB,
	RGBA,
	Rf,
	RGf,
	RGBf,
	RGBAf,
	Depthf,
	DepthStencil
};

static std::tuple<GLint, GLenum, GLenum> getTextureFormat(TextureFormat format) {
	GLenum ifmt;
	GLint fmt;
	GLenum type;
	switch (format) {
		case TextureFormat::R: ifmt = GL_R8; fmt = GL_RED; type = GL_UNSIGNED_BYTE; break;
		case TextureFormat::RG: ifmt = GL_RG8; fmt = GL_RG; type = GL_UNSIGNED_BYTE; break;
		case TextureFormat::RGB: ifmt = GL_RGB8; fmt = GL_RGB; type = GL_UNSIGNED_BYTE; break;
		case TextureFormat::RGBA: ifmt = GL_RGBA8; fmt = GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case TextureFormat::Rf: ifmt = GL_R32F; fmt = GL_RED; type = GL_FLOAT; break;
		case TextureFormat::RGf: ifmt = GL_RG32F; fmt = GL_RG; type = GL_FLOAT; break;
		case TextureFormat::RGBf: ifmt = GL_RGB32F; fmt = GL_RGB; type = GL_FLOAT; break;
		case TextureFormat::RGBAf: ifmt = GL_RGBA32F; fmt = GL_RGBA; type = GL_FLOAT; break;
		case TextureFormat::Depthf: ifmt = GL_DEPTH_COMPONENT24; fmt = GL_DEPTH_COMPONENT; type = GL_FLOAT; break;
		case TextureFormat::DepthStencil: ifmt = GL_DEPTH24_STENCIL8; fmt = GL_DEPTH_STENCIL; type = GL_FLOAT; break;
	}
	return { ifmt, fmt, type };
}

class Texture {
public:

	void create(TextureTarget target);

	void setWrap(
		TextureWrap wrapS,
		TextureWrap wrapT,
		TextureWrap wrapR = TextureWrap::ClampToEdge
	);

	void setFilter(
		TextureFilter minFilter,
		TextureFilter magFilter
	);

	void update(
		TextureFormat format,
		const void* pixels,
		uint32_t width, uint32_t height = 0, uint32_t depth = 0
	);

	void updateCubemapFace(
		uint8_t* pixels,
		uint32_t width, uint32_t height,
		TextureFormat format,
		CubemapFace face
	);

	void generateMipmaps() { glGenerateMipmap((GLenum)m_target); }

	void destroy() { if (valid()) { glDeleteTextures(1, &m_object); m_object = 0; } }
	bool valid() const { return m_object > 0; }

	uint32_t width() const { return m_width; }
	uint32_t height() const { return m_height; }

	void bind(uint32_t slot = 0) { glActiveTexture(GL_TEXTURE0 + slot); glBindTexture((GLenum)m_target, m_object); }
	void unbind() { glBindTexture((GLenum)m_target, 0); }

private:
	GLuint m_object{ 0 };

	TextureTarget m_target;
	uint32_t m_width, m_height;
};
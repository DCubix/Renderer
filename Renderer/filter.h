#pragma once

#include "shader_program.h"

enum class BlendMode {
	Normal = 0,
	Additive
};

class Filter {
public:
	virtual void setup(ShaderProgram& shader) {}

	void create(const std::string& source);
	void setUniforms();

	void destroy() { m_shader.destroy(); }
	bool valid() const { return m_shader.valid(); }

	void bind() { m_shader.bind(); }

	BlendMode blendMode() const { return m_blendMode; }
	void blendMode(BlendMode bm) { m_blendMode = bm; }

private:
	ShaderProgram m_shader{};
	BlendMode m_blendMode{ BlendMode::Normal };
};

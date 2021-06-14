#pragma once

#include "shader_program.h"

enum class BlendMode {
	Normal = 0,
	Additive
};

class Filter {
public:
	virtual void create() {}
	virtual void setup(ShaderProgram& shader) {}

	void load(const std::string& source);

	void setUniforms();

	void destroy() { m_shader.destroy(); }
	bool valid() const { return m_shader.valid(); }

	void bind() { m_shader.bind(); }

	BlendMode blendMode() const { return m_blendMode; }
	void blendMode(BlendMode bm) { m_blendMode = bm; }

	uint32_t repeatCount() const { return m_repeatCount; }
	void repeatCount(uint32_t rc) { m_repeatCount = rc; }

private:
	ShaderProgram m_shader{};
	BlendMode m_blendMode{ BlendMode::Normal };
	uint32_t m_repeatCount{ 1 };
};

class GammaCorrect : public Filter {
public:
	void create() override {
		const std::string src =
#include "gamma_correct.frag"
			;
		load(src);
	}

	void setup(ShaderProgram& shader) override {
		shader["rtRendered"](0);
		shader["uGamma"](gamma);
	}

	float gamma{ 2.2f };

};

class Threshold : public Filter {
public:
	void create() override {
		const std::string src =
#include "threshold.frag"
			;
		load(src);
	}

	void setup(ShaderProgram& shader) override {
		shader["rtRendered"](0);
		shader["uThreshold"](threshold);
	}

	float threshold{ 1.0f };

};

class Blur : public Filter {
public:
	void create() override {
		const std::string src =
#include "blur.frag"
			;
		load(src);
	}

	void setup(ShaderProgram& shader) override {
		float2 direction = m_horizontal ? float2{ 1.0f, 0.0f } : float2{ 0.0f, 1.0f };

		shader["rtRendered"](0);
		shader["uDirection"](direction);
		shader["uResolution"](resolution);

		m_horizontal = !m_horizontal;
	}

	float2 resolution{ 1.0f, 1.0f };

private:
	bool m_horizontal{ false };
};

class Combine : public Filter {
public:
	void create() override {
		const std::string src =
#include "combine.frag"
			;
		load(src);
	}

	void setup(ShaderProgram& shader) override {
		shader["rtA"](0);
		shader["rtB"](1);
	}
};

class Erode : public Filter {
public:
	void create() override {
		const std::string src =
#include "erode.frag"
			;
		load(src);
	}

	void setup(ShaderProgram& shader) override {
		shader["rtRendered"](0);
		shader["uResolution"](resolution);
	}

	float2 resolution{ 1.0f, 1.0f };
};
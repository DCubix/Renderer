#pragma once

#include "linalg.h"
using namespace linalg::aliases;

#include "framebuffer.h"
#include "shader_program.h"
#include "filter.h"
#include "filter_chain.h"

class Renderer;
struct LightParameters;

struct PassParameters {
	uint32_t viewport[4];
	float4x4 view, projection;
};

class RenderPass {
public:
	virtual void render(PassParameters params, RenderPass* previousPass) = 0;
	virtual Framebuffer& passResult() = 0;
};

class LightingPass : public RenderPass {
public:
	LightingPass() = default;
	LightingPass(Renderer* renderer);

	void render(PassParameters params, RenderPass* previousPass) override;

	Framebuffer& passResult() override { return m_passResult; }

private:
	Renderer* m_renderer;

	ShaderProgram m_shadowShader, m_shadowInstancedShader;
	Framebuffer m_gbuffer, m_passResult, m_shadowBuffer;

	ShaderProgram m_ambientShader;
	float3 m_ambientColor{ 0.02f };

	ShaderProgram m_lightShader;

	void gbufferFill(PassParameters params);
	void lighting(PassParameters params);
	void renderToShadowBuffer(LightParameters params, float4x4 view, float4x4 proj);
};

class GammaCorrectionPass : public RenderPass {
public:
	GammaCorrectionPass() = default;
	GammaCorrectionPass(Renderer* renderer);

	void render(PassParameters params, RenderPass* previousPass) override;

	Framebuffer& passResult() override { return m_passResult; }

private:
	Renderer* m_renderer;

	GammaCorrect m_filter{};
	Framebuffer m_passResult;
};

class BloomPass : public RenderPass {
public:
	BloomPass() = default;
	BloomPass(Renderer* renderer);

	void render(PassParameters params, RenderPass* previousPass) override;

	Framebuffer& passResult() override { return m_passResult; }

private:
	Renderer* m_renderer;

	Threshold m_thresholdFilter{};
	Combine m_combineFilter{};
	FilterChain m_blurChain{};

	Framebuffer m_passResult, m_thresholdedResult;

};
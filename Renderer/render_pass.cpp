#include "render_pass.h"

#include "renderer.h"
#include "shaders.hpp"

LightingPass::LightingPass(Renderer* renderer) : m_renderer(renderer) {
	m_ambientShader.create();
	m_ambientShader.addShader(QuadVert, ShaderType::VertexShader);
	m_ambientShader.addShader(AmbientPassFrag, ShaderType::FragmentShader);
	m_ambientShader.link();

	m_lightShader.create();
	m_lightShader.addShader(QuadVert, ShaderType::VertexShader);
	m_lightShader.addShader(LightPassFrag, ShaderType::FragmentShader);
	m_lightShader.link();

	m_shadowShader.create();
	m_shadowShader.addProgram(ShadowSrc);
	m_shadowShader.link();

	m_shadowInstancedShader.create();
	m_shadowInstancedShader.addProgram(ShadowInstSrc);
	m_shadowInstancedShader.link();
}

void LightingPass::render(PassParameters params, RenderPass* previousPass) {
	gbufferFill(params);
	lighting(params);
}

void LightingPass::gbufferFill(PassParameters params) {
	if (!m_gbuffer.valid()) {
		m_gbuffer.create(params.viewport[2], params.viewport[3]);
		m_gbuffer.addColorAttachment(TextureFormat::RGBAf, TextureTarget::Texture2D);
		m_gbuffer.addColorAttachment(TextureFormat::RGBf, TextureTarget::Texture2D);
		m_gbuffer.addColorAttachment(TextureFormat::RGBf, TextureTarget::Texture2D);
		m_gbuffer.addColorAttachment(TextureFormat::RGBf, TextureTarget::Texture2D);
		m_gbuffer.addRenderBuffer(TextureFormat::DepthStencil, Attachment::DepthStencilAttachment);
	}

	m_gbuffer.bind();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(params.viewport[0], params.viewport[1], params.viewport[2], params.viewport[3]);

	m_renderer->renderGeometry(params);

	glBindVertexArray(0);
	glUseProgram(0);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	m_gbuffer.unbind(true);
}

void LightingPass::lighting(PassParameters params) {
	if (!m_passResult.valid()) {
		m_passResult.create(params.viewport[2], params.viewport[3]);
		m_passResult.addColorAttachment(TextureFormat::RGBAf, TextureTarget::Texture2D);
	}

	m_passResult.bind();
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_ambientShader.bind();

	m_gbuffer.colorAttachments()[0].bind(0);
	m_gbuffer.colorAttachments()[1].bind(1);
	m_ambientShader["rtDiffuse"](0);
	m_ambientShader["rtMaterial"](1);
	m_ambientShader["uAmbientColor"](m_ambientColor);

	m_renderer->renderScreenQuad();

	m_lightShader.bind();

	m_gbuffer.colorAttachments()[0].bind(0);
	m_gbuffer.colorAttachments()[1].bind(1);
	m_gbuffer.colorAttachments()[2].bind(2);
	m_gbuffer.colorAttachments()[3].bind(3);
	m_lightShader["rtDiffuse"](0);
	m_lightShader["rtMaterial"](1);
	m_lightShader["rtNormals"](2);
	m_lightShader["rtPosition"](3);

	m_lightShader["uView"](params.view);

	glBlendFunc(GL_ONE, GL_ONE);
	for (auto& light : m_renderer->lights()) {
		if (light.type == LightType::Directional) { // TODO: Implement this for Spot too
			// TODO: Make this adjustable
			const float s = 16.0f;
			float4x4 proj = linalg::ortho_matrix(-s, s, -s, s, -s, s + 4.0f);

			float3 dir = -light.direction;
			float4x4 view = (linalg::lookat_matrix(dir * s / 4.0f, float3{ 0.0f }, float3{ 0.0f, 1.0f, 0.0f }));

			renderToShadowBuffer(light, view, proj);
			m_passResult.bind();
			m_lightShader.bind();

			m_lightShader["uLight.viewProj"](linalg::mul(proj, view));
			m_lightShader["rtShadowEnabled"](1);
			m_lightShader["rtShadow"](4);
			m_lightShader["uNF"](float2{ -s, s+4.0f });

			m_shadowBuffer.depthAttachment().bind(4);
		}

		m_lightShader["uLight.position"](light.position);
		m_lightShader["uLight.colorIntensity"](light.colorIntensity);
		m_lightShader["uLight.type"]((int)light.type);
		m_lightShader["uLight.direction"](light.direction);
		m_lightShader["uLight.radius"](light.radius);
		m_lightShader["uLight.cutOff"](light.cutOff);

		m_renderer->renderScreenQuad();
	}

	glDisable(GL_BLEND);
	m_passResult.unbind(true);
}

void LightingPass::renderToShadowBuffer(LightParameters params, float4x4 view, float4x4 proj) {
	if (params.type != LightType::Directional) return;

	if (!m_shadowBuffer.valid()) {
		m_shadowBuffer.create(4096, 4096);
		m_shadowBuffer.addDepthAttachment();
		//m_shadowBuffer.addRenderBuffer(TextureFormat::Depthf, Attachment::DepthAttachment);
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	m_shadowBuffer.bind();
	glClear(GL_DEPTH_BUFFER_BIT);

	PassParameters pp;
	pp.projection = proj;
	pp.view = view;

	m_renderer->renderGeometryWithShader(pp, m_shadowShader, m_shadowInstancedShader);

	m_shadowBuffer.unbind(true);

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
}

GammaCorrectionPass::GammaCorrectionPass(Renderer* renderer) : m_renderer(renderer) {
	m_filter.create();
}

void GammaCorrectionPass::render(PassParameters params, RenderPass* previousPass) {
	if (!m_passResult.valid()) {
		m_passResult.create(params.viewport[2], params.viewport[3]);
		m_passResult.addColorAttachment(TextureFormat::RGBAf, TextureTarget::Texture2D);
	}

	m_passResult.bind();
	glClear(GL_COLOR_BUFFER_BIT);

	previousPass->passResult().colorAttachments()[0].bind(0);

	m_filter.bind();
	m_filter.setUniforms();

	m_renderer->renderScreenQuad();

	m_passResult.unbind(true);
}

BloomPass::BloomPass(Renderer* renderer) : m_renderer(renderer) {
	m_thresholdFilter.create();
	m_combineFilter.create();

	Blur* blur = new Blur();
	blur->create();
	blur->repeatCount(10);
	m_blurChain.addFilter(blur);
}

void BloomPass::render(PassParameters params, RenderPass* previousPass) {
	if (!m_passResult.valid()) {
		m_passResult.create(params.viewport[2], params.viewport[3]);
		m_passResult.addColorAttachment(TextureFormat::RGBAf, TextureTarget::Texture2D);

		m_thresholdedResult.create(params.viewport[2], params.viewport[3]);
		m_thresholdedResult.addColorAttachment(TextureFormat::RGBAf, TextureTarget::Texture2D);
	}

	const uint32_t downScale = 3;
	m_blurChain.create(params.viewport[2] / downScale, params.viewport[3] / downScale);

	glDisable(GL_BLEND);

	m_thresholdedResult.bind();
	glClear(GL_COLOR_BUFFER_BIT);

	previousPass->passResult().colorAttachments()[0].bind(0);

	m_thresholdFilter.bind();
	m_thresholdFilter.setUniforms();

	m_renderer->renderScreenQuad();

	m_thresholdedResult.unbind(true);

	m_thresholdedResult.colorAttachments()[0].bind(0);
	m_thresholdedResult.colorAttachments()[0].generateMipmaps();

	m_blurChain.pingPongBuffer().bind(FrameBufferTarget::DrawFramebuffer, Attachment::ColorAttachment, 0);
	m_thresholdedResult.bind(FrameBufferTarget::ReadFramebuffer, Attachment::ColorAttachment);
	glBlitFramebuffer(0, 0, params.viewport[2], params.viewport[3], 0, 0, params.viewport[2] / downScale, params.viewport[3] / downScale, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	auto blur = (Blur*) m_blurChain.get(0);
	blur->resolution = float2{ float(params.viewport[2]) / downScale, float(params.viewport[3]) / downScale };
	
	PassParameters bparams{};
	bparams.viewport[0] = 0;
	bparams.viewport[1] = 0;
	bparams.viewport[2] = params.viewport[2] / downScale;
	bparams.viewport[3] = params.viewport[3] / downScale;
	m_blurChain.render(m_renderer, bparams, [&](uint32_t i) {
		m_blurChain.pingPongBuffer().colorAttachments()[m_blurChain.slot()].bind(0);
		m_blurChain.pingPongBuffer().colorAttachments()[m_blurChain.slot()].generateMipmaps();
	});

	/*m_passResult.bind(FrameBufferTarget::DrawFramebuffer, Attachment::ColorAttachment);
	m_blurChain.pingPongBuffer().bind(FrameBufferTarget::ReadFramebuffer, Attachment::ColorAttachment, m_blurChain.slot());
	glBlitFramebuffer(0, 0, params.viewport[2], params.viewport[3], 0, 0, params.viewport[2], params.viewport[3], GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);*/

	m_passResult.bind();

	previousPass->passResult().colorAttachments()[0].bind(0);
	m_blurChain.pingPongBuffer().colorAttachments()[m_blurChain.slot()].bind(1);

	m_combineFilter.bind();
	m_combineFilter.setUniforms();

	m_renderer->renderScreenQuad();

	m_passResult.unbind(true);
}

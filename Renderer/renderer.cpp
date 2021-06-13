#include "renderer.h"

#include "shaders.hpp"

static BufferLayoutEntry InstanceLayout[] = {
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, true } // Color
};

static std::string SlotNames[] = {
	"tDiffuse",
	"tSpecular",
	"tNormals",
	"tEmission"
};

void Renderer::create() {
	m_instanceBuffer.create(BufferType::ArrayBuffer, BufferUsage::StreamDraw);

	Vertex verts[] = {
		Vertex{ .position = float3{ 0.0f, 0.0f, 0.0f } },
		Vertex{ .position = float3{ 1.0f, 0.0f, 0.0f } },
		Vertex{ .position = float3{ 1.0f, 1.0f, 0.0f } },
		Vertex{ .position = float3{ 0.0f, 1.0f, 0.0f } }
	};
	uint32_t inds[] = { 0, 1, 2, 2, 3, 0 };
	m_quad.create(verts, 4, inds, 6);

	m_gbufferShader.create();
	m_gbufferShader.addShader(DefaultVert, ShaderType::VertexShader);
	m_gbufferShader.addShader(GBufferPassFrag, ShaderType::FragmentShader);
	m_gbufferShader.link();

	m_gbufferInstancedShader.create();
	m_gbufferInstancedShader.addShader(DefaultInstancedVert, ShaderType::VertexShader);
	m_gbufferInstancedShader.addShader(GBufferPassFrag, ShaderType::FragmentShader);
	m_gbufferInstancedShader.link();

	m_ambientShader.create();
	m_ambientShader.addShader(QuadVert, ShaderType::VertexShader);
	m_ambientShader.addShader(AmbientPassFrag, ShaderType::FragmentShader);
	m_ambientShader.link();

	m_lightShader.create();
	m_lightShader.addShader(QuadVert, ShaderType::VertexShader);
	m_lightShader.addShader(LightPassFrag, ShaderType::FragmentShader);
	m_lightShader.link();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void Renderer::destroy() {
	m_instanceBuffer.destroy();
	if (m_gbufferShader.valid()) m_gbufferShader.destroy();
	if (m_gbufferInstancedShader.valid()) m_gbufferInstancedShader.destroy();
	if (m_pingPongBuffer.valid()) m_pingPongBuffer.destroy();
}

void Renderer::draw(Mesh mesh, float4x4 model, Material material) {
	RenderCommand cmd{};
	cmd.type = RenderCommand::Type::Single;
	cmd.mesh = mesh;
	cmd.single.model = model;
	cmd.material = material;
	m_commands.push_back(cmd);
}

void Renderer::drawInstanced(Mesh mesh, Instance* instances, size_t count, Material material) {
	if (!mesh.m_hasInstanceBuffer) {
		mesh.vao().bind();
		m_instanceBuffer.bind();
		m_instanceBuffer.setLayout(InstanceLayout, 6, sizeof(Instance), 4);
		m_instanceBuffer.attributeDivisor(4, 1);
		m_instanceBuffer.attributeDivisor(5, 1);
		m_instanceBuffer.attributeDivisor(6, 1);
		m_instanceBuffer.attributeDivisor(7, 1);
		m_instanceBuffer.attributeDivisor(8, 1);
		m_instanceBuffer.attributeDivisor(9, 1);

		m_instanceBuffer.update(instances, count);

		mesh.vao().unbind();
		mesh.m_hasInstanceBuffer = true;
	}

	RenderCommand cmd{};
	cmd.type = RenderCommand::Type::Instanced;
	cmd.instanced.count = count;
	cmd.mesh = mesh;
	cmd.material = material;
	m_commands.push_back(cmd);
}

void Renderer::renderAll(uint32_t vx, uint32_t vy, uint32_t vw, uint32_t vh) {
	if (!m_gbuffer.valid()) {
		m_gbuffer.create(vw, vh);
		m_gbuffer.addColorAttachment(TextureFormat::RGBA, TextureTarget::Texture2D);
		m_gbuffer.addColorAttachment(TextureFormat::RGBf, TextureTarget::Texture2D);
		m_gbuffer.addColorAttachment(TextureFormat::RGBf, TextureTarget::Texture2D);
		m_gbuffer.addColorAttachment(TextureFormat::RGBf, TextureTarget::Texture2D);
		m_gbuffer.addRenderBuffer(TextureFormat::DepthStencil, Attachment::DepthStencilAttachment);
	}

	if (!m_pingPongBuffer.valid()) {
		m_pingPongBuffer.create(vw, vh);
		m_pingPongBuffer.addColorAttachment(TextureFormat::RGBA, TextureTarget::Texture2D);
		m_pingPongBuffer.addColorAttachment(TextureFormat::RGBA, TextureTarget::Texture2D);
	}

	RenderPassParameters params{};
	params.viewport[0] = vx;
	params.viewport[1] = vy;
	params.viewport[2] = vw;
	params.viewport[3] = vh;
	params.view = m_view;
	params.projection = m_projection;

	glEnable(GL_DEPTH_TEST);
	gbufferPass(params);

	m_pingPongBuffer.bind();
	m_pingPongBuffer.setDrawBuffer(0);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(params.viewport[0], params.viewport[1], params.viewport[2], params.viewport[3]);

	ambientPass(params);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	for (auto& light : m_lights) {
		drawOneLight(params, light);
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	auto slot = postProcess(params);

	m_pingPongBuffer.unbind(true);

	m_pingPongBuffer.bind(FrameBufferTarget::ReadFramebuffer, Attachment::ColorAttachment, slot);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, vw, vh, 0, 0, vw, vh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_commands.clear();
	m_lights.clear();
}

void Renderer::setCamera(float4x4 view, float4x4 projection) {
	m_view = view;
	m_projection = projection;
}

void setShaderParams(Material& mat, ShaderProgram& sp, RenderPassParameters params) {
	float4x4 tv = linalg::transpose(params.view);

	sp["uView"](params.view);
	sp["uProjection"](params.projection);

	MaterialParameters mp{};
	mp.shininess = mat.shininess;
	mp.emission = mat.emission;
	mp.diffuse = mat.diffuse;
	sp.uniformBuffer("Material", mp, 2);

	bool textureValid[Material::SlotCount] = { false };
	uint32_t i = 0;
	int slot = 0;
	for (auto& tex : mat.textures) {
		textureValid[i] = tex.valid();
		if (tex.valid()) {
			tex.bind(slot++);
		}
		i++;
	}

	slot = 0;
	for (uint32_t i = 0; i < Material::SlotCount; i++) {
		std::string un = std::string("tTextureValid[") + std::to_string(i) + std::string("]");

		sp[un](textureValid[i] ? 1 : 0);
		sp[SlotNames[i]](slot++);
	}
}

void Renderer::gbufferPass(RenderPassParameters params) {
	m_gbuffer.bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(params.viewport[0], params.viewport[1], params.viewport[2], params.viewport[3]);

	for (auto& cmd : m_commands) {
		switch (cmd.type) {
			case RenderCommand::Type::Single:
			{
				m_gbufferShader.bind();
				m_gbufferShader["uModel"](cmd.single.model);

				setShaderParams(cmd.material, m_gbufferShader, params);

				cmd.mesh.vao().bind();
				glDrawElements(GL_TRIANGLES, cmd.mesh.indexCount(), GL_UNSIGNED_INT, nullptr);
			} break;
			case RenderCommand::Type::Instanced:
			{
				m_gbufferInstancedShader.bind();
				setShaderParams(cmd.material, m_gbufferInstancedShader, params);

				cmd.mesh.vao().bind();
				glDrawElementsInstanced(
					GL_TRIANGLES,
					cmd.mesh.indexCount(),
					GL_UNSIGNED_INT,
					nullptr,
					cmd.instanced.count
				);
			} break;
		}
	}

	glBindVertexArray(0);
	glUseProgram(0);

	m_gbuffer.unbind(true);
}

void Renderer::ambientPass(RenderPassParameters params) {
	m_quad.vao().bind();
	m_ambientShader.bind();

	m_gbuffer.colorAttachments()[0].bind(0);
	m_ambientShader["rtDiffuse"](0);
	m_ambientShader["uAmbientColor"](m_ambientColor);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);
	glUseProgram(0);
}

void Renderer::drawOneLight(RenderPassParameters params, LightParameters light) {
	m_quad.vao().bind();
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

	m_lightShader["uLight.position"](light.position);
	m_lightShader["uLight.colorIntensity"](light.colorIntensity);
	m_lightShader["uLight.type"]((int)light.type);
	m_lightShader["uLight.direction"](light.direction);
	m_lightShader["uLight.radius"](light.radius);
	m_lightShader["uLight.cutOff"](light.cutOff);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

uint32_t Renderer::postProcess(RenderPassParameters params) {
	uint32_t slot = 1;
#define prevSlot (1 - slot)

	auto render = [&](Filter& filter) {
		m_pingPongBuffer.setDrawBuffer(slot);
		glClear(GL_COLOR_BUFFER_BIT);

		m_quad.vao().bind();

		m_gbuffer.colorAttachments()[0].bind(0);
		m_gbuffer.colorAttachments()[1].bind(1);
		m_gbuffer.colorAttachments()[2].bind(2);
		m_gbuffer.colorAttachments()[3].bind(3);
		m_pingPongBuffer.colorAttachments()[prevSlot].bind(4);

		filter.bind();
		filter.setUniforms();
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		slot = prevSlot;
	};

	for (auto& filter : m_filters) {
		render(filter);
	}

	return prevSlot;
}

void Renderer::putPointLight(float3 position, float radius, float3 color, float intensity) {
	LightParameters param{};
	param.position = position;
	param.radius = radius;
	param.colorIntensity = float4{ color, intensity };
	param.type = LightType::Point;
	m_lights.push_back(param);
}

void Renderer::putDirectionalLight(float3 direction, float3 color, float intensity) {
	LightParameters param{};
	param.direction = direction;
	param.radius = 1.0f;
	param.colorIntensity = float4{ color, intensity };
	param.type = LightType::Directional;
	m_lights.push_back(param);
}

void Renderer::putSpotLight(float3 position, float3 direction, float radius, float cutOff, float3 color, float intensity) {
	LightParameters param{};
	param.position = position;
	param.direction = direction;
	param.radius = radius;
	param.cutOff = cutOff;
	param.colorIntensity = float4{ color, intensity };
	param.type = LightType::Spot;
	m_lights.push_back(param);
}

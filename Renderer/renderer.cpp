#include "renderer.h"

#include "shaders.hpp"

static BufferLayoutEntry InstanceLayout[] = {
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, true }, // Color
	{ 1, DataType::Float, false } // Emission
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

	m_nullShader.create();
	m_nullShader.addShader(DefaultVert, ShaderType::VertexShader);
	m_nullShader.addShader("#version 330 core\nvoid main(){}", ShaderType::FragmentShader);
	m_nullShader.link();

	m_nullInstancedShader.create();
	m_nullInstancedShader.addShader(DefaultInstancedVert, ShaderType::VertexShader);
	m_nullInstancedShader.addShader("#version 330 core\nvoid main(){}", ShaderType::FragmentShader);
	m_nullInstancedShader.link();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	addPass(new LightingPass(this));
	addPass(new BloomPass(this));
	addPass(new GammaCorrectionPass(this));
}

void Renderer::destroy() {
	m_instanceBuffer.destroy();
	if (m_gbufferShader.valid()) m_gbufferShader.destroy();
	if (m_gbufferInstancedShader.valid()) m_gbufferInstancedShader.destroy();
}

void Renderer::draw(Mesh* mesh, float4x4 model, Material material) {
	RenderCommand cmd{};
	cmd.type = RenderCommand::Type::Single;
	cmd.mesh = mesh;
	cmd.single.model = model;
	cmd.material = material;
	m_commands.push_back(cmd);
}

void Renderer::drawInstanced(Mesh* mesh, Instance* instances, size_t count, Material material) {
	if (!mesh->m_hasInstanceBuffer) {
		mesh->vao().bind();
		m_instanceBuffer.bind();
		m_instanceBuffer.setLayout(InstanceLayout, 7, sizeof(Instance), 6);
		m_instanceBuffer.attributeDivisor(6, 1);
		m_instanceBuffer.attributeDivisor(7, 1);
		m_instanceBuffer.attributeDivisor(8, 1);
		m_instanceBuffer.attributeDivisor(9, 1);
		m_instanceBuffer.attributeDivisor(10, 1);
		m_instanceBuffer.attributeDivisor(11, 1);
		m_instanceBuffer.attributeDivisor(12, 1);

		mesh->vao().unbind();
		mesh->m_hasInstanceBuffer = true;
	}
	m_instanceBuffer.update(instances, count);

	RenderCommand cmd{};
	cmd.type = RenderCommand::Type::Instanced;
	cmd.instanced.count = count;
	cmd.mesh = mesh;
	cmd.material = material;
	m_commands.push_back(cmd);
}

void Renderer::renderAll(uint32_t vx, uint32_t vy, uint32_t vw, uint32_t vh) {
	PassParameters params{};
	params.viewport[0] = vx;
	params.viewport[1] = vy;
	params.viewport[2] = vw;
	params.viewport[3] = vh;
	params.view = m_view;
	params.projection = m_projection;

	RenderPass* previous = nullptr;
	for (auto& pass : m_passes) {
		pass->render(params, previous);
		previous = pass.get();
	}

	auto& lastPass = m_passes.back()->passResult();
	lastPass.bind(FrameBufferTarget::ReadFramebuffer, Attachment::ColorAttachment);
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

void setShaderParams(Material& mat, Skeleton* skel, ShaderProgram& sp, PassParameters params) {
	sp["uView"](params.view);
	sp["uProjection"](params.projection);

	if (skel != nullptr) {
		float4x4 mats[MaxJoints];
		for (size_t id = 0; id < MaxJoints; id++) {
			mats[id] = linalg::identity;
		}

		for (size_t id = 0; id < skel->jointCount(); id++) {
			auto& joint = skel->getJoint(id);
			mats[id] = skel->jointTransform(id);
				//linalg::mul(joint.correctionMatrix, linalg::mul(skel->jointTransform(id), joint.offset));
		}

		sp.uniformBufferArray("Bones", &mats[0], MaxJoints, 5);
		sp["uHasBones"](true);
	} else {
		sp["uHasBones"](false);
	}

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
			sp[SlotNames[i]](slot-1);
		}

		std::string un = std::string("tTextureValid[") + std::to_string(i) + std::string("]");
		sp[un](textureValid[i] ? 1 : 0);

		i++;
	}
}

void Renderer::putPointLight(float3 position, float radius, float3 color, float intensity) {
	LightParameters param{};
	param.position = position;
	param.radius = radius;
	param.colorIntensity = float4{ color, intensity };
	param.type = LightType::Point;
	m_lights.push_back(param);
}

void Renderer::renderScreenQuad() {
	m_quad.vao().bind();
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void Renderer::renderGeometry(PassParameters params) {
	for (auto& cmd : m_commands) {
		switch (cmd.type) {
			case RenderCommand::Type::Single:
			{
				m_gbufferShader.bind();
				m_gbufferShader["uModel"](cmd.single.model);

				setShaderParams(cmd.material, cmd.mesh->skeleton(), m_gbufferShader, params);

				cmd.mesh->vao().bind();
				glDrawElements(GL_TRIANGLES, cmd.mesh->indexCount(), GL_UNSIGNED_INT, nullptr);
			} break;
			case RenderCommand::Type::Instanced:
			{
				m_gbufferInstancedShader.bind();

				// TODO: Skeletal animation for instanced objects... ooof
				setShaderParams(cmd.material, nullptr, m_gbufferInstancedShader, params);

				cmd.mesh->vao().bind();
				glDrawElementsInstanced(
					GL_TRIANGLES,
					cmd.mesh->indexCount(),
					GL_UNSIGNED_INT,
					nullptr,
					cmd.instanced.count
				);
			} break;
		}
	}
}

void Renderer::renderGeometryWithShader(PassParameters params, ShaderProgram& shader, ShaderProgram& instancedShader) {
	for (auto& cmd : m_commands) {
		switch (cmd.type) {
			case RenderCommand::Type::Single:
			{
				shader.bind();
				shader["uModel"](cmd.single.model);
				shader["uView"](params.view);
				shader["uProjection"](params.projection);

				cmd.mesh->vao().bind();
				glDrawElements(GL_TRIANGLES, cmd.mesh->indexCount(), GL_UNSIGNED_INT, nullptr);
			} break;
			case RenderCommand::Type::Instanced:
			{
				instancedShader.bind();
				instancedShader["uView"](params.view);
				instancedShader["uProjection"](params.projection);

				cmd.mesh->vao().bind();
				glDrawElementsInstanced(
					GL_TRIANGLES,
					cmd.mesh->indexCount(),
					GL_UNSIGNED_INT,
					nullptr,
					cmd.instanced.count
				);
			} break;
		}
	}
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

#include "renderer.h"

static BufferLayoutEntry InstanceLayout[] = {
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, false },
	{ 4, DataType::Float, true }
};

static std::string SlotNames[] = {
	"tDiffuse",
	"tSpecular",
	"tNormals",
	"tEmission"
};

void Renderer::create() {
	m_instanceBuffer.create(BufferType::ArrayBuffer, BufferUsage::StreamDraw);

	std::string defaultVertSrc =
		#include "default.vert"
	;

	std::string defaultVertInstancedSrc =
		#include "default_instanced.vert"
	;

	std::string defaultFragSrc =
		#include "default.frag"
	;

	m_default.create();
	m_default.addShader(defaultVertSrc, ShaderType::VertexShader);
	m_default.addShader(defaultFragSrc, ShaderType::FragmentShader);
	m_default.link();

	m_defaultInstanced.create();
	m_defaultInstanced.addShader(defaultVertInstancedSrc, ShaderType::VertexShader);
	m_defaultInstanced.addShader(defaultFragSrc, ShaderType::FragmentShader);
	m_defaultInstanced.link();
}

void Renderer::destroy() {
	m_instanceBuffer.destroy();
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
	RenderPassParameters params{};
	params.viewport[0] = vx;
	params.viewport[1] = vy;
	params.viewport[2] = vw;
	params.viewport[3] = vh;
	params.view = m_view;
	params.projection = m_projection;

	defaultPass(params);

	m_commands.clear();
	m_lights.clear();
}

void Renderer::setCamera(float4x4 view, float4x4 projection) {
	m_view = view;
	m_projection = projection;
}

void Renderer::defaultPass(RenderPassParameters params) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(params.viewport[0], params.viewport[1], params.viewport[2], params.viewport[3]);

	auto setShaderParams = [&](Material& mat, ShaderProgram& sp) {
		float4x4 tv = linalg::transpose(params.view);

		sp["uView"].value()(params.view);
		sp["uProjection"].value()(params.projection);

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

			auto tvloc = sp[un];
			if (tvloc) {
				tvloc.value()(textureValid[i] ? 1 : 0);
			}

			auto tloc = sp[SlotNames[i]];
			if (tloc) {
				tloc.value()(slot++);
			}
		}

		// send lights
		struct Lights {
			LightParameters lights[64];
			int count;
		} lights;

		size_t j = 0;
		for (LightParameters param : m_lights) {
			lights.lights[j++ % 64] = param;
		}
		lights.count = m_lights.size() > 64 ? 64 : m_lights.size();

		sp.uniformBuffer("Lights", lights, 3);
	};

	for (auto& cmd : m_commands) {
		switch (cmd.type) {
			case RenderCommand::Type::Single: {
				m_default.bind();
				m_default["uModel"].value()(cmd.single.model);
				
				setShaderParams(cmd.material, m_default);

				cmd.mesh.vao().bind();
				glDrawElements(GL_TRIANGLES, cmd.mesh.indexCount(), GL_UNSIGNED_INT, nullptr);
			} break;
			case RenderCommand::Type::Instanced: {
				m_defaultInstanced.bind();
				setShaderParams(cmd.material, m_defaultInstanced);

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

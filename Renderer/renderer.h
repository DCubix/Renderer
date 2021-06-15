#pragma once

#include <memory>
#include <vector>

#include "mesh.h"
#include "shader_program.h"
#include "buffer.h"
#include "texture.h"
#include "framebuffer.h"
#include "filter.h"

#include "linalg.h"

#include "render_pass.h"

struct Instance {
	float4x4 model{ linalg::identity };
	float4 texCoordTransform{ 0.0f, 0.0f, 1.0f, 1.0f };
	float4 color{ 1.0f };
	float emission{ 0.0f };
};

// To the shader
struct MaterialParameters {
	float shininess{ 1.0f }, emission{ 0.0f };
	
	float pad[2];

	float4 diffuse;
};

enum class LightType : int {
	Disabled = 0,
	Directional,
	Point,
	Spot
};

struct LightParameters {
	float3 position;
	float4 colorIntensity{ 0.0f };

	LightType type{ LightType::Disabled };
	float3 direction;

	float radius, cutOff;
};

struct Material {
	enum Slot {
		SlotDiffuse = 0,
		SlotSpecular,
		SlotNormals,
		SlotEmission,
		SlotCount
	};
	Texture textures[SlotCount];
	float shininess{ 0.2f }, emission{ 0.0f };
	float4 diffuse{ 1.0f, 1.0f, 1.0f, 1.0f };
};

struct RenderCommand {
	enum class Type {
		Single = 0,
		Instanced
	} type{ Type::Single };

	union {
		float4x4 model;
	} single;

	union {
		size_t count;
	} instanced;

	Mesh mesh;
	Material material{};
};

class Renderer {
public:

	void create();
	void destroy();

	void draw(Mesh mesh, float4x4 model, Material material);
	void drawInstanced(Mesh mesh, Instance* instances, size_t count, Material material);

	void putPointLight(float3 position, float radius, float3 color, float intensity = 1.0f);
	void putDirectionalLight(float3 direction, float3 color, float intensity = 1.0f);
	void putSpotLight(float3 position, float3 direction, float radius, float cutOff, float3 color, float intensity = 1.0f);

	void renderAll(uint32_t vx, uint32_t vy, uint32_t vw, uint32_t vh);

	// TODO: Replace with a proper camera class
	void setCamera(float4x4 view, float4x4 projection);

	void addPass(RenderPass* pass) { return m_passes.push_back(std::unique_ptr<RenderPass>(pass)); }
	
	void renderGeometry(PassParameters params);
	void renderGeometryWithShader(PassParameters params, ShaderProgram& shader, ShaderProgram& instancedShader);
	void renderScreenQuad();

	std::vector<LightParameters> lights() const { return m_lights; }

private:
	std::vector<RenderCommand> m_commands;
	std::vector<LightParameters> m_lights;
	Buffer m_instanceBuffer;

	ShaderProgram m_gbufferShader, m_gbufferInstancedShader;
	ShaderProgram m_nullShader, m_nullInstancedShader;
	
	Mesh m_quad;

	std::vector<std::unique_ptr<RenderPass>> m_passes;

	float4x4 m_view, m_projection;

};


#include <iostream>

#include "game_window.h"
#include "buffer.h"
#include "shader_program.h"
#include "mesh.h"
#include "texture.h"

#include "renderer.h"

#include "stb_image.h"

static ContextConfig conf = {
	.majorVersion = 3,
	.minorVersion = 3,
	.profile = ContextConfig::ProfileCore
};

#define randf() (float(::rand()) / RAND_MAX)
#define randangle() ((randf() * 2.0f - 1.0f) * 3.141592654f)

class Test : public GameWindow {
public:
	Test() : GameWindow(conf) {
		
	}

	void onCreate() {
		resize(1280, 720, true);

		ren.create();

		Vertex fverts[] = {
			Vertex{.position = float3(-10.0f, 0.0f, -10.0f), .normal = float3{ 0.0f, 1.0f, 0.0f }, .texCoord = float2{ 0.0f, 0.0f }},
			Vertex{.position = float3( 10.0f, 0.0f, -10.0f), .normal = float3{ 0.0f, 1.0f, 0.0f }, .texCoord = float2{ 3.0f, 0.0f }},
			Vertex{.position = float3( 10.0f, 0.0f,  10.0f), .normal = float3{ 0.0f, 1.0f, 0.0f }, .texCoord = float2{ 3.0f, 3.0f }},
			Vertex{.position = float3(-10.0f, 0.0f,  10.0f), .normal = float3{ 0.0f, 1.0f, 0.0f }, .texCoord = float2{ 0.0f, 3.0f }}
		};
		uint32_t finds[] = { 2, 1, 0, 0, 3, 2 };

		floorMesh.create(fverts, 4, finds, 6);
		cube.load("monkey.obj");
		worm.import("test.dae");

		for (int y = -2; y <= 2; y++) {
			for (int x = -2; x <= 2; x++) {
				float4x4 pos = linalg::translation_matrix(float3{ x * 3.0f, 0.0f, y * 3.0f });
				Instance ins{};
				ins.model = pos;
				ins.texCoordTransform = float4{ 0.0f, 0.0f, 1.5f, 1.5f };
				ins.color = float4{ randf(), randf(), randf(), 1.0f };
				instances.push_back(ins);

				instancePulses.push_back(2.0f + randf() * 4.0f);
			}
		}

		tex.create(TextureTarget::Texture2D);
		{
			int w, h, comp;
			auto data = stbi_load("marble.jpg", &w, &h, &comp, 4);
			if (data) {
				tex.bind();
				tex.update(TextureFormat::RGBA, data, w, h);
				tex.setWrap(TextureWrap::Repeat, TextureWrap::Repeat);
				tex.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear);
				tex.generateMipmaps();
				stbi_image_free(data);
			}
		}

		mat.textures[Material::SlotDiffuse] = tex;
		mat.shininess = 1.0f;
		//mat.emission = 1.0f;

		floorMat.diffuse = float4{ 0.4f, 0.6f, 0.4f, 1.0f };
		floorMat.shininess = 1.5f;
		floorMat.textures[Material::SlotDiffuse] = tex;
	}

	void onDraw(float elapsedTime) {
		float s = ::sinf(angle * 0.4f);
		float c = ::cosf(angle * 0.4f);

		float s1 = ::sinf(angle + PI);
		float c1 = ::cosf(angle + PI);

		float4x4 v = linalg::lookat_matrix(float3{ c*10.0f, 4.0f, s*10.0f }, float3{ 0.0f }, float3{ 0.0f, 1.0f, 0.0f });
		float4x4 p = linalg::perspective_matrix(rad(50.0f), float(width()) / height(), 0.01f, 500.0f);
		
		ren.setCamera(v, p);
		//ren.draw(cube, m);
		int k = 0;
		for (auto& i : instances) {
			float x = k % 4 - 2;
			float y = k / 4 - 2;
			float4 rot = linalg::rotation_quat(float3{ 0.0f, 1.0f, 0.0f }, elapsedTime * 0.4f);
			i.model = linalg::mul(i.model, linalg::rotation_matrix(rot));

			float wave = ::sinf(angle * instancePulses[k]) * 0.5f + 0.5f;
			float lpwave = wave * wave * wave * wave;

			i.emission = lpwave * 4.0f;

			ren.putPointLight(float3{ x * 3.0f, 0.0f, y * 3.0f }, 3.0f, i.color.xyz(), lpwave * 2.0f);

			k++;
		}

		//ren.putSpotLight(float3{ 0.0f, 3.0f, 0.0f }, float3{ s1, -1.0f, c1 }, 15.0f, rad(40.0f), float3{ 1.0f, 0.5f, 0.0f });
		//ren.putSpotLight(float3{ 0.0f, 3.0f, 0.0f }, float3{ s, -1.0f, c }, 15.0f, rad(40.0f), float3{ 0.0f, 0.5f, 1.0f });
		//ren.putPointLight(float3{ c * 10.0f, 0.0f, s * 10.0f }, 18.0f, float3{ 1.0f }, 0.6f);
		//ren.putDirectionalLight(float3{ -1.0f, -1.0f, 1.0f }, float3{ 1.0f });

		ren.draw(&floorMesh, linalg::translation_matrix(float3{ 0.0f, -1.0f, 0.0f }), floorMat);
		//ren.draw(&worm, linalg::translation_matrix(float3{ 0.0f, 2.0f, 0.0f }), floorMat);
		ren.drawInstanced(&cube, instances.data(), instances.size(), mat);

		ren.renderAll(0, 0, width(), height());

		angle += elapsedTime;
	}

	Renderer ren;
	std::vector<Instance> instances;
	std::vector<float> instancePulses;

	Texture tex, ntex, stex;
	Material mat{}, floorMat{};
	Mesh cube, floorMesh, worm;

	float angle{ 0.0f };
};

int main(int argc, char** argv) {
	Test gw{};
	gw.run();
	return 0;
}
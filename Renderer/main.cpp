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

		cube.load("monkey.obj");

		for (int y = -3; y < 3; y++) {
			for (int x = -3; x < 3; x++) {
				float4x4 pos = linalg::translation_matrix(float3{ x * 3.0f, 0.0f, y * 3.0f });
				Instance ins{};
				ins.model = pos;
				ins.texCoordTransform = float4{ 0.0f, 0.0f, 1.5f, 1.5f };
				ins.color = float4{ randf(), randf(), randf(), 1.0f };
				instances.push_back(ins);
			}
		}

		tex.create(TextureTarget::Texture2D);
		{
			int w, h, comp;
			auto data = stbi_load("bricks.jpg", &w, &h, &comp, 4);
			if (data) {
				tex.bind();
				tex.update(TextureFormat::RGBA, data, w, h);
				tex.setWrap(TextureWrap::Repeat, TextureWrap::Repeat);
				tex.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear);
				tex.generateMipmaps();
				stbi_image_free(data);
			}
		}

		ntex.create(TextureTarget::Texture2D);
		{
			int w, h, comp;
			auto data = stbi_load("bricks_n.png", &w, &h, &comp, 4);
			if (data) {
				ntex.bind();
				ntex.update(TextureFormat::RGBA, data, w, h);
				ntex.setWrap(TextureWrap::Repeat, TextureWrap::Repeat);
				ntex.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear);
				ntex.generateMipmaps();
				stbi_image_free(data);
			}
		}

		stex.create(TextureTarget::Texture2D);
		{
			int w, h, comp;
			auto data = stbi_load("bricks_s.png", &w, &h, &comp, 4);
			if (data) {
				stex.bind();
				stex.update(TextureFormat::RGBA, data, w, h);
				stex.setWrap(TextureWrap::Repeat, TextureWrap::Repeat);
				stex.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear);
				stex.generateMipmaps();
				stbi_image_free(data);
			}
		}

		mat.textures[Material::SlotDiffuse] = tex;
		mat.textures[Material::SlotSpecular] = stex;
		mat.textures[Material::SlotNormals] = ntex;
		mat.shininess = 1.0f;

		

		Filter gammaCorrect;
		const std::string gammaCorrectSrc =
			#include "gamma_correct.frag"
		;
		gammaCorrect.create(gammaCorrectSrc);
		ren.filters().push_back(gammaCorrect);
	}

	void onDraw(float elapsedTime) {
		float s = ::sinf(angle);
		float c = ::cosf(angle);

		float s1 = ::sinf(angle + PI);
		float c1 = ::cosf(angle + PI);

		float4x4 v = linalg::lookat_matrix(float3{ 12.0f, 6.0f, 12.0f }, float3{ 0.0f }, float3{ 0.0f, 1.0f, 0.0f });
		float4x4 p = linalg::perspective_matrix(rad(50.0f), float(width()) / height(), 0.01f, 500.0f);
		
		ren.setCamera(v, p);
		//ren.draw(cube, m);
		for (auto& i : instances) {
			float4 rot = linalg::rotation_quat(float3{ 0.0f, 1.0f, 0.0f }, elapsedTime * 0.4f);
			i.model = linalg::mul(i.model, linalg::rotation_matrix(rot));
		}

		//ren.putSpotLight(float3{ 0.0f, 3.0f, 0.0f }, float3{ s1, -1.0f, c1 }, 15.0f, rad(40.0f), float3{ 1.0f, 0.5f, 0.0f });
		//ren.putSpotLight(float3{ 0.0f, 3.0f, 0.0f }, float3{ s, -1.0f, c }, 15.0f, rad(40.0f), float3{ 0.0f, 0.5f, 1.0f });
		//ren.putPointLight(float3{ c * 10.0f, 0.0f, s * 10.0f }, 18.0f, float3{ 1.0f }, 0.6f);
		ren.putDirectionalLight(float3{ -1.0f, -1.0f, 1.0f }, float3{ 1.0f });

		ren.drawInstanced(cube, instances.data(), instances.size(), mat);

		ren.renderAll(0, 0, width(), height());

		angle += elapsedTime * 1.7f;
	}

	Renderer ren;
	std::vector<Instance> instances;

	Texture tex, ntex, stex;
	Material mat{};
	Mesh cube;

	float angle{ 0.0f };
};

int main(int argc, char** argv) {
	Test gw{};
	gw.run();
	return 0;
}
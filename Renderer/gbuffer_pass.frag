R""(
#version 330 core

layout (location = 0) out vec4 rtDiffuse;
layout (location = 1) out vec3 rtMaterial;
layout (location = 2) out vec3 rtNormals;
layout (location = 3) out vec3 rtPosition;

layout (std140) uniform Material {
	float shininess;
	float emission;
	vec4 diffuse;
} material;

uniform bool tTextureValid[4];
uniform sampler2D tDiffuse;
uniform sampler2D tSpecular;
uniform sampler2D tNormals;
uniform sampler2D tEmission;

in DATA {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
	vec4 color;
	vec3 eye;
	mat3 tbn;
} VS;

void main() {
	vec4 diff = material.diffuse * VS.color;
	if (tTextureValid[0]) {
		diff *= texture(tDiffuse, VS.uv);
	}
	rtDiffuse = diff;

	if (diff.a <= 0.5) discard;

	float spec = 0.0;
	if (tTextureValid[1]) {
		spec = texture(tSpecular, VS.uv).r;
	}

	float emis = material.emission;
	if (tTextureValid[3]) {
		emis *= texture(tEmission, VS.uv).r;
	}
	rtMaterial = vec3(material.shininess, emis, spec);

	vec3 N = normalize(VS.normal);
	if (tTextureValid[2]) {
		N = normalize(VS.tbn * (texture(tNormals, VS.uv).xyz * 2.0 - 1.0));
	}
	rtNormals = N * 0.5 + 0.5;
	rtPosition = VS.position;
}

)""
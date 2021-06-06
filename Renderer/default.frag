R""(
#version 330 core

out vec4 fragColor;

#define TOTAL_LIGHTS 64

struct Light {
	vec3 position;
	
	float pad1;

	vec4 colorIntensity;
	int type; // 0 = disabled, 1 = directional, 2 = point, 3 = specular (TODO)

	float radius;

	vec2 pad2;
};

layout (std140) uniform Lights {
	Light light[TOTAL_LIGHTS];
	int count;
} lights;

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

uniform vec3 lightPos = vec3(-1.0, -1.0, -1.0);
		
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
	vec4 col = material.diffuse;
	if (tTextureValid[0]) {
		col *= texture(tDiffuse, VS.uv);
	}

	vec3 N = normalize(VS.normal);
	if (tTextureValid[2]) {
		N = normalize(VS.tbn * (texture(tNormals, VS.uv).xyz * 2.0 - 1.0));
	}

	float spec = 1.0;
	if (tTextureValid[1]) {
		spec = texture(tSpecular, VS.uv).x;
	}

	vec3 E = normalize(VS.eye - VS.position);

	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);

	for (int i = 0; i < lights.count; i++) {
		Light l = lights.light[i];
		if (l.type == 0 || l.colorIntensity.w <= 0.0) continue;

		vec3 L = vec3(0.0);
		float att = 1.0;
		if (l.type == 1) {
			L = normalize(-l.position);
		} else if (l.type == 2) {
			L = normalize(l.position - VS.position);

			float len = length(l.position - VS.position);
			if (len < l.radius)	att = smoothstep(l.radius, 0, len);
			else att = 0.0;
		}

		vec3 R = normalize(reflect(-L, N));
		float lambertTerm = clamp(dot(N, L), 0.0, 1.0) + 0.07;
		float phongTerm = pow(max(dot(R, E), 0.0), material.shininess * 128.0) * spec;
		
		float ndv = 1.0 - max(dot(N, E), 0.0);
		float rim = smoothstep(0.4, 1.0, ndv);
		phongTerm += rim * spec;
		phongTerm *= lambertTerm;

		specular += phongTerm * l.colorIntensity.xyz * l.colorIntensity.w * att;
		diffuse += lambertTerm * l.colorIntensity.xyz * l.colorIntensity.w * att;
	}
	
	fragColor = vec4((col.rgb * diffuse) + specular, col.a);
}
)""
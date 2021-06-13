R""(#version 330 core
out vec4 fragColor;
in vec2 uv;
in vec3 eye;

struct Light {
	vec3 position;
	vec4 colorIntensity;

	int type; // 0 = disabled, 1 = directional, 2 = point, 3 = specular (TODO)
	vec3 direction;

	float radius, cutOff;
};

uniform sampler2D rtDiffuse;
uniform sampler2D rtMaterial;
uniform sampler2D rtNormals;
uniform sampler2D rtPosition;

uniform Light uLight;

void main() {
	vec3 P = texture(rtPosition, uv).xyz;
	vec3 E = normalize(eye - P);
	vec3 N = texture(rtNormals, uv).xyz * 2.0 - 1.0;
	vec3 M = texture(rtMaterial, uv).xyz;
	vec4 col = texture(rtDiffuse, uv);

	Light l = uLight;
	if (l.type == 0 || l.colorIntensity.w <= 0.0) {
		fragColor = vec4(vec3(0.0), 1.0);
		return;
	}

	vec3 L = vec3(0.0);
	float att = 1.0;
	if (l.type == 1) {
		L = normalize(-l.direction);
	} else if (l.type == 2) {
		L = normalize(l.position - P);

		float len = length(l.position - P);
		if (len < l.radius)	att = smoothstep(l.radius, 0, len);
		else att = 0.0;
	} else if (l.type == 3) {
		L = normalize(l.position - P);

		float len = length(l.position - P);

		if (len < l.radius)	{
			att = smoothstep(l.radius, 0, len);
			float S = dot(L, normalize(-l.direction));
			float c = cos(l.cutOff);
			if (S > c) {
				att *= (1.0 - (1.0 - S) * 1.0 / (1.0 - c));
			} else {
				att = 0.0;
			}
		} else att = 0.0;
	}

	vec3 H = normalize(E + L);
	float lambertTerm = clamp(dot(N, L), 0.0, 1.0);
	float phongTerm = pow(max(0.0, dot(N, H)), M.x * 128.0) * M.z;
		
	float ndv = 1.0 - max(dot(N, E), 0.0);
	float rim = smoothstep(0.4, 1.0, ndv);
	phongTerm += rim * M.z;
	phongTerm *= lambertTerm;

	float fac = 1.0 - M.y;
	vec3 specular = phongTerm * l.colorIntensity.xyz * l.colorIntensity.w * att * fac;
	vec3 diffuse = lambertTerm * l.colorIntensity.xyz * l.colorIntensity.w * att * fac;

	fragColor = vec4((col.rgb * diffuse) + specular, col.a);
	fragColor.rgb += col.rgb * M.y;
}
)""
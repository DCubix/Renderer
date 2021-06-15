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

	mat4 viewProj;
};

uniform sampler2D rtDiffuse;
uniform sampler2D rtMaterial;
uniform sampler2D rtNormals;
uniform sampler2D rtPosition;

uniform sampler2D rtShadow;
uniform bool rtShadowEnabled = false;
uniform vec2 uNF;

uniform Light uLight;

// ========== SHADOW CALCULATIONS ===========
const mat4 mBias = mat4(
	vec4(0.5, 0.0, 0.0, 0.0),
	vec4(0.0, 0.5, 0.0, 0.0),
	vec4(0.0, 0.0, 0.5, 0.0),
	vec4(0.5, 0.5, 0.5, 1.0)
);
const vec2 PoissonDisk[64] = vec2[](
	vec2(-0.613392, 0.617481),
	vec2(0.170019, -0.040254),
	vec2(-0.299417, 0.791925),
	vec2(0.645680, 0.493210),
	vec2(-0.651784, 0.717887),
	vec2(0.421003, 0.027070),
	vec2(-0.817194, -0.271096),
	vec2(-0.705374, -0.668203),
	vec2(0.977050, -0.108615),
	vec2(0.063326, 0.142369),
	vec2(0.203528, 0.214331),
	vec2(-0.667531, 0.326090),
	vec2(-0.098422, -0.295755),
	vec2(-0.885922, 0.215369),
	vec2(0.566637, 0.605213),
	vec2(0.039766, -0.396100),
	vec2(0.751946, 0.453352),
	vec2(0.078707, -0.715323),
	vec2(-0.075838, -0.529344),
	vec2(0.724479, -0.580798),
	vec2(0.222999, -0.215125),
	vec2(-0.467574, -0.405438),
	vec2(-0.248268, -0.814753),
	vec2(0.354411, -0.887570),
	vec2(0.175817, 0.382366),
	vec2(0.487472, -0.063082),
	vec2(-0.084078, 0.898312),
	vec2(0.488876, -0.783441),
	vec2(0.470016, 0.217933),
	vec2(-0.696890, -0.549791),
	vec2(-0.149693, 0.605762),
	vec2(0.034211, 0.979980),
	vec2(0.503098, -0.308878),
	vec2(-0.016205, -0.872921),
	vec2(0.385784, -0.393902),
	vec2(-0.146886, -0.859249),
	vec2(0.643361, 0.164098),
	vec2(0.634388, -0.049471),
	vec2(-0.688894, 0.007843),
	vec2(0.464034, -0.188818),
	vec2(-0.440840, 0.137486),
	vec2(0.364483, 0.511704),
	vec2(0.034028, 0.325968),
	vec2(0.099094, -0.308023),
	vec2(0.693960, -0.366253),
	vec2(0.678884, -0.204688),
	vec2(0.001801, 0.780328),
	vec2(0.145177, -0.898984),
	vec2(0.062655, -0.611866),
	vec2(0.315226, -0.604297),
	vec2(-0.780145, 0.486251),
	vec2(-0.371868, 0.882138),
	vec2(0.200476, 0.494430),
	vec2(-0.494552, -0.711051),
	vec2(0.612476, 0.705252),
	vec2(-0.578845, -0.768792),
	vec2(-0.772454, -0.090976),
	vec2(0.504440, 0.372295),
	vec2(0.155736, 0.065157),
	vec2(0.391522, 0.849605),
	vec2(-0.620106, -0.328104),
	vec2(0.789239, -0.419965),
	vec2(-0.545396, 0.538133),
	vec2(-0.178564, -0.596057)
);

float findBlockerDistance(sampler2D shadowMap, vec3 coord, float lightSize, float bias) {
	int blockers = 0;
	float avgBlockerDistance = 0.0;
	float sw = lightSize * uNF.x / coord.z;

	float fd = coord.z - bias;
	for (int i = 0; i < 64; i++) {
		float z = texture(shadowMap, coord.xy + PoissonDisk[i % 64] * sw).r;
		if (z < fd) {
			blockers++;
			avgBlockerDistance += z;
		}
	}
	if (blockers < 1) return -1.0;
	return avgBlockerDistance / float(blockers);
}

float PCF(in sampler2D sbuffer, vec3 coord, float bias, float radius) {
	if (coord.z > 1.0) {
		return 0.0;
	}

	float fd = coord.z - bias;
	float sum = 0.0;
	for (int i = 0; i < 64; i++) {
		vec2 uvc = coord.xy + PoissonDisk[i % 64] * radius;
		float z = texture(sbuffer, uvc).r;
		sum += z < fd ? 1.0 : 0.0;
	}
	return sum / 64.0;
}

float PCSS(in sampler2D shadowMap, vec3 coord, float bias, float lightSize) {
	// blocker search
	float dist = findBlockerDistance(shadowMap, coord, lightSize, bias);
	if (dist <= -1.0) return 1.0;

	// penumbra estimation
	float pr = (coord.z - dist) / dist;

	// percentage-close filtering
	float radius = pr * lightSize * uNF.x / coord.z;
	return 1.0 - PCF(shadowMap, coord, bias, radius);
}

// =====================================

void main() {
	vec3 P = texture(rtPosition, uv).xyz;
	vec3 E = normalize(eye - P);
	vec3 N = texture(rtNormals, uv).xyz * 2.0 - 1.0;
	vec3 M = texture(rtMaterial, uv).xyz;
	vec4 col = texture(rtDiffuse, uv);

	Light l = uLight;
	if (l.type == 0 || l.colorIntensity.w <= 0.0) {
		discard;
	}

	vec3 L = vec3(0.0);
	float att = 1.0;
	if (l.type == 1) {
		L = normalize(-l.direction);
	} else if (l.type == 2) {
		L = normalize(l.position - P);

		float len = length(l.position - P);
		if (len < l.radius)	att = smoothstep(l.radius, 0, len);
		else discard;
	} else if (l.type == 3) {
		L = normalize(l.position - P);

		float len = length(l.position - P);

		if (len < l.radius)	{
			att = smoothstep(l.radius, 0, len);
			float S = dot(L, normalize(-l.direction));
			float c = cos(l.cutOff);
			if (S > c) {
				att *= (1.0 - (1.0 - S) * 1.0 / (1.0 - c));
			} else discard;
		} else discard;
	}

	vec3 H = normalize(E + L);
	float lambertTerm = max(dot(N, L), 0.0);
	float phongTerm = pow(max(0.0, dot(N, H)), M.x * 128.0) * M.z;
	
	// Shadow Mapping
	float vis = 1.0;
	if (rtShadowEnabled) {
		vec4 sc = mBias * l.viewProj * vec4(P, 1.0);
		vec3 coord = (sc.xyz / sc.w);

		float bias = 0.04 * tan(acos(clamp(lambertTerm, 0.0, 1.0)));
		bias = clamp(bias, 0.0, 0.01);

		vis = PCSS(rtShadow, coord, bias, 0.001); // TODO: Light size?
		//vis = 1.0 - PCF(rtShadow, coord, bias, 0.0005);
	}

	float ndv = 1.0 - max(dot(N, E), 0.0);
	float rim = smoothstep(0.5, 1.0, ndv);
	phongTerm += rim * M.z;
	phongTerm *= lambertTerm;

	float fac = clamp(1.0 - M.y, 0.0, 1.0);
	vec3 specular = phongTerm * l.colorIntensity.xyz * l.colorIntensity.w * att * fac;
	vec3 diffuse = lambertTerm * l.colorIntensity.xyz * l.colorIntensity.w * att * fac;

	fragColor = vec4(((col.rgb * diffuse) + specular) * vis, col.a);
}
)""
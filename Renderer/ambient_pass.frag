R""(#version 330 core
out vec4 fragColor;
in vec2 uv;

uniform sampler2D rtDiffuse;
uniform sampler2D rtMaterial;
uniform vec3 uAmbientColor = vec3(0.2, 0.2, 0.1);

void main() {
	vec4 col = texture(rtDiffuse, uv);
	float emit = texture(rtMaterial, uv).y;
	fragColor = col * vec4(uAmbientColor, 1.0) + vec4(col.rgb * emit, 0.0);
}
)""
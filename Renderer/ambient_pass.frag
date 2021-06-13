R""(#version 330 core
out vec4 fragColor;
in vec2 uv;

uniform sampler2D rtDiffuse;
uniform vec3 uAmbientColor = vec3(0.2, 0.2, 0.1);

void main() {
	fragColor = texture(rtDiffuse, uv) * vec4(uAmbientColor, 1.0);
}
)""
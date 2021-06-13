R""(#version 330 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;

uniform mat4 uView = mat4(1.0);

out vec2 uv;
out vec3 eye;

void main() {
	gl_Position = vec4(vPosition * 2.0 - 1.0, 1.0);
	uv = vPosition.xy;
	eye = -uView[3].xyz * mat3(uView);
}
)""
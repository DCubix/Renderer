R""(#version 330 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out DATA {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
	vec4 color;
	vec3 eye;
	mat3 tbn;
} VS;

void main() {
	vec4 pos = uModel * vec4(vPosition, 1.0);
	gl_Position = uProjection * uView * pos;

	mat3 nmat = mat3(transpose(inverse(uModel)));

	VS.position = pos.xyz;
	VS.uv = vTexCoord;
	VS.color = vec4(1.0);
	VS.normal = nmat * vNormal;
	VS.eye = -uView[3].xyz * mat3(uView);

	VS.tangent = normalize(nmat * vTangent);
	VS.tangent = normalize(VS.tangent - dot(VS.tangent, VS.normal) * VS.normal);

	vec3 b = cross(VS.tangent, VS.normal);
	VS.tbn = mat3(VS.tangent, b, VS.normal);
}
)""
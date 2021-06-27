R""(#version 330 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;

//layout (location = 4) in vec4 vWeights;
//layout (location = 5) in uvec4 vJointIDs;

// instances
layout (location = 6) in mat4 iModel;
layout (location = 10) in vec4 iTexCoordTransform;
layout (location = 11) in vec4 iColor;
layout (location = 12) in float iEmission;

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
	float emission;
} VS;

void main() {
	vec2 uvt = iTexCoordTransform.xy + vTexCoord * iTexCoordTransform.zw;
	vec4 pos = iModel * vec4(vPosition, 1.0);
	gl_Position = uProjection * uView * pos;

	mat3 nmat = mat3(transpose(inverse(iModel)));

	VS.position = pos.xyz;
	VS.uv = uvt;
	VS.color = iColor;
	VS.emission = iEmission;
	VS.normal = nmat * vNormal;
	VS.eye = -uView[3].xyz * mat3(uView);

	VS.tangent = normalize(nmat * vTangent);
	VS.tangent = normalize(VS.tangent - dot(VS.tangent, VS.normal) * VS.normal);

	vec3 b = cross(VS.tangent, VS.normal);
	VS.tbn = mat3(VS.tangent, b, VS.normal);
}
)""
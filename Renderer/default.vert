R""(#version 330 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;

layout (location = 4) in vec4 vWeights;
layout (location = 5) in ivec4 vJointIDs;

layout (std140) uniform Bones {
	mat4 transform[64];
} uBones;
uniform bool uHasBones = false;

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
	float emission;
} VS;

void main() {
	vec4 posSkinned = vec4(0.0);
    vec4 nrmSkinned = vec4(0.0);
	vec4 tgtSkinned = vec4(0.0);

	if (uHasBones) {
		for (int i = 0; i < 4; i++) {
			float w = vWeights[i];
			int id = vJointIDs[i];
			if (id < 0 || w <= 0.0) continue;

			mat4 bone = uBones.transform[id];

			vec4 pos = bone * vec4(vPosition, 1.0);
			posSkinned += pos * w;
			
			vec4 nrm = bone * vec4(vNormal, 0.0);
			nrmSkinned += nrm * w;

			vec4 tgt = bone * vec4(vTangent, 0.0);
			tgtSkinned += tgt * w;
		}
	} else {
		posSkinned = vec4(vPosition, 1.0);
		nrmSkinned = vec4(vNormal, 0.0);
		tgtSkinned = vec4(vTangent, 0.0);
	}
	
	vec4 fpos = uModel * posSkinned;
	gl_Position = uProjection * uView * fpos;

	mat3 nmat = mat3(transpose(inverse(uModel)));

	VS.position = fpos.xyz;
	VS.uv = vTexCoord;
	VS.color = vec4(1.0);
	VS.emission = 0.0f;
	VS.normal = nmat * nrmSkinned.xyz;
	VS.eye = -uView[3].xyz * mat3(uView);

	VS.tangent = normalize(nmat * tgtSkinned.xyz);
	VS.tangent = normalize(VS.tangent - dot(VS.tangent, VS.normal) * VS.normal);

	vec3 b = cross(VS.tangent, VS.normal);
	VS.tbn = mat3(VS.tangent, b, VS.normal);
}
)""
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
	vec4 pos = vec4(vPosition, 1.0);
    vec4 nrm = vec4(vNormal, 0.0);
	vec4 tgt = vec4(vTangent, 0.0);

	int appliedCount = 0;
	mat4 boneTransform = mat4(0.0);
	for (int i = 0; i < 4; i++) {
		float w = vWeights[i];
		if (w * float(vJointIDs[i]) < 0.0) continue;
		boneTransform += uBones.transform[vJointIDs[i]] * w;
		appliedCount++;
	}

	vec4 posSkinned = appliedCount == 0 ? pos : boneTransform * pos;
    vec4 nrmSkinned = appliedCount == 0 ? nrm : boneTransform * nrm;
	vec4 tgtSkinned = appliedCount == 0 ? tgt : boneTransform * tgt;
	
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
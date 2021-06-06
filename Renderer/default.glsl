R""(

vertex_shader {
	#version 330 core

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
		vec2 uv;
		vec3 eye;
	} VS;

	void main() {
		vec4 pos = uModel * vec4(vPosition, 1.0);
		gl_Position = uProjection * uView * pos;

		mat3 nmat = transpose(inverse(mat3(uView * uModel)));

		VS.position = pos.xyz;
		VS.uv = vTexCoord;
		VS.normal = normalize(nmat * vNormal);
		VS.eye = vec3(uView[3]);
	}
}

fragment_shader {
	#version 330 core

	out vec4 fragColor;

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
		
	in DATA {
		vec3 position;
		vec3 normal;
		vec2 uv;
		vec3 eye;
	} VS;

	void main() {
		vec4 col = material.diffuse;
		if (tTextureValid[0]) {
			col *= texture(tDiffuse, VS.uv);
		}

		fragColor = col;
	}
}

)""
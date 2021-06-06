R""(

vertex_shader {
	#version 330 core

	layout (location = 0) in vec3 vPosition;
	layout (location = 1) in vec3 vNormal;
	layout (location = 2) in vec3 vTangent;
	layout (location = 3) in vec2 vTexCoord;

	// instances
	layout (location = 4) in mat4 iModel;
	layout (location = 8) in vec4 iTexCoordTransform;
	layout (location = 9) in vec4 iColor;

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
		vec2 uvt = iTexCoordTransform.xy + vTexCoord * iTexCoordTransform.zw;
		vec4 pos = iModel * vec4(vPosition, 1.0);
		gl_Position = uProjection * uView * pos;

		mat3 nmat = mat3(transpose(inverse(iModel)));

		VS.position = pos.xyz;
		VS.uv = uvt;
		VS.color = iColor;
		VS.normal = nmat * vNormal;
		VS.eye = -uView[3].xyz * mat3(uView);

		VS.tangent = normalize(nmat * vTangent);
		VS.tangent = normalize(VS.tangent - dot(VS.tangent, VS.normal) * VS.normal);

		vec3 b = cross(VS.tangent, VS.normal);
		VS.tbn = mat3(VS.tangent, b, VS.normal);
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

	uniform vec3 lightPos = vec3(-1.0, -1.0, -1.0);
		
	in DATA {
		vec3 position;
		vec3 normal;
		vec3 tangent;
		vec2 uv;
		vec4 color;
		vec3 eye;
		mat3 tbn;
	} VS;

	void main() {
		vec4 col = material.diffuse;
		if (tTextureValid[0]) {
			col *= texture(tDiffuse, VS.uv);
		}

		vec3 N = normalize(VS.normal);
		if (tTextureValid[2]) {
			N = normalize(VS.tbn * (texture(tNormals, VS.uv).xyz * 2.0 - 1.0));
		}

		float spec = 1.0;
		if (tTextureValid[1]) {
			spec = texture(tSpecular, VS.uv).x;
		}

		vec3 L = normalize(-lightPos); //  - VS.position only for point/spot lights
		vec3 E = normalize(VS.eye - VS.position);
		vec3 R = normalize(reflect(-L, N));

		float lambertTerm = clamp(dot(N, L), 0.0, 1.0) + 0.07;
		float phongTerm = pow(max(dot(R, E), 0.0), 20.0) * spec;

		float ndv = 1.0 - max(dot(N, E), 0.0);
		float rim = smoothstep(0.4, 1.0, ndv);
		phongTerm += rim * spec;
		phongTerm *= lambertTerm;

		fragColor = vec4((col.rgb * lambertTerm) + vec3(phongTerm), col.a);
	}
}

)""
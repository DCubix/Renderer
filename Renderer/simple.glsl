R""(

vertex_shader {
	#version 330 core
	layout (location = 0) in vec3 vPos;
	layout (location = 1) in vec3 vNormal;
	layout (location = 2) in vec2 vTex;
	uniform mat4 mv;
	uniform mat4 proj;
	out vec4 color;
	out vec3 normal;
	out vec3 eye;
	out vec2 uv;
	void main() {
		mat3 normalMatrix = transpose(inverse(mat3(mv)));
		vec4 pos = mv * vec4(vPos, 1.0);
		gl_Position = proj * pos;
		color = vec4(vPos * 0.5 + 0.5, 1.0);
		normal = normalize(normalMatrix * vNormal);
		eye = pos.xyz;
		uv = vTex;
	}
}

fragment_shader {
	#version 330 core
	out vec4 fragColor;
	in vec4 color;
	in vec3 normal;
	in vec3 eye;
	in vec2 uv;

	uniform vec3 lightPos = vec3(1.0, 1.0, 0.0);
	uniform sampler2D tex;

	void main() {
		vec3 L = normalize(lightPos);
		vec3 E = normalize(-eye);
		vec3 R = normalize(-reflect(L, normal));

		float lambertTerm = clamp(dot(normal, L), 0.0, 1.0) + 0.08;
		float phongTerm = pow(max(dot(R, E), 0.0), 67.0);

		float ndv = 1.0 - max(dot(normal, E), 0.0);
		float rim = smoothstep(0.4, 0.9, ndv);
		phongTerm += rim * lambertTerm;
			
		vec4 col = texture(tex, uv*2.0);

		fragColor = col * vec4(vec3(lambertTerm + phongTerm), 1.0);
	}
}

)""
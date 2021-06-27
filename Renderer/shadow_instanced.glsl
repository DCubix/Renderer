R""(

vertex_shader {
	#version 330 core
	layout (location = 0) in vec3 vPosition;
	layout (location = 6) in mat4 iModel;

	uniform mat4 uView;
	uniform mat4 uProjection;

	void main() {
		gl_Position = uProjection * uView * iModel * vec4(vPosition, 1.0);
	}
}

fragment_shader {
	#version 330 core
	void main() { }
}

)""
R""(

vertex_shader {
	#version 330 core
	layout (location = 0) in vec3 vPosition;
	//layout (location = 4) in vec2 vWeights;
	//layout (location = 5) in uvec2 vJointIDs;

	uniform mat4 uModel;
	uniform mat4 uView;
	uniform mat4 uProjection;

	void main() {
		gl_Position = uProjection * uView * uModel * vec4(vPosition, 1.0);
	}
}

fragment_shader {
	#version 330 core
	void main() { }
}

)""
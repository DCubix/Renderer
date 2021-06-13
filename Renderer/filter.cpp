#include "filter.h"

void stringReplace(std::string& subject, const std::string& search, const std::string& replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

void Filter::create(const std::string& source) {
	const std::string fxVert = R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;

out vec2 uTexCoord;

void main() {
	gl_Position = vec4(vPosition * 2.0 - 1.0, 1.0);
	uTexCoord = vPosition.xy;
})";

	std::string fxFrag = R"(#version 330 core
out vec4 fragColor;

in vec2 uTexCoord;

uniform sampler2D rtDiffuse;
uniform sampler2D rtMaterial;
uniform sampler2D rtNormals;
uniform sampler2D rtPosition;
uniform sampler2D rtDepth;
uniform sampler2D rtRendered;

#define FilterFunction filterMain

[filter_code]

void main() {
	fragColor = FilterFunction();
})";
	stringReplace(fxFrag, std::string("[filter_code]"), source);

	m_shader.create();
	m_shader.addShader(fxVert, ShaderType::VertexShader);
	m_shader.addShader(fxFrag, ShaderType::FragmentShader);
	m_shader.link();
}

void Filter::setUniforms() {
	m_shader["rtDiffuse"](0);
	m_shader["rtMaterial"](1);
	m_shader["rtNormals"](2);
	m_shader["rtPosition"](3);
	m_shader["rtRendered"](4);

	setup(m_shader);

	glEnable(GL_BLEND);
	switch (m_blendMode) {
		case BlendMode::Additive: glBlendFunc(GL_ONE, GL_ONE); break;
		case BlendMode::Normal: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
	}
}

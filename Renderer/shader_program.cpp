#include "shader_program.h"

#include "parser_tools.hpp"

void ShaderProgram::create() {
	m_program = glCreateProgram();
}

void ShaderProgram::addShader(const std::string& source, ShaderType type) {
	if (!valid()) {
		LOG(ERROR) << "Cannot add a new shader to an invalid program.\n";
		return;
	}

	GLuint shader = createShader(source, (GLenum)type);
	if (shader) {
		glAttachShader(m_program, shader);
		m_shaders.push_back(shader);
	}
}

void ShaderProgram::addProgram(const std::string& source) {
	Scanner sc(source);

	std::string region = "";

	while (sc.hasNext()) {
		char c = sc.peek();
		if (::isalpha(c)) {
			std::string id = sc.scanWhileMatch([](char c) { return ::isalpha(c) || c == '_'; });
			std::transform(id.begin(), id.end(), id.begin(), ::tolower);
			region = id;
		} else if (c == '{') {
			sc.scan();
			int bracketCount = 1;

			std::string src = "";

			while (bracketCount > 0) {
				src += sc.scan();

				if (sc.peek() == '{')
					bracketCount++;
				else if (sc.peek() == '}')
					bracketCount--;
			}

			if (sc.peek() == '}') sc.scan();

			// trim start (remote leading spaces)
			size_t start = src.find_first_not_of(" \n\r\t\f\v");
			src = (start == std::string::npos) ? "" : src.substr(start);

			if (region == "vertex_shader") {
				addShader(src, ShaderType::VertexShader);
			} else if (region == "fragment_shader") {
				addShader(src, ShaderType::FragmentShader);
			} else if (region == "geometry_shader") {
				addShader(src, ShaderType::GeometryShader);
			} else if (region == "compute_shader") {
				addShader(src, ShaderType::ComputeShader);
			} else {
				LOG(WARNING) << "Unknown shader region.\n";
			}
			region = "";
		} else {
			sc.scan();
		}
	}
}

void ShaderProgram::link() {
	if (!valid()) {
		LOG(ERROR) << "Cannot link an invalid program.\n";
		return;
	}

	glLinkProgram(m_program);

	GLint status;
	glGetProgramiv(m_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		glDeleteProgram(m_program);
		m_program = 0;
		return;
	}

	/*for (auto s : m_shaders) {
		glDetachShader(m_program, s);
		glDeleteShader(s);
	}*/
	m_shaders.clear();
}

inline void ShaderProgram::destroy() {
	for (auto& [k, buf] : m_uniformBuffers) {
		buf.destroy();
	}
	if (valid()) { glDeleteProgram(m_program); m_program = 0; }
}

std::optional<ShaderProgram::Uniform> ShaderProgram::operator[](const std::string& name) {
	auto pos = m_uniforms.find(name);
	if (pos == m_uniforms.end()) {
		GLint loc = glGetUniformLocation(m_program, name.c_str());
		if (loc != -1) {
			m_uniforms[name] = ShaderProgram::Uniform(loc);
		} else return {};
	}
	return m_uniforms[name];
}

std::optional<GLuint> ShaderProgram::attribute(const std::string& name) {
	auto pos = m_attributes.find(name);
	if (pos == m_attributes.end()) {
		GLint loc = glGetAttribLocation(m_program, name.c_str());
		if (loc != -1) {
			m_attributes[name] = loc;
		} else return {};
	}
	return m_attributes[name];
}

GLuint ShaderProgram::createShader(const std::string& source, GLenum type) {
	GLuint shader = glCreateShader(type);

	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		std::string buf; buf.resize(1024);
		glGetShaderInfoLog(shader, buf.size(), nullptr, &buf[0]);

		LOG(ERROR) << buf << "\n";

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

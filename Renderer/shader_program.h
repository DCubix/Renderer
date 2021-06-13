#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>

#include "glad.h"
#include "linalg.h"
#include "aixlog.hpp"

#include "buffer.h"

using namespace linalg::aliases;

enum class ShaderType {
	VertexShader = GL_VERTEX_SHADER,
	FragmentShader = GL_FRAGMENT_SHADER,
	GeometryShader = GL_GEOMETRY_SHADER,
	ComputeShader = GL_COMPUTE_SHADER
};

class ShaderProgram {
public:
	struct Uniform {
		GLint location;

		Uniform() = default;
		Uniform(GLint loc) : location(loc) {}

		void operator ()(int i) { glUniform1i(location, i); }
		void operator ()(unsigned int i) { glUniform1ui(location, i); }
		void operator ()(float v) { glUniform1f(location, v); }
		void operator ()(float2 v) { glUniform2f(location, v.x, v.y); }
		void operator ()(float3 v) { glUniform3f(location, v.x, v.y, v.z); }
		void operator ()(float4 v) { glUniform4f(location, v.x, v.y, v.z, v.w); }
		void operator ()(float2x2 v) { glUniformMatrix2fv(location, 1, false, &v[0][0]); }
		void operator ()(float3x3 v) { glUniformMatrix3fv(location, 1, false, &v[0][0]); }
		void operator ()(float4x4 v) { glUniformMatrix4fv(location, 1, false, &v[0][0]); }

	};

	ShaderProgram() = default;
	~ShaderProgram() = default;

	void create();
	void addShader(const std::string& source, ShaderType type);
	void addProgram(const std::string& source);
	void link();

	void destroy();
	bool valid() const { return m_program > 0; }

	void bind() { glUseProgram(m_program); }

	Uniform operator[](const std::string& name);
	std::optional<GLuint> attribute(const std::string& name);

	template <typename T>
	void uniformBuffer(const std::string& name, T data, uint32_t index = 0) {
		if (m_uniformBuffers.find(name) == m_uniformBuffers.end()) {
			uint32_t ubi = glGetUniformBlockIndex(m_program, name.c_str());
			Buffer buf = Buffer();
			buf.create(BufferType::UniformBuffer, BufferUsage::DynamicDraw, sizeof(T));
			
			glUniformBlockBinding(m_program, ubi, index);
			glBindBufferBase(GL_UNIFORM_BUFFER, index, buf.object());

			m_uniformBuffers[name] = buf;
		}
		T* bdata = m_uniformBuffers[name].map<T>();
		::memcpy(bdata, &data, sizeof(T));
		m_uniformBuffers[name].unmap();
	}

private:
	GLuint m_program{ 0 };
	std::vector<GLuint> m_shaders{};

	std::map<std::string, Uniform> m_uniforms{};
	std::map<std::string, Buffer> m_uniformBuffers{};
	std::map<std::string, GLuint> m_attributes{};

	GLuint createShader(const std::string& source, GLenum type);

};


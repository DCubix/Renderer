#include "mesh.h"

#include <fstream>
#include <string>
#include <vector>
#include <functional>

using Filter = std::function<bool(char)>;

bool whiteSpace(char c) {
	return !::isspace(c);
}

bool fwdSlashWhiteSpace(char c) {
	return c != '/' && !whiteSpace(c);
}

bool newLine(char c) {
	return c != '\n';
}

bool digit(char c) { return ::isdigit(c); }

void Mesh::load(const std::string& fileName) {
	std::ifstream fp{ fileName };
	if (fp.good()) {
		auto readChar = [&]() {
			char c = '\0';
			fp.get(c);
			return c;
		};

		auto readString = [&](const Filter& filter = whiteSpace) {
			//std::getline(fp, ret, stopAt);
			std::string ret = "";
			while (!fp.eof() && filter(fp.peek())) {
				ret += readChar();
			}
			return ret;
		};

		auto readFloat = [&](const Filter& filter = whiteSpace) {
			return std::stof(readString(filter));
		};

		auto readInt = [&](const Filter& filter = whiteSpace) {
			return std::stoi(readString(filter));
		};

		auto readIndices = [&]() {
			int3 vtn{ -1, -1, -1 };
			
			vtn[0] = readInt(digit)-1; // next value is also a number
			readChar(); // eat /
			if (fp.peek() == '/') { // v//vn
				vtn[1] = -1;
			} else {
				vtn[1] = readInt(digit)-1; // v/vt
			}

			if (fp.peek() == '/') {
				readChar(); // remove /
				vtn[2] = readInt(digit)-1;
			}
			readChar(); // white space :yeet_table:
			return vtn;
		};

		std::vector<float2> uvs;
		std::vector<float3> pos, nrm;

		std::vector<int3> indices;

		while (!fp.eof()) {
			char c = readChar();
			if (c == '#' || c == 's' || c == 'o') {
				readString(newLine); readChar();
			} else if (c == 'v') {
				char sub = readChar();
				if (!::isspace(sub))
					readChar(); // skip space
				if (sub == 't') {
					float s = readFloat(); readChar();
					float t = readFloat(); readChar();
					uvs.push_back(float2{ s, t });
				} else if (sub == 'n') {
					float x = readFloat(); readChar();
					float y = readFloat(); readChar();
					float z = readFloat(); readChar();
					nrm.push_back(float3{ x, y, z });
				} else {
					float x = readFloat(); readChar();
					float y = readFloat(); readChar();
					float z = readFloat(); readChar();
					pos.push_back(float3{ x, y, z });
				}
			} else if (c == 'f') {
				readChar(); // skip space
				// there are 4 forms: v | v/vt | v//vn | v/vt/vn... 
				indices.push_back(readIndices());
				indices.push_back(readIndices());
				indices.push_back(readIndices());
			} else {
				readChar();
			}
		}

		fp.close();

		// Now process all the data...
		std::vector<Vertex> verts;
		std::vector<uint32_t> inds;
		uint32_t idx = 0;
		for (int3 i : indices) {
			inds.push_back(idx++);

			Vertex vert;
			vert.tangent = float3{ 0.0f };
			vert.position = pos[i[0]];
			if (i[1] != -1) vert.texCoord = uvs[i[1]];
			if (i[2] != -1) vert.normal = nrm[i[2]];
			verts.push_back(vert);
		}

		// calculate tangent space
		for (uint32_t i = 0; i < inds.size(); i += 3) {
			uint32_t i0 = inds[i + 0];
			uint32_t i1 = inds[i + 1];
			uint32_t i2 = inds[i + 2];

			float3 v0 = verts[i0].position;
			float3 v1 = verts[i1].position;
			float3 v2 = verts[i2].position;

			float2 t0 = verts[i0].texCoord;
			float2 t1 = verts[i1].texCoord;
			float2 t2 = verts[i2].texCoord;

			float3 e0 = v1 - v0;
			float3 e1 = v2 - v0;

			float2 dt1 = t1 - t0;
			float2 dt2 = t2 - t0;

			float dividend = dt1.x * dt2.y - dt1.y * dt2.x;
			float f = dividend == 0.0f ? 0.0f : 1.0f / dividend;

			float3 t{ 0.0f };

			t.x = (f * (dt2.y * e0.x - dt1.y * e1.x));
			t.y = (f * (dt2.y * e0.y - dt1.y * e1.y));
			t.z = (f * (dt2.y * e0.z - dt1.y * e1.z));

			verts[i0].tangent += t;
			verts[i1].tangent += t;
			verts[i2].tangent += t;
		}

		for (auto& v : verts) {
			v.tangent = linalg::normalize(v.tangent);
		}

		create(verts.data(), verts.size(), inds.data(), inds.size());
	}
}

void Mesh::create(Vertex* vertices, size_t count, uint32_t* indices, size_t icount) {
	m_vertexArray.create();
	m_vertexArray.bind();

	m_vertexBuffer.create(BufferType::ArrayBuffer, BufferUsage::DynamicDraw);
	m_vertexBuffer.setLayout(VertexLayout, 4, sizeof(Vertex));
	m_vertexBuffer.update(vertices, count);

	m_indexBuffer.create(BufferType::ElementBuffer, BufferUsage::DynamicDraw);
	m_indexBuffer.update(indices, icount);
	m_indexCount = icount;

	m_vertexArray.unbind();
}

void Mesh::destroy() {
	m_vertexArray.destroy();
	m_vertexBuffer.destroy();
	m_indexBuffer.destroy();
}

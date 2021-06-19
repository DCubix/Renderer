#include "mesh.h"

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <functional>

#include "tinyxml2.h"
#include "parser_tools.hpp"

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

		std::vector<Vertex> verts;
		std::vector<uint32_t> inds;
		process(pos, nrm, uvs, indices, std::vector<std::pair<float2, uint2>>(), verts, inds);
		create(verts.data(), verts.size(), inds.data(), inds.size());
	}
}

void Mesh::import(const std::string& fileName) {
	std::vector<float2> uvs;
	std::vector<float3> pos, nrm;

	std::vector<int3> indices;

	using namespace tinyxml2;
	XMLDocument doc;
	doc.LoadFile(fileName.c_str());

	auto root = doc.RootElement();
	auto library_geometries = root->FirstChildElement("library_geometries");
	auto skin = root->FirstChildElement("library_controllers")->FirstChildElement("controller")->FirstChildElement("skin");

	for (auto geom = library_geometries->FirstChildElement("geometry");
		 geom != nullptr;
		 geom = geom->NextSiblingElement("geometry"))
	{
		for (auto mesh = geom->FirstChildElement("mesh");
			 mesh != nullptr;
			 mesh = mesh->NextSiblingElement("mesh"))
		{
			for (auto src = mesh->FirstChildElement("source");
				 src != nullptr;
				 src = src->NextSiblingElement("source"))
			{
				auto id = std::string(src->Attribute("id"));
				auto data = src->FirstChildElement("float_array")->GetText();
				Scanner scn{ data };
				if (id.find("position") != std::string::npos) {
					while (scn.hasNext()) {
						float x = scn.scanFloat(); scn.cleanSpaces();
						float y = scn.scanFloat(); scn.cleanSpaces();
						float z = scn.scanFloat(); scn.cleanSpaces();
						pos.push_back(float3{ x, y, z });
					}
				} else if (id.find("normal") != std::string::npos) {
					while (scn.hasNext()) {
						float x = scn.scanFloat(); scn.cleanSpaces();
						float y = scn.scanFloat(); scn.cleanSpaces();
						float z = scn.scanFloat(); scn.cleanSpaces();
						nrm.push_back(float3{ x, y, z });
					}
				} else if (id.find("map") != std::string::npos) {
					while (scn.hasNext()) {
						float x = scn.scanFloat(); scn.cleanSpaces();
						float y = scn.scanFloat(); scn.cleanSpaces();
						uvs.push_back(float2{ x, y });
					}
				}
			}

			auto triangles = mesh->FirstChildElement("triangles");
			std::map<size_t, size_t> order;
			for (auto in = triangles->FirstChildElement("input");
				 in != nullptr;
				 in = in->NextSiblingElement("input"))
			{
				auto off = std::stoi(in->Attribute("offset"));
				auto sem = std::string(in->Attribute("semantic"));
				if (sem == "VERTEX") order[off] = 0;
				else if (sem == "NORMAL") order[off] = 2;
				else if (sem == "TEXCOORD") order[off] = 1;
			}

			auto data = triangles->FirstChildElement("p")->GetText();
			Scanner scn{ data };
			while (scn.hasNext()) {
				int3 vtn{ -1, -1, -1 };
				for (size_t k = 0; k < order.size(); k++) {
					vtn[order[k]] = (size_t)scn.scanInt(); scn.cleanSpaces();
				}
				indices.push_back(vtn);
			}
		}
	}
	
	// Oh boy let's do this
	std::vector<std::string> boneNames;
	std::vector<float> weights;

	for (auto src = skin->FirstChildElement("source");
		 src != nullptr;
		 src = src->NextSiblingElement("source"))
	{
		auto id = std::string(src->Attribute("id"));
		if (id.find("skin-joints") != std::string::npos) {
			auto names = src->FirstChildElement("Name_array")->GetText();
			Scanner scn{ names };
			while (scn.hasNext()) {
				auto name = scn.scanWhileMatch([](char c) { return !isspace(c); });
				scn.cleanSpaces();
				boneNames.push_back(name);
			}
		} else if (id.find("weights") != std::string::npos) {
			auto data = src->FirstChildElement("float_array")->GetText();
			Scanner scn{ data };
			while (scn.hasNext()) {
				float w = scn.scanFloat(); scn.cleanSpaces();
				weights.push_back(w);
			}
		}
	}

	auto vertex_weights = skin->FirstChildElement("vertex_weights");

	// assuming it's joint->weight
	auto vcount = vertex_weights->FirstChildElement("vcount")->GetText();
	auto v = vertex_weights->FirstChildElement("v")->GetText();

	Scanner vcountScn{ vcount };
	Scanner vScn{ v };

	std::vector<std::pair<float2, uint2>> jointWeights;
	while (vcountScn.hasNext()) {
		int count = vcountScn.scanInt(); vcountScn.cleanSpaces();
		float2 ws{ 0.0f };
		uint2 ids{ 0u };
		for (int i = 0; i < count; i++) {
			int jointID = vScn.scanInt(); vScn.cleanSpaces();
			int weightID = vScn.scanInt(); vScn.cleanSpaces();
			if (i < 2) {
				ws[i] = weights[weightID];
				ids[i] = jointID;
			}
		}
		jointWeights.push_back({ ws, ids });
	}

	std::vector<Vertex> verts;
	std::vector<uint32_t> inds;
	process(pos, nrm, uvs, indices, jointWeights, verts, inds);
	create(verts.data(), verts.size(), inds.data(), inds.size());
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

void Mesh::process(
	const std::vector<float3>& pos,
	const std::vector<float3>& nrm,
	const std::vector<float2>& uvs,
	const std::vector<int3>& faces,
	const std::vector<std::pair<float2, uint2>>& jointWeights,
	std::vector<Vertex>& verts, std::vector<uint32_t>& indices)
{
	// Now process all the data...
	uint32_t idx = 0;
	for (int3 i : faces) {
		Vertex vert;
		/*if (!jointWeights.empty()) {
			auto [jw, jid] = jointWeights[i[0]];
			vert.jointWeights = jw;
			vert.jointIDs = jid;
		} else {
			vert.jointWeights = float2{ 0.0f };
		}*/

		indices.push_back(idx++);

		vert.tangent = float3{ 0.0f };
		vert.position = pos[i[0]];
		
		if (i[1] != -1) vert.texCoord = uvs[i[1]];
		if (i[2] != -1) vert.normal = nrm[i[2]];
		verts.push_back(vert);
	}

	// calculate tangent space
	for (uint32_t i = 0; i < indices.size(); i += 3) {
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

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
}

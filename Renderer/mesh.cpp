#include "mesh.h"

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <functional>

#include "tinyxml2.h"
#include "parser_tools.hpp"

#include "aixlog.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
		process(pos, nrm, uvs, indices, verts, inds);
		create(verts.data(), verts.size(), inds.data(), inds.size());
	}
}

aiNode* findMeshNode(aiNode* node) {
	if (node->mNumMeshes > 0) return node;
	for (size_t i = 0; i < node->mNumChildren; i++) {
		aiNode* nd = findMeshNode(node->mChildren[i]);
		if (nd != nullptr) return nd;
	}
	return nullptr;
}

aiNode* findBoneNode(aiNode* node) {
	if (node->mNumMeshes == 0 && node->mParent != nullptr) return node;
	for (size_t i = 0; i < node->mNumChildren; i++) {
		aiNode* nd = findBoneNode(node->mChildren[i]);
		if (nd != nullptr) return nd;
	}
	return nullptr;
}

void createBoneHierarchy(Skeleton* skel, aiNode* root, int parentID) {
	int bid = skel->addJoint(std::string(root->mName.data), linalg::identity, parentID);
	for (size_t i = 0; i < root->mNumChildren; i++) {
		aiNode* nd = root->mChildren[i];
		createBoneHierarchy(skel, nd, bid);
	}
}

void Mesh::import(const std::string& fileName) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		fileName,
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		LOG(ERROR) << "ERROR::ASSIMP::" << importer.GetErrorString() << "\n";
		return;
	}

	m_skeleton = std::make_unique<Skeleton>();

	std::vector<Vertex> verts;
	std::vector<uint32_t> inds;
	std::map<size_t, std::vector<std::pair<int, float>>> weights;

	aiNode* node = findMeshNode(scene->mRootNode);
	aiMatrix4x4 invXform = scene->mRootNode->mTransformation.Inverse();
	aiNode* skel = findBoneNode(node->mParent);
	createBoneHierarchy(m_skeleton.get(), skel, -1);

	aiMesh* mesh = scene->mMeshes[node->mMeshes[0]]; // load one mesh for now
	for (size_t j = 0; j < mesh->mNumVertices; j++) {
		Vertex vert{};

		aiVector3D pos = mesh->mVertices[j];
		aiVector3D nrm = mesh->mNormals[j];
		aiVector3D tgt = mesh->mTangents[j];
		vert.position = float3{ pos.x, pos.y, pos.z };
		vert.normal = float3{ nrm.x, nrm.y, nrm.z };
		vert.tangent = float3{ tgt.x, tgt.y, tgt.z };

		if (mesh->mTextureCoords[0]) {
			aiVector3D uv = mesh->mTextureCoords[0][j];
			vert.texCoord = float2{ uv.x, uv.y };
		} else {
			vert.texCoord = float2{ 0.0f };
		}

		verts.push_back(vert);
	}

	for (size_t j = 0; j < mesh->mNumFaces; j++) {
		aiFace face = mesh->mFaces[j];
		for (size_t f = 0; f < face.mNumIndices; f++)
			inds.push_back(face.mIndices[f]);
	}

	
	for (size_t j = 0; j < mesh->mNumBones; j++) {
		aiBone* bone = mesh->mBones[j];
		aiMatrix4x4 off = bone->mOffsetMatrix;

		float4x4 mat = float4x4{
			float4{ off.a1, off.b1, off.c1, off.d1 },
			float4{ off.a2, off.b2, off.c2, off.d2 },
			float4{ off.a3, off.b3, off.c3, off.d3 },
			float4{ off.a4, off.b4, off.c4, off.d4 }
		};

		auto name = std::string(bone->mName.data);
		auto& joint = m_skeleton->getJoint(name);
		joint.offset = mat;

		joint.correctionMatrix = float4x4{
			float4{ invXform.a1, invXform.b1, invXform.c1, invXform.d1 },
			float4{ invXform.a2, invXform.b2, invXform.c2, invXform.d2 },
			float4{ invXform.a3, invXform.b3, invXform.c3, invXform.d3 },
			float4{ invXform.a4, invXform.b4, invXform.c4, invXform.d4 }
		};

		for (size_t k = 0; k < bone->mNumWeights; k++) {
			aiVertexWeight vw = bone->mWeights[k];
			weights[vw.mVertexId].push_back({ m_skeleton->getJointID(name), vw.mWeight });
		}
	}

	for (uint32_t i : inds) {
		Vertex& v = verts[i];
		auto ws = weights[i];

		size_t j = 0;
		for (auto [boneID, weight] : ws) {
			if (j >= 4) break;
			v.jointIDs[j] = boneID;
			v.jointWeights[j] = weight;
			j++;
		}
	}

	//process(pos, nrm, uvs, indices, jointWeights, verts, inds);
	create(verts.data(), verts.size(), inds.data(), inds.size());

}

void Mesh::create(Vertex* vertices, size_t count, uint32_t* indices, size_t icount) {
	m_vertexArray.create();
	m_vertexArray.bind();

	m_vertexBuffer.create(BufferType::ArrayBuffer, BufferUsage::DynamicDraw);
	m_vertexBuffer.setLayout(VertexLayout, 6, sizeof(Vertex));
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
	std::vector<Vertex>& verts, std::vector<uint32_t>& indices)
{
	// Now process all the data...
	uint32_t idx = 0;
	for (int3 i : faces) {
		Vertex vert{};
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

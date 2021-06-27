#pragma once

#include "linalg.h"
using namespace linalg::aliases;

#include <vector>
#include <map>
#include <string>

struct Joint {
	float4x4 transform, offset, correctionMatrix;

	int parent{ -1 };
	std::vector<int> children;
};

class Skeleton {
public:

	int addJoint(const std::string& name, const float4x4& offset, int parentID = -1);
	Joint& getJoint(const std::string& name) { return m_joints[m_jointNames[name]]; }
	Joint& getJoint(int id) { return m_joints[id]; }
	int getJointID(const std::string& name) { return m_jointNames[name]; }
	size_t jointCount() const { return m_joints.size(); }

	float4x4 jointTransform(int id);

private:
	std::vector<Joint> m_joints;
	std::map<std::string, int> m_jointNames;

	int m_currentID{ 0 };
};


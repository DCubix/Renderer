#pragma once

#include "linalg.h"
using namespace linalg::aliases;

#include <vector>
#include <map>
#include <string>

struct Joint {
	float4x4 transform;

	int parent{ -1 };
	std::vector<int> children;
};

class Skeleton {
public:

	int addJoint(const std::string& name, const float4x4& mat, int parentID = -1);

private:
	std::vector<Joint> m_joints;
	std::map<int, Joint*> m_jointMap;
	std::map<std::string, int> m_jointNames;

	int m_currentID{ 0 };
};


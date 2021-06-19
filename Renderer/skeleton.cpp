#include "skeleton.h"

int Skeleton::addJoint(const std::string& name, const float4x4& mat, int parentID) {
	Joint joint{};
	joint.parent = parentID;
	joint.transform = mat;
	m_joints.push_back(joint);

	auto parent = m_jointMap.find(parentID);
	if (parent != m_jointMap.end()) {
		parent->second->children.push_back(m_currentID);
	}

	m_jointMap[m_currentID] = &m_joints.back();
	m_jointNames[name] = m_currentID;

	return m_currentID++;
}

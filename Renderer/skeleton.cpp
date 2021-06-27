#include "skeleton.h"

int Skeleton::addJoint(const std::string& name, const float4x4& offset, int parentID) {
	Joint joint{};
	joint.parent = parentID;
	joint.offset = offset;
	joint.transform = linalg::identity;
	m_joints.push_back(joint);

	if (parentID >= 0 && parentID < m_joints.size()) {
		m_joints[parentID].children.push_back(m_currentID);
	}

	m_jointNames[name] = m_currentID;

	return m_currentID++;
}

float4x4 Skeleton::jointTransform(int id) {
	float4x4 parent = linalg::identity;
	auto& joint = getJoint(id);
	if (joint.parent != -1) {
		parent = jointTransform(joint.parent);
	}
	return linalg::mul(parent, joint.transform);
}

// EngineBuildingBlocks/SceneNode.cpp

#include <EngineBuildingBlocks/Settings.h>

#include <EngineBuildingBlocks/SceneNode.h>

#include <Core/Constants.h>
#include <Core/Platform.h>
#include <Core/SimpleBinarySerialization.hpp>
#include <EngineBuildingBlocks/ErrorHandling.h>
#include <EngineBuildingBlocks/Math/Vector256.h>

#include <unordered_set>

using namespace EngineBuildingBlocks;

inline CORE_FORCEINLINE void Transpose3x3(const float* source, float* target)
{
	target[0] = source[0];
	target[1] = source[3];
	target[2] = source[6];
	target[3] = source[1];
	target[4] = source[4];
	target[5] = source[7];
	target[6] = source[2];
	target[7] = source[5];
	target[8] = source[8];
}

///////////////////////////////////////////////////////////////////////////////////////////

bool SceneNodeMainData::IsStatic() const
{
	return ((Flags & c_DynamicMask) == 0);
}

bool SceneNodeMainData::IsDynamic() const
{
	return ((Flags & c_DynamicMask) != 0);
}

void SceneNodeMainData::SetStatic()
{
	Flags &= ~c_DynamicMask;
}

void SceneNodeMainData::SetDynamic()
{
	Flags |= c_DynamicMask;
}

bool SceneNodeMainData::IsTransformationDirtyForHandler() const
{
	return ((Flags & c_TransformationDirtyForHandlerMask) != 0);
}

bool SceneNodeMainData::IsTransformationDirtyForUser() const
{
	return ((Flags & c_TransformationDirtyForUserMask) != 0);
}

void SceneNodeMainData::SetTransformationDirty()
{
	// The transformation becomes dirty both for the handler and the user.
	Flags |= (c_TransformationDirtyForHandlerMask | c_TransformationDirtyForUserMask);
}

void SceneNodeMainData::SetTransformationUpToDateForHandler()
{
	Flags &= ~c_TransformationDirtyForHandlerMask;
}

void SceneNodeMainData::SetTransformationUpToDateForUser()
{
	Flags &= ~c_TransformationDirtyForUserMask;
}

bool SceneNodeMainData::HasChild() const
{
	return (ParentIndex != Core::c_InvalidIndexU);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

unsigned SceneNodeHandler::_RegisterWrappedSceneNode(WrappedSceneNode* wrappedSceneNode)
{
	return m_WrappedSceneNodes.Add(wrappedSceneNode);
}

void SceneNodeHandler::_DeregisterWrappedSceneNode(unsigned wrappedSceneNodeIndex)
{
	m_WrappedSceneNodes.Remove(wrappedSceneNodeIndex);
}

void SceneNodeHandler::_UpdateWrappedSceneNode(unsigned wrappedSceneNodeIndex, WrappedSceneNode* newWrappedSceneNode)
{
	if (wrappedSceneNodeIndex != Core::c_InvalidIndexU)
	{
		m_WrappedSceneNodes[wrappedSceneNodeIndex] = newWrappedSceneNode;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////

const glm::vec3 SceneNodeHandler::c_WorldUp(0.0f, 1.0f, 0.0f);

unsigned SceneNodeHandler::CreateSceneNode(bool isStatic, unsigned updateLevelHint)
{
	unsigned sceneNodeIndex = m_SceneNodeMainData.Add();
	m_ScaledWorldTransformation.Add();
	m_InverseScaledWorldTransformation.Add();

	auto& nodeData = m_SceneNodeMainData[sceneNodeIndex];

	nodeData.Flags = 0;
	nodeData.ParentIndex = Core::c_InvalidIndexU;
	nodeData.FirstChildIndex = Core::c_InvalidIndexU;
	nodeData.SiblingIndex = Core::c_InvalidIndexU;

	nodeData.LocalTr.Position = glm::vec3();
	nodeData.LocalTr.Orientation = glm::mat3();
	nodeData.Scaler = glm::vec3(1.0f);

	nodeData.SetTransformationDirty();

	if (isStatic)
	{
		nodeData.UpdateLevel = 0;
		nodeData.UpdateIndex = Core::c_InvalidIndexU;

		// We have to update the transformation matrices, since a static node cannot be dirty,
		// because the main update loop ignores them.
		UpdateScaledWorldTransformations(sceneNodeIndex);
	}
	else
	{
		nodeData.SetDynamic();
		RegisterSceneNodeForUpdate(nodeData, sceneNodeIndex, updateLevelHint);
	}

	return sceneNodeIndex;
}

void SceneNodeHandler::DeleteSceneNode(unsigned sceneNodeIndex, bool isUpdatingConnections)
{
	auto& data = m_SceneNodeMainData[sceneNodeIndex];

	if (data.IsDynamic())
	{
		DeregisterSceneNodeForUpdate(data);
	}

	if (isUpdatingConnections)
	{
		OrphanSceneNode(sceneNodeIndex, false);
	}

	m_SceneNodeMainData.Remove(sceneNodeIndex);
}

SceneNodeMainData& SceneNodeHandler::GetSceneNodeData(unsigned sceneNodeIndex)
{
	return m_SceneNodeMainData[sceneNodeIndex];
}

const SceneNodeMainData& SceneNodeHandler::GetSceneNodeData(unsigned sceneNodeIndex) const
{
	return m_SceneNodeMainData[sceneNodeIndex];
}

void SceneNodeHandler::RegisterSceneNodeForUpdate(SceneNodeMainData& nodeData, unsigned sceneNodeIndex,
	unsigned updateLevel)
{
	while(updateLevel >= static_cast<unsigned>(m_SceneNodeIndicesForUpdate.size()))
		m_SceneNodeIndicesForUpdate.emplace_back();
	nodeData.UpdateLevel = static_cast<unsigned char>(updateLevel);
	nodeData.UpdateIndex = m_SceneNodeIndicesForUpdate[updateLevel].Add(sceneNodeIndex);
}

void SceneNodeHandler::DeregisterSceneNodeForUpdate(SceneNodeMainData& nodeData)
{
	m_SceneNodeIndicesForUpdate[nodeData.UpdateLevel].Remove(nodeData.UpdateIndex);
}

void SceneNodeHandler::AddChildToChain(unsigned parentSceneNodeIndex, unsigned childSceneNodeIndex)
{
	auto& parentData = GetSceneNodeData(parentSceneNodeIndex);

	unsigned currentIndex = parentData.FirstChildIndex;
	
	if (currentIndex == Core::c_InvalidIndexU)
	{
		parentData.FirstChildIndex = childSceneNodeIndex;
	}
	else
	{
		unsigned previousIndex;
		while (currentIndex != Core::c_InvalidIndexU)
		{
			assert(currentIndex != childSceneNodeIndex);

			previousIndex = currentIndex;
			currentIndex = GetSceneNodeData(currentIndex).SiblingIndex;
		}
		GetSceneNodeData(previousIndex).SiblingIndex = childSceneNodeIndex;
	}
}

void SceneNodeHandler::RemoveChildFromChain(unsigned parentSceneNodeIndex, unsigned childSceneNodeIndex)
{
	auto& parentData = GetSceneNodeData(parentSceneNodeIndex);
	auto& childData = GetSceneNodeData(childSceneNodeIndex);

	unsigned currentIndex = parentData.FirstChildIndex;
	
	if (currentIndex == childSceneNodeIndex)
	{
		parentData.FirstChildIndex = childData.SiblingIndex;
	}
	else
	{
		unsigned previousIndex;
		while (currentIndex != childSceneNodeIndex)
		{
			assert(currentIndex != Core::c_InvalidIndexU);

			previousIndex = currentIndex;
			currentIndex = GetSceneNodeData(currentIndex).SiblingIndex;
		}

		GetSceneNodeData(previousIndex).SiblingIndex = childData.SiblingIndex;
	}

	childData.SiblingIndex = Core::c_InvalidIndexU;
}

void SceneNodeHandler::SetUpdateLevel(unsigned nodeIndex, unsigned updateLevel)
{
	auto& nodeData = GetSceneNodeData(nodeIndex);

	bool isChildrenUpdateRequired = true;
	if (nodeData.IsDynamic())
	{
		// If the update level is invalid (smaller than the new update level)
		// or loose (larger than the new update level), we change it.
		if (nodeData.UpdateLevel != updateLevel)
		{
			DeregisterSceneNodeForUpdate(nodeData);
			RegisterSceneNodeForUpdate(nodeData, nodeIndex, updateLevel);
		}
		else
		{
			// The childrens' update levels are up-to-date.
			isChildrenUpdateRequired = false;
		}
	}
	else
	{
		nodeData.SetDynamic();
		RegisterSceneNodeForUpdate(nodeData, nodeIndex, updateLevel);
	}
	
	// We update recursively the childrens' update levels.
	if (isChildrenUpdateRequired)
	{
		unsigned childUpdateLevel = updateLevel + 1;
		unsigned childIndex = nodeData.FirstChildIndex;
		while (childIndex != Core::c_InvalidIndexU)
		{
			SetUpdateLevel(childIndex, childUpdateLevel);

			childIndex = GetSceneNodeData(childIndex).SiblingIndex;
		}
	}
}

void SceneNodeHandler::SetConnection(unsigned parentSceneNodeIndex, unsigned childSceneNodeIndex)
{
	assert(parentSceneNodeIndex != childSceneNodeIndex);

	auto& parentData = GetSceneNodeData(parentSceneNodeIndex);
	auto& childData = GetSceneNodeData(childSceneNodeIndex);

	unsigned oldParentIndex = childData.ParentIndex;
	if (oldParentIndex != Core::c_InvalidIndexU)
	{
		if (oldParentIndex == parentSceneNodeIndex)
		{
			return; // There's no change in the connections.
		}

		RemoveChildFromChain(oldParentIndex, childSceneNodeIndex);
	}

	childData.ParentIndex = parentSceneNodeIndex;

	AddChildToChain(parentSceneNodeIndex, childSceneNodeIndex);

	if (parentData.IsDynamic())
	{
		SetUpdateLevel(childSceneNodeIndex, parentData.UpdateLevel + 1);

		assert(parentData.UpdateLevel < 0xff);
	}

	childData.SetTransformationDirty();

	if (childData.IsStatic())
	{
		// We have to update the transformation matrices, since a static node cannot be dirty,
		// because the main update loop ignores them.
		UpdateScaledWorldTransformations(childSceneNodeIndex);
	}
}

void SceneNodeHandler::OrphanSceneNode(unsigned node)
{
	OrphanSceneNode(node, true);
}

unsigned SceneNodeHandler::GetParent(unsigned node) const
{
	return GetSceneNodeData(node).ParentIndex;
}

unsigned SceneNodeHandler::GetRoot(unsigned node) const
{
	auto parent = GetParent(node);
	while (parent != Core::c_InvalidIndexU)
	{
		node = parent;
		parent = GetParent(node);
	}
	return node;
}

unsigned SceneNodeHandler::GetFirstChild(unsigned node) const
{
	return GetSceneNodeData(node).FirstChildIndex;
}

unsigned SceneNodeHandler::GetSibling(unsigned node) const
{
	return GetSceneNodeData(node).SiblingIndex;
}

bool SceneNodeHandler::IsChildOf(unsigned parentNodeIndex, unsigned childNodeIndex) const
{
	unsigned currentIndex = GetSceneNodeData(parentNodeIndex).FirstChildIndex;
	while (currentIndex != Core::c_InvalidIndexU)
	{
		if (currentIndex == childNodeIndex)
		{
			return true;
		}
		currentIndex = GetSceneNodeData(currentIndex).SiblingIndex;
	}
	return false;
}

bool SceneNodeHandler::HasChild(unsigned nodeIndex) const
{
	return (GetSceneNodeData(nodeIndex).HasChild());
}

unsigned SceneNodeHandler::GetCountChildren(unsigned nodeIndex) const
{
	unsigned countChildren = 0;
	unsigned currentIndex = GetSceneNodeData(nodeIndex).FirstChildIndex;
	while (currentIndex != Core::c_InvalidIndexU)
	{
		++countChildren;
		currentIndex = GetSceneNodeData(currentIndex).SiblingIndex;
	}
	return countChildren;
}

void SceneNodeHandler::GetChildren(unsigned nodeIndex, Core::IndexVectorU& children) const
{
	children.Clear();	
	for (unsigned currentIndex = GetSceneNodeData(nodeIndex).FirstChildIndex;
		currentIndex != Core::c_InvalidIndexU;
		currentIndex = GetSceneNodeData(currentIndex).SiblingIndex)
		children.PushBack(currentIndex);
}

void SceneNodeHandler::OrphanSceneNode(unsigned nodeIndex, bool isHandlingUpdateLevel)
{
	auto& nodeData = GetSceneNodeData(nodeIndex);

	unsigned parentIndex = nodeData.ParentIndex;

	if (parentIndex != Core::c_InvalidIndexU)
	{
		RemoveChildFromChain(parentIndex, nodeIndex);

		nodeData.ParentIndex = Core::c_InvalidIndexU;

		if (isHandlingUpdateLevel && nodeData.IsDynamic())
		{
			SetUpdateLevel(nodeIndex, 0);
		}
	}
}

void SceneNodeHandler::AddSubtree(unsigned nodeIndex, Core::IndexVectorU& nodeIndices) const
{
	nodeIndices.PushBack(nodeIndex);
	for (unsigned i = 0; i < nodeIndices.GetSize(); ++i)
	{
		for (unsigned childIndex = GetSceneNodeData(nodeIndices[i]).FirstChildIndex;
			childIndex != Core::c_InvalidIndexU;
			childIndex = GetSceneNodeData(childIndex).SiblingIndex)
			nodeIndices.PushBack(childIndex);
	}
}

void SceneNodeHandler::GetSubtree(unsigned nodeIndex, Core::IndexVectorU& nodeIndices) const
{
	nodeIndices.Clear();
	AddSubtree(nodeIndex, nodeIndices);
}

unsigned SceneNodeHandler::GetBestAncestorCandidate(const Core::IndexVectorU& nodeIndices) const
{
	unsigned countNodes = nodeIndices.GetSize();
	if (countNodes == 0) return Core::c_InvalidIndexU;
	unsigned bestI = 0;
	unsigned smallestUpdateLevel = Core::c_InvalidSizeU;
	for (unsigned i = 0; i < countNodes; ++i)
	{
		auto& sceneNodeData = GetSceneNodeData(nodeIndices[i]);
		unsigned updateLevel = sceneNodeData.UpdateLevel;
		if (sceneNodeData.IsDynamic() && updateLevel < smallestUpdateLevel)
		{
			bestI = i;
			smallestUpdateLevel = updateLevel;
		}
	}
	return nodeIndices[bestI];
}

unsigned SceneNodeHandler::GetRootNode(const Core::IndexVectorU& nodeIndices) const
{
	unsigned ancestor = GetBestAncestorCandidate(nodeIndices);
	if (ancestor == Core::c_InvalidIndexU) return ancestor;
	ancestor = GetRoot(ancestor);
	Core::IndexVectorU subtree;
	AddSubtree(ancestor, subtree);
	std::unordered_set<unsigned> subtreeSet(subtree.GetArray(), subtree.GetEndPointer(), subtree.GetSize());
	unsigned countNodes = nodeIndices.GetSize();
	auto setEnd = subtreeSet.end();
	for (unsigned i = 0; i < countNodes; ++i)
	{
		unsigned node = nodeIndices[i];
		if (subtreeSet.find(node) == setEnd) return Core::c_InvalidIndexU;
	}
	return ancestor;
}

unsigned SceneNodeHandler::GetCommonAncestor(const Core::IndexVectorU& nodeIndices) const
{
	unsigned ancestor = GetBestAncestorCandidate(nodeIndices);
	if (ancestor == Core::c_InvalidIndexU) return ancestor;
	Core::IndexVectorU subtree;
	AddSubtree(ancestor, subtree);
	std::unordered_set<unsigned> subtreeSet(subtree.GetArray(), subtree.GetEndPointer(), subtree.GetSize());
	unsigned countNodes = nodeIndices.GetSize();
	auto setEnd = subtreeSet.end();
	for (unsigned i = 0; i < countNodes; ++i)
	{
		unsigned node = nodeIndices[i];
		if (subtreeSet.find(node) == setEnd)
		{
			auto ancestorParent = GetParent(ancestor);
			if (ancestorParent == Core::c_InvalidIndexU) return ancestorParent; // Two separate trees.
			for (auto apChild = GetSceneNodeData(ancestorParent).FirstChildIndex;
				apChild != Core::c_InvalidIndexU;
				apChild = GetSceneNodeData(apChild).SiblingIndex)
			{
				if (apChild != ancestor)
				{
					GetSubtree(apChild, subtree);
					subtreeSet.reserve(subtreeSet.size() + subtree.GetSize());
					subtreeSet.insert(subtree.GetArray(), subtree.GetEndPointer());
				}
			}
			subtreeSet.insert(ancestorParent);
			ancestor = ancestorParent;
			setEnd = subtreeSet.end();
		}
	}
	return ancestor;
}

///////////////////////////////////////////////////////////////////////////////////////////

const Core::SimpleTypeUnorderedVectorU<SceneNodeMainData>& SceneNodeHandler::GetMainDataVector() const
{
	return m_SceneNodeMainData;
}

Core::SimpleTypeUnorderedVectorU<SceneNodeMainData>& SceneNodeHandler::GetMainDataVector()
{
	return m_SceneNodeMainData;
}

const SceneNodeMainData* SceneNodeHandler::GetMainData() const
{
	return m_SceneNodeMainData.GetArray();
}

SceneNodeMainData* SceneNodeHandler::GetMainData()
{
	return m_SceneNodeMainData.GetArray();
}

const ScaledTransformation* SceneNodeHandler::GetScaledWorldTransformations() const
{
	return m_ScaledWorldTransformation.GetArray();
}

bool SceneNodeHandler::IsStatic(unsigned nodeIndex) const
{
	return GetMainData()[nodeIndex].IsStatic();
}

bool SceneNodeHandler::IsDynamic(unsigned nodeIndex) const
{
	return GetMainData()[nodeIndex].IsDynamic();
}

bool SceneNodeHandler::IsTransformationDirtyForUser(unsigned nodeIndex) const
{
	return GetMainData()[nodeIndex].IsTransformationDirtyForUser();
}

void SceneNodeHandler::SetTransformationUpToDateForUser(unsigned nodeIndex)
{
	GetMainData()[nodeIndex].SetTransformationUpToDateForUser();
}

///////////////////////////////////////////////////////////////////////////////////////////

ScaledTransformation SceneNodeHandler::GetWorldTransformation(unsigned sceneNodeIndex)
{
	auto& nodeData = m_SceneNodeMainData[sceneNodeIndex];
	unsigned parentIndex = nodeData.ParentIndex;
	if (parentIndex == Core::c_InvalidIndexU)
	{
		return nodeData.LocalTr;
	}
	else
	{
		auto& parentTransformation = GetScaledWorldTransformation(parentIndex);
		return
		{
			parentTransformation.A * nodeData.LocalTr.Orientation,
			parentTransformation.A * nodeData.LocalTr.Position + parentTransformation.Position
		};
	}
}

glm::mat3 SceneNodeHandler::GetWorldOrientation(unsigned nodeIndex)
{
	auto& nodeData = m_SceneNodeMainData[nodeIndex];
	unsigned parentIndex = nodeData.ParentIndex;
	if (parentIndex == Core::c_InvalidIndexU)
	{
		return nodeData.LocalTr.Orientation;
	}
	else
	{
		auto& parentTransformation = GetScaledWorldTransformation(parentIndex);
		return parentTransformation.A * nodeData.LocalTr.Orientation;
	}
}

ScaledTransformation SceneNodeHandler::GetInverseWorldTransformation(unsigned sceneNodeIndex)
{
	auto& nodeData = m_SceneNodeMainData[sceneNodeIndex];
	
	auto trOri = glm::transpose(nodeData.LocalTr.Orientation);
	
	unsigned parentIndex = nodeData.ParentIndex;
	if (parentIndex == Core::c_InvalidIndexU)
	{
		return{ trOri, trOri * -nodeData.LocalTr.Position };
	}
	else
	{
		auto& parentInverseTransformation = GetInverseScaledWorldTransformation(parentIndex);
		return
		{
			trOri * parentInverseTransformation.A,
			trOri * (parentInverseTransformation.Position - nodeData.LocalTr.Position)
		};
	}
}

void SceneNodeHandler::SetSceneNodeDirty(unsigned nodeIndex)
{
	HandleLocationPropertyChange(nodeIndex);
}

void SceneNodeHandler::HandleLocationPropertyChange(unsigned node)
{
	auto& nodeData = m_SceneNodeMainData[node];

	// Transformation is marked dirty both for the handler and the user.
	nodeData.SetTransformationDirty();

	if (nodeData.IsStatic())
	{
		// Static nodes' transformations are immediately updated, since they can't be dirty,
		// because the main update loop ignores them.
		UpdateScaledWorldTransformations(node);
	}

	for (unsigned child = nodeData.FirstChildIndex;
		child != Core::c_InvalidIndexU;
		child = m_SceneNodeMainData[child].SiblingIndex)
	{
		HandleLocationPropertyChange(child);
	}
}

void SceneNodeHandler::UpdateScaledWorldTransformations(unsigned nodeIndex)
{
	auto& nodeData = m_SceneNodeMainData[nodeIndex];
	if (nodeData.IsTransformationDirtyForHandler())
	{
		auto& result = m_ScaledWorldTransformation[nodeIndex];
		auto& invResult = m_InverseScaledWorldTransformation[nodeIndex];

		auto worldTransformation = GetWorldTransformation(nodeIndex);
		auto inverseWorldTransformation = GetInverseWorldTransformation(nodeIndex);

		auto& scaler = nodeData.Scaler;
		auto invScaler = glm::vec3(1.0f) / scaler;

		result.A = glm::mat3(
			worldTransformation.A[0] * scaler.x,
			worldTransformation.A[1] * scaler.y,
			worldTransformation.A[2] * scaler.z);
		result.Position = worldTransformation.Position;

		auto& c0 = inverseWorldTransformation.A[0];
		auto& c1 = inverseWorldTransformation.A[1];
		auto& c2 = inverseWorldTransformation.A[2];

		invResult.A = glm::mat3(
			c0.x * invScaler.x, c0.y * invScaler.y, c0.z * invScaler.z,
			c1.x * invScaler.x, c1.y * invScaler.y, c1.z * invScaler.z, 
			c2.x * invScaler.x, c2.y * invScaler.y, c2.z * invScaler.z);
		invResult.Position = inverseWorldTransformation.Position * invScaler;

		nodeData.SetTransformationUpToDateForHandler();
	}
}

void SceneNodeHandler::SetLocalOrientationAndScalingWithoutPropChangeHandling(unsigned nodeIndex, const glm::mat3x3& m)
{
	// Assuming that L = R * S = (x,y,z) * diag(sx, sy, sz) = (sx * x, sy * y, sz * z).
	auto sx = glm::length(m[0]);
	auto sy = glm::length(m[1]);
	auto sz = glm::length(m[2]);
	auto x = m[0] / sx;
	auto y = m[1] / sy;
	auto z = m[2] / sz;
	SetLocalOrientationWithoutPropChangeHandling(nodeIndex, { x, y, z });
	SetLocalScalerWithoutPropChangeHandling(nodeIndex, { sx, sy, sz });
}

const glm::vec3& SceneNodeHandler::GetLocalPosition(unsigned sceneNodeIndex) const
{
	return m_SceneNodeMainData[sceneNodeIndex].LocalTr.Position;
}

void SceneNodeHandler::SetLocalPosition(unsigned sceneNodeIndex, const glm::vec3& localPosition)
{
	SetLocalPositionWithoutPropChangeHandling(sceneNodeIndex, localPosition);
	HandleLocationPropertyChange(sceneNodeIndex);
}

void SceneNodeHandler::SetLocalPositionWithoutPropChangeHandling(unsigned sceneNodeIndex,
	const glm::vec3& localPosition)
{
	m_SceneNodeMainData[sceneNodeIndex].LocalTr.Position = localPosition;
}

const glm::mat3& SceneNodeHandler::GetLocalOrientation(unsigned sceneNodeIndex) const
{
	return m_SceneNodeMainData[sceneNodeIndex].LocalTr.Orientation;
}

void SceneNodeHandler::SetLocalOrientation(unsigned sceneNodeIndex, const glm::mat3& localOrientation)
{
	SetLocalOrientationWithoutPropChangeHandling(sceneNodeIndex, localOrientation);
	HandleLocationPropertyChange(sceneNodeIndex);
}

void SceneNodeHandler::SetLocalOrientationWithoutPropChangeHandling(unsigned sceneNodeIndex,
	const glm::mat3& localOrientation)
{
	m_SceneNodeMainData[sceneNodeIndex].LocalTr.Orientation = localOrientation;
}

const RigidTransformation& SceneNodeHandler::GetLocalTransformation(unsigned nodeIndex) const
{
	return m_SceneNodeMainData[nodeIndex].LocalTr;
}

void SceneNodeHandler::SetLocalTransformation(unsigned nodeIndex, const RigidTransformation& transformation)
{
	SetLocalTransformationWithoutPropChangeHandling(nodeIndex, transformation);
	HandleLocationPropertyChange(nodeIndex);
}

void SceneNodeHandler::SetLocalTransformation(unsigned nodeIndex, const ScaledTransformation& transformation)
{
	SetLocalTransformationWithoutPropChangeHandling(nodeIndex, transformation);
	HandleLocationPropertyChange(nodeIndex);
}

void SceneNodeHandler::SetLocalTransformationWithoutPropChangeHandling(unsigned nodeIndex,
	const RigidTransformation& transformation)
{
	m_SceneNodeMainData[nodeIndex].LocalTr = transformation;
}

void SceneNodeHandler::SetLocalTransformationWithoutPropChangeHandling(unsigned nodeIndex,
	const ScaledTransformation& transformation)
{
	SetLocalOrientationAndScalingWithoutPropChangeHandling(nodeIndex, transformation.A);
	SetLocalPositionWithoutPropChangeHandling(nodeIndex, transformation.Position);
}

const glm::vec3& SceneNodeHandler::GetLocalScaler(unsigned nodeIndex) const
{
	return m_SceneNodeMainData[nodeIndex].Scaler;
}

void SceneNodeHandler::SetLocalScaler(unsigned nodeIndex, const glm::vec3& scaler)
{
	SetLocalScalerWithoutPropChangeHandling(nodeIndex, scaler);
	HandleLocationPropertyChange(nodeIndex);
}

void SceneNodeHandler::SetLocalScalerWithoutPropChangeHandling(unsigned nodeIndex, const glm::vec3& scaler)
{
	m_SceneNodeMainData[nodeIndex].Scaler = scaler;
}

const ScaledTransformation& SceneNodeHandler::GetScaledWorldTransformation(unsigned nodeIndex)
{
	UpdateScaledWorldTransformations(nodeIndex);
	return m_ScaledWorldTransformation[nodeIndex];
}

const ScaledTransformation& SceneNodeHandler::GetInverseScaledWorldTransformation(unsigned nodeIndex)
{
	UpdateScaledWorldTransformations(nodeIndex);
	return m_InverseScaledWorldTransformation[nodeIndex];
}

const ScaledTransformation& SceneNodeHandler::UnsafeGetScaledWorldTransformation(unsigned nodeIndex) const
{
	return m_ScaledWorldTransformation[nodeIndex];
}

const ScaledTransformation& SceneNodeHandler::UnsafeGetInverseScaledWorldTransformation(unsigned nodeIndex) const
{
	return m_InverseScaledWorldTransformation[nodeIndex];
}

const glm::vec3& SceneNodeHandler::GetPosition(unsigned nodeIndex)
{
	UpdateScaledWorldTransformations(nodeIndex);
	return m_ScaledWorldTransformation[nodeIndex].Position;
}

const glm::vec3& SceneNodeHandler::UnsafeGetPosition(unsigned nodeIndex) const
{
	return m_ScaledWorldTransformation[nodeIndex].Position;
}

void SceneNodeHandler::SetPosition(unsigned nodeIndex, const glm::vec3& position)
{
	auto& nodeData = m_SceneNodeMainData[nodeIndex];
	unsigned parentIndex = nodeData.ParentIndex;
	if (parentIndex == Core::c_InvalidIndexU)
	{
		SetLocalPosition(nodeIndex, position);
	}
	else
	{
		// Computing the desired world transformation. Note that the node's scaling must be included in the
		// "desired transformation matrix", since the local 3x3 matrix setting sets both the orientation and scaling.
		auto swA = GetScaledWorldTransformation(nodeIndex).A;

		auto& parentInverseTransformation = GetInverseScaledWorldTransformation(parentIndex);
		SetLocalOrientationAndScalingWithoutPropChangeHandling(nodeIndex, parentInverseTransformation.A * swA);
		nodeData.LocalTr.Position = parentInverseTransformation.A * position + parentInverseTransformation.Position;
		HandleLocationPropertyChange(nodeIndex);
	}
}

glm::vec3 SceneNodeHandler::GetDirection(unsigned sceneNodeIndex)
{
	UpdateScaledWorldTransformations(sceneNodeIndex);
	return glm::normalize(m_ScaledWorldTransformation[sceneNodeIndex].A[0]);
}

glm::vec3 SceneNodeHandler::GetUp(unsigned sceneNodeIndex)
{
	UpdateScaledWorldTransformations(sceneNodeIndex);
	return glm::normalize(m_ScaledWorldTransformation[sceneNodeIndex].A[1]);
}

glm::vec3 SceneNodeHandler::GetRight(unsigned sceneNodeIndex)
{
	UpdateScaledWorldTransformations(sceneNodeIndex);
	return glm::normalize(m_ScaledWorldTransformation[sceneNodeIndex].A[2]);
}

glm::vec3 SceneNodeHandler::UnsafeGetDirection(unsigned sceneNodeIndex) const
{
	return glm::normalize(m_ScaledWorldTransformation[sceneNodeIndex].A[0]);
}

glm::vec3 SceneNodeHandler::UnsafeGetUp(unsigned sceneNodeIndex) const
{
	return glm::normalize(m_ScaledWorldTransformation[sceneNodeIndex].A[1]);
}

glm::vec3 SceneNodeHandler::UnsafeGetRight(unsigned sceneNodeIndex) const
{
	return glm::normalize(m_ScaledWorldTransformation[sceneNodeIndex].A[2]);
}

glm::mat3 SceneNodeHandler::GetOrientation(unsigned sceneNodeIndex)
{
	UpdateScaledWorldTransformations(sceneNodeIndex);
	return UnsafeGetOrientation(sceneNodeIndex);
}

glm::quat SceneNodeHandler::GetOrientationQuaternion(unsigned sceneNodeIndex)
{
	return glm::quat(GetOrientation(sceneNodeIndex));
}

glm::mat3 SceneNodeHandler::UnsafeGetOrientation(unsigned sceneNodeIndex) const
{
	auto& orientation = m_ScaledWorldTransformation[sceneNodeIndex].A;
	return glm::mat3(glm::normalize(orientation[0]),
		glm::normalize(orientation[1]), glm::normalize(orientation[2]));
}

glm::quat SceneNodeHandler::UnsafeGetOrientationQuaternion(unsigned sceneNodeIndex) const
{
	return glm::quat(UnsafeGetOrientation(sceneNodeIndex));
}

void SceneNodeHandler::SetOrientation(unsigned nodeIndex, const glm::mat3& orientation)
{
	auto& nodeData = m_SceneNodeMainData[nodeIndex];
	unsigned parentIndex = nodeData.ParentIndex;
	if (parentIndex == Core::c_InvalidIndexU)
	{
		// Assuming that the orientation matrix is orthonorm.
		SetLocalOrientation(nodeIndex, orientation);
	}
	else
	{
		// Computing the desired world position. Note that the position of the world and scaled world transformations are always equal.
		auto worldPosition = GetWorldTransformation(nodeIndex).Position;

		auto& parentInverseTransformation = GetInverseScaledWorldTransformation(parentIndex);
		SetLocalOrientationAndScalingWithoutPropChangeHandling(nodeIndex, parentInverseTransformation.A * orientation);
		nodeData.LocalTr.Position = parentInverseTransformation.A * worldPosition + parentInverseTransformation.Position;
		HandleLocationPropertyChange(nodeIndex);
	}
}

void SceneNodeHandler::SetOrientation(unsigned nodeIndex, const glm::quat& orientation)
{
	SetOrientation(nodeIndex, glm::mat3_cast(orientation));
}

void SceneNodeHandler::SetDirectionUp(unsigned nodeIndex, const glm::vec3& direction,
	const glm::vec3& up)
{
	auto right = glm::normalize(glm::cross(direction, up));
	auto upN = glm::normalize(glm::cross(right, direction));
	SetOrientation(nodeIndex, { glm::normalize(direction), upN, right });
}

void SceneNodeHandler::SetDirection(unsigned nodeIndex, const glm::vec3& direction)
{
	SetDirectionUp(nodeIndex, direction, c_WorldUp);
}

void SceneNodeHandler::SetLocation(unsigned nodeIndex, const ScaledTransformation& transformation)
{
	auto& nodeData = m_SceneNodeMainData[nodeIndex];
	unsigned parentIndex = nodeData.ParentIndex;
	if (parentIndex == Core::c_InvalidIndexU)
	{
		SetLocalTransformation(nodeIndex, transformation);
	}
	else
	{
		auto& parentInverseTransformation = GetInverseScaledWorldTransformation(parentIndex);
		SetLocalOrientationAndScalingWithoutPropChangeHandling(nodeIndex, parentInverseTransformation.A * transformation.A);
		nodeData.LocalTr.Position = parentInverseTransformation.A * transformation.Position + parentInverseTransformation.Position;
		HandleLocationPropertyChange(nodeIndex);
	}
}

void SceneNodeHandler::SetLocation(unsigned nodeIndex, const glm::vec3& position,
	const glm::vec3& direction, const glm::vec3& up)
{
	auto right = glm::normalize(glm::cross(direction, up));
	auto upN = glm::normalize(glm::cross(right, direction));
	SetLocation(nodeIndex, { { glm::normalize(direction), upN, right }, position });
}

void SceneNodeHandler::SetLocation(unsigned nodeIndex, const glm::vec3& position,
	const glm::vec3& direction)
{
	SetLocation(nodeIndex, position, direction, c_WorldUp);
}

glm::vec3 SceneNodeHandler::GetLookAt(unsigned nodeIndex)
{
	auto& transformation = GetScaledWorldTransformation(nodeIndex);
	return transformation.Position + transformation.A[0];
}

void SceneNodeHandler::LookAt(unsigned nodeIndex, const glm::vec3& lookAtPosition)
{
	auto position = GetPosition(nodeIndex);
	SetDirection(nodeIndex, lookAtPosition - position);
}

///////////////////////////////////////////////////////////////////////////////////////////

void SceneNodeHandler::UpdateTransformations()
{
	auto countUpdateLevels = static_cast<unsigned char>(m_SceneNodeIndicesForUpdate.size());
	for (unsigned char updateLevel = 0; updateLevel < countUpdateLevels; updateLevel++)
	{
		GatherDirtyIndices(updateLevel);
		UpdateTransformationsInParallel(updateLevel);
	}
}

void SceneNodeHandler::UpdateTransformations(const Core::ByteVectorU& allowedMask)
{
	auto countUpdateLevels = static_cast<unsigned char>(m_SceneNodeIndicesForUpdate.size());
	for (unsigned char updateLevel = 0; updateLevel < countUpdateLevels; updateLevel++)
	{
		GatherDirtyIndices(updateLevel, allowedMask);
		UpdateTransformationsInParallel(updateLevel);
	}
}

void SceneNodeHandler::UpdateTransformations(const Core::IndexVectorU& allowedIndices)
{
	auto countUpdateLevels = static_cast<unsigned char>(m_SceneNodeIndicesForUpdate.size());
	for (unsigned char updateLevel = 0; updateLevel < countUpdateLevels; updateLevel++)
	{
		UpdateAllowedMask(allowedIndices);
		GatherDirtyIndices(updateLevel, m_TempAllowedMask);
		UpdateTransformationsInParallel(updateLevel);
	}
}

void SceneNodeHandler::UpdateAllowedMask(const Core::IndexVectorU& allowedIndices)
{
	unsigned countAllowedIndices = allowedIndices.GetSize();
	m_TempAllowedMask.Clear();
	for (unsigned i = 0; i < countAllowedIndices; i++)
	{
		unsigned index = allowedIndices[i];
		unsigned maskArraySize = m_TempAllowedMask.GetSize();
		if (index >= maskArraySize)
			m_TempAllowedMask.PushBack(Core::c_False, index - maskArraySize + 1);
		m_TempAllowedMask[index] = Core::c_True;
	}
}

void SceneNodeHandler::GatherDirtyIndices(unsigned char updateLevel)
{
	auto mainData = GetMainData();
	auto& nodeIndices = m_SceneNodeIndicesForUpdate[updateLevel];
	m_DirtySceneNodeIndices.ClearAndReserve(nodeIndices.GetSize());

	auto it = nodeIndices.GetBeginIterator();
	auto end = nodeIndices.GetEndIterator();
	for (; it != end; ++it)
	{
		unsigned sceneNodeIndex = *it;

		if (mainData[sceneNodeIndex].IsTransformationDirtyForHandler())
		{
			m_DirtySceneNodeIndices.UnsafePushBackPlaceHolder() = sceneNodeIndex;
		}
	}
}

void SceneNodeHandler::GatherDirtyIndices(unsigned char updateLevel, const Core::ByteVectorU& allowedMask)
{
	auto allowedMaskSize = allowedMask.GetSize();
	auto mainData = GetMainData();
	auto& nodeIndices = m_SceneNodeIndicesForUpdate[updateLevel];
	m_DirtySceneNodeIndices.ClearAndReserve(nodeIndices.GetSize());

	auto it = nodeIndices.GetBeginIterator();
	auto end = nodeIndices.GetEndIterator();
	for (; it != end; ++it)
	{
		unsigned sceneNodeIndex = *it;

		if (mainData[sceneNodeIndex].IsTransformationDirtyForHandler()
			&& sceneNodeIndex < allowedMaskSize && allowedMask[sceneNodeIndex])
		{
			m_DirtySceneNodeIndices.UnsafePushBackPlaceHolder() = sceneNodeIndex;
		}
	}

#ifdef _DEBUG
	if (m_CheckForSubsetUpdateSafety)
	{
		for (unsigned i = 0; i < allowedMaskSize; i++)
		{
			if (allowedMask[i])
			{
				unsigned parentIndex = mainData[i].ParentIndex;
				if (parentIndex != Core::c_InvalidIndexU && (parentIndex >= allowedMaskSize || !allowedMask[parentIndex]))
				{
					EngineBuildingBlocks::RaiseException("The given subset of scene nodes cannot be safely updated.");
				}
			}
		}
	}
#endif
}

#ifdef _DEBUG
void SceneNodeHandler::SetIsCheckingForSubsetUpdateSafety(bool isChecking)
{
	m_CheckForSubsetUpdateSafety = isChecking;
}
#endif

void SceneNodeHandler::UpdateTransformationsInParallel(unsigned char updateLevel)
{
	if (updateLevel == 0)
	{
		m_ThreadPool.ExecuteWithStaticScheduling(m_DirtySceneNodeIndices.GetSize(),
			&SceneNodeHandler::UpdateTransformationsInThreadWithoutParent, this);
	}
	else
	{
		m_ThreadPool.ExecuteWithStaticScheduling(m_DirtySceneNodeIndices.GetSize(),
			&SceneNodeHandler::UpdateTransformationsInThreadWithParent, this);
	}
}


void SceneNodeHandler::UpdateTransformationsInThreadWithoutParent(unsigned threadId,
	unsigned startIndex, unsigned endIndex)
{
	auto dirtySceneNodeIndices = m_DirtySceneNodeIndices.GetArray();
	auto nodeDataVector = GetMainData();
	auto transformations = m_ScaledWorldTransformation.GetArray();
	auto invTransformations = m_InverseScaledWorldTransformation.GetArray();

#if(IS_USING_AVX2)

	unsigned laneIndex = 0;
	const unsigned c_CountLanes = 8;

	glm::mat3 trLocalOrientation_;

	__m256i sceneNodeIndices;
	Math::Matrix3x3_256 localOrientation, trLocalOrientation, resultA, invResultA;
	Math::Vector3_256 localPosition, scaler, invResultPosition;

	Math::Vector3_256 oneVector(_mm256_set1_ps(1.0f));

	unsigned limitIndex = endIndex - 1;
	for (unsigned taskIndex = startIndex; taskIndex < endIndex; taskIndex++)
	{
		unsigned sceneNodeIndex = dirtySceneNodeIndices[taskIndex];
		auto& nodeData = nodeDataVector[sceneNodeIndex];

		auto pLocalOrientation = &nodeData.LocalTr.Orientation;

		// Vector-gather data.
		sceneNodeIndices.m256i_u32[laneIndex] = sceneNodeIndex;
		localOrientation.WriteScalar(laneIndex, reinterpret_cast<float*>(pLocalOrientation));
		trLocalOrientation.WriteScalarTransposed(laneIndex, reinterpret_cast<float*>(pLocalOrientation));
		localPosition.WriteScalar(laneIndex, reinterpret_cast<float*>(&nodeData.LocalTr.Position));
		scaler.WriteScalar(laneIndex, reinterpret_cast<float*>(&nodeData.Scaler));

		++laneIndex;

		if (laneIndex == c_CountLanes || taskIndex == limitIndex)
		{
			// Actual computation.
			resultA = Math::Matrix3x3_256(
				localOrientation.Columns[0] * scaler.X,
				localOrientation.Columns[1] * scaler.Y,
				localOrientation.Columns[2] * scaler.Z);

			auto invScaler = oneVector / scaler;
			invResultA = Math::Matrix3x3_256(
				trLocalOrientation.M00 * invScaler.X, trLocalOrientation.M01 * invScaler.Y, trLocalOrientation.M02 * invScaler.Z,
				trLocalOrientation.M10 * invScaler.X, trLocalOrientation.M11 * invScaler.Y, trLocalOrientation.M12 * invScaler.Z,
				trLocalOrientation.M20 * invScaler.X, trLocalOrientation.M21 * invScaler.Y, trLocalOrientation.M22 * invScaler.Z);
			invResultPosition = trLocalOrientation * -localPosition * invScaler;

			// Writing results devectorized.
			for (unsigned i = 0; i < laneIndex; i++)
			{
				unsigned sceneNodeIndexD = sceneNodeIndices.m256i_u32[i];
				auto& transformationD = transformations[sceneNodeIndexD];
				auto& invTransformationD = invTransformations[sceneNodeIndexD];

				resultA.ReadScalar(i, reinterpret_cast<float*>(&transformationD.A));
				localPosition.ReadScalar(i, reinterpret_cast<float*>(&transformationD.Position));
				invResultA.ReadScalar(i, reinterpret_cast<float*>(&invTransformationD.A));
				invResultPosition.ReadScalar(i, reinterpret_cast<float*>(&invTransformationD.Position));
			}
			laneIndex = 0;
		}

		nodeData.SetTransformationUpToDateForHandler();
	}

#else

	glm::vec3 oneVector(1.0f);
	glm::mat3 trLocalOrientation;
	auto pTrLocal = reinterpret_cast<float*>(&trLocalOrientation);

	for (unsigned taskIndex = startIndex; taskIndex < endIndex; taskIndex++)
	{
		unsigned sceneNodeIndex = dirtySceneNodeIndices[taskIndex];
		auto& nodeData = nodeDataVector[sceneNodeIndex];
		auto& localOrientation = nodeData.LocalTr.Orientation;
		auto& localPosition = nodeData.LocalTr.Position;
		auto& scaler = nodeData.Scaler;
		auto& transformation = transformations[sceneNodeIndex];
		auto& invTransformation = invTransformations[sceneNodeIndex];

		Transpose3x3(reinterpret_cast<float*>(&localOrientation), pTrLocal);

		transformation.A = glm::mat3(
			localOrientation[0] * scaler.x,
			localOrientation[1] * scaler.y,
			localOrientation[2] * scaler.z);
		transformation.Position = localPosition;

		auto invScaler = oneVector / scaler;
		invTransformation.A = glm::mat3(
			pTrLocal[0] * invScaler.x, pTrLocal[1] * invScaler.y, pTrLocal[2] * invScaler.z,
			pTrLocal[3] * invScaler.x, pTrLocal[4] * invScaler.y, pTrLocal[5] * invScaler.z,
			pTrLocal[6] * invScaler.x, pTrLocal[7] * invScaler.y, pTrLocal[8] * invScaler.z);
		invTransformation.Position = trLocalOrientation * -localPosition * invScaler;

		nodeData.SetTransformationUpToDateForHandler();
	}

#endif
}

void SceneNodeHandler::UpdateTransformationsInThreadWithParent(unsigned threadId,
	unsigned startIndex, unsigned endIndex)
{
	auto dirtySceneNodeIndices = m_DirtySceneNodeIndices.GetArray();
	auto nodeDataVector = GetMainData();
	auto transformations = m_ScaledWorldTransformation.GetArray();
	auto invTransformations = m_InverseScaledWorldTransformation.GetArray();

#if(IS_USING_AVX2)

	unsigned laneIndex = 0;
	const unsigned c_CountLanes = 8;

	__m256i sceneNodeIndices;
	Math::Matrix3x3_256 localOrientation, trLocalOrientation, parentA, parentInvA, worldA,
		invWorldA, resultA, invResultA;
	Math::Vector3_256 localPosition, scaler, parentPosition, parentInvPosition,
		resultPosition, invResultPosition;

	Math::Vector3_256 oneVector(_mm256_set1_ps(1.0f));

	unsigned limitIndex = endIndex - 1;
	for (unsigned taskIndex = startIndex; taskIndex < endIndex; taskIndex++)
	{
		unsigned sceneNodeIndex = dirtySceneNodeIndices[taskIndex];
		auto& nodeData = nodeDataVector[sceneNodeIndex];
		unsigned parentIndex = nodeData.ParentIndex;
		auto& parentTransformation = transformations[parentIndex];
		auto& parentInvTransformation = invTransformations[parentIndex];

		auto pLocalOrientation = &nodeData.LocalTr.Orientation;

		// Vector-gather data.
		sceneNodeIndices.m256i_u32[laneIndex] = sceneNodeIndex;
		localOrientation.WriteScalar(laneIndex, reinterpret_cast<float*>(pLocalOrientation));
		trLocalOrientation.WriteScalarTransposed(laneIndex, reinterpret_cast<float*>(pLocalOrientation));
		localPosition.WriteScalar(laneIndex, reinterpret_cast<float*>(&nodeData.LocalTr.Position));
		scaler.WriteScalar(laneIndex, reinterpret_cast<float*>(&nodeData.Scaler));
		parentA.WriteScalar(laneIndex, reinterpret_cast<float*>(&parentTransformation.A));
		parentPosition.WriteScalar(laneIndex, reinterpret_cast<float*>(&parentTransformation.Position));
		parentInvA.WriteScalar(laneIndex, reinterpret_cast<float*>(&parentInvTransformation.A));
		parentInvPosition.WriteScalar(laneIndex, reinterpret_cast<float*>(&parentInvTransformation.Position));

		++laneIndex;

		if (laneIndex == c_CountLanes || taskIndex == limitIndex)
		{
			// Actual computation.
			worldA = parentA * localOrientation;
			resultA = Math::Matrix3x3_256(
				worldA.Columns[0] * scaler.X,
				worldA.Columns[1] * scaler.Y,
				worldA.Columns[2] * scaler.Z);
			resultPosition = parentA * localPosition + parentPosition;

			auto invScaler = oneVector / scaler;
			invWorldA = trLocalOrientation * parentInvA;
			invResultA = Math::Matrix3x3_256(
				invWorldA.M00 * invScaler.X, invWorldA.M01 * invScaler.Y, invWorldA.M02 * invScaler.Z,
				invWorldA.M10 * invScaler.X, invWorldA.M11 * invScaler.Y, invWorldA.M12 * invScaler.Z,
				invWorldA.M20 * invScaler.X, invWorldA.M21 * invScaler.Y, invWorldA.M22 * invScaler.Z);
			invResultPosition = (trLocalOrientation * (parentInvPosition - localPosition)) * invScaler;

			// Writing results devectorized.
			for (unsigned i = 0; i < laneIndex; i++)
			{
				unsigned sceneNodeIndexD = sceneNodeIndices.m256i_u32[i];
				auto& transformationD = transformations[sceneNodeIndexD];
				auto& invTransformationD = invTransformations[sceneNodeIndexD];

				resultA.ReadScalar(i, reinterpret_cast<float*>(&transformationD.A));
				resultPosition.ReadScalar(i, reinterpret_cast<float*>(&transformationD.Position));
				invResultA.ReadScalar(i, reinterpret_cast<float*>(&invTransformationD.A));
				invResultPosition.ReadScalar(i, reinterpret_cast<float*>(&invTransformationD.Position));
			}
			laneIndex = 0;
		}

		nodeData.SetTransformationUpToDateForHandler();
	}

#else

	glm::vec3 oneVector(1.0f);
	glm::mat3 trLocalOrientation, worldA, invWorldA;
	auto pTrLocal = reinterpret_cast<float*>(&trLocalOrientation);
	auto pInvWorldA = reinterpret_cast<float*>(&invWorldA);

	for (unsigned taskIndex = startIndex; taskIndex < endIndex; taskIndex++)
	{
		unsigned sceneNodeIndex = dirtySceneNodeIndices[taskIndex];
		auto& nodeData = nodeDataVector[sceneNodeIndex];
		auto& localOrientation = nodeData.LocalTr.Orientation;
		auto& localPosition = nodeData.LocalTr.Position;
		auto& scaler = nodeData.Scaler;
		auto& transformation = transformations[sceneNodeIndex];
		auto& invTransformation = invTransformations[sceneNodeIndex];
		unsigned parentIndex = nodeData.ParentIndex;
		auto& parentTransformation = transformations[parentIndex];
		auto& parentInvTransformation = invTransformations[parentIndex];

		Transpose3x3(reinterpret_cast<float*>(&localOrientation), pTrLocal);

		worldA = parentTransformation.A * localOrientation;
		transformation.A = glm::mat3(
			worldA[0] * scaler.x,
			worldA[1] * scaler.y,
			worldA[2] * scaler.z);
		transformation.Position = parentTransformation.A * localPosition
			+ parentTransformation.Position;

		auto invScaler = oneVector / scaler;
		invWorldA = trLocalOrientation * parentInvTransformation.A;
		invTransformation.A = glm::mat3(
			pInvWorldA[0] * invScaler.x, pInvWorldA[1] * invScaler.y, pInvWorldA[2] * invScaler.z,
			pInvWorldA[3] * invScaler.x, pInvWorldA[4] * invScaler.y, pInvWorldA[5] * invScaler.z,
			pInvWorldA[6] * invScaler.x, pInvWorldA[7] * invScaler.y, pInvWorldA[8] * invScaler.z);
		invTransformation.Position = (trLocalOrientation
			* (parentInvTransformation.Position - localPosition)) * invScaler;

		nodeData.SetTransformationUpToDateForHandler();
	}

#endif
}

std::map<unsigned, unsigned> SceneNodeHandler::CompactSceneNodes(bool isShrinkingUnderlyingVectors)
{
	// Updating SOA unordered vector containers. It also creates the index mapping.
	auto indexMapping = m_SceneNodeMainData.ShrinkToFit(isShrinkingUnderlyingVectors);
	m_ScaledWorldTransformation.ShrinkToFit(isShrinkingUnderlyingVectors);
	m_InverseScaledWorldTransformation.ShrinkToFit(isShrinkingUnderlyingVectors);

	// Updating the update indices.
	unsigned countUpdateLevels = static_cast<unsigned>(m_SceneNodeIndicesForUpdate.size());
	for (unsigned i = 0; i < countUpdateLevels; i++)
	{
		auto& updateIndices = m_SceneNodeIndicesForUpdate[i];
		auto uIt = updateIndices.GetBeginIterator();
		auto uEnd = updateIndices.GetEndIterator();
		for (; uIt != uEnd; ++uIt)
		{
			*uIt = indexMapping[*uIt];
		}
	}

	// For the next updating we need the invalid index to be mapped to the invalid index.
	indexMapping[Core::c_InvalidIndexU] = Core::c_InvalidIndexU;

	// Updating the indices in the scene nodes. We can directly access the compacted scene nodes.
	unsigned countSceneNodes = m_SceneNodeMainData.GetSize();
	for (unsigned i = 0; i < countSceneNodes; i++)
	{
		auto& sceneNodeData = m_SceneNodeMainData[i];
		sceneNodeData.ParentIndex = indexMapping[sceneNodeData.ParentIndex];
		sceneNodeData.FirstChildIndex = indexMapping[sceneNodeData.FirstChildIndex];
		sceneNodeData.SiblingIndex = indexMapping[sceneNodeData.SiblingIndex];
	}

	// Removing the invalid index mapping.
	indexMapping.erase(Core::c_InvalidIndexU);

	// Updating wrapped scene nodes.
	auto wIt = m_WrappedSceneNodes.GetBeginIterator();
	auto wEnd = m_WrappedSceneNodes.GetEndIterator();
	for (; wIt != wEnd; ++wIt)
	{
		(*wIt)->_UpdateSceneNodeIndex(indexMapping);
	}

	return indexMapping;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WrappedSceneNode::WrappedSceneNode()
	: m_Handler(nullptr)
	, m_SceneNodeIndex(Core::c_InvalidIndexU)
	, m_WrappedSceneNodeIndex(Core::c_InvalidIndexU)
{
}

WrappedSceneNode::WrappedSceneNode(SceneNodeHandler* handler, bool isStatic,
	unsigned char updateLevelHint)
	: m_Handler(handler)
{
	m_SceneNodeIndex = m_Handler->CreateSceneNode(isStatic, updateLevelHint);
	m_WrappedSceneNodeIndex = m_Handler->_RegisterWrappedSceneNode(this);
}

WrappedSceneNode::WrappedSceneNode(SceneNodeHandler* handler, unsigned sceneNodeIndex)
	: m_Handler(handler)
	, m_SceneNodeIndex(sceneNodeIndex)
{
	m_WrappedSceneNodeIndex = m_Handler->_RegisterWrappedSceneNode(this);
}

WrappedSceneNode::WrappedSceneNode(WrappedSceneNode&& other)
	: m_Handler(other.m_Handler)
	, m_SceneNodeIndex(other.m_SceneNodeIndex)
	, m_WrappedSceneNodeIndex(other.m_WrappedSceneNodeIndex)
{
	other.m_SceneNodeIndex = Core::c_InvalidIndexU;
	other.m_WrappedSceneNodeIndex = Core::c_InvalidIndexU;
	other.m_Handler = nullptr;

	m_Handler->_UpdateWrappedSceneNode(m_WrappedSceneNodeIndex, this);
}

void WrappedSceneNode::DeleteSceneNode()
{
	if (IsValid())
	{
		m_Handler->_DeregisterWrappedSceneNode(m_WrappedSceneNodeIndex);
		m_Handler->DeleteSceneNode(m_SceneNodeIndex, true);
	}
}

void WrappedSceneNode::_UpdateSceneNodeIndex(std::map<unsigned, unsigned>& indexMapping)
{
	assert(m_SceneNodeIndex != Core::c_InvalidIndexU);

	m_SceneNodeIndex = indexMapping[m_SceneNodeIndex];
}

WrappedSceneNode::~WrappedSceneNode()
{
	DeleteSceneNode();
}

WrappedSceneNode& WrappedSceneNode::operator=(WrappedSceneNode&& other)
{
	std::swap(m_SceneNodeIndex, other.m_SceneNodeIndex);
	std::swap(m_WrappedSceneNodeIndex, other.m_WrappedSceneNodeIndex);
	std::swap(m_Handler, other.m_Handler);

	m_Handler->_UpdateWrappedSceneNode(m_WrappedSceneNodeIndex, this);
	m_Handler->_UpdateWrappedSceneNode(other.m_WrappedSceneNodeIndex, &other);

	return *this;
}

bool WrappedSceneNode::IsValid() const
{
	return (m_SceneNodeIndex != Core::c_InvalidIndexU && m_Handler != nullptr);
}

void WrappedSceneNode::Set(SceneNodeHandler* handler, unsigned sceneNodeIndex)
{
	DeleteSceneNode();
	m_Handler = handler;
	m_SceneNodeIndex = sceneNodeIndex;
	m_WrappedSceneNodeIndex = handler->_RegisterWrappedSceneNode(this);
}

SceneNodeHandler* WrappedSceneNode::GetSceneNodeHandler()
{
	return m_Handler;
}

bool WrappedSceneNode::IsStatic() const
{
	return m_Handler->IsStatic(m_SceneNodeIndex);
}

bool WrappedSceneNode::IsDynamic() const
{
	return m_Handler->IsDynamic(m_SceneNodeIndex);
}

bool WrappedSceneNode::IsTransformationDirtyForUser() const
{
	return m_Handler->IsTransformationDirtyForUser(m_SceneNodeIndex);
}

void WrappedSceneNode::SetTransformationUpToDateForUser()
{
	m_Handler->SetTransformationUpToDateForUser(m_SceneNodeIndex);
}

///////////////////////////////////////////////////////////////////////////////////////////

void WrappedSceneNode::AddChild(unsigned childIndex)
{
	m_Handler->SetConnection(m_SceneNodeIndex, childIndex);
}

void WrappedSceneNode::AddChild(WrappedSceneNode& child)
{
	if (m_Handler != child.m_Handler)
	{
		RaiseException("It's invalid to connect scene nodes of different scene node handlers.");
	}
	AddChild(child.m_SceneNodeIndex);
}

void WrappedSceneNode::RemoveChild(unsigned childIndex)
{
	if (IsChild(childIndex))
	{
		m_Handler->OrphanSceneNode(childIndex);
	}
	else
	{
		RaiseException("It's invalid to disconnect scene nodes without a parent-child relation.");
	}
}

void WrappedSceneNode::RemoveChild(WrappedSceneNode& child)
{
	if (m_Handler != child.m_Handler)
	{
		RaiseException("It's invalid to disconnect scene nodes of different scene node handlers.");
	}
	RemoveChild(child.m_SceneNodeIndex);
}

bool WrappedSceneNode::IsChild(unsigned childIndex) const
{
	return m_Handler->IsChildOf(m_SceneNodeIndex, childIndex);
}

bool WrappedSceneNode::IsChild(WrappedSceneNode& child) const
{
	if (m_Handler != child.m_Handler)
	{
		RaiseException("It's invalid to query scene node connection data of different scene node handlers.");
	}
	return IsChild(child.m_SceneNodeIndex);
}

unsigned WrappedSceneNode::GetParentSceneNodeIndex() const
{
	return m_Handler->GetParent(m_SceneNodeIndex);
}

unsigned WrappedSceneNode::GetCountChildren() const
{
	return m_Handler->GetCountChildren(m_SceneNodeIndex);
}

void WrappedSceneNode::GetChildrenIndices(Core::IndexVectorU& childrenIndices) const
{
	m_Handler->GetChildren(m_SceneNodeIndex, childrenIndices);
}

Core::IndexVectorU WrappedSceneNode::GetChildrenIndices() const
{
	Core::IndexVectorU result;
	GetChildrenIndices(result);
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////

const RigidTransformation& WrappedSceneNode::GetLocalTransformation() const
{
	return m_Handler->GetLocalTransformation(m_SceneNodeIndex);
}

void WrappedSceneNode::SetLocalTransformation(const RigidTransformation& transformation)
{
	m_Handler->SetLocalTransformation(m_SceneNodeIndex, transformation);
}

void WrappedSceneNode::SetLocalTransformation(const ScaledTransformation& transformation)
{
	m_Handler->SetLocalTransformation(m_SceneNodeIndex, transformation);
}

const glm::vec3& WrappedSceneNode::GetLocalPosition() const
{
	return m_Handler->GetLocalPosition(m_SceneNodeIndex);
}

void WrappedSceneNode::SetLocalPosition(const glm::vec3& localPosition)
{
	m_Handler->SetLocalPosition(m_SceneNodeIndex, localPosition);
}

const glm::mat3& WrappedSceneNode::GetLocalOrientation() const
{
	return m_Handler->GetLocalOrientation(m_SceneNodeIndex);
}

void WrappedSceneNode::SetLocalOrientation(const glm::mat3& localOrientation)
{
	m_Handler->SetLocalOrientation(m_SceneNodeIndex, localOrientation);
}

const glm::vec3& WrappedSceneNode::GetScaler() const
{
	return m_Handler->GetLocalScaler(m_SceneNodeIndex);
}

void WrappedSceneNode::SetScaler(const glm::vec3& scaler)
{
	m_Handler->SetLocalScaler(m_SceneNodeIndex, scaler);
}

ScaledTransformation WrappedSceneNode::GetWorldTransformation()
{
	return m_Handler->GetWorldTransformation(m_SceneNodeIndex);
}

glm::mat3 WrappedSceneNode::GetWorldOrientation()
{
	return m_Handler->GetWorldOrientation(m_SceneNodeIndex);
}

ScaledTransformation WrappedSceneNode::GetInverseWorldTransformation()
{
	return m_Handler->GetInverseWorldTransformation(m_SceneNodeIndex);
}

const ScaledTransformation& WrappedSceneNode::GetScaledWorldTransformation()
{
	return m_Handler->GetScaledWorldTransformation(m_SceneNodeIndex);
}

const ScaledTransformation& WrappedSceneNode::GetInverseScaledWorldTransformation()
{
	return m_Handler->GetInverseScaledWorldTransformation(m_SceneNodeIndex);
}

const glm::vec3& WrappedSceneNode::GetPosition()
{
	return m_Handler->GetPosition(m_SceneNodeIndex);
}

void WrappedSceneNode::SetPosition(const glm::vec3& position)
{
	m_Handler->SetPosition(m_SceneNodeIndex, position);
}

glm::vec3 WrappedSceneNode::GetDirection()
{
	return m_Handler->GetDirection(m_SceneNodeIndex);
}

glm::vec3 WrappedSceneNode::GetUp()
{
	return m_Handler->GetUp(m_SceneNodeIndex);
}

glm::vec3 WrappedSceneNode::GetRight()
{
	return m_Handler->GetRight(m_SceneNodeIndex);
}

glm::mat3 WrappedSceneNode::GetOrientation() const
{
	return m_Handler->GetOrientation(m_SceneNodeIndex);
}

glm::quat WrappedSceneNode::GetOrientationQuaternion() const
{
	return m_Handler->GetOrientationQuaternion(m_SceneNodeIndex);
}

void WrappedSceneNode::SetOrientation(const glm::mat3& orientation)
{
	m_Handler->SetOrientation(m_SceneNodeIndex, orientation);
}

void WrappedSceneNode::SetOrientation(const glm::quat& orientation)
{
	m_Handler->SetOrientation(m_SceneNodeIndex, orientation);
}

void WrappedSceneNode::SetDirectionUp(const glm::vec3& direction, const glm::vec3& up)
{
	m_Handler->SetDirectionUp(m_SceneNodeIndex, direction, up);
}

void WrappedSceneNode::SetDirection(const glm::vec3& direction)
{
	m_Handler->SetDirection(m_SceneNodeIndex, direction);
}

void WrappedSceneNode::SetLocation(const ScaledTransformation& transformation)
{
	m_Handler->SetLocation(m_SceneNodeIndex, transformation);
}

void WrappedSceneNode::SetLocation(const glm::vec3& position, const glm::vec3& direction,
	const glm::vec3& up)
{
	m_Handler->SetLocation(m_SceneNodeIndex, position, direction, up);
}

void WrappedSceneNode::SetLocation(const glm::vec3& position, const glm::vec3& direction)
{
	m_Handler->SetLocation(m_SceneNodeIndex, position, direction);
}

glm::vec3 WrappedSceneNode::GetLookAt()
{
	return m_Handler->GetLookAt(m_SceneNodeIndex);
}

void WrappedSceneNode::LookAt(const glm::vec3& lookAtPosition)
{
	m_Handler->LookAt(m_SceneNodeIndex, lookAtPosition);
}
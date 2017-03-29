// EngineBuildingBlocks/SceneNode.h

#ifndef _ENGINEBUILDINGBLOCKS_SCENENODE_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_SCENENODE_H_INCLUDED_

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/DataStructures/SimpleTypeUnorderedVector.hpp>
#include <Core/System/ThreadPool.h>
#include <EngineBuildingBlocks/Math/Transformations.h>

#include <vector>
#include <map>
#include <mutex>

namespace EngineBuildingBlocks
{
	struct SceneNodeMainData
	{
		// Masks.
		static const unsigned c_DynamicMask = 0x00000001;
		static const unsigned c_TransformationDirtyForHandlerMask = 0x00000002;

		// This flags indicates whether the user has already handled the event
		// that the transformation became dirty. Note that we implement this functionality
		// at this level, because here we get it for free.
		static const unsigned c_TransformationDirtyForUserMask = 0x00000004;

		unsigned Flags;
		unsigned char UpdateLevel;
		unsigned UpdateIndex;

		unsigned ParentIndex;
		unsigned FirstChildIndex;
		unsigned SiblingIndex;

		RigidTransformation LocalTr;
		glm::vec3 Scaler;

		bool IsStatic() const;
		bool IsDynamic() const;

		void SetStatic();
		void SetDynamic();

		bool IsTransformationDirtyForHandler() const;
		bool IsTransformationDirtyForUser() const;

		void SetTransformationDirty();
		void SetTransformationUpToDateForHandler();
		void SetTransformationUpToDateForUser();

		bool HasChild() const;
	};

	class WrappedSceneNode;

	class SceneNodeHandler
	{
	private: // The following arrays are indexed by the scene node index.
	         // We may want to have later AOS -> SOA transformation.	

		// Main scene node data: flags, connection data, update data.
		Core::SimpleTypeUnorderedVectorU<SceneNodeMainData> m_SceneNodeMainData;

		Core::SimpleTypeUnorderedVectorU<ScaledTransformation> m_ScaledWorldTransformation;
		Core::SimpleTypeUnorderedVectorU<ScaledTransformation> m_InverseScaledWorldTransformation;

	private:

		// First index: UPDATE LEVEL, second UPDATE INDEX. Data: scene NODE INDEX.
		std::vector<Core::SimpleTypeUnorderedVectorU<unsigned>> m_SceneNodeIndicesForUpdate;

		Core::ThreadPool m_ThreadPool;

	private: // Function local data.

		Core::IndexVectorU m_DirtySceneNodeIndices;

	private: // Special handling for wrapped scene nodes.

		Core::SimpleTypeUnorderedVectorU<WrappedSceneNode*> m_WrappedSceneNodes;

	public:

		unsigned _RegisterWrappedSceneNode(WrappedSceneNode* wrappedSceneNode);
		void _DeregisterWrappedSceneNode(unsigned wrappedSceneNodeIndex);

		void _UpdateWrappedSceneNode(unsigned wrappedSceneNodeIndex, WrappedSceneNode* newWrappedSceneNode);

	public:

		static const glm::vec3 c_WorldUp;

	private:

		SceneNodeMainData& GetSceneNodeData(unsigned sceneNodeIndex);
		const SceneNodeMainData& GetSceneNodeData(unsigned sceneNodeIndex) const;

		void RegisterSceneNodeForUpdate(SceneNodeMainData& nodeData, unsigned sceneNodeIndex,
			unsigned updateLevel);
		void DeregisterSceneNodeForUpdate(SceneNodeMainData& nodeData);

	public: // Scene node creation and deletion.

		unsigned CreateSceneNode(bool isStatic, unsigned updateLevelHint = 0);
		void DeleteSceneNode(unsigned sceneNodeIndex, bool isUpdatingConnections);

	private: // Connection setting.

		void AddChildToChain(unsigned parentSceneNodeIndex, unsigned childSceneNodeIndex);
		void RemoveChildFromChain(unsigned parentSceneNodeIndex, unsigned childSceneNodeIndex);

		void OrphanSceneNode(unsigned nodeIndex, bool isHandlingUpdateLevel);

		void SetUpdateLevel(unsigned nodeIndex, unsigned updateLevel);

		unsigned GetBestAncestorCandidate(const Core::IndexVectorU& nodeIndices) const;

	public:

		void SetConnection(unsigned parentSceneNodeIndex, unsigned childSceneNodeIndex);
		void OrphanSceneNode(unsigned sceneNodeIndex);

		unsigned GetParent(unsigned node) const;
		unsigned GetRoot(unsigned node) const;
		unsigned GetFirstChild(unsigned node) const;
		unsigned GetSibling(unsigned node) const;

		bool IsChildOf(unsigned parentNodeIndex, unsigned childNodeIndex) const;
		bool HasChild(unsigned nodeIndex) const;

		unsigned GetCountChildren(unsigned nodeIndex) const;
		void GetChildren(unsigned nodeIndex, Core::IndexVectorU& children) const;
		
		void AddSubtree(unsigned nodeIndex, Core::IndexVectorU& nodeIndices) const;
		void GetSubtree(unsigned nodeIndex, Core::IndexVectorU& nodeIndices) const;
		
		// Returns the root node of a set of nodes, i.e. the node without parent,
		// which is an ancestor to all nodes.
		unsigned GetRootNode(const Core::IndexVectorU& nodeIndices) const;
		
		// Returns the common ancestor of a set of nodes, i.e. the node,
		// which is an ancestor to all nodes.
		unsigned GetCommonAncestor(const Core::IndexVectorU& nodeIndices) const;

	public: // Direct access to the underlying containers.

		const Core::SimpleTypeUnorderedVectorU<SceneNodeMainData>& GetMainDataVector() const;
		Core::SimpleTypeUnorderedVectorU<SceneNodeMainData>& GetMainDataVector();

		const SceneNodeMainData* GetMainData() const;
		SceneNodeMainData* GetMainData();

		const ScaledTransformation* GetScaledWorldTransformations() const;

	public:

		bool IsStatic(unsigned nodeIndex) const;
		bool IsDynamic(unsigned nodeIndex) const;

		bool IsTransformationDirtyForUser(unsigned nodeIndex) const;
		void SetTransformationUpToDateForUser(unsigned nodeIndex);

		void SetSceneNodeDirty(unsigned nodeIndex);

	private: // Lazy-evaluated wrapped scene node interface.

		void HandleLocationPropertyChange(unsigned node);
		void UpdateScaledWorldTransformations(unsigned nodeIndex);

	public:

		void SetLocalOrientationAndScalingWithoutPropChangeHandling(unsigned nodeIndex, const glm::mat3x3& m);

		const glm::vec3& GetLocalPosition(unsigned sceneNodeIndex) const;
		void SetLocalPosition(unsigned sceneNodeIndex, const glm::vec3& localPosition);
		void SetLocalPositionWithoutPropChangeHandling(unsigned sceneNodeIndex, const glm::vec3& localPosition);

		const glm::mat3& GetLocalOrientation(unsigned sceneNodeIndex) const;
		void SetLocalOrientation(unsigned sceneNodeIndex, const glm::mat3& localOrientation);
		void SetLocalOrientationWithoutPropChangeHandling(unsigned sceneNodeIndex, const glm::mat3& localOrientation);

		const RigidTransformation& GetLocalTransformation(unsigned nodeIndex) const;
		void SetLocalTransformation(unsigned nodeIndex, const RigidTransformation& transformation);
		void SetLocalTransformation(unsigned nodeIndex, const ScaledTransformation& transformation);
		void SetLocalTransformationWithoutPropChangeHandling(unsigned nodeIndex, const RigidTransformation& transformation);
		void SetLocalTransformationWithoutPropChangeHandling(unsigned nodeIndex, const ScaledTransformation& transformation);

		const glm::vec3& GetLocalScaler(unsigned nodeIndex) const;
		void SetLocalScaler(unsigned nodeIndex, const glm::vec3& scaler);
		void SetLocalScalerWithoutPropChangeHandling(unsigned nodeIndex, const glm::vec3& scaler);

		ScaledTransformation GetWorldTransformation(unsigned sceneNodeIndex);
		glm::mat3 GetWorldOrientation(unsigned nodeIndex);
		
		ScaledTransformation GetInverseWorldTransformation(unsigned sceneNodeIndex);

		const ScaledTransformation& GetScaledWorldTransformation(unsigned nodeIndex);
		const ScaledTransformation& GetInverseScaledWorldTransformation(unsigned nodeIndex);

		const ScaledTransformation& UnsafeGetScaledWorldTransformation(unsigned nodeIndex) const;
		const ScaledTransformation& UnsafeGetInverseScaledWorldTransformation(unsigned nodeIndex) const;

		const glm::vec3& GetPosition(unsigned sceneNodeIndex);
		const glm::vec3& UnsafeGetPosition(unsigned sceneNodeIndex) const;
		void SetPosition(unsigned sceneNodeIndex, const glm::vec3& position);

		glm::vec3 GetDirection(unsigned sceneNodeIndex);
		glm::vec3 GetUp(unsigned sceneNodeIndex);
		glm::vec3 GetRight(unsigned sceneNodeIndex);
		glm::vec3 UnsafeGetDirection(unsigned sceneNodeIndex) const;
		glm::vec3 UnsafeGetUp(unsigned sceneNodeIndex) const;
		glm::vec3 UnsafeGetRight(unsigned sceneNodeIndex) const;

		glm::mat3 GetOrientation(unsigned sceneNodeIndex);
		glm::quat GetOrientationQuaternion(unsigned sceneNodeIndex);
		glm::mat3 UnsafeGetOrientation(unsigned sceneNodeIndex) const;
		glm::quat UnsafeGetOrientationQuaternion(unsigned sceneNodeIndex) const;

		void SetOrientation(unsigned nodeIndex, const glm::mat3& orientation);
		void SetOrientation(unsigned nodeIndex, const glm::quat& orientation);
		void SetDirectionUp(unsigned nodeIndex, const glm::vec3& direction,
			const glm::vec3& up);
		void SetDirection(unsigned nodeIndex, const glm::vec3& direction);

		void SetLocation(unsigned nodeIndex, const ScaledTransformation& transformation);
		void SetLocation(unsigned nodeIndex, const glm::vec3& position, const glm::vec3& direction,
			const glm::vec3& up);
		void SetLocation(unsigned nodeIndex, const glm::vec3& position, const glm::vec3& direction);

		glm::vec3 GetLookAt(unsigned nodeIndex);
		void LookAt(unsigned nodeIndex, const glm::vec3& lookAtPosition);

	private: // Location related functions.

		void UpdateTransformationsInThreadWithoutParent(unsigned threadId,
			unsigned startIndex, unsigned endIndex);
		void UpdateTransformationsInThreadWithParent(unsigned threadId,
			unsigned startIndex, unsigned endIndex);

	private: // Scene node updating.

		// TODO: review this design. O(n^2) memory consumption!

		void GatherDirtyIndices(unsigned char updateLevel);
		void GatherDirtyIndices(unsigned char updateLevel, const Core::ByteVectorU& allowedMask);
		void UpdateAllowedMask(const Core::IndexVectorU& allowedIndices);
		void UpdateTransformationsInParallel(unsigned char updateLevel);

		Core::ByteVectorU m_TempAllowedMask;

#ifdef _DEBUG
		bool m_CheckForSubsetUpdateSafety = true;
#endif

	public:

		void UpdateTransformations();
		void UpdateTransformations(const Core::ByteVectorU& allowedMask);
		void UpdateTransformations(const Core::IndexVectorU& allowedIndices);

#ifdef _DEBUG
		void SetIsCheckingForSubsetUpdateSafety(bool isChecking);
#endif

	public:

		// Compacts the scene node vectors and returns the scene node index mapping:
		// (old index, new index). Note that the wrapped scene nodes get automatically
		// updated.
		std::map<unsigned, unsigned> CompactSceneNodes(bool isShrinkingUnderlyingVectors = true);
	};

	// Class wraps the scene node handle and makes the scene node handle ownership to be unique.
	// Note that this class is not intended for effective scene node handling, thus only use it
	// for special purposes.
	class WrappedSceneNode
	{
		SceneNodeHandler* m_Handler;
		unsigned m_SceneNodeIndex;
		unsigned m_WrappedSceneNodeIndex;

		void DeleteSceneNode();

	public: // Internal functions.

		void _UpdateSceneNodeIndex(std::map<unsigned, unsigned>& indexMapping);

	public:

		WrappedSceneNode();
		WrappedSceneNode(SceneNodeHandler* handler, bool isStatic, unsigned char updateLevelHint = 0);
		WrappedSceneNode(SceneNodeHandler* handler, unsigned sceneNodeIndex);
		WrappedSceneNode(WrappedSceneNode&) = delete;
		WrappedSceneNode(WrappedSceneNode&& other);
		~WrappedSceneNode();

		WrappedSceneNode& operator=(const WrappedSceneNode& other) = delete;
		WrappedSceneNode& operator=(WrappedSceneNode&& other);

		bool IsValid() const;

		void Set(SceneNodeHandler* handler, unsigned sceneNodeIndex);

		SceneNodeHandler* GetSceneNodeHandler();

	public:

		bool IsStatic() const;
		bool IsDynamic() const;

		bool IsTransformationDirtyForUser() const;
		void SetTransformationUpToDateForUser();

	public: // Relationship related functions.

		void AddChild(unsigned childIndex);
		void AddChild(WrappedSceneNode& child);
		void RemoveChild(unsigned childIndex);
		void RemoveChild(WrappedSceneNode& child);

		bool IsChild(unsigned childIndex) const;
		bool IsChild(WrappedSceneNode& child) const;

		unsigned GetParentSceneNodeIndex() const;

		unsigned GetCountChildren() const;
		void GetChildrenIndices(Core::IndexVectorU& children) const;
		Core::IndexVectorU GetChildrenIndices() const;

	public: // Location related functions.

		const RigidTransformation&  GetLocalTransformation() const;
		void SetLocalTransformation(const RigidTransformation& transformation);
		void SetLocalTransformation(const ScaledTransformation& transformation);

		const glm::vec3& GetLocalPosition() const;
		void SetLocalPosition(const glm::vec3& localPosition);

		const glm::mat3& GetLocalOrientation() const;
		void SetLocalOrientation(const glm::mat3& localOrientation);

		const glm::vec3& GetScaler() const;
		void SetScaler(const glm::vec3& scaler);

		ScaledTransformation GetWorldTransformation();
		glm::mat3 GetWorldOrientation();

		ScaledTransformation GetInverseWorldTransformation();

		const ScaledTransformation& GetScaledWorldTransformation();
		const ScaledTransformation& GetInverseScaledWorldTransformation();

		const glm::vec3& GetPosition();
		void SetPosition(const glm::vec3& position);

		glm::vec3 GetDirection();
		glm::vec3 GetUp();
		glm::vec3 GetRight();

		glm::mat3 GetOrientation() const;
		glm::quat GetOrientationQuaternion() const;

		void SetOrientation(const glm::mat3& orientation);
		void SetOrientation(const glm::quat& orientation);
		void SetDirectionUp(const glm::vec3& direction, const glm::vec3& up);
		void SetDirection(const glm::vec3& direction);

		void SetLocation(const ScaledTransformation& transformation);
		void SetLocation(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& up);
		void SetLocation(const glm::vec3& position, const glm::vec3& direction);

		glm::vec3 GetLookAt();
		void LookAt(const glm::vec3& lookAtPosition);
	};
}

#endif
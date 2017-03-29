// EngineBuildingBlocks/Graphics/ViewFrustumCuller.h

#ifndef _ENGINEBUILDINGBLOCKS_VIEWFRUSTUMCULLER_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_VIEWFRUSTUMCULLER_H_INCLUDED_

#include <Core/System/ThreadPool.h>
#include <EngineBuildingBlocks/SceneNode.h>
#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>
#include <EngineBuildingBlocks/Math/AABoundingBox.h>

// Disabling 'too long function name' warning.
#pragma warning ( disable: 4503 )

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		inline bool CullConvexPolyhedron(const Math::Plane* frustumPlanes, const glm::vec3* corners)
		{
			for (unsigned i = 0; i < Math::c_CountFrustumPlanes; i++)
			{
				auto& plane = frustumPlanes[i];
				auto& planeNormal = plane.Normal;

				float min = glm::dot(planeNormal, corners[0]);
				for (unsigned j = 1; j < Math::c_CountBoxCorners; j++)
				{
					float value = glm::dot(planeNormal, corners[j]);
					if (value < min)
					{
						min = value;
					}
				}

				if (min + plane.D > 0.0f)
				{
					return false;
				}
			}
			return true;
		}

		class ViewFrustumCuller
		{
			Core::SimpleTypeVectorU<unsigned> m_CountOutputTasks;

		public:

			// This function view frustum culls the tasks and outputs the
			// visible task indices. It assumes, that the scene nodes' scaled
			// model transformation is up-to-date. 
			// The results are conservative: the function only test frustum planes agains bounding box corners.
			template <typename TaskType>
			void ViewFrustumCull(Camera& camera,
				Core::ThreadPool& threadPool,
				const SceneNodeHandler& sceneNodeHandler, const TaskType* taskData,
				const unsigned* inputTaskIndices, Core::IndexVectorU& outputTaskIndices,
				unsigned countTasks)
			{
				outputTaskIndices.Resize(countTasks);

				m_CountOutputTasks.Resize(threadPool.GetCountThreads());
				m_CountOutputTasks.SetByte(0);

				unsigned countThreads = threadPool.ExecuteWithStaticScheduling(countTasks,
					&ViewFrustumCuller::ViewFrustumCullInThread<TaskType>, this,
					camera.GetViewFrustum().GetPlanes().Planes,
					&sceneNodeHandler, taskData, inputTaskIndices, outputTaskIndices.GetArray(),
					m_CountOutputTasks.GetArray());

				// Single-threaded compaction.
				unsigned startIndex, endIndex;
				Core::ThreadingHelper::GetTaskIndices(countTasks, countThreads, 0, startIndex, endIndex);
				unsigned sourceIndex = endIndex - startIndex;
				unsigned targetIndex = m_CountOutputTasks[0];
				for (unsigned i = 1; i < countThreads; i++)
				{
					unsigned countOutputTasks = m_CountOutputTasks[i];
					memcpy(&outputTaskIndices[targetIndex], &outputTaskIndices[sourceIndex],
						countOutputTasks * sizeof(unsigned));
					Core::ThreadingHelper::GetTaskIndices(countTasks, countThreads, i, startIndex, endIndex);
					sourceIndex += endIndex - startIndex;
					targetIndex += countOutputTasks;
				}
				outputTaskIndices.UnsafeResize(targetIndex);
			}

		private:

			template <typename TaskType>
			void ViewFrustumCullInThread(unsigned threadId, unsigned startIndex, unsigned endIndex,
				const EngineBuildingBlocks::Math::Plane* frustumPlanes,
				const EngineBuildingBlocks::SceneNodeHandler* pSceneNodeHandler,
				const TaskType* taskData, const unsigned* inputTaskIndices,
				unsigned* outputTaskIndices, unsigned* outputTaskCountArray)
			{
				auto transformations = pSceneNodeHandler->GetScaledWorldTransformations();

				Math::BoxCorners boxCorners;
				auto corners = boxCorners.Corners;

				unsigned targetIndex = startIndex;

				for (unsigned i = startIndex; i < endIndex; i++)
				{
					unsigned taskIndex = inputTaskIndices[i];
					auto& task = taskData[taskIndex];
					auto& transformation = transformations[task.SceneNodeIndex];
					auto& aabb = task.BoundingBox;

					aabb.GetBoxCorners(boxCorners);

					for (unsigned i = 0; i < Math::c_CountBoxCorners; i++)
					{
						corners[i] = transformation.A * corners[i] + transformation.Position;
					}

					if (CullConvexPolyhedron(frustumPlanes, corners))
					{
						outputTaskIndices[targetIndex++] = taskIndex;
					}
				}

				outputTaskCountArray[threadId] = targetIndex - startIndex;
			}

		public:

			// The results are conservative: we only test frustum1 planes agains frustum2 corners and vica versa.
			static bool HasIntersection(Camera& camera1, Camera& camera2)
			{
				auto& vf1 = camera1.GetViewFrustum();
				auto& vf2 = camera2.GetViewFrustum();
				auto planes1 = vf1.GetPlanes().Planes;
				auto planes2 = vf2.GetPlanes().Planes;
				auto corners1 = vf1.GetCorners().Corners;
				auto corners2 = vf2.GetCorners().Corners;
				return (CullConvexPolyhedron(planes1, corners2) && CullConvexPolyhedron(planes2, corners1));
			}
		};
	}
}

#endif
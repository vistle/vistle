// EngineBuildingBlocks/Graphics/Camera/CubemapHelper.hpp

#ifndef _ENGINEBUILDINGBLOCKS_CUBEMAPHELPER_HPP_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_CUBEMAPHELPER_HPP_INCLUDED_

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>

const unsigned c_CountCubemapSides = 6;

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		// Indexed by the cubemap side convention (OpenGL, DirectX) in view space.
		enum class CubemapSide
		{
			Right,	// Positive X
			Left,	// Negative X
			Top,	// Positive Y
			Bottom,	// Negative Y
			Back,	// Positive Z
			Front	// Negative Z
		};

		struct CubemapCameraGroup
		{
			Camera* Cameras[c_CountCubemapSides];
			
			CubemapCameraGroup(Camera* frontCamera)
			{
				Cameras[(int)CubemapSide::Front] = frontCamera;

				auto sceneNodeHandler = frontCamera->GetSceneNodeHandler();

				for (unsigned i = 0; i < c_CountCubemapSides - 1; i++)
				{
					Cameras[i] = new Camera(sceneNodeHandler);
				}
				UpdateLocation();
				UpdateProjection();
			}

			~CubemapCameraGroup()
			{
				for (unsigned i = 0; i < c_CountCubemapSides - 1; i++)
				{
					delete Cameras[i];
				}
			}

			void UpdateProjection()
			{
				auto frontCamera = Cameras[(int)CubemapSide::Front];
				auto& projection = frontCamera->GetProjection();

				for (unsigned i = 0; i < c_CountCubemapSides - 1; i++)
				{
					Cameras[i]->SetProjection(projection);
				}
			}

			void UpdateLocation()
			{
				auto frontCamera = Cameras[(int)CubemapSide::Front];
				for (unsigned i = 0; i < c_CountCubemapSides - 1; i++)
				{
					glm::mat4 rotation(glm::uninitialize);
					switch (static_cast<CubemapSide>(i))
					{
					case CubemapSide::Right:
						rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); break;
					case CubemapSide::Left:
						rotation = glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); break;
					case CubemapSide::Top:
						rotation = glm::rotate(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f))
							* glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); break;
					case CubemapSide::Bottom:
						rotation = glm::rotate(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f))
							* glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); break;
					case CubemapSide::Back:
						rotation = glm::rotate(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); break;
					default: break;
					}
					Cameras[i]->SetLocation(frontCamera->GetWorldTransformation() * rotation);
				}
			}

			void Update()
			{
				UpdateProjection();
				UpdateLocation();
			}
		};
	}
}

#endif
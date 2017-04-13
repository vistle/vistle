// CubemapStreaming_Client/GridReprojector.h

#ifndef _CUBEMAPSTREAMING_CLIENT_GRIDREPROJECTOR_H_
#define _CUBEMAPSTREAMING_CLIENT_GRIDREPROJECTOR_H_

#include <Core/DataStructures/Properties.h>
#include <Core/System/ThreadPool.h>
#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>
#include <EngineBuildingBlocks/Graphics/Camera/CubemapHelper.hpp>
#include <EngineBuildingBlocks/Graphics/ViewFrustumCuller.h>
#include <OpenGLHelper.h>

#include <vector>

struct TileInstanceData
{
	glm::vec2 TexCoordOffset;
	unsigned RegionIndex;
};

class GridReprojector
{
	Core::ThreadPool m_ThreadPool;
	EngineBuildingBlocks::SceneNodeHandler* m_PSceneNodeHandler;

	OpenGLRender::VertexArrayObject m_GridVAO;
	OpenGLRender::VertexBuffer m_GridVBO;
	OpenGLRender::VertexBuffer m_GridTileInstanceVBO;
	OpenGLRender::IndexBuffer m_GridIndexBuffer;
	unsigned m_CountGridIndices = 0;

	unsigned m_GridSize;
	bool m_IsContainingAlpha;
	float m_SideVisiblePortion, m_PosZVisiblePortion;

	EngineBuildingBlocks::Graphics::Camera m_BackgroundCamera;
	std::vector<EngineBuildingBlocks::Graphics::Camera> m_TileCameras;
	Core::SimpleTypeVectorU<bool> m_TileIsBackground;
	Core::SimpleTypeVectorU<glm::vec2> m_TileOffsets;

	bool m_IsCullingEnabled;
	bool m_IsClearingBuffers;

	Primitive m_GridEdge;

	ShaderRebuilder m_ShaderRebuilder;

	EngineBuildingBlocks::Graphics::ViewFrustumCuller m_ViewFrustumCuller;
	Core::SimpleTypeVectorU<TileInstanceData> m_TileInstanceData;

	struct SideInstanceDrawData
	{
		unsigned Offset;
		unsigned Count;
	};
	SideInstanceDrawData m_SideInstanceData[c_CountCubemapSides];

	void SetGridRasterizationShader(
		EngineBuildingBlocks::Graphics::CubemapCameraGroup& serverCameraGroup,
		EngineBuildingBlocks::Graphics::Camera& clientCamera);
	void SetGridEdgeRasterizationShader(
		EngineBuildingBlocks::Graphics::CubemapCameraGroup& serverCameraGroup,
		EngineBuildingBlocks::Graphics::Camera& clientCamera);

	void CreateTileData();
	void UpdateTileData(
		EngineBuildingBlocks::Graphics::CubemapCameraGroup& serverCameraGroup,
		EngineBuildingBlocks::Graphics::Camera& clientCamera);

	glm::vec2 GetTileOffset(unsigned j, unsigned k,
		const glm::vec2& texelSize) const;
	void SetTileCameraProjection(
		EngineBuildingBlocks::Graphics::Camera& tileCamera,
		unsigned linearTileIndex,
		const glm::vec2& texelSize, const glm::vec2& tileSize,
		float tileZMin, float tileZMax,
		bool isProjectingTo_0_1_Interval);

	void SetTileDepthMinMax(unsigned id, unsigned start, unsigned end,
		unsigned char** depthDataPointers,
		EngineBuildingBlocks::Graphics::CubemapCameraGroup* pServerCameraGroup);

	void CreatePrimitives();

private: // Configuration.

	bool m_IsMultisamplingEnabled = false;
	unsigned m_CountGridTilesX = 1;
	unsigned m_CountGridTilesY = 1;
	float m_BackgroundFarPlaneMultiplier = 1.0f;

public:

	bool IsMultisamplingEnabled() const;

private: // Debugging.

	float m_TileRatio = 1;

public:

	float GetTileRatio() const;

public:

	GridReprojector(
		EngineBuildingBlocks::SceneNodeHandler& sceneNodeHandler,
		std::atomic<bool>* pIShuttingDown);
	~GridReprojector();

	ShaderRebuilder& GetShaderRebuilder();

	void SetCullingEnabled(bool enabled);
	void SetClearingBuffers(bool isClearing);

	void LoadConfiguration(const Core::Properties& configuration);
	void Initialize(const std::string& shadersPath);

	void SetTextureData(
		unsigned char** depthDataPointers,
		unsigned textureWidth, unsigned textureHeight,
		bool isContainingAlpha,
		float sideVisiblePortion, float posZVisiblePortion,
		EngineBuildingBlocks::Graphics::CubemapCameraGroup& serverCameraGroup);

	void Render(
		EngineBuildingBlocks::Graphics::CubemapCameraGroup& serverCubemapCameraGroup,
		EngineBuildingBlocks::Graphics::Camera& clientCamera,
		bool useWireframe = false);

	void Update();
};

#endif
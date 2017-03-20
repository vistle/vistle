// CubemapStreaming_Client/GridReprojector.cpp

// Must be included before OpenGL includes.
#include <External/glew-1.12.0/include/GL/glew.h>

#include <GridReprojector.h>

#include <GridGeometry.h>
#include <Message.h>
#include <Settings.h>

using namespace EngineBuildingBlocks::Graphics;
using namespace OpenGLRender;
using namespace CubemapStreaming;

GridReprojector::GridReprojector(EngineBuildingBlocks::SceneNodeHandler& sceneNodeHandler,
	std::atomic<bool>* pIShuttingDown)
	: m_PSceneNodeHandler(&sceneNodeHandler)
	, m_GridSize(0)
	, m_IsContainingAlpha(false)
	, m_SideVisiblePortion(0.0f)
	, m_PosZVisiblePortion(0.0f)
	, m_BackgroundCamera(&sceneNodeHandler)
	, m_IsCullingEnabled(true)
	, m_IsClearingBuffers(true)
	, m_ShaderRebuilder(pIShuttingDown)
{
}

GridReprojector::~GridReprojector()
{
	for (auto& it : m_ShaderRebuilder.ShaderPrograms)
	{
		DeleteShader(it.second);
	}
}

ShaderRebuilder& GridReprojector::GetShaderRebuilder()
{
	return m_ShaderRebuilder;
}

void GridReprojector::SetCullingEnabled(bool enabled)
{
	this->m_IsCullingEnabled = enabled;
}

void GridReprojector::SetClearingBuffers(bool isClearing)
{
	this->m_IsClearingBuffers = isClearing;
}

bool GridReprojector::IsMultisamplingEnabled() const
{
	return m_IsMultisamplingEnabled;
}

template <typename T>
inline void InitializeGRProperty(const Core::Properties& configuration, const char* name, T& property)
{
	configuration.TryGetPropertyValue((std::string("Configuration.GridReprojection.") + name).c_str(), property);
}

void GridReprojector::LoadConfiguration(const Core::Properties& configuration)
{
	InitializeGRProperty(configuration, "IsMultisamplingEnabled", m_IsMultisamplingEnabled);
	InitializeGRProperty(configuration, "CountGridTilesX", m_CountGridTilesX);
	InitializeGRProperty(configuration, "CountGridTilesY", m_CountGridTilesY);
	InitializeGRProperty(configuration, "BackgroundFarPlaneMultiplier", m_BackgroundFarPlaneMultiplier);
}

void GridReprojector::Initialize(const std::string& shadersPath)
{
	auto& shaderPrograms = m_ShaderRebuilder.ShaderPrograms;

	// Initializing shader programs.
	{
		auto gridRasterVSPath = shadersPath + "CubemapReprojectionGrid_vs.glsl";
		auto gridRasterPSPath = shadersPath + "CubemapReprojectionGrid_ps.glsl";

		auto gridEdgeRasterVSPath = shadersPath + "CubemapReprojectionGridEdge_vs.glsl";
		auto gridEdgeRasterPSPath = shadersPath + "CubemapReprojectionGridEdge_ps.glsl";

		// Grid rasterization shader.
		{
			ShaderProgramData spData;
			InitializeShader(spData);
			spData.DefinesAddingFunction = &AddEmptyShaderDefines;
			spData.Shaders.push_back({ ShaderDescription::FromFile(gridRasterVSPath, ShaderType::Vertex) });
			spData.Shaders.push_back({ ShaderDescription::FromFile(gridRasterPSPath, ShaderType::Fragment) });
			shaderPrograms[(unsigned)ShaderId::GridRasterization] = std::move(spData);
		}

		// Grid edge rasterization shader.
		{
			ShaderProgramData spData;
			InitializeShader(spData);
			spData.DefinesAddingFunction = &AddEmptyShaderDefines;
			spData.Shaders.push_back({ ShaderDescription::FromFile(gridEdgeRasterVSPath, ShaderType::Vertex) });
			spData.Shaders.push_back({ ShaderDescription::FromFile(gridEdgeRasterPSPath, ShaderType::Fragment) });
			shaderPrograms[(unsigned)ShaderId::GridEdgeRasterization] = std::move(spData);
		}
	}

	for (auto& it : shaderPrograms)
	{
		BuildShader(it.second);
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	if (m_IsMultisamplingEnabled) glEnable(GL_MULTISAMPLE);
}

void GridReprojector::CreatePrimitives()
{
	auto& shaderPrograms = m_ShaderRebuilder.ShaderPrograms;

	// Deleting primitives.
	{
		m_GridVAO.Delete();
		m_GridVBO.Delete();
		m_GridTileInstanceVBO.Delete();
		m_GridIndexBuffer.Delete();
		m_GridEdge.Delete();
	}

	// Setting grid vertex and index buffers.
	{
		Vertex_SOA_Data vertexData;
		IndexData indexData;
		CreatePartialReprojectionGrid_TexCoord(m_GridSize, m_CountGridTilesX, m_CountGridTilesY, vertexData, indexData);
		m_CountGridIndices = indexData.GetCountIndices();
		unsigned countTiles = c_CountCubemapSides * m_CountGridTilesX * m_CountGridTilesY;

		auto program = shaderPrograms[(unsigned)ShaderId::GridRasterization].Program;

		m_GridVAO.Initialize();
		m_GridVAO.Bind();

		m_GridVBO.Initialize(OpenGLRender::BufferUsage::StaticDraw, vertexData);
		program->SetInputLayout(vertexData.InputLayout);

		VertexInputLayout instaceBufferIL;
		instaceBufferIL.Elements.push_back({ "TexCoordOffset", VertexElementType::Float, (unsigned)sizeof(float), 2U });
		instaceBufferIL.Elements.push_back({ "RegionIndexIn", VertexElementType::Uint32, (unsigned)sizeof(unsigned), 1U });
		m_GridTileInstanceVBO.Initialize(OpenGLRender::BufferUsage::DynamicDraw, countTiles, instaceBufferIL);
		program->SetInputLayout(instaceBufferIL, 1);

		m_GridIndexBuffer.Initialize(BufferUsage::StaticDraw, indexData);
	}

	// Setting grid edge vertex and index data.
	{
		Vertex_SOA_Data vertexData;
		IndexData indexData;
		CreateReproectionGridEdge_TexCoord_FaceIndex(m_GridSize, vertexData, indexData);
		m_GridEdge.Initialize(OpenGLRender::BufferUsage::StaticDraw, vertexData, indexData);

		auto program = shaderPrograms[(unsigned)ShaderId::GridEdgeRasterization].Program;
		program->SetInputLayout(vertexData.InputLayout);
	}
}

void GridReprojector::SetTextureData(unsigned char** depthDataPointers,
	unsigned textureWidth, unsigned textureHeight,
	bool isContainingAlpha, float sideVisiblePortion, float posZVisiblePortion,
	CubemapCameraGroup& serverCameraGroup)
{
	assert(textureWidth == textureHeight);
	if (m_GridSize != textureWidth)
	{
		m_GridSize = textureWidth;
		CreatePrimitives();
		CreateTileData();
	}
	this->m_IsContainingAlpha = isContainingAlpha;
	this->m_SideVisiblePortion = sideVisiblePortion;
	this->m_PosZVisiblePortion = posZVisiblePortion;
	m_ThreadPool.ExecuteWithStaticScheduling(c_CountCubemapSides * m_CountGridTilesX * m_CountGridTilesY,
		&GridReprojector::SetTileDepthMinMax, this, depthDataPointers, &serverCameraGroup);
}

void GridReprojector::SetGridRasterizationShader(CubemapCameraGroup& serverCameraGroup, Camera& clientCamera)
{
	auto cViewProj = clientCamera.GetViewProjectionMatrix();

	glm::mat4 transformations[c_CountCubemapSides];
	for (unsigned i = 0; i < c_CountCubemapSides; i++)
	{
		auto sInvViewProj = serverCameraGroup.Cameras[i]->GetViewProjectionMatrixInverse();
		transformations[i] = cViewProj * sInvViewProj;
	}

	auto pProgram = m_ShaderRebuilder.ShaderPrograms[(unsigned)ShaderId::GridRasterization].Program;
	pProgram->Bind();

	// Setting tile instance buffer.
	m_GridTileInstanceVBO.Write(m_TileInstanceData.GetArray(), 0U,
		m_TileInstanceData.GetSize() * (unsigned)sizeof(TileInstanceData));

	pProgram->SetUniformValueArray("Transformations", transformations, c_CountCubemapSides);

	pProgram->SetUniformValue("ColorTexture", 0);
	pProgram->SetUniformValue("DepthTexture", 1);
}

void GridReprojector::SetGridEdgeRasterizationShader(CubemapCameraGroup& serverCameraGroup, Camera& clientCamera)
{
	auto cViewProj = clientCamera.GetViewProjectionMatrix();

	glm::mat4 transformations[c_CountCubemapSides];
	for (unsigned i = 0; i < c_CountCubemapSides; i++)
	{
		auto sInvViewProj = serverCameraGroup.Cameras[i]->GetViewProjectionMatrixInverse();
		transformations[i] = cViewProj * sInvViewProj;
	}

	auto pProgram = m_ShaderRebuilder.ShaderPrograms[(unsigned)ShaderId::GridEdgeRasterization].Program;
	pProgram->Bind();
	pProgram->SetUniformValueArray("Transformations", transformations, c_CountCubemapSides);

	pProgram->SetUniformValue("ColorTexture", 0);
	pProgram->SetUniformValue("DepthTexture", 1);
}

void GridReprojector::Render(CubemapCameraGroup& serverCubemapCameraGroup, Camera& clientCamera, bool useWireframe)
{
	if (m_GridSize == 0) return;

	auto& shaderPrograms = m_ShaderRebuilder.ShaderPrograms;

	UpdateTileData(serverCubemapCameraGroup, clientCamera);

	if (m_IsClearingBuffers)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	glPolygonMode(GL_FRONT_AND_BACK, useWireframe ? GL_LINE : GL_FILL);

	// Drawing grid.
	{
		m_GridVAO.Bind();
		SetGridRasterizationShader(serverCubemapCameraGroup, clientCamera);
		glDrawElementsInstanced(GL_TRIANGLE_STRIP, m_CountGridIndices, GL_UNSIGNED_INT, 0,
			m_TileInstanceData.GetSize());
	}

	// Drawing edge.
	{
		m_GridEdge.Bind();
		SetGridEdgeRasterizationShader(serverCubemapCameraGroup, clientCamera);
		glDrawElements(GL_TRIANGLE_STRIP, m_GridEdge.CountIndices, GL_UNSIGNED_INT, 0);
	}
}

void GridReprojector::Update()
{
	m_ShaderRebuilder.ApplyNewShader();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

float GridReprojector::GetTileRatio() const
{
	return m_TileRatio;
}

glm::vec2 GridReprojector::GetTileOffset(unsigned tileIndexX, unsigned tileIndexY, const glm::vec2& texelSize) const
{
	return glm::vec2(
		(float)tileIndexX / (float)m_CountGridTilesX - (tileIndexX == m_CountGridTilesX - 1 ? texelSize.x : 0.0f),
		(float)tileIndexY / (float)m_CountGridTilesY - (tileIndexY == m_CountGridTilesY - 1 ? texelSize.y : 0.0f));
}

void GridReprojector::SetTileCameraProjection(Camera& tileCamera, unsigned linearTileIndex,
	const glm::vec2& texelSize, const glm::vec2& tileSize, float tileZMin, float tileZMax,
	bool isProjectingTo_0_1_Interval)
{
	auto& uvOffset = m_TileOffsets[linearTileIndex];
	auto uv0 = texelSize * 0.5f + uvOffset;
	auto uv1 = uv0 + tileSize;
	auto leftBottom = (uv0 * 2.0f - 1.0f) * tileZMin;
	auto rightTop = (uv1 * 2.0f - 1.0f) * tileZMin;
	tileCamera.SetPerspectiveProjection(leftBottom.x, rightTop.x, leftBottom.y, rightTop.y, tileZMin, tileZMax,
		isProjectingTo_0_1_Interval);
}

void GridReprojector::CreateTileData()
{
	m_TileIsBackground.Clear();
	m_TileOffsets.Clear();
	m_TileCameras.clear();

	auto texelSize = glm::vec2(1.0f / (float)m_GridSize);

	for (unsigned i = 0; i < c_CountCubemapSides; i++)
	{
		for (unsigned j = 0; j < m_CountGridTilesY; j++)
		{
			for (unsigned k = 0; k < m_CountGridTilesX; k++)
			{
				m_TileIsBackground.PushBack(false);
				m_TileOffsets.PushBack(GetTileOffset(k, j, texelSize));
				m_TileCameras.emplace_back(m_PSceneNodeHandler);
			}
		}
	}
}

void GridReprojector::UpdateTileData(CubemapCameraGroup& serverCameraGroup, Camera& clientCamera)
{
	auto projection = clientCamera.GetProjection();
	projection.Projection.Perspective.FarPlaneDistance *= m_BackgroundFarPlaneMultiplier;
	m_BackgroundCamera.SetProjection(projection);
	m_BackgroundCamera.SetLocation(clientCamera);

	unsigned countTiles = c_CountCubemapSides * m_CountGridTilesX * m_CountGridTilesY;
	m_TileInstanceData.ClearAndReserve(countTiles);

	unsigned index = 0;
	for (unsigned i = 0; i < c_CountCubemapSides; i++)
	{
		for (unsigned j = 0; j < m_CountGridTilesY; j++)
		{
			for (unsigned k = 0; k < m_CountGridTilesX; k++, index++)
			{
				auto& testCamera = (m_TileIsBackground[index] ? m_BackgroundCamera : clientCamera);
				if (!m_IsCullingEnabled
					|| m_ViewFrustumCuller.HasIntersection(m_TileCameras[index], testCamera))
				{
					auto& data = m_TileInstanceData.UnsafePushBackPlaceHolder();
					data.RegionIndex = i;
					data.TexCoordOffset = m_TileOffsets[index];
				}
			}
		}
	}

	m_TileRatio = (float)m_TileInstanceData.GetSize() / (float)countTiles;
}

void GridReprojector::SetTileDepthMinMax(unsigned id, unsigned start, unsigned end,
	unsigned char** depthDataPointers, CubemapCameraGroup* pServerCameraGroup)
{
	auto texelSize = glm::vec2(1.0f / (float)m_GridSize);
	auto tileSize = glm::vec2(1.0f / (float)m_CountGridTilesX, 1.0f / (float)m_CountGridTilesY);

	glm::uvec2 sideStarts[c_CountCubemapSides], sideEnds[c_CountCubemapSides];
	GetImageDataSideBounds(m_GridSize, m_GridSize, m_SideVisiblePortion, m_PosZVisiblePortion,
		sideStarts, sideEnds);

	unsigned countTilesPerSide = m_CountGridTilesX * m_CountGridTilesY;
	unsigned xSize = m_GridSize / m_CountGridTilesX;
	unsigned ySize = m_GridSize / m_CountGridTilesY;
	for (unsigned i = start; i < end; i++)
	{
		unsigned side = i / countTilesPerSide;
		unsigned sideIndex = i - side * countTilesPerSide;
		unsigned tileY = sideIndex / m_CountGridTilesX;
		unsigned tileX = sideIndex - tileY * m_CountGridTilesX;
		int xStart = xSize * tileX - (tileX == m_CountGridTilesX - 1 ? 1 : 0);
		int yStart = ySize * tileY - (tileY == m_CountGridTilesY - 1 ? 1 : 0);
		int xEnd = xStart + xSize;
		int yEnd = yStart + ySize;
		auto depths = reinterpret_cast<const float*>(depthDataPointers[side]);

		float minDepth = 1.0f;
		float maxDepth = -1.0f; // Fits for both OpenGL and DirectX.

		int width = sideEnds[side].x - sideStarts[side].x;
		int height = sideEnds[side].y - sideStarts[side].y;

		// Setting limits to received data limits.
		xStart = std::max(xStart - (int)sideStarts[side].x, 0);
		yStart = std::max(yStart - (int)sideStarts[side].y, 0);
		xEnd = std::min(xEnd - (int)sideStarts[side].x, width);
		yEnd = std::min(yEnd - (int)sideStarts[side].y, height);

		for (int j = yStart; j <= yEnd; j++)
		{
			for (int k = xStart; k <= xEnd; k++)
			{
				float depth = depths[j * width + k];
				minDepth = std::min(minDepth, depth);
				maxDepth = std::max(maxDepth, depth);
			}
		}

		if (minDepth > maxDepth) // No depth data is present for the tile.
		{
			assert(minDepth == 1.0f);
			maxDepth = 1.0f;
		}

		auto& serverCamera = *pServerCameraGroup->Cameras[side];
		auto& projection = serverCamera.GetProjection().Projection.Perspective;
		bool isProjectingTo_0_1_Interval = projection.IsProjectingTo_0_1_Interval;
		float farDistance = projection.FarPlaneDistance;

		float minZ = -projection.GetLinearDepth(minDepth);
		float maxZ = -projection.GetLinearDepth(maxDepth);

		assert(maxZ >= minZ);
		const float depthEpsilon = 1e-3f;
		if (maxZ - minZ < depthEpsilon) maxZ = minZ + depthEpsilon;

		const float farEpsilon = 1e-3f;
		m_TileIsBackground[i] = (minZ >= farDistance * (1.0f - farEpsilon));

		auto& tileCamera = m_TileCameras[i];
		tileCamera.SetLocation(serverCamera);
		SetTileCameraProjection(tileCamera, i, texelSize, tileSize, minZ, maxZ, isProjectingTo_0_1_Interval);
	}
}
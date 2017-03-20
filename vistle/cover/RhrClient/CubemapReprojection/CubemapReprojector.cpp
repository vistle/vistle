// CubemapReprojector.h

// Must be included before OpenGL includes.
#include <External/glew-1.12.0/include/GL/glew.h>

#include <CubemapReprojector.h>

#include <Core/Constants.h>
#include <Core/Percentage.hpp>
#include <Core/Platform.h>
#include <Core/String.hpp>
#include <Core/System/SimpleIO.h>
#include <Core/System/Filesystem.h>
#include <EngineBuildingBlocks/ErrorHandling.h>
#include <OculusVR/SimpleVR_GL.h>

// For debugging:
#include <EngineBuildingBlocks/Graphics/Primitives/PrimitiveCreation.h>
#include <EngineBuildingBlocks/Graphics/Resources/ImageHelper.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace OpenGLRender;
using namespace CubemapStreaming;

inline void PrintException(const std::exception& ex)
{
	printf("An application error has been occured: %s\n", ex.what());
	std::cin.get();
}

///////////////////////////////////// CUBEMAP TEXTURES /////////////////////////////////////

void CubemapReprojector::InitializeColorCubemap(unsigned resourceIndex)
{
	auto& texture = m_ColorTextures[resourceIndex];
	texture.Delete();
	OpenGLRender::Texture2DDescription textureDesc;
	textureDesc.Width = m_ServerWidth;
	textureDesc.Height = m_ServerHeight;
	textureDesc.ArraySize = 1;
	textureDesc.Target = TextureTarget::TextureCubemap;
	textureDesc.Format = TextureFormat::RGBA8;
	textureDesc.HasMipmaps = true;
	TextureSamplingDescription samplingDesc;
	samplingDesc.MinifyingFilter = TextureMinifyingFilter::Linear_Mipmap_Linear;
	samplingDesc.MagnificationFilter = TextureMagnificationFilter::Linear;
	samplingDesc.WrapMode = TextureWrapMode::ClampToEdge;
	texture.Initialize(textureDesc, samplingDesc);

	// Setting 0 initially.
	Core::ByteVectorU initData(c_CountCubemapSides * m_ServerWidth * m_ServerHeight * 4, 0);

	texture.Bind();
	texture.SetData(initData.GetArray(), PixelDataFormat::RGBA, PixelDataType::Uint8);
	texture.GenerateMipmaps();
}

void CubemapReprojector::SetColorCubemapData(unsigned resourceIndex, unsigned char** ppData,
	bool isContainigAlpha, glm::uvec2* start, glm::uvec2* end)
{
	auto& texture = m_ColorTextures[resourceIndex];
	texture.Bind();
	for (unsigned i = 0; i < 6; i++)
	{
		unsigned offset[] = { start[i].x, start[i].y, i };
		unsigned size[] = { end[i].x - start[i].x, end[i].y - start[i].y, 1 };
		texture.SetData(ppData[i], (isContainigAlpha ? PixelDataFormat::RGBA : PixelDataFormat::RGB),
			PixelDataType::Uint8, offset, size, 0);
	}
	texture.GenerateMipmaps();
}

void CubemapReprojector::InitializeDepthTextureArrays(unsigned resourceIndex)
{
	auto& texture = m_DepthTextures[resourceIndex];
	texture.Delete();
	OpenGLRender::Texture2DDescription textureDesc;
	textureDesc.Width = m_ServerWidth;
	textureDesc.Height = m_ServerHeight;
	textureDesc.ArraySize = 6;
	textureDesc.Target = TextureTarget::Texture2DArray;
	textureDesc.Format = TextureFormat::R32F;
	textureDesc.HasMipmaps = true;
	TextureSamplingDescription samplingDesc;
	samplingDesc.MinifyingFilter = TextureMinifyingFilter::Linear_Mipmap_Nearest;
	samplingDesc.MagnificationFilter = TextureMagnificationFilter::Linear;
	samplingDesc.WrapMode = TextureWrapMode::ClampToEdge;
	texture.Initialize(textureDesc, samplingDesc);

	// Setting 1.0f initially.
	Core::SimpleTypeVector<float> initData(c_CountCubemapSides * m_ServerWidth * m_ServerHeight, 1.0f);
	unsigned char* ppData[c_CountCubemapSides];
	for (unsigned i = 0; i < c_CountCubemapSides; i++)
	{
		ppData[i] = reinterpret_cast<unsigned char*>(
			initData.GetArray() + i * m_ServerWidth * m_ServerHeight);
	}

	texture.Bind();
	texture.SetData(ppData[0], PixelDataFormat::Red, PixelDataType::Float);
}

void CubemapReprojector::SetDepthData(unsigned resourceIndex, unsigned char** ppData,
	glm::uvec2* start, glm::uvec2* end)
{
	auto& texture = m_DepthTextures[resourceIndex];
	texture.Bind();
	for (unsigned i = 0; i < 6; i++)
	{
		unsigned offset[] = { start[i].x, start[i].y, i };
		unsigned size[] = { end[i].x - start[i].x, end[i].y - start[i].y, 1 };
		texture.SetData(ppData[i], PixelDataFormat::Red, PixelDataType::Float, offset, size, 0);
	}
}

void DebugSourceTexture(PathHandler& pathHandler, BufferType bufferType)
{
	const bool isClearingBuffers = false;
	static bool isInitializing = true;
	static OpenGLRender::ShaderProgram showProgram;
	static Primitive quad;
	if (isInitializing)
	{
		isInitializing = false;

		auto vsPath = "Shaders/ShowBuffer_vs.glsl";
		auto psPath = (bufferType == BufferType::Color
			? "Shaders/ShowColorBuffer_ps.glsl"
			: "Shaders/ShowDepthBuffer_ps.glsl");

		OpenGLRender::ShaderProgramDescription spd;
		spd.Shaders =
		{
			ShaderDescription::FromFile(pathHandler.GetPathFromRootDirectory(vsPath), ShaderType::Vertex),
			ShaderDescription::FromFile(pathHandler.GetPathFromRootDirectory(psPath), ShaderType::Fragment)
		};
		showProgram.Initialize(spd);

		Vertex_SOA_Data vertexData;
		IndexData indexData;
		CreateQuadGeometry(vertexData, indexData, PrimitiveRange::_Minus1_To_Plus1);
		vertexData.RemoveVertexElement(c_PositionVertexElement);
		vertexData.RemoveVertexElement(c_NormalVertexElement);
		quad.Initialize(OpenGLRender::BufferUsage::StaticDraw, vertexData, indexData);
		showProgram.SetInputLayout(vertexData.InputLayout);
	}

	if (isClearingBuffers)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	GLint prevDepthFunc;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
	glDepthFunc(GL_LEQUAL);

	showProgram.Bind();

	if (bufferType == BufferType::Color) showProgram.SetUniformValue("ColorTexture", 0);
	else                                 showProgram.SetUniformValue("DepthTexture", 1);

	quad.Bind();
	glDrawElements(GL_TRIANGLES, quad.CountIndices, GL_UNSIGNED_INT, 0);

	glDepthFunc(prevDepthFunc);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CubemapReprojector::CubemapReprojector()
	: m_ClientWidth(0)
	, m_ClientHeight(0)
	, m_ServerWidth(2)
	, m_ServerHeight(2)
	, m_ThreadPool(1)
	, m_IsShuttingDown(false)
	, m_ServerCamera(&m_SceneNodeHandler)
	, m_ServerCameraCopy(&m_SceneNodeHandler)
	, m_ClientCamera(&m_SceneNodeHandler)
	, m_ClientCameraCopy(&m_SceneNodeHandler)
	, m_ServerCubemapCameraGroup(&m_ServerCamera)
	, m_ServerCubemapCameraGroupCopy(&m_ServerCameraCopy)
	, m_GridReprojector(m_SceneNodeHandler, &m_IsShuttingDown)
	, m_InitializedOpenGLContextId(Core::c_InvalidIndexU)
	, m_IsVRGraphicsInitialized(false)
{
	LoadConfiguration();

	if (!m_IsRenderingInVR && m_GridReprojector.IsMultisamplingEnabled()) m_CountSamples = 8;
	else m_CountSamples = 1;

	m_GridReprojector.SetClearingBuffers(false);
	
	// DEBUG: disabling culling in the grid reprojector.
	m_GridReprojector.SetCullingEnabled(false);

	if (m_IsRenderingInVR)
	{
		InitializeVR();
	}

	InitializeBuffers();
}

void CubemapReprojector::InitializeGL(unsigned openGLContextID)
{
	if (m_InitializedOpenGLContextId != Core::c_InvalidIndexU
		&& m_InitializedOpenGLContextId != openGLContextID)
	{
		printf("Multiple OpenGL contexts are not yet supported.\n");
	}

	if (m_InitializedOpenGLContextId == Core::c_InvalidIndexU)
	{
		InitializeGLEW();
		InitializeGLDebugging();

		auto shadersPath = m_PathHandler.GetPathFromRootDirectory("Shaders/");

		m_IsContainingAlpha = true;

		for (unsigned i = 0; i < c_CountTextures; i++)
		{
			InitializeColorCubemap(i);
			InitializeDepthTextureArrays(i);
		}
		m_CurrentTextureIndex = 0;

		m_GridReprojector.Initialize(shadersPath);

		CheckGLError();

		// Starting shader rebuilding thread.
		m_ThreadPool.GetThread(0).Execute(&ShaderRebuilder::RebuildShaders,
			&m_GridReprojector.GetShaderRebuilder(), &m_PathHandler);

		m_InitializedOpenGLContextId = openGLContextID;
	}
}

void CubemapReprojector::Destroy()
{
	m_IsShuttingDown = true;
	m_ThreadPool.Join();

	if (m_IsRenderingInVR)
	{
		ReleaseVR();
	}
}

// Calling this function twice per update for precise results.
void CubemapReprojector::UpdateVRData()
{
	glm::vec3 outputPosition;
	glm::mat3 outputViewOrientation;
	OculusVR::PerspectiveProjectionParameters projParams;

	m_VRHelper->UpdateLocationTracking();

	for (unsigned i = 0; i < 2; i++)
	{
		auto& inputCamera = m_ClientCamera;
		auto& outputCamera = *m_VRCameras[i];

		m_VRHelper->UpdateBeforeRendering(i, inputCamera.GetPosition(), inputCamera.GetViewOrientation(),
			outputPosition, outputViewOrientation, projParams);

		outputCamera.SetPosition(outputPosition);
		outputCamera.SetViewOrientation(outputViewOrientation);
		outputCamera.SetPerspectiveProjection(projParams.Left, projParams.Right, projParams.Bottom, projParams.Top,
			projParams.NearPlaneDistance, projParams.FarPlaneDistance, projParams.IsProjectingFrom_0_To_1);
	}
}

void CubemapReprojector::Render(unsigned openGLContextID)
{
	// Getting previous state.
	GLint prevActiveTexture, prevProgram;
	{
		glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTexture);
		glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
	}

	{
		std::lock_guard<std::mutex> lock(m_RenderSyncMutex);
		m_ServerCamera.Set(m_ServerCameraCopy);
		m_ClientCamera.Set(m_ClientCameraCopy);
	}

	m_ServerCubemapCameraGroup.Update();

	InitializeGL(openGLContextID);

	m_GridReprojector.Update();

	// We have to postpone VR graphics initialization until this point, since the output FBO is not created
	// at initialization time.
	if (m_IsRenderingInVR)
	{
		InitializeVRGraphics();
	}

	UpdateTextures();
	
	m_ColorTextures[m_CurrentTextureIndex].Bind(0);
	m_DepthTextures[m_CurrentTextureIndex].Bind(1);

	if (m_IsRenderingInVR)
	{
		UpdateVRData();

		for (unsigned i = 0; i < 2; i++)
		{
			m_FBOs[i].Bind();
			RenderReprojection(*m_VRCameras[i]);
		}
		m_VRHelper->OnRenderingFinished();
	}
	else
	{
		RenderReprojection(m_ClientCamera);
	}

	// Setting back previous state.
	{
		glActiveTexture(prevActiveTexture);
		glUseProgram(prevProgram);
	}

	CheckGLError();
}

void CubemapReprojector::RenderReprojection(Camera& clientCamera)
{
	m_GridReprojector.Render(m_ServerCubemapCameraGroup, clientCamera);
	//DebugSourceTexture(m_PathHandler, BufferType::Color);
}

/////////////////////////////////////////// VR ///////////////////////////////////////////

void CubemapReprojector::InitializeVR()
{
	m_VRHelper.reset(new OculusVR::VR_GL_Helper());
	m_VRHelper->InitializeSDK();

	for (unsigned i = 0; i < 2; i++)
	{
		m_VRCameras[i] = new EngineBuildingBlocks::Graphics::Camera(&m_SceneNodeHandler);
	}
}

void CubemapReprojector::ReleaseVR()
{
	m_VRHelper->Release();

	for (unsigned i = 0; i < 2; i++)
	{
		delete m_VRCameras[i];
	}
}

void CubemapReprojector::InitializeVRGraphics()
{
	if (!m_IsVRGraphicsInitialized)
	{
		bool isMultisamplingEnabled = m_GridReprojector.IsMultisamplingEnabled();
		unsigned sampleCount = (isMultisamplingEnabled ? 8 : 1);

		OculusVR::VR_GL_InitData vrInitData;
		vrInitData.WindowSize = glm::uvec2(m_ClientWidth, m_ClientHeight);
		vrInitData.SampleCount = sampleCount;
		vrInitData.Flags = OculusVR::VR_Flags::SetUpViewportForRendering;
		vrInitData.OutputFBO = 0; // Back buffer.

		for (unsigned i = 0; i < 2; i++)
		{
			auto& fbo = m_FBOs[i];
			auto size = m_VRHelper->GetEyeTextureSize(i);
			Texture2D colorTexture(OpenGLRender::Texture2DDescription(size.x, size.y, TextureFormat::RGBA8, 1, sampleCount));
			Texture2D depthTexture(OpenGLRender::Texture2DDescription(size.x, size.y, TextureFormat::DepthComponent32F, 1, sampleCount));
			fbo.Initialize();
			fbo.Bind();
			fbo.Attach(std::move(colorTexture), FrameBufferAttachment::Color);
			fbo.Attach(std::move(depthTexture), FrameBufferAttachment::Depth);
			vrInitData.InputEyeFBOs[i] = m_FBOs[i].GetHandle();
		}

		m_VRHelper->InitializeGraphics(vrInitData);

		m_IsVRGraphicsInitialized = true;
	}
}

///////////////////////////////////// CONFIGURATION, SERIALIZATION /////////////////////////////////////

template <typename T>
inline void InitializeRootProperty(const Core::Properties& configuration, const char* name, T& property)
{
	configuration.TryGetPropertyValue((std::string("Configuration.") + name).c_str(), property);
}

void CubemapReprojector::LoadConfiguration()
{
	m_Configuration.LoadFromXml(
		m_PathHandler.GetPathFromRootDirectory("Configurations/CubemapStreaming_Client_Configuration.xml").c_str());
	InitializeRootProperty(m_Configuration, "IsRenderingInVR", m_IsRenderingInVR);
	InitializeRootProperty(m_Configuration, "IsLoadingFakeCubemap", m_IsLoadingFakeCubemap);
	InitializeRootProperty(m_Configuration, "IsContainingAlpha", m_IsContainingAlpha);
	InitializeRootProperty(m_Configuration, "SideVisiblePortion", m_SideVisiblePortion);
	InitializeRootProperty(m_Configuration, "PosZVisiblePortion", m_PosZVisiblePortion);
	InitializeRootProperty(m_Configuration, "MinPosDiffForServer", m_MinPosDiffForServer);
	InitializeRootProperty(m_Configuration, "MinDirCrossForServer", m_MinDirCrossForServer);

	m_GridReprojector.LoadConfiguration(m_Configuration);
}

void CubemapReprojector::SaveCameraLocation()
{
	auto cameraLocationPath = m_PathHandler.GetPathFromRootDirectory("Temp/CameraLocation.bin");

	Core::ByteVector bytes;
	Core::SerializeSB(bytes, Core::ToPlaceHolder(m_ClientCamera.GetLocalOrientation()));
	Core::SerializeSB(bytes, Core::ToPlaceHolder(m_ClientCamera.GetLocalPosition()));
	Core::WriteAllBytes(cameraLocationPath, bytes.GetArray(), bytes.GetSize());
}

void CubemapReprojector::LoadCameraLocation()
{
	auto cameraLocationPath = m_PathHandler.GetPathFromRootDirectory("Temp/CameraLocation.bin");

	if (Core::FileExists(cameraLocationPath))
	{
		Core::ByteVector bytes;
		Core::ReadAllBytes(cameraLocationPath, bytes);

		glm::mat3 clientOrientation;
		glm::vec3 clientPosition;
		const unsigned char* pBytes = bytes.GetArray();
		Core::DeserializeSB(pBytes, Core::ToPlaceHolder(clientOrientation));
		Core::DeserializeSB(pBytes, Core::ToPlaceHolder(clientPosition));

		m_ClientCamera.SetLocalOrientation(clientOrientation);
		m_ClientCamera.SetLocalPosition(clientPosition);
	}
}

///////////////////////////////////// DATA TRANSFER /////////////////////////////////////

void CubemapReprojector::InitializeBuffers()
{
	m_Buffers.resize(c_CountBuffers);
	m_WriteBufferIndex = 0;
	m_WrittenBufferIndex = Core::c_InvalidIndexU;
	m_ReadBufferIndex = Core::c_InvalidIndexU;
}

unsigned CubemapReprojector::GetBufferIndex(BufferType type, unsigned sideIndex, unsigned tbIndex)
{
	return ((unsigned)type * c_CountCubemapSides + sideIndex) * c_TripleBuffering + tbIndex;
}

unsigned char* CubemapReprojector::GetColorBuffer(unsigned index)
{
	return m_Buffers[GetBufferIndex(BufferType::Color, index, m_WriteBufferIndex)].GetArray();
}

unsigned char* CubemapReprojector::GetDepthBuffer(unsigned index)
{
	return m_Buffers[GetBufferIndex(BufferType::Depth, index, m_WriteBufferIndex)].GetArray();
}

void CubemapReprojector::IncrementWriteBufferIndex()
{
	if (++m_WriteBufferIndex == c_TripleBuffering) m_WriteBufferIndex = 0;
}

void CubemapReprojector::SwapY(void* pBuffer, unsigned width, unsigned height, unsigned elementSize)
{
	unsigned rowSize = width * elementSize;
	m_SwapVector.Resize(rowSize);
	auto tempPtr = m_SwapVector.GetArray();
	auto ptr1 = reinterpret_cast<uint8_t*>(pBuffer);
	auto ptr2 = ptr1 + (height - 1) * rowSize;
	for (; ptr1 < ptr2; ptr1 += rowSize, ptr2 -= rowSize)
	{
		memcpy(tempPtr, ptr1, rowSize);
		memcpy(ptr1, ptr2, rowSize);
		memcpy(ptr2, tempPtr, rowSize);
	}
}

const bool c_IsDebuggingBuffers = false;

void CubemapReprojector::SwapBuffers()
{
	// TODO: implement depth swapping in shader!
	for (unsigned i = 0; i < c_CountCubemapSides; i++)
	{
		auto& buffer = m_Buffers[GetBufferIndex(BufferType::Depth, i, m_WriteBufferIndex)];
		SwapY(buffer.GetArray(), m_ServerWidth, m_ServerHeight, sizeof(float));
	}

	std::lock_guard<std::mutex> lock(m_BufferMutex);

	if (c_IsDebuggingBuffers)
	{
		for (unsigned sideIndex = 0; sideIndex < 6; sideIndex++)
		{
			auto colorBuffer
				= m_Buffers[GetBufferIndex(BufferType::Color, sideIndex, m_WriteBufferIndex)].GetArray();
			char fileRelPath[32];
			snprintf(fileRelPath, 32, "Temp/ColorBuffer_%d.png", sideIndex);
			SaveImageToFile(colorBuffer,
				Image2DDescription::ColorImage(m_ServerWidth, m_ServerHeight, m_IsContainingAlpha ? 4 : 3),
				m_PathHandler.GetPathFromRootDirectory(fileRelPath),
				ImageSaveFlags::IsFlippingY);
		}
	}

	m_WrittenBufferIndex = m_WriteBufferIndex;
	IncrementWriteBufferIndex();
	if (m_WriteBufferIndex == m_ReadBufferIndex)
		IncrementWriteBufferIndex();
}

inline bool NeedsResizing(unsigned bufferSize, unsigned width, unsigned height, bool isContainingAlpha)
{
	unsigned pixelSize = (isContainingAlpha ? 4 : 3);
	return (bufferSize != width * height * pixelSize);
}

void CubemapReprojector::UpdateTextures()
{
	bool isDataAvailable;
	{
		std::lock_guard<std::mutex> lock(m_BufferMutex);
		isDataAvailable = (m_ReadBufferIndex != m_WrittenBufferIndex);
		m_ReadBufferIndex = m_WrittenBufferIndex;
	}
	if (isDataAvailable)
	{
		unsigned writeTextureIndex = 1 - m_CurrentTextureIndex;

		auto& checkBuffer = m_Buffers[GetBufferIndex(BufferType::Color, 0, m_ReadBufferIndex)];
		auto& checkTextureDesc = m_ColorTextures[writeTextureIndex].GetTextureDescription();
		if (NeedsResizing(checkBuffer.GetSizeInBytes(), checkTextureDesc.Width, checkTextureDesc.Height,
			m_IsContainingAlpha))
		{
			InitializeColorCubemap(writeTextureIndex);
			InitializeDepthTextureArrays(writeTextureIndex);
		}

		unsigned char* colorDataPointers[c_CountCubemapSides];
		unsigned char* depthDataPointers[c_CountCubemapSides];

		glm::uvec2 totalSize(m_ServerWidth, m_ServerHeight);
		glm::uvec2 sideStarts[c_CountCubemapSides], sideEnds[c_CountCubemapSides];
		GetImageDataSideBounds(m_ServerWidth, m_ServerHeight, m_SideVisiblePortion, m_PosZVisiblePortion,
			sideStarts, sideEnds);

		// Modifying starts and ends for flipped ranges.
		glm::uvec2 colorSideStarts[c_CountCubemapSides], colorSideEnds[c_CountCubemapSides];
		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			unsigned yIndex;
			if (i == 2 || i == 3) yIndex = 5 - i;
			else yIndex = i;
			colorSideStarts[i] = glm::uvec2(totalSize.x - sideEnds[i].x, sideStarts[yIndex].y);
			colorSideEnds[i] = glm::uvec2(totalSize.x - sideStarts[i].x, sideEnds[yIndex].y);
		}

		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			colorDataPointers[i] = m_Buffers[GetBufferIndex(BufferType::Color, i, m_ReadBufferIndex)].GetArray();
			depthDataPointers[i] = m_Buffers[GetBufferIndex(BufferType::Depth, i, m_ReadBufferIndex)].GetArray();
		}

		SetColorCubemapData(writeTextureIndex, colorDataPointers, m_IsContainingAlpha,
			colorSideStarts, colorSideEnds);
		SetDepthData(writeTextureIndex, depthDataPointers, sideStarts, sideEnds);

		m_GridReprojector.SetTextureData(depthDataPointers, m_ServerWidth, m_ServerHeight,
			m_IsContainingAlpha, m_SideVisiblePortion, m_PosZVisiblePortion,
			m_ServerCubemapCameraGroup);

		m_CurrentTextureIndex = writeTextureIndex;
	}
}

///////////////////////////////////////// OTHER PUBLIC FUNCTIONS /////////////////////////////////////////

glm::mat4 ToMatrix(const double* values)
{
	glm::mat4 m(glm::uninitialize);
	for (int c = 0; c < 16; c++) glm::value_ptr(m)[c] = (float)values[c];
	return m;
}

void CreateFromMatrix(const glm::mat4& m, double* values)
{
	for (int c = 0; c < 16; c++) values[c] = glm::value_ptr(m)[c];
}

RigidTransformation ToRigidTransformation(const double* values)
{
	return RigidTransformation(ToMatrix(values));
}

CameraProjection ToProjection(const double* values)
{
	CameraProjection projection;
	if (!projection.CreateFromMatrix(ToMatrix(values))) printf("Camera projection error!\n");
	return projection;
}

void CubemapReprojector::AdjustDimensionsAndMatrices(unsigned sideIndex,
	unsigned short clientWidth, unsigned short clientHeight,
	unsigned short* serverWidth, unsigned short* serverHeight,
	const double* leftViewMatrix, const double* rightViewMatrix,
	const double* leftProjMatrix, const double* rightProjMatrix,
	double* viewMatrix, double* projMatrix)
{
	// We assume, that the function is first called with the index = 0.
	if (sideIndex == 0)
	{
		m_ClientWidth = clientWidth;
		m_ClientHeight = clientHeight;

		unsigned serverSize = std::max(*serverWidth, *serverHeight);
		m_ServerWidth = serverSize;
		m_ServerHeight = serverSize;

		unsigned colorBufferSize = m_ServerWidth * m_ServerHeight * (m_IsContainingAlpha ? 4 : 3);
		unsigned depthBufferSize = m_ServerWidth * m_ServerHeight * sizeof(float);
		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			for (unsigned j = 0; j < c_TripleBuffering; j++)
			{
				m_Buffers[GetBufferIndex(BufferType::Color, i, j)].Resize(colorBufferSize);
				m_Buffers[GetBufferIndex(BufferType::Depth, i, j)].Resize(depthBufferSize);
			}
		}

		std::lock_guard<std::mutex> lock(m_RenderSyncMutex);

		auto leftViewTr = ToRigidTransformation(leftViewMatrix);
		auto rightViewTr = ToRigidTransformation(rightViewMatrix);
		assert(leftViewTr.Orientation == rightViewTr.Orientation);
		auto pos = (leftViewTr.Position + rightViewTr.Position) * 0.5f;
		auto viewTr = RigidTransformation(leftViewTr.Orientation, pos);
		m_ServerCameraCopy.SetFromViewMatrix(viewTr.AsMatrix4());

		auto leftProj = ToProjection(leftProjMatrix);
		auto rightProj = ToProjection(rightProjMatrix);
		auto& lp = leftProj.Projection.Perspective;
		auto& rp = rightProj.Projection.Perspective;
		assert(leftProj.Type == ProjectionType::Perspective && rightProj.Type == ProjectionType::Perspective);
		assert(lp.NearPlaneDistance == rp.NearPlaneDistance && lp.FarPlaneDistance == rp.FarPlaneDistance);
		assert(lp.Bottom == rp.Bottom && lp.Top == rp.Top && lp.Right - lp.Left == rp.Right - rp.Left);
		CameraProjection proj;
		proj.CreatePerspecive(1.0f, glm::half_pi<float>(), lp.NearPlaneDistance, lp.FarPlaneDistance, false);
		m_ServerCameraCopy.SetProjection(proj);
		m_ServerCubemapCameraGroupCopy.Update();
	}

	*serverWidth = m_ServerWidth;
	*serverHeight = m_ServerHeight;

	auto camera = m_ServerCubemapCameraGroupCopy.Cameras[sideIndex];
	auto& sideViewMatrix = camera->GetViewMatrix();
	auto& sideProjectionMatrix = camera->GetProjectionMatrix();
	CreateFromMatrix(sideViewMatrix, viewMatrix);
	CreateFromMatrix(sideProjectionMatrix, projMatrix);
}

inline void SetCameraFromMatrices(Camera& camera, const double* viewMatrix, const double* projMatrix)
{
	camera.SetFromViewMatrix(ToMatrix(viewMatrix));
	camera.SetProjection(ToProjection(projMatrix));
}

void CubemapReprojector::SetServerCameraTransformations(const double* viewMatrix, const double* projMatrix)
{
	// We don't assume that the previously set adjusted matrices are the ones, which we set here: setting them again.
	std::lock_guard<std::mutex> lock(m_RenderSyncMutex);
	SetCameraFromMatrices(m_ServerCameraCopy, viewMatrix, projMatrix);
}

void CubemapReprojector::SetClientCameraTransformations(const double* viewMatrix, const double* projMatrix)
{
	std::lock_guard<std::mutex> lock(m_RenderSyncMutex);
	SetCameraFromMatrices(m_ClientCameraCopy, viewMatrix, projMatrix);
}
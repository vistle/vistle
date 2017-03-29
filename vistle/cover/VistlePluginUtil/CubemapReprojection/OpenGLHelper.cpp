// CubemapStreaming_Client/OpenGLHelper.cpp

#include <OpenGLHelper.h>

#include <Core/Debug.h>
#include <Core/System/SimpleIO.h>

#include <thread>
#include <sys/stat.h>
#include <sys/types.h>

using namespace EngineBuildingBlocks::Graphics;

void PrintOpenGLContextInfo()
{
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	printf(
		"OpenGL context info:\n"
		"--------------------\n\n"
		"Version: %d.%d\n"
		, major, minor);
}

///////////////////////////////////// TEXTURE PROCESSING /////////////////////////////////////

void SwapHorizontally32Bit(uint32_t* data, int width, int height)
{
	uint32_t* startFrontPtr = data;
	uint32_t* startBackPtr = data + width - 1;
	uint32_t* invalidFrontPtr = data + width * height;
	uint32_t* frontPtr;
	uint32_t* backPtr;
	uint32_t temp;
	while (startFrontPtr != invalidFrontPtr)
	{
		frontPtr = startFrontPtr;
		backPtr = startBackPtr;
		for (; frontPtr < backPtr; ++frontPtr, --backPtr)
		{
			temp = *frontPtr; *frontPtr = *backPtr; *backPtr = temp;
		}
		startFrontPtr += width;
		startBackPtr += width;
	}
}

void SwapHorizontally24Bit(unsigned char* data, int width, int height)
{
	auto startFrontPtr = data;
	auto startBackPtr = data + (width - 1) * 3;
	auto invalidFrontPtr = data + (width * height) * 3;
	auto startPtrIncrement = width * 3;
	unsigned elementSize = 3;
	unsigned char* frontPtr;
	unsigned char* backPtr;
	unsigned char tempPtr[3];
	while (startFrontPtr != invalidFrontPtr)
	{
		frontPtr = startFrontPtr;
		backPtr = startBackPtr;
		for (; frontPtr < backPtr; frontPtr += elementSize, backPtr -= elementSize)
		{
			memcpy(tempPtr, frontPtr, elementSize);
			memcpy(frontPtr, backPtr, elementSize);
			memcpy(backPtr, tempPtr, elementSize);
		}
		startFrontPtr += startPtrIncrement;
		startBackPtr += startPtrIncrement;
	}
}

void SwapVertically32Bit(uint32_t* data, int width, int height, uint32_t* tempRow)
{
	uint32_t* startFrontPtr = data;
	uint32_t* startBackPtr = data + width * (height - 1);
	unsigned rowSize = width * (unsigned)sizeof(uint32_t);
	while (startFrontPtr < startBackPtr)
	{
		memcpy(tempRow, startFrontPtr, rowSize);
		memcpy(startFrontPtr, startBackPtr, rowSize);
		memcpy(startBackPtr, tempRow, rowSize);

		startFrontPtr += width;
		startBackPtr -= width;
	}
}

///////////////////////////////////// PRIMITIVE /////////////////////////////////////

void Primitive::Initialize(OpenGLRender::BufferUsage usage,
	const Vertex_SOA_Data& vertexData, const IndexData& indexData)
{
	CountIndices = indexData.GetCountIndices();
	VAO.Initialize();
	VAO.Bind();
	VBO.Initialize(usage, vertexData);
	IndexBuffer.Initialize(usage, indexData);
}

void Primitive::Delete()
{
	VAO.Delete();
	VBO.Delete();
	IndexBuffer.Delete();
	CountIndices = 0;
}

void Primitive::Bind()
{
	VAO.Bind();
}

///////////////////////////////////// SHADER REBUILDING /////////////////////////////////////

void SetShaderVersion(OpenGLRender::ShaderDescription& shaderDescription)
{
	shaderDescription.Version = "450 core";
}

void AddEmptyShaderDefines(OpenGLRender::ShaderDescription& shaderDescription)
{
	SetShaderVersion(shaderDescription);
}

void InitializeShader(ShaderProgramData& data)
{
	data.NeedsRebuild = true;
	data.IsBound = false;
	data.Program = nullptr;
}

void DeleteShader(ShaderProgramData& data)
{
	delete data.Program;
}

void BuildShader(ShaderProgramData& data)
{
	auto& shaders = data.Shaders;

	if (data.IsBound)
	{
		assert(data.Program != nullptr);
		data.Program->Unbind();
	}
	if (data.Program != nullptr)
	{
		delete data.Program;
	}
	data.Program = new OpenGLRender::ShaderProgram();
	OpenGLRender::ShaderProgramDescription programDescription;
	for (size_t i = 0; i < shaders.size(); i++)
	{
		programDescription.Shaders.push_back(shaders[i].Description);
	}
	data.Program->Initialize(programDescription);
	data.NeedsRebuild = false;

	OpenGLRender::CheckGLError();
}

inline long long GetLastSaveTime(const ShaderData& shaderData,
	const EngineBuildingBlocks::PathHandler* pPathHandler)
{
	struct stat buf = { 0 };
	stat(shaderData.Description.GetFilePath().c_str(), &buf);
	return static_cast<long long>(buf.st_mtime);
}

ShaderRebuilder::ShaderRebuilder(std::atomic<bool>* pIsShuttingDown)
	: m_PIsShuttingDown(pIsShuttingDown)
{
}

void ShaderRebuilder::RebuildShaders(const EngineBuildingBlocks::PathHandler* pPathHandler)
{
	// Initializing last save times.
	for (auto& it : ShaderPrograms)
	{
		auto& shaders = it.second.Shaders;
		for (size_t j = 0; j < shaders.size(); j++)
		{
			auto& shader = shaders[j];
			shader.LastSaved = GetLastSaveTime(shader, pPathHandler);
		}
	}

	// Updating shader programs if one of its shaders were changed.
	while (!m_PIsShuttingDown->load())
	{
		m_ShaderRebuildMutex.lock();
		for (auto& it : ShaderPrograms)
		{
			auto& shaders = it.second.Shaders;
			bool needsToRebuild = false;
			for (size_t j = 0; j < shaders.size(); j++)
			{
				auto& shader = shaders[j];
				auto newLastSaved = GetLastSaveTime(shader, pPathHandler);
				if (shader.LastSaved < newLastSaved)
				{
					needsToRebuild = true;
					break;
				}
			}
			if (needsToRebuild)
			{
				for (size_t j = 0; j < shaders.size(); j++)
				{
					shaders[j].LastSaved = GetLastSaveTime(shaders[j], pPathHandler);
				}
				it.second.NeedsRebuild = true;
			}
		}
		m_ShaderRebuildMutex.unlock();

		// Sleeping for 1s.
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void ShaderRebuilder::ApplyNewShader()
{
	m_ShaderRebuildMutex.lock();
	for (auto& it : ShaderPrograms)
	{
		if (it.second.NeedsRebuild)
		{
			BuildShader(it.second);
		}
	}
	m_ShaderRebuildMutex.unlock();
}
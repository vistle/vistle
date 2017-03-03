// CubemapStreaming_Client/OpenGLHelper.h

#ifndef _CUBEMAPSTREAMING_CLIENT_QTOPENGLHELPER_H_INCLUDED_
#define _CUBEMAPSTREAMING_CLIENT_QTOPENGLHELPER_H_INCLUDED_

#include <EngineBuildingBlocks/Graphics/Primitives/Primitive.h>
#include <OpenGLRender/Resources/Shader.h>
#include <OpenGLRender/Resources/VertexArrayObject.h>
#include <OpenGLRender/Resources/VertexBuffer.h>
#include <OpenGLRender/Resources/IndexBuffer.h>

#include <map>
#include <mutex>
#include <atomic>

void PrintOpenGLContextInfo();

//////////////////////////// TEXTURE PROCESSING ////////////////////////////

void SwapHorizontally32Bit(uint32_t* data, int width, int height);
void SwapHorizontally24Bit(unsigned char* data, int width, int height);
void SwapVertically32Bit(uint32_t* data, int width, int height, uint32_t* tempRow);

//////////////////////////// PRIMITIVE ////////////////////////////

struct Primitive
{
	OpenGLRender::VertexArrayObject VAO;
	OpenGLRender::VertexBuffer VBO;
	OpenGLRender::IndexBuffer IndexBuffer;

	unsigned CountIndices = 0;

	void Initialize(OpenGLRender::BufferUsage usage,
		const EngineBuildingBlocks::Graphics::Vertex_SOA_Data& vertexData,
		const EngineBuildingBlocks::Graphics::IndexData& indexData);
	void Delete();
	void Bind();
};

//////////////////////////// SHADER REBUILDING ////////////////////////////

typedef void(*ShaderDefinesAddingFunction)(OpenGLRender::ShaderDescription& shaderDescription);

struct ShaderData
{
	OpenGLRender::ShaderDescription Description;
	long long LastSaved;
};

struct ShaderProgramData
{
	bool NeedsRebuild;
	bool IsBound;
	OpenGLRender::ShaderProgram* Program;
	ShaderDefinesAddingFunction DefinesAddingFunction;
	std::vector<ShaderData> Shaders;
};

void SetShaderVersion(OpenGLRender::ShaderDescription& shaderDescription);
void AddEmptyShaderDefines(OpenGLRender::ShaderDescription& shaderDescription);

void InitializeShader(ShaderProgramData& data);
void BuildShader(ShaderProgramData& data);
void DeleteShader(ShaderProgramData& data);

class ShaderRebuilder
{
	std::mutex m_ShaderRebuildMutex;
	std::atomic<bool>* m_PIsShuttingDown;

public:

	std::map<unsigned, ShaderProgramData> ShaderPrograms;

	ShaderRebuilder(std::atomic<bool>* pIsShuttingDown);
	void RebuildShaders(const EngineBuildingBlocks::PathHandler* pPathHandler);
	void ApplyNewShader();
};

#endif
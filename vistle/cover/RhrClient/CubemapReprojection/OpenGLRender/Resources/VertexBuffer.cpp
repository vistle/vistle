// OpenGLRender/Resources/VertexBuffer.cpp

#include <OpenGLRender/Resources/VertexBuffer.h>

using namespace OpenGLRender;

void VertexBuffer::Initialize(BufferUsage usage, unsigned countVertices,
	const EngineBuildingBlocks::Graphics::VertexInputLayout& inputLayout)
{
	m_InputLayout = inputLayout;
	m_CountVertices = countVertices;
	m_Buffer.Initialize(BufferTarget::VertexBuffer, inputLayout.GetVertexStride() * countVertices, usage, nullptr);
}

void VertexBuffer::Initialize(BufferUsage usage, const EngineBuildingBlocks::Graphics::Vertex_AOS_Data& aosData)
{
	m_InputLayout = aosData.InputLayout;
	m_CountVertices = aosData.GetCountVertices();
	m_Buffer.Initialize(BufferTarget::VertexBuffer, aosData.GetSize(), usage, aosData.Data.GetArray());
}

void VertexBuffer::Initialize(BufferUsage usage, const EngineBuildingBlocks::Graphics::Vertex_SOA_Data& soaData)
{
	auto aosData = soaData.As_AOS_Data();
	Initialize(usage, aosData);
}

void VertexBuffer::Delete()
{
	m_Buffer.Delete();
}

unsigned VertexBuffer::GetSize() const
{
	return m_Buffer.GetSize();
}

unsigned VertexBuffer::GetCountVertices() const
{
	return m_CountVertices;
}

const EngineBuildingBlocks::Graphics::VertexInputLayout& VertexBuffer::GetInputLayout() const
{
	return m_InputLayout;
}

GLuint VertexBuffer::GetHandle() const
{
	return m_Buffer.GetHandle();
}

void VertexBuffer::Bind()
{
	m_Buffer.Bind();
}

void VertexBuffer::Unbind()
{
	m_Buffer.Unbind();
}

void* VertexBuffer::Map(BufferAccessFlags accessFlags, unsigned offset, unsigned size)
{
	return m_Buffer.Map(accessFlags, offset, size);
}

void VertexBuffer::Unmap()
{
	m_Buffer.Unmap();
}

void VertexBuffer::Read(void* pData, unsigned offset, unsigned size)
{
	m_Buffer.Read(pData, offset, size);
}

void VertexBuffer::Write(const void* pData, unsigned offset, unsigned size)
{
	m_Buffer.Write(pData, offset, size);
}
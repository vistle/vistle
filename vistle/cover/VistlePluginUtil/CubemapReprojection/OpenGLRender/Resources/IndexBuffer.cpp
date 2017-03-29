// OpenGLRender/Resources/IndexBuffer.cpp

#include <OpenGLRender/Resources/IndexBuffer.h>

using namespace OpenGLRender;

void IndexBuffer::Initialize(BufferUsage usage, unsigned countIndices, const unsigned* pData)
{
	auto size = countIndices << 2; // Indices are always unsigned values.
	m_Buffer.Initialize(BufferTarget::IndexBuffer, size, usage, pData);
}

void IndexBuffer::Initialize(BufferUsage usage, const EngineBuildingBlocks::Graphics::IndexData& indexData)
{
	Initialize(usage, indexData.GetCountIndices(), indexData.Data.GetArray());
}

void IndexBuffer::Delete()
{
	m_Buffer.Delete();
}

unsigned IndexBuffer::GetSize() const
{
	return m_Buffer.GetSize();
}

unsigned IndexBuffer::GetCountIndices() const
{
	return (GetSize() >> 2); // Indices are always unsigned values.
}

GLuint IndexBuffer::GetHandle() const
{
	return m_Buffer.GetHandle();
}

void IndexBuffer::Bind()
{
	m_Buffer.Bind();
}

void IndexBuffer::Unbind()
{
	m_Buffer.Unbind();
}

void* IndexBuffer::Map(BufferAccessFlags accessFlags, unsigned offset, unsigned size)
{
	return m_Buffer.Map(accessFlags, offset, size);
}

void IndexBuffer::Unmap()
{
	m_Buffer.Unmap();
}

void IndexBuffer::Read(unsigned* pData, unsigned offset, unsigned size)
{
	m_Buffer.Read(pData, offset, size);
}

void IndexBuffer::Write(const unsigned* pData, unsigned offset, unsigned size)
{
	m_Buffer.Write(pData, offset, size);
}
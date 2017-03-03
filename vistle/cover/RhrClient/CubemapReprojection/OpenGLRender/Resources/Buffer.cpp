// OpenGLRender/Resources/Buffer.cpp

#include <OpenGLRender/Resources/Buffer.h>

#include <cassert>

using namespace OpenGLRender;

Buffer::Buffer()
	: m_Size(0)
{
}

void Buffer::Initialize(BufferTarget target, unsigned size, BufferUsage usage, const void* pData)
{
	m_Target = target;
	m_Size = size;
	m_Usage = usage;
	glGenBuffers(1, m_Handle.GetPointer());
	glBindBuffer((GLenum)target, m_Handle);
	glBufferData((GLenum)target, size, pData, (GLenum)usage);
}

void Buffer::Delete()
{
	m_Handle.Delete();
}

BufferTarget Buffer::GetTarget() const
{
	return m_Target;
}

unsigned Buffer::GetSize() const
{
	return m_Size;
}

BufferUsage Buffer::GetUsage() const
{
	return m_Usage;
}

GLuint Buffer::GetHandle() const
{
	return m_Handle;
}

void Buffer::DeleteResource(GLuint handle)
{
	glDeleteBuffers(1, &handle);
}

void Buffer::Bind()
{
	glBindBuffer((GLenum)m_Target, m_Handle);
}

void Buffer::Unbind()
{
	glBindBuffer((GLenum)m_Target, 0);
}

void* Buffer::Map(BufferAccessFlags accessFlags, unsigned offset, unsigned size)
{
	if (size == 0) size = m_Size - offset;
	glBindBuffer((GLenum)m_Target, m_Handle);
	auto ptr = glMapBufferRange((GLenum)m_Target, offset, size, (GLbitfield)accessFlags);
	assert(ptr != nullptr && "Failed to map the buffer.");
	return ptr;
}

void Buffer::Unmap()
{
	glUnmapBuffer((GLenum)m_Target);
}

void Buffer::Read(void* pData, unsigned offset, unsigned size)
{
	if (size == 0) size = m_Size - offset;
	glBindBuffer((GLenum)m_Target, m_Handle);
	glGetBufferSubData((GLenum)m_Target, offset, size, pData);
}

void Buffer::Write(const void* pData, unsigned offset, unsigned size)
{
	if (size == 0) size = m_Size - offset;
	glBindBuffer((GLenum)m_Target, m_Handle);
	glBufferSubData((GLenum)m_Target, offset, size, pData);
}
// OpenGLRender/Resources/VertexArrayObject.cpp

#include <OpenGLRender/Resources/VertexArrayObject.h>

using namespace OpenGLRender;

void VertexArrayObject::Initialize()
{
	glGenVertexArrays(1, m_Handle.GetPointer());
}

void VertexArrayObject::Delete()
{
	m_Handle.Delete();
}

GLuint VertexArrayObject::GetHandle() const
{
	return m_Handle;
}

void VertexArrayObject::DeleteResource(GLuint handle)
{
	glDeleteVertexArrays(1, &handle);
}

void VertexArrayObject::Bind()
{
	glBindVertexArray(m_Handle);
}

void VertexArrayObject::Unbind()
{
	glBindVertexArray(0U);
}
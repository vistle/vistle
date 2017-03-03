// OpenGLRender/Resources/Resource.h

#ifndef OPENGLRENDER_RESOURCE_H_INCLUDED_
#define OPENGLRENDER_RESOURCE_H_INCLUDED_

#include <Core/Constants.h>
#include <OpenGLRender/OpenGL.h>

#include <utility>

template <typename UserType>
class ResourceHandle
{
	GLuint m_Handle;

	void _Delete() noexcept  { if (m_Handle != Core::c_InvalidIndexU) UserType::DeleteResource(m_Handle); }

public:

	ResourceHandle() noexcept : m_Handle(Core::c_InvalidIndexU) {}
	explicit ResourceHandle(GLuint handle) noexcept : m_Handle(handle) {}
	~ResourceHandle() noexcept { _Delete(); }
	ResourceHandle(const ResourceHandle&) = delete;
	ResourceHandle(ResourceHandle&& other) noexcept { m_Handle = other.m_Handle; other.m_Handle = Core::c_InvalidIndexU; }
	ResourceHandle& operator=(const ResourceHandle&) = delete;
	ResourceHandle& operator=(ResourceHandle&& other) noexcept { std::swap(m_Handle, other.m_Handle); return *this; }
	ResourceHandle& operator=(GLuint handle) noexcept { if (handle != m_Handle) { _Delete(); m_Handle = handle; } return *this; }
	operator GLuint() const noexcept { return m_Handle; }
	GLuint* GetPointer() noexcept { return &m_Handle; }
	void Delete() noexcept { _Delete(); m_Handle = Core::c_InvalidIndexU;  }
};

#endif
// OpenGLRender/Resources/VertexArrayObject.h

#ifndef OPENGLRENDER_VERTEXARRAYOBJECT_H_INCLUDED_
#define OPENGLRENDER_VERTEXARRAYOBJECT_H_INCLUDED_

#include <OpenGLRender/Resources/Resource.h>

namespace OpenGLRender
{
	class VertexArrayObject
	{
	public:

		static void DeleteResource(GLuint handle);

	private:

		ResourceHandle<VertexArrayObject> m_Handle;

	public:

		void Initialize();
		void Delete();

		GLuint GetHandle() const;

		void Bind();
		void Unbind();
	};
}

#endif
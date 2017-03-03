// OpenGLRender/Resources/IndexBuffer.h

#ifndef OPENGLRENDER_INDEXBUFFER_H_INCLUDED_
#define OPENGLRENDER_INDEXBUFFER_H_INCLUDED_

#include <EngineBuildingBlocks/Graphics/Primitives/Primitive.h>
#include <OpenGLRender/Resources/Buffer.h>

namespace OpenGLRender
{
	class IndexBuffer
	{
		Buffer m_Buffer;

	public:

		void Initialize(BufferUsage usage, unsigned countIndices,
			const unsigned* pData = nullptr);
		void Initialize(BufferUsage usage,
			const EngineBuildingBlocks::Graphics::IndexData& indexData);
		void Delete();

		unsigned GetSize() const;
		unsigned GetCountIndices() const;

		GLuint GetHandle() const;

		void Bind();
		void Unbind();

		void* Map(BufferAccessFlags accessFlags, unsigned offset = 0, unsigned size = 0);
		void Unmap();

		void Read(unsigned* pData, unsigned offset = 0, unsigned size = 0);
		void Write(const unsigned* pData, unsigned offset = 0, unsigned size = 0);
	};
}

#endif
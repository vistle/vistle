// OpenGLRender/Resources/VertexBuffer.h

#ifndef OPENGLRENDER_VERTEXBUFFER_H_INCLUDED_
#define OPENGLRENDER_VERTEXBUFFER_H_INCLUDED_

#include <EngineBuildingBlocks/Graphics/Primitives/Primitive.h>
#include <OpenGLRender/Resources/Buffer.h>

namespace OpenGLRender
{
	class VertexBuffer
	{
		Buffer m_Buffer;
		EngineBuildingBlocks::Graphics::VertexInputLayout m_InputLayout;
		unsigned m_CountVertices;

	public:

		void Initialize(BufferUsage usage, unsigned countVertices,
			const EngineBuildingBlocks::Graphics::VertexInputLayout& inputLayout);
		void Initialize(BufferUsage usage,
			const EngineBuildingBlocks::Graphics::Vertex_AOS_Data& aosData);
		void Initialize(BufferUsage usage,
			const EngineBuildingBlocks::Graphics::Vertex_SOA_Data& soaData);
		void Delete();

		unsigned GetSize() const;
		unsigned GetCountVertices() const;
		const EngineBuildingBlocks::Graphics::VertexInputLayout& GetInputLayout() const;

		GLuint GetHandle() const;

		void Bind();
		void Unbind();

		void* Map(BufferAccessFlags accessFlags, unsigned offset = 0, unsigned size = 0);
		void Unmap();

		void Read(void* pData, unsigned offset = 0, unsigned size = 0);
		void Write(const void* pData, unsigned offset = 0, unsigned size = 0);
	};
}

#endif
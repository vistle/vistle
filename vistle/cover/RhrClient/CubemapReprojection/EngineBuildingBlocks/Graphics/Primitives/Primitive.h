// EngineBuildingBlocks/Graphics/Primitives/Primitive.h

#ifndef _ENGINEBUILDINGBLOCKS_PRIMITIVE_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_PRIMITIVE_H_INCLUDED_

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/SimpleBinarySerialization.hpp>
#include <Core/Constants.h>
#include <EngineBuildingBlocks/Math/GLM.h>

#include <vector>
#include <string>

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		enum class VertexElementType : unsigned char
		{
			Unknown = 0, Float, Uint32
		};

		struct VertexElement
		{
			// E.g.: "Position"
			std::string Name;

			// E.g.: float
			VertexElementType Type;

			// E.g.: float32 -> 4
			unsigned TypeSize;

			// E.g.: float3 -> 3
			unsigned Count;

			VertexElement();

			VertexElement(const std::string& name, VertexElementType type,
				unsigned typeSize, unsigned count);

			unsigned GetTotalSize() const;

			bool operator==(const VertexElement& other) const;
			bool operator!=(const VertexElement& other) const;
			bool operator<(const VertexElement& other) const;

			void SerializeSB(Core::ByteVector& bytes) const;
			void DeserializeSB(const unsigned char*& bytes);
		};

		const VertexElement c_PositionVertexElement = { "Position", VertexElementType::Float, sizeof(float), 3 };
		const VertexElement c_TextureCoordinateVertexElement = { "TextureCoordinate", VertexElementType::Float, sizeof(float), 2 };
		const VertexElement c_NormalVertexElement = { "Normal", VertexElementType::Float, sizeof(float), 3 };
		const VertexElement c_TangentVertexElement = { "Tangent", VertexElementType::Float, sizeof(float), 3 };
		const VertexElement c_BitangentVertexElement = { "Bitangent", VertexElementType::Float, sizeof(float), 3 };
		const VertexElement c_VertexColorVertexElement = { "VertexColor", VertexElementType::Float, sizeof(float), 4 };
		const VertexElement c_VertexIndexVertexElement = { "VertexIndex", VertexElementType::Uint32, sizeof(unsigned), 1 };

		struct VertexInputLayout
		{
			std::vector<VertexElement> Elements;

			VertexInputLayout();

			unsigned GetVertexStride() const;
			unsigned GetVertexElementOffset(unsigned index) const;
			unsigned GetVertexElementOffset(const char* name) const;
			unsigned GetVertexElementIndex(const char* name) const;
			const VertexElement& GetVertexElement(const char* name) const;
			
			bool HasVertexElement(const char* name) const;
			bool HasPositions() const;
			bool HasTextureCoordinates() const;
			bool HasNormals() const;
			bool HasVertexColors() const;

			Core::SimpleTypeVectorU<unsigned> GetElementSizes() const;
			Core::SimpleTypeVectorU<unsigned> GetElementOffsets() const;

			void RemoveVertexElement(unsigned index);
			bool RemoveVertexElement(const VertexElement& element);

			static VertexInputLayout Merge(const VertexInputLayout** inputLayouts, unsigned countInputLayouts);

			static bool IsSubset(const VertexInputLayout& superset, const VertexInputLayout& subset);

			bool operator==(const VertexInputLayout& other) const;
			bool operator!=(const VertexInputLayout& other) const;
			bool operator<(const VertexInputLayout& other) const;

			void SerializeSB(Core::ByteVector& bytes) const;
			void DeserializeSB(const unsigned char*& bytes);

		private:

			void RaiseExceptionOnVertexElementNotFound(const std::string& name) const;
		};

		struct Vertex_SOA_Data;

		struct Vertex_AOS_Data
		{
			VertexInputLayout InputLayout;
			Core::ByteVectorU Data;

			void As_SOA_Data(std::vector<Core::ByteVectorU>& resultData) const;
			void As_SOA_Data(unsigned char** resultData) const;
			Vertex_SOA_Data As_SOA_Data() const;

			unsigned GetCountVertices() const;
			unsigned GetSize() const;

			void SerializeSB(Core::ByteVector& bytes) const;
			void DeserializeSB(const unsigned char*& bytes);
		};

		struct Vertex_SOA_Data
		{
			VertexInputLayout InputLayout;
			std::vector<Core::ByteVectorU> Data;

			unsigned GetCountVertices() const;
			unsigned GetSize() const;

			void AddPositionVertexElement(const glm::vec3* positions,
				unsigned countVertices);
			void AddTextureCoordinateVertexElement(const glm::vec2* textureCoordinates,
				unsigned countVertices);
			void AddNormalVertexElement(const glm::vec3* normals,
				unsigned countVertices);
			void AddTangentVertexElement(const glm::vec3* tangents,
				unsigned countVertices);
			void AddBitangentVertexElement(const glm::vec3* bitangents,
				unsigned countVertices);
			void AddVertexColorVertexElement(const glm::vec4* colors,
				unsigned countVertices);
			void AddVertexIndexVertexElement(const unsigned* vertexIndices,
				unsigned countVertices);

			void AddVertexElement(const VertexElement& vertexElement,
				const void* data, unsigned countVertices);
			bool RemoveVertexElement(const VertexElement& vertexElement);

			const glm::vec3* GetPositions() const;
			const glm::vec2* GetTextureCoordinates() const;
			const glm::vec3* GetNormals() const;
			const glm::vec4* GetVertexColors() const;

			template<typename DataType>
			const DataType* GetVertexElements(const char* elementName) const
			{
				unsigned index = InputLayout.GetVertexElementIndex(elementName);
				assert(index != Core::c_InvalidIndexU);
				return reinterpret_cast<const DataType*>(
					Data[index].GetArray());
			}

			// Sets the input layout and creates the attribute arrays.
			void SetInputLayout(const VertexInputLayout& inputLayout);

			void Append(const Vertex_SOA_Data& data);

			void PrepareForAppending(unsigned countNewVertices,
				Core::SimpleTypeVectorU<unsigned char*>& dataArrays, unsigned& baseVertex);

			void As_AOS_Data(Core::ByteVectorU& resultData) const;
			void As_AOS_Data(unsigned char* resultData) const;
			Vertex_AOS_Data As_AOS_Data() const;
		
			void SerializeSB(Core::ByteVector& bytes) const;
			void DeserializeSB(const unsigned char*& bytes);
		};

		enum class PrimitiveTopology : unsigned char
		{
			Undefined,
			PointList,
			LineList,
			LineStrip,
			TriangleList,
			TriangleStrip,
			LineListWithAdjacency,	
			LineStripWithAdjacency,	
			TriangleListWithAdjacency,	
			TriangleStripWithAdjacency,
			ControlPointPatchList_1,
			ControlPointPatchList_2,
			ControlPointPatchList_3,
			ControlPointPatchList_4,
			ControlPointPatchList_5,
			ControlPointPatchList_6,
			ControlPointPatchList_7,
			ControlPointPatchList_8,
			ControlPointPatchList_9,
			ControlPointPatchList_10,
			ControlPointPatchList_11,
			ControlPointPatchList_12,
			ControlPointPatchList_13,
			ControlPointPatchList_14,
			ControlPointPatchList_15,
			ControlPointPatchList_16,
			ControlPointPatchList_17,
			ControlPointPatchList_18,
			ControlPointPatchList_19,
			ControlPointPatchList_20,
			ControlPointPatchList_21,
			ControlPointPatchList_22,
			ControlPointPatchList_23,
			ControlPointPatchList_24,
			ControlPointPatchList_25,
			ControlPointPatchList_26,
			ControlPointPatchList_27,
			ControlPointPatchList_28,
			ControlPointPatchList_29,
			ControlPointPatchList_30,
			ControlPointPatchList_31,
			ControlPointPatchList_32
		};

		struct IndexData
		{
			PrimitiveTopology Topology;
			Core::IndexVectorU Data;

			unsigned GetCountIndices() const;
			unsigned GetSizeInBytes() const;

			void Append(const IndexData& data);

			unsigned* PrepareForAppending(unsigned newIndexCount, unsigned& baseIndex);

			bool operator==(const IndexData& other) const;
			bool operator!=(const IndexData& other) const;

			void SerializeSB(Core::ByteVector& bytes) const;
			void DeserializeSB(const unsigned char*& bytes);
		};
	}
}

#endif
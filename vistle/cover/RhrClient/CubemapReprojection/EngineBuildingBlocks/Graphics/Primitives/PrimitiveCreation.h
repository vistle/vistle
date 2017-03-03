// EngineBuildingBlocks/Graphics/Primitives/PrimitiveCreation.h

#ifndef _ENGINEBUILDINGBLOCKS_PRIMITIVECREATION_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_PRIMITIVECREATION_H_INCLUDED_

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <EngineBuildingBlocks/Graphics/Primitives/Primitive.h>

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		enum class PrimitiveRange
		{
			_0_To_Plus1,
			_Minus1_To_Plus1
		};

		/////////////////////// Direct creation. ///////////////////////

		void CreateQuadGeometry(Vertex_SOA_Data& vertexData,
			IndexData& indexData, PrimitiveRange range);
		void CreateCubeGeometry(Vertex_SOA_Data& vertexData,
			IndexData& indexData);
		void CreateSphereGeometry(Vertex_SOA_Data& vertexData,
			IndexData& indexData,
			unsigned countSlices, unsigned countStacks, bool hasNormals = true);
		void CreateCylinderGeometry(Vertex_SOA_Data& vertexData,
			IndexData& indexData,
			unsigned countSlices, unsigned countStacks, float length);

		/////////////////////// Indirect creation. ///////////////////////
		
		enum class PrimitiveType : unsigned char
		{
			Quad, Cube, Sphere, Cylinder
		};

		struct CodedPrimitiveDescription
		{
			PrimitiveType Type;
			Core::ByteVectorU ParameterData;

			CodedPrimitiveDescription(PrimitiveType type);

			template <typename T>
			CodedPrimitiveDescription(PrimitiveType type, const T& parameters)
				: Type(type)
			{
				ParameterData.PushBack(
					reinterpret_cast<const unsigned char*>(&parameters),
					static_cast<unsigned>(sizeof(T)));
			}

			bool operator==(const CodedPrimitiveDescription& other) const;
			bool operator!=(const CodedPrimitiveDescription& other) const;
			bool operator<(const CodedPrimitiveDescription& other) const;
		};

		void CreatePrimitive(const CodedPrimitiveDescription& description,
				Vertex_SOA_Data& vertexData, IndexData& indexData);

		CodedPrimitiveDescription QuadDescription(
			PrimitiveRange range = PrimitiveRange::_Minus1_To_Plus1);
		CodedPrimitiveDescription CubeDescription();
		CodedPrimitiveDescription SphereDescription(
			unsigned countSlices, unsigned countStacks, bool hasNormals = true);
		CodedPrimitiveDescription CylinderDescription(
			unsigned countSlices, unsigned countStacks, float length);
	}
}

#endif
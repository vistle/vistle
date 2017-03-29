// EngineBuildingBlocks/Graphics/Primitives/PrimitiveCreation.cpp

#include <EngineBuildingBlocks/Graphics/Primitives/PrimitiveCreation.h>

#include <Core/Comparison.h>

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		void CreateQuadGeometry(Vertex_SOA_Data& vertexData, IndexData& indexData, PrimitiveRange range)
		{
			Core::SimpleTypeVectorU<glm::vec3> positions;
			Core::SimpleTypeVectorU<glm::vec2> textureCoordinates;
			Core::SimpleTypeVectorU<glm::vec3> normals;

			auto& indices = indexData.Data;

			if (range == PrimitiveRange::_0_To_Plus1)
			{
				positions = { { 0.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } };
			}
			else
			{
				positions = { { -1.0f, -1.0f, 0.0f },{ 1.0f, -1.0f, 0.0f },{ 1.0f, 1.0f, 0.0f },{ -1.0f, 1.0f, 0.0f } };
			}

			textureCoordinates = { { 0.0f, 0.0f },{ 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f } };
			normals = { { 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } };
			indices = { 0, 1, 2, 0, 2, 3 };

			vertexData.AddPositionVertexElement(positions.GetArray(), positions.GetSize());
			vertexData.AddTextureCoordinateVertexElement(textureCoordinates.GetArray(), textureCoordinates.GetSize());
			vertexData.AddNormalVertexElement(normals.GetArray(), normals.GetSize());
			indexData.Topology = PrimitiveTopology::TriangleList;
		}

		void CreateCubeGeometry(Vertex_SOA_Data& vertexData, IndexData& indexData)
		{
			Core::SimpleTypeVectorU<glm::vec3> positions;
			Core::SimpleTypeVectorU<glm::vec2> textureCoordinates;
			Core::SimpleTypeVectorU<glm::vec3> normals;

			unsigned countVertices = 24;
			positions.Resize(countVertices);
			normals.Resize(countVertices);

			// Positions.
			{
				glm::vec3 xOffset(1.0f, 0.0f, 0.0f);
				glm::vec3 yOffset(0.0f, 1.0f, 0.0f);
				glm::vec3 zOffset(0.0f, 0.0f, 1.0f);

				// Left.
				positions[0] = { -0.5, -0.5f, -0.5f };
				positions[1] = { -0.5f, 0.5f, -0.5f };
				positions[2] = { -0.5f, 0.5f, 0.5f };
				positions[3] = { -0.5f, -0.5f, 0.5f };

				// Bottom.
				positions[8] = { -0.5f, -0.5, -0.5f };
				positions[9] = { 0.5f, -0.5, -0.5f };
				positions[10] = { 0.5f, -0.5, 0.5f };
				positions[11] = { -0.5f, -0.5, 0.5f };

				// Back.
				positions[16] = { -0.5f, -0.5f, -0.5f };
				positions[17] = { 0.5f, -0.5f, -0.5f };
				positions[18] = { 0.5f, 0.5f, -0.5f };
				positions[19] = { -0.5f, 0.5f, -0.5f };

				for (unsigned i = 0; i < 4; i++)
				{
					positions[i + 4] = positions[i] + xOffset;			// Right.
					positions[i + 12] = positions[i + 8] + yOffset;		// Top.
					positions[i + 20] = positions[i + 16] + zOffset;	// Front.
				}
			}

			textureCoordinates =
			{
				{ 0.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f, 1.0f },{ 1.0f, 0.0f },	// Left.
				{ 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f },	// Right.
				{ 0.0f, 0.0f },{ 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f },	// Bottom.
				{ 0.0f, 1.0f },{ 1.0f, 1.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f },	// Top.
				{ 1.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f, 1.0f },	// Back.
				{ 0.0f, 0.0f },{ 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f }		// Front.
			};

			for (int i = 0; i < 4; i++)
			{
				normals[i] = { -1.0f, 0.0f, 0.0f };			// Left.
				normals[i + 4] = { 1.0f, 0.0f, 0.0f };		// Right.
				normals[i + 8] = { 0.0f, -1.0f, 0.0f };		// Bottom.
				normals[i + 12] = { 0.0f, 1.0f, 0.0f };		// Top.
				normals[i + 16] = { 0.0f, 0.0f, -1.0f };	// Back.
				normals[i + 20] = { 0.0f, 0.0f, 1.0f };		// Front.
			}

			indexData.Data =
			{
				1, 0, 2, 2, 0, 3,
				4, 5, 6, 4, 6, 7,
				8, 9, 10, 8, 10, 11,
				13, 12, 14, 14, 12, 15,
				17, 16, 18, 18, 16, 19,
				20, 21, 22, 20, 22, 23,
			};

			vertexData.AddPositionVertexElement(positions.GetArray(), positions.GetSize());
			vertexData.AddTextureCoordinateVertexElement(textureCoordinates.GetArray(), textureCoordinates.GetSize());
			vertexData.AddNormalVertexElement(normals.GetArray(), normals.GetSize());
			indexData.Topology = PrimitiveTopology::TriangleList;
		}

		void CreateSphereGeometry(Vertex_SOA_Data& vertexData, IndexData& indexData,
			unsigned countSlices, unsigned countStacks, bool hasNormals)
		{
			Core::SimpleTypeVectorU<glm::vec3> positions;
			Core::SimpleTypeVectorU<glm::vec2> textureCoordinates;
			Core::SimpleTypeVectorU<glm::vec3> normals;

			unsigned countVertices = (countSlices + 1) * (countStacks + 1);

			positions.Resize(countVertices);
			textureCoordinates.Resize(countVertices);
			if (hasNormals) normals.Resize(countVertices);

			// Setting vertices.
			{
				float phi, theta;
				float dphi = glm::pi<float>() / (float)countStacks;
				float dtheta = 2.0f * glm::pi<float>() / (float)countSlices;
				float x, y, z, sc;
				unsigned index = 0;
				for (unsigned stack = 0; stack <= countStacks; stack++)
				{
					phi = glm::pi<float>() / 2.0f - (float)stack * dphi;
					y = sinf(phi);
					sc = -cosf(phi);
					for (unsigned slice = 0; slice <= countSlices; slice++)
					{
						theta = (float)slice * dtheta;
						x = sc * sinf(theta);
						z = sc * cosf(theta);
						positions[index] = { x, y, z };
						if (hasNormals) normals[index] = { x, y, z };
						textureCoordinates[index] = { (float)slice / (float)countSlices, 1.0f - (float)stack / (float)countStacks };
						index++;
					}
				}
			}

			// Setting indices.
			{
				auto& indices = indexData.Data;
				indices.Resize(countSlices * countStacks * 6);

				unsigned index = 0;
				unsigned k = countSlices + 1;
				for (unsigned stack = 0; stack < countStacks; stack++)
				{
					for (unsigned slice = 0; slice < countSlices; slice++)
					{
						unsigned slice1 = slice + 1;

						indices[index++] = ((stack + 1) * k + slice1);
						indices[index++] = ((stack + 0) * k + slice);
						indices[index++] = ((stack + 1) * k + slice);

						indices[index++] = ((stack + 0) * k + slice);
						indices[index++] = ((stack + 1) * k + slice1);
						indices[index++] = ((stack + 0) * k + slice1);
					}
				}
			}

			vertexData.AddPositionVertexElement(positions.GetArray(), positions.GetSize());
			vertexData.AddTextureCoordinateVertexElement(textureCoordinates.GetArray(), textureCoordinates.GetSize());
			if (normals.GetSize() > 0)
				vertexData.AddNormalVertexElement(normals.GetArray(), normals.GetSize());
			indexData.Topology = PrimitiveTopology::TriangleList;
		}

		void CreateCylinderGeometry(Vertex_SOA_Data& vertexData, IndexData& indexData,
			unsigned countSlices, unsigned countStacks, float length)
		{
			assert(countSlices >= 3 && countStacks >= 2);

			unsigned countVertices = (countSlices + 1) * (countStacks + 2) + 2;
			unsigned countIndices = countSlices * (countStacks + 1) * 2 * 3;

			std::vector<glm::vec3> positions(countVertices);
			std::vector<glm::vec2> texCoords(countVertices);
			std::vector<glm::vec3> normals(countVertices);
			auto& indices = indexData.Data;

			unsigned k = 0;
			for (unsigned i = 0; i < countStacks; i++)
			{
				float v = i / (float)(countStacks - 1);
				float y = length * (v - 0.5f);
				for (unsigned j = 0; j <= countSlices; j++, k++)
				{
					float u = j / (float)(countSlices);
					float angle = u * glm::two_pi<float>();
					float x = cosf(angle);
					float z = -sinf(angle);
					positions[k] = glm::vec3(x, y, z);
					texCoords[k] = glm::vec2(u, v);
					normals[k] = glm::vec3(x, 0.0f, z);
				}
			}

			float y = length * 0.5f;
			for (unsigned j = 0; j <= countSlices; j++, k++)
			{
				float u = j / (float)(countSlices);
				float angle = u * glm::two_pi<float>();
				float x = cosf(angle);
				float z = -sinf(angle);
				positions[k] = glm::vec3(x, y, z);
				texCoords[k] = glm::vec2((x + 1.0f) * 0.5f, (-z + 1.0f) * 0.5f);
				normals[k] = glm::vec3(0.0f, 1.0f, 0.0f);
			}
			positions[k] = glm::vec3(0.0f, y, 0.0f);
			texCoords[k] = glm::vec2(0.5f, 0.5f);
			normals[k] = glm::vec3(0.0f, 1.0f, 0.0f);
			k++;
			y *= -1.0f;
			for (unsigned j = 0; j <= countSlices; j++, k++)
			{
				float u = j / (float)(countSlices);
				float angle = u * glm::two_pi<float>();
				float x = cosf(angle);
				float z = -sinf(angle);
				positions[k] = glm::vec3(x, y, z);
				texCoords[k] = glm::vec2((x + 1.0f) * 0.5f, (-z + 1.0f) * 0.5f);
				normals[k] = glm::vec3(0.0f, -1.0f, 0.0f);
			}
			positions[k] = glm::vec3(0.0f, y, 0.0f);
			texCoords[k] = glm::vec2(0.5f, 0.5f);
			normals[k] = glm::vec3(0.0f, -1.0f, 0.0f);

			indices.Resize(countIndices);
			k = 0;
			unsigned levelOffset = countSlices + 1;
			for (unsigned i = 0; i < countStacks - 1; i++)
			{
				unsigned baseIndex = levelOffset * i;
				for (unsigned j = 0; j < countSlices; j++)
				{
					unsigned i0 = baseIndex + j;
					unsigned i1 = i0 + 1;
					unsigned i2 = i0 + levelOffset;
					unsigned i3 = i1 + levelOffset;
					indices[k++] = i0; indices[k++] = i1; indices[k++] = i2;
					indices[k++] = i2; indices[k++] = i1; indices[k++] = i3;
				}
			}
			unsigned baseIndex = levelOffset * countStacks;
			for (unsigned j = 0; j < countSlices; j++)
			{
				indices[k++] = baseIndex + j; indices[k++] = baseIndex + j + 1; indices[k++] = baseIndex + levelOffset;
			}
			baseIndex += countSlices + 2;
			for (unsigned j = 0; j < countSlices; j++)
			{
				indices[k++] = baseIndex + j + 1; indices[k++] = baseIndex + j; indices[k++] = baseIndex + levelOffset;
			}

			vertexData.AddPositionVertexElement(positions.data(), countVertices);
			vertexData.AddTextureCoordinateVertexElement(texCoords.data(), countVertices);
			vertexData.AddNormalVertexElement(normals.data(), countVertices);
			indexData.Topology = PrimitiveTopology::TriangleList;
		}

		///////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////

		CodedPrimitiveDescription::CodedPrimitiveDescription(PrimitiveType type)
			: Type(type)
		{
		}

		bool CodedPrimitiveDescription::operator==(const CodedPrimitiveDescription& other) const
		{
			NumericalEqualCompareBlock(Type);
			StructureEqualCompareBlock(ParameterData);
			return true;
		}

		bool CodedPrimitiveDescription::operator!=(const CodedPrimitiveDescription& other) const
		{
			return !(*this == other);
		}

		bool CodedPrimitiveDescription::operator<(const CodedPrimitiveDescription& other) const
		{
			NumericalLessCompareBlock(Type);
			StructureLessCompareBlock(ParameterData);
			return false;
		}

		template <typename T>
		const T& GetParameters(const CodedPrimitiveDescription& description)
		{
			return *reinterpret_cast<const T*>(description.ParameterData.GetArray());
		}

		struct QuadParameters
		{
			PrimitiveRange Range;
		};

		struct SphereParameters
		{
			unsigned CountSlices, CountStacks;
			bool HasNormals;
		};

		struct CylinderParameters
		{
			unsigned CountSlices, CountStacks;
			float Length;
		};

		void CreatePrimitive(const CodedPrimitiveDescription& description, Vertex_SOA_Data& vertexData, IndexData& indexData)
		{
			switch (description.Type)
			{
			case PrimitiveType::Quad:
				CreateQuadGeometry(vertexData, indexData, GetParameters<QuadParameters>(description).Range); break;
			case PrimitiveType::Cube:
				CreateCubeGeometry(vertexData, indexData); break;
			case PrimitiveType::Sphere:
			{
				auto& parameters = GetParameters<SphereParameters>(description);
				CreateSphereGeometry(vertexData, indexData, parameters.CountSlices, parameters.CountStacks,
					parameters.HasNormals); break;
			}
			case PrimitiveType::Cylinder:
			{
				auto& parameters = GetParameters<CylinderParameters>(description);
				CreateCylinderGeometry(vertexData, indexData, parameters.CountSlices, parameters.CountStacks,
					parameters.Length); break;
			}
			}
		}

		CodedPrimitiveDescription QuadDescription(PrimitiveRange range)
		{
			return{ PrimitiveType::Quad, QuadParameters{ range } };
		}

		CodedPrimitiveDescription CubeDescription()
		{
			return{ PrimitiveType::Cube };
		}

		CodedPrimitiveDescription SphereDescription(unsigned countSlices, unsigned countStacks, bool hasNormals)
		{
			return{ PrimitiveType::Sphere, SphereParameters{ countSlices, countStacks, hasNormals } };
		}

		CodedPrimitiveDescription CylinderDescription(unsigned countSlices, unsigned countStacks, float length)
		{
			return{ PrimitiveType::Cylinder, CylinderParameters{ countSlices, countStacks, length } };
		}
	}
}
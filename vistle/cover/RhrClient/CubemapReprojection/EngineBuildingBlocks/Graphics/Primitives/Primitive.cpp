// EngineBuildingBlocks/Graphics/Primitives/Primitive.h

#include <EngineBuildingBlocks/Graphics/Primitives/Primitive.h>

#include <Core/Comparison.h>
#include <Core/Constants.h>
#include <EngineBuildingBlocks/ErrorHandling.h>

#include <algorithm>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

VertexElement::VertexElement()
	: Type(VertexElementType::Unknown)
	, TypeSize(0)
	, Count(0)
{
}

VertexElement::VertexElement(const std::string& name, VertexElementType type, unsigned typeSize,
	unsigned count)
	: Name(name)
	, Type(type)
	, TypeSize(typeSize)
	, Count(count)
{
}

unsigned VertexElement::GetTotalSize() const
{
	return Count * TypeSize;
}

bool VertexElement::operator==(const VertexElement& other) const
{
	StringEqualCompareBlock(Name);
	NumericalEqualCompareBlock(Type);
	NumericalEqualCompareBlock(TypeSize);
	NumericalEqualCompareBlock(Count);
	return true;
}

bool VertexElement::operator!=(const VertexElement& other) const
{
	return !(*this == other);
}

bool VertexElement::operator<(const VertexElement& other) const
{
	StringLessCompareBlock(Name);
	NumericalLessCompareBlock(Type);
	NumericalLessCompareBlock(TypeSize);
	NumericalLessCompareBlock(Count);
	return false;
}

void VertexElement::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Name);
	Core::SerializeSB(bytes, Type);
	Core::SerializeSB(bytes, TypeSize);
	Core::SerializeSB(bytes, Count);
}

void VertexElement::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Name);
	Core::DeserializeSB(bytes, Type);
	Core::DeserializeSB(bytes, TypeSize);
	Core::DeserializeSB(bytes, Count);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

VertexInputLayout::VertexInputLayout()
{
}

unsigned VertexInputLayout::GetVertexStride() const
{
	unsigned size = 0;
	for (size_t i = 0; i < Elements.size(); i++)
	{
		size += Elements[i].GetTotalSize();
	}
	return size;
}

unsigned VertexInputLayout::GetVertexElementOffset(unsigned index) const
{
	unsigned size = 0;
	for (unsigned i = 0; i < index; i++)
	{
		size += Elements[i].GetTotalSize();
	}
	return size;
}

unsigned VertexInputLayout::GetVertexElementOffset(const char* name) const
{
	unsigned size = 0;
	for (size_t i = 0; i < Elements.size(); i++)
	{
		if (Elements[i].Name == name)
		{
			break;
		}
		size += Elements[i].GetTotalSize();
	}
	return size;
}

unsigned VertexInputLayout::GetVertexElementIndex(const char* name) const
{
	for (size_t i = 0; i < Elements.size(); i++)
	{
		if (Elements[i].Name == name)
		{
			return static_cast<unsigned>(i);
		}
	}
	return Core::c_InvalidIndexU;
}

const VertexElement& VertexInputLayout::GetVertexElement(const char* name) const
{
	for (size_t i = 0; i < Elements.size(); i++)
	{
		if (Elements[i].Name == name)
		{
			return Elements[i];
		}
	}

	RaiseExceptionOnVertexElementNotFound(name);

	// This code is never executed, but keeps the compiler happy.
	return Elements[0];
}

bool VertexInputLayout::HasVertexElement(const char* name) const
{
	return (GetVertexElementIndex(name) != Core::c_InvalidIndexU);
}

bool VertexInputLayout::HasPositions() const
{
	return HasVertexElement(c_PositionVertexElement.Name.c_str());
}

bool VertexInputLayout::HasTextureCoordinates() const
{
	return HasVertexElement(c_TextureCoordinateVertexElement.Name.c_str());
}

bool VertexInputLayout::HasNormals() const
{
	return HasVertexElement(c_NormalVertexElement.Name.c_str());
}

bool VertexInputLayout::HasVertexColors() const
{
	return HasVertexElement(c_VertexColorVertexElement.Name.c_str());
}

void VertexInputLayout::RaiseExceptionOnVertexElementNotFound(const std::string& name) const
{
	EngineBuildingBlocks::RaiseException("Vertex element was not found: '" + name + "'.");
}

Core::SimpleTypeVectorU<unsigned> VertexInputLayout::GetElementSizes() const
{
	unsigned countElements = static_cast<unsigned>(Elements.size());
	Core::SimpleTypeVectorU<unsigned> sizes(countElements);
	for (unsigned i = 0; i < countElements; i++)
	{
		sizes[i] = Elements[i].GetTotalSize();
	}
	return sizes;
}

Core::SimpleTypeVectorU<unsigned> VertexInputLayout::GetElementOffsets() const
{
	unsigned countElements = static_cast<unsigned>(Elements.size());
	Core::SimpleTypeVectorU<unsigned> offsets(countElements);
	unsigned offset = 0;
	for (unsigned i = 0; i < countElements; i++)
	{
		offsets[i] = offset;
		offset += Elements[i].GetTotalSize();
	}
	return offsets;
}

void VertexInputLayout::RemoveVertexElement(unsigned index)
{
	Elements.erase(Elements.begin() + index);
}

bool VertexInputLayout::RemoveVertexElement(const VertexElement& element)
{
	auto index = GetVertexElementIndex(element.Name.c_str());
	if (index == Core::c_InvalidIndexU) return false;
	assert(Elements[index] == element);
	RemoveVertexElement(index);
	return true;
}

VertexInputLayout VertexInputLayout::Merge(const VertexInputLayout** inputLayouts, unsigned countInputLayouts)
{
	VertexInputLayout output;
	auto& outputElements = output.Elements;
	for (unsigned i = 0; i < countInputLayouts; i++)
	{
		auto& inputElements = inputLayouts[i]->Elements;
		outputElements.insert(outputElements.end(), inputElements.begin(), inputElements.end());
	}
	return output;
}

bool VertexInputLayout::IsSubset(const VertexInputLayout& superset, const VertexInputLayout& subset)
{
	auto count = static_cast<unsigned>(subset.Elements.size());
	for (unsigned i = 0; i < count; i++)
	{
		unsigned j = superset.GetVertexElementIndex(subset.Elements[i].Name.c_str());
		if (j == Core::c_InvalidIndexU || subset.Elements[i] != superset.Elements[j]) return false;
	}
	return true;
}

bool VertexInputLayout::operator==(const VertexInputLayout& other) const
{
	StructureEqualCompareBlock(Elements);
	return true;
}

bool VertexInputLayout::operator!=(const VertexInputLayout& other) const
{
	return !(*this == other);
}

bool VertexInputLayout::operator<(const VertexInputLayout& other) const
{
	StructureLessCompareBlock(Elements);
	return false;
}

void VertexInputLayout::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Elements);
}

void VertexInputLayout::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Elements);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void  Vertex_AOS_Data::As_SOA_Data(std::vector<Core::ByteVectorU>& resultData) const
{
	auto countElements = static_cast<unsigned>(InputLayout.Elements.size());
	auto sizes = InputLayout.GetElementSizes();
	unsigned countVertices = GetCountVertices();

	Core::SimpleTypeVectorU<unsigned char*> targetPtrs(countElements);
	resultData.resize(countElements);
	for (unsigned i = 0; i < countElements; i++)
	{
		auto& targetArray = resultData[i];
		targetArray.Resize(countVertices * sizes[i]);
		targetPtrs[i] = targetArray.GetArray();
	}

	As_SOA_Data(targetPtrs.GetArray());
}

void Vertex_AOS_Data::As_SOA_Data(unsigned char** resultData) const
{
	auto& elements = InputLayout.Elements;
	auto countElements = static_cast<unsigned>(elements.size());
	auto sizes = InputLayout.GetElementSizes();
	unsigned countVertices = GetCountVertices();

	Core::SimpleTypeVectorU<unsigned char*> targetPtrs;
	targetPtrs.PushBack(resultData, countElements);

	const unsigned char* sourcePtr = this->Data.GetArray();
	for (unsigned i = 0; i < countVertices; i++)
	{
		for (unsigned j = 0; j < countElements; j++)
		{
			unsigned size = sizes[j];
			memcpy(targetPtrs[j], sourcePtr, size);
			sourcePtr += size;
			targetPtrs[j] += size;
		}
	}
}

Vertex_SOA_Data Vertex_AOS_Data::As_SOA_Data() const
{
	Vertex_SOA_Data result;
	result.InputLayout = InputLayout;
	As_SOA_Data(result.Data);
	return result;
}

unsigned Vertex_AOS_Data::GetCountVertices() const
{
	return Data.GetSize() / InputLayout.GetVertexStride();
}

unsigned Vertex_AOS_Data::GetSize() const
{
	return Data.GetSize();
}

void Vertex_AOS_Data::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, InputLayout);
	Core::SerializeSB(bytes, Data);
}

void Vertex_AOS_Data::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, InputLayout);
	Core::DeserializeSB(bytes, Data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned Vertex_SOA_Data::GetCountVertices() const
{
	return (Data.size() == 0 ? 0 : Data[0].GetSize() / InputLayout.Elements[0].GetTotalSize());
}

unsigned Vertex_SOA_Data::GetSize() const
{
	unsigned size = 0;
	for (size_t i = 0; i < Data.size(); i++)
	{
		size += Data[i].GetSize();
	}
	return size;
}

void Vertex_SOA_Data::AddPositionVertexElement(const glm::vec3* positions,
	unsigned countVertices)
{
	AddVertexElement(c_PositionVertexElement, positions, countVertices);
}

void Vertex_SOA_Data::AddTextureCoordinateVertexElement(const glm::vec2* textureCoordinates,
	unsigned countVertices)
{
	AddVertexElement(c_TextureCoordinateVertexElement, textureCoordinates, countVertices);
}

void Vertex_SOA_Data::AddNormalVertexElement(const glm::vec3* normals,
	unsigned countVertices)
{
	AddVertexElement(c_NormalVertexElement, normals, countVertices);
}

void Vertex_SOA_Data::AddTangentVertexElement(const glm::vec3* tangents, unsigned countVertices)
{
	AddVertexElement(c_TangentVertexElement, tangents, countVertices);
}

void Vertex_SOA_Data::AddBitangentVertexElement(const glm::vec3* bitangents, unsigned countVertices)
{
	AddVertexElement(c_BitangentVertexElement, bitangents, countVertices);
}

void Vertex_SOA_Data::AddVertexColorVertexElement(const glm::vec4* colors, unsigned countVertices)
{
	AddVertexElement(c_VertexColorVertexElement, colors, countVertices);
}

void Vertex_SOA_Data::AddVertexIndexVertexElement(const unsigned* vertexIndices, unsigned countVertices)
{
	AddVertexElement(c_VertexIndexVertexElement, vertexIndices, countVertices);
}

void Vertex_SOA_Data::AddVertexElement(const VertexElement& vertexElement, const void* data,
	unsigned countVertices)
{
	assert(GetCountVertices() == 0 || GetCountVertices() == countVertices);

	unsigned vertexElementSize = vertexElement.GetTotalSize();
	unsigned dataSize = countVertices * vertexElementSize;

	this->Data.emplace_back();

	if (dataSize != 0)
	{
		auto& newData = this->Data.back();
		newData.Resize(dataSize);
		memcpy(newData.GetArray(), data, dataSize);
	}

	this->InputLayout.Elements.push_back(vertexElement);
}

bool Vertex_SOA_Data::RemoveVertexElement(const VertexElement& vertexElement)
{
	unsigned index = InputLayout.GetVertexElementIndex(vertexElement.Name.c_str());
	if (index == Core::c_InvalidIndexU) return false;
	assert(InputLayout.Elements[index] == vertexElement);
	InputLayout.RemoveVertexElement(index);
	Data.erase(Data.begin() + index);
	return true;
}

const glm::vec3* Vertex_SOA_Data::GetPositions() const
{
	return GetVertexElements<glm::vec3>(c_PositionVertexElement.Name.c_str());
}

const glm::vec2* Vertex_SOA_Data::GetTextureCoordinates() const
{
	return GetVertexElements<glm::vec2>(c_TextureCoordinateVertexElement.Name.c_str());
}

const glm::vec3* Vertex_SOA_Data::GetNormals() const
{
	return GetVertexElements<glm::vec3>(c_NormalVertexElement.Name.c_str());
}

const glm::vec4* Vertex_SOA_Data::GetVertexColors() const
{
	return GetVertexElements<glm::vec4>(c_VertexColorVertexElement.Name.c_str());
}

void Vertex_SOA_Data::SetInputLayout(const VertexInputLayout& inputLayout)
{
	assert(Data.size() == 0);
	InputLayout = inputLayout;
	Data.resize(inputLayout.Elements.size());
}

void Vertex_SOA_Data::Append(const Vertex_SOA_Data& data)
{
	unsigned countVertexElements = static_cast<unsigned>(InputLayout.Elements.size());
	for (unsigned i = 0; i < countVertexElements; i++)
	{
		Data[i].PushBack(data.Data[data.InputLayout.GetVertexElementIndex(InputLayout.Elements[i].Name.c_str())]);
	}
}

void Vertex_SOA_Data::PrepareForAppending(unsigned countNewVertices,
	Core::SimpleTypeVectorU<unsigned char*>& dataArrays, unsigned& baseVertex)
{
	baseVertex = GetCountVertices();
	unsigned newCount = baseVertex + countNewVertices;

	auto& elements = InputLayout.Elements;
	unsigned countVertexElements = static_cast<unsigned>(InputLayout.Elements.size());

	if (countVertexElements == 0)
	{
		EngineBuildingBlocks::RaiseException("Cannot append to an empty vertex buffer.");
	}

	dataArrays.Resize(countVertexElements);

	unsigned newSize = 0;
	for (unsigned i = 0; i < countVertexElements; i++)
	{
		auto& currentData = this->Data[i];
		unsigned vertexElementSize = elements[i].GetTotalSize();
		unsigned arraySize = newCount * vertexElementSize;
		newSize += arraySize;
		currentData.ResizeWithGrowing(arraySize);

		dataArrays[i] = currentData.GetArray() + baseVertex * vertexElementSize;
	}
}

Vertex_AOS_Data Vertex_SOA_Data::As_AOS_Data() const
{
	Vertex_AOS_Data result;
	result.InputLayout = InputLayout;
	As_AOS_Data(result.Data);
	return result;
}

void Vertex_SOA_Data::As_AOS_Data(Core::ByteVectorU& resultData) const
{
	resultData.Resize(GetSize());
	As_AOS_Data(resultData.GetArray());
}

void Vertex_SOA_Data::As_AOS_Data(unsigned char* resultData) const
{
	auto& elements = InputLayout.Elements;
	unsigned countElements = static_cast<unsigned>(elements.size());
	auto sizes = InputLayout.GetElementSizes();
	unsigned countVertices = GetCountVertices();

	Core::SimpleTypeVectorU<const unsigned char*> sourcePtrs(countElements);
	for (unsigned i = 0; i < countElements; i++)
	{
		sourcePtrs[i] = this->Data[i].GetArray();
	}

	unsigned char* targetPtr = resultData;
	for (unsigned i = 0; i < countVertices; i++)
	{
		for (unsigned j = 0; j < countElements; j++)
		{
			unsigned size = sizes[j];
			memcpy(targetPtr, sourcePtrs[j], size);
			sourcePtrs[j] += size;
			targetPtr += size;
		}
	}
}

void Vertex_SOA_Data::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, InputLayout);
	Core::SerializeSB(bytes, Data);
}

void Vertex_SOA_Data::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, InputLayout);
	Core::DeserializeSB(bytes, Data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned IndexData::GetCountIndices() const
{
	return Data.GetSize();
}

unsigned IndexData::GetSizeInBytes() const
{
	return Data.GetSize() * static_cast<unsigned>(sizeof(unsigned));
}

void IndexData::Append(const IndexData& data)
{
	if (data.Topology != Topology) RaiseException("Indiceses with different primitive topologies cannot be appended.");
	Data.PushBack(data.Data);
}

unsigned* IndexData::PrepareForAppending(unsigned newIndexCount, unsigned& baseIndex)
{
	baseIndex = Data.GetSize();
	Data.ResizeWithGrowing(baseIndex + newIndexCount);
	return Data.GetArray() + baseIndex;
}

bool IndexData::operator==(const IndexData& other) const
{
	NumericalEqualCompareBlock(Topology);
	StructureEqualCompareBlock(Data);
	return true;
}

bool IndexData::operator!=(const IndexData& other) const
{
	return !(*this == other);
}

void IndexData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Topology);
	Core::SerializeSB(bytes, Data);
}

void IndexData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Topology);
	Core::DeserializeSB(bytes, Data);
}
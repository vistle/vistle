// EngineBuildingBlocks/Math/GLMSerialization.h

#ifndef _ENGINEBUILDINGBLOCKS_GLMSERIALIZATION_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_GLMSERIALIZATION_H_INCLUDED_

#include <EngineBuildingBlocks/Math/GLM.h>

#include <Core/SimpleXMLSerialization.hpp>
#include <Core/Parse.hpp>
#include <Core/String.hpp>

namespace EngineBuildingBlocks
{
	template <typename T>
	inline void SerializeAsArraySXML(Core::SerializationSXMLData& data, const T* v, size_t size)
	{
		auto& ss = *data.SS;
		size_t limit = size - 1;
		for (size_t i = 0; i < size; i++)
		{
			ss << v[i];
			if (i < limit) ss << ",";
		}
		Core::SetXMLNodeValue(data.Document, data.Node, ss.str().c_str());
		ss.str("");
	}

	template <typename T>
	inline void DeserializeAsArraySXML(Core::DeserializationSXMLData& data, T* v, size_t size)
	{
		data.PlaceNodeValue();
		auto parts = Core::Split(data.Chars->GetArray(), Core::IsCharacter(','), true);
		assert(parts.size() == size);
		for (unsigned i = 0; i < 3; i++) v[i] = Core::Parse<T>(parts[i].c_str());
	}

	template <typename T, glm::precision P>
	inline void SerializeSXML(Core::SerializationSXMLData& data, const glm::tvec2<T, P>& v)
	{
		SerializeAsArraySXML(data, glm::value_ptr(v), 2);
	}

	template <typename T, glm::precision P>
	inline void SerializeSXML(Core::SerializationSXMLData& data, const glm::tvec3<T, P>& v)
	{
		SerializeAsArraySXML(data, glm::value_ptr(v), 3);
	}

	template <typename T, glm::precision P>
	inline void SerializeSXML(Core::SerializationSXMLData& data, const glm::tvec4<T, P>& v)
	{
		SerializeAsArraySXML(data, glm::value_ptr(v), 4);
	}

	template <typename T, glm::precision P>
	inline void SerializeSXML(Core::SerializationSXMLData& data, const glm::tquat<T, P>& q)
	{
		SerializeAsArraySXML(data, glm::value_ptr(q), 4);
	}

	template <typename T, glm::precision P>
	inline void SerializeSXML(Core::SerializationSXMLData& data, const glm::tmat3x3<T, P>& m)
	{
		SerializeAsArraySXML(data, glm::value_ptr(m), 9);
	}

	template <typename T, glm::precision P>
	inline void DeserializeSXML(Core::DeserializationSXMLData& data, glm::tvec2<T, P>& v)
	{
		DeserializeAsArraySXML(data, glm::value_ptr(v), 2);
	}

	template <typename T, glm::precision P>
	inline void DeserializeSXML(Core::DeserializationSXMLData& data, glm::tvec3<T, P>& v)
	{
		DeserializeAsArraySXML(data, glm::value_ptr(v), 3);
	}

	template <typename T, glm::precision P>
	inline void DeserializeSXML(Core::DeserializationSXMLData& data, glm::tvec4<T, P>& v)
	{
		DeserializeAsArraySXML(data, glm::value_ptr(v), 4);
	}

	template <typename T, glm::precision P>
	inline void DeserializeSXML(Core::DeserializationSXMLData& data, glm::tquat<T, P>& q)
	{
		DeserializeAsArraySXML(data, glm::value_ptr(q), 4);
	}

	template <typename T, glm::precision P>
	inline void DeserializeSXML(Core::DeserializationSXMLData& data, glm::tmat3x3<T, P>& m)
	{
		DeserializeAsArraySXML(data, glm::value_ptr(m), 9);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	inline void SerializeSXML(Core::SerializationSXMLData& data, const T& object, const char* objectName)
	{
		auto newData = Core::CreateXMLNode(data, objectName);
		EngineBuildingBlocks::SerializeSXML(newData, object);
	}

	template <typename T>
	inline void DeserializeSXML(Core::DeserializationSXMLData& data, T& object, const char* objectName)
	{
		auto newData = Core::VisitChildXMLNode(data, objectName);
		EngineBuildingBlocks::DeserializeSXML(newData, object);
	}
}

#define EBB_SerializeSXML(data, variable) EngineBuildingBlocks::SerializeSXML(data, variable, #variable)
#define EBB_DeserializeSXML(data, variable) EngineBuildingBlocks::DeserializeSXML(data, variable, #variable)

#endif
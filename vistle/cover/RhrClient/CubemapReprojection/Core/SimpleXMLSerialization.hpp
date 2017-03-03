// Core/SimpleXMLSerialization.hpp

#ifndef _CORE_SIMPLEXMLSERIALIZATION_HPP_
#define _CORE_SIMPLEXMLSERIALIZATION_HPP_

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/Parse.hpp>
#include <Core/String.hpp>
#include <Core/StringStreamHelper.h>

#include <External/rapidxml/rapidxml.hpp>
#include <External/rapidxml/rapidxml_print.hpp>
#include <External/Encoding/Base64Encoding.h>

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include <cstdlib>
#include <type_traits>

namespace Core
{
	struct SerializationSXMLData
	{
		rapidxml::xml_document<>* Document;
		rapidxml::xml_node<>* Node;
		std::stringstream* SS;
	};

	struct DeserializationSXMLData
	{
		rapidxml::xml_node<>* Node;
		Core::SimpleTypeVector<char>* Chars;

	private:

		inline void Place(const char* source, size_t size)
		{
			Chars->Resize(size + 1);
			memcpy(Chars->GetArray(), source, size);
			(*Chars)[size] = 0;
		}

	public:

		inline void PlaceNodeName()
		{
			Place(Node->name(), Node->name_size());
		}

		inline void PlaceNodeValue()
		{
			Place(Node->value(), Node->value_size());
		}

		inline void PlaceAttributeValue(const char* name)
		{
			auto attribute = Node->first_attribute(name);
			Place(attribute->value(), attribute->value_size());
		}
	};

	inline std::string ToBinaryText(const unsigned char* data, size_t size)
	{
		return base64_encode(data, size);
	}

	inline void ToBinaryData(const std::string& str, unsigned char*& data, size_t& size)
	{
		base64_decode(str.data(), str.length(), &data, &size);
	}

#if defined(TARGET_PLATFORM_X64)
	inline void ToBinaryData(const std::string& str, unsigned char*& data, unsigned& size)
	{
		size_t sSize;
		ToBinaryData(str, data, sSize);
		size = static_cast<unsigned>(sSize);
	}
#endif

	struct DeserializationBinaryDataWrapper { unsigned char** PData; size_t* PSize; };

	template <typename T>
	inline DeserializationBinaryDataWrapper AsBinaryData(T** pData, size_t* pSize)
	{
		return { pData, pSize };
	}

	inline rapidxml::xml_node<>* CreateXMLNode(rapidxml::xml_document<>* pDocument, rapidxml::xml_node<>* pNode,
		const char* name, const char* value = nullptr)
	{
		auto nodeName = pDocument->allocate_string(name);
		auto nodeValue = (value == nullptr? nullptr : pDocument->allocate_string(value));
		auto newNode = pDocument->allocate_node(rapidxml::node_type::node_element, nodeName, nodeValue);
		pNode->append_node(newNode);
		return newNode;
	}

	inline SerializationSXMLData CreateXMLNode(SerializationSXMLData& data, const char* name, const char* value = nullptr)
	{
		SerializationSXMLData newData;
		newData.Document = data.Document;
		newData.Node = CreateXMLNode(data.Document, data.Node, name, value);
		newData.SS = data.SS;
		return newData;
	}

	inline void SetXMLNodeValue(rapidxml::xml_document<>* pDocument, rapidxml::xml_node<>* pNode, const char* value)
	{
		auto nodeValue = (value == nullptr ? nullptr : pDocument->allocate_string(value));
		pNode->value(nodeValue, strlen(value));
	}

	inline rapidxml::xml_attribute<>* AddXMLAttribute(rapidxml::xml_document<>* pDocument, rapidxml::xml_node<>* pNode,
		const char* name, const char* value)
	{
		auto attributeName = pDocument->allocate_string(name);
		auto attributeValue = pDocument->allocate_string(value);
		auto attribute = pDocument->allocate_attribute(attributeName, attributeValue);
		pNode->append_attribute(attribute);
		return attribute;
	}

	template <typename T>
	inline void AddXMLAttribute(SerializationSXMLData& data, const char* name, const T& value)
	{
		*data.SS << value;
		AddXMLAttribute(data.Document, data.Node, name, data.SS->str().c_str());
		data.SS->str("");
	}

	inline DeserializationSXMLData VisitChildXMLNode(DeserializationSXMLData& data, const char* childNodeName)
	{
		return { data.Node->first_node(childNodeName), data.Chars };
	}
	
	namespace detail
	{
		template <typename T, bool IsClass>
		struct SerializerSXML
		{
			static inline void SerializeSXML(Core::SerializationSXMLData& data, const T& object)
			{
				object.SerializeSXML(data);
			}
		};

		template <typename T, bool IsClass>
		struct DeserializerSXML
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, T& object)
			{
				object.DeserializeSXML(data);
			}
		};

		template <typename T>
		inline void _SerializeSXML(Core::SerializationSXMLData& data, const T& object, const char* objectName)
		{
			auto newData = Core::CreateXMLNode(data, objectName);
			SerializerSXML<T, std::is_class<T>::value>::SerializeSXML(newData, object);
		}

		template <typename T>
		inline void _DeserializeSXMLDirectly(Core::DeserializationSXMLData& data, T& object)
		{
			DeserializerSXML<T, std::is_class<T>::value>::DeserializeSXML(data, object);
		}

		template <typename T>
		inline void _DeserializeSXML(Core::DeserializationSXMLData& data, T& object, const char* objectName)
		{
			auto newData = Core::VisitChildXMLNode(data, objectName);
			_DeserializeSXMLDirectly(newData, object);
		}

		template <typename T, bool IsElementClass>
		struct ArraySerializerSXML
		{
		};

		template <typename T>
		struct ArraySerializerSXML<T, false>
		{
			static inline void SerializeSXML(Core::SerializationSXMLData& data, const T* array, size_t size)
			{
				for (size_t i = 0; i < size; i++)
				{
					*data.SS << array[i];
					if (i + 1 < size) *data.SS << ";";
				}
				Core::SetXMLNodeValue(data.Document, data.Node, data.SS->str().c_str());
				data.SS->str("");
			}
		};

		template <typename T>
		struct ArraySerializerSXML<T, true>
		{
			static inline void SerializeSXML(Core::SerializationSXMLData& data, const T* array, size_t size)
			{
				for (size_t i = 0; i < size; i++)
				{
					_SerializeSXML(data, array[i], "ael");
				}
			}
		};

		template <typename T>
		inline void SerializeArraySXML(Core::SerializationSXMLData& data, const T* array, size_t size)
		{
			ArraySerializerSXML<T, std::is_class<T>::value>::SerializeSXML(data, array, size);
		}

		template <typename T>
		struct SerializerSXML<T, false>
		{
			static inline void SerializeSXML(Core::SerializationSXMLData& data, const T& object)
			{
				*data.SS << object;
				SetXMLNodeValue(data.Document, data.Node, data.SS->str().c_str());
				data.SS->str("");
			}
		};

		template <typename T>
		struct DeserializerSXML<T, false>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, T& object)
			{
				data.PlaceNodeValue();
				object = Core::Parse<T>(data.Chars->GetArray());
			}
		};

		template <typename T, typename SizeType, typename AllocatorType>
		struct SerializerSXML<SimpleTypeVector<T, SizeType, AllocatorType>, true>
		{
			static inline void SerializeSXML(Core::SerializationSXMLData& data, const SimpleTypeVector<T, SizeType, AllocatorType>& v)
			{
				SerializeArraySXML(data, v.GetArray(), static_cast<size_t>(v.GetSize()));
			}
		};

		template <typename T, typename SizeType, typename AllocatorType, bool IsElementClass>
		struct SimpleTypeVectorDeserializerSXML
		{
		};

		template <typename T, typename SizeType, typename AllocatorType>
		struct SimpleTypeVectorDeserializerSXML<T, SizeType, AllocatorType, false>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, SimpleTypeVector<T, SizeType, AllocatorType>& v)
			{
				data.PlaceNodeValue();
				auto parts = Core::Split(std::string(data.Chars), Core::IsCharacter(';'), true);
				auto countParts = parts.size();
				v.Resize(static_cast<SizeType>(countParts));
				for (size_t i = 0; i < countParts; i++)
				{
					v[i] = Core::Parse<T>(parts[i].c_str());
				}
			}
		};

		template <typename T, typename SizeType, typename AllocatorType>
		struct SimpleTypeVectorDeserializerSXML<T, SizeType, AllocatorType, true>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, SimpleTypeVector<T, SizeType, AllocatorType>& v)
			{
				for (auto cNode = data.Node->first_node(); cNode != nullptr; cNode = cNode->next_sibling())
				{
					_DeserializeSXMLDirectly(Core::DeserializationSXMLData{ cNode, data.Chars }, v.PushBackPlaceHolder());
				}
			}
		};

		template <typename T, typename SizeType, typename AllocatorType>
		struct DeserializerSXML<SimpleTypeVector<T, SizeType, AllocatorType>, true>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, SimpleTypeVector<T, SizeType, AllocatorType>& v)
			{
				SimpleTypeVectorDeserializerSXML<T, SizeType, AllocatorType, std::is_class<T>::value>::DeserializeSXML(data, v);
			}
		};

		template <typename T>
		struct SerializerSXML<std::vector<T>, true>
		{
			static inline void SerializeSXML(Core::SerializationSXMLData& data, const std::vector<T>& v)
			{
				SerializeArraySXML(data, v.data(), v.size());
			}
		};

		template <typename T, bool IsElementClass>
		struct VectorDeserializerSXML
		{
		};

		template <typename T>
		struct VectorDeserializerSXML<T, false>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, std::vector<T>& v)
			{
				data.PlaceNodeValue();
				auto parts = Core::Split(data.Chars->GetArray(), data.Chars->GetSize() - 1,
					Core::IsCharacter(';'), true);
				auto countParts = parts.size();
				v.reserve(countParts);
				for (size_t i = 0; i < countParts; i++)
				{
					v.emplace_back(Core::Parse<T>(parts[i].c_str()));
				}
			}
		};

		template <typename T>
		struct VectorDeserializerSXML<T, true>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, std::vector<T>& v)
			{
				for (auto cNode = data.Node->first_node(); cNode != nullptr; cNode = cNode->next_sibling())
				{
					v.emplace_back();
					_DeserializeSXMLDirectly(Core::DeserializationSXMLData{ cNode, data.Chars }, v.back());
				}
			}
		};

		template <typename T>
		struct DeserializerSXML<std::vector<T>, true>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, std::vector<T>& v)
			{
				VectorDeserializerSXML<T, std::is_class<T>::value>::DeserializeSXML(data, v);
			}
		};

		template <typename K, typename D>
		struct SerializerSXML<std::map<K, D>, true>
		{
			static inline void SerializeSXML(Core::SerializationSXMLData& data, const std::map<K, D>& m)
			{
				for (auto& it : m)
				{
					auto childData = CreateXMLNode(data, "mel");
					_SerializeSXML(childData, it.first, "key");
					_SerializeSXML(childData, it.second, "val");
				}
			}
		};

		template <typename K, typename D>
		struct DeserializerSXML<std::map<K, D>, true>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, std::map<K, D>& m)
			{
				for (auto cNode = data.Node->first_node(); cNode != nullptr; cNode = cNode->next_sibling())
				{
					std::pair<K, D> p;
					_DeserializeSXML(data, p.first, "key");
					_DeserializeSXML(data, p.second, "val");
					m.insert(std::move(p));
				}
			}
		};

		template <typename K, typename D>
		struct SerializerSXML<std::pair<K, D>, true>
		{
			static inline void SerializeSXML(Core::SerializationSXMLData& data, const std::pair<K, D>& p)
			{
				_SerializeSXML(data, p.first, "key");
				_SerializeSXML(data, p.second, "val");
			}
		};

		template <typename K, typename D>
		struct DeserializerSXML<std::pair<K, D>, true>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, std::pair<K, D>& p)
			{
				_DeserializeSXML(data, p.first, "key");
				_DeserializeSXML(data, p.second, "val");
			}
		};

		template <>
		struct SerializerSXML<std::string, true>
		{
			static inline void SerializeSXML(Core::SerializationSXMLData& data, const std::string& str)
			{
				SetXMLNodeValue(data.Document, data.Node, str.c_str());
			}
		};

		template <>
		struct DeserializerSXML<std::string, true>
		{
			static inline void DeserializeSXML(Core::DeserializationSXMLData& data, std::string& str)
			{
				auto nodeValue = data.Node->value();
				str = std::string(nodeValue, nodeValue + data.Node->value_size());
			}
		};
	}

	template <typename T>
	inline void SerializeSXML(SerializationSXMLData& data, const T& object, const char* objectName)
	{
		Core::detail::_SerializeSXML(data, object, objectName);
	}

	template <typename T>
	inline void StartSerializeSXML(std::string& str, const T& object, const char* objectName)
	{
		rapidxml::xml_document<> document;
		auto declarationNode = document.allocate_node(rapidxml::node_declaration);
		AddXMLAttribute(&document, declarationNode, "version", "1.0");
		AddXMLAttribute(&document, declarationNode, "encoding", "utf-8");
		document.append_node(declarationNode);
		std::stringstream toStringSS;
		SerializeSXML(SerializationSXMLData{ &document, &document, &toStringSS }, object, objectName);
		std::stringstream docSS;
		docSS << document;
		str = docSS.str();
	}

	template <typename T>
	inline void DeserializeSXML(Core::DeserializationSXMLData& data, T& object, const char* objectName)
	{
		Core::detail::_DeserializeSXML(data, object, objectName);
	}

	template <typename T>
	inline void StartDeserializeSXML(const std::string& str, T& object, const char* objectName)
	{
		rapidxml::xml_document<> document;
		auto size = str.length();
		auto buffer = new char[size + 1];
		memcpy(buffer, str.c_str(), size);
		buffer[size] = 0;
		document.parse<0>(buffer);
		Core::SimpleTypeVector<char> chars;
		DeserializeSXML(DeserializationSXMLData{ &document, &chars }, object, objectName);
		delete[] buffer;
	}
}

#define Core_SerializeSXML(data, variable)	Core::SerializeSXML(data, variable, #variable)
#define Core_DeserializeSXML(data, variable) Core::DeserializeSXML(data, variable, #variable)

#endif
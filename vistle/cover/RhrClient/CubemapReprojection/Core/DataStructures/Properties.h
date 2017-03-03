// Core/Properties.h

#ifndef _CORE_PROPERTIES_H_INCLUDED_
#define _CORE_PROPERTIES_H_INCLUDED_

#include <Core/DataStructures/SimpleTypeUnorderedVector.hpp>
#include <Core/DataStructures/ResourceUnorderedVector.hpp>
#include <Core/Parse.hpp>

#include <cstddef>
#include <string>

namespace Core
{
	class Properties
	{
	private: // Simple string pool.

		ResourceUnorderedVectorU<std::string> m_Strings;

		unsigned AddString(const char* str, unsigned size);
		unsigned AddString(const char* str);
		void ReleaseString(unsigned index);
		void ModifyString(unsigned index, const char* str);

		const char* GetString(unsigned index) const;

	private:

		struct NameValuePair
		{
			unsigned Name;
			unsigned Value;
		};

		struct Property : public NameValuePair
		{
			SimpleTypeUnorderedVectorU<NameValuePair> AdditionalData;
		};

		struct Node
		{
			unsigned Parent;
			SimpleTypeUnorderedVectorU<unsigned> Children;

			unsigned Name;
			SimpleTypeUnorderedVectorU<unsigned> Properties;
		};

		SimpleTypeUnorderedVectorU<unsigned> m_Roots;
		ResourceUnorderedVectorU<Node> m_Nodes;
		ResourceUnorderedVectorU<Property> m_Properties;

		unsigned CreateNode(const char* name, unsigned parent);
		unsigned CreateNode(const char* name, unsigned nameSize, unsigned parent);
		unsigned CreateProperty(const char* name, const char* value);
		
		bool IsMatchingNodeName(unsigned nameIndex, const char* path,
			const char* dotSearchRes) const;

		void Traverse(const char* path,
			IndexVectorU& propertyIndices,
			unsigned& longestMatchSize,
			unsigned& longestMatchIndex) const;

		void AddPropertyOnNonExistingPath(const char* path,
			unsigned matchSize, unsigned nodeIndex,
			const char* value);

		void RaiseExceptionOnPropertyNotFound(const char* path) const;

	public:

		const char* GetPropertyValueStr(const char* path) const;
		unsigned GetPropertyValuesStr(const char* path, const char** pPath, unsigned maxResultCount);
		void AddProperty(const char* path, const char* value);
		void SetProperty(const char* path, const char* value);

		bool HasProperty(const char* path) const;

		template <typename T>
		bool TryGetPropertyValue(const char* path, T& value) const
		{
			auto pStr = GetPropertyValueStr(path);
			if (pStr == nullptr) return false;
			value = Parse<T>(pStr);
			return true;
		}

		template <typename T>
		inline void TryGetPropertyValue(const char* prefix, const char* name, T& property) const
		{
			TryGetPropertyValue((std::string(prefix) + '.' + name).c_str(), property);
		}

		template <typename T>
		inline void TryGetRootConfigurationValue(const char* name, T& property) const
		{
			TryGetPropertyValue("Configuration", name, property);
		}

		template <typename T>
		inline void TryGetRootConfigurationValue(const std::string& name, T& property) const
		{
			TryGetRootConfigurationValue(name.c_str(), property);
		}

		template <typename T>
		T GetPropertyValue(const char* path) const
		{
			auto pStr = GetPropertyValueStr(path);
			if (pStr == nullptr) RaiseExceptionOnPropertyNotFound(path);
			return Parse<T>(pStr);
		}

		template <typename T>
		inline T GetPropertyValue(const char* prefix, const char* name) const
		{
			return GetPropertyValue<T>((std::string(prefix) + '.' + name).c_str());
		}

		void LoadFromXml(const char* path);
		void LoadFromXmlString(const char* str);
	};
}

#endif
#ifndef VISTLE_INSITU_ARRAY_H
#define VISTLE_INSITU_ARRAY_H
#include "export.h"
#include "dataType.h"

#include <string>
#include <memory>

namespace vistle {
namespace insitu {

enum class Owner {
	Unknown
	, Simulation
	, Vistle
};
enum class DataMapping { //keep in sync with vistle::dataBase::mapping
	Unspecified
	, Vertex
	, Element

};

class V_INSITUCOREEXPORT Array {
public:
	Array(const std::string& name = "");
	Array(const Array& other) = delete;
	Array(Array&& other);
	void operator=(const Array& other) = delete;
	Array& operator=(Array&& other);

	void* data();
	const void* data() const;
	const std::string& name() const;
	const size_t size() const;
	const DataType dataType() const;
	const DataMapping mapping() const;
	const Owner owner() const;
	operator bool()const;
	~Array();

	template<typename T>
	static auto create(const std::string& name, size_t size, DataMapping mapping = DataMapping::Unspecified) {
		T* data = new T[size];
		return Array(name, (void*) data, size, getDataType<T>(), mapping, Owner::Vistle);
	}
	template<typename T>
	static auto create(const std::string& name, T* data, size_t size, DataMapping mapping = DataMapping::Unspecified) {
		return Array(name, (void*)data, size, getDataType<T>(), mapping, Owner::Simulation);
	}
	template<typename T>
	T* dataAs() {
		if (m_dataType == getDataType<T>())
		{
			return static_cast<T*>(m_data);
		}
		return nullptr;
	}

	template<typename T>
	const T* dataAs() const{
		if (m_dataType == getDataType<T>())
		{
			return static_cast<T*>(m_data);
		}
		return nullptr;
	}


private:
	Array(const std::string& name, void* data, size_t size, DataType dataType, DataMapping mapping = DataMapping::Unspecified, Owner owner = Owner::Unknown);
	std::string m_name;
	size_t m_size = 0;
	DataMapping m_mapping = DataMapping::Unspecified;
	Owner m_owner = Owner::Unknown;
	DataType m_dataType = DataType::INVALID;
	void* m_data = nullptr;

};




template<>
constexpr DataType getDataType<Array>() {
	return DataType::ARRAY;
}

//template<typename T>
//auto createArray(const std::string& name, size_t size, DataMapping mapping = DataMapping::Unspecified) {
//	return std::make_unique<detail::ArrayImpl<T>>( name, size, mapping, Owner::Vistle );
//}
//
//template<typename T>
//auto createArray(const std::string& name, T* data, size_t size, DataMapping mapping = DataMapping::Unspecified) {
//	return std::make_unique<detail::ArrayImpl<T>>( name, data, size, mapping, Owner::Simulation );
//
//}

}//insitu
}//vistle
#endif // !VISTLE_INSITU_MESH_H

// Core/SimpleBinarySerialization.hpp

#ifndef _CORE_SIMPLEBINARYSERIALIZATION_HPP_
#define _CORE_SIMPLEBINARYSERIALIZATION_HPP_

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace Core
{
	template <size_t Size>
	struct PlaceHolderStruct
	{
		unsigned char Data[Size];
	};

	template <typename T>
	inline const PlaceHolderStruct<sizeof(T)>& ToPlaceHolder(const T& value)
	{
		return reinterpret_cast<const PlaceHolderStruct<sizeof(T)>&>(value);
	}

	template <typename T>
	inline PlaceHolderStruct<sizeof(T)>& ToPlaceHolder(T& value)
	{
		return reinterpret_cast<PlaceHolderStruct<sizeof(T)>&>(value);
	}

	namespace detail
	{
		template <typename T, bool IsClass>
		struct SerializerSB
		{
			static inline void SerializeSB(ByteVector& bytes, const T& object)
			{
				object.SerializeSB(bytes);
			}
		};

		template <typename T, bool IsClass>
		struct DeserializerSB
		{
			static inline void DeserializeSB(const unsigned char*& bytes, T& object)
			{
				object.DeserializeSB(bytes);
			}
		};

		template <typename T>
		inline void _SerializeSB(ByteVector& bytes, const T& object)
		{
			SerializerSB<T, std::is_class<T>::value>::SerializeSB(bytes, object);
		}

		template <typename T>
		inline void _DeserializeSB(const unsigned char*& bytes, T& object)
		{
			DeserializerSB<T, std::is_class<T>::value>::DeserializeSB(bytes, object);
		}

		template <typename T>
		inline void SerializeSimpleElementSB(ByteVector& bytes, const T& object)
		{
			bytes.PushBack(reinterpret_cast<const unsigned char*>(&object), sizeof(T));
		}

		template <typename T>
		inline void DeserializeSimpleElementSB(const unsigned char*& bytes, T& object)
		{
			const size_t size = sizeof(T);
			memcpy(&object, bytes, size);
			bytes += size;
		}

		template <typename SizeType>
		inline void SerializeSizeSB(ByteVector& bytes, SizeType size)
		{
			// We cast the size to a 64-byte unsigned value to allow serializing and serializing
			// it with different size types (most notably when size_t is different due to different builds).
			_SerializeSB(bytes, static_cast<std::uint64_t>(size));
		}

		template <typename SizeType>
		inline SizeType DeserializeSizeSB(const unsigned char*& bytes)
		{
			std::uint64_t size;
			_DeserializeSB(bytes, size);
			return static_cast<SizeType>(size);
		}

		template <typename T, typename SizeType>
		inline void SerializeArraySB(ByteVector& bytes, const T* data, SizeType size)
		{
			SerializeSizeSB(bytes, size);
			for (size_t i = 0; i < size; i++)
			{
				_SerializeSB(bytes, data[i]);
			}
		}

		template <typename T>
		struct SerializerSB<T, false>
		{
			static inline void SerializeSB(ByteVector& bytes, const T& object)
			{
				SerializeSimpleElementSB(bytes, object);
			}
		};

		template <typename T>
		struct DeserializerSB<T, false>
		{
			static inline void DeserializeSB(const unsigned char*& bytes, T& object)
			{
				DeserializeSimpleElementSB(bytes, object);
			}
		};

		template <size_t Size>
		struct SerializerSB<PlaceHolderStruct<Size>, true>
		{
			static inline void SerializeSB(ByteVector& bytes, const PlaceHolderStruct<Size>& value)
			{
				SerializeSimpleElementSB(bytes, value);
			}
		};

		template <size_t Size>
		struct DeserializerSB<PlaceHolderStruct<Size>, true>
		{
			static inline void DeserializeSB(const unsigned char*& bytes, PlaceHolderStruct<Size>& object)
			{
				DeserializeSimpleElementSB(bytes, object);
			}
		};

		template <typename T, typename SizeType, typename AllocatorType>
		struct SerializerSB<SimpleTypeVector<T, SizeType, AllocatorType>, true>
		{
			static inline void SerializeSB(ByteVector& bytes, const SimpleTypeVector<T, SizeType, AllocatorType>& v)
			{
				// We directly serialize the content of a simple type vector.
				SerializeSizeSB(bytes, v.GetSize());
				bytes.PushBack(reinterpret_cast<const unsigned char*>(v.GetArray()), v.GetSizeInBytes());
			}
		};

		template <typename T, typename SizeType, typename AllocatorType>
		struct DeserializerSB<SimpleTypeVector<T, SizeType, AllocatorType>, true>
		{
			static inline void DeserializeSB(const unsigned char*& bytes, SimpleTypeVector<T, SizeType, AllocatorType>& v)
			{
				auto size = DeserializeSizeSB<SizeType>(bytes);
				v.Resize(size);
				auto copySize = v.GetSizeInBytes();
				memcpy(v.GetArray(), bytes, copySize);
				bytes += copySize;
			}
		};

		template <typename T>
		struct SerializerSB<std::vector<T>, true>
		{
			static inline void SerializeSB(ByteVector& bytes, const std::vector<T>& v)
			{
				SerializeArraySB(bytes, v.data(), v.size());
			}
		};

		template <typename T>
		struct DeserializerSB<std::vector<T>, true>
		{
			static inline void DeserializeSB(const unsigned char*& bytes, std::vector<T>& v)
			{
				auto size = DeserializeSizeSB<size_t>(bytes);
				v.resize(size);
				for (size_t i = 0; i < size; i++)
				{
					_DeserializeSB(bytes, v[i]);
				}
			}
		};

		template <typename K, typename D>
		struct SerializerSB<std::map<K, D>, true>
		{
			static inline void SerializeSB(ByteVector& bytes, const std::map<K, D>& m)
			{
				SerializeSizeSB(bytes, m.size());
				for (auto& it : m)
				{
					_SerializeSB(bytes, it.first);
					_SerializeSB(bytes, it.second);
				}
			}
		};

		template <typename K, typename D>
		struct DeserializerSB<std::map<K, D>, true>
		{
			static inline void DeserializeSB(const unsigned char*& bytes, std::map<K, D>& m)
			{
				auto size = DeserializeSizeSB<size_t>(bytes);
				for (size_t i = 0; i < size; i++)
				{
					K key;
					D data;
					_DeserializeSB(bytes, key);
					_DeserializeSB(bytes, data);
					m[std::move(key)] = std::move(data);
				}
			}
		};

		template <>
		struct SerializerSB<std::string, true>
		{
			static inline void SerializeSB(ByteVector& bytes, const std::string& str)
			{
				SerializeArraySB(bytes, str.data(), str.length());
			}
		};

		template <>
		struct DeserializerSB<std::string, true>
		{
			static inline void DeserializeSB(const unsigned char*& bytes, std::string& str)
			{
				auto size = DeserializeSizeSB<size_t>(bytes);
				str.clear();
				str.append(size, 0);
				for (size_t i = 0; i < size; i++)
				{
					str[i] = bytes[i];
				}
				bytes += size;
			}
		};
	}

	template <typename T>
	inline void SerializeSB(ByteVector& bytes, const T& object)
	{
		Core::detail::_SerializeSB(bytes, object);
	}

	template <typename T>
	inline void StartSerializeSB(ByteVector& bytes, const T& object)
	{
		bytes.Clear();
		SerializeSB(bytes, object);
	}

	template <typename T>
	inline ByteVector StartSerializeSB(const T& object)
	{
		ByteVector bytes;
		StartSerializeSB(bytes, object);
		return bytes;
	}

	template <typename T>
	inline void DeserializeSB(const unsigned char*& bytes, T& object)
	{
		Core::detail::_DeserializeSB(bytes, object);
	}

	template <typename T>
	inline void StartDeserializeSB(const unsigned char* bytes, T& object)
	{
		DeserializeSB(bytes, object);
	}

	template <typename T>
	inline void StartDeserializeSB(const ByteVector& bytes, T& object)
	{
		StartDeserializeSB(bytes.GetArray(), object);
	}
}

#endif

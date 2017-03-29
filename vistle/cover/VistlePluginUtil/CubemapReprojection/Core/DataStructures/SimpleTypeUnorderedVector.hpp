// Core/DataStructures/SimpleTypeUnorderedVector.hpp

#ifndef _CORE_SIMPLETYPEUNORDEREDVECTOR_HPP_
#define _CORE_SIMPLETYPEUNORDEREDVECTOR_HPP_

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <map>

namespace Core
{
	template <typename T, typename SizeType = size_t>
	class SimpleTypeUnorderedVector
	{
		enum class State : unsigned char
		{
			Valid, Invalid, End
		};

		SimpleTypeVector<T, SizeType> m_Data;
		SimpleTypeVector<State, SizeType> m_State;
		SimpleTypeVector<SizeType, SizeType> m_UnusedIndices;
		SizeType m_CountElements;

		inline const T* GetLastElementPointer() const
		{
			assert(m_CountElements > 0);

			auto lastElementIndex = m_Data.GetSize() - 1;
			auto pData = m_Data.GetArray() + lastElementIndex;
			auto pState = m_State.GetArray() + lastElementIndex;

			while (*pState == State::Invalid)
			{
				--pData;
				--pState;
			}

			return pData;
		}

		inline SizeType CreateElementAtTheEnd()
		{
			assert(!m_State.IsEmpty());

			auto index = m_Data.GetSize();
			m_Data.PushBackPlaceHolder();
			m_State.GetLastElement() = State::Valid;
			m_State.PushBack(State::End);
			return index;
		}

	public:

		SimpleTypeUnorderedVector()
			: m_CountElements(0)
		{
			m_State.PushBack(State::End);
		}

		// Initializes vector using a begin and an end iterator.
		// Indices are the integer numbers of the range [0, N-1], if the iterator encounter N elements.
		// If the number of the elements is unknown use the GetSize function
		// to figure out the index interval.
		template <typename Iterator>
		SimpleTypeUnorderedVector(Iterator& begin, Iterator& end)
			: m_CountElements(0)
		{
			for (; begin != end; ++begin)
			{
				m_Data.PushBack(*begin);
				m_State.PushBack(State::Valid);
				m_CountElements++;
			}
			m_State.PushBack(State::End);
		}

		inline SizeType GetSize() const
		{
			return m_CountElements;
		}

		inline T* GetArray()
		{
			return m_Data.GetArray();
		}

		inline const T* GetArray() const
		{
			return m_Data.GetArray();
		}

		inline bool IsEmpty() const
		{
			return (m_CountElements == 0);
		}

		inline const T& operator[](SizeType index) const
		{
			assert(m_State[index] == State::Valid);

			return m_Data[index];
		}

		inline T& operator[](SizeType index)
		{
			assert(m_State[index] == State::Valid);

			return m_Data[index];
		}

		inline SizeType Add()
		{
			++m_CountElements;

			if (m_UnusedIndices.IsEmpty())
			{
				return CreateElementAtTheEnd();
			}
			else
			{
				auto index = m_UnusedIndices.PopBackReturn();
				m_State[index] = State::Valid;
				return index;
			}
		}

		inline SizeType Add(const T& element)
		{
			SizeType index = Add();
			m_Data[index] = element;
			return index;
		}

		inline SizeType AddToTheEnd()
		{
			++m_CountElements;
			return CreateElementAtTheEnd();
		}

		inline SizeType AddToTheEnd(const T& element)
		{
			SizeType index = AddToTheEnd();
			m_Data[index] = element;
			return index;
		}

		inline void Remove(SizeType index)
		{
			assert(m_CountElements > 0 && m_State[index] == State::Valid);

			--m_CountElements;
			m_State[index] = State::Invalid;
			m_UnusedIndices.PushBack(index);
		}

		inline void SetByte(unsigned char value)
		{
			m_Data.SetByte(value);
		}

		inline void Clear()
		{
			m_Data.Clear();
			m_State.Clear();
			m_UnusedIndices.Clear();
			m_State.PushBack(State::End);
			m_CountElements = 0;
		}

		inline SizeType GetCountUnusedElements() const
		{
			return m_UnusedIndices.GetSize();
		}

		inline std::map<SizeType, SizeType> ShrinkToFit(bool isShrinkingUnderlyingVectors = true)
		{
			std::map<SizeType, SizeType> indexMap;

			SizeType arraySize = static_cast<SizeType>(m_Data.GetSize());
			SizeType targetIndex = 0;
			for (SizeType sourceIndex = 0; sourceIndex < arraySize; ++sourceIndex)
			{
				if (m_State[sourceIndex] == State::Valid)
				{
					m_Data[targetIndex] = std::move(m_Data[sourceIndex]);
					indexMap[sourceIndex] = targetIndex;
					targetIndex++;
				}
			}
			m_State.Clear();
			m_State.PushBack(State::Valid, targetIndex);
			m_State.PushBack(State::End);
			m_UnusedIndices.Clear();

			if (isShrinkingUnderlyingVectors)
			{
				m_State.ShrinkToFit();
				m_Data.ShrinkToFit();
			}

			return indexMap;
		}

		inline const T& GetLastElement() const
		{
			return *GetLastElementPointer();
		}

		inline T& GetLastElement()
		{
			return const_cast<T&>(*GetLastElementPointer());
		}

		inline SizeType GetLastElementIndex() const
		{
			return static_cast<SizeType>(GetLastElementPointer() - m_Data.GetArray());
		}

		class Iterator
		{
			T* m_PData;
			State* m_PState;

		public:

			Iterator(T* pData, State* pState)
				: m_PData(pData)
				, m_PState(pState)
			{
				while (*m_PState == State::Invalid)  // State should be either Valid or End.
				{
					++m_PData;
					++m_PState;
				}
			}

			inline bool operator==(Iterator& other)
			{
				return (m_PData == other.m_PData);
			}

			inline bool operator!=(Iterator& other)
			{
				return (m_PData != other.m_PData);
			}

			inline bool operator<(Iterator& other)
			{
				return (m_PData < other.m_PData);
			}

			inline Iterator& operator++()
			{
				assert(*m_PState != State::End);

				do
				{
					++m_PData;
					++m_PState;
				} while (*m_PState == State::Invalid); // State should be either Valid or End.
				
				return *this;
			}

			inline T& operator*()
			{
				return *m_PData;
			}

			inline T* operator->()
			{
				return m_PData;
			}

			inline T* GetDataPtr()
			{
				return m_PData;
			}

			inline const T* GetDataPtr() const
			{
				return m_PData;
			}
		};

		inline Iterator GetBeginIterator()
		{
			return Iterator(m_Data.GetArray(), m_State.GetArray());
		}

		inline Iterator GetEndIterator()
		{
			SizeType arraySize = m_Data.GetSize();
			return Iterator(m_Data.GetArray() + arraySize, m_State.GetArray() + arraySize);
		}

		inline SizeType ToIndex(const Iterator& it) const
		{
			return static_cast<SizeType>(it.GetDataPtr() - m_Data.GetArray());
		}

		bool Contains(const T& element) const
		{
			auto pData = m_Data.GetArray();
			auto pState = m_State.GetArray();
			do
			{
				while (*pState == State::Invalid)
				{
					++pData;
					++pState;
				}
				if (*pState == State::End)
				{
					break;
				}
				if (*pData == element)
				{
					return true;
				}
				++pData;
				++pState;
			} while (true);

			return false;
		}
	};

	template <typename T>
	using SimpleTypeUnorderedVectorU = SimpleTypeUnorderedVector<T, unsigned>;

	template <typename T, typename SizeType = size_t>
	class SimpleTypeSet
	{
		SimpleTypeUnorderedVector<T, SizeType> m_Data;
		std::map<T, SizeType> m_IndexMap;

	public:

		SimpleTypeSet()
		{
		}

		inline SizeType GetSize() const
		{
			return m_Data.GetSize();
		}

		inline bool IsEmpty() const
		{
			return m_Data.IsEmpty();
		}

		inline unsigned Add(const T& value)
		{
			SizeType index = m_Data.Add(value);
			m_IndexMap[value] = index;
			return index;
		}

		inline void Remove(const T& value)
		{
			auto it = m_IndexMap.find(value);

			assert(it != m_IndexMap.end());

			m_Data.Remove(it->second);
			m_IndexMap.erase(it);
		}

		inline void Clear()
		{
			m_Data.Clear();
			m_IndexMap.clear();
		}

		inline bool IsContaining(const T& value, unsigned& index)
		{
			auto it = m_IndexMap.find(value);
			if (it == m_IndexMap.end())
			{
				return false;
			}
			index = it->second;
			return true;
		}

		inline const T& operator[](SizeType index) const
		{
			return m_Data[index];
		}

		inline T& operator[](SizeType index)
		{
			return m_Data[index];
		}

		inline typename SimpleTypeUnorderedVector<T>::Iterator GetBeginIterator()
		{
			return m_Data.GetBeginIterator();
		}

		inline typename SimpleTypeUnorderedVector<T>::Iterator GetEndIterator()
		{
			return m_Data.GetEndIterator();
		}
	};

	template <typename T>
	using SimpleTypeSetU = SimpleTypeSet<T, unsigned>;

	template <typename KeyType, typename DataType, typename SizeType = size_t>
	class SimpleTypeMap
	{
		SimpleTypeUnorderedVector<DataType, SizeType> m_Data;
		std::map<KeyType, SizeType> m_IndexMap;

	public:

		SimpleTypeMap()
		{
		}

		inline SizeType GetSize() const
		{
			return m_Data.GetSize();
		}

		inline bool IsEmpty() const
		{
			return m_Data.IsEmpty();
		}

		inline void Add(const KeyType& key, const DataType& data)
		{
			SizeType index = m_Data.Add(data);
			m_IndexMap[key] = index;
		}

		inline void Remove(const KeyType& key)
		{
			auto it = m_IndexMap.find(key);

			assert(it != m_IndexMap.end());

			m_Data.Remove(it->second);
			m_IndexMap.erase(it);
		}

		inline void Clear()
		{
			m_Data.Clear();
			m_IndexMap.clear();
		}

		inline bool IsContaining(const KeyType& key)
		{
			return (m_IndexMap.find(key) != m_IndexMap.end());
		}

		inline typename SimpleTypeUnorderedVector<DataType>::Iterator GetDataBeginIterator()
		{
			return m_Data.GetBeginIterator();
		}

		inline typename SimpleTypeUnorderedVector<DataType>::Iterator GetDataEndIterator()
		{
			return m_Data.GetEndIterator();
		}

		inline DataType& Get(const KeyType& key)
		{
			return m_Data[m_IndexMap.find(key)->second];
		}

		inline const DataType& Get(const KeyType& key) const
		{
			return m_Data[m_IndexMap.find(key)->second];
		}

		inline DataType& operator[](const KeyType& key)
		{
			return Get(key);
		}

		inline const DataType& operator[](const KeyType& key) const
		{
			return Get(key);
		}
	};
}

#endif
// Core/DataStructures/ResourceUnorderedVector.hpp

#ifndef _CORE_RESOURCEUNORDEREDVECTOR_HPP_
#define _CORE_RESOURCEUNORDEREDVECTOR_HPP_

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <map>
#include <vector>

namespace Core
{
	template <typename T, typename SizeType = size_t>
	class ResourceUnorderedVector
	{
		enum class State : unsigned char
		{
			Valid, Invalid, End
		};

		std::vector<T> m_Data;
		SimpleTypeVector<State, SizeType> m_State;
		SimpleTypeVector<SizeType, SizeType> m_UnusedIndices;
		SizeType m_CountElements;

	public:

		ResourceUnorderedVector()
			: m_CountElements(0)
		{
			m_State.PushBack(State::End);
		}

		// Initializes vector using a begin and an end iterator.
		// Indices are the integer numbers of the range [0, N-1], if the iterator encounter N elements.
		// If the number of the elements is unknown use the GetSize function
		// to figure out the index interval.
		template <typename Iterator>
		ResourceUnorderedVector(Iterator& begin, Iterator& end)
			: m_CountElements(0)
		{
			for (; begin != end; ++begin)
			{
				m_Data.push_back(*begin);
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
			return m_Data.data();
		}

		inline const T* GetArray() const
		{
			return m_Data.data();
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

		template <typename... _T>
		inline SizeType Add(_T&&... element)
		{
			++m_CountElements;
			if (m_UnusedIndices.IsEmpty())
			{
				assert(!m_State.IsEmpty());

				auto index = static_cast<SizeType>(m_Data.size());
				m_Data.emplace_back(std::forward<_T>(element)...);
				m_State.GetLastElement() = State::Valid;
				m_State.PushBack(State::End);
				return index;
			}
			else
			{
				auto index = m_UnusedIndices.PopBackReturn();
				m_Data[index] = T(std::forward<_T>(element)...);
				m_State[index] = State::Valid;
				return index;
			}
		}

		inline void Remove(SizeType index)
		{
			assert(m_CountElements > 0 && m_State[index] == State::Valid);

			--m_CountElements;
			m_State[index] = State::Invalid;
			m_UnusedIndices.PushBack(index);

			// Setting the data to default state.
			m_Data[index] = T();
		}

		inline void Clear()
		{
			m_Data.clear();
			m_State.Clear();
			m_UnusedIndices.Clear();
			m_State.PushBack(State::End);
			m_CountElements = 0;
		}

		inline SizeType GetCountUnusedElements() const
		{
			return m_UnusedIndices.GetSize();
		}

		SimpleTypeVector<SizeType, SizeType> GetIndices() const
		{
			SimpleTypeVector<SizeType, SizeType> indices;
			indices.Reserve(m_CountElements);
			SizeType limit = m_State.GetSize() - 1;
			for (SizeType i = 0; i < limit; i++) if (m_State[i] == State::Valid) indices.UnsafePushBack(i);
			return indices;
		}

		inline std::map<SizeType, SizeType> ShrinkToFit(bool isShrinkingUnderlyingVectors = true)
		{
			std::map<SizeType, SizeType> indexMap;

			SizeType arraySize = static_cast<SizeType>(m_Data.size());
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
				m_Data.shrink_to_fit();
			}

			return indexMap;
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
			return Iterator(m_Data.data(), m_State.GetArray());
		}

		inline Iterator GetEndIterator()
		{
			SizeType arraySize = static_cast<SizeType>(m_Data.size());
			return Iterator(m_Data.data() + arraySize, m_State.GetArray() + arraySize);
		}

		inline SizeType ToIndex(const Iterator& it) const
		{
			return static_cast<SizeType>(it.GetDataPtr() - m_Data.data());
		}
	};

	template <typename T>
	using ResourceUnorderedVectorU = ResourceUnorderedVector<T, unsigned>;
}

#endif
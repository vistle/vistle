// Core/DataStructures/SimpleTypeVector.hpp

#ifndef _CORE_SIMPLETYPEVECTOR_HPP_INCLUDED_
#define _CORE_SIMPLETYPEVECTOR_HPP_INCLUDED_

#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <utility>

#include <Core/Comparison.h>

namespace Core
{
	const unsigned c_SimpleTypeVector_InitialSize = 8U;
	const double c_SimpleTypeVector_GrowFactor = 2.0;

	// Represents a vector with elements that don't require constructor and destructor calls.
	// Note despite the fact that the template specifies which allocator should be used, it is ONLY
	// used to allocate memory and NOT to construct and destroy the object.
	template <typename T,
		typename SizeType = size_t,
		typename AllocatorType = std::allocator<T>>
	class SimpleTypeVector
	{
		T* m_Array;
		SizeType m_Size;
		SizeType m_Capacity;

		AllocatorType m_Allocator;

		inline void Allocate(SizeType capacity)
		{
			assert(capacity > 0);
			m_Array = m_Allocator.allocate(capacity);
			m_Capacity = capacity;
		}

		inline void Deallocate(T* tArray, SizeType count)
		{
			if (tArray != nullptr) m_Allocator.deallocate(tArray, count);
		}

		inline void Deallocate()
		{
			Deallocate(m_Array, m_Capacity);
		}

		inline void Reallocate(SizeType capacity)
		{
			Deallocate();
			Allocate(capacity);
		}

		inline void ReallocateOnInsufficientCapacity(SizeType capacity)
		{
			if (capacity > m_Capacity)
			{
				Reallocate(capacity);
			}
		}

		inline void ChangeCapacityWithElementCopy(SizeType capacity)
		{
			auto tempArray = m_Array;
			auto tempCapacity = m_Capacity;
			Allocate(capacity);
			CopyElements(tempArray, m_Size);
			Deallocate(tempArray, tempCapacity);
		}

		inline void CopyElements(T* source, SizeType size)
		{
			memcpy(m_Array, source, size * sizeof(T));
		}

		inline void CopyValue(const T& value, SizeType size)
		{
			std::fill(m_Array, &m_Array[size], value);
		}

		inline SizeType GetGrownSize() const
		{
			if (m_Capacity == 0)
			{
				return c_SimpleTypeVector_InitialSize;
			}
			return std::max(static_cast<SizeType>(std::floor(static_cast<double>(m_Capacity) * c_SimpleTypeVector_GrowFactor)), m_Size + 1);
		}

		inline SizeType GetGrownSize(SizeType size)
		{
			return std::max(GetGrownSize(), size);
		}

	public:

		SimpleTypeVector()
			: m_Array(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
		}

		explicit SimpleTypeVector(SizeType size)
			: m_Size(size)
		{
			assert(size > 0);
			Allocate(size);
		}

		SimpleTypeVector(SizeType size, const T& value)
			: m_Size(size)
		{
			Allocate(size);
			CopyValue(value, size);
		}

		SimpleTypeVector(const SimpleTypeVector& other)
			: m_Size(other.m_Size)
		{
			if (m_Size > 0)
			{
				Allocate(m_Size);
				CopyElements(other.m_Array, m_Size);
			}
			else
			{
				m_Array = nullptr;
				m_Capacity = 0;
			}
		}

		SimpleTypeVector(SimpleTypeVector&& other)
			: m_Array(other.m_Array)
			, m_Size(other.m_Size)
			, m_Capacity(other.m_Capacity)
		{
			other.m_Array = nullptr;
		}

		SimpleTypeVector(std::initializer_list<T> initializerList)
			: m_Array(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			*this = initializerList;
		}

		explicit SimpleTypeVector(const T* source, SizeType size)
			: m_Array(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			PushBack(source, size);
		}

		~SimpleTypeVector()
		{
			Deallocate();
		}

		inline SimpleTypeVector& operator=(const SimpleTypeVector& other)
		{
			if (other.m_Size == 0)
			{
				Deallocate();
				m_Array = nullptr;
				m_Capacity = 0;
			}
			else
			{
				ReallocateOnInsufficientCapacity(other.m_Size);
				CopyElements(other.m_Array, other.m_Size);
			}
			m_Size = other.m_Size;
			return *this;
		}

		inline SimpleTypeVector& operator=(SimpleTypeVector&& other)
		{
			Deallocate();
			m_Array = other.m_Array;
			m_Size = other.m_Size;
			m_Capacity = other.m_Capacity;
			other.m_Array = nullptr;
			return *this;
		}

		inline SimpleTypeVector& operator=(std::initializer_list<T> initializerList)
		{
			auto size = static_cast<SizeType>(initializerList.size());
			Resize(size);
			auto it = initializerList.begin();
			for (SizeType i = 0; i < size; ++i, ++it)
			{
				m_Array[i] = *it;
			}
			return *this;
		}

		inline T& operator[](SizeType index)
		{
			assert(index < m_Size);

			return m_Array[index];
		}

		inline const T& operator[](SizeType index) const
		{
			assert(index < m_Size);

			return m_Array[index];
		}

		inline T& GetLastElement()
		{
			assert(m_Size > 0);

			return m_Array[m_Size - 1];
		}

		inline const T& GetLastElement() const
		{
			assert(m_Size > 0);

			return m_Array[m_Size - 1];
		}

		inline T* GetArray()
		{
			return m_Array;
		}

		inline const T* GetArray() const
		{
			return m_Array;
		}

		inline T* GetEndPointer()
		{
			return m_Array + m_Size;
		}

		inline const T* GetEndPointer() const
		{
			return m_Array + m_Size;
		}

		// Returns the number of elements contained in the vector.
		inline SizeType GetSize() const
		{
			return m_Size;
		}

		// Returns the storage size of the contained elements in bytes.
		inline SizeType GetSizeInBytes() const
		{
			return m_Size * static_cast<SizeType>(sizeof(T));
		}

		// Returns the allocated storage size.
		inline SizeType GetCapacity() const
		{
			return m_Capacity;
		}

		inline bool IsEmpty() const
		{
			return (m_Size == 0);
		}

		inline bool HasElement() const
		{
			return (m_Size > 0);
		}

		inline void Reserve(SizeType size)
		{
			if (size > m_Capacity)
			{
				ChangeCapacityWithElementCopy(size);
			}
		}

		inline void ReserveWithGrowing(SizeType size)
		{
			if (size > m_Capacity)
			{
				ChangeCapacityWithElementCopy(GetGrownSize(size));
			}
		}

		inline void ReserveAdditionalWithGrowing(SizeType size)
		{
			ReserveWithGrowing(m_Size + size);
		}

		inline void UnsafePushBack(const T& element)
		{
			assert(m_Size < m_Capacity);
			m_Array[m_Size] = element;
			m_Size++;
		}

		inline void PushBack(const T& element)
		{
			if (m_Size == m_Capacity)
			{
				ChangeCapacityWithElementCopy(GetGrownSize());
			}
			UnsafePushBack(element);
		}

		inline void PushBack(const T& element, SizeType countCopies)
		{
			SizeType newSize = m_Size + countCopies;
			ReserveWithGrowing(newSize);
			for (SizeType i = m_Size; i < newSize; ++i)
			{
				m_Array[i] = element;
			}
			m_Size = newSize;
		}

		inline void PopBack()
		{
			assert(m_Size > 0);

			m_Size--;
		}

		// This function is a minor optimization compared to calling GetLastElement() and PopBack().
		// Note that this function COPIES the last element since after calling the function the element
		// is no longer considered to be in the vector.
		inline T PopBackReturn()
		{
			assert(m_Size > 0);
			
			m_Size--;
			return m_Array[m_Size];
		}

		inline T& UnsafePushBackPlaceHolder()
		{
			assert(m_Size < m_Capacity);

			return m_Array[m_Size++];
		}

		inline T& PushBackPlaceHolder()
		{
			if (m_Size == m_Capacity)
			{
				ChangeCapacityWithElementCopy(GetGrownSize());
			}
			return m_Array[m_Size++];
		}

		inline void PushBack(const SimpleTypeVector& other)
		{
			PushBack(other.m_Array, other.m_Size);
		}

		inline void PushBack(const T* source, SizeType size)
		{
			ReserveAdditionalWithGrowing(size);
			UnsafePushBack(source, size);
		}

		inline void UnsafePushBack(const SimpleTypeVector& other)
		{
			UnsafePushBack(other.m_Array, other.m_Size);
		}

		inline void UnsafePushBack(const T* source, SizeType size)
		{
			memcpy(m_Array + m_Size, source, size * sizeof(T));
			m_Size = m_Size + size;
		}

		inline void Insert(const T& element, SizeType index)
		{
			if (index >= m_Size)
			{
				auto newSize = index + 1;
				ReserveWithGrowing(newSize);
				m_Size = newSize;
			}
			m_Array[index] = element;
		}

		inline void Insert(const T& element, SizeType index, const T& defaultElement)
		{
			if (index >= m_Size)
			{
				auto newSize = index + 1;
				ReserveWithGrowing(newSize);
				for (SizeType i = m_Size; i < index; ++i) m_Array[i] = defaultElement;
				m_Size = newSize;
			}
			m_Array[index] = element;
		}

		inline void Remove(SizeType index)
		{
			assert(index < m_Size);
			--m_Size;
			for (SizeType i = index; i < m_Size; i++)
			{
				m_Array[i] = m_Array[i + 1];
			}
		}

		inline void Remove(SizeType start, SizeType end)
		{	
			// Note that 'start == end' is a valid input, but we do not handle it specially.
			assert(start <= end && end <= m_Size);
			SizeType count = end - start;
			m_Size -= count;
			for (SizeType i = start; i < m_Size; i++)
			{
				m_Array[i] = m_Array[i + count];
			}
		}

		inline void RemoveWithLastElementCopy(SizeType index)
		{
			assert(index < m_Size);

			m_Array[index] = m_Array[--m_Size];
		}

		inline void Clear()
		{
			m_Size = 0;
		}

		inline void ClearAndDeallocate()
		{
			m_Size = 0;
			Deallocate();
			m_Array = nullptr;
			m_Capacity = 0;
		}

		inline void ClearAndReserve(SizeType size)
		{
			m_Size = 0;
			ReallocateOnInsufficientCapacity(size);
		}

		// Function reserves if necessary and sets the sizes, but doesn't initialize anything!
		inline void Resize(SizeType size)
		{
			Reserve(size);
			this->m_Size = size;
		}

		// Function reserves if necessary and sets the sizes, but doesn't initialize anything!
		// For the resizing the exponential grow logic is used. Use this function, if you intend to call
		// it multiple times and the used memory size is a less important concern than speed.
		inline void ResizeWithGrowing(SizeType size)
		{
			ReserveWithGrowing(size);
			this->m_Size = size;
		}

		// Function resizes vector as the safe variant without reserving.
		inline void UnsafeResize(SizeType size)
		{
			this->m_Size = size;
		}

		inline void ShrinkToFit()
		{
			if (m_Size < m_Capacity)
			{
				if (m_Size > 0)
				{
					ChangeCapacityWithElementCopy(m_Size);
				}
				else
				{
					Deallocate();
					m_Array = nullptr;
					m_Capacity = 0;
				}
			}
		}

		inline void SetByte(unsigned char value)
		{
			std::memset(m_Array, value, m_Size * sizeof(T));
		}

		inline void SetAtIndex(SizeType index, const T& value)
		{
			if (index >= m_Size) ResizeWithGrowing(index + 1);
			m_Array[index] = value;
		}

		// This function sets the element at the given index and constructs the elements between
		// the currently last and the new element with the forwarded arguments.
		template <typename... Args>
		inline void SetAtIndexWithInitialization(SizeType index, const T& value, Args&&... args)
		{
			if (index >= m_Size)
			{
				SizeType oldSize = m_Size;
				ResizeWithGrowing(index + 1);
				for (SizeType i = oldSize; i < m_Size; ++i)
					m_Allocator.construct(m_Array + i, std::forward<Args>(args)...);
			}
			m_Array[index] = value;
		}

		inline SimpleTypeVector GetSubarray(SizeType start, SizeType end) const
		{
			assert(start <= end && end <= m_Size);
			SizeType count = end - start;
			SimpleTypeVector result(count);
			memcpy(result.m_Array, m_Array + start, count * sizeof(T));
			return result;
		}

		inline bool operator==(const SimpleTypeVector& other) const
		{
			NumericalEqualCompareBlock(m_Size);
			return (memcmp(m_Array, other.m_Array, m_Size * sizeof(T)) == 0);
		}

		inline bool operator!=(const SimpleTypeVector& other) const
		{
			return !(*this == other);
		}

		inline bool operator<(const SimpleTypeVector& other) const
		{
			NumericalLessCompareBlock(m_Size);
			return (memcmp(m_Array, other.m_Array, m_Size * sizeof(T)) < 0);
		}
	};

	template <typename T>
	using SimpleTypeVectorU = SimpleTypeVector<T, unsigned>;

	using ByteVector = SimpleTypeVector<unsigned char>;
	using ByteVectorU = SimpleTypeVectorU<unsigned char>;

	using IndexVector = SimpleTypeVector<unsigned>;
	using IndexVectorU = SimpleTypeVectorU<unsigned>;
}

#endif

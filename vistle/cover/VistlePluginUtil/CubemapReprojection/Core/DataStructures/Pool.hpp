// Core/DataStructures/Pool.hpp

#ifndef _CORE_POOL_HPP_INCLUDED_
#define _CORE_POOL_HPP_INCLUDED_

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <memory>
#include <utility>
#include <algorithm>

namespace Core
{
	const unsigned c_ResourcePool_InitialSize = 32U;
	const double c_ResourcePool_GrowFactor = 2.0;

	template<typename T,
		typename SizeType = size_t,
		typename AllocatorType = std::allocator<T>>
		class ResourcePool
	{
		struct MemoryData
		{
			T* Ptr;
			SizeType CountElements;
		};

		AllocatorType m_Allocator;

		SimpleTypeVectorU<MemoryData> m_Memory;
		SimpleTypeVector<T*, SizeType> m_UnusedElements;

		SizeType m_CountAllocatedElements;
		SizeType m_CountElements;

		void Construct()
		{
			m_CountAllocatedElements = 0;
			m_CountElements = 0;
		}

		T* GetUnusedElement()
		{
			if (m_UnusedElements.GetSize() == 0)
			{
				AllocateNewElements(m_CountAllocatedElements == 0
					? c_ResourcePool_InitialSize
					: static_cast<SizeType>(static_cast<double>(m_CountAllocatedElements) * c_ResourcePool_GrowFactor));
			}

			// We use always the last released element, since this one is the most likely to be in the cache.
			return m_UnusedElements.PopBackReturn();
		}

		void UnsafeAddElementsAsUnused(MemoryData& memory)
		{
			// Pushing the pointers in reverse order to get normal order usage.
			auto ptr = memory.Ptr;
			auto count = memory.CountElements;
			for (SizeType i = count - 1; i > 0; i--)
			{
				m_UnusedElements.UnsafePushBack(ptr + i);
			}
			m_UnusedElements.UnsafePushBack(ptr);
		}

		void AllocateNewElements(SizeType totalElementCount)
		{
			assert(totalElementCount > m_CountAllocatedElements);
			auto countElements = totalElementCount - m_CountAllocatedElements;
			m_CountAllocatedElements = totalElementCount;
			m_Memory.PushBack({ Allocate(countElements), countElements });
			m_UnusedElements.ReserveAdditionalWithGrowing(countElements);
			UnsafeAddElementsAsUnused(m_Memory.GetLastElement());
		}

		void DeallocateAllElements()
		{
			unsigned countLevels = m_Memory.GetSize();
			for (unsigned i = 0; i < countLevels; i++)
			{
				auto& cMemory = m_Memory[i];
				Deallocate(cMemory.Ptr, cMemory.CountElements);
			}
		}

		T* Allocate(SizeType countElements)
		{
			return m_Allocator.allocate(countElements);
		}

		void Deallocate(T* ptr, SizeType count)
		{
			m_Allocator.deallocate(ptr, count);
		}

		template<typename... Types>
		void ConstructElement(T* pElement, Types... args)
		{
			m_Allocator.construct(pElement, std::forward<Types>(args)...);
		}

		void DeleteElement(T* pElement)
		{
			m_Allocator.destroy(pElement);
		}

		// Note that this function changes the element pointer order in 'm_UnusedElements'.
		void DeleteAllUsedElements()
		{
			auto unusedBegin = m_UnusedElements.GetArray();
			auto unusedEnd = m_UnusedElements.GetEndPointer();

			std::sort(unusedBegin, unusedEnd);

			unsigned countLevels = m_Memory.GetSize();
			for (unsigned i = 0; i < countLevels; i++)
			{
				auto& cMemory = m_Memory[i];
				auto ptr = cMemory.Ptr;
				auto count = cMemory.CountElements;
				for (unsigned j = 0; j < count; j++)
				{
					auto pElement = ptr + j;
					if (!std::binary_search(unusedBegin, unusedEnd, pElement))
						DeleteElement(pElement);
				}
			}
		}

	public:

		ResourcePool()
		{
			Construct();
		}

		ResourcePool(SizeType countElements)
		{
			Construct();
			SetAllocatedElementCount(countElements);
		}

		~ResourcePool()
		{
			DeleteAllUsedElements();
			DeallocateAllElements();
		}

		ResourcePool(const ResourcePool&) = delete;
		ResourcePool& operator=(const ResourcePool&) = delete;

		SizeType GetCountElements() const
		{
			return m_CountElements;
		}

		SizeType GetCountAllocatedElements() const
		{
			return m_CountAllocatedElements;
		}

		SizeType GetAllocatedSizeInBytes() const
		{
			return m_CountAllocatedElements * static_cast<SizeType>(sizeof(T));
		}

		void SetAllocatedElementCount(SizeType countElements)
		{
			if (countElements > m_CountAllocatedElements) AllocateNewElements(countElements);
		}

		template<typename... Types>
		T* Request(Types&&... args)
		{
			auto pElement = GetUnusedElement();
			ConstructElement(pElement, std::forward<Types>(args)...);
			++m_CountElements;
			return pElement;
		}

		void Release(T* pElement)
		{
			DeleteElement(pElement);
			m_UnusedElements.PushBack(pElement);
			--m_CountElements;
		}

		void ReleaseAll()
		{
			DeleteAllUsedElements();

			m_UnusedElements.Clear();
			m_UnusedElements.ReserveWithGrowing(m_CountAllocatedElements);

			unsigned countLevels = m_Memory.GetSize();
			for (unsigned i = 0; i < countLevels; i++)
			{
				UnsafeAddElementsAsUnused(m_Memory[i]);
			}
		}

		void Reset()
		{
			DeleteAllUsedElements();
			DeallocateAllElements();
			Construct();
		}
	};

	template <typename T> using ResourcePoolU = ResourcePool<T, unsigned>;
}

#endif

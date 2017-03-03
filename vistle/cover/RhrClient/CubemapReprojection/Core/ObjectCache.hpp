// Core/ObjectCache.hpp

#ifndef _CORE_OBJECTCACHE_HPP_
#define _CORE_OBJECTCACHE_HPP_

#include <Core/DataStructures/Pool.hpp>

#include <map>
#include <typeinfo>

namespace Core
{
	// Object cache provides objects using pools for each type. Types are handled using compile time type inference and template functions.
	// Note that since the underlying pool allocates objects consecutively, the object cache aims not only memory allocation reduction but also
	// cache coherence enhancement.
	class ObjectCache
	{
		struct PooledCacheBase
		{
			virtual ~PooledCacheBase() {}
		};

		template <typename ElementType>
		struct PooledCache : public PooledCacheBase
		{
			ResourcePool<ElementType> Pool;

			PooledCache(size_t poolSize)
				: Pool(poolSize)
			{
			}
		};

		struct CompareTypeInfos
		{
			bool operator ()(const std::type_info* a, const std::type_info* b) const
			{
				return a->before(*b);
			}
		};

		std::map<const std::type_info*, PooledCacheBase*, CompareTypeInfos> m_Pools;

		template <typename ElementType>
		ResourcePool<ElementType>& GetPool()
		{
			auto typeInfoPtr = &typeid(ElementType);
			auto poolIt = m_Pools.find(typeInfoPtr);
			if (poolIt == m_Pools.end())
			{
				auto pooledCache = new PooledCache<ElementType>(m_PoolInitialSize);
				m_Pools[typeInfoPtr] = pooledCache;
				return pooledCache->Pool;
			}
			return static_cast<PooledCache<ElementType>*>(poolIt->second)->Pool;
		}

		size_t m_PoolInitialSize;

	public:

		ObjectCache()
			: m_PoolInitialSize(c_ResourcePool_InitialSize)
		{
		}

		ObjectCache(size_t poolInitialSize)
			: m_PoolInitialSize(poolInitialSize)
		{
		}

		~ObjectCache()
		{
			for (auto& it : m_Pools)
			{
				delete it.second;
			}
		}

		template <typename ElementType, typename... Args>
		ElementType* Request(Args&&... args)
		{
			return GetPool<ElementType>().Request(std::forward<Args>(args)...);
		}

		template <typename ElementType>
		void Release(ElementType* object)
		{
			GetPool<ElementType>().Release(object);
		}
	};
}

#endif

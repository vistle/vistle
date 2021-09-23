#ifndef VISTLE_LIBSIM_SMART_HANDLE_H
#define VISTLE_LIBSIM_SMART_HANDLE_H
#include "Exception.h"
#include "libsimInterface/VisItDataTypes.h"
#include "libsimInterface/UnstructuredMesh.h"
#include "libsimInterface/RectilinearMesh.h"
#include "libsimInterface/DomainList.h"
#include "libsimInterface/VariableData.h"
#include "libsimInterface/SimulationMetaData.h"
#include "libsimInterface/CurvilinearMesh.h"

#include <iostream>
#include <memory>
namespace vistle {
namespace insitu {
namespace libsim {

enum class HandleType {
    CurvilinearMesh,
    UnstructuredMesh,
    RectilinearMesh,
    DomainList,
    VariableData,
    SimulationMetaData,
    Coords,
    LastDummy
};

#define HANDLE_TYPE_TO_FREE_FUNC(type) &simv2_##type##_free
typedef int (*VisitFreeFunction)(visit_handle);
constexpr VisitFreeFunction freeFunctions[static_cast<int>(HandleType::LastDummy)]{
    HANDLE_TYPE_TO_FREE_FUNC(CurvilinearMesh),
    HANDLE_TYPE_TO_FREE_FUNC(UnstructuredMesh),
    HANDLE_TYPE_TO_FREE_FUNC(RectilinearMesh),
    HANDLE_TYPE_TO_FREE_FUNC(DomainList),
    HANDLE_TYPE_TO_FREE_FUNC(VariableData),
    HANDLE_TYPE_TO_FREE_FUNC(SimulationMetaData),
    nullptr //coords are freed with their parent object
};


namespace detail {
template<HandleType T>
class visit_smart_handle {
public:
    visit_smart_handle() = default;
    visit_smart_handle(const visit_handle &h): m_handle(h) {}
    visit_smart_handle(const visit_smart_handle &) = delete;
    visit_smart_handle(visit_smart_handle &&) = delete;
    visit_smart_handle &operator=(const visit_smart_handle &) = delete;
    visit_smart_handle &operator=(visit_smart_handle &&) = delete;


    ~visit_smart_handle()
    {
        if (freeFunctions[static_cast<int>(T)]) //compiler warning before c++17
        {
            if (m_handle != VISIT_INVALID_HANDLE)
                v2check(freeFunctions[static_cast<int>(T)], m_handle);
        }
    }
    operator visit_handle() const noexcept { return m_handle; };
    const visit_handle *operator&() const noexcept { return &m_handle; };
    visit_handle *operator&() noexcept { return &m_handle; };
    operator int() const noexcept { return m_handle; };

private:
    visit_handle m_handle = VISIT_INVALID_HANDLE;
};
} // namespace detail

template<HandleType T>
class visit_smart_handle {
public:
    visit_smart_handle(): m_handle(new detail::visit_smart_handle<T>{}) {}
    visit_smart_handle(const visit_handle &h): m_handle(new detail::visit_smart_handle<T>{h}) {}
    visit_smart_handle &operator=(const visit_handle &h)
    {
        m_handle.reset(new detail::visit_smart_handle<T>{h});
        return *this;
    }
    operator visit_handle() const noexcept { return *m_handle; };
    const visit_handle *operator&() const noexcept { return &(*m_handle); };
    visit_handle *operator&() noexcept { return &(*m_handle); };
    operator int() const noexcept { return *m_handle; };

private:
    std::shared_ptr<detail::visit_smart_handle<T>> m_handle;
};

} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif // !VISTLE_LIBSIM_SMART_HANDLE_H

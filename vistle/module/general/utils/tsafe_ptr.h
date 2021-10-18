#ifndef TSAFE_PTR_H
#define TSAFE_PTR_H

#include <iostream>
#include <memory>
#include <mutex>

namespace vistle {

/**
  * @brief https://www.codeproject.com/Articles/1183379/We-Make-Any-Object-Thread-safe
  *
  * @tparam T
  * @tparam mutex_t
  * @tparam x_lock_t
  */
template<typename T, typename mutex_t = std::recursive_mutex, typename x_lock_t = std::unique_lock<mutex_t>,
         typename s_lock_t = std::unique_lock<mutex_t>>
class safe_ptr {
    typedef mutex_t mtx_t;
    const std::shared_ptr<T> ptr;
    std::shared_ptr<mutex_t> mtx_ptr;

    template<typename req_lock>
    class auto_lock_t {
        T *const ptr;
        req_lock lock;

    public:
        auto_lock_t(auto_lock_t &&o): ptr(std::move(o.ptr)), lock(std::move(o.lock)) {}
        auto_lock_t(T *const _ptr, mutex_t &_mtx): ptr(_ptr), lock(_mtx) { std::cout << "lock" << '\n'; }
        ~auto_lock_t() { std::cout << "unlock" << '\n'; }
        T *operator->() { return ptr; }
        const T *operator->() const { return ptr; }
    };

    template<typename req_lock>
    class auto_lock_obj_t {
        T *const ptr;
        req_lock lock;

    public:
        auto_lock_obj_t(auto_lock_obj_t &&o): ptr(std::move(o.ptr)), lock(std::move(o.lock)) {}
        auto_lock_obj_t(T *const _ptr, mutex_t &_mtx): ptr(_ptr), lock(_mtx) { std::cout << "lock" << '\n'; }
        ~auto_lock_obj_t() { std::cout << "unlock" << '\n'; }
        template<typename arg_t>
        auto operator[](arg_t arg) -> decltype((*ptr)[arg])
        {
            return (*ptr)[arg];
        }
    };

    void lock() { mtx_ptr->lock(); }
    void unlock() { mtx_ptr->unlock(); }
    friend struct link_safe_ptrs;
    template<typename mutex_type>
    friend class std::lock_guard;
    //template< class... mutex_types > friend class std::lock_guard;    // C++17
public:
    template<typename... Args>
    safe_ptr(Args... args): ptr(std::make_shared<T>(args...)), mtx_ptr(std::make_shared<mutex_t>())
    {}

    auto_lock_t<x_lock_t> operator->() { return auto_lock_t<x_lock_t>(ptr.get(), *mtx_ptr); }
    auto_lock_obj_t<x_lock_t> operator*() { return auto_lock_obj_t<x_lock_t>(ptr.get(), *mtx_ptr); }
    const auto_lock_t<s_lock_t> operator->() const { return auto_lock_t<s_lock_t>(ptr.get(), *mtx_ptr); }
    const auto_lock_obj_t<s_lock_t> operator*() const { return auto_lock_obj_t<s_lock_t>(ptr.get(), *mtx_ptr); }
};

} // namespace vistle
#endif

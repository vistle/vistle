#ifndef SIMV2_EXEPTION
#define SIMV2_EXEPTION
#include <exception>
#include <string>

#include <tuple>
#include <functional>

#include "VisItDataTypes.h"
namespace insitu {
//base class that lets us catch all vistleLibSimExeptions
struct VistleLibSimExeption : public std::exception {
public:
    VistleLibSimExeption();

    VistleLibSimExeption& operator<< (const std::string& msg);
    VistleLibSimExeption& operator<< (int msg);
    virtual ~VistleLibSimExeption() = default;

protected:
    std::string m_msg;
    int rank = -1, size = 0;
};

struct SimV2Exeption : public VistleLibSimExeption {
    const char* what() const noexcept override;
};


class EngineExeption : public VistleLibSimExeption {
public:
    EngineExeption(const std::string& message);
    const char* what() const noexcept override;

};

//check simV2_ functions on invalid return valuse or invalid vistle_handles and throw in case
//___________________________________________________________________________________________

template<typename T>
inline void throwIfVisit_handleInvalid(const T& val) {
    //do nothing
}

inline void throwIfVisit_handleInvalid(visit_handle val) {
    if (val == VISIT_INVALID_HANDLE || val == VISIT_ERROR) {
        throw SimV2Exeption{};
    }
}

template<typename ...Args1, typename ...Args2>
inline void v2check(std::function<int(Args1...)>fnct, Args2&& ...args) {
    auto retval = fnct(std::forward<Args2>(args)...);
    if (retval == VISIT_INVALID_HANDLE || retval == VISIT_ERROR) {
        throw SimV2Exeption{};
    }
    using tuple1 = std::tuple<Args1...>;
    using tuple2 = std::tuple<Args2...>;
    auto numArgs1 = std::tuple_size<tuple1>(); //they must be > 0, because functions without arguments are handled by the overload below
    auto numArgs2 = std::tuple_size<tuple2>();
    if (std::is_same<std::tuple_element<numArgs1 - 1, tuple1>, visit_handle&>::value) {
        auto h = std::get<numArgs2 - 1>(tuple2(args...));
        throwIfVisit_handleInvalid(h);
    }
}

template<typename ...Args1, typename ...Args2>
inline void v2check(int(fnct)(Args1...), Args2&& ...args) {
    v2check(std::function<int(Args1...)>(fnct), args...);
}

template<typename ...Args1, typename ...Args2>
inline visit_handle v2check(std::function<visit_handle(Args1...)>fnct, Args2&& ...args) {
    auto retval = fnct(std::forward<Args2>(args)...);
    if (retval == VISIT_INVALID_HANDLE || retval == VISIT_ERROR) {
        throw SimV2Exeption{};
    }
    return retval;
}

template<typename ...Args1, typename ...Args2>
inline visit_handle v2check(visit_handle(fnct)(Args1...), Args2&& ...args) {
    return v2check(std::function<visit_handle(Args1...)>(fnct), args...);
}



inline visit_handle v2check(std::function<visit_handle()>fnct) {
    auto retval = fnct();
    if (retval == VISIT_INVALID_HANDLE || retval == VISIT_ERROR) {
        throw SimV2Exeption{};
    }
    return retval;
}

inline visit_handle v2check(visit_handle(fnct)()) {
    return v2check(std::function<visit_handle()>(fnct));
}

}
#endif // !SIMV2_EXEPTION



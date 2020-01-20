#ifndef VISTLE_UTIL_PRING_H
#define VISTLE_UTIL_PRING_H

#include <iostream>
#include <mpi.h>
#include <string>


namespace print_impl {
template <typename T>
const std::string buildString(const T& t) {
    std::stringstream s;
    s << t;
    return s.str();
}

template<typename T, typename... Args>
const std::string buildString(T t, Args&&... args) // recursive variadic function
{
    std::stringstream s;
    s << t;
    s << buildString(args...);
    return s.str();

}
}
namespace vistle {



template< typename... Args>
void print(const std::string&moduleName, int rank, int size, Args&& ...args) {
    std::stringstream stream;
    stream << moduleName << "[" << rank << "/" << size << "] ";
    stream << print_impl::buildString(args...);
    std::cerr << stream.str() << std::endl;
}

template< typename... Args>
void printDebug(const std::string& moduleName, int rank, int size, Args&& ...args) {
#ifndef DEBUG
    return;
#endif // !DEBUG
    print(moduleName, rank, size, args...);
}

}


#endif // !VISTLE_UTIL_PRING_H

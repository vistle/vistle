#ifndef VISTLE_UTIL_PRINT_H
#define VISTLE_UTIL_PRINT_H
#include <iostream>
namespace vistle {
/*use this to easily switch off prints e.g the following code will do nothing:

DoNotPrintInstance << "hello world" << std::endl;

This is useful in functions that only want to print under certain circumstances e.g:
#ifdef X
#define CERR std::cerr
#else
#define CERR DoNotPrintInstance
#endif

*/
struct DoNotPrint {
public:
    template<typename T>
    constexpr const DoNotPrint &operator<<(const T &t) const
    {
        return *this;
    }

    constexpr const DoNotPrint &operator<<(std::ostream &(*pManip)(std::ostream &)) const { return *this; }
};
static const DoNotPrint DoNotPrintInstance;

/*
    prints to stream if condition is true
*/

struct Print_if {
public:
    Print_if(bool b, std::ostream &stream = std::cerr): m_b(b), m_stream(stream) {}
    template<typename T>
    Print_if &operator<<(const T &t)
    {
        if (m_b) {
            m_stream << t;
        }
        return *this;
    }

    Print_if &operator<<(std::ostream &(*pManip)(std::ostream &))
    {
        if (m_b) {
            m_stream << pManip;
        }
        return *this;
    }

private:
    bool m_b;
    std::ostream &m_stream;
};

} // namespace vistle

#endif // !DO_NOT_PRINT_H

#ifndef VISTLE_ENUM_ARRAY_H
#define VISTLE_ENUM_ARRAY_H
#include <array>
#include <initializer_list>

namespace vistle {
//an array with an entry of type T for each type in Enum (last type must be named LastDummy and is not included)
template<typename T, typename Enum>
class EnumArray {
private:
    typedef std::array<T, static_cast<int>(Enum::LastDummy)> ArrayType;
    ArrayType m_array;

public:
    template<typename... Args>
    EnumArray(Args &&...args): m_array({std::forward<Args>(args)...})
    {}
    inline T &operator[](Enum e) { return m_array[static_cast<int>(e)]; }
    inline const T &operator[](Enum e) const { return m_array[static_cast<int>(e)]; }

    inline typename ArrayType::iterator begin() { return m_array.begin(); }
    inline typename ArrayType::const_iterator begin() const { return m_array.begin(); }
    inline typename ArrayType::iterator end() { return m_array.end(); }
    inline typename ArrayType::const_iterator end() const { return m_array.end(); }
};
} // namespace vistle

#endif //VISTLE_ENUM_ARRAY_H

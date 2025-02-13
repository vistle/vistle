#ifndef VISTLE_UTIL_VECSTREAMBUF_H
#define VISTLE_UTIL_VECSTREAMBUF_H

#include <streambuf>
#include <vector>
#include <cstring>
#include <vistle/util/allocator.h>

namespace vistle {

template<typename Vector, typename TraitsT = std::char_traits<typename Vector::value_type>>
class vecostreambuf: public std::basic_streambuf<typename Vector::value_type, TraitsT> {
public:
    vecostreambuf() {}

    int overflow(int ch)
    {
        if (ch != EOF) {
            m_vector.push_back(ch);
            return 0;
        } else {
            return EOF;
        }
    }

    std::streamsize xsputn(const typename Vector::value_type *s, std::streamsize num)
    {
        size_t oldsize = m_vector.size();
        if (oldsize + num > m_vector.capacity()) {
            m_vector.reserve(std::max(2 * (oldsize + num), m_vector.capacity() * 2));
        }
        m_vector.resize(oldsize + num);
        memcpy(m_vector.data() + oldsize, s, num);
        return num;
    }

    std::size_t write(const void *ptr, std::size_t size)
    {
        return xsputn(static_cast<const typename Vector::value_type *>(ptr), size);
    }

    const Vector &get_vector() const { return m_vector; }

    Vector &get_vector() { return m_vector; }

private:
    Vector m_vector;
};


template<typename Vector, typename TraitsT = std::char_traits<typename Vector::value_type>>
class vecistreambuf: public std::basic_streambuf<typename Vector::value_type, TraitsT> {
public:
    vecistreambuf(const Vector &ve): vec(ve)
    {
        auto &v = const_cast<Vector &>(vec);
        this->setg(v.data(), v.data(), v.data() + v.size());
    }

    std::size_t read(void *ptr, std::size_t size)
    {
        if (cur + size > vec.size())
            size = vec.size() - cur;
        memcpy(ptr, vec.data() + cur, size);
        cur += size;
        return size;
    }

    bool empty() const { return cur == 0; }
    typename Vector::value_type peekch() const { return vec[cur]; }
    typename Vector::value_type getch() { return vec[cur++]; }
    void ungetch(char)
    {
        if (cur > 0)
            --cur;
    }

private:
    const Vector &vec;
    size_t cur = 0;
};


} // namespace vistle
#endif

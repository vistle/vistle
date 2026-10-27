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


template<typename Char, typename TraitsT = std::char_traits<Char>>
class vecistreambuf: public std::basic_streambuf<Char, TraitsT> {
public:
    vecistreambuf(const Char *data, size_t size): data(data), size(size)
    {
        auto d = const_cast<Char *>(data);
        this->setg(d, d, d + size);
    }

    template<class Span>
    vecistreambuf(const Span &sp): vecistreambuf(sp.data(), sp.size())
    {}

    std::size_t read(void *ptr, std::size_t s)
    {
        if (cur + s > size)
            s = size - cur;
        memcpy(ptr, data + cur, s);
        cur += s;
        return s;
    }

    bool empty() const { return cur == 0; }
    Char peekch() const { return *(data + cur); }
    Char getch() { return *(data + cur++); }
    void ungetch(char)
    {
        if (cur > 0)
            --cur;
    }

private:
    const Char *data = nullptr;
    const size_t size = 0;
    size_t cur = 0;
};


} // namespace vistle
#endif

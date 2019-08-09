#ifndef VECSTREAMBUF_H
#define VECSTREAMBUF_H

#include <streambuf>
#include <vector>
#include <cstring>
#include <util/allocator.h>

namespace vistle {

template<typename CharT, typename TraitsT = std::char_traits<CharT>, typename allocator = default_init_allocator<CharT>>
class vecostreambuf: public std::basic_streambuf<CharT, TraitsT> {

 public:
   vecostreambuf() {
   }

   int overflow(int ch) {
      if (ch != EOF) {
         m_vector.push_back(ch);
         return 0;
      } else {
         return EOF;
      }
   }

   std::streamsize xsputn (const CharT *s, std::streamsize num) {
      size_t oldsize = m_vector.size();
      if (oldsize+num > m_vector.capacity()) {
          m_vector.reserve(std::max(2*(oldsize+num), m_vector.capacity()*2));
      }
      m_vector.resize(oldsize+num);
      memcpy(m_vector.data()+oldsize, s, num);
      return num;
   }

   std::size_t write(const void *ptr, std::size_t size) {
       return xsputn(static_cast<const CharT *>(ptr), size);
   }

   const std::vector<CharT, allocator> &get_vector() const {
      return m_vector;
   }

   std::vector<CharT, allocator> &get_vector() {
      return m_vector;
   }

 private:
   std::vector<CharT, allocator> m_vector;
};

template<typename CharT, typename TraitsT = std::char_traits<CharT>, typename allocator = default_init_allocator<CharT>>
class vecistreambuf: public std::basic_streambuf<CharT, TraitsT> {

 public:
     vecistreambuf(const std::vector<CharT, allocator> &vec)
   : vec(vec)
   {
      auto &v = const_cast<std::vector<CharT, allocator> &>(vec);
      this->setg(v.data(), v.data(), v.data()+v.size());
   }

   std::size_t read(void *ptr, std::size_t size) {
       if (cur+size > vec.size())
           size = vec.size()-cur;
       memcpy(ptr, vec.data()+cur, size);
       cur += size;
       return size;
   }

   bool empty() const { return cur==0; }
   CharT peekch() const { return vec[cur]; }
   CharT getch() { return vec[cur++]; }
   void ungetch(char) { if (cur>0) --cur; }

private:
   const std::vector<CharT, allocator> &vec;
   size_t cur = 0;
};

} // namespace vistle
#endif

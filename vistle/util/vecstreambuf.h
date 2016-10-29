#ifndef VECSTREAMBUF_H
#define VECSTREAMBUF_H

#include <streambuf>
#include <vector>

namespace vistle {

template<typename CharT, typename TraitsT = std::char_traits<CharT> >
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
      m_vector.resize(oldsize+num);
      memcpy(m_vector.data()+oldsize, s, num);
      return num;
   }

   const std::vector<CharT> &get_vector() const {
      return m_vector;
   }

 private:
   std::vector<CharT> m_vector;
};

template<typename CharT, typename TraitsT = std::char_traits<CharT> >
class vecistreambuf: public std::basic_streambuf<CharT, TraitsT> {

 public:
   vecistreambuf(const std::vector<CharT> &vec) {
      auto &v = const_cast<std::vector<CharT> &>(vec);
      this->setg(v.data(), v.data(), v.data()+v.size());
   }
};

} // namespace vistle
#endif

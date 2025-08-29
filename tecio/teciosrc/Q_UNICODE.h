 #if !defined Q_UNICODE_H_
 # define Q_UNICODE_H_
 #if defined EXTERN
 #undef EXTERN
 #endif
 #if defined Q_UNICODEMODULE
 #define EXTERN
 #else
 #define EXTERN extern
 #endif
namespace tecplot { EXTERN ___372 ___2071(uint8_t ch); EXTERN ___372 ___2070(uint8_t ch); EXTERN ___372 ___2069(uint8_t ch); EXTERN ___372 ___2049(wchar_t wChar); EXTERN void ___490(); EXTERN ___372 ___2035(const char *S); EXTERN ___372 ___2035(tecplot::___4216 ___4227); EXTERN ___372 ___2014(const char *S); EXTERN ___372 ___2014(tecplot::___4216 S); EXTERN ___372 ___2014(const wchar_t* S);
 #if defined MSWIN
EXTERN std::string  ___2321(std::string& ___3810); EXTERN void ___2622(); EXTERN char *getenv(const char *str);
 #endif
}
 #endif

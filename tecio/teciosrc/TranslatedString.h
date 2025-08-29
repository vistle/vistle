 #ifndef TECPLOT_STRUTIL_TRANSLATEDSTRING
 #define TECPLOT_STRUTIL_TRANSLATEDSTRING
#include "ThirdPartyHeadersBegin.h"
#include <string>
#include "ThirdPartyHeadersEnd.h"
 #if defined MSWIN
 #pragma warning(disable : 4181)
 #endif
namespace tecplot { class ___4216 { public: typedef enum { ___1096, ___1102 } ___2503; ___4216(); static ___4216 ___2763(); static ___4216 ___4215(char const* str, char const* ___4217 = NULL); static ___4216 ___1095(char const* str); virtual ~___4216(); virtual bool ___2033() const; virtual bool ___2034() const; virtual char const* c_str() const;
 #if defined MSWIN && !defined MAKEARCHIVE
virtual wchar_t const* ___796() const;
 #endif
virtual operator std::string();
 #if defined MSWIN && !defined MAKEARCHIVE
virtual operator std::wstring();
 #endif
virtual ___4216& operator=(___4216 const& ___2886); ___4216(___4216 const& ___2886);
 #if !defined NO_ASSERTS
virtual bool ___2065() const;
 #endif 
private: ___4216(___4216::___2503 ___2502, char const*            str, char const*            ___4217); void ___1930(___4216::___2503 ___2502, char const*            str, char const*            ___4217); ___4216::___2503  ___2491; bool                    ___2485; std::string             ___2621; mutable std::string*    ___2664;
 #if defined MSWIN
mutable std::wstring*   ___2673;
 #endif
}; inline ___4216 ___4215(char const* str, char const* ___4217 = NULL) { return ___4216::___4215(str, ___4217); } inline ___4216 ___1095(char const* str) { return ___4216::___1095(str); } inline ___4216 ___1095(std::string const& str) { return ___4216::___1095(str.c_str()); } }
 #endif

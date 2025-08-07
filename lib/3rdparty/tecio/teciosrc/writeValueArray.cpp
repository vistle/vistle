#include "writeValueArray.h"
#include "ThirdPartyHeadersBegin.h"
#include <utility>
#include "ThirdPartyHeadersEnd.h"
#include "AsciiOutputInfo.h"
#include "SzlFileLoader.h"
namespace tecplot { namespace ___3931 {
 #if defined ASCII_ANNOTATE_TYPES
template <typename T> struct AsciiTypeString; template <> struct AsciiTypeString<char>          { static char const* typeString; }; template <> struct AsciiTypeString<uint8_t>       { static char const* typeString; }; template <> struct AsciiTypeString<int16_t>       { static char const* typeString; }; template <> struct AsciiTypeString<uint16_t>      { static char const* typeString; }; template <> struct AsciiTypeString<int32_t>       { static char const* typeString; }; template <> struct AsciiTypeString<uint32_t>      { static char const* typeString; }; template <> struct AsciiTypeString<uint64_t>      { static char const* typeString; }; template <> struct AsciiTypeString<int64_t>       { static char const* typeString; }; template <> struct AsciiTypeString<float>         { static char const* typeString; }; template <> struct AsciiTypeString<double>        { static char const* typeString; }; template <> struct AsciiTypeString<StdPairUInt8>  { static char const* typeString; }; template <> struct AsciiTypeString<StdPairInt16>  { static char const* typeString; }; template <> struct AsciiTypeString<StdPairInt32>  { static char const* typeString; }; template <> struct AsciiTypeString<StdPairFloat>  { static char const* typeString; }; template <> struct AsciiTypeString<StdPairDouble> { static char const* typeString; }; char const* AsciiTypeString<char>::typeString          = "char"; char const* AsciiTypeString<uint8_t>::typeString       = "uint8_t"; char const* AsciiTypeString<int16_t>::typeString       = "int16_t"; char const* AsciiTypeString<uint16_t>::typeString      = "uint16_t"; char const* AsciiTypeString<int32_t>::typeString       = "int32_t"; char const* AsciiTypeString<uint32_t>::typeString      = "uint32_t"; char const* AsciiTypeString<uint64_t>::typeString      = "uint64_t"; char const* AsciiTypeString<int64_t>::typeString       = "int64_t"; char const* AsciiTypeString<float>::typeString         = "float"; char const* AsciiTypeString<double>::typeString        = "double"; char const* AsciiTypeString<StdPairUInt8>::typeString  = "2*uint8_t"; char const* AsciiTypeString<StdPairInt16>::typeString  = "2*int16_t"; char const* AsciiTypeString<StdPairInt32>::typeString  = "2*int32_t"; char const* AsciiTypeString<StdPairFloat>::typeString  = "2*float"; char const* AsciiTypeString<StdPairDouble>::typeString = "2*double";
 #endif
template ___372 encodeAsciiValue<uint32_t, false, 0>(char *str, int ___418, uint32_t const& ___4296);
 #define SPECIALIZE_MIN_MAX_ENCODING_FOR_TYPE(T) \
 template<> \
 ___372 encodeAsciiValue<std::pair<T, T>, false, 0>(char* str, int ___418, std::pair<T, T> const& minMax) \
 { \
 REQUIRE(VALID_REF(str)); \
 REQUIRE(___418 > 0); \
 REQUIRE(minMax.first == minMax.first); \
 REQUIRE(minMax.second == minMax.second); \
 std::string ___1472 = std::string(___199<T, false>::___1472) + " " + ___199<T, false>::___1472; \
 int nChars = snprintf(str, ___418, ___1472.c_str(), \
 -___199<T, false>::size, minMax.first, -___199<T, false>::size, minMax.second); \
 ___372 ___2037 = (nChars > 0 && nChars < ___418); \
 ENSURE(VALID_BOOLEAN(___2037)); \
 return ___2037; \
 }
SPECIALIZE_MIN_MAX_ENCODING_FOR_TYPE(uint8_t) SPECIALIZE_MIN_MAX_ENCODING_FOR_TYPE(int16_t) SPECIALIZE_MIN_MAX_ENCODING_FOR_TYPE(int32_t) SPECIALIZE_MIN_MAX_ENCODING_FOR_TYPE(float) SPECIALIZE_MIN_MAX_ENCODING_FOR_TYPE(double) template <typename T, bool ___2023, int base> ___372 ___4560(FileWriterInterface& file, char const*          ___970, ___81           ___1249, size_t               ___2795, T const*             ___4297, size_t               ___4331  ) { ___372 ___2037 = ___4224; REQUIRE(file.___2039()); REQUIRE(VALID_DESCRIPTION(___970)); REQUIRE("extraID could have any value. NO_EXTRA_ID show only the description"); REQUIRE(VALID_REF(___4297)); if (___2795 > 0) { if ( file.___2000() ) { ASSERT_ONLY(___1391 beginningLocation = file.fileLoc()); std::string ___1416 = ___970; if ( ___1249 != ___2743 ) ___1416 += ___4185(___1249+1);
 #if defined ASCII_ANNOTATE_TYPES
___1416.append(1, ' ').append(AsciiTypeString<T>::typeString);
 #endif
if ( ___2795 == 1 ) file.fprintf("%*s  ", -___206, ___1416.c_str()); else file.fprintf("%*s\r\n", -___206, ___1416.c_str()); const int buffSize = 100; char buff[buffSize]; std::string outputBuffer; for ( size_t pos = 1; pos <= ___2795; ++pos ) { ___2037 = ___2037 && encodeAsciiValue<T, ___2023, base>(buff, buffSize, ___4297[pos-1]); outputBuffer.append(buff); if( (pos % ___4331) == 0 || (pos == ___2795 ) ) outputBuffer.append("\r\n"); else outputBuffer.append("  "); } file.fwrite(outputBuffer.c_str(), sizeof(char), outputBuffer.size()); ASSERT_ONLY(___1391 endingLocation = file.fileLoc()); ASSERT_ONLY(___1391 outputSize = (___1391)___206 + 2 + outputBuffer.size()); ___476(endingLocation - beginningLocation == outputSize); } else { file.fwrite(___4297, sizeof(T), ___2795); } } ENSURE(VALID_BOOLEAN(___2037)); return ___2037; } template <typename OutType> ___372 ___4525( FileWriterInterface&        file, char const*                 ___970, ___81                  ___1249, size_t                      ___2795, ___2477 const*               ___4297, size_t                      ___4331  ) { ___372 ___2037 = ___4224; if (___2795 > 0) { ___2238<std::pair<OutType, OutType> > outputArray; ___2037 = outputArray.alloc(___2795); if (___2037) { for (size_t i = 0; i < ___2795; ++i) { outputArray[i].first = static_cast<OutType>(___4297[i].minValue()); outputArray[i].second = static_cast<OutType>(___4297[i].maxValue()); } ___2037 = ___4560<std::pair<OutType, OutType>, false, 0>(file, ___970, ___1249, ___2795, outputArray.data(), ___4331); } } return ___2037; } template <typename T, bool ___2023> uint64_t arrayValueSizeInFile(bool ___2000) { if (___2000) return ___199<T, ___2023>::size + ASCII_SPACING_LEN; return sizeof(T); } template <typename T, bool ___2023> uint64_t arraySizeInFile(size_t ___2795, bool ___2000) { uint64_t charsPerNumber = arrayValueSizeInFile<T, ___2023>(___2000); ___476(charsPerNumber > 0); uint64_t ___3356 = static_cast<uint64_t>(___2795) * charsPerNumber; if (___2000 && ___2795 > 0) ___3356 += static_cast<uint64_t>(___206) + ASCII_SPACING_LEN; return ___3356; } template <typename T, bool ___2023> uint64_t valueSizeInFile(bool ___2000) { return arraySizeInFile<T, ___2023>(1, ___2000); }
 #define INSTANTIATE_FOR_TYPE(T, ___2023) \
 template ___372 ___4560< T, ___2023, 0 >( \
 FileWriterInterface& file, \
 char const*          ___970, \
 ___81           ___1249, \
 size_t               ___2795, \
 T const*             ___4297, \
 size_t               ___4331); \
 template uint64_t arrayValueSizeInFile< T, ___2023 > (bool ___2000); \
 template uint64_t valueSizeInFile< T, ___2023 > (bool ___2000); \
 template uint64_t arraySizeInFile< T, ___2023 > (size_t ___2795, bool ___2000);
INSTANTIATE_FOR_TYPE(char, false) INSTANTIATE_FOR_TYPE(uint8_t, false) INSTANTIATE_FOR_TYPE(uint8_t, true) INSTANTIATE_FOR_TYPE(int16_t, false) INSTANTIATE_FOR_TYPE(uint16_t, false) INSTANTIATE_FOR_TYPE(uint16_t, true) INSTANTIATE_FOR_TYPE(int32_t, false) INSTANTIATE_FOR_TYPE(uint32_t, false) INSTANTIATE_FOR_TYPE(uint64_t, false) INSTANTIATE_FOR_TYPE(uint64_t, true) INSTANTIATE_FOR_TYPE(int64_t, false) INSTANTIATE_FOR_TYPE(int64_t, true) INSTANTIATE_FOR_TYPE(float, false) INSTANTIATE_FOR_TYPE(double, false) INSTANTIATE_FOR_TYPE(StdPairFloat, false) INSTANTIATE_FOR_TYPE(StdPairDouble, false) INSTANTIATE_FOR_TYPE(StdPairInt32, false) INSTANTIATE_FOR_TYPE(StdPairInt16, false) INSTANTIATE_FOR_TYPE(StdPairUInt8, false) template ___372 ___4560< uint32_t, false, 1 >(FileWriterInterface& file, char const* ___970, ___81 ___1249, size_t ___2795, uint32_t const* ___4297, size_t ___4331);
 #define INSTANTIATE_MIN_MAX_OUTPUT_FOR_TYPE(T) \
 template ___372 ___4525 <T>( \
 FileWriterInterface&        file, \
 char const*                 ___970, \
 ___81                  ___1249, \
 size_t                      ___2795, \
 ___2477 const*               ___4297, \
 size_t                      ___4331);
INSTANTIATE_MIN_MAX_OUTPUT_FOR_TYPE(uint8_t) INSTANTIATE_MIN_MAX_OUTPUT_FOR_TYPE(int16_t) INSTANTIATE_MIN_MAX_OUTPUT_FOR_TYPE(int32_t) INSTANTIATE_MIN_MAX_OUTPUT_FOR_TYPE(float) INSTANTIATE_MIN_MAX_OUTPUT_FOR_TYPE(double) }}

 #pragma once
void ___2887();
 #if !defined NO_ASSERTS && !defined NO_DEBUG_FIELDVALUES && !defined DEBUG_FIELDVALUES
 #define DEBUG_FIELDVALUES
 #endif
struct ___1360 { void*                    ___814; ___1381 ___1779; ___1382 ___3504; ZoneType_e               ___4689; FieldDataType_e          ___4332; ValueLocation_e          ___4324; ___1360( ___2225 numIPts, ___2225 numJPts, ___2225 numKPts); ~___1360(); ___2225 iDim; ___2225 jDim; ___2225 kDim; ___2225 numIPts() const; ___2225 numJPts() const; ___2225 numKPts() const; }; inline void* ___1718(___1359 ___1350) { return ___1350->___814; } inline void ___3485(___1359 ___1350, void* ___3269) { ___1350->___814 = ___3269; } inline ___1381 ___1694(___1359 ___1350) { return ___1350->___1779; } inline ___1382 ___1721(___1359 ___1350) { return ___1350->___3504; } ___2225 ___1715(___1359 ___1350); inline ValueLocation_e ___1727(___1359 ___1350) { return ___1350->___4324; } inline FieldDataType_e ___1724(___1359 ___1350) { return ___1350->___4332; } double getUniformFieldValueAdapter(___1359 ___1349, ___2225 point); double ___1740(const ___1359 ___1306, ___2225 ___3247); double ___1739(const ___1359 ___1306, ___2225 ___3247); double ___1742(const ___1359 ___1306, ___2225 ___3247); double ___1741(const ___1359 ___1306, ___2225 ___3247); double ___1738(const ___1359 ___1306, ___2225 ___3247); double ___1737(const ___1359 ___1306, ___2225 ___3247); ___1381 DetermineFieldDataGetFunction(___1359 ___1349); ___1382 DetermineFieldDataSetFunction(___1359 ___1349); inline bool ___2017(___1359 ___1350) { return ___1718(___1350) != NULL; } typedef uint32_t ___1435; typedef uint64_t ___1109; typedef uint16_t ___1959; typedef uint32_t ___1964; typedef uint64_t ___1969; inline float*       ___1688(___1359 ___1306)     { return (float*)      ___1718(___1306); } inline ___1435*  ___1691(___1359 ___1306)  { return (___1435*) ___1718(___1306); } inline double*      ___1682(___1359 ___1306)    { return (double*)     ___1718(___1306); } inline ___1109* ___1685(___1359 ___1306) { return (___1109*)___1718(___1306); } inline int64_t*     ___1709(___1359 ___1306)     { return (int64_t*)    ___1718(___1306); } inline ___1969*  ___1712(___1359 ___1306)  { return (___1969*) ___1718(___1306); } inline int32_t*     ___1703(___1359 ___1306)     { return (int32_t*)    ___1718(___1306); } inline ___1964*  ___1706(___1359 ___1306)  { return (___1964*) ___1718(___1306); } inline int16_t*     ___1697(___1359 ___1306)     { return (int16_t*)    ___1718(___1306); }
inline ___1959*  ___1700(___1359 ___1306)  { return (___1959*) ___1718(___1306); } inline uint8_t*     ___1679(___1359 ___1306)      { return (uint8_t*)    ___1718(___1306); } inline uint16_t*    ___1670(___1359 ___1306)     { return (uint16_t*)   ___1718(___1306); } inline uint32_t*    ___1673(___1359 ___1306)     { return (uint32_t*)   ___1718(___1306); } inline uint64_t*    ___1676(___1359 ___1306)     { return (uint64_t*)   ___1718(___1306); } inline void*        ___1730(___1359 ___1306)      { return               ___1718(___1306); } ___1359 ___28(___2225       ___2840, FieldDataType_e ___4234, ___372       ___3569); void ___936(___1359 *___3447); void ___432(___1359  ___1351, double       *min_ptr, double       *max_ptr, ___2225     ___3682, ___1927 *___1926); void ___677( FieldDataType_e ___4332, void*           ___1120, ___2225       ___1125, void const*     ___3655, ___2225       ___3663, ___2225       ___3657); void ___3909(FieldDataType_e  ___4332, void            *___3655, ___2225        ___3663, ___2225        ___3657, ___2225        ___3662); void ___3910(FieldDataType_e  ___4332, void            *___3655, ___2225        ___3663, ___2225        ___3657, ___2225        ___3662); void ___672(___1359 ___1119, ___2225    ___1126, ___1359 ___3654, ___2225    ___3664, ___2225    ___3658); void ___671(___1359 ___1119, ___1359 ___3654); void ___673(___1359 ___1119, ___2225    ___1122, ___1359 ___3654, ___2225    ___3659); void SetFieldDataArrayBytesToZero(___1359 ___1306); void ___3484(___1359 ___1351); inline double ___1733(___1359 ___1306, ___2225 ___3247) {
 #if !defined GET_FIELD_VALUE_BY_VIRTUAL_FUNCTION && \
 !defined GET_FIELD_VALUE_BY_FLOAT_ONLY_TEST && \
 !defined GET_FIELD_VALUE_BY_DOUBLE_ONLY_TEST && \
 !defined GET_FIELD_VALUE_BY_FLOAT_AND_DOUBLE_TEST
 #if !defined NO_ASSERTS || defined DEBUG_FIELDVALUES
 #define GET_FIELD_VALUE_BY_VIRTUAL_FUNCTION
 #else
 #define GET_FIELD_VALUE_BY_FLOAT_AND_DOUBLE_TEST
 #endif
 #endif
 #if defined GET_FIELD_VALUE_BY_VIRTUAL_FUNCTION
return ___1694(___1306)(___1306,___3247);
 #elif defined GET_FIELD_VALUE_BY_FLOAT_ONLY_TEST
return ___1694(___1306) == ___1740 ? ___1688(___1306)[___3247] : ___1694(___1306)(___1306,___3247);
 #elif defined GET_FIELD_VALUE_BY_DOUBLE_ONLY_TEST
return ___1694(___1306) == ___1739 ? ___1682(___1306)[___3247] : ___1694(___1306)(___1306,___3247);
 #elif defined GET_FIELD_VALUE_BY_FLOAT_AND_DOUBLE_TEST
return ___1694(___1306) == ___1740 ? ___1688(___1306)[___3247] : (___1694(___1306) == ___1739 ? ___1682(___1306)[___3247] : ___1694(___1306)(___1306,___3247));
 #else
 #error "Need to define one of GET_FIELD_VALUE_BY_XXX constants"
 #endif
} inline void ___3488(___1359 ___1306, ___2225 ___3247, double ___4296) { ___1721(___1306)(___1306,___3247,___4296); }
 #if defined _DEBUG
 #define USEFUNCTIONSFORNODEVALUES
 #endif
___372 ___1353( ___1359 ___1350, ___372    ___3569); void ___1356(___1359 ___1350); void ___1355(___1359 *___1350, ___372     ___1101);

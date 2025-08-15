 #pragma once
 #define _GLOBAL_H 
 #if !defined _MASTER_H_ && defined TECPLOTKERNEL
 #error "Must include MASTER.h before including GLOBAL.h"
 #endif
#include "ItemAddress.h"
#include "StandardIntegralTypes.h"
 #if defined MSWIN
 #define _USE_MATH_DEFINES 
#include <math.h> 
 #else
#include <cmath> 
 #endif
#include <limits>
 #if defined EXTERN
 #undef EXTERN
 #endif
 #define EXTERN extern
 #define EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
 #if defined ___4224
 #undef ___4224
 #endif
 #if defined ___1303
 #undef ___1303
 #endif
 #if defined MIN
 #undef MIN
 #endif
 #if defined MAX
 #undef MAX
 #endif
 #if defined ___3417
 #undef ___3417
 #endif
 #if defined ___3418
 #undef ___3418
 #endif
 #if defined ___4225
 #undef ___4225
 #endif
 #define ___4224                  ((___372)1)
 #define ___1303                 ((___372)0)
 #if defined MSWIN
 #define STDABSINT64 _abs64
 #else
 #define STDABSINT64 std::abs
 #endif
 #define ___1(X)                ((X) >= 0 ? (X) : -(X) )
 #define MAX(X,Y)              ((X) > (Y) ? (X) : (Y) )
 #define MIN(X,Y)              ((X) < (Y) ? (X) : (Y) )
 #define ___352(X)      ((X) == ___4452 ? ___364 : ___4452)
 #define ___3421(X)      ((BYTE)((X)+0.499))
 #define ___3420(X)             ((short)((X)+0.499))
 #define ROUND32(X)            ((int32_t)((X)+0.499))
 #define ___3419(X)             ((___2225)((X)+0.499))
 #define ___3418(X)             ((X) >= 0 ? ((int)((X)+0.499)) : ((int)((X)-0.499)))
 #define ___4225(X)              ((short) (X))
 #define ___3265(___3264)       (180.*(___3264)/M_PI)
 #define ___953(___951)       (M_PI*(___951)/180.)
 # define ___442(C) ((char)(('a'<=(C)&&(C)<='z') ? ((C)+('A'-'a')) : (C))) 
 #define ___2015(S)      ( ((const char*)(S))[0] == '\0' )
 #define ___2080(C)       ((C == ' ') || (C == '\t') || (C == '\n'))
 #define ___2053(C)        ((C == ' ') || (C == '\t') || (C == ','))
 #define ___486(value,___2322,___1828) ((value)<(___2322) ? (___2322) : (value) > (___1828) ? (___1828) : (value))
 #define ___1972(n, d) (((int)(n)+(int)(d)-1)/(int)(d))
 #define ___1852(___799,___1830,___2104,___2133) ((___1830) + \
 ((___2104)*(___799)->___2807) + \
 ((___2133)*(___799)->___2805))
 #define ___1840(___799,N) ((N) % (___799)->___2807)
 #define ___2110(___799,N) (((N) % (___799)->___2805)/(___799)->___2807)
 #define ___2155(___799,N) ((N)/(___799)->___2805)
 #define ___1925(index,___3601) ((___3601) == 1 || ((index) % (___3601) == 0))
 #define ___3911(___4234,A,B)      do {___4234 T = (A); (A) = (B); (B) = T;} while (___1303)
 #define ___3912(A,B)   ___3911(double, (A), (B))
 #define ___1480(x)          (___372)((x) > 0)
 #define ___1805(F)      (((F)->___3110 == ___3113))
 #if defined ___4276
 #undef ___4276
 #endif
 #define ___4276(___2969) (void)___2969
 #define ___648(___4296) \
 ( (___4296) >= ___3630 \
 ? ( (___4296) < ___2178 \
 ? (float)(___4296) \
 : (float)___2178 \
 ) \
 : ( (___4296) <= -___3630  \
 ? ( (___4296) > -___2178 \
 ? (float)(___4296) \
 : (float)-___2178 \
 ) \
 : (float)0.0 \
 ) \
 )
 #define ___487(___4296) \
 ( (___4296) >= ___3626 \
 ? ( (___4296) < ___2177 \
 ? (double)(___4296) \
 : (double)___2177 \
 ) \
 : ( (___4296) <= -___3626  \
 ? ( (___4296) > -___2177 \
 ? (double)(___4296) \
 : (double)-___2177 \
 ) \
 : (double)0.0 \
 ) \
 )
 #define ___650(___4296) \
 ( (___4296) >= 1.0 \
 ? ( (___4296) < ___2180 \
 ? (int32_t)(___4296) \
 : (int32_t)___2180 \
 ) \
 : ( (___4296) <= -1.0  \
 ? ( (___4296) > (int32_t)-___2180 \
 ? (int32_t)(___4296) \
 : (int32_t)-___2180 \
 ) \
 : (int32_t)0.0 \
 ) \
 )
 #define ___649(___4296) \
 ( (___4296) >= 1.0 \
 ? ( (___4296) < ___2179 \
 ? (int16_t)(___4296) \
 : (int16_t)___2179 \
 ) \
 : ( (___4296) <= -1.0  \
 ? ( (___4296) > (int16_t)-___2179 \
 ? (int16_t)(___4296) \
 : (int16_t)-___2179 \
 ) \
 : (int16_t)0.0 \
 ) \
 )
 #define CONVERT_DOUBLE_TO_UINT8(___4296) \
 ( (___4296) >= 1.0 \
 ? ( (___4296) < 255.0 \
 ? (uint8_t)(___4296) \
 : 255 \
 ) \
 : 0 \
 )
 #define ___665(___1121, ___3656) \
 do { \
   \
   \
 ((uint8_t *)(___1121))[0] = ((uint8_t *)(___3656))[0]; \
 ((uint8_t *)(___1121))[1] = ((uint8_t *)(___3656))[1]; \
 } while (___1303)
 #define ___668(___1121, ___3656) \
 do { \
   \
   \
 ((uint8_t *)(___1121))[0] = ((uint8_t *)(___3656))[1]; \
 ((uint8_t *)(___1121))[1] = ((uint8_t *)(___3656))[0]; \
 } while (___1303)
 #define ___666(___1121, ___3656) \
 do { \
   \
   \
 ((uint8_t *)(___1121))[0] = ((uint8_t *)(___3656))[0]; \
 ((uint8_t *)(___1121))[1] = ((uint8_t *)(___3656))[1]; \
 ((uint8_t *)(___1121))[2] = ((uint8_t *)(___3656))[2]; \
 ((uint8_t *)(___1121))[3] = ((uint8_t *)(___3656))[3]; \
 } while (___1303)
 #define ___669(___1121, ___3656) \
 do { \
   \
   \
 ((uint8_t *)(___1121))[0] = ((uint8_t *)(___3656))[3]; \
 ((uint8_t *)(___1121))[1] = ((uint8_t *)(___3656))[2]; \
 ((uint8_t *)(___1121))[2] = ((uint8_t *)(___3656))[1]; \
 ((uint8_t *)(___1121))[3] = ((uint8_t *)(___3656))[0]; \
 } while (___1303)
 #define ___667(___1121, ___3656) \
 do { \
   \
   \
 ((uint8_t *)(___1121))[0] = ((uint8_t *)(___3656))[0]; \
 ((uint8_t *)(___1121))[1] = ((uint8_t *)(___3656))[1]; \
 ((uint8_t *)(___1121))[2] = ((uint8_t *)(___3656))[2]; \
 ((uint8_t *)(___1121))[3] = ((uint8_t *)(___3656))[3]; \
 ((uint8_t *)(___1121))[4] = ((uint8_t *)(___3656))[4]; \
 ((uint8_t *)(___1121))[5] = ((uint8_t *)(___3656))[5]; \
 ((uint8_t *)(___1121))[6] = ((uint8_t *)(___3656))[6]; \
 ((uint8_t *)(___1121))[7] = ((uint8_t *)(___3656))[7]; \
 } while (___1303)
 #define ___670(___1121, ___3656) \
 do { \
   \
   \
 ((uint8_t *)(___1121))[0] = ((uint8_t *)(___3656))[7]; \
 ((uint8_t *)(___1121))[1] = ((uint8_t *)(___3656))[6]; \
 ((uint8_t *)(___1121))[2] = ((uint8_t *)(___3656))[5]; \
 ((uint8_t *)(___1121))[3] = ((uint8_t *)(___3656))[4]; \
 ((uint8_t *)(___1121))[4] = ((uint8_t *)(___3656))[3]; \
 ((uint8_t *)(___1121))[5] = ((uint8_t *)(___3656))[2]; \
 ((uint8_t *)(___1121))[6] = ((uint8_t *)(___3656))[1]; \
 ((uint8_t *)(___1121))[7] = ((uint8_t *)(___3656))[0]; \
 } while (___1303)
 #define ___3363(___417) \
 do { \
 uint8_t ___423 = ((uint8_t *)(___417))[0]; \
 ___476(sizeof(*(___417))==1 || sizeof(*(___417))==2); \
 ((uint8_t *)(___417))[0] = ((uint8_t *)(___417))[1]; \
 ((uint8_t *)(___417))[1] = ___423; \
 } while (___1303)
 #define ___3364(___417) \
 do { \
 uint16_t ___826 = ((uint16_t *)(___417))[0]; \
 ___476(sizeof(*(___417))==1 || sizeof(*(___417))==2); \
 ((uint16_t *)(___417))[0] = (((___826)<<8) | \
 ((___826&0xff))); \
 } while (___1303)
 #define ___3362 ___3363
 #define ___3366(___417) \
 do { \
 uint8_t ___423 = ((uint8_t *)(___417))[0]; \
 uint8_t ___424 = ((uint8_t *)(___417))[1]; \
 ___476(sizeof(*(___417))==1 || sizeof(*(___417))==4); \
 ((uint8_t *)(___417))[0] = ((uint8_t *)(___417))[3]; \
 ((uint8_t *)(___417))[1] = ((uint8_t *)(___417))[2]; \
 ((uint8_t *)(___417))[2] = ___424; \
 ((uint8_t *)(___417))[3] = ___423; \
 } while (___1303)
 #define ___3367(___417) \
 do { \
 uint32_t ___826 = *((uint32_t *)(___417)); \
 ___476(sizeof(*(___417))==1 || sizeof(*(___417))==4); \
 *((uint32_t *)(___417)) = (((___826)<<24)            | \
 ((___826&0x0000ff00)<<8)  | \
 ((___826&0x00ff0000)>>8)  | \
 ((___826)>>24)); \
 } while (___1303)
 #if defined MSWIN
 #define ___3365 ___3367
 #else
 #define ___3365 ___3366
 #endif
 #define ___3369(___417) \
 do { \
 uint8_t ___423 = ((uint8_t *)(___417))[0]; \
 uint8_t ___424 = ((uint8_t *)(___417))[1]; \
 uint8_t ___425 = ((uint8_t *)(___417))[2]; \
 uint8_t ___426 = ((uint8_t *)(___417))[3]; \
 ___476(sizeof(*(___417))==1 || sizeof(*(___417))==8); \
 ((uint8_t *)(___417))[0] = ((uint8_t *)(___417))[7]; \
 ((uint8_t *)(___417))[1] = ((uint8_t *)(___417))[6]; \
 ((uint8_t *)(___417))[2] = ((uint8_t *)(___417))[5]; \
 ((uint8_t *)(___417))[3] = ((uint8_t *)(___417))[4]; \
 ((uint8_t *)(___417))[4] = ___426; \
 ((uint8_t *)(___417))[5] = ___425; \
 ((uint8_t *)(___417))[6] = ___424; \
 ((uint8_t *)(___417))[7] = ___423; \
 } while (___1303)
 #define ___3370(___417) \
 do { \
 uint16_t ___827 = ((uint16_t *)(___417))[0]; \
 uint16_t ___828 = ((uint16_t *)(___417))[1]; \
 uint16_t ___829 = ((uint16_t *)(___417))[2]; \
 uint16_t ___830 = ((uint16_t *)(___417))[3]; \
 ___476(sizeof(*(___417))==1 || sizeof(*(___417))==8); \
 ((uint16_t *)(___417))[0] = (((___830)<<8) | \
 ((___830&0xff))); \
 ((uint16_t *)(___417))[1] = (((___829)<<8) | \
 ((___829&0xff))); \
 ((uint16_t *)(___417))[2] = (((___828)<<8) | \
 ((___828&0xff))); \
 ((uint16_t *)(___417))[3] = (((___827)<<8) | \
 ((___827&0xff))); \
 } while (___1303)
 #define ___3371(___417) \
 do { \
 uint32_t ___827 = ((uint32_t *)(___417))[0]; \
 uint32_t ___828 = ((uint32_t *)(___417))[1]; \
 ___476(sizeof(*(___417))==1 || sizeof(*(___417))==8); \
 ((uint32_t *)(___417))[0] = (((___828)<<24)           | \
 ((___828&0x0000ff00)<<8) | \
 ((___828&0x00ff0000)>>8) | \
 ((___828)>>24)); \
 ((uint32_t *)(___417))[1] = (((___827)<<24)           | \
 ((___827&0x0000ff00)<<8) | \
 ((___827&0x00ff0000)>>8) | \
 ((___827)>>24)); \
 } while (___1303)
 #define ___3372(___417) \
 do { \
 uint64_t ___826 = *((uint64_t *)(___417)); \
 ___476(sizeof(*(___417))==1 || sizeof(*(___417))==8); \
 *((uint64_t *)(___417)) = (((___826)<<56) | \
 ((___826&0x000000000000ff00)<<40) | \
 ((___826&0x0000000000ff0000)<<24) | \
 ((___826&0x00000000ff000000)<<8)  | \
 ((___826&0x000000ff00000000)>>8)  | \
 ((___826&0x0000ff0000000000)>>24) | \
 ((___826&0x00ff000000000000)>>40) | \
 ((___826)>>56)); \
 } while (___1303)
 #if defined MSWIN
 #define ___3368 ___3371
 #else
 #define ___3368 ___3369
 #endif
 #define STDCALL 
 #if defined (__cplusplus)
 #   define EXTERNC extern "C"
 #   define TP_GLOBAL_NAMESPACE ::
 #else
 #   define EXTERNC
 #   define TP_GLOBAL_NAMESPACE
 #endif 
//  #if defined MAKEARCHIVE
//  #define tpsdkbase_API
//  #else
// #include "tpsdkbase_Exports.h"
//  #endif
 #define ___2289 EXTERNC tpsdkbase_API
 #if defined MSWIN
 # define ___1233 EXTERNC _declspec ( dllexport )
 #else
 # define ___1233 EXTERNC __attribute__ ((visibility ("default")))
 #endif 
 #define ___1234 ___1233
 #define ___1937           ___1938
 #define ___3967 "InitTecAddOn113"
 #if defined (__cplusplus) && !defined _DEBUG
 # define ___1939 inline
 #else
 # define ___1939 static
 #endif 
 #if defined (MSWIN) ||\
 defined (___1983) ||\
 defined (LINUX) ||\
 defined (___3889) ||\
 defined (___532) ||\
 defined (DEC) ||\
 defined (__LITTLE_ENDIAN__)
 #define ___2324
 #endif
 #if defined( ___2324 )
 # define ___3908(___1975) (!(___1975))
 #else
 # define ___3908(___1975) (___1975)
 #endif
 #if defined DECALPHA   || \
 defined LINUXALPHA || \
 defined LINUX64    || \
 defined MAC64      || \
 defined ___1831  || \
 defined ___3884        || \
 defined ___1829         || \
 defined ___532
 #define ___2318
 #endif
 #define ___2183              ((size_t)-1)
 #define ___2181               (std::numeric_limits<int64_t>::max()-1)
 #define ___2180               (std::numeric_limits<int32_t>::max()-1)
 #define ___2179               (std::numeric_limits<int16_t>::max()-1)
 #define ___2182                (std::numeric_limits<int8_t>::max()-1)
 #define ___2191              (std::numeric_limits<uint64_t>::max()-1)
 #define ___2190              (std::numeric_limits<uint32_t>::max()-1)
 #define ___2189              (std::numeric_limits<uint16_t>::max()-1)
 #define ___2192               (std::numeric_limits<uint8_t>::max()-1)
 #ifdef INDEX_16_BIT
 #define ___2371                 ((___2225)___2179)
 #else
 #define ___2371                 ((___2225)(___2181>>1))
 #endif
 #define ___2389               ((___1170)___2180)
 #define ___2177              1.0e+150
 #define ___3626              1.0e-150
 #define ___2188          150
 #define ___3629         -150
 #define ___3627           ___3626
 #define ___2187    308
 #define ___3628   -307
 #define ___2186            1.0e+308
 #define ___2178               std::numeric_limits<float>::max() 
 #define ___3630               std::numeric_limits<float>::min() 
 #define ___3631            1.0e-307
 #define ___1187                      3
 #define ___2294                      0.69314718055994530942
 #define ___2293                     2.30258509299404568402
 #define ___3086                  1.57079632679489661923
 #define ___4233                    6.28318530717958647692
 #define ___58             1.0e-10
 #define ___2185             (4*M_PI+___58)
 #define ___952            57.295779513082323
 #define ___504                2.54
 #define ___3141            72.0
 #define ___1458             192
 #define ___1445         128
 #define ___1456             64
 #define ___333            ((___3491)-1)
 #define ___334           (static_cast<___1170>(-1))
 #define BAD_ZONE_OFFSET          (static_cast<___1170>(-1))
 #define BAD_FIELDMAP_OFFSET      (static_cast<___1170>(-1))
 #define ___2418      (0)
 #define ___2419       (-1)
 #define ___1989        0
 #define VALID_UNIQUEID(uniqueID) ((uniqueID) != ___1989)
 #define NO_EMBEDDED_LPK_IMAGE_NUMBER 0
 #define ___332              ___333
 #define ___3635       0
 #define ___328             (-1.0)
 #define ___2471  4
 #define ___4310(___3784) (0 <= (___3784) && (___3784) < ___2389)
 #define ___3786          (-1)
 #define ___3785         (-2)
 #if defined DEFER_TRANSIENT_OPERATIONS
 #define STRAND_ID_INVALID         (-3)
 #endif
 #define INVALID_PART_INDEX (-1)
 #define ___2345 1
 #define ___2346 6
 #define ___1985 -1
 #define ___4299(clipPlane) (0 <= clipPlane && clipPlane < ___2346)
 #define    ___2380           ___2389
 #define    ___2387                    5
 #define    ___2388                    5
 #define    ___2369              50
 #define    ___2382       720
 #define    ___2368                   2048
 #define    ___2363          10
 #define    ___2367                20000
 #define    ___2360        16
 #define    ___2381           16
 #define    ___2470      12
 #define    ___2357      360
 #define    ___2386    8
 #define    ___2361            8
 #define    ___2372         8
 #define    ___2373 3
 #define    ___2384              8
 #define    MaxNumHOESubdivisionLevels  5
 #define    ___950      15
 #define    ___946        ((int32_t)0)
 #define    ___945      ((int32_t)0)
 #define    ___949 ((int32_t)0)
 #define    ___331             ((int32_t)-1)
 #define    ___4277          ((int32_t)0)
 #define    ___327          ((int32_t)-1)
 #define ___4305(___1815) (((((int32_t)___1815) >= 0) && (((int32_t)___1815) < ___2372)))
 #define ___4309(___1815)      (((((int32_t)___1815) >= 0) && (((int32_t)___1815) < ___2384)))
 #define    ___2347  6
 #define    ___2353       256
 #define    ___2356            128
 #define    ___2354            128
 #define    ___2355        128
 #define    ___2351     32000
 #define    ___2352       1024
 #define    ___2379               16
 #define    ___2348             5
 #define    ___2358  50
 #define    ___2383     800
 #define    ___2365         100
 #define    ___2366      100
 #define    ___2359         20
 #define    ___2480     0.5
 #define    ___2474             0.25
 #define    ___2473            0.25
 #define    ___2469             0.1
 #define    ___329              255
 #define MAX_NODES_PER_CLASSIC_FACE    4
 #define MAX_NODES_PER_CLASSIC_ELEMENT 8
 #define MAX_HOE_FACE_NODES_PER_ELEM   size_t(25)  
 #define MAX_HOE_NODES_PER_ELEM        size_t(125) 
 #define MAX_NODES_PER_ELEM            size_t(125)
 #define MAX_FACE_NODES_PER_ELEM       size_t(25)
 #define ___3866 16
 #define AuxData_Common_Incompressible               "Common.Incompressible"
 #define AuxData_Common_Density                      "Common.Density"
 #define AuxData_Common_SpecificHeat                 "Common.SpecificHeat"
 #define AuxData_Common_SpecificHeatVar              "Common.SpecificHeatVar"
 #define AuxData_Common_GasConstant                  "Common.GasConstant"
 #define AuxData_Common_GasConstantVar               "Common.GasConstantVar"
 #define AuxData_Common_Gamma                        "Common.Gamma"
 #define AuxData_Common_GammaVar                     "Common.GammaVar"
 #define AuxData_Common_Viscosity                    "Common.Viscosity"
 #define AuxData_Common_ViscosityVar                 "Common.ViscosityVar"
 #define AuxData_Common_Conductivity                 "Common.Conductivity"
 #define AuxData_Common_ConductivityVar              "Common.ConductivityVar"
 #define AuxData_Common_AngleOfAttack                "Common.AngleOfAttack"
 #define AuxData_Common_SpeedOfSound                 "Common.SpeedOfSound"
 #define AuxData_Common_ReferenceU                   "Common.ReferenceU"
 #define AuxData_Common_ReferenceV                   "Common.ReferenceV"
 #define AuxData_Common_XVar                         "Common.XVar"
 #define AuxData_Common_YVar                         "Common.YVar"
 #define AuxData_Common_ZVar                         "Common.ZVar"
 #define AuxData_Common_CVar                         "Common.CVar"
 #define AuxData_Common_UVar                         "Common.UVar"
 #define AuxData_Common_VVar                         "Common.VVar"
 #define AuxData_Common_WVar                         "Common.WVar"
 #define AuxData_Common_VectorVarsAreVelocity        "Common.VectorVarsAreVelocity"
 #define AuxData_Common_PressureVar                  "Common.PressureVar"
 #define AuxData_Common_TemperatureVar               "Common.TemperatureVar"
 #define AuxData_Common_DensityVar                   "Common.DensityVar"
 #define AuxData_Common_StagnationEnergyVar          "Common.StagnationEnergyVar"
 #define AuxData_Common_MachNumberVar                "Common.MachNumberVar"
 #define AuxData_Common_ReferenceMachNumber          "Common.ReferenceMachNumber"
 #define AuxData_Common_ReferenceW                   "Common.ReferenceW"
 #define AuxData_Common_PrandtlNumber                "Common.PrandtlNumber"
 #define AuxData_Common_Axisymmetric                 "Common.Axisymmetric"
 #define AuxData_Common_AxisOfSymmetryVarAssignment  "Common.AxisOfSymmetryVarAssignment"
 #define AuxData_Common_AxisValue                    "Common.AxisValue"
 #define AuxData_Common_SteadyState                  "Common.SteadyState"
 #define AuxData_Common_TurbulentKineticEnergyVar    "Common.TurbulentKineticEnergyVar"
 #define AuxData_Common_TurbulentDissipationRateVar  "Common.TurbulentDissipationRateVar"
 #define AuxData_Common_TurbulentDynamicViscosityVar "Common.TurbulentDynamicViscosityVar"
 #define AuxData_Common_TurbulentViscosityVar        "Common.TurbulentViscosityVar"
 #define AuxData_Common_TurbulentFrequencyVar        "Common.TurbulentFrequencyVar"
 #define AuxData_Common_Gravity                      "Common.Gravity"
 #define AuxData_Common_IsBoundaryZone               "Common.IsBoundaryZone"
 #define AuxData_Common_BoundaryCondition            "Common.BoundaryCondition"
 #define AuxData_Common_Time                         "Common.Time"
 #define AuxData_Common_PartName                     "Common.PartName"
 #define AuxData_Common_Mean                         "Common.Mean"
 #define AuxData_Common_Median                       "Common.Median"
 #define AuxData_Common_Variance                     "Common.Variance"
 #define AuxData_Common_StdDev                       "Common.StdDev"
 #define AuxData_Common_AvgDev                       "Common.AvgDev"
 #define AuxData_Common_GeoMean                      "Common.GeoMean"
 #define AuxData_Common_ChiSqre                      "Common.ChiSqre"
 #define AuxData_Common_TransientZoneVisibility      "Common.TransientZoneVisibility"
 #define    ___364           ((___514)0)
 #define    ___3299             ((___514)1)
 #define    ___1808           ((___514)2)
 #define    ___366            ((___514)3)
 #define    ___797            ((___514)4)
 #define    ___4584          ((___514)5)
 #define    ___3254          ((___514)6)
 #define    ___4452           ((___514)7)
 #define    ___743         ((___514)8)
 #define    ___754         ((___514)9)
 #define    ___765         ((___514)10)
 #define    ___776         ((___514)11)
 #define    ___784         ((___514)12)
 #define    ___785         ((___514)13)
 #define    ___786         ((___514)14)
 #define    ___787         ((___514)15)
 #define    ___788         ((___514)16)
 #define    ___733         ((___514)17)
 #define    ___734         ((___514)18)
 #define    ___735         ((___514)19)
 #define    ___736         ((___514)20)
 #define    ___737         ((___514)21)
 #define    ___738         ((___514)22)
 #define    ___739         ((___514)23)
 #define    ___740         ((___514)24)
 #define    ___741         ((___514)25)
 #define    ___742         ((___514)26)
 #define    ___744         ((___514)27)
 #define    ___745         ((___514)28)
 #define    ___746         ((___514)29)
 #define    ___747         ((___514)30)
 #define    ___748         ((___514)31)
 #define    ___749         ((___514)32)
 #define    ___750         ((___514)33)
 #define    ___751         ((___514)34)
 #define    ___752         ((___514)35)
 #define    ___753         ((___514)36)
 #define    ___755         ((___514)37)
 #define    ___756         ((___514)38)
 #define    ___757         ((___514)39)
 #define    ___758         ((___514)40)
 #define    ___759         ((___514)41)
 #define    ___760         ((___514)42)
 #define    ___761         ((___514)43)
 #define    ___762         ((___514)44)
 #define    ___763         ((___514)45)
 #define    ___764         ((___514)46)
 #define    ___766         ((___514)47)
 #define    ___767         ((___514)48)
 #define    ___768         ((___514)49)
 #define    ___769         ((___514)50)
 #define    ___770         ((___514)51)
 #define    ___771         ((___514)52)
 #define    ___772         ((___514)53)
 #define    ___773         ((___514)54)
 #define    ___774         ((___514)55)
 #define    ___775         ((___514)56)
 #define    ___777         ((___514)57)
 #define    ___778         ((___514)58)
 #define    ___779         ((___514)59)
 #define    ___780         ((___514)60)
 #define    ___781         ((___514)61)
 #define    ___782         ((___514)62)
 #define    ___783         ((___514)63)
 #define    ___2660      ((___514)(-1))
 #define    ___2706         ((___514)(-2))
 #define    ___2653     ((___514)(-3))
 #define    ___2654     ((___514)(-4))
 #define    ___2655     ((___514)(-5))
 #define    ___3373        ((___514)(-6))
 #define    ___2656     ((___514)(-7))
 #define    ___2657     ((___514)(-8))
 #define    ___2658     ((___514)(-9))
 #define    ___2659     ((___514)(-10))
 #define    ___1986    ((___514)(-255))
 #define    ___1420  ___743
 #define    ___2194   ___783
 #define    ___2789   (___2194-___1420+1)
 #define    ___1418   ___364
 #define    ___2193    ___2194
 #define    ___2764    (___2193-___1418+1)
 #define    ___2870   ((___514)255)
 #define    ___1806                 (___2193+1)
 #define    ___813             (___2193+2) 
 #define    ___4568             (___2193+3)
 #define    ___1421    ___1806
 #define    ___2198     ___4568
 #define    ___2806    (___2198-___1421+1)
 #define    ___2786      (___1545.___2239.___2377+1)
 #define    ___2783 (___2764+___2806+___2786)
 #define    ___342      (0)
 #define    ___1979  (___2764)
 #define    ___612    (___2764+___2806)
 #define    ___2140           (short)31
 #define    ___2149           (short)13
 #define    ___2144              (short)27
 #define    ___2139        (short)8
 #define    ___2141        (short)127
 #define    ___2145        (short)29
 #define    ___2150       (short)30
 #define    ___2153          (short)11
 #define    ___2143        (short)10
 #define    ___2147             (short)43
 #define    ___2146            (short)45
 #define    ___2151      (short)128 
 #define    ___2152           (short)19  
 #define    ___2148         (short)18  
 #define    ___2142        (short)2   
 #define ZoneMarkerFull32Bit   299.0
 #define ZoneMarkerFace64Bit   298.0
 #define ___1615            399.0
 #define ___4110            499.0
 #define ___790     599.0
 #define ___4284         699.0
 #define ___885      799.0
 #define ___4336          899.0
 #define EndHeaderMarker       357.0
 #define    ___293          ___787+1
 #define    ___2332       ___787+2
 #define    ___2475       ___787+3
 #define    ___2337    ___787+4
 #define    ___3795    ___787+5
 #define    ___513   ___787+6
 #define    ___392      ___787+7
 #define    ___2170         ___787+8
 #define    ___2824   ___787+9
 #define    ___229    ___787+10
 #define    ___1988       ___787+99
 #define    ___1422   ___293
 #define    ___2200    ___2170
 #define    ___958           0.0001
 #define    ___326         NULL
 #ifdef MSWIN
 # define ___1088 "\\"
 #else
 # define ___1088 "/"
 #endif
 #define ___4196  fread
 #define ___4200 fwrite
 #if defined MSWIN
 #if _MSC_VER <= 1800 
 #define snprintf _snprintf
 #endif
 #define ___4194   fflush
 #define ___4193   fclose
 #define ___4204  TP_GLOBAL_NAMESPACE remove
 #define ___4202   TP_GLOBAL_NAMESPACE ___3397
 #define ___4203    TP_GLOBAL_NAMESPACE _stat
 #define ___4201  TP_GLOBAL_NAMESPACE getenv
 #if defined _WIN64
 #define ___4198(___3790,___2863,whence) TP_GLOBAL_NAMESPACE _fseeki64((___3790),(__int64)(___2863),(whence))
 #define ___4199                       TP_GLOBAL_NAMESPACE _ftelli64
 #else
 #define ___4198(___3790, ___2863, whence) TP_GLOBAL_NAMESPACE fseek((___3790), (long)(___2863), (whence))
 #define ___4199                         TP_GLOBAL_NAMESPACE ftell
 #endif
 #else
 #define FileStat_s struct _stat
 #define ___4202  TP_GLOBAL_NAMESPACE ___3396
 #define ___4204 TP_GLOBAL_NAMESPACE unlink
 #define ___4193 TP_GLOBAL_NAMESPACE fclose
 #define ___4194 TP_GLOBAL_NAMESPACE fflush
 #define ___4198  TP_GLOBAL_NAMESPACE fseeko
 #define ___4199  TP_GLOBAL_NAMESPACE ftello
 #define ___4203   TP_GLOBAL_NAMESPACE stat
 #define _stat     TP_GLOBAL_NAMESPACE stat 
 #define ___4201 TP_GLOBAL_NAMESPACE getenv
 #endif
 #if !defined MSWIN
typedef FILE* HDC; typedef uint64_t MSG; typedef void* HMODULE; typedef void* LPCWSTR; typedef bool BOOL;
 #endif
typedef    ___2225 ___2730; typedef int64_t ___90; typedef int64_t ___4262; typedef int64_t ___1396; typedef uint64_t ___2403; typedef    int32_t          ___514; typedef    int16_t          ___3879; typedef    char             ___372; typedef    char            *___4651; typedef    char            *___4364; typedef    char            *___2323; typedef    ___2225        ___1823; typedef    ___2225        ___3459[___2369]; typedef    double           BasicSize_t[___2348]; typedef    double          *___4352; typedef    ___2225        ___3491; typedef    unsigned long ___3478; typedef    ___3478 ___3481; typedef    ___3481* ___3479;
 #define ___1311                               (1L << 0) 
 #define ___1312                         (1L << 1)
 #define ___1310                               (1L << 2) 
 #define ___1336                               (1L << 3) 
 #define ___1314                        (1L << 4)
 #define ___1335                 (1L << 5)
 #define FEATURE_NUMBER_OF_FRAMES_GREATER_THAN_1  (1L << 6)
 #define FEATURE_NUMBER_OF_ZONES_GREATER_THAN_1   (1L << 7)
 #define FEATURE_NUMBER_OF_FRAMES_GREATER_THAN_5  (1L << 8)
 #define FEATURE_NUMBER_OF_ZONES_GREATER_THAN_5   (1L << 9)
 #define FEATURE_NUMBER_OF_FRAMES_GREATER_THAN_10 (1L << 10)
 #define FEATURE_NUMBER_OF_ZONES_GREATER_THAN_10  (1L << 11)
 #define FEATURE_READ_NONOEM_TECPLOT_DATA         (1L << 12) 
 #define FEATURE_FOREIGN_DATALOADERS              (1L << 13) 
 #define ___1316            (1L << 14) 
 #define ___1322                     (1L << 15) 
 #define ___1327            (1L << 16) 
 #define ___1321                 (1L << 17) 
 #define ___1332                      (1L << 18) 
 #define ___1333                (1L << 19) 
 #define ___1319                     (1L << 20) 
 #define ___1318                        (1L << 21) 
 #define ___1334                          (1L << 22) 
 #define FEATURE_FEMIXED                          (1L << 23) 
 #define ___1313                          (1L << 24) 
 #define FEATURE_DATASETSIZE                      (1L << 25) 
 #define FEATURE_RPC                              (1L << 26) 
 #define FEATURE_DATALOADERS_EXCEPT_ALLOWED       (1L << 27) 
 #define FEATURE_NUMBER_OF_PAGES_GREATER_THAN_1   (1L << 28) 
 #define FEATURE_BATCH_MODE                       (1L << 29) 
 #define FEATURE_SIMPLEZONECREATION               (1L << 30) 
 #define NUM_POSSIBLE_INHIBITED_FEATURES 31
 #define ___2161 (___1311                               |\
 ___1312                         |\
 ___1310                               |\
 ___1336                               |\
 ___1314                        |\
 ___1335                 |\
 FEATURE_NUMBER_OF_FRAMES_GREATER_THAN_1  |\
 FEATURE_NUMBER_OF_ZONES_GREATER_THAN_1   |\
 FEATURE_NUMBER_OF_FRAMES_GREATER_THAN_5  |\
 FEATURE_NUMBER_OF_ZONES_GREATER_THAN_5   |\
 FEATURE_NUMBER_OF_FRAMES_GREATER_THAN_10 |\
 FEATURE_NUMBER_OF_ZONES_GREATER_THAN_10  |\
 FEATURE_READ_NONOEM_TECPLOT_DATA         |\
 FEATURE_FOREIGN_DATALOADERS              |\
 ___1316            |\
 ___1322                     |\
 ___1327            |\
 ___1321                 |\
 ___1332                      |\
 ___1333                |\
 ___1319                     |\
 ___1318                        |\
 ___1334                          |\
 FEATURE_FEMIXED                          |\
 ___1313                          |\
 FEATURE_DATASETSIZE                      |\
 FEATURE_RPC                              |\
 FEATURE_DATALOADERS_EXCEPT_ALLOWED       |\
 FEATURE_NUMBER_OF_PAGES_GREATER_THAN_1   |\
 FEATURE_BATCH_MODE                       |\
 FEATURE_SIMPLEZONECREATION)
 #define ___4300(___1309) (((___1309) & ___2161) != 0)
 #define ___4301(___2342) (((___2342) & ~___2161)==0)
typedef    uint64_t  ___1320; typedef    uint64_t  ___1323;
 #define MAX_UTF8_BYTES 4
 #define ___3914 1 + MAX_UTF8_BYTES + 1
typedef    char             ___3915[___3914]; typedef int32_t ___1293; typedef int32_t ___1144; typedef int32_t ___1255; typedef enum { Projection_Orthographic, Projection_Perspective, END_Projection_e, Projection_Invalid = ___329 } Projection_e; typedef enum { ___3089, ___3090, ___3091, END_PlacementPlaneOrientation_e, ___3088 = ___329 } PlacementPlaneOrientation_e; typedef enum { ___3847, ___3850, ___3848, END_StringMode_e, ___3849 = ___329 } StringMode_e; typedef enum { ___3593, ___3591, END_SidebarSizing_e, ___3592 = ___329 } SidebarSizing_e; typedef enum { ___3588, ___3589,    /**@internal TP_NOTAVAILABLE*/ ___3590,      /**@internal TP_NOTAVAILABLE*/ ___3586,   /**@internal TP_NOTAVAILABLE*/ END_SidebarLocation_e, ___3587 = ___329 } SidebarLocation_e; typedef enum { ___2413, ___2416, ___2414, ___2415, END_MenuItem_e, ___2412 = ___329 } MenuItem_e; typedef enum { ___3668, ___3667, ___3677, ___3674, ___3671, ___3666, ___3669, ___3678, ___3676, ___3670, ___3665, ___3673, ___3675, END_StandardMenu_e, ___3672 = ___329 } StandardMenu_e; typedef enum { ___1379, ___1376, ___1380, ___1377, END_FieldProbeDialogPage_e, ___1378 = ___329 } FieldProbeDialogPage_e; enum BooleanCache_e { ___367, ___369, ___370, END_BooleanCache_e, ___368 = ___329 }; enum LinePickLocation_e { ___2274, ___2275, ___2273, ___2272, ___2270, END_LinePickLocation_e, ___2271 = ___329 }; enum ViewDest_e { ___4419, ___4421, END_ViewDest_e, ___4420 = ___329 }; enum DataSetReaderOrigin_e { ___899, ___897, END_DataSetReaderOrigin_e, ___898 = ___329 }; enum ExtendedCurveFitOrigin_e { ___1247, ___1245, END_ExtendedCurveFitOrigin_e, ___1246 = ___329 }; enum CollapsedStatus_e { ___511, ___507, ___506, ___508, ___509, END_CollapsedStatus_e, ___510 = ___329 }; typedef enum { ___4242, ___4247,
___4251, ___4243, ___4252, ___4253, ___4249, ___4248, ___4240, ___4241, ___4250, ___4244, ___4246, END_UndoStateCategory_e, ___4245 = ___329 } UndoStateCategory_e; typedef enum { ___2292, ___2290, END_LinkType_e, ___2291 = ___329 } LinkType_e; typedef enum { ___1507, ___1509, END_FrameCollection_e, ___1508 = ___329 } FrameCollection_e; typedef enum { SurfaceGenerationMethod_AllowQuads, SurfaceGenerationMethod_AllTriangles, SurfaceGenerationMethod_AllPolygons, SurfaceGenerationMethod_Auto, END_SurfaceGenerationMethod_e, SurfaceGenerationMethod_Invalid = ___329 } SurfaceGenerationMethod_e; typedef enum { ___2213, ___2214, ___2215, END_LegendProcess_e, ___2216 = ___329 } LegendProcess_e; typedef enum { ___3380, ___3376, ___3375, ___3379, ___3377, ___3374, END_RGBLegendOrientation_e, ___3378 = ___329 } RGBLegendOrientation_e; struct ___3389 { uint8_t  ___3263; uint8_t  G; uint8_t  B; }; typedef ___3389 ___3386[256]; typedef enum { ___3756, ___3755, ___3764, ___3763, ___3737, ___3709, ___3734, ___3747, ___3704, ___3733, ___3697, ___3717, ___3698, ___3724, ___3723, ___3745, ___3762, ___3754, ___3718, ___3716, ___3758, ___3695, ___3699, ___3746, ___3732, ___3730, ___3739, ___3740, ___3741, ___3742, ___3714, ___3751, ___3748, ___3703, ___3702, StateChange_Text, ___3711, ___3705, ___3708, ___3744, ___3743, ___3688, ___3690, ___3689, ___3757, ___3749, ___3712, ___3753, ___3752, ___3738, ___3735, ___3731, ___3710, ___3736, ___3719, ___3687,
___3686, ___3722, ___3721, ___3720, ___3765, ___3715, ___3700, ___3696, StateChange_OpenLayout, StateChange_MacroLoaded, StateChange_PerformingUndoBegin, StateChange_PerformingUndoEnd, StateChange_SolutionTimeChangeBlockEnd, StateChange_SolutionTimeClustering, END_StateChange_e, ___3713 = ___329, ___3701            = ___3714, ___3707             = ___3751, ___3706            = ___3748, ___3760            = ___3717, ___3761                  = ___3718, ___3759    = ___3716, StateChange_SolutiontimeChangeBlockEnd = StateChange_SolutionTimeChangeBlockEnd } StateChange_e; typedef enum { VarParseReturnCode_Ok, VarParseReturnCode_InvalidInteger, VarParseReturnCode_InvalidVariableName, VarParseReturnCode_VariableNotInDataSet, END_VarParseReturnCode_e, VarParseReturnCode_Invalid = ___329 } VarParseReturnCode_e; typedef enum { AnimationType_LineMap, AnimationType_Time, AnimationType_Zone, AnimationType_IJKBlanking, AnimationType_IJKPlanes, AnimationType_IsoSurfaces, AnimationType_Slices, AnimationType_ContourLevels, AnimationType_Streamtraces, END_AnimationType_e, AnimationType_Invalid = ___329 } AnimationType_e; typedef enum { ___3728, ___3729, ___3726, ___3727, END_StateChangeMode_e, ___3725 = ___329 } StateChangeMode_e; typedef enum { ___3693, ___3691, ___3692, END_StateChangeCallbackAPI_e, ___3694 = ___329 } StateChangeCallbackAPI_e; typedef enum { ___86, ___84, ___87, END_AppMode_e, ___85 = ___329 } AppMode_e; typedef enum { ___2207, ___2209, ___2206, END_LayoutPackageObject_e, ___2208 = ___329 } LayoutPackageObject_e; typedef enum { ___4353, ___4354, END_VarLoadMode_e, ___4355 = ___329 } VarLoadMode_e; typedef enum { ___1901, ___1902, END_ImageSelection_e, ___1900 = ___329 } ImageSelection_e; typedef enum { ___2230, ___2233, ___2232, END_LibraryType_e, ___2231 = ___329 } LibraryType_e;   /**@internal TP_NOPYTHON*/ typedef enum { ___220, ___223, ___222, ___224, ___219, ___215, ___216, ___218, ___217, END_AssignOp_e, ___221 = ___329
} AssignOp_e; typedef enum { ___979, ___998, ___1030, ___1086, ___1048, ___1049, ___1080, ___1045, ___1046, ___1034, ___1035, ___1055, ___1056, ___1027, ___1085, ___1043, ___1017, ___999, ___1028, ___1029, ___977, ___1066, ___1050, ___1069, ___1071, ___1067, ___1020, ___1064, ___981, ___1082, ___1084, ___1081, ___1083, ___1061, ___1059, ___1060, ___1052, ___1051, ___1025, ___1016, ___994, ___1023, ___976, ___1079, ___1041, ___990, ___1068, ___1065, ___1076, ___1053, ___983, ___985, ___984, ___996, ___1032, ___986, ___987, ___992, ___993, ___1000, ___1002, ___1003, ___1007, ___1006, ___1008, ___1009, ___1001, ___1005, ___1004, ___1024, ___1019, ___1021, ___1078, ___989, ___988, ___991, ___1038, ___1037, ___1054, ___1073, ___1072, ___1077, ___1044, ___980, ___1033, ___1063, ___1058, ___1057, ___1026, ___1014, ___1013, ___1047, ___997, ___982, ___1070, ___1074, ___1039, ___1015, ___1011, ___978, Dialog_FourierTransform, Dialog_RotateData, Dialog_AxialDuplicate, END_Dialog_e, ___1018 = ___329, ___1040 = ___1086 } Dialog_e;   /**@internal TP_NOPYTHON*/ typedef enum { ___49, ___48, ___50, ___46, ___45, ___47, ___42, ___41, ___43, END_AnchorAlignment_e, ___44 = ___329 } AnchorAlignment_e; enum PositionAtAnchor_e { ___3163, ___3164, ___3161, END_PositionAtAnchor_e, ___3162 = ___329 }; struct ___1042
{ AnchorAlignment_e  ___40; ___372          ___51; ___372          ___55; int32_t            ___2481; ___2225          ___1991; ___2225          ___2124; PositionAtAnchor_e ___3160; ___372          ___1817; };
 #if defined EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
typedef enum { ___3220, ___3221, ___3222, ___3223, ___3224, ___3225, ___3226, ___3227, ___3228, ___3229, ___3230, END_ProcessXYMode_e, ___3219 = ___329 } ProcessXYMode_e;
 #endif
typedef enum { ___717, ___720, ___719, END_CurveInfoMode_e, ___718 = ___329 } CurveInfoMode_e; enum ProcessLineMapMode_e { ___3204, ___3213, ___3205, ___3212, ___3202, ___3208, ___3207, ___3210, ___3206, ___3211, ___3203, ___3216, ___3218, ___3214, ___3209, ___3217, END_ProcessLineMapMode_e, ___3215 = ___329 }; typedef enum { ___3862, ___3861, END_StyleBase_e, ___3863 = ___329 } StyleBase_e; typedef enum { ___3280, ___3278, ___3282,
 #if defined ENABLE_ORPHANED_DATASETS
___3281,
 #endif
END_ReadDataOption_e, ___3279 = ___329 } ReadDataOption_e;
 #if defined EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
typedef enum { ___2717, ___2719, ___2720, END_NodeLabel_e, ___2718 = ___329 } NodeLabel_e;
 #endif
typedef enum { ___2173, ___2175, ___2176, END_LabelType_e, ___2174 = ___329 } LabelType_e;
 #if defined EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
typedef enum { ___3873, ___3871, ___3875, ___3872, END_SubBoundaryEditOption_e, ___3874 = ___329 } SubBoundaryEditOption_e;
 #endif
typedef enum { ___374, ___373, ___377, ___375, END_BorderAction_e, ___376 = ___329 } BorderAction_e; typedef enum { ___3128, ___3129, ___3130, ___3121, ___3131, ___3132, ___3133, ___3137, ___3138, ___3124, ___3126, ___3127, ___3134, ___3122, ___3135, ___3136, ___3125, END_PointerStyle_e, ___3123 = ___329 } PointerStyle_e; typedef enum { ___709, ___707, ___693, ___694, ___706, ___714, ___701, ___711, ___712, ___698, ___702, ___703, ___705, ___695, ___708, ___710, ___699, ___713, ___700, ___704, ___696, END_CursorStyle_e, ___697 = ___329 } CursorStyle_e; typedef enum { ___3074, ___3083, ___3075, ___3080, ___3082, ___3084, ___3085, ___3077, ___3078, ___3076, ___3081, END_PickSubPosition_e, ___3079 = ___329 } PickSubPosition_e; typedef enum { ___3962, ___3961, ___3960, ___3958, END_TecEngInitReturnCode_e, ___3959 = ___329 } TecEngInitReturnCode_e; typedef enum { ___1790, ___1791, ___1792, ___1787, ___1788, END_GetValueReturnCode_e, ___1789 = ___329, ___1785              = ___1790, ___1786 = ___1791, ___1793     = ___1792, ___1780    = ___1787, ___1782 = ___1788, ___1783         = ___1789 } GetValueReturnCode_e; typedef enum { ___3533, ___3522, ___3528, ___3530, ___3531, ___3532, ___3535, ___3536, ___3519, ___3529, ___3525, ___3520,
___3521, ___3534, ___3523, ___3526, END_SetValueReturnCode_e, ___3524 = ___3522, ___3527 = ___329, ___3517                       = ___3533, ___3507           = ___3522, ___3512     = ___3528, ___3514   = ___3530, ___3515     = ___3531, ___3516  = ___3532, ___3537          = ___3535, ___3538         = ___3536, ___3503            = ___3519, ___3513         = ___3529, ___3510      = ___3525, ___3505            = ___3520, ___3506            = ___3521, ___3518 = ___3534, ___3508      = ___3523, ___3509                  = ___3524, ___3511                  = ___3527 } SetValueReturnCode_e; typedef enum { ___815, ___816, END_DataAlterMode_e, ___817 = ___329 } DataAlterMode_e; typedef enum { ___825, ___823, ___824, ___821, ___822, ___820, ___818, END_DataAlterReturnCode_e, ___819 = ___329 } DataAlterReturnCode_e; typedef enum { ___2852, ___2853, ___2850, ___2854, ___2849, END_ObjectAlign_e, ___2851 = ___329 } ObjectAlign_e; typedef enum { ___2165, ___2164, ___2167, END_LabelAlignment_e, ___2166 = ___329 } LabelAlignment_e; typedef enum { ___4422, ___4418, ___4412, ___4430, ___4416, ___4434, ___4435, ___4425, ___4417, ___4428, ___4429, ___4431, ___4427, ___4414, ___4426, ___4413, ___4415, ___4423, END_View_e, ___4424 = ___329 } View_e; typedef enum { ___4467, ___4465, ___4466, ___4470, ___4469, ___4473, ___4471, ___4472,
END_WorkspaceView_e, ___4468 = ___329 } WorkspaceView_e; typedef enum { ___192, ___189, ___190, END_ArrowheadStyle_e, ___191 = ___329, ___186   = ___192, ___183  = ___189, ___184  = ___190, ___185 = ___191 } ArrowheadStyle_e; typedef enum { ___181, ___177, ___179, ___178, END_ArrowheadAttachment_e, ___180 = ___329, ___182        = ___181, ___172 = ___177, ___174       = ___179, ___173  = ___178, ___175     = ___180 } ArrowheadAttachment_e; typedef enum { ___496, ___495, END_Clipping_e, ___497 = ___329 } Clipping_e; typedef enum { ___3769, ___3770, ___3771, ___3774, ___3773, END_StatusInfo_e, ___3772 = ___329 } StatusInfo_e;
 #if defined EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
typedef enum { ___1513, ___1516, ___1517, ___1518, ___1515, END_FrameMode_e, ___1514 = ___329, ___1510   = ___1513, ___1524  = ___1516, ___1525    = ___1517, ___1526      = ___1518, ___1523  = ___1515, ___1512 = ___1514 } FrameMode_e;
 #endif
typedef enum { ___3111, ___3113, ___3112, ___3119, ___3116, ___3115, END_PlotType_e, ___3114 = ___329 } PlotType_e;
 #define ___4308(___3110) ( VALID_ENUM((___3110), PlotType_e) && \
 ((___3110) != ___3111) )
 #define ___4306(___3110) ( (___3110) == ___3119 || \
 (___3110) == ___3115 )
 #define ___4303(___3110) ( (___3110) == ___3112 || \
 (___3110) == ___3113 )
 #define ___3117(___3110) ___4303((___3110))
 #define ___3118(___3110) ___4306((___3110))
 #define ___4311(___3110) ( (___3110) == ___3116 || \
 (___3110) == ___3119 || \
 (___3110) == ___3112 || \
 (___3110) == ___3113 )
typedef enum { ___553, ___554, END_ContLineCreateMode_e, ___552 = ___329 } ContLineCreateMode_e; typedef enum { ___3053, ___3046, ___3041, ___3064, ___3047, ___3063, ___3045, ___3044, ___3058, ___3051, ___3056, ___3055, ___3061, ___3062, ___3054, ___3069, ___3068, ___3060, ___3059, ___3049, ___3057, ___3052, ___3042, END_PickObjects_e, ___3048 = ___329, ___3036                   = ___3053, ___3030                  = ___3046, ___3027                   = ___3041, ___3026      = ___3064, ___3031                   = ___3047, ___3070                   = ___3063, ___3029          = ___3045, ___3028           = ___3044, ___3043          = ___3058, ___3034             = ___3051, ___3071               = ___3051, ___3039        = ___3056, ___3038 = ___3055, ___3066    = ___3061, ___3067    = ___3062, ___3037                  = ___3054, ___3073                   = ___3069, ___3072              = ___3068, ___3065         = ___3060, ___3050               = ___3059, ___3033          = ___3049, ___3040              = ___3057, ___3035            = ___3052, ___3032                = ___3048 } PickObjects_e; enum SingleEditState_e { ___3596, ___3595, ___3597, END_SingleEditState_e, ___1141 = ___329 }; enum AxisSubPosition_e { ___307, ___306, ___309, ___305, ___310, ___311, END_AxisSubPosition_e, ___308 = ___329, ___304 = ___307, ___303 = ___309, ___313 = ___307, ___312 = ___311 }; typedef enum
{ ___300, ___299, ___302, END_AxisSubObject_e, ___301 = ___329 } AxisSubObject_e; typedef enum { ___2509, ___2510, ___2508, END_MouseButtonClick_e, ___2507 = ___329 } MouseButtonClick_e; typedef enum { ___2512, ___2533, ___2524, ___2532, ___2523, ___2513, ___2522, ___2528, ___2530, ___2535, ___2526, ___2534, ___2525, ___2514, ___2521, ___2529, ___2531, ___2536, ___2527, ___2518, ___2520, ___2517, ___2519, END_MouseButtonDrag_e, ___2511 = ___329, ___2516  = ___2522, ___2515 = ___2521 } MouseButtonDrag_e; struct ___2506 { MouseButtonClick_e ___421; MouseButtonDrag_e  ___3594; MouseButtonDrag_e  ___644; MouseButtonDrag_e  ___31; MouseButtonDrag_e  ___3555; MouseButtonDrag_e  ___642; MouseButtonDrag_e  ___647; MouseButtonDrag_e  ___35; MouseButtonDrag_e  ___643; }; struct ___2504 { ___2506 ___2466; ___2506 ___3392; };
 #define ___2749   0
 #define ___2212   1
 #define ___2467 2
 #define ___3394  3
typedef enum { ___33, ___34, END_AltMouseButtonMode_e, ___32 = ___329 } AltMouseButtonMode_e; typedef enum { ___2555, ___2566, ___2537, ___2576, ___2571, ___2556, ___2570, ___2549, ___2552, ___2547, ___2550, ___2548, ___2551, ___2543, ___2561, ___2557, ___2562, ___2563, ___2564, ___2565, ___2541, ___2539, ___2540, ___2569, ___2568, ___2546, ___2545, ___2544, ___2542, ___2567, ___2554, ___2572, ___2573, ___2574, ___2575, ___2559, ___2560, ___2538, END_MouseButtonMode_e, ___2553 = ___329, ___2558 = ___2561, ___2593                = ___2555, ___2601                = ___2566, ___2505                = ___2537, ___2611                  = ___2576, ___2606             = ___2571, ___2594                 = ___2556, ___2605                  = ___2570, ___2588          = ___2549, ___2591            = ___2552, ___2586            = ___2547, ___2589         = ___2550, ___2587           = ___2548, ___2590            = ___2551, ___2582           = ___2543, ___2596       = ___2561, ___2595      = ___2557, ___2597           = ___2562, ___2598           = ___2563, ___2599           = ___2564, ___2600           = ___2565, ___2580          = ___2541, ___2578            = ___2539, ___2579         = ___2540, ___2604          = ___2569, ___2603         = ___2568, ___2585         = ___2546, ___2584           = ___2545, ___2583 = ___2544,
___2581    = ___2542, ___2602                 = ___2567, ___2607                 = ___2572, ___2608                 = ___2573, ___2609                 = ___2574, ___2610                 = ___2575, ___2592               = ___2553 } MouseButtonMode_e; typedef enum { ___974, ___973, ___975, END_DetailsButtonState_e, ___972 = ___329 } DetailsButtonState_e; typedef enum { ___1191, ___1192, ___1190, ___1196, ___1193, ___1195, Event_KeyRelease, END_Event_e, ___1194 = ___329 } Event_e; typedef enum { ___2855, ___2857, ___2859, ___2858, END_ObjectDrawMode_e, ___2856 = ___329 } ObjectDrawMode_e; typedef enum { ___4161, ___4163, END_ThreeDViewChangeDrawLevel_e, ___4162 = ___329 } ThreeDViewChangeDrawLevel_e; typedef enum { ___2744, ___2746, END_NonCurrentFrameRedrawLevel_e, ___2745 = ___329 } NonCurrentFrameRedrawLevel_e; typedef enum { ___3310, ___3313, ___3311, ___3314, ___3303, ___3304, ___3305, ___3301, ___3302, ___3309, ___3308, ___3307, END_RedrawReason_e, ___3306 = ___329, ___3312 = ___3310, ___3315 = ___3313 } RedrawReason_e; typedef enum { ___3416, ___3415, ___3414, END_RotationMode_e, ___3413 = ___329 } RotationMode_e; typedef enum { ___3407, ___3408, ___3409, ___3403, ___3404, ___3399, ___3405, ___3406, ___3401, ___3398, ___3400,   /**@internal TP_NOTAVAILABLE*/ END_RotateAxis_e, ___3402 = ___329 } RotateAxis_e; typedef enum { ___3410, ___3412, END_RotateOriginLocation_e, ___3411 = ___329 } RotateOriginLocation_e; typedef enum { ___2883, ___2885, END_OriginResetLocation_e, ___2884 = ___329 } OriginResetLocation_e; typedef enum { ___3612, ___3613, ___3611,
___3610, END_SliceSource_e, ___3609 = ___329 } SliceSource_e; typedef enum { ___1948, ___1947, ___1944, ___1943, ___1940, ___1946, ___1951, ___1941, END_Input_e, ___1945 = ___329 } Input_e; typedef enum { ___3250, ___3252, ___3253, END_PtSelection_e, ___3251 = ___329 } PtSelection_e; typedef enum { ___1117, ___1116, ___1118, END_Drift_e, ___1115 = ___329 } Drift_e; typedef enum { ___964, ___965, ___969, ___968, ___967, END_DerivPos_e, ___966 = ___329 } DerivPos_e; typedef enum { ___2240, ___2242, END_LinearInterpMode_e, ___2241 = ___329 } LinearInterpMode_e; typedef enum { ___4439, ___4440, END_VolumeCellInterpolationMode_e, ___4438 = ___329 } VolumeCellInterpolationMode_e; typedef enum { ___3154, ___3152, END_PolyCellInterpolationMode_e, ___3153 = ___329 } PolyCellInterpolationMode_e; typedef enum { ___547, ___546, END_ConstraintOp2Mode_e, ___545 = ___329 } ConstraintOp2Mode_e; typedef enum { ___875, ___877, END_DataProbeVarLoadMode_e, ___876 = ___329 } DataProbeVarLoadMode_e; typedef enum { ___3934, ___3932, END_SZLSubzoneLoadModeForStreams_e, ___3933 = ___329 } SZLSubzoneLoadModeForStreams_e; typedef enum { ___4313, ___4314, ___4317, END_ValueBlankCellMode_e, ___4315 = ___329, ___4316 = ___4317 } ValueBlankCellMode_e; typedef enum { ___4318, ___4321, ___4319, END_ValueBlankMode_e, ___4320 = ___329 } ValueBlankMode_e; typedef enum { ___452, ___453, ___450, ___454, END_CellBlankedCond_e, ___451 = ___329 } CellBlankedCond_e; typedef enum { ___3329, ___3326, ___3328, ___3325, ___3324, ___3330, END_RelOp_e, ___3327 = ___329 } RelOp_e; typedef enum { ___1844, ___1843, END_IJKBlankMode_e, ___1845 = ___329 } IJKBlankMode_e; typedef enum { ___3107, ___3109, ___3106,
END_PlotApproximationMode_e, ___3108 = ___329 } PlotApproximationMode_e; typedef enum { ___3650, ___3651, ___3648, END_SphereScatterRenderQuality_e, ___3649 = ___329 } SphereScatterRenderQuality_e; typedef enum { ExtractMode_SingleZone, ExtractMode_OneZonePerConnectedRegion, ExtractMode_OneZonePerSourceZone, END_ExtractMode_e, ExtractMode_Invalid = ___329 } ExtractMode_e; typedef enum { Resulting1DZoneType_IOrderedIfPossible, Resulting1DZoneType_FELineSegment, Resulting1DZoneType_Unused, END_Resulting1DZoneType_e, ResultingDZoneType_Invalid = ___329 } Resulting1DZoneType_e; typedef enum { ___2988, ___2986, ___2987, ___2982, END_FillPat_e, ___2983 = ___329 } FillPat_e; typedef enum { ___4222, ___4220, ___4221, ___4218, END_Translucency_e, ___4219 = ___329 } Translucency_e; typedef enum { ___3887, ___3888, ___3885, END_SunRaster_e, ___3886 = ___329 } SunRaster_e; typedef enum { ___384, ___387, ___386, END_BoundaryCondition_e, ___385 = ___329 } BoundaryCondition_e; typedef enum { ___289, ___292, ___291, END_AxisMode_e, ___290 = ___329 } AxisMode_e; typedef enum { ___3261, ___3259, ___3262, END_QuickColorMode_e, ___3260 = ___329 } QuickColorMode_e; typedef enum { ___1412, ___1415, ___1414, ___1413, END_FillMode_e, ___1411 = ___329 } FillMode_e; typedef enum { ___2269, ___2265, ___2263, ___2266, ___2268, ___2264, END_LinePattern_e, ___2267 = ___329 } LinePattern_e; typedef enum { ___2127, ___2128, ___2125, END_LineJoin_e, ___2126 = ___329 } LineJoin_e; typedef enum { ___440, ___443, ___444, END_LineCap_e, ___441 = ___329 } LineCap_e; typedef enum { ___1585, ___1587, ___1588, ___1581, ___1582, ___1586, ___1583, END_GeomForm_e, ___1584 = ___329, GeomType_LineSegs = ___1585, GeomType_Rectangle = ___1587, GeomType_Square = ___1588, GeomType_Circle = ___1581, GeomType_Ellipse = ___1582, GeomType_LineSegs3D = ___1586, GeomType_Image = ___1583, END_GeomType_e = END_GeomForm_e, GeomType_Invalid = ___1584 } GeomForm_e; typedef GeomForm_e GeomType_e; typedef enum { ___4345, ___4344, END_VariableDerivationMethod_e, ___4346 = ___329 } VariableDerivationMethod_e;
typedef enum { ___270, END_AuxDataType_e, ___269 = ___329 } AuxDataType_e; typedef enum { ___259, ___253, ___254, ___258, ___256, ___257, AuxDataLocation_Layout, END_AuxDataLocation_e, ___255 = ___329 } AuxDataLocation_e; typedef enum { ___4701, ___4699, ___4697, ___4698, ___4692, ___4693, ___4695, ___4696, ZoneType_FEMixed, END_ZoneType_e, ___4700 = ___329, ___4694 = ZoneType_FEMixed } ZoneType_e; typedef enum { FECellShape_Bar, FECellShape_Triangle, FECellShape_Quadrilateral, FECellShape_Tetrahedron, FECellShape_Hexahedron, FECellShape_Pyramid, FECellShape_Prism, END_FECellShape_e, FECellShape_Invalid = ___329 } FECellShape_e; typedef uint32_t FEGridOrder_t; typedef enum { FECellBasisFunction_Lagrangian, END_FECellBasisFunction_e, FECellBasisFunction_Invalid = ___329 } FECellBasisFunction_e; typedef enum { ___4656, ___4661, ___4663, ___4657, ___4659, ___4662, ___4658, END_ZoneOrder_e, ___4660 = ___329 } ZoneOrder_e; typedef enum { ___851, ___852, ___849, ___850, END_DataFormat_e, ___853 = ___329 } DataFormat_e; typedef enum { ___872, ___874, END_DataPacking_e, ___873 = ___329 } DataPacking_e; typedef enum { ProbeObject_None, ProbeObject_Streamtrace, ProbeObject_StreamtraceMarker, ProbeObject_Slice, ProbeObject_IsoSurface, ProbeObject_FieldZone, END_ProbeObject_e, ProbeObject_First = ProbeObject_Streamtrace, ProbeObject_Last  = ProbeObject_FieldZone, ProbeObject_Invalid = ___329 } ProbeObject_e; typedef enum { ProbeNearest_Position, ProbeNearest_Node, END_ProbeNearest_e, ProbeNearest_Invalid = ___329 } ProbeNearest_e; typedef enum { ___2990, ___2991, ___2994, ___2993, ___2989, ___2995, ___2996, ___2997, END_PrinterDriver_e, ___2992 = ___329 } PrinterDriver_e; typedef enum { ___1886, ___1903, ___1881, ___1883, END_EPSPreviewImage_e, ___1885 = ___329 } EPSPreviewImage_e; typedef enum { ___4170, ___4172, END_TIFFByteOrder_e, ___4171 = ___329 } TIFFByteOrder_e; typedef enum { ___2131, ___2130, END_JPEGEncoding_e, ___2129 = ___329 } JPEGEncoding_e; typedef enum { ___1431, ___1430, ___1428, END_FlashImageType_e, ___1429 = ___329, ___1427 = ___1428 } FlashImageType_e; typedef enum { ___1424, ___1426, END_FlashCompressionType_e, ___1425 = ___329 } FlashCompressionType_e; typedef enum { ___1223, ___1227,
___1224, ___1225, ___1231, ___1222, ___1214, ___1215, ___1221, ___1211, ___1218, ___1228, ___1209, ___1220, ___1208, ___1210,    /**@internal TP_NOTAVAILABLE*/ ___1217, ___1212, ___1230, ___1226, ___1213, ___1219, ___1229, END_ExportFormat_e, ___1216 = ___329 } ExportFormat_e; typedef enum { ___275, ___277, ___278, END_AVICompression_e, ___276 = ___329 } AVICompression_e; typedef enum { ___65, ___59, ___64, ___60, ___61, ___63, ___66, END_AnimationDest_e, ___62 = ___329 } AnimationDest_e; typedef enum { ___69, ___67, ___71, ___68, END_AnimationOperation_e, ___70 = ___329 } AnimationOperation_e; typedef enum { ___73, ___78, ___72, ___79, ___75, ___77, ___76, END_AnimationStep_e, ___74 = ___329 } AnimationStep_e; typedef enum { ___4602, ___4600, ___4603, END_ZoneAnimationMode_e, ___4601 = ___329 } ZoneAnimationMode_e;
 #if defined EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
typedef enum { ___361, ___360, ___363, END_BitDumpRegion_e, ___362 = ___329 } BitDumpRegion_e;
 #endif
typedef enum { ___1237, ___1236, ___1239, END_ExportRegion_e, ___1238 = ___329 } ExportRegion_e; typedef enum { ___2954, ___2952, ___2949, ___2948, ___2950, ___2951, END_PaperSize_e, ___2953 = ___329, ___2947  = ___2954, ___2945  = ___2952, ___2942      = ___2949, ___2941      = ___2948, ___2943 = ___2950, ___2944 = ___2951, ___2946 = ___2953 } PaperSize_e; typedef enum { ___2956, ___2959, ___2968, ___2963, ___2957, ___2960, ___2965, ___2967, ___2966, ___2955, ___2964, ___2962, ___2961, END_PaperUnitSpacing_e, ___2958 = ___329 } PaperUnitSpacing_e; typedef enum { ___2918, ___2919, ___2916, END_Palette_e, ___2917 = ___329 } Palette_e; typedef enum { ___3189, ___3187, END_PrintRenderType_e, ___3188 = ___329 } PrintRenderType_e; typedef enum { ___4267, ___4266, ___4269, ___4270, ___4265, END_Units_e, ___4268 = ___329 } Units_e; typedef enum { ___657, ___658, END_CoordScale_e, ___656 = ___329, ___3434 = ___657, ___3435 = ___658, ___3433 = ___656 } CoordScale_e;
 #define ___1751(___3263) ( ((___3263) < ___3626) ? ___3629 : ( ((___3263) > ___2177) ? ___2188 : ___2314((___3263)) ) )
typedef enum { CoordSys_Grid, CoordSys_Frame, ___659, ___662, ___663, ___660, CoordSys_Grid3D, ___664, END_CoordSys_e, ___661 = ___329 } CoordSys_e; typedef enum { ___3442, ___3444, END_Scope_e, ___3443 = ___329 } Scope_e; typedef enum { ___4048, ___4043, ___4053, ___4050, ___4049, ___4051, ___4045, ___4044, ___4046, ___4052, END_TextAnchor_e, ___4047 = ___329 } TextAnchor_e; typedef enum { TextType_Regular, TextType_LaTeX, END_TextType_e, TextType_Invalid = ___329 } TextType_e; typedef enum { ___4073, ___4061, ___4067, END_TextBox_e, ___4068 = ___329 } TextBox_e; typedef enum { ___1645, ___1635, ___1637, ___1643, ___1639, ___1636, ___1633, ___1634, ___1644, ___1640, ___1642, ___1641, GeomShape_LineArt, END_GeomShape_e, ___1638 = ___329 } GeomShape_e; typedef enum { ___348, ___347, ___346, ___345, ___343, END_BasicSize_e, ___344 = ___329 } BasicSize_e; typedef enum { ___2247, ___2244, ___2245, ___2249, ___2250, ___2248, END_LineForm_e, ___2246 = ___329 } LineForm_e; typedef enum { ___727, ___729, ___724, ___730, ___731, ___728, ___725, END_CurveType_e, ___726 = ___329, ___723 = ___729 } CurveType_e; typedef enum { ___3453, ___3455, ___3454, END_Script_e, ___3452 = ___329 } Script_e; typedef enum { ___1452, ___1453, ___1447, ___1457, ___1468, ___1464, ___1466, ___1465, ___1467, ___1443, ___1444, ___1446, Font_HelveticaItalic, Font_HelveticaItalicBold, Font_CourierItalic, Font_CourierItalicBold, END_Font_e, ___1454 = ___329 } Font_e; typedef enum { ___1463, ___1462, ___1459, ___1460, END_FontStyle_e, ___1461 = ___329 } FontStyle_e; typedef enum { ___4231, ___4230, END_TwoDDrawOrder_e, ___4232 = ___329 } TwoDDrawOrder_e; typedef enum { ___1112, ___1113, END_DrawOrder_e, ___1114 = ___329 } DrawOrder_e; typedef enum { ___3804, ___3805, ___3807, ___3808, ___3809, ___3806, END_Streamtrace_e, ___3802 = ___329 } Streamtrace_e; typedef enum { ___3792, ___3794, ___3791, END_StreamDir_e, ___3793 = ___329 } StreamDir_e;
typedef enum { ___1090, ___1091, ___1092, ___1093, DistributionRegion_SurfacesOfSuppliedZones, END_DistributionRegion_e, ___1089 = ___329 } DistributionRegion_e; typedef enum { ___2043, ___2045, ___2047, ___2046, END_IsoSurfaceSelection_e, ___2044 = ___329 } IsoSurfaceSelection_e; typedef enum { ___4325, ___4327, END_ValueLocation_e, ___4326 = ___329 } ValueLocation_e; typedef enum { OffsetDataType_32Bit, OffsetDataType_64Bit, END_OffsetDataType_e, OffsetDataType_Invalid = ___329 } OffsetDataType_e;
 #define VALID_32OR64BIT_OFFSET_TYPE(t) ((t) == OffsetDataType_32Bit || (t) == OffsetDataType_64Bit)
typedef enum { ___1369,   /**@internal TP_NOTAVAILABLE*/ FieldDataType_Float, FieldDataType_Double, FieldDataType_Int32, FieldDataType_Int16, FieldDataType_Byte, ___1363, END_FieldDataType_e, ___1364,     /**@internal TP_NOTAVAILABLE*/ ___1366,   /**@internal TP_NOTAVAILABLE*/ ___1368 = FieldDataType_Int32, ___1371 = FieldDataType_Int16, ___1367 = ___329 } FieldDataType_e;
 #define VALID_FIELD_DATA_TYPE(___1362) (VALID_ENUM((___1362),FieldDataType_e) && \
 (___1362)!=___1369)
 #if defined EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
typedef enum { ___2429, ___2424, ___2422, END_MeshPlotType_e, ___2423 = ___329 } MeshPlotType_e;
 #endif
typedef enum { ___2428, ___2427, ___2425, END_MeshType_e, ___2426 = ___329 } MeshType_e;
 #if defined EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
typedef enum { ___634, ___614, ___635, ___555, ___613, END_ContourPlotType_e, ___615 = ___329 } ContourPlotType_e;
 #endif
typedef enum { ___639, ___637, ___640, ___636, ___641, END_ContourType_e, ___638 = ___329 } ContourType_e; typedef enum { ___565, ___556, ___557, ___558, ___559, ___560, ___561, ___562, ___563, END_ContourColoring_e, ___564 = ___329 } ContourColoring_e;
 #if defined EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
typedef enum { ___4397, ___4393, ___4396, ___4394, END_VectorPlotType_e, ___4395 = ___329 } VectorPlotType_e;
 #endif
typedef enum { ___4402, ___4398, ___4401, ___4399, END_VectorType_e, ___4400 = ___329 } VectorType_e; typedef enum { ___3545, ___3544, ___3542, ___3541, ___3540, END_ShadePlotType_e, ___3543 = ___329 } ShadePlotType_e; typedef enum { ___2237, ___2234, ___2236, END_LightingEffect_e, ___2235 = ___329 } LightingEffect_e; enum IJKLines_e { ___1855, ___1857, ___1858, END_IJKLines_e, ___1856 = ___329, ___2281       = ___1855, ___2283       = ___1857, ___2284       = ___1858, ___2282 = ___1856 }; typedef enum { ___1848, ___1846, ___1849, END_IJKCellType_e, ___1847 = ___329 } IJKCellType_e; typedef enum { ___1865, ___1870, ___1872, ___1864, ___1866, ___1871, ___1868, ___1867, ___1874, ___1873, END_IJKPlanes_e, ___1869 = ___329, ___3096       = ___1865, ___3101       = ___1870, ___3103       = ___1872, ___3097      = ___1866, ___3102      = ___1871, ___3099      = ___1868, ___3098     = ___1867, ___3095    = ___1864, ___3105  = ___1874, ___3104  = ___1873, ___3100 = ___1869 } IJKPlanes_e; typedef enum { ___3896, ___3897, ___3902, ___3904, ___3905, ___3899, ___3903, ___3900, ___3898, ___3895, ___3906, END_SurfacesToPlot_e, ___3901 = ___329 } SurfacesToPlot_e; typedef enum { ___3148, ___3145, ___3147, ___3143, ___3144, END_PointsToPlot_e, ___3149 = ___3148, ___3142          = ___3145, ___3146 = ___329 } PointsToPlot_e; typedef enum { ___3620, ___3622, ___3624, ___3617, ___3618, ___3619, ___3615, ___3614, END_SliceSurface_e, ___3616 = ___329 } SliceSurface_e;   /* pytecplot defines this separately with CVar removed. */ typedef enum { ___501, ___499, ___498, END_ClipPlane_e, ___500 = ___329 } ClipPlane_e; typedef enum { ___3604, ___3603, END_SkipMode_e, ___3605 = ___329 } SkipMode_e; typedef enum { ___1137, ___1139, ___1138, END_EdgeType_e, ___1140 = ___329 } EdgeType_e;
 #if defined EXPORT_DEPRECATED_INTERFACES_TO_ADK_ONLY
typedef enum { ___391, ___390, ___389, ___383, END_BoundPlotType_e, ___388 = ___329 } BoundPlotType_e;
 #endif
typedef enum { ___397, ___396, ___395, ___393, END_BoundaryType_e, ___394 = ___329 } BoundaryType_e; typedef enum { ___382, ___381, ___380, ___378, END_BorderLocation_e, ___379 = ___329 } BorderLocation_e; typedef enum { ___608, ___579, ___582, ___576, ___611, ___610, ___609, ___590, ___567, ___568, ___566, ___569, ___570, ___571, ___573, ___574, ___575, ___577, ___580, ___572, ___581, ___583, ___584, ___585, ___586, ___587, ___588, ___589, ___591, ___592, ___593, ___595, ___594, ___596, ___597, ___598, ___602, ___599, ___600, ___601, ___603, ___604, ___605, ___606, ___607, END_ContourColorMap_e, ___578 = ___329, ___528    = ___608, ___525    = ___579, ___526       = ___582, ___523    = ___576, ___531         = ___611, ___530      = ___610, ___529     = ___609, ___527   = ___590, ___524      = ___578 } ContourColorMap_e; typedef enum { ___1182, ___1177, ___1180, ___1181, ___1178, ___1183, ___1176, END_ErrorBar_e, ___1179 = ___329 } ErrorBar_e; typedef enum { ___633, ___632, ___630, END_ContourLineMode_e, ___631 = ___329 } ContourLineMode_e; enum Panel_e { ___2920, ___2921, ___2940, ___2931, ___2926, ___2922, ___2924, ___2930, ___2923, ___2937, ___2938, ___2936, ___2928, Panel_IsoSurfaceVector, ___2929, ___2932, ___2935, ___2934, ___2933, ___2925, ___2939, END_Panel_e, ___2927 = ___329 }; typedef enum { ___2441, ___2445, ___2442, ___2444,     /**@internal TP_NOPYTHON*/
___2447, ___2448, ___2446, END_MessageBoxType_e, ___2443 = ___329, ___2431           = ___2441, ___2449         = ___2445, ___2432     = ___2442, ___2434        = ___2444, ___2451           = ___2447, ___2452     = ___2448, ___2450 = ___2446, ___2433         = ___2443 } MessageBoxType_e; typedef enum { ___2439, ___2437, ___2435, ___2438, END_MessageBoxReply_e, ___2436 = ___329 } MessageBoxReply_e; typedef enum { ___2770, ___2769, ___2768, ___2765, ___2774, ___2766, ___2772, ___2773, ___2767, ___2775, END_NumberFormat_e, ___2771 = ___329 } NumberFormat_e; typedef NumberFormat_e ValueFormat_e; typedef enum { ___324, ___325, ___323, END_BackingStoreMode_e, ___322 = ___329 } BackingStoreMode_e; typedef enum { ___4166, ___4168, ___4165, END_TickDirection_e, ___4167 = ___329 } TickDirection_e; typedef enum { ___320, ___318, ___321, END_AxisTitlePosition_e, ___319 = ___329 } AxisTitlePosition_e; typedef enum { ___315, ___317, ___316, END_AxisTitleMode_e, ___314 = ___329 } AxisTitleMode_e; typedef enum { ___288, ___286, ___285, ___284, AxisAlignment_WithSpecificAngle, ___283, ___280, ___281, ___282, END_AxisAlignment_e, ___279 = ___329 } AxisAlignment_e; typedef enum { ___1537, ___1538, END_FunctionDependency_e, ___1534 = ___329, ___1536 = ___1537, ___1535 = ___1538 } FunctionDependency_e; typedef enum { LegendShow_Always, LegendShow_Never, ___2217, END_LegendShow_e, ___2218 = ___329, ___2220 = LegendShow_Always, ___2219  = LegendShow_Never } LegendShow_e; typedef enum { ___2257, LineMapSort_ByIndependentVar, LineMapSort_ByDependentVar, LineMapSort_BySpecificVar, END_LineMapSort_e, ___2256 = ___329, ___2255 = LineMapSort_ByIndependentVar, ___2254   = LineMapSort_ByDependentVar,
___2258    = LineMapSort_BySpecificVar, } LineMapSort_e; typedef enum { ___549, ___550, ___548, END_ContLegendLabelLocation_e, ___551 = ___329 } ContLegendLabelLocation_e; typedef enum { ___4142, ___4144, ___4141, END_ThetaMode_e, ___4143 = ___329 } ThetaMode_e; typedef enum { ___4211, ___4214, ___4212, ___4213, END_Transform_e, ___4210 = ___329 } Transform_e; typedef enum { ___4461, ___4462, ___4459, ___4458, END_WindowFunction_e, ___4460 = ___329 } WindowFunction_e; typedef enum { ___2203, ___2204, ___2202, END_LaunchDialogMode_e, ___2201 = ___329 } LaunchDialogMode_e; typedef enum { ___3466, ___3465, ___3463, ___3469, ___3468, END_SelectFileOption_e, ___3464 = ___329 } SelectFileOption_e; typedef enum { ___356, ___357, ___358, BinaryFileVersion_Tecplot2019, ___354 = BinaryFileVersion_Tecplot2019, END_BinaryFileVersion_e, ___355 = ___329 } BinaryFileVersion_e; typedef enum { ___4411, ___4409, ___4408, END_ViewActionDrawMode_e, ___4410 = ___329 } ViewActionDrawMode_e; typedef enum { ___2896, ___2897, ___2895, ___2902, ___2903, ___2899, ___2901, ___2900, END_PageAction_e, ___2898 = ___329 } PageAction_e; typedef enum { ___1505, ___1501, ___1499, ___1488, ___1490, ___1503, ___1500, ___1504, ___1487, ___1485, ___1486, ___1482, ___1483, ___1484, ___1495, ___1496, ___1497, ___1492, ___1493, ___1494, ___1506, FrameAction_DeleteByNumber, FrameAction_Reset, END_FrameAction_e, ___1491 = ___329, ___1498 = ___1501, ___1502 = ___1504, ___1489 = ___1488 } FrameAction_e; typedef enum { ___3008, ___3002, ___3003, ___3013,
___3011, ___3010, ___3009, ___3016, ___3017, ___3021, ___3015, ___3019, ___3018, ___3020, ___3012, ___3007, ___3006, ___3005, ___3004, END_PickAction_e, ___3014 = ___329 } PickAction_e; typedef enum { ___627, ___626, ___629, END_ContourLevelsInitializationMode_e, ___628 = ___329 } ContourLevelsInitializationMode_e; typedef enum { ___619, ___623, ___621, ___624, ___625, ___620, END_ContourLevelAction_e, ___622 = ___329 } ContourLevelAction_e; typedef enum { ___616, ___617, END_ContourLabelAction_e, ___618 = ___329 } ContourLabelAction_e; typedef enum { ___3796, ___3797, ___3798, ___3801, ___3800, END_StreamtraceAction_e, ___3799 = ___329 } StreamtraceAction_e; typedef enum { ___517, ___515, ___518, END_ColorMapControlAction_e, ___516 = ___329 } ColorMapControlAction_e; typedef enum { ___520, ___519, END_ColorMapDistribution_e, ___521 = ___329 } ColorMapDistribution_e; typedef enum { ___3385, ___3384, ___3383, ___3382, END_RGBMode_e, ___3381 = ___329 } RGBMode_e; typedef enum { TransientZoneVisibility_ZonesAtOrBeforeSolutionTime, TransientZoneVisibility_ZonesAtSolutionTime, END_TransientZoneVisibility_e, TransientZoneVisibility_Invalid = ___329 } TransientZoneVisibility_e; typedef enum { TimeScaling_Linear, TimeScaling_Logarithmic, END_TimeScaling_e, TimeScaling_Invalid = ___329 } TimeScaling_e; typedef enum { FEDerivativeMethod_GreenGauss, FEDerivativeMethod_MovingLeastSquares, END_FEDerivativeMethod_e, FEDerivativeMethod_Invalid = ___329 } FEDerivativeMethod_e; typedef enum { TransientOperationMode_SingleSolutionTime, TransientOperationMode_AllSolutionTimes, TransientOperationMode_AllSolutionTimesDeferred, END_TransientOperationMode_e, TransientOperationMode_Invalid = ___329 } TransientOperationMode_e; typedef enum { ExtractionStrandIDAssignment_DoNotAssignStrandIDs, ExtractionStrandIDAssignment_Auto, ExtractionStrandIDAssignment_OneStrandPerGroup, ExtractionStrandIDAssignment_OneStrandPerSubExtraction, END_ExtractionStrandIDAssignment_e, ExtractionStrandIDAssignment_Invalid = ___329
} ExtractionStrandIDAssignment_e; typedef enum { ___4035, ___4036, END_TecUtilErr_e, ___4034 = ___329 } TecUtilErr_e; enum AxisShape_e { ___298, ___296, ___297, ___294, END_AxisShape_e, ___295 = ___329 }; enum RunMode_e { ___3424, ___3425, ___3426, END_RunMode_e, ___3427 = ___329 }; typedef enum { ___1206, ___1202, ___1207, ___1201, ___1200, ___1205, ___1204, END_ExportCustReturnCode_e, ___1203 = ___329 } ExportCustReturnCode_e; typedef enum { ___806, ___805, ___808, ___809, ___811, ___812, ___810, END_CZType_e, ___807 = ___329 } CZType_e; typedef enum { ___1288, ___1287, ___1285, ___1284, END_FaceNeighborMode_e, ___1286 = ___329 } FaceNeighborMode_e; typedef enum { ___2912, PageRenderDest_WorkArea, PageRenderDest_AnnotateWorkArea, ___2913, ___2910, END_PageRenderDest_e, ___2911 = ___329 } PageRenderDest_e; enum RenderDest_e { ___3346, RenderDest_AnnotateWorkArea, ___3339, ___3338, ___3337, ___3336, ___3344, ___3341, END_RenderDest_e, ___3342 = ___329, ___3340 = ___3346, ___3343 = ___3336 }; typedef enum { ___3779, ___3780, ___3782, END_Stipple_e, ___3781 = ___329 } Stipple_e; typedef enum { ___843, ___844, ___846, END_DataFileType_e, ___845 = ___329 } DataFileType_e; typedef enum { ___538, ___539, END_ConditionAwakeReason_e, ___537 = ___329 } ConditionAwakeReason_e; typedef enum { ___3198, ___3199, ___3196, END_ProbeStatus_e, ___3197 = ___329 } ProbeStatus_e; typedef enum { ___1521, ___1522, END_FrameSizePosUnits_e, ___1520 = ___329 } FrameSizePosUnits_e; typedef enum { ___1812, ___1814, ___1813, END_Gridline_e, ___1811 = ___329 } Gridline_e; typedef enum { ___3168, ___3166, END_PositionMarkerBy_e, ___3167 = ___329 } PositionMarkerBy_e; typedef enum { ___2296, ___2297, ___2298,
END_LoaderCallbackVersion_e, ___2295 = ___329 } LoaderCallbackVersion_e; typedef enum { LoaderAdvancedOptions_NotAvailable, LoaderAdvancedOptions_Allow, LoaderAdvancedOptions_ForceLaunch, END_LoaderAdvancedOptions_e, LoaderAdvancedOptions_Invalid = ___329 } LoaderAdvancedOptions_e; typedef enum { ___3023, ___3025, ___3022, PickCollectMode_HomogeneousAdd, END_PickCollectMode_e, ___3024 = ___329 } PickCollectMode_e; typedef enum { ___398, ___400, END_BoundingBoxMode_e, ___399 = ___329 } BoundingBoxMode_e; typedef enum { ImageRenderingStrategy_Auto, ImageRenderingStrategy_OpenGL, ImageRenderingStrategy_OSMesa, END_ImageRenderingStrategy_e, ImageRenderingStrategy_Mesa = ImageRenderingStrategy_OSMesa, ImageRenderingStrategy_Invalid = ___329 } ImageRenderingStrategy_e; typedef enum { PreTranslateData_Auto, PreTranslateData_On, PreTranslateData_Off, END_PreTranslateData_e, PreTranslateData_Invalid = ___329 } PreTranslateData_e; struct MinMax_s { double minValue; double maxValue; }; typedef struct ___2663* ___2662; typedef void*(*___4149)(___90 ___4148); typedef struct ___541* ___540; typedef struct ___2120* ___2118; typedef void (*___4158)(___90 ___2122); typedef struct StringList_s* ___3837; typedef struct Menu_s*       ___2417; typedef struct ___135*  ___134; typedef struct LineSegmentProbeResult_s* ___2280; typedef enum { ___1898, ___1890, ___1895, ___1896, ___1899, ___1889, ___1891, ___1892, ___1897, ___1893, END_ImageResizeFilter_e, ___1894 = ___329 } ImageResizeFilter_e; typedef enum { ___4376, ___4371, ___4374, ___4372, ___4375, END_VarStatus_e, ___4373 = ___329 } VarStatus_e; typedef enum { ElementOrientation_Standard, ElementOrientation_Reversed, ElementOrientation_Arbitrary, END_ElementOrientation_e, ElementOrientation_Invalid = ___329 } ElementOrientation_e; typedef struct ___3500* ___3499; struct ___4576 { double X; double Y; }; struct ___4579 { double X; double Y; double Z;
 #if defined __cplusplus
bool operator==(___4579 const& ___2886) const { return (X == ___2886.X) && (Y == ___2886.Y) && (Z == ___2886.Z); }
 #endif
}; typedef ___4579 ___4578[8]; struct ___3248 { double ___3246; double ___4139; double ___30; }; namespace tecplot { class ___4235; } struct ___1548 { double ___4290; double ___4292; double ___4294; }; struct ___4147 { double ___4139; double ___3263; }; union ___54 { ___1548 ___1546; ___4579         ___4577; ___4147      ___4145; }; struct ___839 { char*                  ___3181; char*                  ___4038; DataFileType_e         ___1406; ___1396           ___840; ___3837          ___4360; ___1170             ___2845; ___1170             NumVars; double                 ___3637; struct ___839* ___2701; }; struct ___3870 { ___372 ___1917; ___372 ___1912; ___372 ___1920; ___372 ___1919; ___372 ___1914; ___372 ___1915; ___372 ___1909; ___372 ___1918; ___372 ___1910; ___372 UpdateInvalidContourLevels; ___372 ___1911; ___372 ___535; ___372 ___2421; ___372 ___1913; ___372 ___4282; }; struct ___2042 { ___372 ___3561; ___372 ___3575; ___372 ___3564; ___372 ___3584; ___372 ___3581; ___372 ___4281; ___372 ___4285; }; struct ___3608 { ___372 ___3561; ___372 ___3575; ___372 ___3564; ___372 ___3584; ___372 ___3581; ___372 ___3567; ___372 ___4281; ___372 ___4285; }; struct ___3803 { ___372 ___3561; ___372 ___3576; ___372 ___3565; ___372 ___3562; ___372 ___3575; ___372 ___3564; ___372 ___3581; ___372 ___3574; ___372 ___4281; ___372 ___4285; }; struct ___1372 {
 #if 0 
___372       ___3561;
 #endif
TwoDDrawOrder_e ___4229; ___372       ___3575; ___372       ___3564; ___372       ___3584; ___372       ___3580; ___372       ___3581; ___372       ___3567; ___372       ___4281; ___372       ___4285; }; struct ___2277 {
 #if 0 
___372       ___3561;
 #endif
___372 ___3573; ___372 ___3583; ___372 ___3563; ___372 ___3571; }; union ___1978 { double    ___3432; ___2225 ___3553; }; typedef ___372 (*___3883)(TP_IN_OUT double* ___4312, const char*       ___3881); struct ___1949 { Input_e           ___4234; double            ___2468; double            ___2344; ___1978 ___1976; ___3883 ___3882; }; struct ___3388 { ___514 ___3263; ___514 G; ___514 B; bool operator==(___3388 const& ___3390) const { return ___3263 == ___3390.___3263 && G == ___3390.G && B == ___3390.B; } }; struct ___646 { double ___522; ___3388  ___2210; ___3388  ___4209; bool operator==(___646 const& ___3390) const { return ___522 == ___3390.___522 && ___2210          == ___3390.___2210          && ___4209         == ___3390.___4209; } }; struct ___1189 { int       ___1830; int       ___2104; int       ___2195; int       ___2199; int       ___337; int       ___338; int       ___422; Event_e   ___1188; ___372 ___2055; ___372 ___1997; ___372 ___2009; ___372 ___4447; ___372 ___4445; ___372 ___4446; }; struct ___2327 { ___2323          ___2330; struct ___2327* ___2700; }; struct ___1974 { ___2225 ___4564; ___2225 ___4581; ___2225 ___4565; ___2225 ___4582; }; struct ___3297 { double ___4564; double ___4581; double ___4565; double ___4582; bool operator==(___3297 const& ___2886) const { return ___4564 == ___2886.___4564 && ___4581 == ___2886.___4581 && ___4565 == ___2886.___4565 && ___4582 == ___2886.___4582; } bool operator!=(___3297 const& ___2886) const { return !operator==(___2886); } }; struct ___1877 { ___2225  ___1830; ___2225  ___2104; ___2225  ___2133; }; struct ___1172 { ___1170 ___2468; ___1170 ___2344; ___1170 ___3602; }; struct ___1927 { ___2225 ___2468; ___2225 ___2344; ___2225 ___3602; }; struct ___4121 { Font_e                    ___1442; double                    ___1825; Units_e                   ___3599; };
 #define ___202(S)       (((S)->___4280 == ___1303) && (___1455::___1956().___1441((S)->___4236) == ___1447))
 #define ___203(S)        (((S)->___4280 == ___1303) && (___1455::___1956().___1441((S)->___4236) == ___1457))
 #define ___204(S) (((S)->___4280 == ___1303) && (___1455::___1956().___1441((S)->___4236) == ___1468))
struct ___205 { ___372                ___4280; tecplot::___4235 const* ___4236; ___3915             ___470; }; struct ___3917 { GeomShape_e  ___1632; ___372    ___2001; ___205 ___201; };
 #ifdef NOT_USED
struct AddOnList_s { int ___1127; };
 #endif
typedef struct AddOnList_s* ___11; struct ZoneMetrics; typedef ZoneMetrics* ZoneMetrics_pa; namespace tecplot { class ElemToNodeMap; } typedef tecplot::ElemToNodeMap* ___2725; typedef struct ___3868*   ___3867; typedef struct ___832*  ___831; typedef struct ___3865* ___3864; typedef struct ___2753*       ___2752; typedef struct ___832* ElementOrientation_pa;
 #define ___1987 (-1)
 #define ___2747 (-1)
 #define ___2748    (-1)
 #define ___4271  (___2747-2) 
typedef struct ___1291* ___1290; typedef struct ___1270* ___1269; typedef struct ___1150* ___1149; namespace tecplot { class NodeToElemMap; } typedef tecplot::NodeToElemMap* ___2740; enum RecordingLanguage_e { RecordingLanguage_TecplotMacro, RecordingLanguage_Python, END_RecordingLanguage_e, RecordingLanguage_Invalid = ___329 }; enum FaceNeighborMemberArray_e { ___1280, ___1278, ___1276, ___1277, ___1275, FaceNeighborMemberArray_BndryConnectZoneUniqueIds, END_FaceNeighborMemberArray_e, ___1281 = ___329 }; int const ___1289 = (int)END_FaceNeighborMemberArray_e; enum FaceMapMemberArray_e { ___1264, ___1265, ___1263, ___1266, ___1262, ___1260, FaceMapMemberArray_FaceBndryItemElemZoneUniqueIds, END_FaceMapMemberArray_e, ___1267 = ___329 }; int const ___1268 = (int)END_FaceMapMemberArray_e; enum ElemToFaceMapMemberArray_e { ___1145, ___1146, END_ElemToFaceMapMemberArray_e, ___1147 = ___329 }; int const ___1148 = (int)END_ElemToFaceMapMemberArray_e; enum NodeToElemMapMemberArray_e { ___2737, ___2738, END_NodeToElemMapMemberArray_e, ___2736 = ___329 }; int const ___2739 = (int)END_NodeToElemMapMemberArray_e; typedef struct ___1360* ___1359; typedef struct AuxData_s* ___264; typedef class LayoutReadDataInfo_s* LayoutReadDataInfo_pa; typedef enum { ___927, ___928, ___929, ___931 = ___929, END_DataValueStructure_e, ___930 = ___329 } DataValueStructure_e; typedef enum { ___869, ___870, END_DataNodeStructure_e, ___871 = ___329 } DataNodeStructure_e; typedef enum { ___4358, ___4356, END_VarLockMode_e, ___4357 = ___329 } VarLockMode_e; typedef enum { ___1374, ___1375, END_FieldMapMode_e, ___1373 = ___329 } FieldMapMode_e; typedef enum { ___4272, ___4275, ___4274, END_UnloadStrategy_e, ___4273 = ___329 } UnloadStrategy_e; struct ___4645 { ___514       ___3172; ___372          ___2025; }; struct ___4680 { std::string ___2683; ___1170  ___2973;
___1170  ___3784; double      ___3639; ___2225 ___2828; ___2225 ___2829; ___2225 ___2830; ___2225 ___2803; ___2225 ___2797; ___2225 ___2798; ZoneType_e     ___4234; ___372      ___2062; ___4645 ___4644; ___264     ___230; ___372      ___419; FaceNeighborMode_e ___1438; ___372          ___228; short tecplotFileZoneVersion; ___4680(ZoneType_e ___4689); ___4680(___4680 const& ___2886); ___4680& operator=(___4680 const& ___3390); ~___4680(); void setType(ZoneType_e ___4689); ___2225 numIPts() const; ___2225 numJPts() const; ___2225 numKPts() const; ___2225 numNodesPerFace() const; ___2225 ___2802() const; ___2225 numFaceBndryFaces() const; ___2225 numFaceBndryItems() const; void assignMetrics( ___2225  numIPts, ___2225  numJPts, ___2225  numKPts); void assignMetrics( ___2225  numIPts, ___2225  numJPts, ___2225  numKPts, ___2225  ___2802, ___2225  numFaceBndryFaces, ___2225  numFaceBndryItems); }; struct ___4075 { TextBox_e        ___411; double           ___2336; double           ___2288; ___514     ___351; ___514     ___1408; }; struct ___4116 { ___4262                              ___4261; ___54                             ___52; CoordSys_e                              ___3165; ___1170                              ___4597; ___372                               ___227; ___514                            ___351; ___4121                             ___4119; ___4075                               ___401; double                                  ___57; TextAnchor_e                            ___39; double                                  ___2286; Scope_e                                 ___3441; std::string                             ___2329; Clipping_e                              ___494; std::string                             Text; TextType_e                              TextType; struct ___4116* ___2703; struct ___4116* ___3180; }; struct ___1550 { ___1359  ___4291; ___1359  ___4293; ___1359  ___4295; }; struct ___3151 { ___1359  ___4140; ___1359  ___3273; }; struct ___446 { ___1359  ___4566; ___1359  ___4583; ___1359  ___4592; }; union ___1573 { ___1550   ___1546; ___446 ___4577; ___3151     ___4145; }; struct ___1630 { ___1630(); struct GeomAnchor { public: GeomAnchor( ); double XOrTheta() const; void setXOrTheta(double value); double YOrR() const; void setYOrR(double); double Z() const; void setZ(double); ___54 anchorPosition() const; bool operator==(GeomAnchor const&) const; bool operator!=(GeomAnchor const&) const; GeomAnchor& operator=(GeomAnchor const&); private: double         m_XOrTheta; double         m_YOrR; mutable double m_Z; mutable bool   m_zPosHasBeenAssigned; }; ___4262              ___4261; GeomType_e              ___1650;
CoordSys_e              ___3165; GeomAnchor              position; ___372               ___227; ___1170              ___4597; ___514            ___351; ___372               ___2021; ___514            ___1408; LinePattern_e           ___2262; double                  ___2985; double                  ___2288; Scope_e                 ___3441; DrawOrder_e             ___1111; Clipping_e              ___494; FieldDataType_e         ___905; char                   *___2329; ArrowheadStyle_e        ___188; ArrowheadAttachment_e   ___176; double                  ___187; double                  ___171; int32_t                 ___2792; char*                   ___1882; char*                   WorldFileName; ___2225               EmbeddedLpkImageNumber; ___372               ___2331; double                  ___3087; int32_t                 ___2834; ___3459           ___2836; ___1573              ___1571; ImageResizeFilter_e     ___1888; struct ___1630*          ___2702; struct ___1630*          ___3174; double _WorldFileAssignedWidth; double _WorldFileAssignedHeight; double _WorldFileAssignedXPos; double _WorldFileAssignedYPos; }; typedef struct ___4116* ___4112; typedef struct ___1630* ___1622; typedef enum { MarchingCubeAlgorithm_Classic, MarchingCubeAlgorithm_ClassicPlus, MarchingCubeAlgorithm_MC33, END_MarchingCubeAlgorithm_e, MarchingCubeAlgorithm_Invalid = ___329 } MarchingCubeAlgorithm_e; typedef enum { DataStoreStrategy_Auto, DataStoreStrategy_Heap, END_DataStoreStrategy_e, DataStoreStrategy_Invalid = ___329 } DataStoreStrategy_e; typedef enum { ArgListArgType_ArbParamPtr, ArgListArgType_DoublePtr, ArgListArgType_ArbParam, ArgListArgType_Array, ArgListArgType_Double, ArgListArgType_Function, ArgListArgType_Int, ArgListArgType_Set, ArgListArgType_String, ArgListArgType_StringList, ArgListArgType_StringPtr, ArgListArgType_Int64Array, END_ArgListArgType_e, ArgListArgType_Invalid = ___329 } ArgListArgType_e; typedef enum { StateModernizationLevel_Latest, StateModernizationLevel_LatestFocus, StateModernizationLevel_2006, StateModernizationLevel_2012, StateModernizationLevel_2012Focus, StateModernizationLevel_2013, StateModernizationLevel_2013Focus, StateModernizationLevel_2014, StateModernizationLevel_2014Focus, StateModernizationLevel_2016R1, StateModernizationLevel_2016R1Focus, StateModernizationLevel_2018R3, StateModernizationLevel_2018R3Focus, END_StateModernizationLevel_e, StateModernizationLevel_Invalid = ___329 } StateModernizationLevel_e; typedef ___372 (*___2906)(___3837 ___2905, ___90    ___3323); typedef void (*___2907)(___90 ___2904, ___90 ___3323); typedef void (*___2908)(___90 ___2904, ___90 ___3323); typedef ___372 (*___2860)(int32_t                  ___4456,
int32_t                  ___1825, ExportRegion_e           ___1235, ImageRenderingStrategy_e imageRenderingStrategy, ___90               ___3323, TP_OUT ___90*       ___1884); typedef void (*___2861)(___90 ___1884, ___90 ___3323); typedef ___372 (*___2862)(___90            ___1884, int32_t               ___3422, ___90            ___3323, TP_ARRAY_OUT uint8_t* ___3298, TP_ARRAY_OUT uint8_t* ___1807, TP_ARRAY_OUT uint8_t* ___365); typedef ___372 (*___4464)(HDC        ___3186, ___90 ___1884, Palette_e  ___2915, ___90 ___3323); typedef HDC(*___4463)(___90 ___3323); typedef ___372 (*___3334)(PageRenderDest_e ___2909, ___90       ___3335, ___90       ___3323); typedef ___372 (*___3348)(___90 ___2904, ___90 ___3323); typedef void (*___3345)(___90      ___2904, ___90      ___3323, TP_OUT int32_t* ___4456, TP_OUT int32_t* ___1825); typedef void (*___3907)(___90 ___2904, ___90 ___3323); typedef void (*___2154)(___90        ___3323, TP_OUT ___372* ___2056, TP_OUT ___372* ___1998, TP_OUT ___372* ___2008); typedef ___372 (*___2577)(int        ___420, ___90 ___3323); typedef void (*___4442)(___372  ___2, ___90 ___3323); typedef void (*___335)(CursorStyle_e ___692, ___90    ___3347, ___90    ___3323); typedef void (*___3470)(double ___4564, double ___4581, double ___4565, double ___4582, ___372     ___512, ___90    ___3323); typedef void (*___3200)(___90 ___3323); typedef void (*___1166)(___90 ___3323); typedef ___372 (*___1022)(___90 ___3323); typedef void (*___995)(___90 ___3323); typedef void (*___1099)(___90     ___3323, TP_OUT double* ___1836, TP_OUT double* ___2108); typedef ___372(*___1012)(const char* ___1957, char**      ___4328, ___372   ___3171, ___90  ___3322); typedef MessageBoxReply_e(*___1031)(const char*      ___2454, MessageBoxType_e ___2440, ___90       ___3323); typedef ___372 (*___1062)(SelectFileOption_e ___1036, const char*        ___1075, const char*        ___948, const char*        ___947, TP_GIVES char**    ___3361,
___90         ___3323); typedef void (*___3775)(const char* ___3778, ___90  ___3323); typedef void (*___3579)(const char* ___1419, const char* ___3456, ___90  ___3323); typedef void (*___3241)(int        ___3244, ___90 ___3323); typedef void (*___3243)(___372  ___3578, ___372  ___2027, ___90 ___3323); typedef void (*___3242)(___90 ___3323); typedef void (*___2616)(const char* ___2205, ___372 operationSucceeded, ___90  ___3323); typedef TP_GIVES char* (*OpenGLGetStringCallback_pf)(uint32_t ___2683, ___90  ___3323); typedef void (*___2408)(___90 ___3323); typedef ___372 (*___2410)(___90 ___3323); typedef ___372 (*___2411)(___90 ___3323); typedef void (*___3191)(___372 ___2032); typedef void (*___3192)(___372  ___4448, ___372  ___2032, ___90 ___492); typedef ___372 (*___1110)(RedrawReason_e ___3300, ___90     ___492); typedef ___372 (*WriteLayoutPreWriteCallback_pf)(___3499 PageList, ___90 ___492); typedef int (*___3842)(const char* ___3812, const char* ___3813, ___90  ___492); typedef ___372 (*DeferredZoneMetricsLoad_pf)(___90     clientData, ZoneMetrics_pa zoneMetrics); typedef void (*DeferredZoneMetricsCleanup_pf)(___90 clientData); typedef double(*___1381)(___1359 ___1307, ___2225    ___3247); typedef void (*___1382)(___1359 ___1307, ___2225    ___3247, double       ___4296); typedef ___372 (*___2309)(___1359 ___1350); typedef ___372 (*___2310)(___1359 ___1350); typedef void (*___2308)(___1359 ___1350); typedef ___2730 (*NodeMapGetValue_pf)(___2725 ___2721, ___2225  nodeOffset); typedef void (*NodeMapSetValue_pf)(___2725 ___2721, ___2225  nodeOffset, ___2730  nodeValue); typedef ___372 (*___2306)(___2725 ___2722); typedef ___372 (*___2307)(___2725 ___2722); typedef void (*___2305)(___2725 ___2722); typedef ___372 (*___2303)(___1290 ___1273); typedef ___372 (*___2304)(___1290 ___1273); typedef void (*___2302)(___1290 ___1273); typedef ___372 (*___2300)(___1269 ___1258); typedef ___372 (*___2301)(___1269 ___1258); typedef void (*___2299)(___1269 ___1258); typedef ___372 (*LoadOnDemandElemToFaceMapLoad_pf)(___1149 EFM);
typedef ___372 (*LoadOnDemandElemToFaceMapUnload_pf)(___1149 EFM); typedef void (*LoadOnDemandElemToFaceMapCleanup_pf)(___1149 EFM); typedef void (*___1248)(___2225 ___2827, double*   ___4574, double*   ___4591); typedef void (*___651)(const char*  ___3175, const char*  ___3176, ___3499       ___3177); typedef ___372 (*___886)(char*           ___848, char*           ___4039, TP_GIVES char** ___2454); typedef ___372 (*___896)(___3837 ___1958); typedef ___372 (*___859)(___3837 ___1958, ___90    ___492); typedef void (*___860)(); typedef void (* ___861)( ___3837 ___3461, ___372     ___21, ___372     ___82, ___90    ___492); typedef void (* ___862)( ___3837 ___3461, ___372     ___21, ___90    ___492); typedef ___372 (*___895)(___3837  ___1958); typedef ___372 (*___858)(___3837 ___1958, ___90    ___492); typedef ___372 (* ___932)(const char*   ___3460, ___90    ___492); typedef void (*___1662)(___3499        ___2253, ___3837 ___3462); typedef void (*___1658)(___1170      ___2251, char*           ___722, TP_GIVES char** ___0); typedef ___372 (*___1661)(___1359    ___3272, ___1359    ___3271, CoordScale_e    ___1928, CoordScale_e    ___961, ___2225       ___2831, ___1170      ___2251, char*           ___722, TP_GIVES char** ___721); typedef ___372 (*___1750)(___1359   ___3272, ___1359   ___3271, CoordScale_e   ___1928, CoordScale_e   ___961, ___2225      ___2831, ___2225      ___2788, ___1170     ___2251, char*          ___722, TP_OUT double* ___1923, TP_OUT double* ___960); typedef ___372 (*___1768)(___1359   ___3272, ___1359   ___3271, CoordScale_e   ___1928, CoordScale_e   ___961, ___2225      ___2831, ___2225      ___2788, ___1170     ___2252, char*          ___722, double         ___3193, TP_OUT double* ___3190); typedef ___372 (*___3173)(MSG *___3120); typedef ___372 (*___1134)(double          ___4312, ___90      ___492, TP_GIVES char** ___2172); typedef void (*___2872)(___90 ___492); typedef ___372 (*___3450)(const char *___3451, ___90  ___492); typedef ___372 (* ___2279)(___2225         ___4449, ___1170        ___4597, ___2225         ___448,
___2225         ___1250, TP_GIVES double * ___3159, ___90        ___492); typedef ___372 (*___4238)(const uint32_t* ___2211, const uint32_t* ___3393, ___90      ___492); typedef ___372 (*___4239)(const uint64_t* ___2211, const uint64_t* ___3393, ___90      ___492); typedef void (*TUAbort_pf)(const char* error_message); typedef ___3837 (* MatchVariablesCallback_pf)(___3837         existingVariables, ___3837 const * incomingVariableLists, ___2225             numIncomingVariableLists, ___90            clientData);
 #define ___1476          0
 #define ___1475          1
 #define ___1478              2
 #define ___1477 3
typedef struct ViewState_s* ___3430; typedef struct ViewState_s* ___4433; typedef struct ProbeInfo_s* ___3195; static const char* const ___4029 = "support@tecplot.com";
 #define ___4033 0 
 #define TECUTIL_DEFAULT_TEXT_ID -1
typedef ___90     TextID_t; typedef ___90     GeomID_t;

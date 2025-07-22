 #pragma once
#include "ItemAddress.h"
 #define UNINITIALIZED_REF ((void *)0x11111111)
 #define INVALID_FN_REF    ((void *)NULL)
 #define VALID_HANDLE(handle)       ((handle)!=0)
 #define VALID_FN_REF_OR_NULL(___3249) IMPLICATION((___3249) != NULL, VALID_FN_REF(___3249))
 #define VALID_TRANSLATED_STRING(___4226) (!(___4226).___2033())
struct ___800; bool VALID_FE_CLASSIC_CELL_INDEX( ___800 const* ___798, ___2225       ___460); bool VALID_FE_CELL_INDEX( ___800 const* ___798, ___2225       ___460); bool VALID_FE_CELL_INDEX( ___800 const*               ___798, tecplot::ItemAddress64 const& ___449);
 #define VALID_IPLANE_CELL_INDEX(___799,___461) \
 (  \
 (___461) >= 0 && \
 ___1840((___799),___461) <= MAX((___799)->___2809,1) && \
 ___2110((___799),___461) <  MAX((___799)->___2814,1) && \
 ___2155((___799),___461) <  MAX((___799)->___2817,1))
 #define VALID_JPLANE_CELL_INDEX(___799,___461) \
 (  \
 (___461) >= 0 && \
 ___1840((___799),___461) <  MAX((___799)->___2809,1) && \
 ___2110((___799),___461) <= MAX((___799)->___2814,1) && \
 ___2155((___799),___461) <  MAX((___799)->___2817,1))
 #define VALID_KPLANE_CELL_INDEX(___799,___461) \
 (  \
 (___461) >= 0 && \
 ___1840((___799),___461) <  MAX((___799)->___2809,1) && \
 ___2110((___799),___461) <  MAX((___799)->___2814,1) && \
 ___2155((___799),___461) <= MAX((___799)->___2817,1))
 #define VALID_ORDERED_CELL_INDEX(___799, ___461, ___3093) \
 (  \
 ((IJKPlanes_e)(___3093) == ___1865 || \
 (IJKPlanes_e)(___3093) == ___1870 || \
 (IJKPlanes_e)(___3093) == ___1872 || \
 (IJKPlanes_e)(___3093) == ___1874) && \
 \
   \
 (IMPLICATION(((IJKPlanes_e)(___3093) == ___1865 || \
 (IJKPlanes_e)(___3093) == ___1874), \
 VALID_IPLANE_CELL_INDEX((___799),___461)) && \
 IMPLICATION(((IJKPlanes_e)(___3093) == ___1870 || \
 (IJKPlanes_e)(___3093) == ___1874), \
 VALID_JPLANE_CELL_INDEX((___799),___461)) && \
 IMPLICATION(((IJKPlanes_e)(___3093) == ___1872 || \
 (IJKPlanes_e)(___3093) == ___1874), \
 VALID_KPLANE_CELL_INDEX((___799),___461))))
bool VALID_CELL_INDEX( ___800 const* ___798, ___2225       ___460, IJKPlanes_e     ___1863); bool VALID_CELL_INDEX( ___800 const*               ___798, tecplot::ItemAddress64 const& ___449, IJKPlanes_e                   ___1863);
 #define VALID_DATASET(___880,___482) (((___880) != NULL) && \
 IMPLICATION((___482),(___880)->___2845 >= 1))
 #define VALID_SET_INDEX(___3490) (((___3491)___3490)>=(___3491)1)
 #define VALID_FILE_HANDLE(___3790) ((___3790) != NULL)
 #define VALID_BASIC_COLOR(___351) \
 (___1418<=(___351) && (___351)<=___2193)
 #define VALID_CONTOUR_COLOR(Color) \
 (___612<=(Color) && \
 (Color)<___612+___1545.___2239.___2377+1)
 #define VALID_PLOTTING_COLOR(Color) \
 (VALID_BASIC_COLOR(Color) || VALID_CONTOUR_COLOR(Color))
 #define VALID_INTERFACE_SPECIFIC_COLOR(___351) \
 (___1421<=(___351) && (___351)<=___2198)
 #define VALID_INTERFACE_COLOR(Color) \
 (VALID_PLOTTING_COLOR(Color) || VALID_INTERFACE_SPECIFIC_COLOR(Color))
 #define VALID_MULTICOLOR_COLOR(Color) \
 (((Color) == ___2660) || ((Color) == ___2653) || \
 ((Color) == ___2654) || ((Color) == ___2655) || \
 ((Color) == ___2656) || ((Color) == ___2657) || \
 ((Color) == ___2658) || ((Color) == ___2659))
 #define VALID_RGB_COLOR(Color) \
 ((Color) == ___3373)
 #define VALID_ASSIGNABLE_COLOR(C) \
 (VALID_BASIC_COLOR(C)      || \
 VALID_MULTICOLOR_COLOR(C) || \
 VALID_RGB_COLOR(C))
 #define VALID_PEN_OFFSET(___2998) \
 (___364<=(___2998) && (___2998)<=___2824)
 #define VALID_PEN_OFFSET_FOR_OBJECT(___2998) \
 (___1422<=(___2998) && (___2998)<=___2200)
 #define VALID_NAME(___2684, ___2374) \
 (VALID_REF(___2684) && \
 (___2015(___2684) || \
 (!tecplot::isspace((___2684)[0]) && !tecplot::isspace((___2684)[strlen(___2684)-1]))) && \
 strlen(___2684) <= (___2374))
 #define VALID_ZONE_NAME(___2684) VALID_NAME((___2684), ___2356)
 #define VALID_VAR_NAME(___2684)  VALID_NAME((___2684), ___2354)
 #define VALID_LIGHTINGEFFECT(___2163) \
 (((___2163) == ___2237) || ((___2163) == ___2234))
 #if defined NO_ASSERTS
 #if !defined NOT_IMPLEMENTED
 #if defined ___1838
 #define NOT_IMPLEMENTED() ___476(___1303)
 #else
 #if defined MSWIN
 #define NOT_IMPLEMENTED(x)  ASSERT(!("Not Implemented"))
 #elif defined UNIXX
 #define NOT_IMPLEMENTED()  not ___1905 
 #endif
 #endif
 #endif
 #else
 #if defined NICE_NOT_IMPLEMENTED
 #define NOT_IMPLEMENTED() ___2704()
 #else
 #define NOT_IMPLEMENTED() ASSERT(!("Not Implemented"))
 #endif
 #endif
 #if defined NICE_NOT_IMPLEMENTED
extern void ___2704();
 #endif

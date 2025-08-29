 #if !defined ARRLIST_h
 #define ARRLIST_h
 #if defined EXTERN
 #  undef EXTERN
 #endif
 #if defined ARRLISTMODULE
 #  define EXTERN
 #else
 #  define EXTERN extern
 #endif
#include "CodeContract.h"
 #if !defined TECPLOTKERNEL
typedef struct ___135* ___134;
 #endif
typedef enum { ArrayListType_UInt8, ArrayListType_UInt16, ArrayListType_UInt32, ArrayListType_UInt64, ArrayListType_Int64, ArrayListType_Char, ArrayListType_Short, ArrayListType_Int, ArrayListType_Long, ArrayListType_Float, ArrayListType_Double, ArrayListType_LgIndex, ArrayListType_EntIndex, ArrayListType_SmInteger, ArrayListType_Boolean, ArrayListType_ArbParam, ArrayListType_UInt8Ptr, ArrayListType_UInt16Ptr, ArrayListType_UInt32Ptr, ArrayListType_UInt64Ptr, ArrayListType_Int64Ptr, ArrayListType_CharPtr, ArrayListType_ShortPtr, ArrayListType_IntPtr, ArrayListType_LongPtr, ArrayListType_FloatPtr, ArrayListType_DoublePtr, ArrayListType_LgIndexPtr, ArrayListType_EntIndexPtr, ArrayListType_SmIntegerPtr, ArrayListType_BooleanPtr, ArrayListType_ArbParamPtr, ArrayListType_VoidPtr, ArrayListType_FunctionPtr, ArrayListType_Any, END_ArrayListType_e, ArrayListType_Invalid = ___329 } ArrayListType_e; typedef union { uint8_t         UInt8; uint16_t        UInt16; uint32_t        UInt32; uint64_t        UInt64; int64_t         ___1967; char            ___470; short           Short; int             Int; long            Long; float           Float; double          Double; ___2225       ___2223; ___1170      ___1168; int32_t     ___3632; ___372       ___350; ___90      ___88; uint8_t*        UInt8Ptr; uint16_t*       UInt16Ptr; uint32_t*       UInt32Ptr; uint64_t*       UInt64Ptr; int64_t*        ___1968; char*           ___472; short*          ___3559; int*            ___1984; long*           ___2319; float*          ___1434; double*         ___1108; ___2225*      ___2224; ___1170*     ___1169; int32_t*    ___3633; ___372*      ___371; ___90*     ___89; void*           ___4437; void (*___1539)(); } ArrayListItem_u; typedef ___372 (*ArrayListItemVisitor_pf)(void*      ___2096, ___90 ___492);
 #if 0 
{ REQUIRE(VALID_REF(___4237)); REQUIRE(VALID_REF(*___4237) || *___4237 == NULL); ___372 ___1094 = ___4224; <type>* ___4237 = static_cast<<type>*>(___2096); ENSURE(VALID_BOOLEAN(___1094)); return ___1094; }
 #endif
typedef ArrayListItemVisitor_pf ArrayListItemDestructor_pf; typedef ___372 (*ArrayListItemDuplicator_pf)(void*      ___3947, void*      ___3643, ___90 ___492);
 #if 0 
{ REQUIRE(VALID_REF(___3948)); REQUIRE(VALID_REF(___3644)); REQUIRE(VALID_REF(*___3644) || *___3644 == NULL); ___372 ___2038 = ___4224; <type>* ___3948 = static_cast<<type>*>(___3947); <type>* ___3644 = static_cast<<type>*>(___3643); ENSURE(VALID_BOOLEAN(___2038)); return ___2038; }
 #endif
typedef ___2225 (*ArrayListCapacityRequestAdjuster_pf)(___134 ___94, ___2225    ___691, ___2225    ___3352, ___90   ___492);
 #if 0 
{ REQUIRE(ArrayListIsValid(___94)); REQUIRE((___3352 == 0 && ___691 == 0) || ___3352 > ___94->___437); ___2225 ___3357; ENSURE(___3357 == 0 || ___3357 >= ___3352); return ___3357; }
 #endif
struct ___135 { char*            Array; ArrayListType_e  ___4234; int32_t      ___2100; ___2225        ___682; ___2225        ___437; ___372        ___2078; ArrayListCapacityRequestAdjuster_pf ___438; ___90                          ___439; }; typedef int (*ArrayListItemComparator_pf)(ArrayListItem_u ___2085, ArrayListItem_u ___2086, ___90      ___492); EXTERN ___372 ArrayListIsValid(___134 ___94); EXTERN ArrayListType_e ArrayListGetType(___134 ___94); EXTERN ___372 ArrayListEnlargeCapacity(___134 ___94, ___2225    ___3352); EXTERN ___134 ArrayListAlloc(___2225                           ___1186, ArrayListType_e                     ___4234, ArrayListCapacityRequestAdjuster_pf ___438 = 0, ___90                          ___439 = 0); EXTERN void ArrayListDealloc(___134*              ___94, ArrayListItemDestructor_pf ___2092 = 0, ___90                 ___492 = 0); EXTERN void ArrayListDeleteAllItems(___134               ___94, ArrayListItemDestructor_pf ___2092 = 0, ___90                 ___492 = 0); EXTERN void ArrayListDeleteItems(___134               ___94, ___2225                  ___2095, ___2225                  ___682, ArrayListItemDestructor_pf ___2092 = 0, ___90                 ___492 = 0); EXTERN void ArrayListDeleteItem(___134               ___94, ___2225                  ___2095, ArrayListItemDestructor_pf ___2092 = 0, ___90                 ___492 = 0); EXTERN ___134 ArrayListRemoveItems(___134 ___94, ___2225    ___2095, ___2225    ___682); EXTERN ArrayListItem_u ArrayListRemoveItem(___134 ___94, ___2225    ___2095); EXTERN ___372 ArrayListInsertItem(___134    ___94, ___2225       ___2095, ArrayListItem_u ___2084); EXTERN ___372 ArrayListInsert(___134 ___3944, ___2225    ___2095, ___134 ___3640); EXTERN ___372 ArrayListVisitItems(___134            ___94, ___2225               ___2095, ___2225               ___682, ArrayListItemVisitor_pf ___2101, ___90              ___492); EXTERN ___134 ArrayListGetItems(___134 ___94, ___2225    ___2095, ___2225    ___682); EXTERN ArrayListItem_u ArrayListGetItem(___134 ___94, ___2225    ___2095); EXTERN ___372 ArrayListSetItem(___134               ___94, ___2225                  ___2095, ArrayListItem_u            ___2084, ArrayListItemDestructor_pf ___2092 = 0, ___90                 ___492 = 0); EXTERN ___372 ArrayListAppendItem(___134    ___94, ArrayListItem_u ___2084); EXTERN ___372 ArrayListAppend(___134 ___3944, ___134 ___3640); EXTERN ___134 ArrayListCopy(___134               ___94, ArrayListItemDuplicator_pf ___2093 = 0,
___90                 ___492 = 0); EXTERN void* ArrayListToArray(___134               ___94, ArrayListItemDuplicator_pf ___2093, ___90                 ___492); EXTERN ___134 ArrayListFromArray(void*                      ___3640, ___2225                  ___682, ArrayListType_e            ___4234, ArrayListItemDuplicator_pf ___2093 = 0, ___90                 ___492 = 0); EXTERN ___372 ArrayListBSearch(___134               ___94, ArrayListItem_u            ___2084, ArrayListItemComparator_pf ___533, ___90                 ___492, ___2225*                 ___2094 = 0);
 #if defined USE_MACROS_FOR_FUNCTIONS
 #  define ___112     ___113
 #  define ___115 ArrayListGetItemInternalRef_MACRO
 #  define ___101           ArrayListGetCount_MACRO
 #  define ___131(___94, ___2095)            ___124(___94, ___2095, uint8_t)
 #  define ___125(___94, ___2095)           ___124(___94, ___2095, uint16_t)
 #  define ___127(___94, ___2095)           ___124(___94, ___2095, uint32_t)
 #  define ___129(___94, ___2095)           ___124(___94, ___2095, uint64_t)
 #  define ___110(___94, ___2095)            ___124(___94, ___2095, int64_t)
 #  define ___99(___94, ___2095)             ___124(___94, ___2095, char)
 #  define ___120(___94, ___2095)            ___124(___94, ___2095, short)
 #  define ___109(___94, ___2095)              ___124(___94, ___2095, int)
 #  define ___118(___94, ___2095)             ___124(___94, ___2095, long)
 #  define ___106(___94, ___2095)            ___124(___94, ___2095, float)
 #  define ___102(___94, ___2095)           ___124(___94, ___2095, double)
 #  define ___116(___94, ___2095)          ___124(___94, ___2095, ___2225)
 #  define ___104(___94, ___2095)         ___124(___94, ___2095, ___1170)
 #  define ___122(___94, ___2095)        ___124(___94, ___2095, int32_t)
 #  define ___97(___94, ___2095)          ___124(___94, ___2095, ___372)
 #  define ___95(___94, ___2095)         ___124(___94, ___2095, ___90)
 #  define ___132(___94, ___2095)         ___124(___94, ___2095, uint8_t*)
 #  define ___126(___94, ___2095)        ___124(___94, ___2095, uint16_t*)
 #  define ___128(___94, ___2095)        ___124(___94, ___2095, uint32_t*)
 #  define ___130(___94, ___2095)        ___124(___94, ___2095, uint64_t*)
 #  define ___111(___94, ___2095)         ___124(___94, ___2095, int64_t*)
 #  define ___100(___94, ___2095)          ___124(___94, ___2095, char*)
 #  define ___121(___94, ___2095)         ___124(___94, ___2095, short*)
 #  define ___114(___94, ___2095)           ___124(___94, ___2095, int*)
 #  define ___119(___94, ___2095)          ___124(___94, ___2095, long*)
 #  define ___107(___94, ___2095)         ___124(___94, ___2095, float*)
 #  define ___103(___94, ___2095)        ___124(___94, ___2095, double*)
 #  define ___117(___94, ___2095)       ___124(___94, ___2095, ___2225*)
 #  define ___105(___94, ___2095)      ___124(___94, ___2095, ___1170*)
 #  define ___123(___94, ___2095)     ___124(___94, ___2095, int32_t*)
 #  define ___98(___94, ___2095)       ___124(___94, ___2095, ___372*)
 #  define ___96(___94, ___2095)      ___124(___94, ___2095, ___90*)
 #  define ___133(___94, ___2095)          ___124(___94, ___2095, void*)
 #  define ___108(___94, ___2095)      ___124(___94, ___2095, (**)(void))
 #else
 #  define ___112     ArrayListGetInternalRef_FUNC
 #  define ___115 ArrayListGetItemInternalRef_FUNC
 #  define ___101           ArrayListGetCount_FUNC
 #  define ___131(___94, ___2095)                    (*(static_cast<uint8_t*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___125(___94, ___2095)                  (*(static_cast<uint16_t*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___127(___94, ___2095)                  (*(static_cast<uint32_t*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___129(___94, ___2095)                  (*(static_cast<uint64_t*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___110(___94, ___2095)                    (*(static_cast<int64_t*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___99(___94, ___2095)                        (*(static_cast<char*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___120(___94, ___2095)                      (*(static_cast<short*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___109(___94, ___2095)                          (*(static_cast<int*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___118(___94, ___2095)                        (*(static_cast<long*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___106(___94, ___2095)                      (*(static_cast<float*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___102(___94, ___2095)                    (*(static_cast<double*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___116(___94, ___2095)                (*(static_cast<___2225*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___104(___94, ___2095)              (*(static_cast<___1170*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___122(___94, ___2095)            (*(static_cast<int32_t*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___97(___94, ___2095)                (*(static_cast<___372*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___95(___94, ___2095)              (*(static_cast<___90*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___132(___94, ___2095)                (*(static_cast<uint8_t**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ArrayListGetUInt16tPtr(___94, ___2095)             (*(static_cast<uint16_t**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ArrayListGetUInt32tr(___94, ___2095)               (*(static_cast<uint32_t**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___130(___94, ___2095)              (*(static_cast<uint64_t**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___111(___94, ___2095)                (*(static_cast<int64_t**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___100(___94, ___2095)                    (*(static_cast<char**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___121(___94, ___2095)                  (*(static_cast<short**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___114(___94, ___2095)                      (*(static_cast<int**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___119(___94, ___2095)                    (*(static_cast<long**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___107(___94, ___2095)                  (*(static_cast<float**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___103(___94, ___2095)                (*(static_cast<double**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___117(___94, ___2095)            (*(static_cast<___2225**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___105(___94, ___2095)          (*(static_cast<___1170**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___123(___94, ___2095)        (*(static_cast<int32_t**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___98(___94, ___2095)            (*(static_cast<___372**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___96(___94, ___2095)          (*(static_cast<___90**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___133(___94, ___2095)                    (*(static_cast<void**>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #  define ___108(___94, ___2095)             (*(static_cast<**(void)*>(const_cast<void*>(ArrayListGetItemInternalRef_FUNC(___94, ___2095)))))
 #endif
 #if !defined USE_MACROS_FOR_FUNCTIONS
EXTERN void const* ArrayListGetInternalRef_FUNC(___134 ___94); EXTERN void const* ArrayListGetItemInternalRef_FUNC(___134 ___94, ___2225    ___2095); EXTERN ___2225 ArrayListGetCount_FUNC(___134 ___94);
 #endif
 #define ___113(___94)                 static_cast<void const*>((___94)->Array)
 #define ArrayListGetItemInternalRef_MACRO(___94, ___2095) static_cast<void const*>(&((___94)->Array[(___2095)*(___94)->___2100]))
 #define ArrayListGetCount_MACRO(___94)                       ((___94)->___682)
 #define ArrayListGetTypedArrayRef(___94, ___2686)         reinterpret_cast<___2686*>((___94)->Array)
 #define ___124(___94, ___2095, ___2686) (ArrayListGetTypedArrayRef(___94,___2686)[___2095])
inline bool ArrayListOffsetWithinCapacity( ___134 arrayList, ___2225    itemOffset) { REQUIRE(itemOffset >= 0); return itemOffset < arrayList->___437; }
 #define ___161(___94, ___2095, ___2084, ___2686) \
 ((ArrayListOffsetWithinCapacity((___94), (___2095)) || \
 ArrayListEnlargeCapacity((___94), (___2095)+1)) \
 ? ((void)((ArrayListGetTypedArrayRef((___94),___2686)[(___2095)]) = (___2084)), \
 (((___2095)+1 > (___94)->___682) \
 ? (((___94)->___682 = (___2095)+1), ___4224) \
 : (___4224))) \
 : (___1303))
 #define ArrayListAppendTypedItem(___94, ___2084, ___2686) \
 ((ArrayListOffsetWithinCapacity((___94), (___94)->___682) || \
 ArrayListEnlargeCapacity((___94), (___94)->___682+1)) \
 ? ((void)((ArrayListGetTypedArrayRef((___94),___2686)[(___94)->___682]) = (___2084)), \
 (((___94)->___682 = (___94)->___682+1), ___4224)) \
 : (___1303))
 #define ___168(___94, ___2095, ___2084)            ___161((___94),(___2095),(___2084),uint8_t)
 #define ___162(___94, ___2095, ___2084)           ___161((___94),(___2095),(___2084),uint16_t)
 #define ___164(___94, ___2095, ___2084)           ___161((___94),(___2095),(___2084),uint32_t)
 #define ___166(___94, ___2095, ___2084)           ___161((___94),(___2095),(___2084),uint64_t)
 #define ___150(___94, ___2095, ___2084)            ___161((___94),(___2095),(___2084),int64_t)
 #define ___140(___94, ___2095, ___2084)             ___161((___94),(___2095),(___2084),char)
 #define ___157(___94, ___2095, ___2084)            ___161((___94),(___2095),(___2084),short)
 #define ___149(___94, ___2095, ___2084)              ___161((___94),(___2095),(___2084),int)
 #define ___155(___94, ___2095, ___2084)             ___161((___94),(___2095),(___2084),long)
 #define ___146(___94, ___2095, ___2084)            ___161((___94),(___2095),(___2084),float)
 #define ___142(___94, ___2095, ___2084)           ___161((___94),(___2095),(___2084),double)
 #define ___153(___94, ___2095, ___2084)          ___161((___94),(___2095),(___2084),___2225)
 #define ___144(___94, ___2095, ___2084)         ___161((___94),(___2095),(___2084),___1170)
 #define ___159(___94, ___2095, ___2084)        ___161((___94),(___2095),(___2084),int32_t)
 #define ___138(___94, ___2095, ___2084)          ___161((___94),(___2095),(___2084),___372)
 #define ___136(___94, ___2095, ___2084)         ___161((___94),(___2095),(___2084),___90)
 #define ___169(___94, ___2095, ___2084)         ___161((___94),(___2095),(___2084),uint8_t*)
 #define ___163(___94, ___2095, ___2084)       ___161((___94),(___2095),(___2084),uint16_t*)
 #define ___165(___94, ___2095, ___2084)         ___161((___94),(___2095),(___2084),uint32_t*)
 #define ___167(___94, ___2095, ___2084)        ___161((___94),(___2095),(___2084),uint64_t*)
 #define ___151(___94, ___2095, ___2084)         ___161((___94),(___2095),(___2084),int64_t*)
 #define ___141(___94, ___2095, ___2084)          ___161((___94),(___2095),(___2084),char*)
 #define ___158(___94, ___2095, ___2084)         ___161((___94),(___2095),(___2084),short*)
 #define ___152(___94, ___2095, ___2084)           ___161((___94),(___2095),(___2084),int*)
 #define ___156(___94, ___2095, ___2084)          ___161((___94),(___2095),(___2084),long*)
 #define ___147(___94, ___2095, ___2084)         ___161((___94),(___2095),(___2084),float*)
 #define ___143(___94, ___2095, ___2084)        ___161((___94),(___2095),(___2084),double*)
 #define ___154(___94, ___2095, ___2084)       ___161((___94),(___2095),(___2084),___2225*)
 #define ___145(___94, ___2095, ___2084)      ___161((___94),(___2095),(___2084),___1170*)
 #define ___160(___94, ___2095, ___2084)     ___161((___94),(___2095),(___2084),int32_t*)
 #define ___139(___94, ___2095, ___2084)       ___161((___94),(___2095),(___2084),___372*)
 #define ___137(___94, ___2095, ___2084)      ___161((___94),(___2095),(___2084),___90*)
 #define ___170(___94, ___2095, ___2084)          ___161((___94),(___2095),(___2084),void*)
 #define ___148(___94, ___2095, ___2084)      ___161((___94),(___2095),(___2084),(**)(void))
 #define ArrayListAppendUInt8(___94, ___2084)            ArrayListAppendTypedItem((___94),(___2084),uint8_t)
 #define ArrayListAppendUInt16(___94, ___2084)           ArrayListAppendTypedItem((___94),(___2084),uint16_t)
 #define ArrayListAppendUInt32(___94, ___2084)           ArrayListAppendTypedItem((___94),(___2084),uint32_t)
 #define ArrayListAppendUInt64(___94, ___2084)           ArrayListAppendTypedItem((___94),(___2084),uint64_t)
 #define ArrayListAppendInt64(___94, ___2084)            ArrayListAppendTypedItem((___94),(___2084),int64_t)
 #define ArrayListAppendChar(___94, ___2084)             ArrayListAppendTypedItem((___94),(___2084),char)
 #define ArrayListAppendShort(___94, ___2084)            ArrayListAppendTypedItem((___94),(___2084),short)
 #define ArrayListAppendInt(___94, ___2084)              ArrayListAppendTypedItem((___94),(___2084),int)
 #define ArrayListAppendLong(___94, ___2084)             ArrayListAppendTypedItem((___94),(___2084),long)
 #define ArrayListAppendFloat(___94, ___2084)            ArrayListAppendTypedItem((___94),(___2084),float)
 #define ArrayListAppendDouble(___94, ___2084)           ArrayListAppendTypedItem((___94),(___2084),double)
 #define ArrayListAppendLgIndex(___94, ___2084)          ArrayListAppendTypedItem((___94),(___2084),___2225)
 #define ArrayListAppendEntIndex(___94, ___2084)         ArrayListAppendTypedItem((___94),(___2084),___1170)
 #define ArrayListAppendSmInteger(___94, ___2084)        ArrayListAppendTypedItem((___94),(___2084),int32_t)
 #define ArrayListAppendBoolean(___94, ___2084)          ArrayListAppendTypedItem((___94),(___2084),___372)
 #define ArrayListAppendArbParam(___94, ___2084)         ArrayListAppendTypedItem((___94),(___2084),___90)
 #define ArrayListAppendUInt8Ptr(___94, ___2084)         ArrayListAppendTypedItem((___94),(___2084),uint8_t*)
 #define ArrayListAppendUInt16tPtr(___94, ___2084)       ArrayListAppendTypedItem((___94),(___2084),uint16_t*)
 #define ArrayListAppendUInt32tr(___94, ___2084)         ArrayListAppendTypedItem((___94),(___2084),uint32_t*)
 #define ArrayListAppendUInt64Ptr(___94, ___2084)        ArrayListAppendTypedItem((___94),(___2084),uint64_t*)
 #define ArrayListAppendInt64Ptr(___94, ___2084)         ArrayListAppendTypedItem((___94),(___2084),int64_t*)
 #define ArrayListAppendCharPtr(___94, ___2084)          ArrayListAppendTypedItem((___94),(___2084),char*)
 #define ArrayListAppendShortPtr(___94, ___2084)         ArrayListAppendTypedItem((___94),(___2084),short*)
 #define ArrayListAppendIntPtr(___94, ___2084)           ArrayListAppendTypedItem((___94),(___2084),int*)
 #define ArrayListAppendLongPtr(___94, ___2084)          ArrayListAppendTypedItem((___94),(___2084),long*)
 #define ArrayListAppendFloatPtr(___94, ___2084)         ArrayListAppendTypedItem((___94),(___2084),float*)
 #define ArrayListAppendDoublePtr(___94, ___2084)        ArrayListAppendTypedItem((___94),(___2084),double*)
 #define ArrayListAppendLgIndexPtr(___94, ___2084)       ArrayListAppendTypedItem((___94),(___2084),___2225*)
 #define ArrayListAppendEntIndexPtr(___94, ___2084)      ArrayListAppendTypedItem((___94),(___2084),___1170*)
 #define ArrayListAppendSmIntegerPtr(___94, ___2084)     ArrayListAppendTypedItem((___94),(___2084),int32_t*)
 #define ArrayListAppendBooleanPtr(___94, ___2084)       ArrayListAppendTypedItem((___94),(___2084),___372*)
 #define ArrayListAppendArbParamPtr(___94, ___2084)      ArrayListAppendTypedItem((___94),(___2084),___90*)
 #define ArrayListAppendVoidPtr(___94, ___2084)          ArrayListAppendTypedItem((___94),(___2084),void*)
 #define ArrayListAppendFunctionPtr(___94, ___2084)      ArrayListAppendTypedItem((___94),(___2084),(**)(void))
 #endif 

#include "stdafx.h"
#include "MASTER.h"
 #define ___1557
#include "GLOBAL.h"
#include "CodeContract.h"
#include "TASSERT.h"
#include "ALLOC.h"
#include "GEOM.h"
#include "TEXT.h"
#include "STRUTIL.h"
#include "GEOM2.h"
#include "DATASET0.h"
FieldDataType_e ___1745(___1630 const* ___1554) { FieldDataType_e ___3357; REQUIRE(VALID_REF(___1554)); REQUIRE(VALID_REF(___1554->___1571.___1546.___4291)); ___3357 = ___1554->___905; ENSURE(VALID_GEOM_FIELD_DATA_TYPE(___3357)); ENSURE(IMPLICATION(VALID_REF(___1554->___1571.___1546.___4291), ___3357 == ___1724(___1554->___1571.___1546.___4291))); ENSURE(IMPLICATION(VALID_REF(___1554->___1571.___1546.___4293), ___3357 == ___1724(___1554->___1571.___1546.___4293))); ENSURE(IMPLICATION(VALID_REF(___1554->___1571.___1546.___4295), ___3357 == ___1724(___1554->___1571.___1546.___4295))); return ___3357; }

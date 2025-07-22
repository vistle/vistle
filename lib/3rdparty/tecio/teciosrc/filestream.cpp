#include "stdafx.h"
#include "MASTER.h"
 #define ___1402
#include "GLOBAL.h"
#include "CodeContract.h"
#include "ALLOC.h"
#include "SYSTEM.h"
#include "FILESTREAM.h"
___1403 *___1400(FILE      *File, ___372  ___2005) { REQUIRE(VALID_REF(File) || File == NULL); ___1403 *___3357 = ALLOC_ITEM(___1403, "FileStream"); if (___3357 != NULL) { ___3357->File              = File; ___3357->___2005 = ___2005; } ENSURE(VALID_REF(___3357) || ___3357 == NULL); return ___3357; } void ___1401(___1403 **___1399) { REQUIRE(VALID_REF(___1399)); REQUIRE(VALID_REF(*___1399) || *___1399 == NULL); if (*___1399 != NULL) { ___1529(*___1399, "FileStream"); *___1399 = NULL; } ENSURE(*___1399 == NULL); }

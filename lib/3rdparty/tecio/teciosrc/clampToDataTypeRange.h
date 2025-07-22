 #pragma once
#include "MASTER.h"
#include "GLOBAL.h"
#include "CodeContract.h"
inline double clampToDataTypeRange(double ___4296, FieldDataType_e ___1361) { switch (___1361) { case FieldDataType_Float: return ___648(___4296); case FieldDataType_Double: return ___487(___4296); case FieldDataType_Int32: return ___650(___4296); case FieldDataType_Int16: return ___649(___4296); case FieldDataType_Byte: return CONVERT_DOUBLE_TO_UINT8(___4296); case ___1363: return (___4296 < 1.0 ? 0.0 : 1.0); default: ___476(___1303); return ___2177; } }

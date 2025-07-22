 #pragma once
#include "MASTER.h"
#include "GLOBAL.h"
#include "FileReaderInterface.h"
#include "LightweightVector.h"
#include "ioDescription.h"
 #define STRING_SIZE 100
 #define STRING_FORMAT "%99s"
namespace tecplot { namespace ___3931 { template<typename T, bool ___2023, int baseValue> ___372 readValues( ___1397& file, size_t               ___2794, T*                   ___4297, IODescription const& ___970); template<typename T, bool ___2023, int base> ___372 readValueArray( ___1397&  file, size_t                ___2863, size_t                ___2793, ___2238<T>& ___4297, IODescription const&  ___970); template<typename T> ___372 readMinMaxArray( ___1397& file, size_t               ___2863, size_t               ___2793, ___2479&         ___2478, IODescription const& ___970); template<typename T, bool ___2023> ___372 readValue( ___1397& file, T&                   ___4296, IODescription const& ___970); template<typename T, bool ___2023> ___372 readAndVerifyValue( ___1397& file, T const              expectedVal, IODescription const& ___970); ___372 readString( ___1397& file, size_t               length, ___471&           ___4296, IODescription const& ___970); ___372 readStringArray( ___1397& file, size_t               ___2863, size_t               ___2810, ___3814&         itemNameArray, IODescription const& ___970); }}

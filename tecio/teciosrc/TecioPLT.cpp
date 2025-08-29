#include "ThirdPartyHeadersBegin.h"
#include <vector>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "CodeContract.h"
#include "SYSTEM.h"
#include "FILESTREAM.h"
#include "TecplotVersion.h"
#include "CHARTYPE.h"
#include "DATAIO4.h"
#include "DATASET0.h"
#include "TecioPLT.h"
#include "DATAUTIL.h"
#include "ALLOC.h"
#include "FileSystem.h"
 #if !defined MAKEARCHIVE
#include "AUXDATA.h"
 #endif 
 #if defined MSWIN
#include <io.h>
 #endif
 #if defined UNIXX
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
 #endif
 #define TECIO_STRINGIZE(s) #s
 #if defined MAKEARCHIVE
 #if defined MSWIN && defined _DEBUG
 #define ___3182(fmt) do { OutputDebugStringA((fmt)); printf(fmt); } while (0)
 #define ___3183(fmt,p1) do { char ___416##__FILE__##__LINE__[512]; sprintf(___416##__FILE__##__LINE__,(fmt),(p1)); OutputDebugStringA(___416##__FILE__##__LINE__); printf(___416##__FILE__##__LINE__); } while (0)
 #define ___3184(fmt,p1,p2) do { char ___416##__FILE__##__LINE__[512]; sprintf(___416##__FILE__##__LINE__,(fmt),(p1),(p2)); OutputDebugStringA(___416##__FILE__##__LINE__); printf(___416##__FILE__##__LINE__); } while (0)
 #define ___3185(fmt,p1,p2,p3) do { char ___416##__FILE__##__LINE__[512]; sprintf(___416##__FILE__##__LINE__,(fmt),(p1),(p2),(p3)); OutputDebugStringA(___416##__FILE__##__LINE__); printf(___416##__FILE__##__LINE__); } while (0)
 #define PRINT4(fmt,p1,p2,p3,p4) do { char ___416##__FILE__##__LINE__[512]; sprintf(___416##__FILE__##__LINE__,(fmt),(p1),(p2),(p3),(p4)); OutputDebugStringA(___416##__FILE__##__LINE__); printf(___416##__FILE__##__LINE__); } while (0)
 #else
 #define ___3182(fmt) printf((fmt))
 #define ___3183(fmt,p1) printf((fmt),(p1))
 #define ___3184(fmt,p1,p2) printf((fmt),(p1),(p2))
 #define ___3185(fmt,p1,p2,p3) printf((fmt),(p1),(p2),(p3))
 #define PRINT4(fmt,p1,p2,p3,p4) printf((fmt),(p1),(p2),(p3),(p4))
 #endif
 #else
 #if defined MSWIN
 #define ___3182(fmt) ((void)0)
 #define ___3183(fmt,p1) ((void)0)
 #define ___3184(fmt,p1,p2) ((void)0)
 #define ___3185(fmt,p1,p2,p3) ((void)0)
 #define PRINT4(fmt,p1,p2,p3,p4) ((void)0)
 #else
 #define ___3182(fmt) printf((fmt))
 #define ___3183(fmt,p1) printf((fmt),(p1))
 #define ___3184(fmt,p1,p2) printf((fmt),(p1),(p2))
 #define ___3185(fmt,p1,p2,p3) printf((fmt),(p1),(p2),(p3))
 #define PRINT4(fmt,p1,p2,p3,p4) printf((fmt),(p1),(p2),(p3))
 #endif
 #endif
 #define ___2378    10
 #define BYTES_PER_CHUNK 4096
 #define TECIO_NO_NEIGHBORING_ELEM 0
 #define TECIO_NO_NEIGHBORING_ZONE 0
namespace {
 #if defined MAKEARCHIVE
___2225                 DebugLevel[___2378] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
 #endif
int32_t                   ___2040[___2378]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; int32_t                   NumErrs[___2378] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; int32_t                   ___2845[___2378] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; int32_t                   NumVars[___2378]; ___372                 fileContains64BitFaceCounts[___2378] = { ___1303, ___1303, ___1303, ___1303, ___1303, ___1303, ___1303, ___1303, ___1303, ___1303 }; char*                     DestFName[___2378] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }; char*                     BlckFName[___2378] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }; ___1403*             BlckFile[___2378]; ___1403*             HeadFile[___2378]; std::vector<___1396> MinMaxOffset[___2378]; std::vector<double>       VarMinValue[___2378]; std::vector<double>       VarMaxValue[___2378]; ___372                 DoWriteForeign = ___1303; ___372                 IsWritingNative[___2378]; ___372                 ___2004[___2378]; int32_t                   ___4690[___2378]; int32_t                   ___1904[___2378]; int32_t                   ___2112[___2378]; int64_t                   ___2157[___2378]; std::vector<int64_t>      ___4191[___2378]; int32_t                   ___4189[___2378]; int32_t                   ___4188[___2378];
 #if !defined ___2180
 #define ___2180 ((int32_t)(0x7FFFFFFF-1)) 
 #endif
int64_t                   InternalPolyFace32BitLimit = ___2180; struct PolyZoneWriteInfo { PolyZoneWriteInfo() : zoneUses64BitFaceCounts(___1303) , numFacesWritten(0) , faceNodeSum(0) , numBoundaryFacesWritten(0) , boundaryConnectionSum(0) , faceMetadataOffset(0) , faceNodesOffset(0) , leftElemsOffset(0) , rightElemsOffset(0) , connectionCountsOffset(0) , connectionElemsOffset(0) , connectionZonesOffset(0) {} ___372    zoneUses64BitFaceCounts; int64_t      numFacesWritten; int64_t      faceNodeSum; int32_t      numBoundaryFacesWritten; int32_t      boundaryConnectionSum; ___1396 faceMetadataOffset; ___1396 faceNodesOffset; ___1396 leftElemsOffset; ___1396 rightElemsOffset; ___1396 connectionCountsOffset; ___1396 connectionElemsOffset; ___1396 connectionZonesOffset; }; std::vector<PolyZoneWriteInfo> PolyZoneWriteInfos[___2378]; std::vector<int32_t>           ___2801[___2378]; int32_t                        ___1283[___2378]; std::vector<int32_t>           FaceNeighborsOrMapWritten[___2378]; int32_t                        NumIndices[___2378]; ___2225                      NumDataValuesWritten[___2378]; ___2225                      NumOrderedCCDataValuesWritten[___2378]; ___2225                      NumDataValuesToWrite[___2378]; std::vector<___2225>         NumRunningVarValues[___2378]; std::vector<___372>         IsSharedVar[___2378]; std::vector<___372>         IsPassiveVar[___2378]; int32_t                        ___732[___2378]; int32_t                        ___716[___2378]; int32_t                        ___1362[___2378]; std::vector<___372>         IsCellCentered[___2378]; ___372                      HasFECONNECT[___2378]; int32_t                        FileTypes[___2378]; std::vector<___2225>         NumConnectivityNodes[___2378]; std::vector<___2225>         NumConnectivityNodesWritten[___2378]; std::vector<___372>         ConnectivityWritten[___2378]; }
 #define ___2880 0
 #define ___1338 1
 #define ___1346 2
 #define ___1341 3
 #define ___1345 4
 #define FEBRICK 5
 #define ___1339 6
 #define ___1340 7
 #define ___1533 0
 #define ___1809 1
 #define ___3636 2
 #if defined MAKEARCHIVE
namespace { char const* ZoneTypes[] = { "ORDERED", "FELINESEG", "FETRIANGLE", "FEQUADRILATERAL", "FETETRAHEDRON", "FEBRICK", "FEPOLYGON", "FEPOLYHEDRON" }; }
 #endif 
namespace { void WriteErr( int32_t    ___1395, char const* routine_name) {
 #if defined MAKEARCHIVE
{ ___3184("Err: (%s) Write failure on file %d.\n", routine_name, ___1395 + 1); }
 #else
{ ___4276(routine_name); }
 #endif
NumErrs[___1395]++; } } namespace { ___1403* OpenFileStream( char const* FilePath, char const* AccessMode, ___372   ___2005) { REQUIRE(VALID_REF(FilePath)); REQUIRE(VALID_REF(AccessMode)); ___1403 *___3357 = NULL; FILE         *File   = tecplot::filesystem::fileOpen(FilePath, AccessMode); if (File != NULL) { ___3357 = ___1400(File, ___2005); if (___3357 == NULL) ___4193(File); } ENSURE((VALID_REF(___3357) && VALID_REF(___3357->File)) || ___3357 == NULL); return ___3357; } } namespace { void CloseFileStream(___1403** ___1399) { REQUIRE(VALID_REF(___1399)); REQUIRE(VALID_REF(*___1399) || *___1399 == NULL); if (*___1399 != NULL) { ___4193((*___1399)->File); ___1401(___1399); } ENSURE(*___1399 == NULL); } } namespace { char getBestTerminatorChar(char const* CompoundStr) { REQUIRE(VALID_REF(CompoundStr)); if (strchr(CompoundStr, '\n') != NULL) return '\n'; else if (strchr(CompoundStr, ',') != NULL) return ','; else return ' '; } } int32_t ___3980(int32_t ___1395) { return ___2845[___1395]; } int32_t ___3979(int32_t ___1395) { return NumVars[___1395]; } int32_t TECIOPLT_InternalGetPolyFace32BitLimit(int64_t* value) { REQUIRE(VALID_REF(value)); INVARIANT(0 <= InternalPolyFace32BitLimit && InternalPolyFace32BitLimit <= ___2180); *value = InternalPolyFace32BitLimit; return 0; } int32_t TECIOPLT_InternalSetPolyFace32BitLimit(int64_t* value) { for (int f = 0; f < ___2378; ++f) { if (___2040[f]) return -1; } if (0 <= *value && *value <= ___2180) { InternalPolyFace32BitLimit = *value; return 0; } return -1; } int32_t ___3975( int32_t        ___1395, char const*    ___4176, char const*    ___4348, char const*    ___1437, char const*    ___3446, int32_t const* ___1406, int32_t const* ___940, int32_t const* ___4436) {
 #if defined MAKEARCHIVE
___1933();
 #endif
if (___2040[___1395]) {
 #if defined MAKEARCHIVE
___3183("Err: (TECINI142) file %d is already open.\n", ___1395+1);
 #endif
return (-1); } ___2845[___1395] = 0;
 #if defined MAKEARCHIVE
{ DebugLevel[___1395] = *___940; }
 #else
{ ___4276(___940); }
 #endif
___476(VarMinValue[___1395].empty()); ___476(VarMaxValue[___1395].empty()); ___476(NumRunningVarValues[___1395].empty()); ___476(IsSharedVar[___1395].empty()); ___476(IsPassiveVar[___1395].empty()); ___476(IsCellCentered[___1395].empty()); ___476(MinMaxOffset[___1395].empty()); ___476(___4191[___1395].empty()); ___476(PolyZoneWriteInfos[___1395].empty()); ___476(___2801[___1395].empty()); ___476(FaceNeighborsOrMapWritten[___1395].empty()); ___476(NumConnectivityNodes[___1395].empty()); ___476(NumConnectivityNodesWritten[___1395].empty()); ___476(ConnectivityWritten[___1395].empty()); ___732[___1395] = -1; size_t ___2163 = 0; if (___1437 != NULL) ___2163 = strlen(___1437); if (___2163 == 0) {
 #if defined MAKEARCHIVE
___3183("Err: (TECINI142) Bad file name for file %d.\n", ___1395+1);
 #endif
return (-1); } DestFName[___1395] = ___23(___2163 + 1, char, "data set fname"); strcpy(DestFName[___1395], ___1437); char RName[80];
 #if defined (___1098)
{ sprintf(RName, "BLCKFILE.%03d", (int)(___1395 + 1)); }
 #else
{ sprintf(RName, "tp%1dXXXXXX", ___1395 + 1); }
 #endif
___2163 = strlen(RName); if (___3446 != NULL) ___2163 += strlen(___3446) + 1; BlckFName[___1395] = ___23(___2163 + 1, char, "data set fname"); if (___3446 != NULL) { strcpy(BlckFName[___1395], ___3446);
 #if defined ___1098 || defined MSWIN
{ strcat(BlckFName[___1395], "\\"); }
 #else
{ strcat(BlckFName[___1395], "/"); }
 #endif
} else { BlckFName[___1395][0] = '\0'; } strcat(BlckFName[___1395], RName); ___476(strlen(BlckFName[___1395]) <= ___2163);
 #if defined MSWIN
{ if (!_mktemp(BlckFName[___1395])) { WriteErr(___1395, "Error creating new file name."); return (-1); } }
 #elif defined UNIXX
{ mode_t OrigUmask = umask(0022); int FileDesc = mkstemp(BlckFName[___1395]); if (FileDesc != -1) close(FileDesc); umask(OrigUmask); }
 #endif
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) { ___3184("Scratch File #%d: %s\n", ___1395 + 1, BlckFName[___1395]); ___3184("Dest    File #%d: %s\n", ___1395 + 1, DestFName[___1395]); }
 #endif
IsWritingNative[___1395] = !DoWriteForeign; HeadFile[___1395] = OpenFileStream(DestFName[___1395], "wb", IsWritingNative[___1395]); BlckFile[___1395] = OpenFileStream(BlckFName[___1395], "wb", IsWritingNative[___1395]); if (BlckFile[___1395] == NULL) {
 #if defined MAKEARCHIVE
___3182("Err: (TECINI142) Cannot open scratch file for output.\n"); ___3182("     Check permissions in scratch directory.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (HeadFile[___1395] == NULL) {
 #if defined MAKEARCHIVE
___3182("Err: (TECINI142) Cannot open plot file.  Check permissions.\n");
 #endif
NumErrs[___1395]++; return (-1); } ___4491(*HeadFile[___1395], TecplotSDKBinaryFileVersion); ___4489(HeadFile[___1395]); if (*___1406 >= ___1533 && *___1406 <= ___3636) FileTypes[___1395] = *___1406; else {
 #if defined MAKEARCHIVE
___3182("Err: (TECINI142) Bad filetype argument.  Check documentation.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (!___4487(HeadFile[___1395], (___2225)FileTypes[___1395])) { WriteErr(___1395, "TECINI142"); return (-1); } if (!___1129(HeadFile[___1395], ___4176, ___4224)) { WriteErr(___1395, "TECINI142"); return (-1); } NumVars[___1395] = 0; char const* ___683 = ___4348; { char terminator = getBestTerminatorChar(___683); while (*___683) { while (*___683 && *___683 == ' ') ___683++; if (*___683) { NumVars[___1395]++; while (*___683 && *___683 != terminator) ___683++; if (*___683) ___683++; } } }
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) ___3183("NumVars=%d\n", NumVars[___1395]);
 #endif
try { VarMinValue[___1395].resize(NumVars[___1395]); VarMaxValue[___1395].resize(NumVars[___1395]); NumRunningVarValues[___1395].resize(NumVars[___1395]); IsSharedVar[___1395].resize(NumVars[___1395]); IsPassiveVar[___1395].resize(NumVars[___1395]); IsCellCentered[___1395].resize(NumVars[___1395]); } catch (std::bad_alloc const&) {
 #if defined MAKEARCHIVE
___3182("Err: (TECINI142) Memory allocation error.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (!___4487(HeadFile[___1395], (___2225)NumVars[___1395])) { WriteErr(___1395, "TECINI142"); return (-1); } ___683 = ___4348; { char TString[___2354+1]; int ___1830; char terminator = getBestTerminatorChar(___683); while (*___683) { while (*___683 && *___683 == ' ') ___683++; if (*___683) { ___1830 = 0; while (*___683 && *___683 != terminator) { TString[___1830++] = *___683++; } if (*___683) ___683++; ___1830--; while (___1830 >= 0 && TString[___1830] == ' ') ___1830--; TString[___1830+1] = '\0'; if (!___1129(HeadFile[___1395], TString, ___4224)) { WriteErr(___1395, "TECINI142"); return (-1); } } } } ___2040[___1395] = 1; fileContains64BitFaceCounts[___1395] = ___1303; if (*___4436) ___1362[___1395] = FieldDataType_Double; else ___1362[___1395] = FieldDataType_Float; return (0); } namespace { int CheckData( int32_t    ___1395, char const* routine_name) { if (NumDataValuesToWrite[___1395] != NumDataValuesWritten[___1395]) {
 #if defined MAKEARCHIVE
{ ___3184("Err: (%s) Wrong number of data values in file %d:\n", routine_name, ___1395 + 1); ___3184("     %lld data values for Zone %d were processed,\n", (long long)NumDataValuesWritten[___1395], ___732[___1395] + 1); ___3183("     %lld data values were expected.\n", (long long)NumDataValuesToWrite[___1395]); }
 #else
{ ___4276(routine_name); }
 #endif
NumErrs[___1395]++; return (-1); } return (0); } } namespace { int CheckFile( int32_t    ___1395, char const* routine_name) { if ((___1395 < 0) || (___1395 >= ___2378)) { ___3185("Err: (%s) Attempt to use invalid file %d file must be between 1 and %d inclusive.\n", routine_name, ___1395+1, ___2378); return (-1); } if (!___2040[___1395]) { ___3184("Err: (%s) Attempt to use file %d that hasn't been initialized with TECINI142.\n", routine_name, ___1395+1); return (-1); } return (0); } } namespace { void AdvanceToNextVarWithValues(int32_t ___1395) { do { ___716[___1395]++; } while (___716[___1395] < NumVars[___1395] && (IsSharedVar[___1395][___716[___1395]] || IsPassiveVar[___1395][___716[___1395]])); } } int32_t ___3990( const char*    routineName, int32_t        ___1395, char const*    ___4595, int32_t const* ___4596, int32_t const* ___1908, int32_t const* ___2115, int64_t const* ___2160, double const*  ___3639, int32_t const* ___3784, int32_t const* /*___2973*/, int32_t const* ___2003, int32_t const* ___2799, int32_t const* ___1438, int64_t const* ___2803, int32_t const* ___2797, int32_t const* ___2796, int32_t const* ___2981, int32_t const* ___4324, int32_t const* ___3550, int32_t const* ___3548) { int ___1830; if (CheckFile(___1395, routineName) < 0) return (-1); if (___732[___1395] > -1) { if (CheckData(___1395, routineName) < 0) return (-1); } if (NumVars[___1395] == 0) { WriteErr(___1395, routineName);
 #if defined MAKEARCHIVE
___3184("Err: (%s) Cannot write out zones if numvars is equal to zero (file %d).\n", routineName, ___1395 + 1);
 #endif
return (-1); } if (___732[___1395] > ___2380 - 2) { WriteErr(___1395, routineName);
 #if defined MAKEARCHIVE
___3185("Err: (%s) Exceeded max number of zones (%d) in file %d.\n", routineName, ___2380, ___1395 + 1);
 #endif
return (-1); } if (*___3784 < -1) {
 #if defined MAKEARCHIVE
___3185("Err: (%s) Invalid StrandID supplied for file %d, zone %d.\n", routineName, ___1395 + 1, ___732[___1395] + 1 + 1);
 #endif
return (-1); } if (*___2003 != 1) {
 #if defined MAKEARCHIVE
___3185("Err: (%s) Point data is not currently allowed. " " Please use block format for file %d, zone %d.\n", routineName, ___1395 + 1, ___732[___1395] + 1 + 1);
 #endif
return (-1); } ___4198(BlckFile[___1395]->File, 0, SEEK_END); ___372 zoneUses64BitFaceCounts = ___1303; NumDataValuesWritten[___1395] = 0; NumOrderedCCDataValuesWritten[___1395] = 0; ___732[___1395]++; try { MinMaxOffset[___1395].resize(___732[___1395] + 1); ___4191[___1395].resize(___732[___1395] + 1); PolyZoneWriteInfos[___1395].resize(___732[___1395] + 1); ___2801[___1395].resize(___732[___1395] + 1); FaceNeighborsOrMapWritten[___1395].resize(___732[___1395] + 1); NumConnectivityNodes[___1395].resize(___732[___1395] + 1); NumConnectivityNodesWritten[___1395].resize(___732[___1395] + 1); ConnectivityWritten[___1395].resize(___732[___1395] + 1); } catch (std::bad_alloc const&) {
 #if defined MAKEARCHIVE
___3183("Err: (%s) Memory allocation error.\n", routineName);
 #endif
NumErrs[___1395]++; return (-1); } ___4690[___1395] = *___4596; ___1904[___1395] = *___1908; ___2112[___1395] = *___2115; ___2157[___1395] = *___2160; FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___1303; NumConnectivityNodesWritten[___1395][___732[___1395]] = 0; ConnectivityWritten[___1395][___732[___1395]] = ___1303; if (___4690[___1395] == ZoneType_FEMixed) {
 #if defined MAKEARCHIVE
___3183("Err: (%s) FE Mixed zone type not yet supported.\n", routineName);
 #endif
NumErrs[___1395]++; return (-1); } else if (___4690[___1395] == ___4695 || ___4690[___1395] == ___4696) { zoneUses64BitFaceCounts = (*___2803 > InternalPolyFace32BitLimit || *___2160 > InternalPolyFace32BitLimit); PolyZoneWriteInfos[___1395][___732[___1395]].zoneUses64BitFaceCounts = zoneUses64BitFaceCounts; if (zoneUses64BitFaceCounts) fileContains64BitFaceCounts[___1395] = ___4224; ___2801[___1395][___732[___1395]] = 0; ___1283[___1395]   = 0; NumConnectivityNodes[___1395][___732[___1395]] = 0; ___2004[___1395]                             = ___4224; ___4191[___1395][___732[___1395]] = *___2803; ___4189[___1395]              = *___2797; ___4188[___1395]              = *___2796; if (*___3548) { if (*___2797 > 0 || *___2796 > 0) {
 #if defined MAKEARCHIVE
___3183("Err: (%s) Cannot share poly zone connectivity if there are connected boundary faces.\n", routineName);
 #endif
NumErrs[___1395]++; return (-1); } else { FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___4224; } } } else { ___2004[___1395]                              = (*___2003 != 0); ___2801[___1395][___732[___1395]] = *___2799; ___1283[___1395]                     = *___1438; ___4191[___1395][___732[___1395]] = 0; ___4189[___1395]              = 0; ___4188[___1395]              = 0; } if ( zoneUses64BitFaceCounts) ___4490(HeadFile[___1395], ZoneMarkerFace64Bit, FieldDataType_Float); else ___4490(HeadFile[___1395], ZoneMarkerFull32Bit, FieldDataType_Float); if (!___1129(HeadFile[___1395], ___4595, ___4224)) { WriteErr(___1395, routineName); return (-1); } if (___3550) { for (int32_t ___4333 = 0; ___4333 < NumVars[___1395]; ++___4333) { if (___3550[___4333] && ___3550[___4333] >= ___732[___1395] + 1) {
 #if defined MAKEARCHIVE
if (___732[___1395] == 0) ___3183("Err: (%s) The first zone cannot share variables because there is no zone to share from.\n", routineName); else if (___3550[___4333] == ___732[___1395] + 1) ___3183("Err: (%s) A zone cannot share variables with itself.\n", routineName); else ___3183("Err: (%s) A zone can only share variables with previous zones.\n", routineName); PRINT4("     Zone %d in file %d tried to share variable %d with zone %d.\n", ___732[___1395] + 1, ___1395 + 1, ___4333 + 1, ___3550[___4333]);
 #endif
NumErrs[___1395]++; return (-1); } } } if (*___3548 && *___3548 >= ___732[___1395] + 1) {
 #if defined MAKEARCHIVE
if (___732[___1395] == 0) ___3183("Err: (%s) The first zone cannot share connectivity because there is no zone to share from.\n", routineName); else if (*___3548 == ___732[___1395] + 1) ___3183("Err: (%s) A zone cannot share connectivity with itself.\n", routineName); else ___3183("Err: (%s) A zone can only share connectivity with previous zones.\n", routineName); ___3185("     Zone %d in file %d tried to share connectivity with zone %d.\n", ___732[___1395] + 1, ___1395 + 1, *___3548);
 #endif
NumErrs[___1395]++; return (-1); } switch (___4690[___1395]) { case ___2880: NumIndices[___1395] = 0; break; case ___1338: NumIndices[___1395] = 2; break; case ___1346: NumIndices[___1395] = 3; break; case ___1341: NumIndices[___1395] = 4; break; case ___1345: NumIndices[___1395] = 4; break; case FEBRICK: NumIndices[___1395] = 8; break; } if (___4690[___1395] != ___4695    && ___4690[___1395] != ___4696 && *___3548 == 0            && FileTypes[___1395] != ___3636) { NumConnectivityNodes[___1395][___732[___1395]] = NumIndices[___1395] * static_cast<int64_t>(___2112[___1395]); } ___4487(HeadFile[___1395], (___2225)(-1)); ___4487(HeadFile[___1395], (___2225)(*___3784) - 1); ___4490(HeadFile[___1395], *___3639, FieldDataType_Double); ___4487(HeadFile[___1395], (___2225) - 1); ___4487(HeadFile[___1395], ___4690[___1395]); NumDataValuesToWrite[___1395] = 0; for (___1830 = 0; ___1830 < NumVars[___1395]; ___1830++) { IsSharedVar[___1395][___1830]  = (___3550 != NULL && ___3550[___1830] != 0); IsPassiveVar[___1395][___1830] = (___2981   != NULL && ___2981[___1830]   == 1); } ___2225 NumNodes; ___2225 NumCells; if (___4690[___1395] == ___2880) { NumNodes = ___1904[___1395] * ___2112[___1395] * ___2157[___1395]; NumCells = (MAX(___1904[___1395] - 1, 1) * MAX(___2112[___1395] - 1, 1) * MAX(___2157[___1395] - 1, 1)); } else { NumNodes = ___1904[___1395]; NumCells = ___2112[___1395]; } ___372 writeLocationArray = ___1303; for (___1830 = 0; ___1830 < NumVars[___1395]; ___1830++) { ___2225 numValues = NumNodes; IsCellCentered[___1395][___1830] = ___1303; if (___4324 != NULL && ___4324[___1830] != ___4327) { IsCellCentered[___1395][___1830] = ___4224; numValues = NumCells; writeLocationArray = ___4224; if (___4324[___1830] != ___4325) {
 #if defined MAKEARCHIVE
___3185("Err: (%s) Bad zone value location for file %d, variable %d.\n", routineName, ___1395 + 1, ___1830 + 1);
 #endif
NumErrs[___1395]++; return(-1); } } if (___1830 == 0) NumRunningVarValues[___1395][___1830] = 0; else NumRunningVarValues[___1395][___1830] = NumRunningVarValues[___1395][___1830 - 1]; if (!IsSharedVar[___1395][___1830] && !IsPassiveVar[___1395][___1830]) { NumDataValuesToWrite[___1395] += numValues; NumRunningVarValues[___1395][___1830] += numValues; } } ___4487(HeadFile[___1395], int32_t(writeLocationArray)); if (writeLocationArray) { for (___1830 = 0; ___1830 < NumVars[___1395]; ___1830++) ___4487(HeadFile[___1395], int32_t(IsCellCentered[___1395][___1830])); } ___4487(HeadFile[___1395], 0); ___4487(HeadFile[___1395], ___2801[___1395][___732[___1395]]); if (___2801[___1395][___732[___1395]] > 0) { ___4487(HeadFile[___1395], ___1283[___1395]); if (___4690[___1395] != ___2880) ___4487(HeadFile[___1395], 0); } ___4487(HeadFile[___1395], ___1904[___1395]); if (___4690[___1395] == ___1339 || ___4690[___1395] == ___1340) { if (zoneUses64BitFaceCounts) { ___476(fileContains64BitFaceCounts[___1395]); WriteBinaryInt64(HeadFile[___1395], ___2157[___1395]); WriteBinaryInt64(HeadFile[___1395], ___4191[___1395][___732[___1395]]); } else { ___476(___2157[___1395] <= ___2180); ___476(___4191[___1395][___732[___1395]] <= ___2180); ___4487(HeadFile[___1395], static_cast<int32_t>(___2157[___1395])); ___4487(HeadFile[___1395], static_cast<int32_t>(___4191[___1395][___732[___1395]])); } ___476(TecplotSDKBinaryFileVersion == 191); if (___4189[___1395] > 0) { if (___4188[___1395] < ___4189[___1395]) {
 #if defined MAKEARCHIVE
___3184("Err: (%s) There must be at least 1 boundary connection for each boundary face in zone %d.\n", routineName, ___732[___1395] + 1); ___3184("     %lld boundary faces and %lld boundary connections were specified.\n", (long long)___4189[___1395], (long long)___4188[___1395]);
 #endif
NumErrs[___1395]++; return(-1); } ___4487(HeadFile[___1395], ___4189[___1395] + 1); } else ___4487(HeadFile[___1395], ___4189[___1395]); ___4487(HeadFile[___1395], ___4188[___1395]); } ___4487(HeadFile[___1395], ___2112[___1395]); if (___4690[___1395] == ___2880) { if (___2157[___1395] <= ___2180) ___4487(HeadFile[___1395], static_cast<int32_t>(___2157[___1395])); else {
 #if defined MAKEARCHIVE
___3185("Err: (%s) KMax out of range for PLT files (File=%lld,KMax=%lld).\n", routineName, (long long)___1395, (long long)___2157[___1395]);
 #endif
return -1; } } else { ___4487(HeadFile[___1395], 0); ___4487(HeadFile[___1395], 0); ___4487(HeadFile[___1395], 0); } ___4487(HeadFile[___1395], (___2225)0); if (zoneUses64BitFaceCounts) ___4490(BlckFile[___1395], ZoneMarkerFace64Bit, FieldDataType_Float); else ___4490(BlckFile[___1395], ZoneMarkerFull32Bit, FieldDataType_Float); ___476(TecplotSDKBinaryFileVersion == 191); for (___1830 = 0; ___1830 < NumVars[___1395]; ___1830++) { if (!___4510(BlckFile[___1395], (FieldDataType_e)___1362[___1395], ___4224)) { WriteErr(___1395, routineName); return (-1); } } { ___372 hasPassiveVars = ___1303; if (___2981) { for (int32_t ___4333 = 0; ___4333 < NumVars[___1395]; ++___4333) { if (___2981[___4333] == 1) { hasPassiveVars = ___4224; break; } } } if (hasPassiveVars) { ___4487(BlckFile[___1395], 1); for (int32_t ___4333 = 0; ___4333 < NumVars[___1395]; ++___4333) ___4487(BlckFile[___1395], ___2981[___4333]); } else { ___4487(BlckFile[___1395], 0); } } { ___372 hasVariableSharing = ___1303; if (___3550) { for (int32_t ___4333 = 0; ___4333 < NumVars[___1395]; ++___4333) if (___3550[___4333] != 0) { hasVariableSharing = ___4224; break; } } if (hasVariableSharing) { ___4487(BlckFile[___1395], 1); for (int32_t ___4333 = 0; ___4333 < NumVars[___1395]; ++___4333) ___4487(BlckFile[___1395], ___3550[___4333] - 1); } else { ___4487(BlckFile[___1395], 0); } } ___4487(BlckFile[___1395], *___3548 - 1); ___716[___1395] = -1; AdvanceToNextVarWithValues(___1395); MinMaxOffset[___1395][___732[___1395]] = (___1396)___4199(BlckFile[___1395]->File); for (___1830 = 0; ___1830 < NumVars[___1395]; ___1830++) { VarMinValue[___1395][___1830] = ___2177; VarMaxValue[___1395][___1830] = -___2177; if (!IsSharedVar[___1395][___1830] && !IsPassiveVar[___1395][___1830]) { ___4490(BlckFile[___1395], 0.0, FieldDataType_Double); ___4490(BlckFile[___1395], 0.0, FieldDataType_Double); } }
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) { ___3183("Writing Zone %d:\n", ___732[___1395] + 1); ___3183("      Title = %s\n", ___4595); ___3183("       Type = %s\n", ZoneTypes[___4690[___1395]]); if (___4690[___1395] == ___2880) { ___3183("       IMax = %lld\n", (long long)___1904[___1395]); ___3183("       JMax = %lld\n", (long long)___2112[___1395]); ___3183("       KMax = %lld\n", (long long)___2157[___1395]); } else { ___3183("      Nodes = %lld\n", (long long)___1904[___1395]); ___3183("      Cells = %lld\n", (long long)___2112[___1395]); if (___4690[___1395] == ___1339 || ___4690[___1395] == ___1340) { ___3183("      Faces = %lld\n", (long long)___2157[___1395]); ___3183(" Face Nodes = %lld\n", (long long)___4191[___1395][___732[___1395]]); } } if (___3550) { char ___1132[1024] = ""; for (___1830 = 0; ___1830 < NumVars[___1395]; ___1830++) { if (___1830 > 0) strcat(___1132, ","); sprintf(&___1132[strlen(___1132)], "%d", ___3550[___1830]); } ___3183("    DupList = %s\n", ___1132); } }
 #endif
return (0); } namespace { void RewritePendingMinMaxValues(int32_t ___1395) { ___1396 CurrentOffset = (___1396)___4199(BlckFile[___1395]->File); ___4198(BlckFile[___1395]->File, MinMaxOffset[___1395][___732[___1395]], SEEK_SET); for (int ___1830 = 0; ___1830 < NumVars[___1395]; ___1830++) { if (!IsSharedVar[___1395][___1830] && !IsPassiveVar[___1395][___1830]) { ___4490(BlckFile[___1395], VarMinValue[___1395][___1830], FieldDataType_Double); ___4490(BlckFile[___1395], VarMaxValue[___1395][___1830], FieldDataType_Double); } } ___4198(BlckFile[___1395]->File, CurrentOffset, SEEK_SET); } } int32_t ___3970( int32_t        ___1395, int32_t const* N, void const*     ___814, int32_t const* ___2012) { ___2225  ___1830; double    *dptr = (double *)___814; float     *fptr = (float *)___814; if (CheckFile(___1395, "TECDAT142") < 0) return (-1);
 #if defined MAKEARCHIVE
if (DebugLevel[___1395] && (*N > 1)) ___3184("Writing %d values to file %d.\n", *N, ___1395 + 1);
 #endif
for (___1830 = 0; ___1830 < *N; ___1830++) { double ___4312 = (*___2012 == 1 ? dptr[___1830] : fptr[___1830]); if (___4312 < VarMinValue[___1395][___716[___1395]]) VarMinValue[___1395][___716[___1395]] = ___4312; if (___4312 > VarMaxValue[___1395][___716[___1395]]) VarMaxValue[___1395][___716[___1395]] = ___4312; if (!___4490(BlckFile[___1395], ___4312, (FieldDataType_e)___1362[___1395])) { WriteErr(___1395, "TECDAT142"); return (-1); } if (IsCellCentered[___1395][___716[___1395]] && ___4690[___1395] == ___2880) { ___476(___2004[___1395]); ___2225 PIndex = (NumOrderedCCDataValuesWritten[___1395]); ___2225 FinalIMax = MAX(___1904[___1395] - 1, 1); ___2225 FinalJMax = MAX(___2112[___1395] - 1, 1); ___2225 FinalKMax = MAX(___2157[___1395] - 1, 1); ___2225 IIndex = (PIndex % ___1904[___1395]); ___2225 JIndex = ((PIndex % (___1904[___1395] * ___2112[___1395])) / ___1904[___1395]); ___2225 KIndex = (PIndex / (___1904[___1395] * ___2112[___1395])); ___2225 IMaxAdjust = 0; ___2225 JMaxAdjust = 0; ___2225 KMaxAdjust = 0; if (___2157[___1395] > 1) KMaxAdjust = 1; else if (___2112[___1395] > 1) JMaxAdjust = 1; else if (___1904[___1395] > 1) IMaxAdjust = 1; if (IIndex + 1 == FinalIMax && FinalIMax < ___1904[___1395] - IMaxAdjust) { NumOrderedCCDataValuesWritten[___1395]++; if (!___4490(BlckFile[___1395], 0.0, (FieldDataType_e)___1362[___1395])) { WriteErr(___1395, "TECDAT142"); return (-1); } } if (IIndex + 1 == FinalIMax && (JIndex + 1 == FinalJMax && FinalJMax < ___2112[___1395] - JMaxAdjust)) { ___2225 II; for (II = 1; II <= ___1904[___1395] - IMaxAdjust; II++) { NumOrderedCCDataValuesWritten[___1395]++; if (!___4490(BlckFile[___1395], 0.0, (FieldDataType_e)___1362[___1395])) { WriteErr(___1395, "TECDAT142"); return (-1); } } } if (IIndex + 1 == FinalIMax && JIndex + 1 == FinalJMax && (KIndex + 1 == FinalKMax && FinalKMax < ___2157[___1395] - KMaxAdjust)) { ___2225 JJ, II; for (JJ = 1; JJ <= ___2112[___1395] - JMaxAdjust; JJ++) for (II = 1; II <= ___1904[___1395] - IMaxAdjust; II++) { NumOrderedCCDataValuesWritten[___1395]++; if (!___4490(BlckFile[___1395], 0.0, (FieldDataType_e)___1362[___1395])) { WriteErr(___1395, "TECDAT142"); return (-1); } } } NumOrderedCCDataValuesWritten[___1395]++; } NumDataValuesWritten[___1395]++; if (___2004[___1395]) { if (NumRunningVarValues[___1395][___716[___1395]] == NumDataValuesWritten[___1395]) { AdvanceToNextVarWithValues(___1395); if (___716[___1395] < NumVars[___1395]       && IsCellCentered[___1395][___716[___1395]] && ___4690[___1395] == ___2880) NumOrderedCCDataValuesWritten[___1395] = 0; } } else { AdvanceToNextVarWithValues(___1395); if (___716[___1395] >= NumVars[___1395]) { ___716[___1395] = -1; AdvanceToNextVarWithValues(___1395); } }
 #if defined MAKEARCHIVE
if (DebugLevel[___1395] > 1) ___3184("%lld %G\n", (long long)NumDataValuesWritten[___1395], ___4312);
 #endif
} if (HasFECONNECT[___1395] && (NumDataValuesToWrite[___1395] == NumDataValuesWritten[___1395])) { if (!___4487(BlckFile[___1395], (___2225)1)) { WriteErr(___1395, "TECDAT142"); return (-1); } } if (NumDataValuesToWrite[___1395] == NumDataValuesWritten[___1395]) RewritePendingMinMaxValues(___1395); return (0); } int32_t ___3977( int32_t        ___1395, int32_t const* ___2687) { ___2225 ___2163 = NumConnectivityNodes[___1395][___732[___1395]]; ConnectivityWritten[___1395][___732[___1395]] = ___4224; if (CheckFile(___1395, "TECNOD142") < 0) return (-1); if (___4690[___1395] == ___1339 || ___4690[___1395] == ___1340) {
 #if defined MAKEARCHIVE
___3182("Err: (TECNOD142) Cannot call TECNOD142 for polygonal or polyhedral zones.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (HasFECONNECT[___1395]) { return (-1); } if (FileTypes[___1395] == ___3636) {
 #if defined MAKEARCHIVE
___3182("Err: (TECNOD142) Cannot call TECNOD142 if file type is SOLUTIONFILE.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (___4690[___1395] == ___2880) {
 #if defined MAKEARCHIVE
___3182("Err: (TECNOD142) Cannot call TECNOD142 if zone type is ORDERED.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (CheckData(___1395, "TECNOD142") < 0) return (-1); for (___2225 ___1830 = 0; ___1830 < ___2163; ___1830++) { if ((___2687[___1830] > ___1904[___1395]) || (___2687[___1830] < 1)) {
 #if defined MAKEARCHIVE
___3183("Err: (TECNOD142) Invalid node map value at position %lld:\n", (long long)___1830); ___3184("     node map value = %lld, max value = %lld.\n", (long long)___2687[___1830], (long long)___1904[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } if (!___4487(BlckFile[___1395], ___2687[___1830] - 1)) { WriteErr(___1395, "TECNOD142"); return (-1); } } return (0); } int32_t ___3978( int32_t        ___1395, int32_t const* N, int32_t const* ___2687) { if (CheckFile(___1395, "TECNODE142") < 0) return (-1); if (___4690[___1395] == ___1339 || ___4690[___1395] == ___1340) {
 #if defined MAKEARCHIVE
___3182("Err: (TECNODE142) Cannot call TECNODE142 for polygonal or polyhedral zones.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (HasFECONNECT[___1395]) { return (-1); } if (FileTypes[___1395] == ___3636) {
 #if defined MAKEARCHIVE
___3182("Err: (TECNODE142) Cannot call TECNODE142 if file type is SOLUTIONFILE.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (___4690[___1395] == ___2880) {
 #if defined MAKEARCHIVE
___3182("Err: (TECNODE142) Cannot call TECNODE142 if zone type is ORDERED.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (CheckData(___1395, "TECNODE142") < 0) return (-1); if (NumConnectivityNodesWritten[___1395][___732[___1395]] + *N > NumConnectivityNodes[___1395][___732[___1395]]) {
 #if defined MAKEARCHIVE
___3182("Err: (TECNODE142) Connectivity Nodes chunk exceeds the total number of  Connectivity Nodes:\n"); ___3184("     Nodes written = %lld, Current chunk size = %lld, ", (long long)NumConnectivityNodesWritten[___1395][___732[___1395]], (long long)*N); ___3183("total connectivity nodes = %lld.\n", (long long)NumConnectivityNodes[___1395][___732[___1395]]);
 #endif
NumErrs[___1395]++; return (-1); } for (___2225 ___1830 = 0; ___1830 < *N; ___1830++) { if ((___2687[___1830] > ___1904[___1395]) || (___2687[___1830] < 1)) {
 #if defined MAKEARCHIVE
___3183("Err: (TECNODE142) Invalid node map value at position %lld:\n", (long long)___1830); ___3184("     node map value = %lld, max value = %lld.\n", (long long)___2687[___1830], (long long)___1904[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } if (!___4487(BlckFile[___1395], ___2687[___1830] - 1)) { WriteErr(___1395, "TECNODE142"); return (-1); } NumConnectivityNodesWritten[___1395][___732[___1395]]++; } if (NumConnectivityNodesWritten[___1395][___732[___1395]] == NumConnectivityNodes[___1395][___732[___1395]]) { ConnectivityWritten[___1395][___732[___1395]] = ___4224; } return (0); } int32_t ___3971(int32_t ___1395) { int RetVal = 0; if (CheckFile(___1395, "TECEND142") < 0) RetVal = -1; if (RetVal == 0) { if (FileTypes[___1395] != ___3636) { for (int ZoneIndex = 0; (RetVal == 0) && (ZoneIndex <= ___732[___1395]); ZoneIndex++) { if (((NumConnectivityNodes[___1395][ZoneIndex] > 0) && (ConnectivityWritten[___1395][ZoneIndex] == ___1303))) {
 #if defined MAKEARCHIVE
___3183("Err: (TECEND142) File %d is being closed without writing connectivity data.\n", ___1395 + 1); if (NumConnectivityNodesWritten[___1395][ZoneIndex] == 0) ___3183("     Zone %d was defined with a Classic FE zone type but TECNOD142() was not called.\n", ZoneIndex + 1); else ___3183("     Zone %d was defined with a Classic FE zone type but TECNODE142() was not called for all node chunks.\n", ZoneIndex + 1);
 #endif
NumErrs[___1395]++; RetVal = -1; } if (((___2801[___1395][ZoneIndex] > 0) && (FaceNeighborsOrMapWritten[___1395][ZoneIndex] == ___1303))) {
 #if defined MAKEARCHIVE
___3183("Err: (TECEND142) File %d is being closed without writing face neighbor data.\n", ___1395 + 1); ___3184("     %d connections were specified for zone %d but TECFACE142() was not called.\n", ___2801[___1395][ZoneIndex], ZoneIndex + 1);
 #endif
NumErrs[___1395]++; RetVal = -1; } else if (((___4191[___1395][ZoneIndex] > 0) && (FaceNeighborsOrMapWritten[___1395][ZoneIndex] == ___1303))) {
 #if defined MAKEARCHIVE
___3183("Err: (TECEND142) File %d is being closed without writing face map data.\n", ___1395 + 1); ___3184("     %lld face nodes were specified for zone %d but TECPOLYFACE142() and/or\n" "     TECPOLYBCONN142() was not called for all face chunks.\n", (long long)___4191[___1395][ZoneIndex], ZoneIndex + 1);
 #endif
NumErrs[___1395]++; RetVal = -1; } } } if (RetVal == 0) { if (CheckData(___1395, "TECEND142") < 0) RetVal = -1; } if (RetVal == 0) if (!___4490(HeadFile[___1395], EndHeaderMarker, FieldDataType_Float)) { WriteErr(___1395, "TECEND142"); RetVal = -1; } CloseFileStream(&BlckFile[___1395]); if (RetVal == 0) { BlckFile[___1395] = OpenFileStream(BlckFName[___1395], "rb", IsWritingNative[___1395]); char ___416[BYTES_PER_CHUNK]; size_t totalBytesCopied = 0; while ((RetVal == 0) && (feof(BlckFile[___1395]->File) == 0)) { size_t const bytesRead = fread((void*)___416, 1, BYTES_PER_CHUNK, BlckFile[___1395]->File); int const readError = ferror(BlckFile[___1395]->File); if (readError == 0) { size_t const bytesWritten = fwrite((void*)___416, 1, bytesRead, HeadFile[___1395]->File); if (bytesWritten != bytesRead) {
 #if defined MAKEARCHIVE
int const writeError = ferror(HeadFile[___1395]->File); ___3184("Err: (TECEND142) write failure during repack on file %d. Write error = %d.\n", ___1395 + 1, writeError); ___3185("     Wrote %llu bytes of %llu bytes requested (after a total of %llu bytes previously copied).\n", (unsigned long long)bytesWritten, (unsigned long long)bytesRead, (unsigned long long)totalBytesCopied);
 #endif
NumErrs[___1395]++; RetVal = -1; } } else {
 #if defined MAKEARCHIVE
___3184("Err: (TECEND142) Read failure during repack on file %d. Read error = %d.\n", ___1395 + 1, readError); ___3185("     Read %llu bytes of %llu bytes requested (after a total of %llu bytes previously copied).\n", (unsigned long long)bytesRead, (unsigned long long)BYTES_PER_CHUNK, (unsigned long long)totalBytesCopied);
 #endif
NumErrs[___1395]++; RetVal = -1; } totalBytesCopied += bytesRead; } CloseFileStream(&BlckFile[___1395]); } ___4204(BlckFName[___1395]); if (RetVal == 0 && !fileContains64BitFaceCounts[___1395]) { ___4194(HeadFile[___1395]->File); if (___4198(HeadFile[___1395]->File, 0, SEEK_SET) != 0) { WriteErr(___1395, "TECEND142"); RetVal = -1; } else { ___476(TecplotSDKBinaryFileVersion == 191); if (!___4491(*HeadFile[___1395], 112)) { WriteErr(___1395, "TECEND142"); RetVal = -1; } } } CloseFileStream(&HeadFile[___1395]);
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) { ___3183("File %d closed.\n", ___1395 + 1); if (NumErrs[___1395]) { ___3182("********************************************\n"); ___3183("      %d Errors occurred on this file\n", NumErrs[___1395]); ___3182("********************************************\n"); } }
 #endif
NumErrs[___1395] = 0; ___2040[___1395] = 0; if (DestFName[___1395]) ___1528(DestFName[___1395], "data set fname"); if (BlckFName[___1395]) ___1528(BlckFName[___1395], "data set fname"); BlckFName[___1395] = NULL; DestFName[___1395] = NULL; VarMinValue[___1395].clear(); VarMaxValue[___1395].clear(); NumRunningVarValues[___1395].clear(); IsSharedVar[___1395].clear(); IsPassiveVar[___1395].clear(); IsCellCentered[___1395].clear(); MinMaxOffset[___1395].clear(); ___4191[___1395].clear(); PolyZoneWriteInfos[___1395].clear(); ___2801[___1395].clear(); FaceNeighborsOrMapWritten[___1395].clear(); NumConnectivityNodes[___1395].clear(); NumConnectivityNodesWritten[___1395].clear(); ConnectivityWritten[___1395].clear(); } return RetVal; } namespace { void GetNextLabel( char const** ___683, char*        NextLabel) { int N = 0; char *NPtr = NextLabel; *NPtr = '\0'; while ((**___683) && (**___683 !='"')) (*___683)++; if (**___683) (*___683)++; while ((N < 60) && (**___683) && (**___683 != '"')) { if (**___683 == '\\') { (*___683)++; } *NPtr++ = **___683; N++; (*___683)++; } if (**___683) (*___683)++; *NPtr = '\0'; } } int32_t ___3976( int32_t    ___1395, char const* S) { char const* ___683 = S; int32_t     N = 0; char        Label[60]; if (CheckFile(___1395, "TECLAB142") < 0) return (-1);
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) ___3182("\nInserting Custom Labels:\n");
 #endif
do { GetNextLabel(&___683, Label); if (*Label) N++; } while (*Label); if (N == 0) {
 #if defined MAKEARCHIVE
___3183("Err: (TECLAB142) Invalid custom label string: %s\n", (S ? S : " "));
 #endif
NumErrs[___1395]++; return (-1); } ___4490(HeadFile[___1395], ___790, FieldDataType_Float); if (!___4487(HeadFile[___1395], N)) { WriteErr(___1395, "TECLAB142"); return (-1); } ___683 = S; do { GetNextLabel(&___683, Label); if (*Label) { if (!___1129(HeadFile[___1395], Label, ___4224)) { WriteErr(___1395, "TECLAB142"); return (-1); }
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) printf("          %s\n", Label);
 #endif
} } while (*Label); return (0); } int32_t ___3987( int32_t    ___1395, char const* S) { if (CheckFile(___1395, "TECUSR142") < 0) return (-1);
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) ___3183("\nInserting UserRec: %s\n", S);
 #endif
if ((S == NULL) || (*S == '\0')) {
 #if defined MAKEARCHIVE
___3182("Err: (TECUSR142) Invalid TECUSR142 string\n");
 #endif
NumErrs[___1395]++; return (-1); } ___4490(HeadFile[___1395], ___4284, FieldDataType_Float); if (!___1129(HeadFile[___1395], S, ___4224)) {
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) printf("Err: (TECUSR142) Write failure for file %d\n", ___1395 + 1);
 #endif
NumErrs[___1395]++; return (-1); } return (0); } int32_t ___3974( int32_t        ___1395, double const*   ___4572, double const*   ___4589, double const*   ___4713, int32_t const* ___3158, int32_t const* ___227, int32_t const* ___4597, int32_t const* Color, int32_t const* ___1410, int32_t const* ___2021, int32_t const* ___1650, int32_t const* ___2262, double const*   ___2985, double const*   ___2288, int32_t const* ___2792, int32_t const* ___188, int32_t const* ___176, double const*   ___187, double const*   ___171, int32_t const* ___3441, int32_t const* ___494, int32_t const* ___2834, int32_t const* ___2836, float const*    ___4570, float const*    ___4587, float const*    ___4594, char const*     mfc) { int    ___1830, RetVal; int    RawDataSize = 0; double Fract; ___1630 ___1554; if (CheckFile(___1395, "TECGEO142") < 0) return (-1); ___1554.___3165 = (CoordSys_e) * ___3158; if (___1554.___3165 == CoordSys_Frame) Fract = 0.01; else Fract = 1.0; ___1554.position.setXOrTheta((*___4572) * Fract); ___1554.position.setYOrR((*___4589) * Fract); ___1554.position.setZ((*___4713) * Fract); ___1554.___227           = *___227 != 0; ___1554.___4597                   = *___4597 - 1; ___1554.___351                 = (___514) * Color; ___1554.___1408             = (___514) * ___1410; ___1554.___2021               = *___2021 != 0; ___1554.___1650               = (GeomType_e) * ___1650; ___1554.___2262            = (LinePattern_e) * ___2262; ___1554.___2985          = *___2985 / 100.0; ___1554.___2288          = *___2288 / 100.0; ___1554.___2792          = (int32_t)*___2792; ___1554.___188         = (ArrowheadStyle_e) * ___188; ___1554.___176    = (ArrowheadAttachment_e) * ___176; ___1554.___187          = *___187 / 100.0; ___1554.___171         = *___171 / ___952; ___1554.___3441                  = (Scope_e) * ___3441; ___1554.___1111              = ___1112; ___1554.___494               = (Clipping_e) * ___494; ___1554.___2834            = (int32_t)*___2834; ___1554.___2329   = const_cast<char*>(mfc); ___1554.___1882          = NULL; ___1554.WorldFileName          = NULL; ___1554.EmbeddedLpkImageNumber = 0; ___1554.___2331    = ___4224; ___1554.___3087       = 1.0; ___1554.___1888      = ___1898; if (___1554.___1650 == GeomType_LineSegs3D) { ___1554.___1650         = GeomType_LineSegs; ___1554.___3165 = CoordSys_Grid3D; }
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) ___3182("\nInserting Geometry\n");
 #endif
switch (___1554.___1650) { case GeomType_LineSegs : { RawDataSize = 0; for (int i = 0; i < *___2834; i++) { ___1554.___2836[i] = ___2836[i]; RawDataSize += ___2836[i]; } } break; case GeomType_Rectangle : case GeomType_Square : case GeomType_Circle : case GeomType_Ellipse : { RawDataSize = 1; } break; case GeomType_Image : { ___476(___1303); } break; default : { ___476(___1303); } break; } ___1554.___905                = FieldDataType_Float; ___1554.___1571.___1546.___4291 = ___28(RawDataSize, FieldDataType_Float, ___4224); ___1554.___1571.___1546.___4293 = ___28(RawDataSize, FieldDataType_Float, ___4224); ___1554.___1571.___1546.___4295 = ___28(RawDataSize, FieldDataType_Float, ___4224); for (___1830 = 0; ___1830 < RawDataSize; ___1830++) { ___3488(___1554.___1571.___1546.___4291, ___1830, (double)___4570[___1830]*Fract); ___3488(___1554.___1571.___1546.___4293, ___1830, (double)___4587[___1830]*Fract); ___3488(___1554.___1571.___1546.___4295, ___1830, (double)___4594[___1830]*Fract); } if (___1130(HeadFile[___1395], &___1554, ___4224, ___1303)) RetVal = 0; else RetVal = -1; ___936(&___1554.___1571.___1546.___4291); ___936(&___1554.___1571.___1546.___4293); ___936(&___1554.___1571.___1546.___4295); return RetVal; } int32_t ___3986( int32_t        ___1395, double const*   ___4572, double const*   ___4589, double const*   ___4711, int32_t const* ___3158, int32_t const* ___227, int32_t const* ___4597, int32_t const* ___353, int32_t const* ___1451, double const*   ___1449, int32_t const* ___411, double const*   ___409, double const*   ___407, int32_t const* ___403, int32_t const* ___405, double const*   ___57, int32_t const* ___39, double const*   ___2286, int32_t const* ___4079, int32_t const* ___3441, int32_t const* ___494, char const*     ___3811, char const*     mfc) { int    RetVal; ___4116 Text; double Fract; if (CheckFile(___1395, "TECTXT142") < 0) return (-1); Text.___3165    = (CoordSys_e) * ___3158; if (Text.___3165 == CoordSys_Frame) Fract = 0.01; else Fract = 1.0; Text.___52.___1546.___4290 = (*___4572) * Fract; Text.___52.___1546.___4292 = (*___4589) * Fract; Text.___52.___1546.___4294 = (*___4711) * Fract; Text.___227         = *___227 != 0; Text.___4597                 = *___4597 - 1; Text.___351               = static_cast<___514>(*___4079); Text.___4119.___1442       = static_cast<Font_e>(*___353); Text.___4119.___3599  = static_cast<Units_e>(*___1451); if (Text.___4119.___3599 == ___4266) Text.___4119.___1825 = (*___1449) / 100.0; else Text.___4119.___1825 = *___1449; Text.___401.___411          = static_cast<TextBox_e>(*___411); Text.___401.___2336           = *___409 / 100.0; Text.___401.___2288    = *___407 / 100.0; Text.___401.___351           = static_cast<___514>(*___403); Text.___401.___1408       = static_cast<___514>(*___405);
Text.___39               = static_cast<TextAnchor_e>(*___39); Text.___2286          = *___2286; Text.___57                = *___57 / ___952; Text.___3441                = static_cast<Scope_e>(*___3441); if (___3811) Text.Text = const_cast<char*>(___3811); if (mfc) Text.___2329 = const_cast<char*>(mfc); Text.___494             = static_cast<Clipping_e>(*___494);
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) ___3183("\nInserting Text: %s\n", ___3811);
 #endif
if (___1131(HeadFile[___1395], &Text, ___4224, ___1303)) RetVal = 0; else RetVal = -1; return RetVal; } void ___3973( int32_t        ___1395, int32_t const* ___2889) { ___4276(___1395); REQUIRE(VALID_REF(___2889)); DoWriteForeign = (*___2889 != 0); }
 #if defined MAKEARCHIVE
namespace { ___372 ___250( char      ___470, ___372 ___2028) { REQUIRE(0 <= ___470 && "Char <= 127"); REQUIRE(VALID_BOOLEAN(___2028)); ___372 IsValidNameChar = (___470 == '_' || tecplot::___1996(___470)); if (!___2028) IsValidNameChar = (IsValidNameChar || ___470 == '.'     || tecplot::___2010(___470)); ENSURE(VALID_BOOLEAN(IsValidNameChar)); return IsValidNameChar; } }
 #endif 
 #if defined MAKEARCHIVE
namespace { ___372 ___249(char const* ___2684) { REQUIRE(VALID_REF(___2684)); ___372 IsValidName = ___250(*___2684, ___4224); for (char const* NPtr = ___2684; IsValidName && *NPtr != '\0'; ++NPtr) { IsValidName = ___250(*NPtr, ___1303); } ENSURE(VALID_BOOLEAN(IsValidName)); return IsValidName; } }
 #endif 
int32_t ___3969( int32_t    ___1395, char const* ___2684, char const* ___4312) { if (CheckFile(___1395, "TECAUXSTR142") < 0) return (-1);
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) ___3184("\nInserting data set aux data: '%s' = '%s'\n", ___2684, ___4312);
 #endif
if ((___2684 == NULL) || !___249(___2684)) {
 #if defined MAKEARCHIVE
___3182("Err: (TECAUXSTR142) Invalid Name string\n");
 #endif
NumErrs[___1395]++; return (-1); } if ((___4312 == NULL) || (*___4312 == '\0')) {
 #if defined MAKEARCHIVE
___3182("Err: (TECAUXSTR142) Invalid Value string\n");
 #endif
NumErrs[___1395]++; return (-1); } if (!___4490(HeadFile[___1395], ___885, FieldDataType_Float)  || !___1129(HeadFile[___1395], ___2684, ___4224  ) || !___4487(HeadFile[___1395], (___2225)___270) || !___1129(HeadFile[___1395], ___4312, ___4224  )) {
 #if defined MAKEARCHIVE
printf("Err: (TECAUXSTR142) Write failure for file %d\n", ___1395 + 1);
 #endif
NumErrs[___1395]++; return (-1); } return (0); } int32_t ___3989( int32_t    ___1395, char const* ___2684, char const* ___4312) { if (CheckFile(___1395, "TECZAUXSTR142") < 0) return (-1); if (___732[___1395] == -1) {
 #if defined MAKEARCHIVE
___3182("Err: (TECZAUXSTR142) Must call TECZNE142 or TECPOLYZNE142 prior to TECZAUXSTR142\n");
 #endif
NumErrs[___1395]++; return (-1); }
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) ___3184("\nInserting zone aux data: '%s' = '%s'\n", ___2684, ___4312);
 #endif
if ((___2684 == NULL) || !___249(___2684)) {
 #if defined MAKEARCHIVE
___3182("Err: (TECZAUXSTR142) Invalid Name string\n");
 #endif
NumErrs[___1395]++; return (-1); } if ((___4312 == NULL) || (*___4312 == '\0')) {
 #if defined MAKEARCHIVE
___3182("Err: (TECZAUXSTR142) Invalid Value string\n");
 #endif
NumErrs[___1395]++; return (-1); } if (___4198(HeadFile[___1395]->File, -4, SEEK_CUR) || !___4487(HeadFile[___1395], 1)  || !___1129(HeadFile[___1395], ___2684, ___4224  ) || !___4487(HeadFile[___1395], (___2225)___270) || !___1129(HeadFile[___1395], ___4312, ___4224  ) || !___4487(HeadFile[___1395], 0)) {
 #if defined MAKEARCHIVE
printf("Err: (TECZAUXSTR142) Write failure for file %d\n", ___1395 + 1);
 #endif
NumErrs[___1395]++; return (-1); } return (0); } int32_t ___3988( int32_t        ___1395, int32_t const* ___4334, char const*     ___2684, char const*     ___4312) { if (CheckFile(___1395, "TECVAUXSTR142") < 0) return (-1);
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) ___3184("\nInserting variable aux data: '%s' = '%s'\n", ___2684, ___4312);
 #endif
if ((___2684 == NULL) || !___249(___2684)) {
 #if defined MAKEARCHIVE
___3182("Err: (TECVAUXSTR142) Invalid Name string\n");
 #endif
NumErrs[___1395]++; return (-1); } if ((___4312 == NULL) || (*___4312 == '\0')) {
 #if defined MAKEARCHIVE
___3182("Err: (TECVAUXSTR142) Invalid Value string\n");
 #endif
NumErrs[___1395]++; return (-1); } if (!___4490(HeadFile[___1395], ___4336, FieldDataType_Float)  || !___4487(HeadFile[___1395], *___4334 - 1) || !___1129(HeadFile[___1395], ___2684, ___4224  ) || !___4487(HeadFile[___1395], (___2225)___270) || !___1129(HeadFile[___1395], ___4312, ___4224  )) {
 #if defined MAKEARCHIVE
printf("Err: (TECVAUXSTR142) Write failure for file %d\n", ___1395 + 1);
 #endif
NumErrs[___1395]++; return (-1); } return (0); } int32_t ___3972( int32_t        ___1395, int32_t const* ___1256) { FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___4224; if (CheckFile(___1395, "TECFACE142") < 0) return (-1); if (___4690[___1395] == ___1339 || ___4690[___1395] == ___1340) {
 #if defined MAKEARCHIVE
___3182("Err: (TECFACE142) Cannot call TECFACE142 for polygonal or polyhedral zones.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (FileTypes[___1395] == ___3636) {
 #if defined MAKEARCHIVE
___3182("Err: (TECFACE142) Cannot call TECFACE142 if the file type is SOLUTIONFILE.\n");
 #endif
NumErrs[___1395]++; return (-1); }
 #if defined MAKEARCHIVE
if (DebugLevel[___1395]) ___3182("\nInserting face neighbor data\n");
 #endif
if (___1256 == NULL) {
 #if defined MAKEARCHIVE
___3182("Err: (TECFACE142) Invalid array\n");
 #endif
NumErrs[___1395]++; return (-1); } int32_t const* Ptr = ___1256; int32_t i = 0; while (i < ___2801[___1395][___732[___1395]]) { int32_t n; int32_t NumNum = 0; switch (___1283[___1395]) { case ___1288: NumNum = 3; i++; break; case ___1287: NumNum = 4 + Ptr[3]; i += Ptr[3]; break; case ___1285: NumNum = 4; i++; break; case ___1284: NumNum = 4 + 2 * Ptr[3]; i += Ptr[3]; break; default: ___476(___1303); break; } n = 0; if (___1283[___1395] == ___1287 || ___1283[___1395] == ___1284) { if (!___4487(BlckFile[___1395], Ptr[n++] - 1) || !___4487(BlckFile[___1395], Ptr[n++] - 1) || !___4487(BlckFile[___1395], Ptr[n++])   || !___4487(BlckFile[___1395], Ptr[n++])) {
 #if defined MAKEARCHIVE
printf("Err: (TECFACE142) Write failure for file %d\n", ___1395 + 1);
 #endif
NumErrs[___1395]++; return (-1); } } for (; n < NumNum; n++) if (!___4487(BlckFile[___1395], Ptr[n] - 1)) {
 #if defined MAKEARCHIVE
printf("Err: (TECFACE142) Write failure for file %d\n", ___1395 + 1);
 #endif
NumErrs[___1395]++; return (-1); } Ptr += NumNum; } return (0); } int32_t ___3984(int32_t ___1395) { return ___4188[___1395]; } int32_t ___3985(int32_t ___1395) { return ___4189[___1395]; } namespace { int32_t writePolyhedralFaceMetadata( int32_t            fileNum, int64_t            numFacesToWrite, int32_t const*     faceNodeCounts, ___372          zoneUses64BitFaceCounts, int64_t*           faceNodeSum, char const*        routineName) { REQUIRE(___4690[fileNum] == ___4696); REQUIRE(0 <= fileNum && fileNum < ___2378); REQUIRE(numFacesToWrite > 0); REQUIRE(VALID_REF(faceNodeCounts)); REQUIRE(VALID_BOOLEAN(zoneUses64BitFaceCounts)); REQUIRE(VALID_REF(faceNodeSum) && *faceNodeSum >= 0); REQUIRE(VALID_REF(routineName)); if (!zoneUses64BitFaceCounts && *faceNodeSum == 0) { if (!___4487(BlckFile[fileNum], 0)) { WriteErr(fileNum, routineName); return -1; } } for (int64_t ___1924 = 0; ___1924 < numFacesToWrite; ___1924++) { int32_t const faceNodeCount = faceNodeCounts[___1924]; *faceNodeSum += faceNodeCount; if (faceNodeCounts[___1924] < 3) {
 #if defined MAKEARCHIVE
___3184("Err: (%s) Invalid face node count value at face %lld.  There must be at least 3 nodes in a face.\n", routineName, (long long)___1924 + 1); ___3183("     Face node count value = %lld.\n", (long long)faceNodeCounts[___1924]);
 #endif
NumErrs[fileNum]++; return -1; } else if (*faceNodeSum > ___4191[fileNum][___732[fileNum]]) {
 #if defined MAKEARCHIVE
___3184("Err: (%s) The running face node count exceeds the TotalNumFaceNodes (%lld) specified.\n", routineName, (long long)___4191[fileNum][___732[fileNum]]); ___3183("     Face node count value = %lld.\n", (long long)faceNodeCounts[___1924]);
 #endif
NumErrs[fileNum]++; return -1; } else { int64_t const faceMetadata = (zoneUses64BitFaceCounts ? faceNodeCounts[___1924] : *faceNodeSum); ___476(faceMetadata <= ___2180); if (!___4487(BlckFile[fileNum], static_cast<int32_t>(faceMetadata))) { WriteErr(fileNum, routineName); return -1; } } } return 0; } } namespace { int32_t writePolyhedralFaceMetadataPiece( int32_t            fileNum, PolyZoneWriteInfo& polyZoneWriteInfo, int32_t            numFacesToWrite, int32_t const*     faceNodeCounts, char const*        routineName) { REQUIRE(___4690[fileNum] == ___4696); REQUIRE(0 <= fileNum && fileNum < ___2378); REQUIRE(numFacesToWrite > 0); REQUIRE(VALID_REF(faceNodeCounts)); REQUIRE(VALID_REF(routineName)); if (___4198(BlckFile[fileNum]->File, polyZoneWriteInfo.faceMetadataOffset, SEEK_SET) != 0) { WriteErr(fileNum, routineName); return -1; } if (writePolyhedralFaceMetadata(fileNum, numFacesToWrite, faceNodeCounts, polyZoneWriteInfo.zoneUses64BitFaceCounts, &polyZoneWriteInfo.faceNodeSum, routineName) != 0) return -1; polyZoneWriteInfo.faceMetadataOffset = (___1396)___4199(BlckFile[fileNum]->File); return 0; } } namespace { int32_t writeAllPolyhedralFaceMetadata( int32_t            fileNum, int32_t const*     faceNodeCounts, char const*        routineName) { REQUIRE(___4690[fileNum] == ___4696); REQUIRE(0 <= fileNum && fileNum < ___2378); REQUIRE(VALID_REF(faceNodeCounts)); REQUIRE(VALID_REF(routineName)); int64_t faceNodeSum = 0; if (writePolyhedralFaceMetadata(fileNum, ___2157[fileNum], faceNodeCounts, PolyZoneWriteInfos[fileNum][___732[fileNum]].zoneUses64BitFaceCounts, &faceNodeSum, routineName) != 0) return -1; if (faceNodeSum != ___4191[fileNum][___732[fileNum]]) {
 #if defined MAKEARCHIVE
___3184("Err: (%s) File %d has invalid field map.\n", routineName, fileNum + 1); PRINT4("     %lld face nodes were specified for zone %d but %s() was not called with %lld.\n", (long long)faceNodeSum, ___732[fileNum] + 1, routineName, (long long)___4191[fileNum][___732[fileNum]]);
 #endif
NumErrs[fileNum]++; return -1; } return 0; } } int32_t ___3981( int32_t        ___1395, int32_t const* ___1292, int32_t const* ___1295, int32_t const* ___1257, int32_t const* ___1301, int32_t const* ___1251, int32_t const* ___1252, int32_t const* ___1254) { int64_t ___2804 = ___2157[___1395]; int32_t ___3357 = 0; ___2225 ___1924; ___2225 MinNeighborValue = TECIO_NO_NEIGHBORING_ELEM; FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___4224; if (___2804 == 0 || (___4690[___1395] != ___1339 && ___4690[___1395] != ___1340)) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLY142) The zone type must be FEPOLYGON or FEPOLYHEDRON and have NumFaces (KMax) > 0.\n"); ___3183("     NumFaces = %lld\n", (long long)___2804);
 #endif
NumErrs[___1395]++; return (-1); } if (FileTypes[___1395] == ___3636) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLY142) Cannot call TECPOLY142 if file type is SOLUTIONFILE.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (___4690[___1395] == ___1340) { if (___4191[___1395][___732[___1395]] <= 0) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLY142) TotalNumFaceNodes MUST be specified for polyhedral zones.\n"); ___3183("     TotalNumFaceNodes = %lld\n", (long long)___4191[___1395][___732[___1395]]);
 #endif
NumErrs[___1395]++; return (-1); } } else { if (___4191[___1395][___732[___1395]] != (2 * ___2804)) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLY142) TotalNumFaceNodes is specified for the polygonal zone but is not equal to 2 * NumFaces.\n"); ___3184("     TotalNumFaceNodes = %lld.  If specified, it must be 2 * %lld.", (long long)___4191[___1395][___732[___1395]], (long long)___2804);
 #endif
NumErrs[___1395]++; return (-1); } } if ((___4189[___1395] > 0  && ___4188[___1395] > 0) || (___4189[___1395] == 0 && ___4188[___1395] == 0)) { if (___4189[___1395] > 0) MinNeighborValue = -___4189[___1395]; } else {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLY142) TotalNumFaceBndryFaces and TotalNumFaceBndryConns must both be 0 or both be > 0.\n"); ___3184("     TotalNumFaceBndryFaces = %lld, TotalNumFaceBndryConns = %lld\n", (long long)___4189[___1395], (long long)___4188[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } if (___3357 == 0) { if (___4690[___1395] == ___1340) { if (writeAllPolyhedralFaceMetadata(___1395, ___1292, "TECPOLY142")!=0) { NumErrs[___1395]++; return (-1); } } } for (___1924 = 0; (___3357 == 0) && (___1924 < ___4191[___1395][___732[___1395]]); ___1924++) { if (___1295[___1924] < 1 || ___1295[___1924] > ___1904[___1395]) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Invalid face node value at node %lld:\n", (long long)___1924 + 1); ___3184("     face node value = %lld, valid values are are 1 to %lld (inclusive).\n", (long long)___1295[___1924], (long long)___1904[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], ___1295[___1924] - 1)) ___3357 = -1; } for (___1924 = 0; (___3357 == 0) && (___1924 < ___2804); ___1924++) { if (___1257[___1924] < MinNeighborValue || ___1257[___1924] > ___2112[___1395]) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Invalid left neighbor value at face %lld:\n", (long long)___1924); ___3184("     left neighbor value = %lld, min value = %lld,", (long long)___1257[___1924], (long long)MinNeighborValue); ___3183(" max value = %lld.\n", (long long)___2112[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], ___1257[___1924] - 1)) ___3357 = -1; } for (___1924 = 0; (___3357 == 0) && (___1924 < ___2804); ___1924++) { if (___1301[___1924] < MinNeighborValue || ___1301[___1924] > ___2112[___1395]) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Invalid right neighbor value at face %lld:\n", (long long)___1924); ___3184("     right neighbor value = %lld, min value = %lld,", (long long)___1301[___1924], (long long)MinNeighborValue); ___3183(" max value = %lld.\n", (long long)___2112[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], ___1301[___1924] - 1)) ___3357 = -1; if (___3357 == 0 && (___1257[___1924] == TECIO_NO_NEIGHBORING_ELEM && ___1301[___1924] == TECIO_NO_NEIGHBORING_ELEM)) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Both left and right neighbors are set to no neighboring element at face %lld.\n", (long long)___1924);
 #endif
NumErrs[___1395]++; return (-1); } } if (___3357 == 0 && ___4189[___1395] > 0) { ___476(TecplotSDKBinaryFileVersion == 191); if (!(___4487(BlckFile[___1395], 0) && ___4487(BlckFile[___1395], 0))) ___3357 = -1; int32_t BndryConnCount = 0; for (___1924 = 0; (___3357 == 0) && (___1924 < ___4189[___1395]); ___1924++) { BndryConnCount += ___1251[___1924]; if (___1251[___1924] < 0 || BndryConnCount > ___4188[___1395]) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Invalid boundary connection count at boundary face %lld:\n", (long long)___1924 + 1); ___3183("     boundary connection count = %lld.\n", (long long)___1251[___1924]);
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], BndryConnCount)) ___3357 = -1; } if (BndryConnCount != ___4188[___1395]) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLY142) Invalid number of boundary connections:\n"); ___3184("     number of boundary connections written = %lld, total number of boundary connections = %lld.", (long long)BndryConnCount, (long long)___4188[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } BndryConnCount = 0; for (___1924 = 0; (___3357 == 0) && (___1924 < ___4189[___1395]); ___1924++) { for (___2225 BIndex = 0; (___3357 == 0) && (BIndex < ___1251[___1924]); BIndex++) { if (BIndex > 0 && ___1252[BndryConnCount] == TECIO_NO_NEIGHBORING_ELEM) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Partially obscured faces must specify no neighboring element first. See boundary connections for face %lld.\n", (long long)___1924 + 1);
 #endif
NumErrs[___1395]++; return (-1); } if (___1252[BndryConnCount] < TECIO_NO_NEIGHBORING_ELEM) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Invalid boundary element value at boundary connections for face %lld:\n", (long long)___1924 + 1);
 #endif
NumErrs[___1395]++; return (-1); } if (___1252[BndryConnCount] == TECIO_NO_NEIGHBORING_ELEM && ___1254[BndryConnCount] != TECIO_NO_NEIGHBORING_ZONE) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Invalid boundary element/zone pair at boundary connections for face %lld:\n", (long long)___1924 + 1); ___3182("     Boundary elements specified as no neighboring element must also specify no neighboring zone.\n");
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], ___1252[BndryConnCount] - 1)) ___3357 = -1; BndryConnCount++; } } BndryConnCount = 0; for (___1924 = 0; (___3357 == 0) && (___1924 < ___4189[___1395]); ___1924++) { for (___2225 BIndex = 0; (___3357 == 0) && (BIndex < ___1251[___1924]); BIndex++) { if (___1254[BndryConnCount] < TECIO_NO_NEIGHBORING_ZONE) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Invalid boundary zone value at boundary connections for face %lld:\n", (long long)___1924 + 1);
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], ___1254[BndryConnCount] - 1)) ___3357 = -1; BndryConnCount++; } } } if (___3357 != 0) { ___3357 = -1; WriteErr(___1395, "TECPOLY142"); } return ___3357; } namespace { void calculateInitialPolyZoneOffsets(int32_t ___1395) { PolyZoneWriteInfo& polyZoneWriteInfo = PolyZoneWriteInfos[___1395][___732[___1395]]; polyZoneWriteInfo.faceMetadataOffset = (___1396)___4199(BlckFile[___1395]->File); if (___4690[___1395] == ___1340) { int64_t const numFaceMetadataEntries = (polyZoneWriteInfo.zoneUses64BitFaceCounts) ? ___2157[___1395] : ___2157[___1395] + 1; polyZoneWriteInfo.faceNodesOffset = polyZoneWriteInfo.faceMetadataOffset + sizeof(int32_t) * numFaceMetadataEntries; } else { polyZoneWriteInfo.faceNodesOffset = polyZoneWriteInfo.faceMetadataOffset; } polyZoneWriteInfo.leftElemsOffset = polyZoneWriteInfo.faceNodesOffset + sizeof(int32_t) * ___4191[___1395][___732[___1395]]; polyZoneWriteInfo.rightElemsOffset = polyZoneWriteInfo.leftElemsOffset + sizeof(int32_t) * ___2157[___1395]; polyZoneWriteInfo.connectionCountsOffset = polyZoneWriteInfo.rightElemsOffset + sizeof(int32_t) * ___2157[___1395]; polyZoneWriteInfo.connectionElemsOffset = polyZoneWriteInfo.connectionCountsOffset + sizeof(int32_t) * (___4189[___1395] + 2); polyZoneWriteInfo.connectionZonesOffset = polyZoneWriteInfo.connectionElemsOffset + sizeof(int32_t) * ___4188[___1395]; } } namespace { int32_t checkForPolyFacePreconditions(int32_t ___1395) { if (CheckFile(___1395, "TECPOLYFACE142") < 0) return (-1); if (FaceNeighborsOrMapWritten[___1395][___732[___1395]]) { if (NumErrs[___1395] == 0) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLYFACE142) TECPOLYFACE142 called after all specified faces were already written.\n");
 #endif
NumErrs[___1395]++; } return (-1); } if (___2157[___1395] == 0 || (___4690[___1395] != ___1339 && ___4690[___1395] != ___1340)) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLYFACE142) The zone type must be FEPOLYGON or FEPOLYHEDRON and have KMax (NumFaces) > 0.\n"); ___3183("     KMax = %lld\n", (long long)___2157[___1395]);
 #endif
NumErrs[___1395]++; FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___4224; return (-1); } if (FileTypes[___1395] == ___3636) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLYFACE142) Cannot call TECPOLYFACE142 if file type is SOLUTIONFILE.\n");
 #endif
NumErrs[___1395]++; return (-1); } if (___4690[___1395] == ___1340) { if (___4191[___1395][___732[___1395]] <= 0) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLYFACE142) TotalNumFaceNodes MUST be specified for polyhedral zones.\n"); ___3183("     TotalNumFaceNodes = %lld\n", (long long)___4191[___1395][___732[___1395]]);
 #endif
NumErrs[___1395]++; FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___4224; return (-1); } } else { if (___4191[___1395][___732[___1395]] != (2 * ___2157[___1395])) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLYFACE142) TotalNumFaceNodes is specified for the polygonal zone but is not equal to 2 * KMax.\n"); ___3184("     TotalNumFaceNodes = %lld.  If specified, it must be 2 * %lld.", (long long)___4191[___1395][___732[___1395]], (long long)___2157[___1395]);
 #endif
NumErrs[___1395]++; FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___4224; return (-1); } } return 0; } } namespace { int32_t getMinNeighborValue( int32_t   ___1395, int32_t&   MinNeighborValue) { MinNeighborValue = TECIO_NO_NEIGHBORING_ELEM; if ((___4189[___1395] > 0  && ___4188[___1395] > 0) || (___4189[___1395] == 0 && ___4188[___1395] == 0)) { if (___4189[___1395] > 0) MinNeighborValue = -___4189[___1395]; } else {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLYFACE142) TotalNumFaceBndryFaces and TotalNumFaceBndryConns must both be 0 or both be > 0.\n"); ___3184("     TotalNumFaceBndryFaces = %lld, TotalNumFaceBndryConns = %lld\n", (long long)___4189[___1395], (long long)___4188[___1395]);
 #endif
NumErrs[___1395]++; FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___4224; return -1; } return 0; } } namespace { int32_t writeFaceNodes( int32_t           ___1395, PolyZoneWriteInfo& polyZoneWriteInfo, ___2225          BeginningFaceNodeSum, int32_t           ___2804, int32_t const*    ___1295) { ___4198(BlckFile[___1395]->File, polyZoneWriteInfo.faceNodesOffset, SEEK_SET); ___2225 LocalFaceNodeSum; if (___4690[___1395] == ___1340) LocalFaceNodeSum = polyZoneWriteInfo.faceNodeSum - BeginningFaceNodeSum; else LocalFaceNodeSum = 2 * (___2804); for (___2225 ___1924 = 0; ___1924 < LocalFaceNodeSum; ___1924++) { if (___1295[___1924] < 1 || ___1295[___1924] > ___1904[___1395]) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLY142) Invalid face node value at node %lld:\n", (long long)___1924 + 1); ___3184("     face node value = %lld, valid values are are 1 to %lld (inclusive).\n", (long long)___1295[___1924], (long long)___1904[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], ___1295[___1924] - 1)) { WriteErr(___1395, "TECPOLYFACE142"); return -1; } } polyZoneWriteInfo.faceNodesOffset = (___1396)___4199(BlckFile[___1395]->File); return 0; } } namespace { int32_t checkElements( int32_t        ___1395, int32_t        ___2804, int32_t const* ___1257, int32_t const* ___1301, int32_t        MinNeighborValue) { for (___2225 ___1924 = 0; ___1924 < ___2804; ___1924++) { if (___1257[___1924] < MinNeighborValue || ___1257[___1924] > ___2112[___1395]) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLYFACE142) Invalid left neighbor value at face %lld:\n", (long long)___1924); ___3184("     left neighbor value = %lld, min value = %lld,", (long long)___1257[___1924], (long long)MinNeighborValue); ___3183(" max value = %lld.\n", (long long)___2112[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } else if (___1301[___1924] < MinNeighborValue || ___1301[___1924] > ___2112[___1395]) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLYFACE142) Invalid right neighbor value at face %lld:\n", (long long)___1924); ___3184("     right neighbor value = %lld, min value = %lld,", (long long)___1301[___1924], (long long)MinNeighborValue); ___3183(" max value = %lld.\n", (long long)___2112[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } else if (___1257[___1924] == TECIO_NO_NEIGHBORING_ELEM && ___1301[___1924] == TECIO_NO_NEIGHBORING_ELEM) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLYFACE142) Both left and right neighbors are set to no neighboring element at face %lld.\n", (long long)___1924);
 #endif
NumErrs[___1395]++; return (-1); } } return 0; } } namespace { int32_t writeDecrementedIntegerArray( int32_t        ___1395, int32_t        ___2810, int32_t const* ___2097) { for (int32_t i = 0; i < ___2810; ++i) { if (!___4487(BlckFile[___1395], ___2097[i] - 1)) { WriteErr(___1395, "TECPOLYFACE142"); return -1; } } return 0; } } namespace { int32_t writeElements( int32_t           ___1395, PolyZoneWriteInfo& polyZoneWriteInfo, int32_t           ___2804, int32_t const*    ___1257, int32_t const*    ___1301, int32_t           MinNeighborValue) { if (checkElements(___1395, ___2804, ___1257, ___1301, MinNeighborValue) != 0) return (-1); ___4198(BlckFile[___1395]->File, polyZoneWriteInfo.leftElemsOffset, SEEK_SET); if (writeDecrementedIntegerArray(___1395, ___2804, ___1257) != 0) return -1; polyZoneWriteInfo.leftElemsOffset = (___1396)___4199(BlckFile[___1395]->File); ___4198(BlckFile[___1395]->File, polyZoneWriteInfo.rightElemsOffset, SEEK_SET); if (writeDecrementedIntegerArray(___1395, ___2804, ___1301) != 0) return -1; polyZoneWriteInfo.rightElemsOffset = (___1396)___4199(BlckFile[___1395]->File); return 0; } } namespace { int32_t writeBoundaryConnectionCounts( int32_t           ___1395, PolyZoneWriteInfo& polyZoneWriteInfo, int32_t           ___2776, int32_t const*    ___1251) { ___4198(BlckFile[___1395]->File, polyZoneWriteInfo.connectionCountsOffset, SEEK_SET); if (polyZoneWriteInfo.boundaryConnectionSum == 0) { ___476(TecplotSDKBinaryFileVersion == 191); if (!(___4487(BlckFile[___1395], 0) && ___4487(BlckFile[___1395], 0))) { WriteErr(___1395, "TECPOLYFACE142"); return -1; } } for (___2225 ___1924 = 0; ___1924 < ___2776; ___1924++) { polyZoneWriteInfo.boundaryConnectionSum += ___1251[___1924]; if (___1251[___1924] < 0) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLYBCONN142) Invalid boundary connection count at position %lld:\n", (long long)___1924 + 1); ___3183("     boundary connection count = %lld.\n", (long long)___1251[___1924]);
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], polyZoneWriteInfo.boundaryConnectionSum)) { WriteErr(___1395, "TECPOLYFACE142"); return -1; } } if (polyZoneWriteInfo.boundaryConnectionSum > ___4188[___1395]) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLYBCONN142) Invalid number of boundary connections:\n"); ___3184("     number of boundary connections written = %lld, total number of boundary connections = %lld.", (long long)polyZoneWriteInfo.boundaryConnectionSum, (long long)___4188[___1395]);
 #endif
NumErrs[___1395]++; return (-1); } polyZoneWriteInfo.connectionCountsOffset = (___1396)___4199(BlckFile[___1395]->File); return 0; } } namespace { int32_t writeBoundaryConnectionElements( int32_t           ___1395, PolyZoneWriteInfo& polyZoneWriteInfo, int32_t           ___2776, int32_t const*    ___1251, int32_t const*    ___1252, int32_t const*    ___1254) { if (___4198(BlckFile[___1395]->File, polyZoneWriteInfo.connectionElemsOffset, SEEK_SET) != 0) {
 #if defined MAKEARCHIVE
___3182("Err: (TECPOLYBCONN142) File stream reposition failed.");
 #endif
NumErrs[___1395]++; return (-1); } ___2225 LocalBndryConnCount = 0; for (___2225 ___1924 = 0; ___1924 < ___2776; ___1924++) { for (___2225 BIndex = 0; BIndex < ___1251[___1924]; BIndex++) { if (BIndex > 0 && ___1252[LocalBndryConnCount] == TECIO_NO_NEIGHBORING_ELEM) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLYBCONN142) Partially obscured faces must specify no neighboring element first. See boundary connections for face %lld.\n", (long long)polyZoneWriteInfo.numBoundaryFacesWritten + ___1924 + 1);
 #endif
NumErrs[___1395]++; return (-1); } if (___1252[LocalBndryConnCount] < TECIO_NO_NEIGHBORING_ELEM) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLYBCONN142) Invalid boundary element value at boundary connections for face %lld:\n", (long long)polyZoneWriteInfo.numBoundaryFacesWritten + ___1924 + 1);
 #endif
NumErrs[___1395]++; return (-1); } if (___1252[LocalBndryConnCount] == TECIO_NO_NEIGHBORING_ELEM && ___1254[LocalBndryConnCount] != TECIO_NO_NEIGHBORING_ZONE) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLYBCONN142) Invalid boundary element/zone pair at boundary connections for face %lld:\n", (long long)___1924 + 1); ___3182("     Boundary elements specified as no neighboring element must also specify no neighboring zone.\n");
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], ___1252[LocalBndryConnCount] - 1)) { WriteErr(___1395, "TECPOLYFACE142"); return -1; } LocalBndryConnCount++; } } polyZoneWriteInfo.connectionElemsOffset = (___1396)___4199(BlckFile[___1395]->File); return 0; } } namespace { int32_t writeBoundaryConnectionZones( int32_t           ___1395, PolyZoneWriteInfo& polyZoneWriteInfo, int32_t           ___2776, int32_t const*    ___1251, int32_t const*    ___1254) { ___4198(BlckFile[___1395]->File, polyZoneWriteInfo.connectionZonesOffset, SEEK_SET); int32_t LocalBndryConnCount = 0; for (___2225 ___1924 = 0; ___1924 < ___2776; ___1924++) { for (___2225 BIndex = 0; BIndex < ___1251[___1924]; BIndex++) { if (___1254[LocalBndryConnCount] < TECIO_NO_NEIGHBORING_ZONE) {
 #if defined MAKEARCHIVE
___3183("Err: (TECPOLYBCONN142) Invalid boundary zone value at boundary connections for face %lld:\n", (long long)___1924 + 1);
 #endif
NumErrs[___1395]++; return (-1); } else if (!___4487(BlckFile[___1395], ___1254[LocalBndryConnCount] - 1)) { WriteErr(___1395, "TECPOLYFACE142"); return -1; } LocalBndryConnCount++; } } polyZoneWriteInfo.connectionZonesOffset = (___1396)___4199(BlckFile[___1395]->File); return 0; } } int32_t ___3983( int32_t        ___1395, int32_t const* ___2804, int32_t const* ___1292, int32_t const* ___1295, int32_t const* ___1257, int32_t const* ___1301) { int32_t ___3357 = checkForPolyFacePreconditions(___1395); if (___3357 != 0) return ___3357; int32_t MinNeighborValue; ___3357 = getMinNeighborValue(___1395, MinNeighborValue); if (___3357 != 0) return ___3357;
 #if defined MAKEARCHIVE
if (DebugLevel[___1395] && (*___2804 > 1)) ___3184("Writing %d faces to file %d.\n", *___2804, ___1395 + 1);
 #endif
if (PolyZoneWriteInfos[___1395][___732[___1395]].numFacesWritten == 0 && PolyZoneWriteInfos[___1395][___732[___1395]].numBoundaryFacesWritten == 0) { calculateInitialPolyZoneOffsets(___1395); } PolyZoneWriteInfo& polyZoneWriteInfo = PolyZoneWriteInfos[___1395][___732[___1395]]; ___2225 const beginningFaceNodeSum = polyZoneWriteInfo.faceNodeSum; if (___4690[___1395] == ___1340) ___3357 = writePolyhedralFaceMetadataPiece(___1395, polyZoneWriteInfo, *___2804, ___1292, "TECPOLYFACE142"); if (___3357 != 0) return ___3357; ___3357 = writeFaceNodes(___1395, polyZoneWriteInfo, beginningFaceNodeSum, *___2804, ___1295); if (___3357 != 0) return ___3357; ___3357 = writeElements(___1395, polyZoneWriteInfo, *___2804, ___1257, ___1301, MinNeighborValue); if (___3357 != 0) return ___3357; polyZoneWriteInfo.numFacesWritten += *___2804; if (polyZoneWriteInfo.numFacesWritten == ___2157[___1395] && polyZoneWriteInfo.numBoundaryFacesWritten == ___4189[___1395]) { FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___4224; } return 0; } int32_t ___3982( int32_t        ___1395, int32_t const* ___2776, int32_t const* ___1251, int32_t const* ___1252, int32_t const* ___1254) { if (CheckFile(___1395, "TECPOLYBCONN142") < 0) return (-1); if (PolyZoneWriteInfos[___1395][___732[___1395]].numFacesWritten == 0 && PolyZoneWriteInfos[___1395][___732[___1395]].numBoundaryFacesWritten == 0) { calculateInitialPolyZoneOffsets(___1395); } PolyZoneWriteInfo& polyZoneWriteInfo = PolyZoneWriteInfos[___1395][___732[___1395]]; if (___4189[___1395] > 0) { if (writeBoundaryConnectionCounts(___1395, polyZoneWriteInfo, *___2776, ___1251) != 0) return -1; if (writeBoundaryConnectionElements(___1395, polyZoneWriteInfo, *___2776, ___1251, ___1252, ___1254) != 0) return -1; if (writeBoundaryConnectionZones(___1395, polyZoneWriteInfo, *___2776, ___1251, ___1254) != 0) return -1; } polyZoneWriteInfo.numBoundaryFacesWritten += *___2776; if (polyZoneWriteInfo.numFacesWritten == ___2157[___1395] && polyZoneWriteInfo.numBoundaryFacesWritten == ___4189[___1395]) { FaceNeighborsOrMapWritten[___1395][___732[___1395]] = ___4224; } return 0; }

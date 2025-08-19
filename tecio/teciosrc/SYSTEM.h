 #ifndef TECPLOT_SYSTEM_H
 #define TECPLOT_SYSTEM_H
 #if defined EXTERN
 #undef EXTERN
 #endif
 #if defined ___3919
 #define EXTERN
 #else
 #define EXTERN extern
 #endif
 #if defined ALLOW_INTERRUPTS_DURING_SLOW_READS_WRITES
 #define MAX_BYTES_PER_CHUNK 4096L 
 #else
 #define MAX_BYTES_PER_CHUNK 131072L 
 #endif
 #if defined TECPLOTKERNEL
size_t ___485(void const* ___3642, size_t      ___2099, size_t      ___2811, FILE*       ___3946, size_t      maxBytesPerChunk = MAX_BYTES_PER_CHUNK);
 #endif
EXTERN ___372 ___4403(const char *___1437); EXTERN ___372 ___2067(const char *___1437); EXTERN ___372 ___1387(const char *F, ___372   ___3568); EXTERN void ___1174(const char *___1437); EXTERN ___372 ___2068(const char *___1393, ___372   ___2052, ___372   ___3570); EXTERN ___372 ___3355(FILE   *File, int64_t ___2222); EXTERN ___372 ___503(FILE     **F, ___372  ___3568); EXTERN ___372 ___2875(FILE       **F, const char *___1437, ___372  ___2052, ___372  ___1999, ___372  ___1471, ___372  ___3568, ___372  ___2001);
 #endif

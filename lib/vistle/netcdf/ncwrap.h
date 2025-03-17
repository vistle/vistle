#ifndef VISTLE_NETCDF_NCWRAP_H
#define VISTLE_NETCDF_NCWRAP_H

#include "export.h"

#ifdef USE_NETCDF
#include <netcdf.h>
#ifdef NETCDF_PARALLEL
#include <netcdf_par.h>
#endif

#include <boost/mpi.hpp>

#include <string>
#include <memory>

namespace vistle {


struct V_NETCDFEXPORT NcFile {
    bool parallel = false;
    int error = NC_NOERR;
    bool valid = false;
    std::string name;
    int id = -1;
    NcFile(int id, const std::string &name, bool parallel);
    NcFile(const std::string &name, int err);
    NcFile(NcFile &&o);
    NcFile(const NcFile &o) = delete;
    NcFile &operator=(const NcFile &o) = delete;
    ~NcFile();
    operator bool() const;
    operator int() const;

    static std::unique_ptr<NcFile> open(const std::string &name);
    static std::unique_ptr<NcFile> open(const std::string &name, const MPI_Comm &comm);
};

//DIMENSION EXISTS
//check if a given dimension exists in the NC-files
bool V_NETCDFEXPORT hasDimension(int ncid, std::string findName);
size_t V_NETCDFEXPORT getDimension(int ncid, std::string name);
//VARIABLE EXISTS
//check if a given variable exists in the NC-files
bool V_NETCDFEXPORT hasVariable(int ncid, std::string findName);

template<typename T>
bool getVariable(int ncid, std::string name, T *data, std::vector<size_t> start, std::vector<size_t> count);
template<typename T>
std::vector<T> getVariable(int ncid, std::string name, std::vector<size_t> start, std::vector<size_t> count);
template<typename T>
std::vector<T> getVariable(int ncid, std::string name);
template<typename T>
bool getVariable(int ncid, std::string name, T *data, std::vector<size_t> dims);

#define NCWRAP_GETVAR(T, EXT, EXP) \
    EXT template bool EXP getVariable<T>(int ncid, std::string name, T *data, std::vector<size_t> start, \
                                         std::vector<size_t> count); \
    EXT template std::vector<T> EXP getVariable(int ncid, std::string name, std::vector<size_t> start, \
                                                std::vector<size_t> count); \
    EXT template std::vector<T> EXP getVariable(int ncid, std::string name); \
    EXT template bool EXP getVariable(int ncid, std::string name, T *data, std::vector<size_t> dims);

#define NCWRAP_GETVAR_ALL(EXT, EXP) \
    NCWRAP_GETVAR(unsigned char, EXT, EXP) \
    NCWRAP_GETVAR(uint32_t, EXT, EXP) \
    NCWRAP_GETVAR(uint64_t, EXT, EXP) \
    NCWRAP_GETVAR(float, EXT, EXP) \
    NCWRAP_GETVAR(double, EXT, EXP)

NCWRAP_GETVAR_ALL(extern, V_NETCDFEXPORT)

} // namespace vistle
#endif
#endif

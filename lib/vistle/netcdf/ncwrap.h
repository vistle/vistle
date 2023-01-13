#ifndef VISTLE_NCWRAP_H
#define VISTLE_NCWRAP_H

#ifdef USE_NETCDF
#include <netcdf.h>
#ifdef NETCDF_PARALLEL
#include <netcdf_par.h>
#endif

#include <boost/mpi.hpp>

#include <string>

namespace vistle {


struct NcFile {
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

    static NcFile open(const std::string &name);
    static NcFile open(const std::string &name, const MPI_Comm &comm);
};

//DIMENSION EXISTS
//check if a given dimension exists in the NC-files
bool hasDimension(int ncid, std::string findName);
size_t getDimension(int ncid, std::string name);
//VARIABLE EXISTS
//check if a given variable exists in the NC-files
bool hasVariable(int ncid, std::string findName);

template<typename T>
bool getVariable(int ncid, std::string name, T *data, std::vector<size_t> start, std::vector<size_t> count);
template<typename T>
std::vector<T> getVariable(int ncid, std::string name, std::vector<size_t> start, std::vector<size_t> count);
template<typename T>
std::vector<T> getVariable(int ncid, std::string name);
template<typename T>
bool getVariable(int ncid, std::string name, T *data, std::vector<size_t> dims);

} // namespace vistle
#endif
#endif

#include "ncwrap.h"

namespace vistle {

NcFile::NcFile(int id, const std::string &name, bool parallel): parallel(parallel), valid(true), name(name), id(id)
{}
NcFile::NcFile(const std::string &name, int err): error(err), valid(false), name(name)
{}
NcFile::NcFile(NcFile &&o): parallel(o.parallel), valid(std::exchange(o.valid, false)), name(o.name), id(o.id)
{}
NcFile::~NcFile()
{
    if (valid) {
        nc_close(id);
        valid = false;
    }
}
NcFile::operator bool() const
{
    return error == NC_NOERR;
}
NcFile::operator int() const
{
    return id;
}

NcFile NcFile::open(const std::string &name)
{
    int ncid = -1;
    int err = nc_open(name.c_str(), NC_NOWRITE, &ncid);
    if (err != NC_NOERR) {
        std::cerr << "error opening NetCDF file " << name << ": " << nc_strerror(err) << std::endl;
        return NcFile(name, err);
    }
    return NcFile(ncid, name, false);
}

NcFile NcFile::open(const std::string &name, const MPI_Comm &comm)
{
#ifdef NETCDF_PARALLEL
    int ncid = -1;
    int err = nc_open_par(name.c_str(), NC_NOWRITE, comm, MPI_INFO_NULL, &ncid);
    if (err != NC_NOERR) {
        std::cerr << "error opening NetCDF file " << name << ": " << nc_strerror(err) << std::endl;
        return NcFile(name, err);
    }
    return NcFile(ncid, name, true);
#else
    return open(name);
#endif
}

//DIMENSION EXISTS
//check if a given dimension exists in the NC-files
bool hasDimension(int ncid, std::string findName)
{
    int dimid = -1;
    int err = nc_inq_dimid(ncid, findName.c_str(), &dimid);
    return err == NC_NOERR;
}

size_t getDimension(int ncid, std::string name)
{
    size_t dim = 0;
    int dimid = -1;
    int err = nc_inq_dimid(ncid, name.c_str(), &dimid);
    if (err != NC_NOERR) {
        return dim;
    }
    err = nc_inq_dimlen(ncid, dimid, &dim);
    if (err != NC_NOERR) {
        return dim;
    }
    return dim;
}

//VARIABLE EXISTS
//check if a given variable exists in the NC-files
bool hasVariable(int ncid, std::string findName)
{
    int varid = -1;
    int err = nc_inq_varid(ncid, findName.c_str(), &varid);
    return err == NC_NOERR;
}

template<typename S>
struct NcFuncMapBase {
    typedef std::function<int(int, int, S *)> get_var_func();
    //static const get_var_func get_var;
};
template<typename S>
struct NcFuncMap;

#define NCFUNCS(Type, getvar, getvara) \
    template<> \
    struct NcFuncMap<Type> { \
        typedef std::function<int(int, int, Type *)> get_var_func; \
        static get_var_func get_var; \
        typedef std::function<int(int, int, const size_t *, const size_t *, Type *)> get_vara_func; \
        static get_vara_func get_vara; \
    }; \
    NcFuncMap<Type>::get_var_func NcFuncMap<Type>::get_var = getvar; \
    NcFuncMap<Type>::get_vara_func NcFuncMap<Type>::get_vara = getvara;

NCFUNCS(unsigned char, nc_get_var_uchar, nc_get_vara_uchar)
NCFUNCS(unsigned, nc_get_var_uint, nc_get_vara_uint)
NCFUNCS(unsigned long long, nc_get_var_ulonglong, nc_get_vara_ulonglong)
NCFUNCS(float, nc_get_var_float, nc_get_vara_float)
NCFUNCS(double, nc_get_var_double, nc_get_vara_double)

template<typename T>
bool getVariable(int ncid, std::string name, T *data, std::vector<size_t> start, std::vector<size_t> count)
{
    int varid = -1;
    int err = nc_inq_varid(ncid, name.c_str(), &varid);
    if (err != NC_NOERR) {
        std::cerr << "Nc: nc_inq_varid " << name << " error: " << nc_strerror(err) << std::endl;
        return false;
    }
    int ndims = -1;
    err = nc_inq_varndims(ncid, varid, &ndims);
    if (err != NC_NOERR) {
        std::cerr << "Nc: nc_inq_varndims " << name << " error: " << nc_strerror(err) << std::endl;
        return false;
    }
    assert(start.size() == size_t(ndims));
    assert(count.size() == size_t(ndims));
    std::vector<int> dimids(ndims);
    err = nc_inq_vardimid(ncid, varid, dimids.data());
    if (err != NC_NOERR) {
        std::cerr << "Nc: nc_inq_vardimd " << name << " error: " << nc_strerror(err) << std::endl;
        return false;
    }
    std::vector<size_t> dims(ndims);
    for (int i = 0; i < ndims; ++i) {
        err = nc_inq_dimlen(ncid, dimids[i], &dims[i]);
        if (err != NC_NOERR) {
            std::cerr << "Nc: nc_inq_dimlen " << name << " error: " << nc_strerror(err) << std::endl;
            return false;
        }
    }
    size_t size = std::accumulate(count.begin(), count.end(), 1, [](size_t a, size_t b) { return a * b; });

    size_t intmax = std::numeric_limits<int>::max();
    size_t nreads = std::max(size_t(1), (size * sizeof(T) + intmax - 1) / intmax);
    unsigned splitdim = 0;
    while (splitdim < count.size() - 1 && count[splitdim] == 1)
        ++splitdim;
    size_t splitcount = (count[splitdim] + nreads - 1) / nreads;
    std::vector blockcount(count);
    blockcount[splitdim] = splitcount;
    size_t nvalues = std::accumulate(blockcount.begin(), blockcount.end(), 1, [](size_t a, size_t b) { return a * b; });
    for (size_t i = 0; i < nreads; ++i) {
        std::vector<size_t> s(start), c(count);
        s[splitdim] = start[splitdim] + i * splitcount;
        c[splitdim] = std::min(splitcount, count[splitdim] - i * splitcount);
        if (s[splitdim] >= start[splitdim] + count[splitdim])
            continue;
        err = NcFuncMap<T>::get_vara(ncid, varid, s.data(), c.data(), data + i * nvalues);
        if (err != NC_NOERR) {
            std::cerr << "Nc: get_vara " << name << " error: " << nc_strerror(err) << std::endl;
            std::cerr << "i=" << i << ", start:";
            for (auto v: s)
                std::cerr << " " << v;
            std::cerr << ", count:";
            for (auto v: c)
                std::cerr << " " << v;
            std::cerr << std::endl;
            return false;
        }
    }

    return true;
}

template<typename T>
std::vector<T> getVariable(int ncid, std::string name, std::vector<size_t> start, std::vector<size_t> count)
{
    std::vector<T> data;
    size_t size = std::accumulate(count.begin(), count.end(), 1, [](size_t a, size_t b) { return a * b; });
    data.resize(size);
    if (!getVariable(ncid, name, data.data(), start, count)) {
        data.clear();
        data.shrink_to_fit();
    }
    return data;
}

template<typename T>
bool getVariable(int ncid, std::string name, T *data, std::vector<size_t> dims)
{
    std::vector<size_t> start(dims.size());
    return getVariable(ncid, name, data, start, dims);
}

template<typename T>
std::vector<T> getVariable(int ncid, std::string name)
{
    std::vector<T> data;
    int varid = -1;
    int err = nc_inq_varid(ncid, name.c_str(), &varid);
    if (err != NC_NOERR) {
        std::cerr << "Nc: nc_inq_varid " << name << " error: " << nc_strerror(err) << std::endl;
        return data;
    }
    int ndims = -1;
    err = nc_inq_varndims(ncid, varid, &ndims);
    if (err != NC_NOERR) {
        std::cerr << "Nc: nc_inq_varndims " << name << " error: " << nc_strerror(err) << std::endl;
        return data;
    }
    std::vector<int> dimids(ndims);
    err = nc_inq_vardimid(ncid, varid, dimids.data());
    if (err != NC_NOERR) {
        std::cerr << "Nc: nc_inq_vardimd " << name << " error: " << nc_strerror(err) << std::endl;
        return data;
    }
    std::vector<size_t> dims(ndims);
    for (int i = 0; i < ndims; ++i) {
        err = nc_inq_dimlen(ncid, dimids[i], &dims[i]);
        if (err != NC_NOERR) {
            std::cerr << "Nc: nc_inq_dimlen " << name << " error: " << nc_strerror(err) << std::endl;
            return data;
        }
    }
    size_t size = std::accumulate(dims.begin(), dims.end(), 1, [](size_t a, size_t b) { return a * b; });
    data.resize(size);
    if (!getVariable(ncid, name, data.data(), dims)) {
        data.clear();
        data.shrink_to_fit();
    }
    return data;
}

template std::vector<unsigned char> getVariable(int ncid, std::string name, std::vector<size_t> start,
                                                std::vector<size_t> count);
template std::vector<unsigned> getVariable(int ncid, std::string name, std::vector<size_t> start,
                                           std::vector<size_t> count);
template std::vector<unsigned long long> getVariable(int ncid, std::string name, std::vector<size_t> start,
                                                     std::vector<size_t> count);
template std::vector<float> getVariable(int ncid, std::string name, std::vector<size_t> start,
                                        std::vector<size_t> count);
template std::vector<double> getVariable(int ncid, std::string name, std::vector<size_t> start,
                                         std::vector<size_t> count);

template std::vector<unsigned char> getVariable(int ncid, std::string name);
template std::vector<unsigned> getVariable(int ncid, std::string name);
template std::vector<unsigned long long> getVariable(int ncid, std::string name);
template std::vector<float> getVariable(int ncid, std::string name);
template std::vector<double> getVariable(int ncid, std::string name);

} // namespace vistle

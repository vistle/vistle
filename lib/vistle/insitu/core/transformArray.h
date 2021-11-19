#ifndef TRANSFORM_ARRAY_H
#define TRANSFORM_ARRAY_H

#include "callFunctionWithVoidToTypeCast.h"
#include "exception.h"

#include <vistle/core/database.h>
#include <vistle/core/object.h>
#include <vistle/core/uniformgrid.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <type_traits>

namespace vistle {
namespace insitu {

struct TransformArrayException: public InsituException {
    TransformArrayException(const std::string &msg): m_msg("TransformArrayException: " + msg) {}
    const char *what() const noexcept override { return m_msg.c_str(); }
    std::string m_msg;
};

namespace detail {

template<typename Source, typename Dest>
void transformArray(const Source *source, size_t size, Dest d)
{
    for (size_t i = 0; i < size; i++) {
        d[i] = static_cast<typename std::remove_pointer<Dest>::type>(source[i]);
    }
}

template<typename T>
void transformArray(const T *source, size_t size, T *d)
{
    std::copy(source, source + size, d);
}

// dest must be of plain type (no pointer or reference)
template<typename Source, typename Dest>
struct ArrayTransformer {
    void operator()(const Source *s, size_t size, Dest d) { transformArray(s, size, d); }
};

// takes a single array of type Source and and std::array of dim (or bigger) and converts and distributes from source to
// dest
template<typename Source, typename Dest, typename p_Dim>
struct InterleavedArrayTransformer {
    void operator()(const Source *source, size_t size, Dest dest, int dim)
    {
        assert(dest.size() >= static_cast<size_t>(dim));
        for (size_t j = 0; j < size; ++j) {
            for (size_t i = 0; i < static_cast<size_t>(dim); ++i) {
                dest[i][0] = static_cast<typename std::remove_pointer<typename Dest::value_type>::type>(source[0]);
                ++source;
                ++dest[i];
            }
        }
    }
};

template<typename T, typename p_Object, typename p_Mapping, typename p_Interleaved>
struct VtkArray2VistleConverter {
    vistle::DataBase::ptr operator()(const T *source, size_t n, vistle::Object::const_ptr grid,
                                     vistle::DataBase::Mapping m, bool interleaved = false)
    {
        using namespace vistle;
        bool perCell = false;
        Index dim[3] = {Index(n), 1, 1};
        Index numGridDivisions[3] = {Index(n), 1, 1};
        if (auto sgrid = StructuredGridBase::as(grid)) {
            for (int c = 0; c < 3; ++c) {
                dim[c] = sgrid->getNumDivisions(c);
                numGridDivisions[c] = sgrid->getNumDivisions(c);
            }
            if (m == DataBase::Mapping::Element) {
                for (int c = 0; c < 3; ++c)
                    dim[c] > 1 ? --dim[c] : dim[c];
                perCell = true;
            }
            if ((size_t)dim[0] * (size_t)dim[1] * (size_t)dim[2] != n) {
                std::cerr << "vtkArray2Vistle: non-matching grid size: [" << dim[0] << "*" << dim[1] << "*" << dim[2]
                          << "] != " << n << std::endl;
                return nullptr;
            }
        }


        if (interleaved) {
            Vec<Scalar, 3>::ptr cv(new Vec<Scalar, 3>(n));
            Scalar *x = cv->x().data();
            Scalar *y = cv->y().data();
            Scalar *z = cv->z().data();
            Index l = 0;
            for (Index k = 0; k < dim[2]; ++k) {
                for (Index j = 0; j < dim[1]; ++j) {
                    for (Index i = 0; i < dim[0]; ++i) {
                        const Index idx = perCell ? StructuredGridBase::cellIndex(i, j, k, numGridDivisions)
                                                  : StructuredGridBase::vertexIndex(i, j, k, numGridDivisions);
                        x[idx] = source[3 * l];
                        y[idx] = source[3 * l + 1];
                        z[idx] = source[3 * l + 2];

                        ++l;
                    }
                }
            }
            cv->setGrid(grid);
            cv->setMapping(m);
            return cv;
        } else {
            Vec<Scalar, 1>::ptr cf(new Vec<Scalar, 1>(n));
            Scalar *x = cf->x().data();
            Index l = 0;
            for (Index k = 0; k < dim[2]; ++k) {
                for (Index j = 0; j < dim[1]; ++j) {
                    for (Index i = 0; i < dim[0]; ++i) {
                        const Index idx = perCell ? StructuredGridBase::cellIndex(i, j, k, numGridDivisions)
                                                  : StructuredGridBase::vertexIndex(i, j, k, numGridDivisions);
                        x[idx] = source[l];
                        ++l;
                    }
                }
            }
            cf->setGrid(grid);
            cf->setMapping(m);
            return cf;
        }
        std::cerr << "vtkArray2Vistle: something went wrong: [" << dim[0] << "*" << dim[1] << "*" << dim[2]
                  << "] != " << n << std::endl;
        return nullptr;
    }
};

template<typename T, typename p_separator>
struct Printer {
    void operator()(const T *source, size_t size, const std::string &separeator = " ")
    {
        for (size_t i = 0; i < size; i++) {
            std::cerr << source[i] << separeator;
        }
    }
};

template<typename Source, typename Dest, typename p_Pos>
struct ConversionInserter {
    void operator()(const Source *source, size_t SourcePos, Dest dest, size_t destPos)
    {
        dest[destPos] = source[SourcePos];
    }
};

} // namespace detail

// copies array from source to dest and converts from dataType to T
template<typename T>
void transformArray(const void *source, T *dest, size_t size, DataType dataType)
{
    detail::callFunctionWithVoidToTypeCast<void, detail::ArrayTransformer>(source, dataType, size, dest);
}

// copies the data from source to dest while transforming its type
// source : interleaved array, dest: separate destination arrays, size : number of entries in source, dim : number of
// arrays that are interleaved
template<typename T, size_t I>
void transformInterleavedArray(const void *source, std::array<T *, I> dest, int size, DataType dataType, int dim)
{
    if (static_cast<size_t>(dim) > I) {
        throw TransformArrayException("transformInterleavedArray: destination array is smaller than given dimension");
    }
    detail::callFunctionWithVoidToTypeCast<void, detail::InterleavedArrayTransformer>(source, dataType, size, dest,
                                                                                      dim);
}

inline vistle::DataBase::ptr vtkData2Vistle(const void *source, size_t n, DataType dataType,
                                            vistle::Object::const_ptr grid, vistle::DataBase::Mapping m,
                                            bool interleaved = false)
{
    if (!source)
        return nullptr;
    return detail::callFunctionWithVoidToTypeCast<vistle::DataBase::ptr, detail::VtkArray2VistleConverter>(
        source, dataType, n, grid, m, interleaved);
}

template<typename T>
void expandRectilinearToStructured(void *source[3], DataType dataType, const int size[3], std::array<T *, 3> dest)
{
    using namespace vistle;
    const Index dim[3] = {(Index)size[0], (Index)size[1], (Index)size[2]};
    for (Index i = 0; i < dim[0]; i++) {
        for (Index j = 0; j < dim[1]; j++) {
            for (Index k = 0; k < dim[2]; k++) {
                const Index insertionIndex = UniformGrid::vertexIndex(i, j, k, dim);

                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[0], dataType, i,
                                                                                         dest[0], insertionIndex);
                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[1], dataType, j,
                                                                                         dest[1], insertionIndex);
                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[2], dataType, k,
                                                                                         dest[2], insertionIndex);
            }
        }
    }
}

inline vistle::Index VTKVertexIndex(vistle::Index x, vistle::Index y, vistle::Index z, const vistle::Index dims[3])
{
    return ((z * dims[1] + y) * dims[0] + x);
}

template<typename T>
void expandRectilinearToVTKStructured(void *source[3], DataType dataType, const int size[3], std::array<T *, 3> dest)
{
    using namespace vistle;
    const Index dim[3] = {(Index)size[0], (Index)size[1], (Index)size[2]};
    for (Index k = 0; k < dim[2]; ++k) {
        for (Index j = 0; j < dim[1]; ++j) {
            for (Index i = 0; i < dim[0]; ++i) {
                const Index insertionIndex = VTKVertexIndex(i, j, k, dim);

                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[0], dataType, i,
                                                                                         dest[0], insertionIndex);
                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[1], dataType, j,
                                                                                         dest[1], insertionIndex);
                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[2], dataType, k,
                                                                                         dest[2], insertionIndex);
            }
        }
    }
}

inline void printArray(const void *source, DataType dataType, int size, const std::string &separeator = " ")
{
    detail::callFunctionWithVoidToTypeCast<void, detail::Printer>(source, dataType, size, separeator);
}

} // namespace insitu
} // namespace vistle

#endif // !TRANSFORM_ARRAY_H

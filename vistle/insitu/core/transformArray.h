#ifndef TRANSFORM_ARRAY_H
#define TRANSFORM_ARRAY_H

#include "exeption.h"
#include <algorithm>
#include <array>
#include <core/object.h>
#include <core/database.h>
#include <core/uniformgrid.h>
#include <type_traits>
#include <cassert>
#include "dataAdaptor.h"

namespace insitu{

struct TransformArrayExeption: public InsituExeption {
    TransformArrayExeption(const std::string& msg) :m_msg(msg) {}
     const char* what() const noexcept override {
         return ("TransformArrayExeption" + m_msg).c_str();
}
    std::string m_msg;
};

namespace detail {
//cast the void* to dataType* and call Func(dataType*, size, Args2)
//Args1 must be equal to Args2 without pointer or reference
template<typename RetVal, template<typename...Args1> typename Func, typename ...Args2>
RetVal callFunctionWithVoidToTypeCast(const void* source, DataType dataType, size_t size, Args2&&...args) {

    switch (dataType) {
    case DataType::CHAR:
    {
        const char* f = static_cast<const char*>(source);
        return Func<char, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    }
    break;
    case DataType::INT:
    {
        const int* f = static_cast<const int*>(source);
        return Func<int, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    }
    break;
    case DataType::FLOAT:
    {
        const float* f = static_cast<const float*>(source);
        return Func<float, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    }
    break;
    case DataType::DOUBLE:
    {
        const double* f = static_cast<const double*>(source);
        return Func<double, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    }
    break;
    case DataType::LONG:
    {
        const long* f = static_cast<const long*>(source);
        return Func<long, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    }
    break;
    default:
    {
        throw TransformArrayExeption("callFunctionWithVoidToTypeCast: non-numeric data types can not be converted");
    }
    break;
    }
}

template<typename Source, typename Dest>
void transformArray(const Source* source, size_t size, Dest d) {
    std::transform(source, source + size, d, [](Source val) {
        return static_cast<typename std::remove_pointer<Dest>::type>(val);
        });
}

template<typename T>
void transformArray(const T* source, size_t size, T* d) {
    std::copy(source, source + size, d);
}

//dest must be of plain type (no pointer or reference)
template<typename Source, typename Dest>
struct ArrayTransformer {
    void operator()(const Source* s, size_t size, Dest d) {
        transformArray(s, size, d);
    }
};

//takes a singe array of type Source and and std::array of dim (or bigger) and converts and distributes from source to dest
template<typename Source, typename Dest, typename p_Dim>
struct InterleavedArrayTransformer {
    void operator()(const Source* source, size_t size, Dest dest, int dim) {
        assert(dest.size() < dim);
        for (size_t j = 0; j < size; ++j) {
            for (size_t i = 0; i < dim; ++i) {
                dest[i][0] = static_cast<typename std::remove_pointer<typename Dest::value_type>::type>(source[0]);
                ++source;
                ++dest[i];
            }
        }
    }
};

template<typename T, typename p_Object, typename p_Mapping, typename p_Interleaved>
struct VtkArray2VistleConverter {
    vistle::DataBase::ptr operator()(const T* source, size_t n, vistle::Object::const_ptr grid, vistle::DataBase::Mapping m, bool interleaved = false) {
        using namespace vistle;
        bool perCell = false;
        Index dim[3] = { Index(n), 1, 1 };
        Index numGridDivisions[3] = { Index(n), 1, 1 };
        if (auto sgrid = StructuredGridBase::as(grid)) {
            for (int c = 0; c < 3; ++c)
            {
                dim[c] = sgrid->getNumDivisions(c);
                numGridDivisions[c] = sgrid->getNumDivisions(c);
            }
        }

        if (m == DataBase::Mapping::Element) {
            for (int c = 0; c < 3; ++c)
                dim[c] > 1 ? --dim[c] : dim[c];
            perCell = true;
        }
        if (dim[0] * dim[1] * dim[2] != n) {
            std::cerr << "vtkArray2Vistle: non-matching grid size: [" << dim[0] << "*" << dim[1] << "*" << dim[2] << "] != " << n << std::endl;
            return nullptr;
        }

        if (interleaved) {
            Vec<Scalar, 3>::ptr cv(new Vec<Scalar, 3>(n));
            float* x = cv->x().data();
            float* y = cv->y().data();
            float* z = cv->z().data();
            Index l = 0;
            for (Index k = 0; k < dim[2]; ++k) {
                for (Index j = 0; j < dim[1]; ++j) {
                    for (Index i = 0; i < dim[0]; ++i) {
                        const Index idx = perCell ? StructuredGridBase::cellIndex(i, j, k, numGridDivisions) : StructuredGridBase::vertexIndex(i, j, k, numGridDivisions);
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
        }
        else {
            Vec<Scalar, 1>::ptr cf(new Vec<Scalar, 1>(n));
            float* x = cf->x().data();
            Index l = 0;
            for (Index k = 0; k < dim[2]; ++k) {
                for (Index j = 0; j < dim[1]; ++j) {
                    for (Index i = 0; i < dim[0]; ++i) {
                        const Index idx = perCell ? StructuredGridBase::cellIndex(i, j, k, numGridDivisions) : StructuredGridBase::vertexIndex(i, j, k, numGridDivisions);
                        x[idx] = source[l];
                        ++l;
                    }
                }
            }
            cf->setGrid(grid);
            cf->setMapping(m);
            return cf;
        }
        std::cerr << "vtkArray2Vistle: something went wrong: [" << dim[0] << "*" << dim[1] << "*" << dim[2] << "] != " << n << std::endl;
        return nullptr;
    }
};

template<typename T, typename p_separator>
struct Printer
{
    void operator()(const T* source, size_t size, const std::string& separeator = " ")
    {
        for (size_t i = 0; i < size; i++)
        {
            std::cerr << source[i] << separeator;
        }
    }
};

template<typename Source, typename Dest, typename p_Pos>
struct ConversionInserter {
    void operator()(const Source* source, size_t SourcePos, Dest dest, size_t destPos) {
        dest[destPos] = source[SourcePos];
    }

};

} //detail

template<typename T>
void transformArray(const Array &source, T* dest) {
    detail::callFunctionWithVoidToTypeCast<void, detail::ArrayTransformer>(source.data, source.dataType, source.size, dest);
}

//copies array from source to dest and converts from dataType to T
template<typename T>
void transformArray(const void* source, T* dest, size_t size, DataType dataType) {
    detail::callFunctionWithVoidToTypeCast<void, detail::ArrayTransformer> (source, dataType, size, dest);
}









//copies the data from source to dest while transforming its type
//source : interleaved array, dest: separate destination arrays, size : number of entris in source, dim : number of arrays that are interleaved
template<typename T, size_t I>
void transformInterleavedArray(const void* source, std::array<T*, I> dest, int size, DataType dataType, int dim) {
    if (dim > I) {
        throw TransformArrayExeption("transformInterleavedArray: destination array is smaler than given dimension");
    }
    detail::callFunctionWithVoidToTypeCast<void, detail::InterleavedArrayTransformer>(source, dataType, size, dest, dim);
}

template<typename T, size_t I>
void transformInterleavedArray(const Array& source, std::array<T*, I> dest, int dim) {
    if (dim > I) {
        throw TransformArrayExeption("transformInterleavedArray: destination array is smaler than given dimension");
    }
    detail::callFunctionWithVoidToTypeCast<void, detail::InterleavedArrayTransformer>(source.data, source.dataType, source.size, dest, dim);
}



inline vistle::DataBase::ptr vtkData2Vistle(const void* source, size_t n, DataType dataType, vistle::Object::const_ptr grid, vistle::DataBase::Mapping m, bool interleaved = false) {
    using namespace vistle;
    DataBase::ptr data;

    if (!source)
        return data;

    return detail::callFunctionWithVoidToTypeCast< vistle::DataBase::ptr, detail::VtkArray2VistleConverter>(source, dataType, n, grid, m, interleaved);
}



template<typename T>
void expandRectilinearToStructured(void* source[3], DataType dataType, const int size[3], std::array<T*, 3> dest) {
    using namespace vistle;
    const Index dim[3] = { (Index)size[0], (Index)size[1], (Index)size[2] };
    for (Index i = 0; i < dim[0]; i++) {
        for (Index j = 0; j < dim[1]; j++) {
            for (Index k = 0; k < dim[2]; k++) {
                const Index insertionIndex = UniformGrid::vertexIndex(i, j, k, dim);

                //unstrGridOut->x()[insertionIndex] = obj->coords(0)[i];
                //unstrGridOut->y()[insertionIndex] = obj->coords(1)[j];
                //unstrGridOut->z()[insertionIndex] = obj->coords(2)[k];

                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[0], dataType, i, dest[0], insertionIndex);
                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[1], dataType, j, dest[1], insertionIndex);
                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[2], dataType, k, dest[2], insertionIndex);
            }
        }
    }
}

inline vistle::Index VTKVertexIndex(vistle::Index x, vistle::Index y, vistle::Index z, const vistle::Index dims[3])
{
    return ((z * dims[1] + y) * dims[0] + x);
}

template<typename T>
void expandRectilinearToVTKStructured(void* source[3], DataType dataType, const int size[3], std::array<T*, 3> dest) {
    using namespace vistle;
    const Index dim[3] = { (Index)size[0], (Index)size[1], (Index)size[2] };
    for (Index k = 0; k < dim[2]; ++k) {
        for (Index j = 0; j < dim[1]; ++j) {
            for (Index i = 0; i < dim[0]; ++i) {
                const Index insertionIndex = VTKVertexIndex(i, j, k, dim);

                //unstrGridOut->x()[insertionIndex] = obj->coords(0)[i];
                //unstrGridOut->y()[insertionIndex] = obj->coords(1)[j];
                //unstrGridOut->z()[insertionIndex] = obj->coords(2)[k];

                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[0], dataType, i, dest[0], insertionIndex);
                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[1], dataType, j, dest[1], insertionIndex);
                detail::callFunctionWithVoidToTypeCast<void, detail::ConversionInserter>(source[2], dataType, k, dest[2], insertionIndex);
            }
        }
    }
}



inline void printArray(const void* source, DataType dataType, int size, const std::string& separeator = " ")
{
    detail::callFunctionWithVoidToTypeCast<void, detail::Printer>(source, dataType, size, separeator);
}


}//insitu



#endif // !TRANSFORM_ARRAY_H

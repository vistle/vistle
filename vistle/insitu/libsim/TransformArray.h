#ifndef TRANSFORM_ARRAY_H
#define TRANSFORM_ARRAY_H

#include "Exeption.h"
#include <algorithm>
#include <array>
#include "VisItDataTypes.h"
#include <core/object.h>
#include <core/database.h>
#include <type_traits>
#include <cassert>
namespace insitu{

struct TransformArrayExeption: public VistleLibSimExeption {
    TransformArrayExeption(const std::string& msg) :m_msg(msg) {}
     const char* what() const noexcept override {
         return ("TransformArrayExeption" + m_msg).c_str();
}
    std::string m_msg;
};


//cast the void* to dataType* and call Func(dataType*, size, Args2)
//Args1 must be equal to Args2 without pointer or reference
template<typename RetVal, template<typename...Args1> typename Func, typename ...Args2>
RetVal callFunctionWithVoidToTypeCast(void* v, int dataType, size_t size, Args2 &&...args) {
    
    switch (dataType) {
    case VISIT_DATATYPE_CHAR:
    {
        char* f = static_cast<char*>(v);
        return Func<char,typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    }
    break;
    case VISIT_DATATYPE_INT:
    {
        int* f = static_cast<int*>(v);
        return Func<int, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    }
    break;
    case VISIT_DATATYPE_FLOAT:
    {
        float* f = static_cast<float*>(v);
        return Func<float, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    }
    break;
    case VISIT_DATATYPE_DOUBLE:
    {
        double* f = static_cast<double*>(v );
        return Func<double, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    }
    break;
    case VISIT_DATATYPE_LONG:
    {
        long* f = static_cast<long*>(v);
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
//dest must be of plain type (no pointer or reference)
template<typename Source, typename Dest>
struct ArrayTransformer {
    void operator()(Source* s, size_t size, Dest d) {
        transformArray(s, size, d);
          }
};
template<typename Source, typename Dest>
void transformArray(Source* s, size_t size, Dest d) {
    std::transform(s, s + size, d, [](Source val) {
        return static_cast<typename std::remove_pointer<Dest>::type>(val);
        });
}

template<typename T>
void transformArray(T* s, size_t size, T* d) {
    std::copy(s, s + size, d);
}



//copies array from source to dest and converts from dataType to T
template<typename T>
void transformArray(void* source, T* dest, size_t size, int dataType) {
    callFunctionWithVoidToTypeCast<void, ArrayTransformer> (source, dataType, size, dest);
}


//takes a singe array of type Source and and std::array of dim (or bigger) and converts and distributes from source to dest
template<typename Source, typename Dest, typename p_Dim>
struct InterleavedArrayTransformer {
    void operator()(Source* source, size_t size, Dest dest, int dim) {
        assert(dest.size() < dim);
        for (size_t j = 0; j < size; ++j) {
            for (size_t i = 0; i < dim; ++i) {
                dest[i][0] = static_cast<typename std::remove_pointer<Dest::value_type>::type>(*source);
                ++source;
                ++dest[i];
            }
        }
    }
};





//copies the data from source to dest while transforming its type
//source : interleaved array, dest: separate destination arrays, size : number of entris in source, dim : number of arrays that are interleaved
template<typename T, size_t I>
void transformInterleavedArray(void* source, std::array<T*, I> dest, int size, int dataType, int dim) {
    if (dim > I) {
        throw TransformArrayExeption("transformInterleavedArray: destination array is smaler than given dimension");
    }
    callFunctionWithVoidToTypeCast<void, InterleavedArrayTransformer>(source, dataType, size, dest, dim);
}



template<typename T, typename p_Object, typename p_Mapping, typename p_Interleaved>
struct VtkArray2VistleConverter     {
    vistle::DataBase::ptr operator()(T* vd, size_t n, vistle::Object::const_ptr grid, vistle::DataBase::Mapping m, bool interleaved = false) {
        using namespace vistle;
        bool perCell = false;
        Index dim[3] = { 1, 1, 1 };
        if (auto sgrid = StructuredGridBase::as(grid)) {
            for (int c = 0; c < 3; ++c)
                dim[c] = sgrid->getNumDivisions(c);
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
                        const Index idx = perCell ? StructuredGridBase::cellIndex(i, j, k, dim) : StructuredGridBase::vertexIndex(i, j, k, dim);
                        x[idx] = vd[3 * l];
                        y[idx] = vd[3 * l + 1];
                        z[idx] = vd[3 * l + 2];

                        ++l;
                    }
                }
            }
            cv->setGrid(grid);
            cv->setMapping(m);
            return cv;
        } else {
            Vec<Scalar, 1>::ptr cf(new Vec<Scalar, 1>(n));
            float* x = cf->x().data();
            Index l = 0;
            for (Index k = 0; k < dim[2]; ++k) {
                for (Index j = 0; j < dim[1]; ++j) {
                    for (Index i = 0; i < dim[0]; ++i) {
                        const Index idx = perCell ? StructuredGridBase::cellIndex(i, j, k, dim) : StructuredGridBase::vertexIndex(i, j, k, dim);
                        x[idx] = vd[l];
                        ++l;
                    }
                }
            }
            cf->setGrid(grid);
            cf->setMapping(m);
            return cf;
        }
        std::cerr << "vtkArray2Vistle: soething went wrong: [" << dim[0] << "*" << dim[1] << "*" << dim[2] << "] != " << n << std::endl;
        return nullptr;
    }
};



vistle::DataBase::ptr vtkData2Vistle(void* source, size_t n, int dataType, vistle::Object::const_ptr grid, vistle::DataBase::Mapping m, bool interleaved = false) {
    using namespace vistle;
    DataBase::ptr data;

    if (!source)
        return data;

    return callFunctionWithVoidToTypeCast< vistle::DataBase::ptr, VtkArray2VistleConverter>(source, dataType, n, grid, m, interleaved);
}

}



#endif // !TRANSFORM_ARRAY_H

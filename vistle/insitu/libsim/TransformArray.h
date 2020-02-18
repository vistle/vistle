#ifndef TRANSFORM_ARRAY_H
#define TRANSFORM_ARRAY_H

#include "Exeption.h"
#include <algorithm>
#include <array>
#include "VisItDataTypes.h"
#include <core/object.h>
#include <core/database.h>

namespace insitu{

struct TransformArrayExeption: public VistleLibSimExeption {
    TransformArrayExeption(const std::string& msg) :m_msg(msg) {}
     const char* what() const noexcept override {
         return ("TransformArrayExeption" + m_msg).c_str();
}
    std::string m_msg;
};


template<typename Source, typename Dest>
void transformArray(Source* s, Source* end, Dest* d) {
    std::transform(s, end, d, [](Source val) {
        return static_cast<Dest>(val);
        });
}

template<typename SourceDest>
void transformArray(SourceDest* s, SourceDest* end, SourceDest* d) {
    std::copy(s, end, d);
}
//copies array from source to dest and converts from dataType to T
template<typename T>
void transformArray(void* source, T* dest, int size, int dataType) {
    switch (dataType) {
    case VISIT_DATATYPE_CHAR:
    {
        char* f = static_cast<char*>(source);
        transformArray(f, f + size, dest);
    }
    break;
    case VISIT_DATATYPE_INT:
    {
        int* f = static_cast<int*>(source);
        transformArray(f, f + size, dest);
    }
    break;
    case VISIT_DATATYPE_FLOAT:
    {
        float* f = static_cast<float*>(source);
        transformArray(f, f + size, dest);
    }
    break;
    case VISIT_DATATYPE_DOUBLE:
    {
        double* f = static_cast<double*>(source);
        transformArray(f, f + size, dest);
    }
    break;
    case VISIT_DATATYPE_LONG:
    {
        long* f = static_cast<long*>(source);
        transformArray(f, f + size, dest);
    }
    break;
    default:
    {
        throw TransformArrayExeption("non-numeric data types can not be converted");
    }
    break;
    }
}

template<typename Source, typename Dest, size_t I>
void transformInterleavedArray(Source* source, Source* end, std::array<Dest*, I> dest, int dim) {
    while (source != end) {
        for (size_t i = 0; i < dim; ++i) {
            dest[i][0] = static_cast<Dest>(*source);
            ++source;
            ++dest[i];
        }
    }
}
//copies the data from source to dest while transforming its type
//source : interleaved array, dest: separate destination arrays, size : number of entris in source, dim : number of arrays that are interleaved
template<typename T, size_t I>
void transformInterleavedArray(void* source, std::array<T*, I> dest, int size, int dataType, int dim) {
    if (dim > I) {
        throw TransformArrayExeption("transformInterleavedArray: destination array is smaler than given dimension");
    }
    switch (dataType) {
    case VISIT_DATATYPE_CHAR:
    {
        char* f = static_cast<char*>(source);
        transformInterleavedArray(f, f + size, dest, dim);
    }
    break;
    case VISIT_DATATYPE_INT:
    {
        int* f = static_cast<int*>(source);
        transformInterleavedArray(f, f + size, dest, dim);
    }
    break;
    case VISIT_DATATYPE_FLOAT:
    {
        float* f = static_cast<float*>(source);
        transformInterleavedArray(f, f + size, dest, dim);    }
    break;
    case VISIT_DATATYPE_DOUBLE:
    {
        double* f = static_cast<double*>(source);
        transformInterleavedArray(f, f + size, dest, dim);
    }
    break;
    case VISIT_DATATYPE_LONG:
    {
        long* f = static_cast<long*>(source);
        transformInterleavedArray(f, f + size, dest, dim);
    }
    break;
    default:
    {
        throw TransformArrayExeption("non-numeric data types can not be converted");
    }
    break;
    }
}



template<typename T>
vistle::DataBase::ptr vtkArray2Vistle(T* vd, vistle::Index n, vistle::Object::const_ptr grid, bool interleaved = false) {
    using namespace vistle;
    bool perCell = false;
    Index dim[3] = { n, 1, 1 };
    if (auto sgrid = StructuredGridBase::as(grid)) {
        for (int c = 0; c < 3; ++c)
            dim[c] = sgrid->getNumDivisions(c);
    }
    if (n > 0 && n == (dim[0] - 1) * (dim[1] - 1) * (dim[2] - 1)) {
        perCell = true;
        // cell-mapped data
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
                    x[idx] = vd[3*l];
                    y[idx] = vd[3 * l + 1];
                    z[idx] = vd[3 * l + 2];

                    ++l;
                }
            }
        }
        cv->setGrid(grid);
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
        return cf;
    }

    return nullptr;
}

vistle::DataBase::ptr vtkData2Vistle(void* source, vistle::Index n, int dataType, vistle::Object::const_ptr grid, bool interleaved = false) {
    using namespace vistle;
    DataBase::ptr data;

    if (!source)
        return data;


    Index dim[3] = { n, 1, 1 };
    if (auto sgrid = StructuredGridBase::as(grid)) {
        for (int c = 0; c < 3; ++c) {
            dim[c] = sgrid->getNumDivisions(c);
        }
    }
    if (n > 0 && n == (dim[0] - 1) * (dim[1] - 1) * (dim[2] - 1)) {
        // cell-mapped data
        for (int c = 0; c < 3; ++c)
            --dim[c];
    }
    if (dim[0] * dim[1] * dim[2] != n) {
        std::cerr << "vtkData2Vistle: non-matching grid size: [" << dim[0] << "*" << dim[1] << "*" << dim[2] << "] != " << n << std::endl;
        return NULL;
    }
    switch (dataType) {
        case VISIT_DATATYPE_CHAR:
        {
            char* f = static_cast<char*>(source);
            return vtkArray2Vistle(f, n, grid, interleaved);
        }
        break;
        case VISIT_DATATYPE_INT:
        {
            int* f = static_cast<int*>(source);
            return vtkArray2Vistle(f, n, grid, interleaved);


        }
        break;
        case VISIT_DATATYPE_FLOAT:
        {
            float* f = static_cast<float*>(source);
            return vtkArray2Vistle(f, n, grid, interleaved);

        }
            break;
        case VISIT_DATATYPE_DOUBLE:
        {
            double* f = static_cast<double*>(source);
            return vtkArray2Vistle(f, n, grid, interleaved);


        }
        break;
        case VISIT_DATATYPE_LONG:
        {
            long* f = static_cast<long*>(source);
            return vtkArray2Vistle(f, n, grid, interleaved);


        }
        break;
        default:
        {
            throw TransformArrayExeption("non-numeric data types can not be converted");
        }
        break;
    }

    return NULL;
}


}



#endif // !TRANSFORM_ARRAY_H

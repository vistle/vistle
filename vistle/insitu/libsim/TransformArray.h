#ifndef TRANSFORM_ARRAY_H
#define TRANSFORM_ARRAY_H

#include "Exeption.h"
#include <algorithm>
#include "VisItDataTypes.h"

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



}



#endif // !TRANSFORM_ARRAY_H

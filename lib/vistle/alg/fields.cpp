#include "fields.h"

#include <vistle/core/vec.h>
#include <boost/mpl/for_each.hpp>

namespace vistle {

template<class Map>
struct MapAccess {
    typedef typename Map::const_iterator iterator;

    const Map &map;

    MapAccess(const Map &map): map(map) {}
    iterator begin() { return map.begin(); }
    iterator end() { return map.end(); }
};

template<>
struct MapAccess<std::vector<Index>> {
    typedef std::vector<Index> Map;

    struct value_type {
        value_type(Index first, Index second): first(first), second(second) {}

        Index first = 0;
        Index second = 0;
    };
    struct iterator {
        const Map &vec;
        value_type iv;

        iterator(const Map &vec, Index idx): vec(vec), iv(idx, 0) {}

        iterator &operator++()
        {
            ++iv.first;
            return *this;
        }
        iterator operator++(int) { return iterator(vec, iv.first + 1); }
        bool operator!=(const iterator &other) const { return iv.first != other.iv.first; }
        const value_type &operator*()
        {
            iv.second = vec[iv.first];
            return iv;
        }
        const value_type *operator->()
        {
            iv.second = vec[iv.first];
            return &iv;
        }
    };

    const std::vector<Index> &vec;

    MapAccess(const std::vector<Index> &vec): vec(vec) {}
    iterator begin() { return iterator(vec, 0); }
    iterator end() { return iterator(vec, vec.size()); }
};

template<class T, int Dim, class Mapping>
typename Vec<T, Dim>::ptr remapDataTempl(typename Vec<T, Dim>::const_ptr &in, const Mapping &mapping, bool &inverse)
{
    auto size = mapping.size();
    auto out = in->cloneType();
    out->setSize(size);

    const T *data_in[Dim];
    T *data_out[Dim];
    for (int d = 0; d < Dim; ++d) {
        data_in[d] = in->x(d).data();
        data_out[d] = out->x(d).data();
    }

    MapAccess<Mapping> map(mapping);

    if (inverse) {
        for (auto it = map.begin(); it != map.end(); ++it) {
            Index to = it->first;
            Index from = it->second;
            assert(from < in->getSize());
            assert(to < out->getSize());
            for (int d = 0; d < Dim; ++d) {
                data_out[d][to] = data_in[d][from];
            }
        }
    } else {
        for (auto it = map.begin(); it != map.end(); ++it) {
            Index from = it->first;
            Index to = it->second;
            assert(from < in->getSize());
            assert(to < out->getSize());
            for (int d = 0; d < Dim; ++d) {
                data_out[d][to] = data_in[d][from];
            }
        }
    }

    out->copyAttributes(in);
    out->setMapping(in->guessMapping());

    return out;
}


template<class Mapping>
class RemapImpl {
    DataBase::const_ptr &in;
    const Mapping &mapping;
    bool &inverse;
    DataBase::ptr &out;
    bool &found;

public:
    RemapImpl(DataBase::const_ptr &in, const Mapping &mapping, bool &inverse, DataBase::ptr &out, bool &found)
    : in(in), mapping(mapping), inverse(inverse), out(out), found(found)
    {
        assert(in);
    }

    template<typename T>
    void operator()(T)
    {
        if (found)
            return;

        switch (in->dimension()) {
        case 1:
            if (auto in1 = Vec<T, 1>::as(in)) {
                found = true;
                out = remapDataTempl<T, 1, Mapping>(in1, mapping, inverse);
            }
            break;
        case 3:
            if (auto in3 = Vec<T, 3>::as(in)) {
                found = true;
                out = remapDataTempl<T, 3, Mapping>(in3, mapping, inverse);
            }
            break;
        default:
            std::cerr << "remapData: unsupported dimension" << std::endl;
            assert("unsupported dimension" == nullptr);
            break;
        }
    }

    void apply() { boost::mpl::for_each<Scalars>(*this); }
};

template<class Mapping>
DataBase::ptr remapData(typename DataBase::const_ptr &in, const Mapping &mapping, bool inverse)
{
    if (!in)
        return nullptr;

    bool found = false;
    DataBase::ptr out;
    RemapImpl<Mapping> impl(in, mapping, inverse, out, found);
    impl.apply();
    if (!found) {
        std::cerr << "remapData: no matching type found for " << *in << std::endl;
    }
    assert(found);
    return out;
}

template DataBase::ptr remapData<std::vector<Index>>(typename DataBase::const_ptr &in,
                                                     const std::vector<Index> &mapping, bool inverse);
template DataBase::ptr remapData<std::map<Index, Index>>(typename DataBase::const_ptr &in,
                                                         const std::map<Index, Index> &mapping, bool inverse);
} // namespace vistle

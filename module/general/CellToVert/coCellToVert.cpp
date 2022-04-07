
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                                     (C)2005 Visenso ++
// ++ Description: Interpolation from Cell Data to Vertex Data            ++
// ++                 ( CellToVert module functionality  )                ++
// ++                                                                     ++
// ++ Author: Sven Kufer( sk@visenso.de)                                  ++
// ++                                                                     ++
// ++**********************************************************************/

#include "coCellToVert.h"
#include <vistle/core/vec.h>
#include <vistle/core/texture1d.h>
#include <vistle/core/polygons.h>
#include <vistle/core/lines.h>
#include <vistle/core/unstr.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>


namespace vistle {
inline double sqr(Scalar x)
{
    return double(x) * double(x);
}
} // namespace vistle

using namespace vistle;

////// workin' routines
template<typename S>
bool coCellToVert::interpolate(bool unstructured, Index num_elem, Index num_conn, Index num_point,
                               const Index *elem_list, const Index *conn_list, const Byte *type_list,
                               const Index *neighbour_cells, const Index *neighbour_idx, const Scalar *xcoord,
                               const Scalar *ycoord, const Scalar *zcoord, Index numComp, Index dataSize,
                               const S *in_data[], S *out_data[], Algorithm algo_option)
{
    // check for errors
    if (!xcoord || !ycoord || !zcoord) {
        return false;
    }
    if (unstructured && !elem_list) {
        return false;
    }
    if (elem_list && !conn_list) {
        return false;
    }

    for (Index c = 0; c < numComp; ++c) {
        if (!in_data[c])
            return false;
        if (!out_data[c])
            return false;
    }

    // copy original data if already vertex based
    if (dataSize == num_point && dataSize != num_elem) {
        for (Index c = 0; c < numComp; ++c) {
            const S *in = in_data[c];
            S *out = out_data[c];
            for (Index i = 0; i < num_point; i++) {
                out[i] = in[i];
            }
        }
        return true;
    }

    if (unstructured) {
        switch (algo_option) {
        case SIMPLE:
            return simpleAlgo(num_elem, num_conn, num_point, elem_list, conn_list, type_list, numComp, in_data,
                              out_data);

        case SQR_WEIGHT:
            return weightedAlgo(num_elem, num_conn, num_point, elem_list, conn_list, type_list, neighbour_cells,
                                neighbour_idx, xcoord, ycoord, zcoord, numComp, in_data, out_data);
        }
    } else {
        return simpleAlgo(num_elem, num_conn, num_point, elem_list, conn_list, nullptr, numComp, in_data, out_data);
    }

    return true;
}

template<typename S>
bool coCellToVert::simpleAlgo(Index num_elem, Index num_conn, Index num_point, const Index *elem_list,
                              const Index *conn_list, const Byte *type_list, Index numComp, const S *in_data[],
                              S *out_data[])
{
    std::vector<Byte> weight(num_point);
    Byte *weight_num = weight.data();

    std::vector<Scalar> tmp;
    Scalar *tmp_out[3]{nullptr};

    tmp.resize(num_point * numComp);
    for (Index c = 0; c < numComp; ++c) {
        tmp_out[c] = tmp.data() + c * num_point;
    }

    if (elem_list) {
        for (Index i = 0; i < num_elem; i++) {
            if (type_list && (type_list[i] & UnstructuredGrid::TYPE_MASK) == UnstructuredGrid::POLYHEDRON) {
                Index begin = elem_list[i], end = elem_list[i + 1];
                std::vector<Index> verts;
                verts.reserve(end - begin);
                for (Index j = begin; j < end; ++j) {
                    verts.push_back(conn_list[j]);
                }
                std::sort(verts.begin(), verts.end());
                auto last = std::unique(verts.begin(), verts.end());
                for (auto it = verts.begin(); it != last; ++it) {
                    Index vertex = *it;
                    ++weight_num[vertex];
                    for (Index c = 0; c < numComp; ++c) {
                        tmp_out[c][vertex] += in_data[c][i];
                    }
                }
            } else {
                const Index n = elem_list[i + 1] - elem_list[i];
                for (Index j = 0; j < n; j++) {
                    Index vertex = conn_list[elem_list[i] + j];
                    ++weight_num[vertex];
                    for (Index c = 0; c < numComp; ++c) {
                        tmp_out[c][vertex] += in_data[c][i];
                    }
                }
            }
        }
    } else {
        // triangles/quads
        Index N = 0;
        if (conn_list)
            N = num_conn / num_elem;
        else
            N = num_point / num_elem;
        assert(N == 3 || N == 4);
        for (Index elem = 0; elem < num_elem; ++elem) {
            for (Index j = 0; j < N; ++j) {
                const Index vertex = conn_list ? conn_list[elem * N + j] : elem * N + j;
                ++weight_num[vertex];
                for (Index c = 0; c < numComp; ++c) {
                    tmp_out[c][vertex] += in_data[c][elem];
                }
            }
        }
        if (!conn_list) {
            // all weights are 1
            return true;
        }
    }

    // divide value sum by 'weight' (# adjacent cells)
    for (Index vertex = 0; vertex < num_point; vertex++) {
        if (weight_num[vertex] >= 1) {
            for (Index c = 0; c < numComp; ++c) {
                tmp_out[c][vertex] /= weight_num[vertex];
            }
        }
        for (Index c = 0; c < numComp; ++c) {
            out_data[c][vertex] = tmp_out[c][vertex];
        }
    }

    return true;
}

template<>
bool coCellToVert::simpleAlgo<Scalar>(Index num_elem, Index num_conn, Index num_point, const Index *elem_list,
                                      const Index *conn_list, const Byte *type_list, Index numComp,
                                      const Scalar *in_data[], Scalar *out_data[])
{
    // reset everything to 0, != 0 to prevent div/0 errors
    std::vector<unsigned char> weight(num_point);
    unsigned char *weight_num = weight.data();

    for (Index c = 0; c < numComp; ++c) {
        memset(out_data[c], 0, sizeof(Scalar) * num_point);
    }

    if (elem_list) {
        for (Index i = 0; i < num_elem; i++) {
            if (type_list && (type_list[i] & UnstructuredGrid::TYPE_MASK) == UnstructuredGrid::POLYHEDRON) {
                Index begin = elem_list[i], end = elem_list[i + 1];
                std::vector<Index> verts;
                verts.reserve(end - begin);
                for (Index j = begin; j < end; ++j) {
                    verts.push_back(conn_list[j]);
                }
                std::sort(verts.begin(), verts.end());
                auto last = std::unique(verts.begin(), verts.end());
                for (auto it = verts.begin(); it != last; ++it) {
                    Index vertex = *it;
                    ++weight_num[vertex];
                    for (Index c = 0; c < numComp; ++c) {
                        out_data[c][vertex] += in_data[c][i];
                    }
                }
            } else {
                const Index n = elem_list[i + 1] - elem_list[i];
                for (Index j = 0; j < n; j++) {
                    Index vertex = conn_list[elem_list[i] + j];
                    ++weight_num[vertex];
                    for (Index c = 0; c < numComp; ++c) {
                        out_data[c][vertex] += in_data[c][i];
                    }
                }
            }
        }
    } else {
        // triangles/quads
        Index N = 0;
        if (conn_list)
            N = num_conn / num_elem;
        else
            N = num_point / num_elem;
        assert(N == 3 || N == 4);
        for (Index elem = 0; elem < num_elem; ++elem) {
            for (Index j = 0; j < N; ++j) {
                const Index vertex = conn_list ? conn_list[elem * N + j] : elem * N + j;
                ++weight_num[vertex];
                for (Index c = 0; c < numComp; ++c) {
                    out_data[c][vertex] += in_data[c][elem];
                }
            }
        }
        if (!conn_list) {
            // all weights are 1
            return true;
        }
    }

    // divide value sum by 'weight' (# adjacent cells)
    for (Index vertex = 0; vertex < num_point; vertex++) {
        if (weight_num[vertex] >= 1) {
            for (Index c = 0; c < numComp; ++c) {
                out_data[c][vertex] /= (double)weight_num[vertex];
            }
        }
    }

    return true;
}


template<typename S>
bool coCellToVert::weightedAlgo(Index num_elem, Index num_conn, Index num_point, const Index *elem_list,
                                const Index *conn_list, const Byte *type_list, const Index *neighbour_cells,
                                const Index *neighbour_idx, const Scalar *xcoord, const Scalar *ycoord,
                                const Scalar *zcoord, Index numComp, const S *in_data[], S *out_data[])
{
    if (!neighbour_cells || !neighbour_idx)
        return false;

    // now go through all elements and calculate their center

    Index *ePtr = (Index *)elem_list;
    Byte *tPtr = (Byte *)type_list;

    Index el_type;
    Index elem, num_vert_elem;
    Index vert, vertex, *vertex_id;

    Scalar *xc, *yc, *zc;

    Scalar *cell_center_0 = new Scalar[num_elem];
    Scalar *cell_center_1 = new Scalar[num_elem];
    Scalar *cell_center_2 = new Scalar[num_elem];

    for (elem = 0; elem < num_elem; elem++) {
        el_type = *tPtr++; // get this elements type (then go on to the next one)
        if (elem == num_elem - 1) {
            num_vert_elem = num_conn - elem_list[elem];
        } else {
            num_vert_elem = elem_list[elem + 1] - elem_list[elem];
        }
        // # of vertices in current element
        vertex_id = (Index *)conn_list + (*ePtr++); // get ptr to the first vertex-id of current element
        // then go on to the next one

        // place where to store the calculated center-coordinates in
        xc = &cell_center_0[elem];
        yc = &cell_center_1[elem];
        zc = &cell_center_2[elem];

        // reset
        (*xc) = (*yc) = (*zc) = 0.0;

        // the center can be calculated now
        if ((el_type & UnstructuredGrid::TYPE_MASK) == UnstructuredGrid::POLYHEDRON) {
            assert("broken" == nullptr);
            std::vector<Index> vertices;
            vertices.reserve(num_vert_elem);

            int num_averaged = 0;

            for (vert = 0; vert < num_vert_elem; vert++) {
                Index cur_vert = conn_list[elem_list[elem] + vert];
                vertices.emplace_back(cur_vert);
            }

            std::sort(vertices.begin(), vertices.end());
            auto newend = std::unique(vertices.begin(), vertices.end());
            for (auto it = vertices.begin(); it != newend; ++it) {
                Index cur_vert = *it;
                (*xc) += xcoord[cur_vert];
                (*yc) += ycoord[cur_vert];
                (*zc) += zcoord[cur_vert];
                ++num_averaged;
            }
            (*xc) /= num_averaged;
            (*yc) /= num_averaged;
            (*zc) /= num_averaged;
        } else {
            (*xc) += xcoord[*vertex_id];
            (*yc) += ycoord[*vertex_id];
            (*zc) += zcoord[*vertex_id];
            vertex_id++;
        }
        (*xc) /= num_vert_elem;
        (*yc) /= num_vert_elem;
        (*zc) /= num_vert_elem;
    }

    Index actIndex = 0;
    Index ni, cp;

    Index *niPtr = (Index *)neighbour_idx + 1;
    Index *cellPtr = (Index *)neighbour_cells;

    double weight_sum, weight;
    double value_sum_0, value_sum_1, value_sum_2;
    Scalar vx, vy, vz, ccx, ccy, ccz;

    for (vertex = 0; vertex < num_point; vertex++) {
        weight_sum = 0.0;
        value_sum_0 = 0.0;
        value_sum_1 = 0.0;
        value_sum_2 = 0.0;

        vx = xcoord[vertex];
        vy = ycoord[vertex];
        vz = zcoord[vertex];

        ni = *niPtr;

        while (actIndex < ni) // loop over neighbour cells
        {
            cp = *cellPtr;
            ccx = cell_center_0[cp];
            ccy = cell_center_1[cp];
            ccz = cell_center_2[cp];

            // cells with 0 volume are not weighted
            //XXX: was soll das?
            //weight = (weight==0.0) ? 0 : (1.0/weight);
            weight = sqr(vx - ccx) + sqr(vy - ccy) + sqr(vz - ccz);
            weight_sum += weight;

            if (numComp == 1)
                value_sum_0 += weight * in_data[0][cp];
            else {
                value_sum_0 += weight * in_data[0][cp];
                value_sum_1 += weight * in_data[1][cp];
                value_sum_2 += weight * in_data[2][cp];
            }

            actIndex++;
            cellPtr++;
        }

        niPtr++;
        if (weight_sum == 0)
            weight_sum = 1.0;

        if (numComp == 1)
            out_data[0][vertex] = (S)(value_sum_0 / weight_sum);
        else {
            out_data[0][vertex] = (S)(value_sum_0 / weight_sum);
            out_data[1][vertex] = (S)(value_sum_1 / weight_sum);
            out_data[2][vertex] = (S)(value_sum_2 / weight_sum);
        }
    }

    return true;
}

DataBase::ptr coCellToVert::interpolate(Object::const_ptr geo_in, DataBase::const_ptr data_in, Algorithm algo_option)
{
    algo_option = SIMPLE;

    if (!geo_in || !data_in) {
        return DataBase::ptr();
    }

    Index num_elem = 0, num_conn = 0, num_point = 0;
    const Index *elem_list = nullptr, *conn_list = nullptr;
    const Byte *type_list = nullptr;
    const Scalar *xcoord = nullptr, *ycoord = nullptr, *zcoord = nullptr;

    Index *neighbour_cells = nullptr;
    Index *neighbour_idx = nullptr;

    bool unstructured = false;
    if (auto pgrid_in = Indexed::as(geo_in)) {
        num_elem = pgrid_in->getNumElements();
        num_point = pgrid_in->getNumCoords();
        num_conn = pgrid_in->getNumCorners();
        xcoord = &pgrid_in->x()[0];
        ycoord = &pgrid_in->y()[0];
        zcoord = &pgrid_in->z()[0];
        conn_list = &pgrid_in->cl()[0];
        elem_list = &pgrid_in->el()[0];
        if (auto ugrid_in = UnstructuredGrid::as(pgrid_in)) {
            unstructured = true;
            type_list = &ugrid_in->tl()[0];
#if 0
         if( algo_option==SQR_WEIGHT )
         {
            Index vertex;
            ugrid_in->getNeighborList(&vertex,&neighbour_cells,&neighbour_idx); 
         }
#endif
        }
    } else if (auto tri = Triangles::as(geo_in)) {
        num_point = tri->getNumCoords();
        num_conn = tri->getNumCorners();
        num_elem = num_conn > 0 ? num_conn / 3 : num_point / 3;
        xcoord = &tri->x()[0];
        ycoord = &tri->y()[0];
        zcoord = &tri->z()[0];
        if (num_conn > 0)
            conn_list = &tri->cl()[0];
    } else if (auto qua = Quads::as(geo_in)) {
        num_point = qua->getNumCoords();
        num_conn = qua->getNumCorners();
        num_elem = num_conn > 0 ? num_conn / 4 : num_point / 4;
        xcoord = &qua->x()[0];
        ycoord = &qua->y()[0];
        zcoord = &qua->z()[0];
        if (num_conn > 0)
            conn_list = &qua->cl()[0];
    }

    const Index dataSize = data_in->getSize();
    if (dataSize < num_elem) {
        return DataBase::ptr();
    }

    Index numComp = 0;
    DataBase::ptr data_return;
    const Scalar *in_data[3] = {nullptr, nullptr, nullptr};
    Scalar *out_data[3] = {nullptr, nullptr, nullptr};
    const Index *in_data_i[3] = {nullptr, nullptr, nullptr};
    Index *out_data_i[3] = {nullptr, nullptr, nullptr};
    const Byte *in_data_b[3] = {nullptr, nullptr, nullptr};
    Byte *out_data_b[3] = {nullptr, nullptr, nullptr};

    if (auto tex_in = Texture1D::as(data_in)) {
        numComp = 1;
        in_data[0] = &tex_in->x()[0];

        Texture1D::ptr tex(new Texture1D(0, 0., 1.));
        tex->d()->pixels = tex_in->d()->pixels;
        tex->setSize(num_point);
        out_data[0] = tex->x().data();
        data_return = tex;
    } else if (auto s_data_in = Vec<Scalar>::as(data_in)) {
        numComp = 1;
        in_data[0] = &s_data_in->x()[0];

        Vec<Scalar>::ptr sdata(new Vec<Scalar>(num_point));
        out_data[0] = sdata->x().data();
        data_return = sdata;
    } else if (auto v_data_in = Vec<Scalar, 3>::as(data_in)) {
        numComp = 3;
        in_data[0] = &v_data_in->x()[0];
        in_data[1] = &v_data_in->y()[0];
        in_data[2] = &v_data_in->z()[0];

        Vec<Scalar, 3>::ptr vdata(new Vec<Scalar, 3>(num_point));
        out_data[0] = vdata->x().data();
        out_data[1] = vdata->y().data();
        out_data[2] = vdata->z().data();
        data_return = vdata;
    } else if (auto s_data_in = Vec<Index>::as(data_in)) {
        numComp = 1;
        in_data_i[0] = &s_data_in->x()[0];

        Vec<Index>::ptr sdata(new Vec<Index>(num_point));
        out_data_i[0] = sdata->x().data();
        data_return = sdata;
    } else if (auto v_data_in = Vec<Index, 3>::as(data_in)) {
        numComp = 3;
        in_data_i[0] = &v_data_in->x()[0];
        in_data_i[1] = &v_data_in->y()[0];
        in_data_i[2] = &v_data_in->z()[0];

        Vec<Index, 3>::ptr vdata(new Vec<Index, 3>(num_point));
        out_data_i[0] = vdata->x().data();
        out_data_i[1] = vdata->y().data();
        out_data_i[2] = vdata->z().data();
        data_return = vdata;
    } else if (auto s_data_in = Vec<Byte>::as(data_in)) {
        numComp = 1;
        in_data_b[0] = &s_data_in->x()[0];

        Vec<Byte>::ptr sdata(new Vec<Byte>(num_point));
        out_data_b[0] = sdata->x().data();
        data_return = sdata;
    } else if (auto v_data_in = Vec<Byte>::as(data_in)) {
        numComp = 3;
        in_data_b[0] = &v_data_in->x()[0];
        in_data_b[1] = &v_data_in->y()[0];
        in_data_b[2] = &v_data_in->z()[0];

        Vec<Byte, 3>::ptr vdata(new Vec<Byte, 3>(num_point));
        out_data_b[0] = vdata->x().data();
        out_data_b[1] = vdata->y().data();
        out_data_b[2] = vdata->z().data();
        data_return = vdata;
    }

    if (in_data[0]) {
        if (interpolate(unstructured, num_elem, num_conn, num_point, elem_list, conn_list, type_list, neighbour_cells,
                        neighbour_idx, xcoord, ycoord, zcoord, numComp, dataSize, in_data, out_data, algo_option)) {
            return data_return;
        }
    } else if (in_data_i[0]) {
        if (interpolate(unstructured, num_elem, num_conn, num_point, elem_list, conn_list, type_list, neighbour_cells,
                        neighbour_idx, xcoord, ycoord, zcoord, numComp, dataSize, in_data_i, out_data_i, algo_option)) {
            return data_return;
        }
    } else if (in_data_b[0]) {
        if (interpolate(unstructured, num_elem, num_conn, num_point, elem_list, conn_list, type_list, neighbour_cells,
                        neighbour_idx, xcoord, ycoord, zcoord, numComp, dataSize, in_data_b, out_data_b, algo_option)) {
            return data_return;
        }
    }

    return DataBase::ptr();
}

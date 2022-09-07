#ifndef CO_CELLTOVERT_H
#define CO_CELLTOVERT_H

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                                     (C)2005 Visenso ++
// ++ Description: Interpolation from Cell Data to Vertex Data            ++
// ++                 ( CellToVert module functionality  )                ++
// ++                                                                     ++
// ++ Author: Sven Kufer( sk@visenso.de)                                  ++
// ++                                                                     ++
// ++**********************************************************************/


#include <cstdlib>

#include <vistle/core/object.h>
#include <vistle/core/scalar.h>
#include <vistle/core/vec.h>

using namespace vistle;

class coCellToVert {
private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Original algorithm by Andreas Werner: + Assume data value related to center of elements.
    //                                       + Calculate the average values weighted by the distance to the
    //                                           surrounding center of elements.
    //
    //        - only implemented for unstructured grids
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    template<typename S>
    static bool weightedAlgo(Index num_elem, Index num_conn, Index num_point, const Index *elem_list,
                             const Index *conn_list, const Byte *type_list, const Index *neighbour_cells,
                             const Index *neighbour_idx, const Scalar *xcoord, const Scalar *ycoord,
                             const Scalar *zcoord, Index numComp, const S *in_data[], S *out_data[]);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //   Take the average value of all elements which the point includes
    //
    //   implemtented for POLYGN, LINES, UNSGRD
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    template<typename S>
    static bool simpleAlgo(Index num_elem, Index num_conn, Index num_point, const Index *elem_list,
                           const Index *conn_list, const Byte *type_list, Index numComp, const S *in_data[],
                           S *out_data[]);

public:
    typedef enum { SQR_WEIGHT = 1, SIMPLE = 2 } Algorithm;

    // note: no attributes copied
    DataBase::ptr interpolate(Object::const_ptr geo_in, DataBase::const_ptr data_in, Algorithm algo_option = SIMPLE);

    //
    //  returns false in case of an error
    //
    template<typename S>
    static bool interpolate(bool unstructured, Index num_elem, Index num_conn, Index num_point, const Index *elem_list,
                            const Index *conn_list, const Byte *type_list, const Index *neighbour_cells,
                            const Index *neighbour_idx, const Scalar *xcoord, const Scalar *ycoord,
                            const Scalar *zcoord, Index numComp, Index dataSize, const S *in_data[], S *out_data[],
                            Algorithm algo_option = SIMPLE);
};
#endif

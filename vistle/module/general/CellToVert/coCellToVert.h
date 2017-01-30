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
#include <core/object.h>
#include <core/scalar.h>
#include <core/vec.h>

using namespace vistle;

class coCellToVert
{
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
       
       bool weightedAlgo(Index num_elem, Index num_conn, Index num_point,
                             const Index *elem_list, const Index *conn_list, const unsigned char *type_list, 
			     const Index *neighbour_cells, const Index *neighbour_idx,
			     const Scalar *xcoord, const Scalar *ycoord, const Scalar *zcoord,
                             Index numComp, const Scalar *in_data[], Scalar *out_data[]);
	
       ////////////////////////////////////////////////////////////////////////////////////////////////////
       //
       //   Take the average value of all elements which the point includes
       //
       //   implemtented for POLYGN, LINES, UNSGRD
       //
       ////////////////////////////////////////////////////////////////////////////////////////////////////
       			     
       bool simpleAlgo(Index num_elem, Index num_conn, Index num_point,
                           const Index *elem_list, const Index *conn_list, const unsigned char *type_list,
                           Index numComp, const Scalar *in_data[], Scalar *out_data[]);

    public:

        typedef enum { SQR_WEIGHT=1, SIMPLE=2 } Algorithm;
	
	// note: no attributes copied		   
        DataBase::ptr interpolate(Object::const_ptr geo_in, DataBase::const_ptr data_in,
                           Algorithm algo_option=SIMPLE );			   
               
        //
	//  returns false in case of an error
	//
        bool  interpolate(bool unstructured, Index num_elem, Index num_conn, Index num_point,
                           const Index *elem_list, const Index *conn_list, const unsigned char *type_list, const Index *neighbour_cells, const Index *neighbour_idx,
               const Scalar *xcoord, const Scalar *ycoord, const Scalar *zcoord,
                           Index numComp, Index dataSize, const Scalar *in_data[], Scalar *out_data[],  Algorithm algo_option=SIMPLE );
};
#endif

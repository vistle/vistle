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
       
       bool weightedAlgo(Index num_elem,  Index num_poIndex,
                             const Index *elem_list, const Index *conn_list, const unsigned char *type_list, 
			     const Index *neighbour_cells, const Index *neighbour_idx,
			     const Scalar *xcoord, const Scalar *ycoord, const Scalar *zcoord,
                             Index numComp, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2,
                             Scalar *out_data_0, Scalar *out_data_1, Scalar *out_data_2 );
	
       ////////////////////////////////////////////////////////////////////////////////////////////////////
       //
       //   Take the average value of all elements which the poIndex includes
       //
       //   implemtented for POLYGN, LINES, UNSGRD
       //
       ////////////////////////////////////////////////////////////////////////////////////////////////////
       			     
       bool simpleAlgo(Index num_elem, Index num_conn, Index num_poIndex,
                           const Index *elem_list, const Index *conn_list,
                           Index numComp, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2,
                           Scalar *out_data_0, Scalar *out_data_1, Scalar *out_data_2 );

    public:

        typedef enum { SQR_WEIGHT=1, SIMPLE=2 } Algorithm;
	
	//
	//  geoType/dataType: type string, .e.g. "UNSGRD"
	//
	//  returns NULL in case of an error
	//
        Object::ptr interpolate( bool unstructured, Index num_elem, Index num_conn, Index num_poIndex,
                           const Index *elem_list, const Index *conn_list, const unsigned char *type_list, const Index *neighbour_cells, const Index *neighbour_idx,
			   const Scalar *xcoord, const Scalar *ycoord, const Scalar *zcoord,
                           Index numComp, Index &dataSize, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2, 
                           Algorithm algo_option=SIMPLE );
 
        Object::ptr interpolate(Object::const_ptr geo_in,
                           Index numComp, Index &dataSize, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2, 
                           Algorithm algo_option=SIMPLE );
		
	// note: no attributes copied		   
        Object::ptr interpolate(Object::const_ptr geo_in, Object::const_ptr data_in,
                           Algorithm algo_option=SIMPLE );			   
               
        //
	//  returns false in case of an error
	//
        bool  interpolate( bool unstructured, Index num_elem, Index num_conn, Index num_poIndex,
                           const Index *elem_list, const Index *conn_list, const unsigned char *type_list, const Index *neighbour_cells, const Index *neighbour_idx,
			   const Scalar *xcoord, const Scalar *ycoord, const Scalar *zcoord,
                           Index numComp, Index &dataSize, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2, 
                           Scalar *out_data_0, Scalar *out_data_1, Scalar *out_data_2,  Algorithm algo_option=SIMPLE );
};
#endif

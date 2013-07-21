
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                                     (C)2005 Visenso ++
// ++ Description: Interpolation from Cell Data to Vertex Data            ++
// ++                 ( CellToVert module functionality  )                ++
// ++                                                                     ++
// ++ Author: Sven Kufer( sk@visenso.de)                                  ++
// ++                                                                     ++
// ++**********************************************************************/

#include "coCellToVert.h"
#include <core/vec.h>
#include <core/polygons.h>
#include <core/lines.h>
#include <core/unstr.h>


namespace vistle
{
inline double sqr(Scalar x)
{
   return double(x)*double(x);
}
}

using namespace vistle;

#define NODES_IN_ELEM(i) ( ((i)==num_elem-1) ? num_conn-elem_list[(i)]:elem_list[(i)+1]-elem_list[(i)])

////// workin' routines
bool
coCellToVert::interpolate( bool unstructured, Index num_elem, Index num_conn, Index num_point,
      const Index *elem_list, const Index *conn_list, const unsigned char *type_list, const Index *neighbour_cells, const Index *neighbour_idx,
      const Scalar *xcoord, const Scalar *ycoord, const Scalar *zcoord,
      Index numComp, Index &dataSize, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2, 
      Scalar *out_data_0, Scalar *out_data_1, Scalar *out_data_2, Algorithm algo_option)
{
   // check for errors
   if( !elem_list || !conn_list || !xcoord || !ycoord || !zcoord || !in_data_0   )
   {      
      return( false );
   }   

   if( numComp != 1 && numComp != 3 )
   {
      return( false );
   }

   // copy original data if already vertex based   
   if( dataSize==num_point )
   {
      Index i;
      for( i=0; i<num_point; i++ )
      {
         out_data_0[i] = in_data_0[i];
         if( numComp==3 )
         {
            out_data_1[i] = in_data_1[i];
            out_data_2[i] = in_data_2[i];
         }
      }
      return true;
   }
   // Make sure we have enough elements
   else if( dataSize < num_elem )
   {      
      return false;
   }

   dataSize=num_elem;

   if( unstructured )
   {
      switch( algo_option )
      {
         case SIMPLE:     return simpleAlgo( num_elem, num_conn, num_point,
                                elem_list, conn_list,
                                numComp, in_data_0, in_data_1, in_data_2,
                                out_data_0, out_data_1, out_data_2);			    

         case SQR_WEIGHT: return weightedAlgo( num_elem, num_point,
                                elem_list, conn_list, type_list, neighbour_cells, neighbour_idx, 
                                xcoord, ycoord, zcoord,
                                numComp, in_data_0, in_data_1, in_data_2,
                                out_data_0, out_data_1, out_data_2);

      }			       
   } 
   else
   {
      return simpleAlgo( num_elem, num_conn, num_point,
            elem_list, conn_list,
            numComp, in_data_0, in_data_1, in_data_2,
            out_data_0, out_data_1, out_data_2);
   }

   return true;			       
}

bool
coCellToVert::simpleAlgo( Index num_elem, Index num_conn, Index num_point,
      const Index *elem_list, const Index *conn_list,
      Index numComp, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2,
      Scalar *out_data_0, Scalar *out_data_1, Scalar *out_data_2 )
{
   Index i,j,n,vertex;
   enum{ SCALAR=1, VECTOR=3};

   Scalar *weight_num;
   weight_num = new Scalar[num_point];

   if( !weight_num )
      return false;

   // reset everything to 0, != 0 to prevent div/0 errors
   for ( vertex=0; vertex<num_point; vertex++ )
      weight_num[vertex] = 1.0e-30f;

   if( numComp==SCALAR )
      for( vertex=0; vertex<num_point; vertex++ )
         out_data_0[vertex] = 0.0;
   else
   {
      for( vertex=0; vertex<num_point; vertex++ )
         out_data_0[vertex] = 0.0;
      for( vertex=0; vertex<num_point; vertex++ )
         out_data_1[vertex] = 0.0;
      for( vertex=0; vertex<num_point; vertex++ )
         out_data_2[vertex] = 0.0;
   }

   if( numComp==SCALAR )
      for( i=0; i<num_elem; i++ )
      {
         n = NODES_IN_ELEM(i);
         for( j=0; j<n; j++ )
         {
            vertex = conn_list[ elem_list[i]+j ];
            weight_num[ vertex ] += 1.0;
            out_data_0[vertex] += in_data_0[i];
         }
      }
   else
      for( i=0; i<num_elem; i++ )
      {
         n = NODES_IN_ELEM(i);
         for( j=0; j<n; j++ )
         {
            vertex = conn_list[ elem_list[i]+j ];
            weight_num[ vertex ] += 1.0;
            out_data_0[vertex] += in_data_0[i];
            out_data_1[vertex] += in_data_1[i];
            out_data_2[vertex] += in_data_2[i];
         }
      }

   // divide value sum by 'weight' (# adjacent cells)

   if( numComp==SCALAR )
   {
      for( vertex=0; vertex<num_point; vertex++ )
         if (weight_num[vertex]>=1.0)
            out_data_0[vertex] /= weight_num[vertex];	 	  
   }
   else
   {
      for( vertex=0; vertex<num_point; vertex++ )
      {
         if (weight_num[vertex]>=1.0)
         {
            out_data_0[vertex] /= weight_num[vertex];
            out_data_1[vertex] /= weight_num[vertex];
            out_data_2[vertex] /= weight_num[vertex];
         }	 
      }
   }

   // clean up
   delete[] weight_num;

   return true;
}

bool
coCellToVert::weightedAlgo( Index num_elem, /*Index num_conn,*/ Index num_point,
      const Index *elem_list, const Index *conn_list, const unsigned char *type_list, const Index *neighbour_cells, const Index *neighbour_idx,
      const Scalar *xcoord, const Scalar *ycoord, const Scalar *zcoord,
      Index numComp, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2,
      Scalar *out_data_0, Scalar *out_data_1, Scalar *out_data_2 )
{
   if( !neighbour_cells || !neighbour_idx )
      return false;

   // now go through all elements and calculate their center

   Index *ePtr = (Index *) elem_list;
   unsigned char *tPtr = (unsigned char *) type_list;

   Index el_type;
   Index elem, num_vert_elem;
   Index vert, vertex, *vertex_id;

   Scalar *xc, *yc, *zc;

   Scalar *cell_center_0 = new Scalar[num_elem];
   Scalar *cell_center_1 = new Scalar[num_elem];
   Scalar *cell_center_2 = new Scalar[num_elem];

   static const Index num_vertices_per_element[] = {0,2,3,4,4,5,6,8};

   for( elem=0; elem<num_elem; elem++ )
   {
      el_type = *tPtr++;                    // get this elements type (then go on to the next one)
      num_vert_elem = num_vertices_per_element[el_type];
      // # of vertices in current element
      vertex_id = (Index *)conn_list + (*ePtr++);    // get ptr to the first vertex-id of current element
      // then go on to the next one

      // place where to store the calculated center-coordinates in
      xc = &cell_center_0[elem];
      yc = &cell_center_1[elem];
      zc = &cell_center_2[elem];

      // reset
      (*xc) = (*yc) = (*zc) = 0.0;

      // the center can be calculated now
      for( vert=0; vert<num_vert_elem; vert++ )
      {
         (*xc) += xcoord[ *vertex_id ];
         (*yc) += ycoord[ *vertex_id ];
         (*zc) += zcoord[ *vertex_id ];
         vertex_id++;
      }
      (*xc) /= num_vert_elem;
      (*yc) /= num_vert_elem;
      (*zc) /= num_vert_elem;
   }


   Index actIndex = 0;
   Index ni, cp;

   Index *niPtr   = (Index *) neighbour_idx + 1;
   Index *cellPtr = (Index *) neighbour_cells;

   double weight_sum,weight;
   double value_sum_0, value_sum_1, value_sum_2;
   Scalar vx, vy, vz, ccx, ccy, ccz;

   for (vertex=0; vertex<num_point; vertex++)
   {
      weight_sum=0.0;
      value_sum_0 = 0.0;
      value_sum_1 = 0.0;
      value_sum_2 = 0.0;

      vx = xcoord[vertex];
      vy = ycoord[vertex];
      vz = zcoord[vertex];

      ni = *niPtr;

      while( actIndex < ni )             // loop over neighbour cells
      {
         cp = *cellPtr;
         ccx = cell_center_0[cp];
         ccy = cell_center_1[cp];
         ccz = cell_center_2[cp];

         // cells with 0 volume are not weigthed
         //XXX: was soll das?
         //weight = (weight==0.0) ? 0 : (1.0/weight);
         weight = sqr(vx-ccx) + sqr(vy-ccy) + sqr(vz-ccz) ;
         weight_sum += weight;

         if ( numComp==1 )
            value_sum_0 += weight * in_data_0[cp];
         else
         {
            value_sum_0 += weight * in_data_0[cp];
            value_sum_1 += weight * in_data_1[cp];
            value_sum_2 += weight * in_data_2[cp];
         }

         actIndex++;
         cellPtr++;
      }

      niPtr++;
      if (weight_sum==0)
         weight_sum=1.0;

      if( numComp==1 )
         out_data_0[vertex] = (Scalar)(value_sum_0 / weight_sum);
      else
      {
         out_data_0[vertex] = (Scalar)(value_sum_0 / weight_sum);
         out_data_1[vertex] = (Scalar)(value_sum_1 / weight_sum);
         out_data_2[vertex] = (Scalar)(value_sum_2 / weight_sum);
      }
   }

   return true;
}

Object::ptr
coCellToVert::interpolate( bool unstructured, Index num_elem, Index num_conn, Index num_point,
      const Index *elem_list, const Index *conn_list, const unsigned char *type_list, 
      const Index *neighbour_cells, const Index *neighbour_idx,
      const Scalar *xcoord, const Scalar *ycoord, const Scalar *zcoord,
      Index numComp, Index &dataSize, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2, 
      Algorithm algo_option )
{
   Object::ptr data_return;

   Scalar *out_data_0 = NULL;
   Scalar *out_data_1 = NULL;
   Scalar *out_data_2 = NULL;

   if( numComp==1 ) {

      Vec<Scalar>::ptr sdata(new Vec<Scalar>(num_point));
      out_data_0 = sdata->x().data();
      data_return = sdata;
   } else if( numComp == 3 ) {
      Vec<Scalar, 3>::ptr vdata(new Vec<Scalar, 3>(num_point));
      out_data_0 = vdata->x().data();
      out_data_1 = vdata->y().data();
      out_data_2 = vdata->z().data();
      data_return = vdata;
   } 

   if( !interpolate( unstructured, num_elem, num_conn, num_point,
            elem_list, conn_list, type_list, neighbour_cells, neighbour_idx, xcoord, ycoord, zcoord,
            numComp, dataSize, in_data_0, in_data_1, in_data_2, out_data_0, out_data_1, out_data_2, algo_option ) ) 
   {
      return Object::ptr();
   }

   return data_return;			        
}			   

Object::ptr
coCellToVert::interpolate(Object::const_ptr geo_in,
      Index numComp, Index &dataSize, const Scalar *in_data_0, const Scalar *in_data_1, const Scalar *in_data_2, 
      Algorithm algo_option )
{
   Index num_elem, num_conn, num_point;
   Index *elem_list, *conn_list;
   unsigned char *type_list=NULL;
   Scalar *xcoord, *ycoord, *zcoord;

   if( !geo_in )
   {
      return NULL;   
   }

   Index *neighbour_cells = NULL;
   Index *neighbour_idx   = NULL;

   bool unstructured = false;
   if(auto pgrid_in = Indexed::as(geo_in)) {
      num_elem = pgrid_in->getNumElements();
      num_point = pgrid_in->getNumCoords();
      num_conn = pgrid_in->getNumCorners();
      xcoord = pgrid_in->x().data();
      ycoord = pgrid_in->y().data();
      zcoord = pgrid_in->z().data();
      conn_list = pgrid_in->cl().data();
      elem_list = pgrid_in->el().data();
      if (auto ugrid_in = UnstructuredGrid::as(pgrid_in)) {
         unstructured = true;
         type_list = ugrid_in->tl().data();
#if 0
         if( algo_option==SQR_WEIGHT )
         {
            Index vertex;
            ugrid_in->getNeighborList(&vertex,&neighbour_cells,&neighbour_idx); 
         }	      
#endif
      }
   } else {
      return Object::ptr();
   }

   return interpolate( unstructured, num_elem, num_conn, num_point,
         elem_list, conn_list, type_list, neighbour_cells, neighbour_idx, xcoord, ycoord, zcoord,
         numComp, dataSize, in_data_0, in_data_1, in_data_2, algo_option);    
}		   

Object::ptr
coCellToVert::interpolate(Object::const_ptr geo_in, Object::const_ptr data_in, Algorithm algo_option )
{
   if( !geo_in || !data_in )
   {
      return NULL;
   }

   Scalar *in_data_0=NULL, *in_data_1=NULL, *in_data_2=NULL;
   Index dataSize;

   Index numComp = 0;
   if(auto s_data_in = Vec<Scalar>::as(data_in)) {      
      in_data_0 = s_data_in->x().data();
      in_data_1 = NULL;
      in_data_2 = NULL;
      dataSize = s_data_in->getSize();
      numComp = 1;
   }   
   else if(auto v_data_in = Vec<Scalar, 3>::as(data_in)) {
      in_data_0 = v_data_in->x().data();
      in_data_1 = v_data_in->y().data();
      in_data_2 = v_data_in->z().data();
      dataSize = v_data_in->getSize();    
      numComp = 3;
   }

   return interpolate( geo_in, numComp, dataSize, in_data_0, in_data_1, in_data_2, algo_option);
}


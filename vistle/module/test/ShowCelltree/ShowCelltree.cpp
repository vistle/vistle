#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/unstr.h>
#include <core/lines.h>

#include "ShowCelltree.h"

MODULE_MAIN(ShowCelltree)

using namespace vistle;

ShowCelltree::ShowCelltree(const std::string &shmname, const std::string &name, int moduleID)
: Module("ShowCelltree", shmname, name, moduleID) {

   setDefaultCacheMode(ObjectCache::CacheDeleteLate);

   m_maxDepth = addIntParameter("maximum_depth", "maximum depth of nodes to show", 10);

   createInputPort("grid_in");
   createOutputPort("grid_out");
   createOutputPort("data_out");
}

ShowCelltree::~ShowCelltree() {

}

void visit(Celltree3::Node *nodes, Celltree3::Node &cur, Vector3 min, Vector3 max, Lines::ptr lines, Vec<Scalar>::ptr data, int depth, int maxdepth) {

   if (cur.isLeaf())
      return;

   if (depth > maxdepth)
      return;

   const int D = cur.dim;
   auto &el = lines->el();
   auto &x = lines->x();
   auto &y = lines->y();
   auto &z = lines->z();
   auto &cl = lines->cl();

   el.push_back(el.back()+5);
   for (int i=0; i<5; ++i) {
      cl.push_back(x.size()+(i%4));
   }
   el.push_back(el.back()+5);
   for (int i=0; i<5; ++i) {
      cl.push_back(x.size()+4+(i%4));
   }

   switch(D) {
      case 0:
         x.push_back(cur.Lmax);
         y.push_back(min[1]);
         z.push_back(min[2]);

         x.push_back(cur.Lmax);
         y.push_back(min[1]);
         z.push_back(max[2]);

         x.push_back(cur.Lmax);
         y.push_back(max[1]);
         z.push_back(max[2]);

         x.push_back(cur.Lmax);
         y.push_back(max[1]);
         z.push_back(min[2]);

         x.push_back(cur.Rmin);
         y.push_back(min[1]);
         z.push_back(min[2]);

         x.push_back(cur.Rmin);
         y.push_back(min[1]);
         z.push_back(max[2]);

         x.push_back(cur.Rmin);
         y.push_back(max[1]);
         z.push_back(max[2]);

         x.push_back(cur.Rmin);
         y.push_back(max[1]);
         z.push_back(min[2]);

         x.push_back(cur.Rmin);
         y.push_back(min[1]);
         z.push_back(min[2]);
         break;

      case 1:
         x.push_back(min[0]);
         y.push_back(cur.Lmax);
         z.push_back(min[2]);

         x.push_back(min[0]);
         y.push_back(cur.Lmax);
         z.push_back(max[2]);

         x.push_back(max[0]);
         y.push_back(cur.Lmax);
         z.push_back(max[2]);

         x.push_back(max[0]);
         y.push_back(cur.Lmax);
         z.push_back(min[2]);

         x.push_back(min[0]);
         y.push_back(cur.Rmin);
         z.push_back(min[2]);

         x.push_back(min[0]);
         y.push_back(cur.Rmin);
         z.push_back(max[2]);

         x.push_back(max[0]);
         y.push_back(cur.Rmin);
         z.push_back(max[2]);

         x.push_back(max[0]);
         y.push_back(cur.Rmin);
         z.push_back(min[2]);
         break;

      case 2:
         x.push_back(min[0]);
         y.push_back(min[1]);
         z.push_back(cur.Lmax);

         x.push_back(min[0]);
         y.push_back(max[1]);
         z.push_back(cur.Lmax);

         x.push_back(max[0]);
         y.push_back(max[1]);
         z.push_back(cur.Lmax);

         x.push_back(max[0]);
         y.push_back(min[1]);
         z.push_back(cur.Lmax);

         x.push_back(min[0]);
         y.push_back(min[1]);
         z.push_back(cur.Rmin);

         x.push_back(min[0]);
         y.push_back(max[1]);
         z.push_back(cur.Rmin);

         x.push_back(max[0]);
         y.push_back(max[1]);
         z.push_back(cur.Rmin);

         x.push_back(max[0]);
         y.push_back(min[1]);
         z.push_back(cur.Rmin);
         break;
   }
   for (int i=0; i<8; ++i) {
      data->x().push_back(Scalar(depth));
   }

   const Index L = cur.child;
   const Index R = cur.child+1;
   Vector3 lmax = max;
   lmax[D] = cur.Lmax;
   Vector3 rmin = min;
   rmin[D] = cur.Rmin;
   visit(nodes, nodes[L], min, lmax, lines, data, depth+1, maxdepth);
   visit(nodes, nodes[R], rmin, max, lines, data, depth+1, maxdepth);
}

bool ShowCelltree::compute() {

   UnstructuredGrid::const_ptr in = expect<UnstructuredGrid>("grid_in");
   if (!in)
      return true;

   auto ct = in->getCelltree();

   vistle::Lines::ptr out(new vistle::Lines(Object::Initialized));
   vistle::Vec<Scalar>::ptr data(new vistle::Vec<Scalar>(Object::Initialized));

   if (!ct->nodes().empty()) {
      Vector3 min(ct->min()[0], ct->min()[1], ct->min()[2]);
      Vector3 max(ct->max()[0], ct->max()[1], ct->max()[2]);
      visit(ct->nodes().data(), ct->nodes()[0], min, max, out, data, 0, m_maxDepth->getValue());
   }

   out->copyAttributes(in);
   addObject("grid_out", out);
   addObject("data_out", data);

   return true;
}

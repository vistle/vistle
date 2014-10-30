#ifndef CELLTREE_IMPL_H
#define CELLTREE_IMPL_H

namespace vistle {

//#define CT_DEBUG

//const Scalar Epsilon = std::numeric_limits<Scalar>::epsilon();

template<typename Scalar, typename Index, int NumDimensions>
void Celltree<Scalar, Index, NumDimensions>::init(const Celltree::Vector *min, const Celltree::Vector *max,
      const Celltree::Vector &gmin, const Celltree::Vector &gmax) {

   assert(nodes().size() == 1);
   for (int i=0; i<NumDimensions; ++i)
      this->min()[i] = gmin[i];
   for (int i=0; i<NumDimensions; ++i)
      this->max()[i] = gmax[i];
   refine(min, max, 0, gmin, gmax);
#ifdef CT_DEBUG
   std::cerr << "created celltree: " << nodes().size() << " nodes, " << cells().size() << " cells" << std::endl;
#endif
}

template<typename Scalar, typename Index, int NumDimensions>
void Celltree<Scalar, Index, NumDimensions>::refine(const Celltree::Vector *min, const Celltree::Vector *max, Index curNode,
      const Celltree::Vector &gmin, const Celltree::Vector &gmax) {

   const Scalar smax = std::numeric_limits<Scalar>::max();

   const int NumBuckets = 5;

   Node *node = &(nodes()[curNode]);

   // only split node if necessary
   if (node->size <= 8)
      return;

   // cell index array, contains runs of cells belonging to nodes
   Index *cells = (*d()->m_cells)()->data();

   // sort cells into buckets for each possible split dimension

   // initialize min/max extents of buckets
   std::vector<Index> bucket[NumDimensions][NumBuckets];
   Vector bmin[NumBuckets], bmax[NumBuckets];
   for (int i=0; i<NumBuckets; ++i) {
      bmin[i] = Vector(smax, smax, smax);
      bmax[i] = Vector(-smax, -smax, -smax);
   }

   // find min/max extents of cell centers
   Vector cmin(smax, smax, smax);
   Vector cmax(-smax, -smax, -smax);
   for (Index i=node->start; i<node->start+node->size; ++i) {
      const Index cell = cells[i];
      Vector center = (min[cell]+max[cell])*Scalar(0.5);
      for (int d=0; d<NumDimensions; ++d) {
         if (cmin[d] > center[d])
            cmin[d] = center[d];
         if (cmax[d] < center[d])
            cmax[d] = center[d];
      }
   }

   // sort cells into buckets
   const Vector crange = cmax - cmin;
   for (Index i=node->start; i<node->start+node->size; ++i) {
      const Index cell = cells[i];
      const Vector center = (min[cell]+max[cell])*Scalar(0.5);
      for (int d=0; d<NumDimensions; ++d) {
         int b = int((center[d] - cmin[d])/crange[d] * NumBuckets);
         bool print=false;
         if (b < 0) {
            print = true;
            b = 0;
         }
         if (b >= NumBuckets) {
            print = true;
            b = NumBuckets-1;
         }
         if (print) {
#ifdef CT_DEBUG
            std::cerr << "bad bucket: min=" << cmin[d] << ", max=" << cmax[d] << ", range=" << crange[d] << ", center=" << center[d] << std::endl;
#endif
         }
         assert(b >= 0);
         assert(b < NumBuckets);
         bucket[d][b].push_back(cell);
         if (bmin[b][d] > min[cell][d])
            bmin[b][d] = min[cell][d];
         if (bmax[b][d] < max[cell][d])
            bmax[b][d] = max[cell][d];
      }
   }

   // adjust bucket bounds for empty buckets
   for (int d=0; d<NumDimensions; ++d) {
      for (int b=NumBuckets-2; b>=0; --b) {
         if (bmin[b][d] > bmin[b+1][d])
            bmin[b][d] = bmin[b+1][d];
      }
      for (int b=1; b<NumBuckets; ++b) {
         if (bmax[b][d] < bmax[b-1][d])
            bmax[b][d] = bmax[b-1][d];
      }
   }

   // find best split dimension and plane
   Scalar min_weight(smax);
   int best_dim=-1, best_bucket=-1;
   for (int d=0; d<NumDimensions; ++d) {
      Index nleft = 0;
      for (int split_b=0; split_b<NumBuckets-1; ++split_b) {
         nleft += bucket[d][split_b].size();
         assert(node->size >= nleft);
         const Index nright = node->size - nleft;
         Scalar weight = nleft * (bmax[split_b][d]-bmin[0][d])
            + nright * (bmax[NumBuckets-1][d]-bmin[split_b+1][d]);
         weight /= node->size;
         //std::cerr << "d="<<d<< ", b=" << split_b<< ", weight=" << weight << std::endl;
         if (nleft>0 && nright>0 && weight < min_weight) {
            min_weight = weight;
            best_dim = d;
            best_bucket = split_b;
         }
      }
   }
   if (best_dim == -1) {
      std::cerr << "abandoning split with " << node->size << " children" << std::endl;
      return;
   }
#ifdef CT_DEBUG
   std::cerr << "split: dim=" << best_dim << ", bucket=" << best_bucket << std::endl;
#endif

   // split index lists...
   const Index start = node->start;
   Index i = start;

   // record children into node being split
   *node = Node(best_dim, bmax[best_bucket][best_dim], bmin[best_bucket+1][best_dim], nodes().size());

   // add leaf nodes:
   // ...left node
   for (int b=0; b<best_bucket+1; ++b) {
      for (typename std::vector<Index>::const_iterator it = bucket[best_dim][b].begin();
            it != bucket[best_dim][b].end();
            ++it) {
         const Index c = *it;
         cells[i] = c;
         ++i;
      }
   }
   Index l = nodes().size();
   Index lSize = i-start;
   nodes().push_back(Node(start, i-start));

   // ...right node
   for (int b=best_bucket+1; b<NumBuckets; ++b) {
      for (typename std::vector<Index>::const_iterator it = bucket[best_dim][b].begin();
            it != bucket[best_dim][b].end();
            ++it) {
         const Index c = *it;
         cells[i] = c;
         ++i;
      }
   }
   Index r = nodes().size();
   nodes().push_back(Node(start+lSize, i-start-lSize));

   assert(nodes()[l].size < size);
   assert(nodes()[r].size < size);
   assert(nodes()[l].size + nodes()[r].size == size);

   Vector nmin = gmin;
   Vector nmax = gmax;

   // further refinement for left...
   nmin[best_dim] = bmin[0][best_dim];
   nmax[best_dim] = bmax[best_bucket][best_dim];
   refine(min, max, l, nmin, nmax);

   // ...and right subnodes
   nmin[best_dim] = bmin[best_bucket+1][best_dim];
   nmax[best_dim] = bmax[NumBuckets-1][best_dim];
   refine(min, max, r, nmin, nmax);
}

template<typename Scalar, typename Index, int NumDimensions>
template<class BoundsFunctor>
bool Celltree<Scalar, Index, NumDimensions>::validateTree(BoundsFunctor &boundFunc) const {

#ifdef CT_DEBUG
   std::cerr << "validateCelltree: min=" << min()[0] << " " << min()[1] << " " << min()[2] << ", max=" << max()[0] << " " << max()[1] << " " << max()[2] << std::endl;
#endif
   return validateNode(boundFunc, 0,
                       Vector(min()[0], min()[1], min()[2]),
                       Vector(max()[0], max()[1], max()[2]));
}

template<typename Scalar, typename Index, int NumDimensions>
template<class BoundsFunctor>
bool Celltree<Scalar, Index, NumDimensions>::validateNode(BoundsFunctor &boundFunc, Index nodenum, const Celltree::Vector &min, const Celltree::Vector &max) const {

   bool valid = true;
   Index *cells = (*d()->m_cells)()->data();
   Node *node = &(nodes()[nodenum]);
   if (node->isLeaf()) {
      for (Index i=node->start; i<node->start+node->size; ++i) {
         const Index elem = cells[i];
         Vector emin, emax;
         boundFunc(elem, &emin, &emax);
         for (int i=0; i<NumDimensions; ++i) {
            if (emin[i] < min[i]) {
               std::cerr << "celltree: min violation on elem " << elem << ": is " << emin << ", should " << min << std::endl;
               valid = false;
            }
            if (emax[i] > max[i]) {
               std::cerr << "celltree: max violation on elem " << elem << ": is " << emax << ", should " << max << std::endl;
               valid = false;
            }
         }
      }
      return valid;
   }

   Vector lmax(max), rmin(min);
   lmax[node->dim] = node->Lmax;
   rmin[node->dim] = node->Rmin;

   return validateNode(boundFunc, node->left(), min, lmax)
      && validateNode(boundFunc, node->right(), rmin, max);
}

template <class Scalar, class Index, int NumDimensions>
template <class Archive>
void Celltree<Scalar, Index, NumDimensions>::Data::serialize(Archive &ar, const unsigned int version) {

   m_bounds->resize(NumDimensions*2);

   assert("serialization of Celltree::Node not implemented" == 0);
}

template<typename Scalar, typename Index, int NumDimensions>
Celltree<Scalar, Index, NumDimensions>::Celltree(const Index numCells,
      const Meta &meta)
: Celltree::Base(Celltree::Data::create(numCells, meta))
{
}

template<typename Scalar, typename Index, int NumDimensions>
bool Celltree<Scalar, Index, NumDimensions>::isEmpty() const {

   return d()->m_nodes->empty() || d()->m_cells->empty();
}

template<typename Scalar, typename Index, int NumDimensions>
bool Celltree<Scalar, Index, NumDimensions>::checkImpl() const {

   V_CHECK(d()->m_nodes->size() >= 1);
   V_CHECK((*d()->m_nodes)[0].size <= d()->m_cells->size());
   V_CHECK(d()->m_nodes->size() <= d()->m_cells->size());
   V_CHECK(d()->m_bounds->size() == 2*NumDimensions);
   return true;
}

template<typename Scalar, typename Index, int NumDimensions>
Celltree<Scalar, Index, NumDimensions>::Data::Data(const Celltree::Data &o, const std::string &n)
: Celltree::Base::Data(o, n)
, m_bounds(o.m_bounds)
, m_cells(o.m_cells)
, m_nodes(o.m_nodes)
{
}

template<typename Scalar, typename Index, int NumDimensions>
Celltree<Scalar, Index, NumDimensions>::Data::Data(const std::string &name, const Index numCells,
                     const Meta &meta)
: Celltree::Base::Data(Object::Type(Object::CELLTREE1-1+NumDimensions), name, meta)
, m_bounds(new ShmVector<Scalar>(2*NumDimensions))
, m_cells(new ShmVector<Index>(numCells))
, m_nodes(new ShmVector<Node>(1))
{

   Index *cells = m_cells->data();
   for (Index i=0; i<numCells; ++i) {
      cells[i] = i;
   }

   (*m_nodes)[0] = Node(0, numCells);
}

template<typename Scalar, typename Index, int NumDimensions>
typename Celltree<Scalar, Index, NumDimensions>::Data *Celltree<Scalar, Index, NumDimensions>::Data::create(const Index numCells,
                              const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *ct = shm<Data>::construct(name)(name, numCells, meta);
   publish(ct);

   return ct;
}
} // namespace vistle
#endif



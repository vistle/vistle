#include <sstream>
#include <limits>
#include <algorithm>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>

#include <module/module.h>
#include <core/vec.h>
#include <core/scalars.h>
#include <core/message.h>
#include <core/coords.h>
#include <core/lines.h>
#include <core/triangles.h>
#include <core/indexed.h>

using namespace vistle;

class Stats: public vistle::Module {

 public:
   Stats(const std::string &shmname, const std::string &name, int moduleID);
   ~Stats();

   struct stats {
      Index blocks; //!< no. of blocks/partitions
      Index elements; //! no. of elements
      Index vertices; //! no. of vertices
      Index coords; //! no. of coordinates
      Index data[4]; //! no. of data values of correspoinding dim

      stats()
      : blocks(0)
      , elements(0)
      , vertices(0)
      , coords(0)
      {
         data[0] = data[1] = data[2] = data[3] = 0;
      }

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & blocks;
         ar & elements;
         ar & vertices;
         ar & coords;
         for (int i=0; i<4; ++i) {
            ar & data[i];
         }
      }

      template<class operation>
      void apply(const operation &op, const stats &rhs) {

         blocks = op(blocks, rhs.blocks);
         elements = op(elements, rhs.elements);
         vertices = op(vertices, rhs.vertices);
         coords = op(coords, rhs.coords);
         for (int i=0; i<4; ++i) {
            data[i] = op(data[i], rhs.data[i]);
         }
      }

      template<class operation>
      static stats apply(const operation &op, const stats &lhs, const stats &rhs) {

         stats result = lhs;
         result.apply(op, rhs);
         return result;
      }

      stats &operator+=(const stats &rhs) {
         apply(std::plus<Index>(), rhs);
         return *this;
      }
   };

 private:
   static const int MaxDim = ParamVector::MaxDimension;

   virtual bool compute();
   virtual bool reduce(int timestep);

   bool prepare();

   int m_timesteps; //!< no. of time steps
   stats m_cur; //! current timestep
   stats m_min; //! min. values across all timesteps
   stats m_max; //! max. values across all timesteps
   stats m_total; //! accumulated values
   std::map<std::string, Index> m_types; //!< object type counts
};

std::ostream &operator<<(std::ostream &str, const Stats::stats &s) {

   str << "blocks: " << s.blocks
      << ", elements: " << s.elements
      << ", vertices: " << s.vertices
      << ", coords: " << s.coords
      << ", 1-dim data: " << s.data[1]
      << ", 3-dim data: " << s.data[3];

   return str;
}

template<typename T>
struct minimum {
   T operator()(const T &lhs, const T &rhs) const {

      return std::min<T>(lhs, rhs);
   }
};

template<>
struct minimum<Stats::stats> {
   Stats::stats operator()(const Stats::stats &lhs, const Stats::stats &rhs) {

      return Stats::stats::apply(minimum<Index>(), lhs, rhs);
   }
};

template<typename T>
struct maximum {
   T operator()(const T &lhs, const T &rhs) const {

      return std::max<T>(lhs, rhs);
   }
};

template<>
struct maximum<Stats::stats> {
   Stats::stats operator()(const Stats::stats &lhs, const Stats::stats &rhs) {

      return Stats::stats::apply(maximum<Index>(), lhs, rhs);
   }
};

Stats::stats operator+(const Stats::stats &lhs, const Stats::stats &rhs) {

   return Stats::stats::apply(std::plus<Index>(), lhs, rhs);
}

using namespace vistle;

Stats::Stats(const std::string &shmname, const std::string &name, int moduleID)
   : Module("object statistics", shmname, name, moduleID)
{

   setDefaultCacheMode(ObjectCache::CacheAll);
   setReducePolicy(message::ReducePolicy::OverAll);

   createInputPort("data_in", "input data", Port::MULTI);
}

Stats::~Stats() {

}

bool Stats::compute() {

   //std::cerr << "ObjectStatistics: compute: execcount=" << m_executionCount << std::endl;

   while(Object::const_ptr obj = takeFirstObject("data_in")) {

      if (obj->getTimestep() > m_timesteps)
         m_timesteps = obj->getTimestep();

      stats s;
      if (auto i = Indexed::as(obj)) {
         s.elements = i->getNumElements();
         s.vertices = i->getNumCorners();
      } else if (auto t = Triangles::as(obj)) {
         s.elements = t->getNumElements();
         s.vertices = t->getNumCorners();
      }
      if (auto c = Coords::as(obj)) {
         s.coords = c->getNumCoords();
      } else if(auto v = Vec<Scalar, 3>::as(obj)) {
         s.data[3] = v->getSize();
      } else if(auto v = Vec<Scalar, 1>::as(obj)) {
         s.data[1] = v->getSize();
      }
      s.blocks = 1;
      m_cur += s;
   }

   return true;
}

bool Stats::prepare() {

   m_timesteps = 0;
   m_types.clear();

   m_total = stats();
   m_min = stats();
   m_max = stats();
   m_cur = stats();

   return true;
}

bool Stats::reduce(int timestep) {

   if (timestep >= 0)
      ++m_timesteps;

   auto comm = boost::mpi::communicator();

   //std::cerr << "reduction for timestep " << timestep << std::endl;
   boost::mpi::reduce(comm, m_cur, m_min, minimum<stats>(), 0);
   boost::mpi::reduce(comm, m_cur, m_max, maximum<stats>(), 0);
   stats total;
   boost::mpi::reduce(comm, m_cur, total, std::plus<stats>(), 0);
   m_total += total;

   int timesteps = 0;
   boost::mpi::reduce(comm, m_timesteps, timesteps, maximum<int>(), 0);

   std::stringstream str;

   if (timestep >= 0) {
      str << "timestep " << timestep << ": " << total << std::endl;
   } else {
      str << "total";
      if (timesteps > 0)
         str << " (" << timesteps << " timesteps)";
      str << ": ";
      str << m_total << std::endl;
      str << "  rank min: " << m_min << std::endl;
      str << "  rank max: " << m_max << std::endl;
   }

   if (rank() == 0)
      sendInfo(str.str());

   m_cur = stats();

   return Module::reduce(timestep);
}

MODULE_MAIN(Stats)


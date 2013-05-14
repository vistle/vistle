#include <sstream>
#include <limits>
#include <algorithm>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives.hpp>

#include <core/vec.h>
#include <core/module.h>
#include <core/scalars.h>
#include <core/paramvector.h>

using namespace vistle;

class Extrema: public vistle::Module {

 public:
   Extrema(const std::string &shmname, int rank, int size, int moduleID);
   ~Extrema();

 private:
   static const int MaxDim = 4;

   int dim;
   bool handled;
   ParamVector min, max, gmin, gmax;

   virtual bool compute();

   template<int Dim>
   friend struct Compute;

   template<int Dim>
   struct Compute {
      Object::const_ptr object;
      Extrema *module;
      Compute(Object::const_ptr obj, Extrema *module)
         : object(obj)
         , module(module)
      {
      }
      template<typename S> void operator()(S) {

         typedef Vec<S,Dim> V;
         typename V::const_ptr in(V::as(object));
         if (!in)
            return;

         module->handled = true;
         module->min.dim = Dim;
         module->max.dim = Dim;

         size_t size = in->getSize();
         for (unsigned int index = 0; index < size; index ++) {
            for (int c=0; c<Dim; ++c) {
               if (module->min[c] > in->x(c)[index])
                  module->min[c] = in->x(c)[index];
               if (module->max[c] < in->x(c)[index])
                  module->max[c] = in->x(c)[index];
            }
         }
      }
   };

};

using namespace vistle;

Extrema::Extrema(const std::string &shmname, int rank, int size, int moduleID)
   : Module("Extrema", shmname, rank, size, moduleID) {

   Port *din = createInputPort("data_in", "input data", Port::MULTI);
   Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
   din->link(dout);

   addVectorParameter("min",
         "output parameter: minimum",
         ParamVector(
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max()
            ));
   addVectorParameter("max",
         "output parameter: maximum",
         ParamVector(
            -std::numeric_limits<double>::max(),
            -std::numeric_limits<double>::max(),
            -std::numeric_limits<double>::max()
            ));
}

Extrema::~Extrema() {

}

bool Extrema::compute() {

   dim = -1;
   for (int c=0; c<MaxDim; ++c) {
      gmin[c] =  std::numeric_limits<double>::max();
      gmax[c] = -std::numeric_limits<double>::max();
   }

   while(Object::const_ptr obj = takeFirstObject("data_in")) {
      handled = false;

      for (int c=0; c<MaxDim; ++c) {
         min[c] =  std::numeric_limits<double>::max();
         max[c] = -std::numeric_limits<double>::max();
      }

      boost::mpl::for_each<Scalars>(Compute<1>(obj, this));
      boost::mpl::for_each<Scalars>(Compute<3>(obj, this));

      if (!handled) {

         std::string error("could not handle input");
         std::cerr << "Extrema: " << error << std::endl;
         throw(vistle::exception(error));
      }

      if (dim == -1) {
         dim = min.dim;
         gmin.dim = dim;
         gmax.dim = dim;
      } else if (dim != min.dim) {
         std::string error("input dimensions not equal");
         std::cerr << "Extrema: " << error << std::endl;
         throw(vistle::exception(error));
      }

      Object::ptr out = obj->clone();
      out->addAttribute("min", min.str());
      out->addAttribute("max", max.str());
      std::cerr << "Extrema: min " << min << ", max " << max << std::endl;

      addObject("data_out", out);

      boost::mpi::all_reduce(boost::mpi::communicator(),
            min[0], min[0],
            boost::mpi::minimum<double>());

      for (int c=0; c<MaxDim; ++c) {
         if (gmin[c] > min[c])
            gmin[c] = min[c];
         if (gmax[c] < max[c])
            gmax[c] = max[c];
      }
   }

   setVectorParameter("min", gmin);
   setVectorParameter("max", gmax);

   return true;
}

MODULE_MAIN(Extrema)


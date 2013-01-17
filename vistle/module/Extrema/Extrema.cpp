#include <sstream>
#include <iomanip>
#include <limits>
#include <algorithm>

#include <vec.h>
#include <module.h>
#include <scalars.h>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives.hpp>


using namespace vistle;

class Extrema: public vistle::Module {

 public:
   Extrema(const std::string &shmname, int rank, int size, int moduleID);
   ~Extrema();

 private:
   static const int MaxDim = 4;

   int dim;
   bool handled;
   double min[MaxDim], max[MaxDim];
   double gmin[MaxDim], gmax[MaxDim];

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
         module->dim = Dim;

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

   createInputPort("data_in");
   createOutputPort("data_out");

   addVectorParameter("min",
         Vector(
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max()
            ));
   addVectorParameter("max",
         Vector(
            -std::numeric_limits<double>::max(),
            -std::numeric_limits<double>::max(),
            -std::numeric_limits<double>::max()
            ));
}

Extrema::~Extrema() {

}

bool Extrema::compute() {

   for (int c=0; c<MaxDim; ++c) {
      gmin[c] =  std::numeric_limits<double>::max();
      gmax[c] = -std::numeric_limits<double>::max();
   }

   while(Object::const_ptr obj = takeFirstObject("data_in")) {
      handled = false;
      dim = -1;

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

      Object::ptr out = obj->clone();
      std::stringstream smin;
      for (int c=0; c<dim; ++c) {
         if (c > 0)
            smin << " ";
         smin << min[c];
      }
      out->addAttribute("min", smin.str());
      std::stringstream smax;
      for (int c=0; c<dim; ++c) {
         if (c > 0)
            smax << " ";
         smax << max[c];
      }
      out->addAttribute("max", smax.str());
      std::cerr << "Extrema: min " << smin.str() << ", max " << smax.str() << std::endl;

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

   setVectorParameter("min", Vector(gmin[0], gmin[1], gmin[2]));
   setVectorParameter("max", Vector(gmax[0], gmax[1], gmax[2]));

   return true;
}

MODULE_MAIN(Extrema)


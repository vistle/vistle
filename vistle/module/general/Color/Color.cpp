#include <sstream>
#include <iomanip>
#include <cfloat>
#include <limits>
#include <core/vector.h>
#include <core/object.h>
#include <core/vec.h>
#include <core/texture1d.h>
#include <core/coords.h>
#include <util/math.h>

#include "Color.h"

MODULE_MAIN(Color)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(TransferFunction,
                                    (COVISE)
                                    (CoolWarmBrewer)
                                    (Star)
                                    (ITSM)
                                    (Rainbow)
                                    (Blue_Light)
                                    (ANSYS)
                                    (CoolWarm)
                                    (Frosty)
                                    (Dolomiti)
                                    )

ColorMap::ColorMap(std::map<vistle::Scalar, vistle::Vector> & pins,
                   const size_t steps, const size_t w): width(w) {

   data = new unsigned char[width * 4];

   std::map<vistle::Scalar, vistle::Vector>::iterator current = pins.begin();
   std::map<vistle::Scalar, vistle::Vector>::iterator next = ++pins.begin();

   for (size_t index = 0; index < width; index ++) {

       Scalar x = 0.5;
       if (steps > 1) {
           int step = Scalar(index)/(Scalar(width)/Scalar(steps));
           x = step / (vistle::Scalar) (steps-1);
       }
       while (next != pins.end() && x > next->first) {
           ++next;
           ++current;
       }

      if (next == pins.end()) {
         data[index * 4] = current->second[0];
         data[index * 4 + 1] = current->second[1];
         data[index * 4 + 2] = current->second[2];
         data[index * 4 + 3] = 1.0;
      } else {

         vistle::Scalar a = current->first;
         vistle::Scalar b = next->first;

         vistle::Scalar t = (x-a)/(b-a);

         data[index * 4] =
            (lerp(current->second[0], next->second[0], t) * 255.99);
         data[index * 4 + 1] =
            (lerp(current->second[1], next->second[1], t) * 255.99);
         data[index * 4 + 2] =
            (lerp(current->second[2], next->second[2], t) * 255.99);
         data[index * 4 + 3] = 1.0;
      }
   }
}

ColorMap::~ColorMap() {

   delete[] data;
}

Color::Color(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Color", shmname, name, moduleID) {

   createInputPort("data_in");
   createOutputPort("data_out");

   addFloatParameter("min", "minimum value of range to map", 0.0);
   addFloatParameter("max", "maximum value of range to map", 0.0);
   auto map = addIntParameter("map", "transfer function name", CoolWarmBrewer, Parameter::Choice);
   V_ENUM_SET_CHOICES(map, TransferFunction);
   auto steps = addIntParameter("steps", "number of color map steps", 32);
   setParameterRange(steps, (Integer)1, (Integer)1024);

   m_autoRangePara = addIntParameter("auto_range", "compute range automatically", m_autoRange, Parameter::Boolean);
   addIntParameter("preview", "use preliminary colormap for showing preview when determining bounds", true, Parameter::Boolean);

   setCurrentParameterGroup("Nested Color Map");
   m_nestPara = addIntParameter("nest", "inset another color map", m_nest, Parameter::Boolean);
   m_autoInsetCenterPara = addIntParameter("auto_center", "compute center of nested color map", m_autoInsetCenter, Parameter::Boolean);
   addIntParameter("inset_relative", "width and center of inset are relative to range", true, Parameter::Boolean);
   addFloatParameter("inset_center", "where to inset other color map (auto range: 0.5=middle)", 0.5);
   addFloatParameter("inset_width", "range covered by inseted color map (auto range: relative)", 0.1);
   auto inset_map = addIntParameter("inset_map", "transfer function to inset", COVISE, Parameter::Choice);
   V_ENUM_SET_CHOICES(inset_map, TransferFunction);
   auto inset_steps = addIntParameter("inset_steps", "number of color map steps for inset (0: as outer map)", 0);
   setParameterRange(inset_steps, (Integer)0, (Integer)1024);
   auto res = addIntParameter("resolution", "number of steps to compute", 1024);
   setParameterRange(res, (Integer)1, (Integer)1024);

   TF pins;

   using vistle::Vector;
   pins.insert(std::make_pair(0.0, vistle::Vector(0.0, 0.0, 1.0)));
   pins.insert(std::make_pair(0.5, vistle::Vector(1.0, 0.0, 0.0)));
   pins.insert(std::make_pair(1.0, vistle::Vector(1.0, 1.0, 0.0)));
   transferFunctions[COVISE] = pins;
   pins.clear();

   pins[0.00] = Vector(0.10, 0.0, 0.90);
   pins[0.07] = Vector(0.00, 0.00, 1.00);
   pins[0.14] = Vector(0.63, 0.63, 1.00);
   pins[0.21] = Vector(0.00, 0.75, 1.00);
   pins[0.28] = Vector(0.00, 1.00, 1.00);
   pins[0.35] = Vector(0.10, 0.80, 0.70);
   pins[0.42] = Vector(0.10, 0.90, 0.00);
   pins[0.50] = Vector(0.50, 1.00, 0.63);
   pins[0.57] = Vector(0.75, 1.00, 0.25);
   pins[0.64] = Vector(1.00, 1.00, 0.00);
   pins[0.71] = Vector(1.00, 0.80, 0.10);
   pins[0.78] = Vector(1.00, 0.60, 0.30);
   pins[0.85] = Vector(1.00, 0.67, 0.95);
   pins[0.92] = Vector(1.00, 0.00, 0.50);
   pins[1.00] = Vector(1.00, 0.00, 0.00);
   transferFunctions[Star] = pins;
   pins.clear();

   pins[0.] = Vector(1, 1, 1);
   pins[1.] = Vector(0, 0, 1);
   transferFunctions[Blue_Light] = pins;
   pins.clear();

   pins[0.00] = Vector(0, 0, 1);
   pins[0.25] = Vector(0, 1, 1);
   pins[0.50] = Vector(0, 1, 0);
   pins[0.75] = Vector(1, 1, 0);
   pins[1.00] = Vector(1, 0, 0);
   transferFunctions[ANSYS] = pins;
   pins.clear();

   pins[0.00] = Vector(0.231, 0.298, 0.752);
   pins[0.25] = Vector(0.552, 0.690, 0.996);
   pins[0.50] = Vector(0.866, 0.866, 0.866);
   pins[0.75] = Vector(0.956, 0.603, 0.486);
   pins[1.00] = Vector(0.705, 0.015, 0.149);
   transferFunctions[CoolWarm] = pins;
   pins.clear();

   pins[0.00] = Vector(1,133,113)/255.;
   pins[0.25] = Vector(128,205,193)/255.;
   pins[0.50] = Vector(245,245,245)/255.;
   pins[0.75] = Vector(223,194,125)/255.;
   pins[1.00] = Vector(166, 97, 26)/255.;
   transferFunctions[Frosty] = pins;
   pins.clear();

   pins[0.00] = Vector(77,172,38)/255.;
   pins[0.25] = Vector(184,225,134)/255.;
   pins[0.50] = Vector(247,247,247)/255.;
   pins[0.75] = Vector(241,182,218)/255.;
   pins[1.00] = Vector(208,28,139)/255.;
   transferFunctions[Dolomiti] = pins;
   pins.clear();

   pins[0.00] = Vector(44,123,182)/255.;
   pins[0.25] = Vector(171,217,233)/255.;
   pins[0.50] = Vector(255,255,191)/255.;
   pins[0.75] = Vector(253,174,97)/255.;
   pins[1.00] = Vector(215,25,28)/255.;
   transferFunctions[CoolWarmBrewer] = pins;
   pins.clear();

   pins[0.00] = Vector(0.00, 0.00, 0.35);
   pins[0.05] = Vector(0.00, 0.00, 1.00);
   pins[0.26] = Vector(0.00, 1.00, 1.00);
   pins[0.50] = Vector(0.00, 0.00, 1.00);
   pins[0.74] = Vector(1.00, 1.00, 0.00);
   pins[0.95] = Vector(1.00, 0.00, 0.00);
   pins[1.00] = Vector(0.40, 0.00, 0.00);
   transferFunctions[ITSM] = pins;
   pins.clear();

   pins[0.0] = Vector(0.40, 0.00, 0.40);
   pins[0.2] = Vector(0.00, 0.00, 1.00);
   pins[0.4] = Vector(0.00, 1.00, 1.00);
   pins[0.6] = Vector(0.00, 1.00, 0.00);
   pins[0.8] = Vector(1.00, 1.00, 0.00);
   pins[1.0] = Vector(1.00, 0.00, 0.00);
   transferFunctions[Rainbow] = pins;
   pins.clear();
}

Color::~Color() {

}

void Color::getMinMax(vistle::DataBase::const_ptr object,
                      vistle::Scalar & min, vistle::Scalar & max) {

   size_t numElements = object->getSize();

   if (Vec<Index>::const_ptr scal = Vec<Index>::as(object)) {
      const vistle::Index *x = &scal->x()[0];
#pragma omp parallel
      {
         Index tmin = std::numeric_limits<Index>::max();
         Index tmax = std::numeric_limits<Index>::min();
#pragma omp for
         for (ssize_t index = 0; index < numElements; index ++) {
            if (x[index] < tmin)
               tmin = x[index];
            if (x[index] > tmax)
               tmax = x[index];
         }
#pragma omp critical
         {
            if (tmin < min)
               min = tmin;
            if (tmax > max)
               max = tmax;
         }
      }
   } else  if (Vec<Scalar>::const_ptr scal = Vec<Scalar>::as(object)) {
      const vistle::Scalar *x = &scal->x()[0];
#pragma omp parallel
      {
         Scalar tmin = std::numeric_limits<Scalar>::max();
         Scalar tmax = -std::numeric_limits<Scalar>::max();
#pragma omp for
         for (ssize_t index = 0; index < numElements; index ++) {
            if (x[index] < tmin)
               tmin = x[index];
            if (x[index] > tmax)
               tmax = x[index];
         }
#pragma omp critical
         {
            if (tmin < min)
               min = tmin;
            if (tmax > max)
               max = tmax;
         }
      }
   } else  if (Vec<Scalar,3>::const_ptr vec = Vec<Scalar,3>::as(object)) {
      const vistle::Scalar *x = &vec->x()[0];
      const vistle::Scalar *y = &vec->y()[0];
      const vistle::Scalar *z = &vec->z()[0];
#pragma omp parallel
      {
         Scalar tmin = std::numeric_limits<Scalar>::max();
         Scalar tmax = -std::numeric_limits<Scalar>::max();
#pragma omp for
         for (ssize_t index = 0; index < numElements; index ++) {
            Scalar v = Vector(x[index], y[index], z[index]).norm();
            if (v < tmin)
               tmin = v;
            if (v > tmax)
               tmax = v;
         }
#pragma omp critical
         {
            if (tmin < min)
               min = tmin;
            if (tmax > max)
               max = tmax;
         }
      }
   }
}

void Color::binData(vistle::DataBase::const_ptr object, std::vector<unsigned long> &binsVec) {

   const int numBins = binsVec.size();

   const size_t numElements = object->getSize();
   const Scalar w = m_max-m_min;
   unsigned long *bins = binsVec.data();

   if (Vec<Index>::const_ptr scal = Vec<Index>::as(object)) {
      const vistle::Index *x = &scal->x()[0];
      for (ssize_t index = 0; index < numElements; index ++) {
          const int bin = clamp<int>((x[index]-m_min)/w*numBins, 0, numBins-1);
          ++bins[bin];
      }
   } else  if (Vec<Scalar>::const_ptr scal = Vec<Scalar>::as(object)) {
      const vistle::Scalar *x = &scal->x()[0];
      for (ssize_t index = 0; index < numElements; index ++) {
          const int bin = clamp<int>((x[index]-m_min)/w*numBins, 0, numBins-1);
          ++bins[bin];
      }
   } else  if (Vec<Scalar,3>::const_ptr vec = Vec<Scalar,3>::as(object)) {
      const vistle::Scalar *x = &vec->x()[0];
      const vistle::Scalar *y = &vec->y()[0];
      const vistle::Scalar *z = &vec->z()[0];
      for (ssize_t index = 0; index < numElements; index ++) {
          const Scalar v = Vector(x[index], y[index], z[index]).norm();
          const int bin = clamp<int>((v-m_min)/w*numBins, 0, numBins-1);
          ++bins[bin];
      }
   }
}


bool Color::changeParameter(const Parameter *p) {

    bool changeReduce = false;
    if (p == m_autoRangePara) {
        m_autoRange = m_autoRangePara->getValue();
        changeReduce = true;
    } else if (p == m_autoInsetCenterPara) {
        m_autoInsetCenter = m_autoInsetCenterPara->getValue();
        changeReduce = true;
    } else if (p == m_nestPara) {
        m_nest = m_nestPara->getValue();
        changeReduce = true;
    }

    if (changeReduce) {
        if (m_autoRange || (m_nest && m_autoInsetCenter)) {
            setReducePolicy(message::ReducePolicy::OverAll);
        } else {
            setReducePolicy(message::ReducePolicy::Locally);
        }
    }

    return true;
}

vistle::Texture1D::ptr Color::addTexture(vistle::DataBase::const_ptr object,
      const vistle::Scalar min, const vistle::Scalar max,
      const ColorMap & cmap) {

   const Scalar invRange = 1.f / (max - min);

   vistle::Texture1D::ptr tex(new vistle::Texture1D(cmap.width, min, max));
   unsigned char *pix = &tex->pixels()[0];
   for (size_t index = 0; index < cmap.width * 4; index ++)
      pix[index] = cmap.data[index];

   const size_t numElem = object->getSize();
   tex->coords().resize(numElem);
   auto tc = tex->coords().data();

   if (Vec<Scalar>::const_ptr f = Vec<Scalar>::as(object)) {

      const vistle::Scalar *x = &f->x()[0];

#pragma omp parallel for
      for (ssize_t index = 0; index < numElem; index ++)
         tc[index] = (x[index] - min) * invRange;
   } else if (Vec<Index>::const_ptr f = Vec<Index>::as(object)) {

      const vistle::Index *x = &f->x()[0];

#pragma omp parallel for
      for (ssize_t index = 0; index < numElem; index ++)
         tc[index] = (x[index] - min) * invRange;
   } else  if (Vec<Scalar,3>::const_ptr f = Vec<Scalar,3>::as(object)) {

      const vistle::Scalar *x = &f->x()[0];
      const vistle::Scalar *y = &f->y()[0];
      const vistle::Scalar *z = &f->z()[0];

#pragma omp parallel for
      for (ssize_t index = 0; index < numElem; index ++) {
         const Scalar v = Vector(x[index], y[index], z[index]).norm();
         tc[index] = (v - min) * invRange;
      }
   } else {
       std::cerr << "Color: cannot handle input of type " << object->getType() << std::endl;

#pragma omp parallel for
      for (ssize_t index = 0; index < numElem; index ++) {
         tc[index] = 0.;
      }
   }

   return tex;
}

void Color::computeMap() {

   auto pins = transferFunctions[getIntParameter("map")];
   if (pins.empty()) {
       pins = transferFunctions[COVISE];
   }
   int steps = getIntParameter("steps");
   int resolution = steps;
   bool relative = getIntParameter("inset_relative");
   if (m_nest) {
       resolution = getIntParameter("resolution");
       double width = getFloatParameter("inset_width");
       int inset_steps = getIntParameter("inset_steps");
       if (inset_steps <= 0)
           inset_steps = steps;
       if (relative && width > 0.) {
           int res2 = inset_steps/width;
           if (resolution < res2)
               resolution = std::min(res2, 0x1000);
       }
   }
   //std::cerr << "computing color map with " << steps << " steps and a resolution of " << resolution << std::endl;
   m_colors.reset(new ColorMap(pins, steps, resolution));

   if (m_nest) {
       auto inset_pins = transferFunctions[getIntParameter("inset_map")];
       if (inset_pins.empty()) {
           inset_pins = transferFunctions[COVISE];
       }
       int inset_steps = getIntParameter("inset_steps");
       if (inset_steps <= 0)
           inset_steps = steps;
       double width = getFloatParameter("inset_width");
       double center = getFloatParameter("inset_center");
       if (!relative) {
           width /= m_max-m_min;
           center = (center-m_min)/(m_max-m_min);
       }
       int insetStart = clamp(center-0.5*width, 0., 1.) * (resolution-1);
       int insetEnd = clamp(center+0.5*width, 0., 1.) * (resolution-1);
       int insetRes = insetEnd - insetStart + 1;
       assert(insetEnd >= insetStart);
       assert(insetStart >= 0);
       assert(insetEnd < m_colors->width);
       ColorMap inset(inset_pins, inset_steps, insetRes);
       for (int i=insetStart; i<=insetEnd; ++i) {
           for (int c=0; c<4; ++c)
               m_colors->data[i*4+c] = inset.data[(i-insetStart)*4+c];
       }
   }
}

bool Color::prepare() {

   m_min = std::numeric_limits<Scalar>::max();
   m_max = -std::numeric_limits<Scalar>::max();

   if (!m_autoRange) {
      m_min = getFloatParameter("min");
      m_max = getFloatParameter("max");
      if (m_min >= m_max)
          m_max = m_min + 1.;
   }
   m_inputQueue.clear();

   computeMap();

   bool preview = getIntParameter("preview");
   if (m_autoRange || (m_nest && m_autoInsetCenter)) {
       if (preview)
           startIteration();
   }

   return true;
}

bool Color::compute() {

   Coords::const_ptr coords = accept<Coords>("data_in");
   if (coords) {
      passThroughObject("data_out", coords);
   }
   DataBase::const_ptr data = accept<DataBase>("data_in");
   if (!data) {
      Object::const_ptr obj = accept<Object>("data_in");
      passThroughObject("data_out", obj);
      return true;
   }

   bool preview = getIntParameter("preview");
   if (m_autoRange) {
       getMinMax(data, m_min, m_max);
       m_inputQueue.push_back(data);
       if (preview)
           process(data);
   } else if (m_nest && m_autoInsetCenter) {
       m_inputQueue.push_back(data);
       if (preview)
           process(data);
   } else {
       process(data);
   }

   return true;
}

bool Color::reduce(int timestep) {

    assert(timestep == -1);
    bool preview = getIntParameter("preview");

    if (m_autoRange) {
        m_min = boost::mpi::all_reduce(comm(), m_min, boost::mpi::minimum<Scalar>());
        m_max = boost::mpi::all_reduce(comm(), m_max, boost::mpi::maximum<Scalar>());
        setFloatParameter("min", m_min);
        setFloatParameter("max", m_max);
    } else {
        m_min = getFloatParameter("min");
        m_max = getFloatParameter("max");
    }

    if (m_nest && m_autoInsetCenter) {
        std::vector<unsigned long> bins(getIntParameter("resolution"));
        for (auto data: m_inputQueue) {
            binData(data, bins);
        }
        for (size_t i=0; i<bins.size(); ++i) {
            bins[i] = boost::mpi::all_reduce(comm(), bins[i], std::plus<unsigned long>());
        }

        bool relative = getIntParameter("inset_relative");
        double width = getFloatParameter("inset_width");
        if (!relative) {
            width /= m_max-m_min;
        }
        int insetRes = width*bins.size();
        unsigned long numEnt=0;
        for (size_t i=0; i<insetRes; ++i) {
            numEnt += bins[i];
        }
        unsigned long maxEnt = numEnt;
        size_t maxIdx = 0;
        for (size_t i=0; i<bins.size()-insetRes; ++i) {
            numEnt -= bins[i];
            numEnt += bins[i+insetRes];
            if (numEnt >= maxEnt) {
                maxIdx = i+1;
                maxEnt = numEnt;
            }
        }
        if (bins.size() > 1) {
            if (relative) {
                setFloatParameter("inset_center", (maxIdx+0.5*insetRes)/(bins.size()-1));
            } else {
                setFloatParameter("inset_center", (maxIdx+0.5*insetRes)/(bins.size()-1)*(m_max-m_min)+m_min);
            }
        }
    }

    if (m_autoRange || (m_nest && m_autoInsetCenter)) {
        computeMap();
        if (preview)
            startIteration();
    }

    while(!m_inputQueue.empty()) {
        if (cancelRequested())
            break;

        auto data = m_inputQueue.front();
        m_inputQueue.pop_front();
        process(data);
    }

    return true;
}

void Color::process(const DataBase::const_ptr data, int iteration) {

   auto out(addTexture(data, m_min, m_max, *m_colors));
   out->setGrid(data->grid());
   out->setMeta(data->meta());
   out->copyAttributes(data);
   out->setIteration(iteration);

   addObject("data_out", out);
}

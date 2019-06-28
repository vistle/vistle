#ifndef COLOR_H
#define COLOR_H

#include <module/module.h>
#include <core/vector.h>
#include <core/texture1d.h>

#include <deque>

class ColorMap {

public:
   ColorMap(std::map<vistle::Scalar, vistle::Vector> & pins, const size_t steps, const size_t width);
   ~ColorMap();

   unsigned char *data;
   const size_t width;
};

class Color: public vistle::Module {

 public:
   Color(const std::string &name, int moduleID, mpi::communicator comm);
   ~Color();

 private:
   vistle::Texture1D::ptr addTexture(vistle::DataBase::const_ptr object,
                               const vistle::Scalar min, const vistle::Scalar max,
                               const ColorMap & cmap);

   void getMinMax(vistle::DataBase::const_ptr object, vistle::Scalar & min, vistle::Scalar & max);
   void binData(vistle::DataBase::const_ptr object, std::vector<unsigned long> &binsVec);
   void computeMap();
   void sendColorMap();

   bool changeParameter(const vistle::Parameter *p) override;
   bool prepare() override;
   bool compute() override;
   bool reduce(int timestep) override;
   void connectionAdded(const vistle::Port *from, const vistle::Port *to) override;

   void process(const vistle::DataBase::const_ptr data);

   typedef std::map<vistle::Scalar, vistle::Vector> TF;
   std::map<int, TF> transferFunctions;

   std::shared_ptr<ColorMap> m_colors;

   bool m_autoRange = true, m_autoInsetCenter = true, m_nest = false;
   vistle::IntParameter *m_autoRangePara, *m_autoInsetCenterPara, *m_nestPara;
   vistle::FloatParameter *m_minPara = nullptr, *m_maxPara = nullptr;
   std::deque<vistle::DataBase::const_ptr> m_inputQueue;

   vistle::Scalar m_min, m_max;

   std::string m_species;
   bool m_colorMapSent = false;
   vistle::Port *m_dataIn = nullptr;
   vistle::Port *m_dataOut = nullptr, *m_colorOut = nullptr;
};

#endif

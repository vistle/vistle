#ifndef READIAGTECPLOT_H
#define READIAGTECPLOT_H

#include <module/module.h>

#include "tecplotfile.h"

class ReadIagTecplot: public vistle::Module {

 public:
   ReadIagTecplot(const std::string &name, int moduleID, mpi::communicator comm);
   ~ReadIagTecplot();

 private:
   static const int NumPorts = 3;

   bool changeParameter(const vistle::Parameter *p) override;
   bool prepare() override;
   void setChoices();

   vistle::StringParameter *m_filename;
   vistle::StringParameter *m_cellDataChoice[NumPorts], *m_pointDataChoice[NumPorts];
   vistle::Port *m_cellPort[NumPorts], *m_pointPort[NumPorts];

   std::shared_ptr<TecplotFile> m_tecplot;
};
#endif

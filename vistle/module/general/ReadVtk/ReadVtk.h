#ifndef READVTK_H
#define READVTK_H

#include <module/module.h>

class vtkDataSet;

class ReadVtk: public vistle::Module {

 public:
   ReadVtk(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadVtk();

 private:
   static const int NumPorts = 3;

   bool parameterChanged(const vistle::Parameter *p) override;
   bool compute() override;

   void setChoices(vtkDataSet *ds);

   vistle::StringParameter *m_filename;
   vistle::StringParameter *m_cellDataChoice[NumPorts], *m_pointDataChoice[NumPorts];
   vistle::Port *m_cellPort[NumPorts], *m_pointPort[NumPorts];
};

#endif

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

   bool changeParameter(const vistle::Parameter *p) override;
   bool compute() override;

   bool load(const std::string &filename, const vistle::Meta &meta = vistle::Meta(), int piece=-1, bool ghost=false, const std::string &part=std::string());
   void setChoices(vtkDataSet *ds);

   vistle::StringParameter *m_filename;
   vistle::StringParameter *m_cellDataChoice[NumPorts], *m_pointDataChoice[NumPorts];
   vistle::Port *m_cellPort[NumPorts], *m_pointPort[NumPorts];
   vistle::IntParameter *m_readPieces, *m_ghostCells;
};

#endif

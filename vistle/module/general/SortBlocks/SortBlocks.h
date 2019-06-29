#ifndef SORTBLOCKS_H
#define SORTBLOCKS_H

#include <module/module.h>
#include <core/vector.h>
#include <core/vec.h>

class SortBlocks: public vistle::Module {

 public:
   SortBlocks(const std::string &name, int moduleID, mpi::communicator comm);
   ~SortBlocks();

 private:
   virtual bool compute() override;
   virtual bool changeParameter(const vistle::Parameter *p) override;
   vistle::IntParameter *m_criterionParam = nullptr;
   vistle::IntParameter *m_minParam = nullptr;
   vistle::IntParameter *m_maxParam = nullptr;
   vistle::IntParameter *m_modulusParam = nullptr;
   vistle::IntParameter *m_invertParam = nullptr;
};

#endif

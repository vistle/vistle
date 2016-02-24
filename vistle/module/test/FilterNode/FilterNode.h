#ifndef FILTERNODE_H
#define FILTERNODE_H

#include <module/module.h>
#include <core/vector.h>
#include <core/vec.h>

class FilterNode: public vistle::Module {

 public:
   FilterNode(const std::string &shmname, const std::string &name, int moduleID);
   ~FilterNode();

 private:
   virtual bool compute() override;
   virtual bool parameterChanged(const vistle::Parameter *p) override;
   vistle::IntParameter *m_criterionParam;
   vistle::IntParameter *m_nodeParam;
   vistle::IntParameter *m_invertParam;
};

#endif

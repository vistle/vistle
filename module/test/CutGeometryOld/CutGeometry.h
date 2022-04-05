#ifndef CUTGEOMETRY_H
#define CUTGEOMETRY_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include "../../general/IsoSurface/IsoDataFunctor.h"

class CutGeometry: public vistle::Module {
public:
    CutGeometry(const std::string &name, int moduleID, mpi::communicator comm);
    ~CutGeometry();

    vistle::Object::ptr cutGeometry(vistle::Object::const_ptr object) const;

private:
    bool compute(std::shared_ptr<vistle::BlockTask> task) const override;
    virtual bool changeParameter(const vistle::Parameter *param) override;
    IsoController isocontrol;
};

#endif

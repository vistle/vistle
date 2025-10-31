#ifndef VISTLE_CUTGEOMETRY_CUTGEOMETRY_H
#define VISTLE_CUTGEOMETRY_CUTGEOMETRY_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include "../../map/IsoSurface/IsoDataFunctor.h"

class CutGeometry: public vistle::Module {
public:
    CutGeometry(const std::string &name, int moduleID, mpi::communicator comm);

    vistle::Object::ptr cutGeometry(vistle::Object::const_ptr object) const;

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    bool changeParameter(const vistle::Parameter *param) override;
    IsoController isocontrol;
};

#endif

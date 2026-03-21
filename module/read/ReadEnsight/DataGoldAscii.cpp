// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                                  (C)2003 VirCinity  ++
// ++ Description:                                                        ++
// ++             Implementation of class DataGoldAscii                    ++
// ++                                                                     ++
// ++ Author:  Ralf Mikulla (rm@vircinity.com)                            ++
// ++                                                                     ++
// ++               VirCinity GmbH                                        ++
// ++               Nobelstrasse 15                                       ++
// ++               70569 Stuttgart                                       ++
// ++                                                                     ++
// ++ Date: 07.04.2003                                                    ++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "DataGoldAscii.h"
#include "ReadEnsight.h"

#include <vector>
#include <vistle/core/vec.h>

using namespace vistle;

#define CERR std::cerr << "DataGoldAscii::" << __func__ << ": " << where() << ": "

DataGoldAscii::DataGoldAscii(ReadEnsight *mod, const std::string &name, int dim, bool perVertex)
: EnFile(mod, name, dim, CaseFile::NOBIN), lineCnt_(0), perVertex_(perVertex)
{}

bool DataGoldAscii::parseForParts()
{
    //cerr << "DataGoldAscii::read() called" << std::endl;

    if (!isOpen()) {
        return false;
    }
    auto &in = in_;

    char buf[lineLen];
    // 1 lines description - ignore it
    fgets(buf, lineLen, in);
    ++lineCnt_;

    if (perVertex_) {
        size_t id(0);
        int actPartNr = -1;
        EnPart *actPart = nullptr;
        size_t numVal = 0;

        while (!feof(in)) {
            ssize_t fpos = file::tell(in);
            fgets(buf, lineLen, in);
            ++lineCnt_;
            std::string tmp(buf);

            // we found a part line
            id = tmp.find("part");
            if (id != std::string::npos) {
                ssize_t partStart = fpos;
                // part line found   -- we skip it for the moment --
                fgets(buf, lineLen, in);
                ++lineCnt_;
                actPartNr = atol(buf);

                //cerr << "DataGoldAscii::read( ) part " <<  actPartNr << std::endl;
                actPart = findPart(actPartNr);
                if (!actPart) {
                    CERR << "part " << actPartNr << " not found" << std::endl;
                    return false;
                }
                actPart->setStartPos(partStart, name());
                CERR << "part " << actPartNr << " found: " << *actPart << ", cur @ " << partStart << std::endl;

                // allocate memory for whole parts
                numVal = actPart->numCoords();
            }

            // coordinates -line
            id = tmp.find("coordinates");
            if (id != std::string::npos) {
                numVal *= dim_;
                for (size_t i = 0; i < numVal; ++i) {
                    fgets(buf, lineLen, in);
                    ++lineCnt_;
                }
            }
        }

        return true;
    }

    size_t id(0);
    EnPart *actPart(nullptr);
    int actPartNr = -1;

    while (!feof(in)) {
        ssize_t fpos = file::tell(in);
        fgets(buf, lineLen, in);
        ++lineCnt_;
        std::string tmp(buf);

        // we found a part line
        id = tmp.find("part");
        if (id != std::string::npos) {
            ssize_t partStart = fpos;
            // part line found   -- we skip it for the moment --
            // length of "part" +1
            fgets(buf, lineLen, in);
            ++lineCnt_;
            actPartNr = atol(buf);
            actPart = findPart(actPartNr);
            if (!actPart) {
                CERR << "part " << actPartNr << " not found" << std::endl;
                return false;
            }
            actPart->setStartPos(partStart, name());
            // allocate memory for whole parts
        }

        // coordinates -line
        id = tmp.find("coordinates");
        if (id == std::string::npos) {
            size_t numEle = actPart->getNumEle();
            // skip data
            while (numEle > 0) {
                fgets(buf, lineLen, in);
                ++lineCnt_;
                std::string elementType(buf);
                EnElement elem(elementType);
                // we have a valid EnSight element
                if (elem.valid()) {
                    fgets(buf, lineLen, in);
                    ++lineCnt_;
                }
                numEle--;
            }
        }
    }

    return true;
}

vistle::Object::ptr DataGoldAscii::read(int timestep, int block, EnPart *part)
{
    //cerr << "DataGoldAscii::read() called" << std::endl;
    Object::ptr result;
    auto in = open();
    if (!in) {
        return result;
    }

    vistle::Vec<Scalar>::ptr scal;
    vistle::Vec<Scalar, 3>::ptr vec3;

    std::array<vistle::Scalar *, 3> arr{nullptr, nullptr, nullptr};
    auto setNumVals = [this, &scal, &vec3, &arr, &result](size_t numVals) {
        if (dim_ == 1) {
            scal = std::make_shared<Vec<Scalar, 1>>(numVals);
            result = scal;
            arr[0] = scal->x().data();
        } else if (dim_ == 3) {
            vec3 = std::make_shared<Vec<Scalar, 3>>(numVals);
            result = vec3;
            arr[0] = vec3->x().data();
            arr[1] = vec3->y().data();
            arr[2] = vec3->z().data();
        }
    };

    int partToRead = part->getPartNum();


    file::seek(in, part->startPos(name()), SEEK_SET);

    char buf[lineLen];
    if (perVertex_) {
        float val;
        size_t id(0);
        int actPartNr = -1;
        size_t numVal = 0;

        while (!feof(in)) {
            fgets(buf, lineLen, in);
            ++lineCnt_;
            std::string tmp(buf);

            // we found a part line
            id = tmp.find("part");
            if (id != std::string::npos) {
                // part line found   -- we skip it for the moment --
                fgets(buf, lineLen, in);
                ++lineCnt_;
                actPartNr = atol(buf);
                if (actPartNr != partToRead) {
                    //CERR << "part " << actPartNr << ", but reading for " << partToRead << ", returning" << std::endl;
                    return result;
                }

                numVal = part->numCoords();
                setNumVals(numVal);
            }

            // coordinates -line
            id = tmp.find("coordinates");
            if (id != std::string::npos) {
                switch (dim_) {
                case 1:
                    for (size_t i = 0; i < numVal; ++i) {
                        // we have data
                        fgets(buf, lineLen, in);
                        ++lineCnt_;
                        sscanf(buf, "%e", &val);
                        arr[0][i] = val;
                    }
                    break;
                case 3:
                    for (size_t i = 0; i < numVal; ++i) {
                        // we have data
                        fgets(buf, lineLen, in);
                        ++lineCnt_;
                        sscanf(buf, "%e", &val);
                        arr[0][i] = val;
                    }
                    for (size_t i = 0; i < numVal; ++i) {
                        // we have data
                        fgets(buf, lineLen, in);
                        ++lineCnt_;
                        sscanf(buf, "%e", &val);
                        arr[1][i] = val;
                    }
                    for (size_t i = 0; i < numVal; ++i) {
                        // we have data
                        fgets(buf, lineLen, in);
                        ++lineCnt_;
                        sscanf(buf, "%e", &val);
                        arr[2][i] = val;
                    }
                    break;
                }
            }
        }
        return result;
    }

    float val;
    size_t id(0);
    int actPartNr = -1;
    size_t eleCnt2d = 0, eleCnt3d = 0;

    while (!feof(in)) {
        fgets(buf, lineLen, in);
        ++lineCnt_;
        std::string tmp(buf);

        // we found a part line
        id = tmp.find("part");
        if (id != std::string::npos) {
            // part line found   -- we skip it for the moment --
            // length of "part" +1
            fgets(buf, lineLen, in);
            ++lineCnt_;
            actPartNr = atol(buf);
            if (actPartNr != partToRead) {
                return result;
            }
            eleCnt2d = 0;
            eleCnt3d = 0;
            // allocate memory for whole parts

            //int numCells( actPart->getTotNumEle() );
            switch (dim_) {
            case 1:
            case 3:
                // create mem for 2d/3d data
                if (part->numEleRead3d() > 0) {
                    setNumVals(part->numEleRead3d() + 1);
                } else if (part->numEleRead2d() > 0) {
                    setNumVals(part->numEleRead2d() + 1); // FIXME
                }
                break;
            }
        }

        std::stringstream err;
        err << "in  block " << block << " timestep " << timestep << " on part " << part->comment() << ": ";

        // coordinates -line
        id = tmp.find("coordinates");
        if (id == std::string::npos) {
            std::string elementType(tmp);
            EnElement elem(elementType);

            // we have a valid EnSight element
            if (!elem.valid()) {
                err << "unknown element type " << elementType;
                ens->sendError(err.str());
                result.reset();
                return result;
            }

            size_t anzEle = part->getElementNum(elem.getEnType());
            //CERR << anzEle << " for " << elementType  << std::endl;
            const auto *thisEle = part->findElementType(elem.getEnType());
            if (!thisEle) {
                err << "element not found for type " << elem.getEnType();
                ens->sendError(err.str());
                result.reset();
                return result;
            }
            const auto &bl = thisEle->getBlanklist();
            if (bl.size() != anzEle) {
                err << "blanklist size " << bl.size() << " does not match number of elements " << anzEle
                    << " for element type " << elementType;
                ens->sendError(err.str());
                result.reset();
                return result;
            }

            switch (dim_) {
            case 1:
            case 3: {
                size_t num2d = eleCnt2d, num3d = eleCnt3d;
                for (int j = 0; j < dim_; j++) {
                    num2d = eleCnt2d;
                    num3d = eleCnt3d;
                    for (size_t i = 0; i < anzEle; ++i) {
                        fgets(buf, lineLen, in);
                        ++lineCnt_;
                        sscanf(buf, "%e", &val);
                        if (bl[i] > 0) {
                            if (thisEle->getDim() == EnElement::D2) {
                                arr[j][num2d] = val;
                                ++num2d;
                            } else if (thisEle->getDim() == EnElement::D3) {
                                arr[j][num3d] = val;
                                ++num3d;
                            }
                        }
                    }
                }
                eleCnt2d = num2d;
                eleCnt3d = num3d;
                break;
            }
            }
        }
    }

    return result;
}

//
// Destructor
//
DataGoldAscii::~DataGoldAscii()
{}

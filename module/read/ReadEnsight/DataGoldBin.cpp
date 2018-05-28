// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                                  (C)2003 VirCinity  ++
// ++ Description:                                                        ++
// ++             Implementation of class DataGoldBin                    ++
// ++                                                                     ++
// ++ Author:  Ralf Mikulla (rm@vircinity.com)                            ++
// ++                                                                     ++
// ++               VirCinity GmbH                                        ++
// ++               Nobelstrasse 15                                       ++
// ++               70569 Stuttgart                                       ++
// ++                                                                     ++
// ++ Date: 07.04.2003                                                    ++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "DataGoldBin.h"
#include "ReadEnsight.h"
#include "ByteSwap.h"

#include <vistle/core/vec.h>
#include <vistle/core/indexed.h>

using namespace vistle;

#define CERR std::cerr << "DataGoldBin::" << __func__ << ": "

//
// Constructor
//
DataGoldBin::DataGoldBin(ReadEnsight *mod, const std::string &name, int dim, bool perVertex, CaseFile::BinType binType)
: EnFile(mod, name, dim, binType), perVertex_(perVertex)
{
    byteSwap_ = mod->byteSwap();
}

bool DataGoldBin::parseForParts()
{
    if (!isOpen()) {
        return false;
    }
    auto &in = in_;

    if (perVertex_) {
        while (!feof(in)) {
            std::string tmp(getStr(in));
            // we only read the first time step
            if (tmp.find("END TIME STEP") != std::string::npos)
                break;
            // any text including the word "part" will disturb the passing
            // work around for particles
            if (tmp.find("particles") != std::string::npos) {
                tmp = getStr(in);
            }
            // we found a part line
            size_t id = tmp.find("part");
            if (id != std::string::npos) {
                long partStart = filePos();
                // part line found   -- we skip it for the moment --
                int actPartNr = getInt(in);
                auto actPart = findPart(actPartNr);
                if (!actPart) {
                    CERR << "part " << actPartNr << " not found" << std::endl;
                    return false;
                }
                actPart->setStartPos(partStart, name());
                size_t numCoord = actPart->numCoords();
                tmp = getStr(in);
                id = tmp.find("coordinates");
                if (id != std::string::npos) {
                    for (unsigned d = 0; d < dim_; ++d) {
                        skipFloat(in, numCoord);
                    }
                }
            }
        }
        return true;
    }


    // 1 lines description - ignore it
    std::string currentLine(getStr(in));

    int actPartNr = -1;
    EnPart *actPart = nullptr;
    while (!feof(in)) // read all timesteps
    {
        size_t tt = currentLine.find("END TIME STEP");
        if (tt != std::string::npos) {
            currentLine = getStr(in); // read description
            break;
        }
        tt = currentLine.find("BEGIN TIME STEP");
        if (tt != std::string::npos) {
            currentLine = getStr(in); // read description
        }

        currentLine = getStr(in); // this should be the part line or an element name
        size_t id = currentLine.find("part");
        if (id != std::string::npos) {
            long partStart = filePos();
            // part line found   -- we skip it for the moment --
            // length of "part" +1
            actPartNr = getInt(in);
            actPart = findPart(actPartNr);
            if (!actPart) {
                CERR << "part " << actPartNr << " not found" << std::endl;
                return false;
            }
            actPart->setStartPos(partStart, name());

            currentLine = getStr(in); // this should be the elementName now
            CERR << "elem part " << actPartNr << " found, currentLine=" << currentLine << std::endl;
        }

        std::string elementType(currentLine);
        EnElement elem(elementType);
        if (elem.valid()) {
            // we have a valid EnSight element
            auto anzEle = actPart->getElementNum(elem.getEnType());
            switch (dim_) {
            case 1:
                // scalar data
                skipFloat(in, anzEle);
                break;
            case 3:
                // 3-dim vector data
                skipFloat(in, anzEle);
                skipFloat(in, anzEle);
                skipFloat(in, anzEle);
                break;
            }
        } else if (currentLine.empty() && feof(in)) {
            break;
        } else {
            CERR << "unknown element type " << elementType << std::endl;
            return false;
        }
    }

    return true;
}

vistle::Object::ptr DataGoldBin::read(int timestep, int block, EnPart *part)
{
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
    fseek(in, part->startPos(name()), SEEK_SET);

    if (perVertex_) {
        while (!feof(in)) {
            std::string tmp(getStr(in));
            // we only read the first time step
            if (tmp.find("END TIME STEP") != std::string::npos)
                break;
            // any text including the word "part" will disturb the passing
            // work around for particles
            if (tmp.find("particles") != std::string::npos) {
                tmp = getStr(in);
            }
            // we found a part line
            size_t id = tmp.find("part");
            if (id != std::string::npos) {
                // part line found   -- we skip it for the moment --
                int actPartNr = getInt(in);
                if (actPartNr != partToRead) {
                    //result.reset();
                    return result;
                }
                size_t numCoord = part->numCoords();
                tmp = getStr(in);
                id = tmp.find("coordinates");
                if (id != std::string::npos) {
                    setNumVals(numCoord);
                    CERR << "found part to read with " << numCoord << " values" << std::endl;
                    // coordinates -line
                    for (unsigned d = 0; d < dim_; ++d) {
                        getFloatArr(in, numCoord, arr[d]);
                    }
                    break;
                }
            }
        }
        return result;
    }


    // per-element data
    size_t eleCnt2d = 0, eleCnt3d = 0;

    std::string currentLine(getStr(in));
    int actPartNr = -1;
    while (!feof(in)) // read all timesteps
    {
        size_t tt = currentLine.find("END TIME STEP");
        if (tt != std::string::npos) {
            currentLine = getStr(in); // read description
            break;
        }
        tt = currentLine.find("BEGIN TIME STEP");
        if (tt != std::string::npos) {
            currentLine = getStr(in); // read description
        }

        size_t id = currentLine.find("part");
        if (id != std::string::npos) {
            // part line found   -- we skip it for the moment --
            // length of "part" +1
            actPartNr = getInt(in);
            if (actPartNr != partToRead) {
                //result.reset();
                return result;
            }

            // allocate memory for whole parts
            setNumVals(part->getTotNumEle());
            eleCnt2d = 0;
            eleCnt3d = 0;

            currentLine = getStr(in); // this should be the elementName now
            CERR << "elem part " << actPartNr << " found, currentLine=" << currentLine << std::endl;
        }

        std::string elementType(currentLine);
        EnElement elem(elementType);
        if (elem.valid()) {
            // we have a valid EnSight element
            auto anzEle = part->getElementNum(elem.getEnType());
            //cerr << "DataGoldBin::readCells() " << anzEle << " for " << elementType  << std::endl;
            EnElement thisEle = part->findElementType(elem.getEnType());
            auto &bl = thisEle.getBlanklist();
            if (thisEle.getBlanklist().size() != anzEle) {
                CERR << "blanklist size problem " << bl.size() << std::endl;
            }

            switch (dim_) {
            case 1: {
                auto tArr1 = new vistle::Scalar[anzEle];
                CERR << "read " << anzEle << " 1D values for " << elementType << std::endl;
                // scalar data
                getFloatArr(in, anzEle, tArr1);
                if (thisEle.getDim() == EnElement::D2) {
                    for (size_t i = 0; i < anzEle; ++i) {
                        if (bl[i] > 0) {
                            arr[0][eleCnt2d] = tArr1[i];
                            ++eleCnt2d;
                        }
                    }
                } else if (thisEle.getDim() == EnElement::D3) {
                    for (size_t i = 0; i < anzEle; ++i) {
                        if (bl[i] > 0) {
                            arr[0][eleCnt3d] = tArr1[i];
                            ++eleCnt3d;
                        }
                    }
                }
                // 			cerr << " EleCnt2d: " << eleCnt2d;
                // 			cerr << " ACT-PART numEleRead2d " <<  part->numEleRead2d();
                // 			cerr << std::endl;

                // 			cerr << " EleCnt3d: " << eleCnt3d;
                // 			cerr << " ACT-PART numEleRead3d " <<  part->numEleRead3d();
                // 			cerr << std::endl;
                delete[] tArr1;
                break;
            }
            case 3: {
                auto tArr1 = new vistle::Scalar[anzEle];
                auto tArr2 = new vistle::Scalar[anzEle];
                auto tArr3 = new vistle::Scalar[anzEle];
                CERR << "read " << anzEle << " 3D values for " << elementType << std::endl;
                // 3-dim vector data
                getFloatArr(in, anzEle, tArr1);
                getFloatArr(in, anzEle, tArr2);
                getFloatArr(in, anzEle, tArr3);
                if (thisEle.getDim() == EnElement::D2) {
                    for (size_t i = 0; i < anzEle; ++i) {
                        if (bl[i] > 0) {
                            arr[0][eleCnt2d] = tArr1[i];
                            arr[1][eleCnt2d] = tArr2[i];
                            arr[2][eleCnt2d] = tArr3[i];
                            ++eleCnt2d;
                        }
                    }
                } else if (thisEle.getDim() == EnElement::D3) {
                    for (size_t i = 0; i < anzEle; ++i) {
                        if (bl[i] > 0) {
                            arr[0][eleCnt3d] = tArr1[i];
                            arr[1][eleCnt3d] = tArr2[i];
                            arr[2][eleCnt3d] = tArr3[i];
                            ++eleCnt3d;
                        }
                    }
                }
                delete[] tArr1;
                delete[] tArr2;
                delete[] tArr3;
                break;
            }
            }
        } else {
            CERR << "unknown element type " << elementType << std::endl;
            result.reset();
            return result;
        }

        currentLine = getStr(in);
    }

    return result;
}

//
// Destructor
//
DataGoldBin::~DataGoldBin()
{}

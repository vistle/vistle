// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                                  (C)2003 VirCinity  ++
// ++ Description:                                                        ++
// ++             Implementation of class DataGold                        ++
// ++                                                                     ++
// ++ Author:  Ralf Mikulla (rm@vircinity.com)                            ++
// ++                                                                     ++
// ++               VirCinity GmbH                                        ++
// ++               Nobelstrasse 15                                       ++
// ++               70569 Stuttgart                                       ++
// ++                                                                     ++
// ++ Date: 07.04.2003                                                    ++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "DataGold.h"
#include "EnElement.h"
#include "ReadEnsight.h"
#include "EnPart.h"

#include <sstream>
#include <array>
#include <memory>

#include <vistle/core/vec.h>

using namespace vistle;

#define CERR std::cerr << "DataGold::" << __func__ << ": " << where() << ": "

DataGold::DataGold(ReadEnsight *mod, const std::string &name, int dim, bool perVertex, CaseFile::BinType binType)
: EnFile(mod, name, dim, binType), perVertex_(perVertex)
{}

// helper: read the next line of interest from a data file.
// Returns NextLine::Part when a part line is found (the part number is in
// `partNrRead`, the position of the part line in `partStart`, the file is
// left positioned right after the part number); NextLine::Element when a
// non-part line is found (returned in `line`); NextLine::End at the end of
// the file or of the first time step.
// END/BEGIN TIME STEP lines are followed by a description line that is
// consumed; the line following text mentioning "particles" is skipped (the
// passing is disturbed by such text containing the word "part").
DataGold::NextLine DataGold::nextDataLine(FILE *in, std::string &line, std::streamoff &partStart, int &partNrRead)
{
    std::string l(getStr(in));
    while (!feof(in)) {
        if (l.find("END TIME STEP") != std::string::npos) {
            getStr(in); // read description
            return NextLine::End;
        }
        if (l.find("BEGIN TIME STEP") != std::string::npos || l.find("particles") != std::string::npos) {
            // "BEGIN TIME STEP" is followed by a description line, and any
            // text including the word "part" will disturb the passing
            l = getStr(in);
            continue;
        }
        if (parsePartLine(l)) {
            partStart = filePos();
            partNrRead = getInt(in);
            return NextLine::Part;
        }
        line = l;
        return NextLine::Element;
    }
    return NextLine::End;
}

bool DataGold::parseForParts()
{
    if (!isOpen()) {
        CERR << "file not opened" << std::endl;
        return false;
    }
    auto &in = in_;

    getStr(in); // description

    std::streamoff partStart = 0;
    int partNrRead = 0;
    if (perVertex_) {
        // per-vertex data: record part positions and skip the data
        while (true) {
            std::string line;
            if (nextDataLine(in, line, partStart, partNrRead) != NextLine::Part)
                return true;
            auto actPart = findPart(partNrRead);
            if (!actPart) {
                CERR << "part " << partNrRead << " not found" << std::endl;
                return false;
            }
            actPart->setStartPos(partStart, name());
            size_t numCoord = actPart->numCoords();
            std::string nextLine = getStr(in);
            if (nextLine.find("coordinates") != std::string::npos) {
                for (unsigned d = 0; d < dim_; ++d) {
                    skipFloat(in, numCoord);
                }
            } else {
                // no coordinates line: restore position to right after the part number
                // so the next nextDataLine call re-reads this line correctly
                file::seek(in, filePos(), SEEK_SET);
            }
        }
    }

    // per-element data: record part positions and skip the data
    EnPart *actPart = nullptr;
    while (true) {
        std::string line;
        auto next = nextDataLine(in, line, partStart, partNrRead);
        if (next == NextLine::Part) {
            actPart = findPart(partNrRead);
            if (!actPart) {
                CERR << "part " << partNrRead << " not found" << std::endl;
                return false;
            }
            actPart->setStartPos(partStart, name());
            continue;
        }
        if (next == NextLine::End)
            return true;
        bool undefined = false;
        if (line.ends_with(" undef")) {
            line = line.substr(0, line.size() - 6);
            undefined = true;
        }
        EnElement elem(line);
        if (elem.valid()) {
            if (undefined) {
                // skip the undefined value
                skipFloat(in, 1);
            }
            // we have a valid EnSight element
            auto anzEle = actPart->getElementNum(elem.getEnType());
            for (unsigned d = 0; d < dim_; ++d) {
                skipFloat(in, anzEle);
            }
        } else if (!line.empty()) {
            CERR << "unknown element type " << line << std::endl;
            return false;
        }
    }
}

vistle::Object::ptr DataGold::read(int timestep, int block, EnPart *part, const EnPart *refPart)
{
    int partToRead = part->getPartNum();
    vistle::Object::ptr result;
    if (part->startPos(name()) <= 0) {
        CERR << "invalid start pos for part " << partToRead << " - skipping" << std::endl;
        return result;
    }

    auto in = open();
    if (!in) {
        CERR << "file not open - skipping" << std::endl;
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

    file::seek(in, part->startPos(name()), SEEK_SET);

    std::streamoff partStart = 0;
    int partNrRead = 0;
    if (perVertex_) {
        // per-vertex data
        std::string line;
        while (true) {
            auto next = nextDataLine(in, line, partStart, partNrRead);
            if (next == NextLine::End)
                return result;
            if (next == NextLine::Element)
                continue;
            // next == NextLine::Part
            if (partNrRead != partToRead)
                return result;
            size_t numCoord = part->numCoords();
            std::streamoff posAfterInt = file::tell(in);
            std::string nextLine = getStr(in);
            if (nextLine.find("coordinates") != std::string::npos) {
                setNumVals(numCoord);
                CERR << "found part to read with " << numCoord << " values" << std::endl;
                // coordinates -line
                for (unsigned d = 0; d < dim_; ++d)
                    getFloatArr(in, numCoord, arr[d]);
            } else {
                // empty part without coordinates: restore position and return
                file::seek(in, posAfterInt, SEEK_SET);
            }
            return result;
        }
    }

    // per-element data
    std::array<size_t, 4> elemCount{};
    while (true) {
        std::string line;
        auto next = nextDataLine(in, line, partStart, partNrRead);
        if (next == NextLine::End)
            break;
        if (next == NextLine::Part) {
            if (partNrRead != partToRead)
                return result;
            // allocate memory for whole parts
            setNumVals(part->getTotNumEle());
            for (int d = 0; d < 4; ++d)
                elemCount[d] = 0;
            continue; // next line should be the element name
        }
        std::stringstream err;
        err << "in  block " << block << " timestep " << timestep << " on part " << part->comment() << ": ";
        bool undefined = false;
        if (line.ends_with(" undef")) {
            line = line.substr(0, line.size() - 6);
            undefined = true;
        }
        EnElement elem(line);
        if (!elem.valid()) {
            err << "unknown element type " << line << std::endl;
            CERR << err.str();
            ens->sendError(err.str());
            result.reset();
            return result;
        }
        const auto *thisEle = part->findElementType(elem.getEnType());
        if (!thisEle) {
            err << "element not found for type " << elem.getEnType();
            CERR << err.str();
            ens->sendError(err.str());
            result.reset();
            return result;
        }
        if (undefined) {
            float undef_value = 0.;
            getFloatArr(in, 1, &undef_value);
        }
        auto anzEle = part->getElementNum(elem.getEnType());
        const auto &bl = thisEle->getBlanklist();
        if (bl.size() != anzEle) {
            err << "blanklist size " << bl.size() << " does not match number of elements " << anzEle
                << " for element type " << line;
            CERR << err.str();
            ens->sendError(err.str());
            result.reset();
            return result;
        }
        CERR << "read " << anzEle << " " << dim_ << "D values for " << line << std::endl;
        // read the component-major values and scatter them into the 2d/3d
        // element slots according to the blanklist
        size_t *start = &elemCount[thisEle->getDim()];
        size_t idx = 0;
        if (start) {
            for (unsigned d = 0; d < dim_; ++d) {
                idx = *start;
                auto tArr = std::make_unique_for_overwrite<vistle::Scalar[]>(anzEle);
                getFloatArr(in, anzEle, tArr.get());
                for (size_t i = 0; i < anzEle; ++i) {
                    if (bl[i] > 0) {
                        arr[d][idx] = tArr[i];
                        ++idx;
                    }
                }
            }
            *start = idx;
        }
    }

    return result;
}

DataGold::~DataGold() = default;

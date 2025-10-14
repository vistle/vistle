#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/util/enum.h>
#include <bitset>

#include "GenIsoDat.h"


DEFINE_ENUM_WITH_STRING_CONVERSIONS(Choices, (Tetrahedron)(Pyramid)(Prism)(Hexahedron)(Polyhedron))

MODULE_MAIN(GenIsoDat)

using namespace vistle;

GenIsoDat::GenIsoDat(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createOutputPort("data_out", "scalar data on grid");
    createOutputPort("mapdata_out", "additional field");

    m_cellTypeParam = addIntParameter("cell_type", "type of cells", 1, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_cellTypeParam, Choices);

    m_caseNumParam = addIntParameter("case_num", "case number (-1: all)", -1);
}

GenIsoDat::~GenIsoDat()
{}

bool GenIsoDat::prepare()
{
    std::cerr << "cell type: " << m_cellTypeParam->getValue() << std::endl;

    vistle::UnstructuredGrid::ptr grid(new vistle::UnstructuredGrid(0, 0, 0));

    vistle::Vec<Scalar>::ptr data(new vistle::Vec<Scalar>(size_t(0)));
    data->setBlock(rank());
    data->setNumBlocks(size());
    vistle::Vec<Scalar>::ptr mapdata(new vistle::Vec<Scalar>(size_t(0)));
    mapdata->setBlock(rank());
    mapdata->setNumBlocks(size());


    auto &cl = grid->cl();
    auto &el = grid->el();
    auto &x = grid->x();
    auto &y = grid->y();
    auto &z = grid->z();
    auto &tl = grid->tl();

    Index numVert = -1;
    Index numShift = -1;
    Index numElements = -1;

    switch (m_cellTypeParam->getValue()) {
    case Hexahedron: {
        if (m_caseNumParam->getValue() == -1) {
            numVert = 2048;
            numShift = 32;
            numElements = 256;
        } else {
            numVert = 8;
            numShift = 2;
            numElements = 1;
        };

        for (Index i = 0; i < numVert; i++) {
            cl.push_back(i);
        }

        for (Index i = 0; i < numShift; i += 2) {
            for (Index j = 0; j < numShift; j += 2) {
                x.push_back(i);
                x.push_back(i);
                x.push_back(i + 1);
                x.push_back(i + 1);
                x.push_back(i);
                x.push_back(i);
                x.push_back(i + 1);
                x.push_back(i + 1);


                y.push_back(j);
                y.push_back(j + 1);
                y.push_back(j + 1);
                y.push_back(j);
                y.push_back(j);
                y.push_back(j + 1);
                y.push_back(j + 1);
                y.push_back(j);
            }
        }

        for (Index i = 0; i < numElements; i++) {
            z.push_back(1);
            z.push_back(1);
            z.push_back(1);
            z.push_back(1);
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
        }

        for (Index i = 0; i < numElements; i++) {
            el.push_back(el[i] + 8);
        }

        for (Index i = 0; i < numElements; i++) {
            tl.push_back(UnstructuredGrid::HEXAHEDRON);
        }

        std::bitset<8> newdata;
        for (Index i = 0; i < numElements; i++) {
            if (m_caseNumParam->getValue() == -1) {
                newdata = i;
            } else {
                newdata = m_caseNumParam->getValue();
            }

            for (int j = 0; j < 8; j++) {
                if (newdata[j]) {
                    data->x().push_back(1);
                } else {
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            }
        }
        break;
    }

    case Tetrahedron: {
        if (m_caseNumParam->getValue() == -1) {
            numVert = 64;
            numShift = 8;
            numElements = 16;
        } else {
            numVert = 4;
            numShift = 2;
            numElements = 1;
        };

        for (Index i = 0; i < numVert; i++) {
            cl.push_back(i);
        }

        for (Index i = 0; i < numShift; i += 2) {
            for (Index j = 0; j < numShift; j += 2) {
                x.push_back(i);
                x.push_back(i + 1);
                x.push_back(i + 0.5);
                x.push_back(i + 0.5);

                y.push_back(j);
                y.push_back(j);
                y.push_back(j + 1);
                y.push_back(j + 0.5);
            }
        }

        for (Index i = 0; i < numElements; i++) {
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(1);
        }

        for (Index i = 0; i < numElements; i++) {
            el.push_back(el[i] + 4);
        }

        for (Index i = 0; i < numElements; i++) {
            tl.push_back(UnstructuredGrid::TETRAHEDRON);
        }

        std::bitset<4> newdata;
        for (Index i = 0; i < numElements; i++) {
            if (m_caseNumParam->getValue() == -1) {
                newdata = i;
            } else {
                newdata = m_caseNumParam->getValue();
            }


            for (int j = 0; j < 4; j++) {
                if (newdata[j]) {
                    data->x().push_back(1);
                } else {
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            }
        };
        break;
    }

    case Prism: {
        if (m_caseNumParam->getValue() == -1) {
            numVert = 384;
            numShift = 16;
            numElements = 64;
        } else {
            numVert = 6;
            numShift = 2;
            numElements = 1;
        };


        for (Index i = 0; i < numVert; i++) {
            cl.push_back(i);
        }

        for (Index i = 0; i < numShift; i += 2) {
            for (Index j = 0; j < numShift; j += 2) {
                x.push_back(i);
                x.push_back(i);
                x.push_back(i + 1);
                x.push_back(i);
                x.push_back(i);
                x.push_back(i + 1);

                y.push_back(j);
                y.push_back(j + 1);
                y.push_back(j + 1);
                y.push_back(j);
                y.push_back(j + 1);
                y.push_back(j + 1);
            }
        }

        for (Index i = 0; i < numElements; i++) {
            z.push_back(1);
            z.push_back(1);
            z.push_back(1);
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
        }

        for (Index i = 0; i < numElements; i++) {
            el.push_back(el[i] + 6);
        }

        for (Index i = 0; i < numElements; i++) {
            tl.push_back(UnstructuredGrid::PRISM);
        }

        std::bitset<6> newdata;
        for (Index i = 0; i < numElements; i++) {
            if (m_caseNumParam->getValue() == -1) {
                newdata = i;
            } else {
                newdata = m_caseNumParam->getValue();
            }

            for (int j = 0; j < 6; j++) {
                if (newdata[j]) {
                    data->x().push_back(1);
                } else {
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            }

            std::cerr << newdata << std::endl;
        };
        break;
    }

    case Pyramid: {
        Index numSideshift;

        if (m_caseNumParam->getValue() == -1) {
            numVert = 160;
            numShift = 32;
            numElements = 32;
            numSideshift = 4;
        } else {
            numVert = 5;
            numShift = 2;
            numElements = 1;
            numSideshift = 2;
        };


        for (Index i = 0; i < numVert; i++) {
            cl.push_back(i);
        }

        for (Index i = 0; i < numSideshift; i += 2) {
            for (Index j = 0; j < numShift; j += 2) {
                x.push_back(i);
                x.push_back(i + 1);
                x.push_back(i + 1);
                x.push_back(i);
                x.push_back(i + 0.5);

                y.push_back(j);
                y.push_back(j);
                y.push_back(j + 1);
                y.push_back(j + 1);
                y.push_back(j + 0.5);
            }
        }

        for (Index i = 0; i < numElements; i++) {
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(1);
        }

        for (Index i = 0; i < numElements; i++) {
            el.push_back(el[i] + 5);
        }

        for (Index i = 0; i < numElements; i++) {
            tl.push_back(UnstructuredGrid::PYRAMID);
        }

        std::bitset<5> newdata;

        for (Index i = 0; i < numElements; i++) {
            if (m_caseNumParam->getValue() == -1) {
                newdata = i;
            } else {
                newdata = m_caseNumParam->getValue();
            }

            for (int j = 0; j < 5; j++) {
                if (newdata[j]) {
                    data->x().push_back(1);
                } else {
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            };
        };
        break;
    }
    case Polyhedron: {
        if (m_caseNumParam->getValue() == -1 && false) {
            numVert = 64;
            numShift = 8;
            numElements = 16;
        } else {
            numVert = 4;
            numShift = 2;
            numElements = 1;
        }

        cl.push_back(1);
        cl.push_back(3);
        cl.push_back(0);
        cl.push_back(1);

        cl.push_back(3);
        cl.push_back(2);
        cl.push_back(0);
        cl.push_back(3);

        cl.push_back(2);
        cl.push_back(1);
        cl.push_back(0);
        cl.push_back(2);

        cl.push_back(2);
        cl.push_back(3);
        cl.push_back(1);
        cl.push_back(2);

        for (Index i = 0; i < numShift; i += 2) {
            for (Index j = 0; j < numShift; j += 2) {
                x.push_back(i);
                x.push_back(i + 1);
                x.push_back(i + 0.5);
                x.push_back(i + 0.5);

                y.push_back(j);
                y.push_back(j);
                y.push_back(j + 1);
                y.push_back(j + 0.5);
            }
        }

        for (Index i = 0; i < numElements; i++) {
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(1);
        }

        el.push_back(16);

        for (Index i = 0; i < numElements; i++) {
            tl.push_back(UnstructuredGrid::POLYHEDRON);
        }

        std::bitset<4> newdata;
        for (Index i = 0; i < numElements; i++) {
            if (m_caseNumParam->getValue() == -1) {
                newdata = i;
            } else {
                newdata = m_caseNumParam->getValue();
            }

            for (int j = 0; j < 4; j++) {
                if (newdata[j]) {
                    data->x().push_back(1);
                } else {
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            }
        };
        break;
    }
    }

    for (Index i = 0; i < grid->getSize(); ++i) {
        z[i] += 2 * rank();
    }

    updateMeta(grid);

    //std::cerr << "pre-add data refcount: " << data->refcount() << std::endl;
    data->setGrid(grid);
    data->describe("iso", id());
    updateMeta(data);
    addObject("data_out", data);
    //std::cerr << "post-add data refcount: " << data->refcount() << std::endl;

    mapdata->setGrid(grid);
    updateMeta(mapdata);
    mapdata->describe("mapped", id());
    addObject("mapdata_out", mapdata);

    return true;
}

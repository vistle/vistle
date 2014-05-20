#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/vec.h>
#include <core/unstr.h>
#include <core/lines.h>
#include <core/points.h>

#include "../IsoSurface/tables.h"

#include "GenIsoDat.h"




MODULE_MAIN(GenIsoDat)

using namespace vistle;

GenIsoDat::GenIsoDat(const std::string &shmname, int rank, int size, int moduleID)
    : Module("GenIsoDat", shmname, rank, size, moduleID) {

    createOutputPort("grid_out");
    createOutputPort("data_out");
    createOutputPort("mapdata_out");
    createOutputPort("Vertices_low");
    createOutputPort("Vertices_high");

    m_cellTypeParam = addIntParameter("cell_type", "type of cells", 1, Parameter::Choice);
    std::vector<std::string> choices;

    choices.push_back("Hexahedron");
    choices.push_back("Tetrahedron");
    choices.push_back("Prism");
    choices.push_back("Pyramid");
    choices.push_back("Polyhedron");

    setParameterChoices(m_cellTypeParam, choices);

    m_caseNumParam = addIntParameter("case_num", "case number (-1: all)", -1);
}

GenIsoDat::~GenIsoDat() {

}

bool GenIsoDat::compute() {

    std::cerr << "cell type: " << m_cellTypeParam->getValue() << std::endl;

    vistle::UnstructuredGrid::ptr grid(new vistle::UnstructuredGrid(0, 0, 0));

    vistle::Points::ptr points_high(new vistle::Points(Index(0)));
    vistle::Vec<Scalar>::ptr data(new vistle::Vec<Scalar>(Index(0)));
    vistle::Vec<Scalar>::ptr mapdata(new vistle::Vec<Scalar>(Index(0)));


    auto &cl = grid->cl();
    auto &el = grid->el();
    auto &x = grid->x();
    auto &y = grid->y();
    auto &z = grid->z();
    auto &tl = grid->tl();

    Index numVert = -1;
    Index numShift = -1 ;
    Index numElements = -1;

    switch(m_cellTypeParam->getValue()){

    case 0:{

        if(m_caseNumParam->getValue() == -1){

            numVert = 2048;
            numShift = 32;
            numElements = 256;

        }
        else{

            numVert = 8;
            numShift = 2;
            numElements = 1;

        };

        for(int i = 0; i < numVert; i++){
            cl.push_back(i);
        }

        for(int i = 0; i < numShift; i+=2){

            for(int j = 0; j < numShift; j+=2){

                x.push_back(i);
                x.push_back(i);
                x.push_back(i+1);
                x.push_back(i+1);
                x.push_back(i);
                x.push_back(i);
                x.push_back(i+1);
                x.push_back(i+1);


                y.push_back(j);
                y.push_back(j+1);
                y.push_back(j+1);
                y.push_back(j);
                y.push_back(j);
                y.push_back(j+1);
                y.push_back(j+1);
                y.push_back(j);

            }
        }

        for(int i = 0; i < numElements; i++){

            z.push_back(1);
            z.push_back(1);
            z.push_back(1);
            z.push_back(1);
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);

        }

        for(int i = 0; i < numElements; i++){

            el.push_back(el[i]+8);

        }

        for(int i = 0; i < numElements; i++){

            tl.push_back(UnstructuredGrid::HEXAHEDRON);
        }




        std::bitset<8> newdata;


        for(int i = 0; i < numElements; i++){

            if(m_caseNumParam->getValue() == -1){
                newdata = i;

            }
            else{

                newdata = m_caseNumParam->getValue();

            }

            for(int j = 0; j < 8; j++){
                if(newdata[j]){
                    data->x().push_back(1);
                    points_high->x().push_back(x[j]);
                    points_high->y().push_back(y[j]);
                    points_high->z().push_back(z[j]);

                }
                else{
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            }

        };



    }; break;
    case 1:{

        if(m_caseNumParam->getValue() == -1){

            numVert = 64;
            numShift = 8;
            numElements = 16;

        }
        else{

            numVert = 4;
            numShift = 2;
            numElements = 1;

        };


        for(int i = 0; i < numVert; i++){
            cl.push_back(i);
        }

        for(int i = 0; i < numShift; i+=2){


            for(int j = 0; j < numShift; j+=2){

                x.push_back(i);
                x.push_back(i+1);
                x.push_back(i+0.5);
                x.push_back(i+0.5);

                y.push_back(j);
                y.push_back(j);
                y.push_back(j+1);
                y.push_back(j+0.5);


            }
        }

        for(int i = 0; i < numElements; i++){

            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(1);


        }

        for(int i = 0; i < numElements; i++){

            el.push_back(el[i]+4);

        }

        for(int i = 0; i < numElements; i++){

            tl.push_back(UnstructuredGrid::TETRAHEDRON);
        }



        std::bitset<4> newdata;


        for(int i = 0; i < numElements; i++){

            if(m_caseNumParam->getValue() == -1){
                newdata = i;

            }else{

                newdata = m_caseNumParam->getValue();

            }




            for(int j = 0; j < 4; j++){
                if(newdata[j]){
                    data->x().push_back(1);
                    points_high->x().push_back(x[j]);
                    points_high->y().push_back(y[j]);
                    points_high->z().push_back(z[j]);

                }
                else{
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            }

        };





    }; break;

    case 2:{

        if(m_caseNumParam->getValue() == -1){

            numVert = 384;
            numShift = 16;
            numElements = 64;

        }
        else{

            numVert = 6;
            numShift = 2;
            numElements = 1;

        };


        for(int i = 0; i < numVert; i++){
            cl.push_back(i);
        }

        for(int i = 0; i < numShift; i+=2){


            for(int j = 0; j < numShift; j+=2){

                x.push_back(i);
                x.push_back(i);
                x.push_back(i+1);
                x.push_back(i);
                x.push_back(i);
                x.push_back(i+1);

                y.push_back(j);
                y.push_back(j+1);
                y.push_back(j+1);
                y.push_back(j);
                y.push_back(j+1);
                y.push_back(j+1);

            }
        }

        for(int i = 0; i < numElements; i++){

            z.push_back(1);
            z.push_back(1);
            z.push_back(1);
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);

        }

        for(int i = 0; i < numElements; i++){

            el.push_back(el[i]+6);

        }

        for(int i = 0; i < numElements; i++){

            tl.push_back(UnstructuredGrid::PRISM);
        }



        std::bitset<6> newdata;


        for(int i = 0; i < numElements; i++){

            if(m_caseNumParam->getValue() == -1){
                newdata = i;

            }else{

                newdata = m_caseNumParam->getValue();

            }

            for(int j = 0; j < 6; j++){
                if(newdata[j]){
                    data->x().push_back(1);
                    points_high->x().push_back(x[j]);
                    points_high->y().push_back(y[j]);
                    points_high->z().push_back(z[j]);

                }
                else{
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            }


            std::cerr << newdata << std::endl;

        };


    };break;

    case 3:{


        if(m_caseNumParam->getValue() == -1){

            numVert = 160;
            numShift = 16;
            numElements = 32;

        }
        else{

            numVert = 5;
            numShift = 2;
            numElements = 1;

        };


        for(int i = 0; i < numVert; i++){
            cl.push_back(i);
        }

        for(int i = 0; i < numShift; i+=2){

            for(int j = 0; j < numShift; j+=2){

                x.push_back(i);
                x.push_back(i+1);
                x.push_back(i+1);
                x.push_back(i);
                x.push_back(i+0.5);

                y.push_back(j);
                y.push_back(j);
                y.push_back(j+1);
                y.push_back(j+1);
                y.push_back(j+0.5);

            }
        }

        for(int i = 0; i < numElements; i++){

            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(1);

        }

        for(int i = 0; i < numElements; i++){

            el.push_back(el[i]+5);

        }

        for(int i = 0; i < numElements; i++){

            tl.push_back(UnstructuredGrid::PYRAMID);
        }



        std::bitset<5> newdata;

        for(int i = 0; i < numElements; i++){

            if(m_caseNumParam->getValue() == -1){
                newdata = i;

            }else{

                newdata = m_caseNumParam->getValue();

            }

            for(int j = 0; j < 5; j++){
                if(newdata[j]){
                    data->x().push_back(1);
                    points_high->x().push_back(x[j]);
                    points_high->y().push_back(y[j]);
                    points_high->z().push_back(z[j]);

                }
                else{
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            };


        };


    }; break;
    case 4:{


        if(m_caseNumParam->getValue() == -1){

            numVert = 64;
            numShift = 8;
            numElements = 16;

        }
        else{

            numVert = 4;
            numShift = 2;
            numElements = 1;

        };

        cl.push_back(0);
        cl.push_back(1);
        cl.push_back(3);
        cl.push_back(0);
        cl.push_back(0);
        cl.push_back(3);
        cl.push_back(2);
        cl.push_back(0);
        cl.push_back(0);
        cl.push_back(2);
        cl.push_back(1);
        cl.push_back(0);
        cl.push_back(1);
        cl.push_back(2);
        cl.push_back(3);
        cl.push_back(1);



        for(int i = 0; i < numShift; i+=2){


            for(int j = 0; j < numShift; j+=2){

                x.push_back(i);
                x.push_back(i+1);
                x.push_back(i+0.5);
                x.push_back(i+0.5);

                y.push_back(j);
                y.push_back(j);
                y.push_back(j+1);
                y.push_back(j+0.5);

            }
        }

        for(int i = 0; i < numElements; i++){

            z.push_back(0);
            z.push_back(0);
            z.push_back(0);
            z.push_back(1);

        }

        el.push_back(16);

        for(int i = 0; i < numElements; i++){

            tl.push_back(UnstructuredGrid::POLYHEDRON);
        }



        std::bitset<4> newdata;

        for(int i = 0; i < numElements; i++){

            if(m_caseNumParam->getValue() == -1){
                newdata = i;

            }else{

                newdata = m_caseNumParam->getValue();

            }

            for(int j = 0; j < 4; j++){

                if(newdata[j]){

                    data->x().push_back(1);
                    points_high->x().push_back(x[j]);
                    points_high->y().push_back(y[j]);
                    points_high->z().push_back(z[j]);

                }
                else{
                    data->x().push_back(-1);
                }
                mapdata->x().push_back(j);
            }


        };


    }
        break;
    }

    addObject("data_out", data);
    addObject("mapdata_out", mapdata);
    addObject("grid_out", grid);
    addObject("Vertices_high", points_high);

    return true;
}

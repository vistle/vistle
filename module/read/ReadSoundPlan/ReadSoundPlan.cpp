/**************************************************************************\
**                                                           (C)2023 HLRS **
**                                                                        **
** Description: Read module for SoundPLAN data format (Port from COVISE)  **
**                                                                        **
**                                                                        **
**                                                                        **
**                                                                        **
**                                                                        **
**                                                                        **
** History:                                                               **
** März  23	    M. Djuric	    V1.0                                      **
**                                                                        **
**                                                                        **
*\**************************************************************************/

#include "ReadSoundPlan.h"

//vistle
#include "vistle/core/points.h"
#include "vistle/core/scalar.h"
#include "vistle/module/reader.h"

//boost
#include <boost/algorithm/string/predicate.hpp>

//mpi
#include <boost/graph/graph_traits.hpp>
#include <memory>
#include <mpi.h>

//std
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace vistle;
using namespace std;
MODULE_MAIN(ReadSoundPlan)

ReadSoundPlan::ReadSoundPlan(const string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    m_file = addStringParameter("Filename", "SoundPLAN data file path", "/home/pow/covise/data", Parameter::Filename);

    m_meshOutPort = createOutputPort("Mesh", "Points (point data)");
    m_daySoundOutPort = createOutputPort("Daysound", "Float (daysound)");
    m_nightSoundOutPort = createOutputPort("Nightsound", "Float (nightsound)");
    setPartitions(size());
    setTimesteps(1);
}

bool ReadSoundPlan::examine(const vistle::Parameter *param)
{
    if (!param || param == m_file)
        if (!boost::algorithm::ends_with(m_file->getValue(), ".txt")) {
            if (rank() == 0)
                sendInfo("Only SoundPLAN data files with .txt extension are valid.");
            return false;
        }

    return true;
}

bool ReadSoundPlan::read(Token &token, int timestep, int block)
{
    // TODO: implement MPI Version
    /* if (timestep > -1) */
    /*     return true; */

    /* const auto &fileName = m_file->getValue(); */
    /* string errMsg = "ERROR: Can't open file >> " + fileName; */
    /* string fileContent; */
    /* MPI_File fileHandler; */
    /* auto err = MPI_File_open(comm(), fileName.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &fileHandler); */
    /* if (err) { */
    /*     if (rank() == 0) */
    /*         sendInfo(errMsg); */
    /*     return false; */
    /* } */
    /* MPI_Offset fileSize; */
    /* MPI_File_get_size(fileHandler, &fileSize); */

    /* int numBlocks = token.meta().numBlocks(); */
    /* int count = fileSize / numBlocks; */
    /* auto displacement = block * count *  sizeof(byte); */

    //skip header on rank 0
    /* if (rank() == 0) */
    /*     displacement += */


    /* sendInfo("NumBlocks: %d\nFileSize: %lld\nCount: %d\nDisplacement: %lu\n",numBlocks, fileSize, count, displacement); */

    /* MPI_file_read_at(fileHandler, displacement, ); */

    /* MPI_File_close(&fileHandler); */
    /* return true; */

    /* ifstream soundPlanFile(fileName); */
    /* if (!soundPlanFile.is_open()) { */
    /*     sendInfo(errorMsg); */
    /*     return false; */
    /* } */

    /* while (soundPlanFile) { */
    /*     getline(soundPlanFile,soundPlanContent); */
    /* } */


    /* Points::ptr meshPoints(new Points()); */

    //////// OLD COVISE VERSION ////////
    if ((timestep > -1) || (block != 0))
        return true;

    FILE *fp;
    int num_elem, num_field;
    int i;
    char buf[300], tmp[400];

    //temp data
    num_elem = 0;
    num_field = 0;
    char *pch;

    // the COVISE output objects (located in shared memory)
    Points::ptr meshObj;
    Vec<Scalar>::ptr daySoundObj;
    Vec<Scalar>::ptr nightSoundObj;

    bool haveW = false;
    bool haveHeader = false;

    // read the file browser parameter
    string fileName = m_file->getValue();

    // open the file
    if ((fp = fopen(fileName.c_str(), "r")) == NULL) {
        sendInfo("ERROR: Can't open file >> %s", fileName.c_str());
        return false;
    }

    //check for number of objects
    while (fgets(tmp, sizeof(tmp), fp) != NULL) {
        if (num_elem == 0) {
            pch = strtok(tmp, "\t;");
            while (pch != NULL) {
                printf("%s\n", pch);
                pch = strtok(NULL, " \t;");
                if (num_field == 0)
                    if (pch[0] == 'Y')
                        haveHeader = true;
                if (num_field == 2)
                    if (pch[0] == 'W')
                        haveW = true;
                num_field++;
            }
        }
        num_elem++;
    }
    rewind(fp);
    printf("%i\n", num_field);
    if (haveHeader) {
        fgets(tmp, sizeof(tmp), fp); // read header line
        num_elem--;
    }

    // create the unstructured grid object for the mesh
    meshObj = make_shared<Points>(num_elem);
    daySoundObj = make_shared<Vec<Scalar>>(num_elem);
    nightSoundObj = make_shared<Vec<Scalar>>(num_elem);

    // get pointers to the element, vertex and coordinate lists
    auto dBDay = daySoundObj->x().data();
    auto dBNight = nightSoundObj->x().data();
    auto x_coord = meshObj->x().data(), y_coord = meshObj->y().data(), z_coord = meshObj->z().data();

    // read the coordinate lines
    for (i = 0; i < num_elem; i++) {
        // read the line which contains the coordinates and scan it
        if (fgets(buf, 300, fp) != NULL) {
            //sscanf(buf,"%s%f%f%f%f%f\n",tmp, x_coord, y_coord, z_coord,dBDay, dBNight);
            char *c = buf;
            while (*c != '\0') {
                if (*c == ',')
                    *c = '.';
                c++;
            }
            float W;
            if (sizeof(*x_coord) == sizeof(float)) {
                if (haveW)
                    sscanf(buf, "%f%f%f%f%f%f", x_coord, y_coord, z_coord, &W, dBDay, dBNight);
                else
                    sscanf(buf, "%f%f%f%f%f", x_coord, y_coord, z_coord, dBDay, dBNight);
            } else {
                if (haveW)
                    sscanf(buf, "%lf%lf%lf%lf%lf%lf", x_coord, y_coord, z_coord, &W, dBDay, dBNight);
                else
                    sscanf(buf, "%lf%lf%lf%lf%lf", x_coord, y_coord, z_coord, dBDay, dBNight);
            }
            x_coord++;
            y_coord++;
            z_coord++;
            dBDay++;
            dBNight++;
        }
    }
    // close the file
    fclose(fp);
    meshObj->setBlock(block);
    meshObj->setTimestep(timestep);
    token.applyMeta(meshObj);
    token.addObject(m_meshOutPort, meshObj);
    daySoundObj->setMapping(DataBase::Vertex);
    daySoundObj->setGrid(meshObj);
    daySoundObj->setBlock(block);
    daySoundObj->setTimestep(timestep);
    nightSoundObj->setMapping(DataBase::Vertex);
    nightSoundObj->setGrid(meshObj);
    nightSoundObj->setBlock(block);
    nightSoundObj->setTimestep(timestep);
    token.applyMeta(nightSoundObj);
    token.applyMeta(daySoundObj);
    token.addObject(m_daySoundOutPort, daySoundObj);
    token.addObject(m_nightSoundOutPort, nightSoundObj);
    return true;
}

/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>

#include <iostream>

#include <control/executor.h>

class Vistle: public vistle::Executor {

   public:
   Vistle() : Executor("vistle") {}
   void config();
};

int main(int argc, char ** argv) {

   MPI_Init(&argc, &argv);

   Vistle().run();

   MPI_Finalize();
   
   return 0;
}
   
void Vistle::config() {
#if 0
   vistle::message::Spawn readCovise1(0, rank, 1, "ReadCovise");
   comm->handleMessage(readCovise1);

   vistle::message::Spawn readCovise2(0, rank, 2, "ReadCovise");
   comm->handleMessage(readCovise2);

   vistle::message::Spawn isoSurface(0, rank, 3, "IsoSurface");
   comm->handleMessage(isoSurface);

   vistle::message::Spawn showUSG(0, rank, 4, "ShowUSG");
   comm->handleMessage(showUSG);

   vistle::message::Spawn renderer(0, rank, 5, "OSGRenderer");
   comm->handleMessage(renderer);

   vistle::message::Connect connect13g(0, rank, 1, "grid_out", 3, "grid_in");
   comm->handleMessage(connect13g);

   vistle::message::Connect connect13d(0, rank, 2, "grid_out", 3, "data_in");
   comm->handleMessage(connect13d);

   vistle::message::Connect connect14(0, rank, 1, "grid_out", 4, "grid_in");
   comm->handleMessage(connect14);

   vistle::message::Connect connect45(0, rank, 4, "grid_out", 5, "data_in");
   comm->handleMessage(connect45);

   vistle::message::Connect connect35(0, rank, 3, "grid_out", 5, "data_in");
   comm->handleMessage(connect35);

   vistle::message::SetFileParameter param1(0, rank, 1, "filename",
                                           "/tmp/g.covise");
   comm->handleMessage(param1);

   vistle::message::SetFileParameter param2(0, rank, 2, "filename",
                                           "/tmp/p.covise");
   comm->handleMessage(param2);

   vistle::message::Compute compute1(0, rank, 1);
   comm->handleMessage(compute1);

   vistle::message::Compute compute2(0, rank, 2);
   comm->handleMessage(compute2);
#endif

#if 0
   vistle::message::Spawn readCovise(0, rank, 1, "ReadCovise");
   comm->handleMessage(readCovise);

   vistle::message::SetFileParameter param(0, rank, 1, "filename",
                                           "/tmp/single_geom2d.covise");
   comm->handleMessage(param);

   vistle::message::Spawn renderer(0, rank, 2, "OSGRenderer");
   comm->handleMessage(renderer);

   vistle::message::Connect connect12(0, rank, 1, "grid_out", 2, "data_in");
   comm->handleMessage(connect12);

   vistle::message::Compute compute(0, rank, 1);
   comm->handleMessage(compute);
#endif

#if 0
   vistle::message::Spawn gendat(0, rank, 1, "Gendat");
   comm->handleMessage(gendat);
   vistle::message::Spawn renderer(0, rank, 2, "OSGRenderer");
   comm->handleMessage(renderer);

   vistle::message::Connect connect(0, rank, 1, "data_out", 2, "data_in");
   comm->handleMessage(connect);

   vistle::message::Compute compute(0, rank, 1);
   comm->handleMessage(compute);
#endif

#if 1
   enum { RGEO = 1, RGRID, RPRES, CUTGEO, CUTSURF, ISOSURF, COLOR, COLLECT, RENDERER, WRITEVISTLE };

   spawn(RGEO,  "ReadVistle");
   /*
   spawn(RGRID, "ReadCovise");
   spawn(RPRES, "ReadCovise");
   */
   spawn(CUTGEO, "CutGeometry");
   /*
   spawn(CUTSURF, "CuttingSurface");
   spawn(ISOSURF, "IsoSurface");

   spawn(COLOR, "Color");
   spawn(COLLECT, "Collect");
   spawn(WRITEVISTLE, "WriteVistle");
   */
   spawn(RENDERER, "OSGRenderer");

   setParam(RGEO, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/three_geo2d.vistle");

   setParam(RGRID, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/multi_geo3d.covise");

   setParam(RPRES, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/multi_p.covise");
   /*
   setParam(comm, rank, WRITEVISTLE, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/three_geo2d.vistle");
   */
   setParam(ISOSURF, "isovalue", -1.0);

   setParam(CUTSURF, "distance", 0.0);
   setParam(CUTSURF, "normal", vistle::Vector(1.0, 0.0, 0.0));

   connect(RGEO, "grid_out", CUTGEO, "grid_in");
   connect(CUTGEO, "grid_out", RENDERER, "data_in");

   connect(RGRID, "grid_out", CUTSURF, "grid_in");
   connect(RPRES, "grid_out", CUTSURF, "data_in");
   connect(CUTSURF, "grid_out", COLLECT, "grid_in");
   connect(CUTSURF, "data_out", COLOR, "data_in");
   connect(COLOR, "data_out", COLLECT, "texture_in");
   connect(COLLECT, "grid_out", RENDERER, "data_in");

   connect(RGRID, "grid_out", ISOSURF, "grid_in");
   connect(RPRES, "grid_out", ISOSURF, "data_in");
   connect(ISOSURF, "grid_out", RENDERER, "data_in");

   //connect(RGEO, "grid_out", WRITEVISTLE, "grid_in");

   compute(RGEO);
   compute(RGRID);
   compute(RPRES);
#endif

#if 0
   vistle::message::Spawn readCovise(0, rank, 1, "ReadCovise");
   comm->handleMessage(readCovise);

   vistle::message::SetFileParameter param(0, rank, 1, "filename",
                     "/data/OpenFOAM/PumpTurbine/covise/p.covise");
   comm->handleMessage(param);

   vistle::message::Spawn color(0, rank, 2, "Color");
   comm->handleMessage(color);

   vistle::message::Connect connect12(0, rank, 1, "grid_out", 2, "data_in");
   comm->handleMessage(connect12);

   vistle::message::Compute compute(0, rank, 1);
   comm->handleMessage(compute);
#endif

#if 0
   enum { READ = 1, WRITE };
   spawn(READ, "ReadCovise");
   spawn(WRITE, "WriteVistle");

   setParam(READ, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/geo3d.covise");

   setParam(WRITE, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/geo3d.vistle");

   connect(READ, "grid_out", WRITE, "grid_in");

   compute(READ);
#endif

#if 0
   enum { READ = 1, SHOWUSG, RENDERER, CUTSURF, COLOR, COLLECT };
   spawn(READ, "ReadFOAM");
   spawn(SHOWUSG, "ShowUSG");
   spawn(RENDERER, "OSGRenderer");

   spawn(CUTSURF, "CuttingSurface");
   spawn(COLOR, "Color");
   spawn(COLLECT, "Collect");

   setParam(READ, "filename",
            "/data/OpenFOAM/PumpTurbine/transient");

   setParam(CUTSURF, "distance", 0.02);
   setParam(CUTSURF, "normal", vistle::Vector(1.0, 0.0, 0.0));

   setParam(COLOR, "min", -15.0);
   setParam(COLOR, "max", 0.0);

   //connect(READ, "grid_out", SHOWUSG, "grid_in");
   connect(READ, "grid_out", CUTSURF, "grid_in");
   connect(READ, "p_out", CUTSURF, "data_in");

   connect(CUTSURF, "data_out", COLOR, "data_in");
   connect(CUTSURF, "grid_out", COLLECT, "grid_in");

   connect(COLOR, "data_out", COLLECT, "texture_in");
   connect(COLLECT, "grid_out", RENDERER, "data_in");

   //connect(SHOWUSG, "grid_out", RENDERER, "data_in");

   compute(READ);
#endif
}

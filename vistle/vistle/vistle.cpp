/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>

#include <iostream>
#include <fstream>

#include <control/executor.h>
#include <control/pythonembed.h>

using namespace vistle;

class Vistle: public vistle::Executor {

   public:
   Vistle(int argc, char *argv[]) : Executor(argc, argv) {}
   void config();
};

int main(int argc, char ** argv) {

   MPI_Init(&argc, &argv);

   PythonEmbed *pi = NULL;

   {
      Vistle vistle(argc, argv);

      if (!vistle.getRank()) {
         pi = new PythonEmbed(argc, argv);
         vistle.registerInterpreter(pi);
         if (argc > 1) {
            std::ifstream file(argv[1]);
            std::string input;

            input.insert(input.begin(),
                  std::istreambuf_iterator<char>(file),
                  std::istreambuf_iterator<char>());
            std::cerr << "INPUT was: " << input << std::endl;
            vistle.setInput(input);
         }
      }

      vistle.run();
   }

   delete pi;

   MPI_Finalize();
   
   return 0;
}

//#define ISOSURFACE
//#define READCOVISE
//#define GENDATA
#define TURBINEVISTLE
//#define TURBINECOVISE
//#define CONVERT
//#define CUTTINGSURFACE
   
void Vistle::config() {
#ifdef ISOSURFACE
   enum { RGEO = 1, RPRESSURE, ISOSURF, SHOWUSG, REND };

   spawn(RGEO, "ReadCovise");
   spawn(RPRESSURE, "ReadCovise");
   spawn(ISOSURF, "IsoSurface");
   spawn(SHOWUSG, "ShowUSG");
   spawn(REND, "OSGRenderer");

   connect(RGEO, "grid_out", ISOSURF, "grid_in");
   connect(RPRESSURE, "grid_out", ISOSURF, "data_in");
   connect(RGEO, "grid_out", SHOWUSG, "grid_in");
   connect(SHOWUSG, "grid_out", REND, "data_in");
   connect(ISOSURF, "grid_out", REND, "data_in");

   setParam(RGEO, "filename", "/tmp/g.covise");
   setParam(RPRESSURE, "filename", "/tmp/p.covise");

   compute(RGEO);
   compute(RPRESSURE);
#endif

#ifdef READCOVISE
   enum { RGEO = 1, REND };

   spawn(RGEO, "ReadCovise");
   spawn(REND, "OSGRenderer");

   setParam(RGEO, "filename", "/tmp/single_geom2d.covise");

   connect(RGEO, "grid_out", REND, "data_in");

   compute(RGEO);
#endif

#ifdef GENDATA
   enum { GENDAT = 1, REND };

   spawn(GENDAT, "Gendat");
   spawn(REND, "OSGRenderer");

   connect(GENDAT, "data_out", REND, "data_in");

   compute(GENDAT);
#endif

#ifdef TURBINEVISTLE
   //enum { RGEO = 1, RGRID, RPRES, CUTGEO, CUTSURF, ISOSURF, COLOR, COLLECT, RENDERER, WRITEVISTLE, WRITEARCHIVE };

   //int RGEO = spawn("ReadCovise");
   int RGRID = spawn("ReadCovise");
   int RPRES = spawn("ReadCovise");
   int CUTGEO = spawn("CutGeometry");
   //int CUTSURF = spawn("CuttingSurface");
   int ISOSURF = spawn("IsoSurface");

   int COLOR = spawn("Color");
   int COLLECT = spawn("Collect");
   /*
   int WRITEVISTLE = spawn(WRITEVISTLE, "WriteVistle");
   */
   //int RENDERER = spawn("OSGRenderer");
   int WRITEARCHIVE = spawn("WriteArchive");

   int RGEO = WRITEARCHIVE + 3;
   int CUTSURF = WRITEARCHIVE + 3;
   int RENDERER = WRITEARCHIVE + 3;

#if 1
   setParam(RGEO, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/three_geo2d.covise");
#endif

   setParam(RGRID, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/multi_geo3d.covise");

   setParam(RPRES, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/multi_p.covise");
   /*
   setParam(WRITEVISTLE, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/three_geo2d.vistle");
   */
   setParam(ISOSURF, "isovalue", -5.0);

   setParam(CUTSURF, "distance", 0.0);
   setParam(CUTSURF, "normal", vistle::Vector(1.0, 0.0, 0.0));

   setParam(WRITEARCHIVE, "filename",
         "turbinevistle.archive");
   setParam(WRITEARCHIVE, "format", 0); // 0=binary, 1=text, 2=XML

   connect(RGEO, "grid_out", CUTGEO, "grid_in");
   connect(CUTGEO, "grid_out", RENDERER, "data_in");
   connect(CUTGEO, "grid_out", WRITEARCHIVE, "grid_in");

   connect(RGRID, "grid_out", CUTSURF, "grid_in");
   connect(RPRES, "grid_out", CUTSURF, "data_in");
   connect(RGRID, "grid_out", ISOSURF, "grid_in");
   connect(RPRES, "grid_out", ISOSURF, "data_in");
   connect(CUTSURF, "grid_out", COLLECT, "grid_in");
   connect(CUTSURF, "data_out", COLOR, "data_in");
   connect(COLOR, "data_out", COLLECT, "texture_in");
   connect(COLLECT, "grid_out", RENDERER, "data_in");
   connect(COLLECT, "grid_out", WRITEARCHIVE, "grid_in");


   connect(ISOSURF, "grid_out", RENDERER, "data_in");
   connect(ISOSURF, "grid_out", WRITEARCHIVE, "grid_in");

   //connect(RGEO, "grid_out", WRITEVISTLE, "grid_in");

   compute(RGEO);
   compute(RGRID);
   compute(RPRES);
#endif

#ifdef TURBINECOVISE
   enum { RPRESSURE = 1, COLOR };

   spawn(RPRESSURE, "ReadCovise");
   spawn(COLOR, "Color");

   setParam(RPRESSURE, "filename", "/data/OpenFOAM/PumpTurbine/covise/p.covise");

   connect(RPRESSURE, "grid_out", COLOR, "data_in");

   compute(RPRESSURE);
#endif

#ifdef CONVERT
   enum { READ = 1, WRITE };

   spawn(READ, "ReadCovise");
   spawn(WRITE, "WriteVistle");

   setParam(READ, "filename", "/data/OpenFOAM/PumpTurbine/covise/geo3d.covise");
   setParam(WRITE, "filename", "/data/OpenFOAM/PumpTurbine/covise/geo3d.vistle");

   connect(READ, "grid_out", WRITE, "grid_in");

   compute(READ);
#endif

#ifdef CUTTINGSURFACE
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

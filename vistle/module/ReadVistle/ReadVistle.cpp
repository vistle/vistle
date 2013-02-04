#include <sstream>
#include <iomanip>
#include <string>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include <core/object.h>

#include "ReadVistle.h"

using namespace vistle;
namespace ba = boost::archive;

MODULE_MAIN(ReadVistle)

ReadVistle::ReadVistle(const std::string &shmname, int rank, int size, int moduleID)
   : Module("ReadVistle", shmname, rank, size, moduleID) {

   createOutputPort("grid_out");
   addStringParameter("filename", "");
}

ReadVistle::~ReadVistle() {

}


bool ReadVistle::load(const std::string & name) {

   std::ifstream ifs(name.c_str());

   // read header
   std::string header;
   std::getline(ifs, header);
   std::string vistle, format;
   int version;
   std::stringstream str(header);
   str >> vistle >> format >> version;
   std::cerr << "vistle: " << vistle << ", format: " << format << ", version: " << version << std::endl;
   if (vistle != "vistle") {

      std::cerr << "ReadVistle: " << name << " is not a vistle file" << std::endl;
      return false;
   }

   do {
      {
         std::ios::pos_type pos = ifs.tellg();
         std::string firstline;
         std::getline(ifs, firstline);
         std::cerr << "Reading: pos=" << pos << std::endl;
         if (format != "binary")
            std::cerr << "First line: " << firstline << std::endl;
         ifs.seekg(pos);
      }
      ba::detail::basic_iarchive *ia = NULL;
      try {
         if (format == "binary") {
            ia  = new ba::binary_iarchive(ifs);
         } else if (format == "text") {
            ia = new ba::text_iarchive(ifs);
         } else if (format == "xml") {
            ia = new ba::xml_iarchive(ifs);
         } else {
            std::cerr << "ReadVistle: " << format << " is not supported" << std::endl;
            return false;
         }
      } catch(ba::archive_exception e) {

         if (ifs.eof()) {
            return true;
         }

         std::cerr << "ReadVistle: failed to initialize archive from stream, exception: " << e.what() << std::endl;
         return false;
      }

      try {
         Object::ptr obj;
         if (ba::binary_iarchive *bia = dynamic_cast<ba::binary_iarchive *>(ia)) {
            obj = Object::load(*bia);
            delete bia;
         } else if (ba::text_iarchive *tia = dynamic_cast<ba::text_iarchive *>(ia)) {
            obj = Object::load(*tia);
            delete tia;
         } else if (ba::xml_iarchive *xia = dynamic_cast<ba::xml_iarchive *>(ia)) {
            obj = Object::load(*xia);
            delete xia;
         } else {
            assert("add support for another archive format" == NULL);
            return false;
         }
         ia = NULL;
         if (obj) {
            addObject("grid_out", obj);
         } else {
            std::cerr << "Loading failed: no object" << std::endl;
         }
      } catch(ba::archive_exception e) {

         if (ifs.eof()) {
            return true;
         }

         std::cerr << "ReadVistle: archive exception: " << e.what() << std::endl;
         if (format != "binary") {
            std::string line;
            std::getline(ifs, line);
            std::cerr << "    next: '" << line << "'" << std::endl;
            std::getline(ifs, line);
            std::cerr << "    next: '" << line << "'" << std::endl;
            std::getline(ifs, line);
            std::cerr << "    next: '" << line << "'" << std::endl;
         }
         return false;
      }

      std::string line;
      while (line != "vistle separator") {
         std::getline(ifs, line);
      } 
   } while(!ifs.eof());

   return true;
}

bool ReadVistle::compute() {

   load(getStringParameter("filename"));

   return true;
}

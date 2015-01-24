/**\file
 * \brief class coRemoteCoviseInteractor
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013 HLRS
 *
 * \copyright GPL2+
 */

#ifndef CO_REMOTE_INTERACTOR_H
#define CO_REMOTE_INTERACTOR_H

#include <PluginUtil/coBaseCoviseInteractor.h>

namespace opencover {
class RenderObject;
}
class VncClient;

//! modify COVISE parameters from within COVER through VNC connection
class coRemoteCoviseInteractor: public opencover::coBaseCoviseInteractor
{
   private:
      VncClient *m_plugin; //< plugin for interfacing with COVER with COVISE connection, used in sendFeedback
      void sendFeedback(const char *info, const char *key, const char *data=NULL); //< send through VNC connection

   public:
      coRemoteCoviseInteractor(VncClient *plugin, const char *objectname, opencover::RenderObject *o, const char *attrib);
      ~coRemoteCoviseInteractor();

};
#endif

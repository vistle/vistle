/**\file
 * \brief class coRemoteCoviseInteractor
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013 HLRS
 *
 * \copyright GPL2+
 */

#include <util/common.h>
#include <cover/coVRPluginSupport.h>
#include <cover/coVRMSController.h>
#include <cover/RenderObject.h>

#include "coRemoteCoviseInteractor.h"
#include "VncClient.h"

#undef VERBOSE

using namespace covise;
using namespace opencover;

coRemoteCoviseInteractor::coRemoteCoviseInteractor(VncClient *plugin, const char *n, RenderObject *o, const char *attrib)
: coBaseCoviseInteractor(n, o, attrib)
, m_plugin(plugin)
{
}

coRemoteCoviseInteractor::~coRemoteCoviseInteractor()
{
}

void coRemoteCoviseInteractor::sendFeedback(const char *info, const char *keyword, const char *data)
{
   if (m_plugin)
      m_plugin->sendFeedback(info, keyword, data);
}

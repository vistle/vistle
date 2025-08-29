 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <map>
#include <utility>
#include "ThirdPartyHeadersEnd.h"
 #define _TEXT_H_INCLUDED
 #define VALID_TEXT_COORDSYS(___3918)  (((___3918)==CoordSys_Frame)||((___3918)==CoordSys_Grid)||((___3918)==CoordSys_Grid3D))
 #define VALID_TEXT_UNITS(___4264)  (((___4264)==___4267)||((___4264)==___4266)||((___4264)==___4269))
 #define VALID_TEXT_COORDSYS_AND_UNITS(___3169, ___3600) \
 ( VALID_TEXT_COORDSYS((___3169)) && \
 VALID_TEXT_UNITS((___3600)) && \
 ! ((___3169) == CoordSys_Frame && (___3600) == ___4267) )
 #define VALID_FONT_SIZEUNITS(___4264)  (((___4264)==___4267)||((___4264)==___4266)||((___4264)==___4269)||(___4264)==___4265)

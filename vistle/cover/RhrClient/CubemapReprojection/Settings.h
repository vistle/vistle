// CubemapStreaming_Client/Settings.h

#ifndef _CUBEMAPSTREAMING_CLIENT_SETTINGS_H_
#define _CUBEMAPSTREAMING_CLIENT_SETTINGS_H_

enum class ShaderId : unsigned
{
	GridRasterization, GridEdgeRasterization
};

// Double buffering the used texture.
const unsigned c_CountTextures = 2;

#endif
#ifndef VISTLE_ISOHEIGHTSURFACE_MAPHEIGHT_H
#define VISTLE_ISOHEIGHTSURFACE_MAPHEIGHT_H

#include <gdal_priv.h>

class MapHeight {
private:
    GDALDataset *heightDataset;
    GDALRasterBand *heightBand;

    float *rasterData = NULL;

    double xOrigin, yOrigin;
    double pixelWidth, pixelHeight;
    int cols, rows;

public:
    void openImage(std::string &name);
    void closeImage();
    MapHeight();
    ~MapHeight();
    float getAlt(double x, double y);
};

#endif

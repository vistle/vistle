#include "MapHeight.h"
#include <iostream>

MapHeight::MapHeight()
{
    GDALAllRegister();
}

MapHeight::~MapHeight()
{}

void MapHeight::openImage(std::string &name)
{
    heightDataset = (GDALDataset *)GDALOpen(name.c_str(), GA_ReadOnly);
    if (heightDataset != NULL) {
        int nBlockXSize, nBlockYSize;
        int bGotMin, bGotMax;
        double adfMinMax[2];
        double adfGeoTransform[6];

        printf("Size is %dx%dx%d\n", heightDataset->GetRasterXSize(), heightDataset->GetRasterYSize(),
               heightDataset->GetRasterCount());
        if (heightDataset->GetGeoTransform(adfGeoTransform) == CE_None) {
            printf("Origin = (%.6f,%.6f)\n", adfGeoTransform[0], adfGeoTransform[3]);
            printf("Pixel Size = (%.6f,%.6f)\n", adfGeoTransform[1], adfGeoTransform[5]);
        }

        heightBand = heightDataset->GetRasterBand(1);
        heightBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
        cols = heightDataset->GetRasterXSize();
        rows = heightDataset->GetRasterYSize();
        double transform[100];
        heightDataset->GetGeoTransform(transform);

        xOrigin = transform[0];
        yOrigin = transform[3];
        pixelWidth = transform[1];
        pixelHeight = -transform[5];
        delete[] rasterData;
        rasterData = new float[cols * rows];
        float *pafScanline;
        int nXSize = heightBand->GetXSize();
        pafScanline = (float *)CPLMalloc(sizeof(float) * nXSize);
        for (int i = 0; i < rows; i++) {
            if (heightBand->RasterIO(GF_Read, 0, i, nXSize, 1, pafScanline, nXSize, 1, GDT_Float32, 0, 0) ==
                CE_Failure) {
                std::cerr << "MapDrape::openImage: GDALRasterBand::RasterIO failed" << std::endl;
                break;
            }
            memcpy(&(rasterData[(i * cols)]), pafScanline, nXSize * sizeof(float));
        }

        if (heightBand->ReadBlock(0, 0, rasterData) == CE_Failure) {
            std::cerr << "MapDrape::openImage: GDALRasterBand::ReadBlock failed" << std::endl;
            return;
        }

        adfMinMax[0] = heightBand->GetMinimum(&bGotMin);
        adfMinMax[1] = heightBand->GetMaximum(&bGotMax);
        if (!(bGotMin && bGotMax))
            GDALComputeRasterMinMax((GDALRasterBandH)heightBand, TRUE, adfMinMax);

        printf("Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1]);
    }
    return;
}

void MapHeight::closeImage()
{
    if (heightDataset)
        GDALClose(heightDataset);
}

float MapHeight::getAlt(double x, double y)
{
    int col = int((x - xOrigin) / pixelWidth);
    int row = int((yOrigin - y) / pixelHeight);
    if (col < 0)
        col = 0;
    if (col >= cols)
        col = cols - 1;
    if (row < 0)
        row = 0;
    if (row >= rows)
        row = rows - 1;
    return rasterData[col + (row * cols)];
    float *pafScanline;
    int nXSize = heightBand->GetXSize();

    delete[] pafScanline;
    pafScanline = new float[nXSize];
    auto err = heightBand->RasterIO(GF_Read, x, y, 1, 1, pafScanline, nXSize, 1, GDT_Float32, 0, 0);
    float height = pafScanline[0];
    delete[] pafScanline;

    if (err != CE_None) {
        std::cerr << "MapDrape::getAlt: error" << std::endl;
        return 0.f;
    }

    return height;
}

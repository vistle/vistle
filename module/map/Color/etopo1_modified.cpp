/*

Downloadead from http://soliton.vm.bytemark.co.uk/pub/cpt-city/ngdc/tn/ETOPO1.png.index.html

# Colormap used in the ETOPO1 global relief map:
# http://ngdc.noaa.gov/mgg/global/global.html
#
# Above sea level is a modified version of GMT_globe.cpt, 
# Designed by Lester M. Anderson (CASP, UK) lester.anderson@casp.cam.ac.uk,
# Modified by Jesse Varner and Elliot Lim (NOAA/NGDC) with a smaller band of white for the highest elevations.
# The ocean colors are adapted from GMT_haxby.cpt, popularized by Bill Haxby, LDEO
# COLOR_MODEL = RGB

*/

#include "etopo1_modified.h"


/* cut off for symmetry
#-11000	10	0	121
#-10500	26	0	137
#-10000	38	0	152
#-9500	27	3	166
#-9000	16	6	180
#-8500	5	9	193
*/

const float etopo1_modified_data[36][4] = {
    {-8000, 0, 14, 203},   {-7500, 0, 22, 210},     {-7000, 0, 30, 216},   {-6500, 0, 39, 223},   {-6000, 12, 68, 231},
    {-5500, 26, 102, 240}, {-5000, 19, 117, 244},   {-4500, 14, 133, 249}, {-4000, 21, 158, 252}, {-3500, 30, 178, 255},
    {-3000, 43, 186, 255}, {-2500, 55, 193, 255},   {-2000, 65, 200, 255}, {-1500, 79, 210, 255}, {-1000, 94, 223, 255},
    {-500, 138, 227, 255}, {0.001f, 188, 230, 255}, {0.01f, 51, 102, 0},   {100, 51, 204, 102},   {200, 187, 228, 146},
    {500, 255, 220, 185},  {1000, 243, 202, 137},   {1500, 230, 184, 88},  {2000, 217, 166, 39},  {2500, 168, 154, 31},
    {3000, 164, 144, 25},  {3500, 162, 134, 19},    {4000, 159, 123, 13},  {4500, 156, 113, 7},   {5000, 153, 102, 0},
    {5500, 162, 89, 89},   {6000, 178, 118, 118},   {6500, 183, 147, 147}, {7000, 194, 176, 176}, {7500, 204, 204, 204},
    {8000, 229, 229, 229}};

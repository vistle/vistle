#ifndef VISTLE_COVER_HEIGHTMAP_H
#define VISTLE_COVER_HEIGHTMAP_H

#include <osg/Matrix>
#include <osg/Array>
#include <osg/Geode>
#include <osg/Camera>
#include <osg/Geometry>
#include <osg/TextureRectangle>

#include <memory>
#include <array>

// losely based on osg::HeightField
class HeightMap: public osg::Drawable {
public:
    enum DataMode {
        NoData,
        VertexData,
        CellData,
    };
    HeightMap();

    HeightMap(const HeightMap &mesh, const osg::CopyOp &copyop = osg::CopyOp::SHALLOW_COPY);

    void setup();
    void build();
    void drawImplementation(osg::RenderInfo & /*renderInfo*/) const override;
    osg::BoundingBox computeBoundingBox() const override;

    typedef std::vector<float> HeightList;

    void allocate(unsigned int numColumns, unsigned int numRows, DataMode mode = NoData);
    void dirty();

    inline unsigned int getNumColumns() const { return _columns; }
    inline unsigned int getNumRows() const { return _rows; }

    void setOrigin(const osg::Vec3 &origin);
    inline const osg::Vec3 &getOrigin() const { return _origin; }

    void setXInterval(float dx);
    inline float getXInterval() const { return _dx; }

    void setYInterval(float dy);
    inline float getYInterval() const { return _dy; }

    /** Set the width in number of cells in from the edge that the height field should be rendered from.
          * This exists to allow gradient and curvature continutity to be maintained between adjacent HeightMap, where
          * the border cells will overlap adjacent HeightMap.*/
    void setBorderWidth(const std::array<unsigned int, 4> &borderWidth)
    {
        _borderWidth = borderWidth;
        _dirty = true;
    }

    /** Get the width in number of cells in from the edge that the height field should be rendered from.*/
    const std::array<unsigned int, 4> &getBorderWidth() const { return _borderWidth; }

    float *getHeights() { return _heights; }
    const float *getHeights() const { return _heights; }

    float *getData() { return _data; }
    const float *getData() const { return _data; }

    /* set a single height point in the height field */
    inline void setHeight(unsigned int c, unsigned int r, float value) { _heights[c + r * _columns] = value; }

    /* Get address of single height point in the height field, allows user to change. */
    inline float &getHeight(unsigned int c, unsigned int r) { return _heights[c + r * _columns]; }

    /* Get value of single height point in the height field, not editable. */
    inline float getHeight(unsigned int c, unsigned int r) const { return _heights[c + r * _columns]; }

    inline osg::Vec3 getVertex(unsigned int c, unsigned int r) const
    {
        return osg::Vec3(_origin.x() + getXInterval() * (float)c, _origin.y() + getYInterval() * (float)r,
                         _origin.z() + _heights[c + r * _columns]);
    }

    osg::Vec3 getNormal(unsigned int c, unsigned int r) const;

    osg::Vec2 getHeightDelta(unsigned int c, unsigned int r) const;

protected:
    virtual ~HeightMap();

    unsigned int _columns = 0, _rows = 0;

    osg::Vec3 _origin; // _origin is the min value of the X and Y coordinates.
    float _dx;
    float _dy;

    std::array<unsigned int, 4> _borderWidth = {0, 0, 0, 0};
    bool _dirty = true;

    osg::ref_ptr<osg::Geometry> _geom;

    osg::ref_ptr<osg::Image> _heightImg;
    float *_heights = nullptr;
    osg::ref_ptr<osg::TextureRectangle> _dataTex;
    osg::ref_ptr<osg::Image> _dataImg;
    float *_data = nullptr;

    osg::ref_ptr<osg::Uniform> _dataValid;
    osg::ref_ptr<osg::Uniform> _sizeUni;
    osg::ref_ptr<osg::Uniform> _patchSizeUni;
    osg::ref_ptr<osg::Uniform> _distUni;
    osg::ref_ptr<osg::Uniform> _originUni;
};

#endif

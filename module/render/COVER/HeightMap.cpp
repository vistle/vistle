#include "HeightMap.h"
#include <osg/TextureRectangle>
#include <iostream>
#include <cassert>

using namespace osg;

const int DataTexUnit = 0;
const int HeightTexUnit = 2;

const unsigned PatchSize[] = {3, 3};

void HeightMap::setup()
{
    setSupportsDisplayList(false);
    _geom->setSupportsDisplayList(false);

    _geom->setUseDisplayList(false);
    _geom->setUseVertexBufferObjects(true);
    _xy = new osg::Vec2Array;
    _geom->setVertexArray(_xy);
    _geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, _xy->size()));

    auto state = getOrCreateStateSet();

    _heightImg = new osg::Image;
    _heightImg->setInternalTextureFormat(GL_LUMINANCE32F_ARB);
    auto heightTex = new osg::TextureRectangle;
    state->setTextureAttributeAndModes(HeightTexUnit, heightTex, osg::StateAttribute::ON);
    heightTex->setImage(_heightImg);
    heightTex->setDataVariance(osg::Object::STATIC);
    heightTex->setResizeNonPowerOfTwoHint(false);
    heightTex->setFilter(osg::TextureRectangle::MIN_FILTER, osg::TextureRectangle::NEAREST);
    heightTex->setFilter(osg::TextureRectangle::MAG_FILTER, osg::TextureRectangle::NEAREST);
    osg::Uniform *heightSampler = new osg::Uniform(osg::Uniform::SAMPLER_2D_RECT, "heightTex");
    heightSampler->set(HeightTexUnit);
    state->addUniform(heightSampler);

    _dataImg = new osg::Image;
    _dataImg->setInternalTextureFormat(GL_LUMINANCE32F_ARB);
    _dataTex = new osg::TextureRectangle;
    _dataTex->setDataVariance(osg::Object::STATIC);
    _dataTex->setResizeNonPowerOfTwoHint(false);
    _dataTex->setFilter(osg::TextureRectangle::MIN_FILTER, osg::TextureRectangle::NEAREST);
    _dataTex->setFilter(osg::TextureRectangle::MAG_FILTER, osg::TextureRectangle::NEAREST);
    _dataTex->setImage(_dataImg);
    osg::Uniform *dataSampler = new osg::Uniform(osg::Uniform::SAMPLER_2D_RECT, "dataTex");
    dataSampler->set(DataTexUnit);
    state->addUniform(dataSampler);
    _dataValid = new osg::Uniform(osg::Uniform::BOOL, "dataValid");
    _dataValid->set(false);
    state->addUniform(_dataValid);

    _sizeUni = new osg::Uniform(osg::Uniform::FLOAT_VEC2, "size");
    _sizeUni->set(osg::Vec2(_columns, _rows));
    state->addUniform(_sizeUni);

    _patchSizeUni = new osg::Uniform(osg::Uniform::FLOAT_VEC2, "patchSize");
    _patchSizeUni->set(osg::Vec2(PatchSize[0], PatchSize[1]));
    state->addUniform(_patchSizeUni);

    _distUni = new osg::Uniform(osg::Uniform::FLOAT_VEC2, "dist");
    _distUni->set(osg::Vec2(_dx, _dy));
    state->addUniform(_distUni);

    _originUni = new osg::Uniform(osg::Uniform::FLOAT_VEC4, "origin");
    _originUni->set(osg::Vec4(_origin.x(), _origin.y(), _origin.z(), 1.));
    state->addUniform(_originUni);

    setStateSet(state);
}

HeightMap::HeightMap(): _columns(0), _rows(0), _origin(0.0f, 0.0f, 0.0f), _dx(1.0f), _dy(1.0f)
{
    _geom = new osg::Geometry;

    setup();
}

HeightMap::HeightMap(const HeightMap &mesh, const CopyOp &copyop)
: _columns(mesh._columns)
, _rows(mesh._rows)
, _origin(mesh._origin)
, _dx(mesh._dx)
, _dy(mesh._dy)
, _borderWidth(mesh._borderWidth)
{
    _geom = new osg::Geometry(*mesh._geom, copyop);

    setup();
}

HeightMap::~HeightMap()
{}

void HeightMap::setOrigin(const osg::Vec3 &origin)
{
    _origin = origin;
    _originUni->set(osg::Vec4(_origin.x(), _origin.y(), _origin.z(), 1));
}

void HeightMap::setXInterval(float dx)
{
    _dx = dx;
    _distUni->set(osg::Vec2f(_dx, _dy));
}

void HeightMap::setYInterval(float dy)
{
    _dy = dy;
    _distUni->set(osg::Vec2f(_dx, _dy));
}

osg::BoundingBox HeightMap::computeBoundingBox() const
{
    if (!_heights)
        return osg::BoundingBox();

    float min = std::numeric_limits<float>::max();
    float max = std::numeric_limits<float>::lowest();

    for (auto h = _heights; h < _heights + _columns * _rows; ++h) {
        min = std::min(min, *h);
        max = std::max(max, *h);
    }

    return osg::BoundingBox(_origin + osg::Vec3(0, 0, min), _origin + osg::Vec3(_dx * _columns, _dy * _rows, max));
}

void HeightMap::allocate(unsigned int numColumns, unsigned int numRows, HeightMap::DataMode mode)
{
    if (_columns != numColumns || _rows != numRows) {
        if (numColumns >= 1 + _borderWidth[0] + _borderWidth[1] && numRows >= 1 + _borderWidth[2] + _borderWidth[3]) {
            _xy->resize(((numColumns - 1 - _borderWidth[0] - _borderWidth[1] + PatchSize[0] - 1) / PatchSize[0] *
                         (numRows - 1 - _borderWidth[2] - _borderWidth[3] + PatchSize[1] - 1) / PatchSize[1]));
        } else {
            _xy->clear();
        }

        _heightImg->allocateImage(numColumns, numRows, 1, GL_LUMINANCE, GL_FLOAT);
        _heights = (float *)_heightImg->data();

        switch (mode) {
        case VertexData:
            std::cerr << "HeightMap: allocating with vertex data: " << numColumns << "x" << numRows << std::endl;
            _dataImg->allocateImage(numColumns, numRows, 1, GL_LUMINANCE, GL_FLOAT);
            _data = (float *)_dataImg->data();
            break;
        case CellData:
            std::cerr << "HeightMap: allocating with cell data: " << numColumns << "x" << numRows << std::endl;
            if (numColumns > 1 && numRows > 1) {
                _dataImg->allocateImage(numColumns - 1, numRows - 1, 1, GL_LUMINANCE, GL_FLOAT);
                _data = (float *)_dataImg->data();
                break;
            }
        default:
            std::cerr << "HeightMap: allocating without data: " << numColumns << "x" << numRows << std::endl;
            _dataImg->allocateImage(0, 0, 0, GL_LUMINANCE, GL_FLOAT);
            _data = nullptr;
            break;
        }
        bool haveData = _data != nullptr;
        _dataValid->set(haveData);
        auto state = getOrCreateStateSet();
        state->setTextureAttributeAndModes(DataTexUnit, _dataTex,
                                           haveData ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
    }
    _columns = numColumns;
    _rows = numRows;
    _sizeUni->set(osg::Vec2f(_columns, _rows));
    _dirty = true;
}

void HeightMap::dirty()
{
    _dirty = true;
}

void HeightMap::build()
{
    if (!_dirty)
        return;

    _dataImg->dirty();
    _heightImg->dirty();

    unsigned idx = 0;
    for (unsigned int r = _borderWidth[2]; r < _rows - 1 - _borderWidth[3]; r += PatchSize[1]) {
        for (unsigned int c = _borderWidth[0]; c < _columns - 1 - _borderWidth[1]; c += PatchSize[0]) {
            (*_xy)[idx] = osg::Vec2(c, r);
            ++idx;
        }
    }
    _xy->dirty();

    _geom->setPrimitiveSet(0, new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, _xy->size()));
    _geom->dirtyGLObjects();

    _dirty = false;
}

Vec3 HeightMap::getNormal(unsigned int c, unsigned int r) const
{
    // four point normal generation.
    float dz_dx;
    if (c == 0) {
        dz_dx = (getHeight(c + 1, r) - getHeight(c, r)) / getXInterval();
    } else if (c == getNumColumns() - 1) {
        dz_dx = (getHeight(c, r) - getHeight(c - 1, r)) / getXInterval();
    } else // assume 0<c<_numColumns-1
    {
        dz_dx = 0.5f * (getHeight(c + 1, r) - getHeight(c - 1, r)) / getXInterval();
    }

    float dz_dy;
    if (r == 0) {
        dz_dy = (getHeight(c, r + 1) - getHeight(c, r)) / getYInterval();
    } else if (r == getNumRows() - 1) {
        dz_dy = (getHeight(c, r) - getHeight(c, r - 1)) / getYInterval();
    } else // assume 0<r<_numRows-1
    {
        dz_dy = 0.5f * (getHeight(c, r + 1) - getHeight(c, r - 1)) / getYInterval();
    }

    Vec3 normal(-dz_dx, -dz_dy, 1.0f);
    normal.normalize();

    return normal;
}

Vec2 HeightMap::getHeightDelta(unsigned int c, unsigned int r) const
{
    // four point height generation.
    Vec2 heightDelta;
    if (c == 0) {
        heightDelta.x() = (getHeight(c + 1, r) - getHeight(c, r));
    } else if (c == getNumColumns() - 1) {
        heightDelta.x() = (getHeight(c, r) - getHeight(c - 1, r));
    } else // assume 0<c<_numColumns-1
    {
        heightDelta.x() = 0.5f * (getHeight(c + 1, r) - getHeight(c - 1, r));
    }

    if (r == 0) {
        heightDelta.y() = (getHeight(c, r + 1) - getHeight(c, r));
    } else if (r == getNumRows() - 1) {
        heightDelta.y() = (getHeight(c, r) - getHeight(c, r - 1));
    } else // assume 0<r<_numRows-1
    {
        heightDelta.y() = 0.5f * (getHeight(c, r + 1) - getHeight(c, r - 1));
    }

    return heightDelta;
}

void HeightMap::drawImplementation(osg::RenderInfo &renderInfo) const
{
    assert(!_dirty);
    _geom->drawImplementation(renderInfo);
}

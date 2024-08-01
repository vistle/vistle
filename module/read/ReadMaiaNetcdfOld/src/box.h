#ifndef MAIAVISBOX_H_
#define MAIAVISBOX_H_

#include <array>
#include <cmath>
#include <vector>

#include "constants.h"
#include "INCLUDE/maiaconstants.h"
#include "UTIL/debug.h"
#include "point.h"
#include "INCLUDE/maiatypes.h"

namespace maiapv {

  using namespace maiapv;
/// \brief Class for efficiently obtaining a unique set of points.
///
/// \author Michael Schlottke (mic) <mic@aia.rwth-aachen.de>
/// \date 2015-04-20
template <MInt nDim> class Box {
 private:

 public:
  Box(const Point<nDim> min, const Point<nDim> max);
  MInt insert(const Point<nDim> point, std::vector<Point<nDim>>& points);
  MBool isPointInBox(const Point<nDim> p);
 private:
  Box(const Point<nDim> center, const Point<nDim> edge, const MBool child);
  void split();
  void addPointId(const MInt id);
  MInt childId(const Point<nDim> p) const;
  MInt pointId(const Point<nDim> p, std::vector<Point<nDim>>& points);
  MBool isSplit() const { return m_children.size() > 0; }
  Point<nDim> m_center = {};
  Point<nDim> m_length = {};
  const MFloat offset = 1e-12;

  static const MInt maxNoPoints = 20;
  std::array<MInt, maxNoPoints> m_pointIds = {};
  MInt m_noPoints = 0;

  std::vector<Box> m_children = {};
};


/// \brief Constructor for box class.
///
/// \author Michael Schlottke (mic) <mic@aia.rwth-aachen.de>
/// \date 2015-04-21
///
/// \param[in] min Minimum extent of box.
/// \param[in] max Maximum extent of box.
template <MInt nDim> Box<nDim>::Box(const Point<nDim> min, const Point<nDim> max) {
  // TRACE_IN();

  for (MInt d = 0; d < nDim; d++) {
    // FIXME offset quantity is used to avoid boundaries between two boxes with
    // points located on them.
    m_center[d] = 0.5 * (max[d] + min[d]) + offset;
    m_length[d] = std::fabs(max[d] - min[d]) + 2 * offset;
  }

  // TRACE_OUT();
}


/// \brief Insert point to box and point list (if not already present) and
///        return its id.
///
/// \author Michael Schlottke (mic) <mic@aia.rwth-aachen.de>
/// \date 2015-04-21
///
/// \param[in] p Point to add.
/// \param[in] points Unique list of points where new points are appended.
///
/// \return The index of the point in the points list.
template <MInt nDim>
MInt Box<nDim>::insert(const Point<nDim> p, std::vector<Point<nDim>>& points) {
  // TRACE_IN();

  MInt id = -1;
  if (isSplit()) {
    // If this box was already split into child boxes, insert into matching
    // child box
    id = m_children[childId(p)].insert(p, points);
  } else {
    // Otherwise check if point already exists
    id = pointId(p, points);

    // If point does not yet exist, try to add it to the current box
    if (id == -1) {
      if (m_noPoints < maxNoPoints) {
        // If there is still room, store with current box
        id = points.size();
        points.push_back(p);
        addPointId(id);
      } else {
        // Otherwise split box into child boxes
        split();

        // Move current points to child boxes
        for (MInt i = 0; i < m_noPoints; i++) {
          m_children[childId(points[m_pointIds[i]])].addPointId(m_pointIds[i]);
        }
        m_noPoints = 0;

        // Recursively try to add point again
        id = insert(p, points);
      }
    }
  }

  // TRACE_OUT();
  return id;
}


/// \brief Constructor for child boxes (private).
///
/// \author Michael Schlottke (mic) <mic@aia.rwth-aachen.de>
/// \date 2015-04-21
///
/// \param[in] center Center of child box.
/// \param[in] length Side lengths of child box.
/// \param[in] MBool Used only to overload on public constructor.
template <MInt nDim>
Box<nDim>::Box(const Point<nDim> center, const Point<nDim> length, MBool)
  : m_center(center), m_length(length) {
  // TRACE_IN();
  
  // TRACE_OUT();
}


/// \brief Split box into child boxes.
///
/// \author Michael Schlottke (mic) <mic@aia.rwth-aachen.de>
/// \date 2015-04-21
template <MInt nDim> void Box<nDim>::split() {
  // TRACE_IN();

  // Make room for child boxes (4 in 2D, 8 in 3D)
  m_children.reserve(IPOW2(nDim));

  // Determine center and side lengths of child boxes, then add new child box
  for (MInt i = 0; i < IPOW2(nDim); ++i) {
    // Initialize child center/side length from parent box
    Point<nDim> center(m_center);
    Point<nDim> length(m_length);

    for (MInt d = 0; d < nDim; d++) {
      // The child center equals the parent center, shifted by 1/4 of the side
      // length
      center[d] += 0.25 * m_length[d] * binaryId3D[d + (i * 3)];

      // The child side length is just half of its parent
      length[d] *= 0.5;
    }

    m_children.push_back(Box(center, length, true));
  }

  // TRACE_OUT();
}


/// \brief Add point it do list of points for this box.
///
/// \author Michael Schlottke (mic) <mic@aia.rwth-aachen.de>
/// \date 2015-04-21
///
/// \param[in] id Point it do add to box.
template <MInt nDim> void Box<nDim>::addPointId(const MInt id) {
  // TRACE_IN();

  m_pointIds[m_noPoints] = id;
  m_noPoints++;

  // TRACE_OUT();
}


/// \brief Return child box id where point (if already existing) is located.
///
/// \author Michael Schlottke (mic) <mic@aia.rwth-aachen.de>
/// \date 2015-04-21
///
/// \param[in] p Point in question.
///
/// \return The id of the child box.
template <MInt nDim> MInt Box<nDim>::childId(const Point<nDim> p) const {
  // TRACE_IN();

  MInt id = 0;
  for (MInt i = 0; i < nDim; i++) {
    if (p[i] > m_center[i]) {
      id += IPOW2(i);
    }
  }

  // TRACE_OUT();
  return id;
}


/// \brief Get id of point in point list.
///
/// \author Michael Schlottke (mic) <mic@aia.rwth-aachen.de>
/// \date 2015-04-21
///
/// \param[in] p Point to look for.
/// \param[in] points Unique list of points.
///
/// \return The id of the point in the list if found, -1 otherwise.
template <MInt nDim>
MInt Box<nDim>::pointId(const Point<nDim> p, std::vector<Point<nDim>>& points) {
  // TRACE_IN();

  MInt id = -1;
  if (isSplit()) {
    // If this box has child boxes, search for point in correct child box
    id = m_children[childId(p)].pointId(p, points);
  } else {
    // Otherwise iterate over points and do floating point comparisons to find
    // point
    for (MInt i = 0; i < m_noPoints; ++i) {
      MBool found = true;
      for (MInt d = 0; d < nDim; d++) {
        if (!approx(p[d], points[m_pointIds[i]][d], MFloatEps)) {
          found = false;
        }
      }

      // If point was found, exit loop
      if (found) {
        id = m_pointIds[i];
        break;
      }
    }
  }

  // TRACE_OUT();
  return id;
}

/** \brief checks if a point lies within the box
 *
 *  @author Pascal Meysonnat
 *  @date forgotten
 **/
template <MInt nDim>
MBool Box<nDim>::isPointInBox(const Point<nDim> p){
  MBool liesInside=true;
  for(MInt d=0; d<nDim; d++){
    MFloat minBox=m_center.at(d)-(m_length.at(d)/2.0);
    MFloat maxBox=m_center.at(d)+(m_length.at(d)/2.0);
    if(p.at(d)<minBox || p.at(d) > maxBox){
      liesInside=false;
      break;
    }
  }
  return liesInside;
}



} // namespace maiapv

#endif // ifndef MAIAVISBOX_H_

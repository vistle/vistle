#include <sstream>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <cmath>

#include <boost/mpl/for_each.hpp>

#include <vistle/core/object.h>
#include <vistle/core/triangles.h>
#include <vistle/core/normals.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>
#include <vistle/core/texture1d.h>
#include <vistle/core/unstr.h>
#include <vistle/core/points.h>
#include <vistle/core/lines.h>
#include <vistle/alg/objalg.h>

#include "ToTriangles.h"

MODULE_MAIN(ToTriangles)

using namespace vistle;

ToTriangles::ToTriangles(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    auto pin = createInputPort("grid_in", "geometry, e.g. tubes, spheres, polygons");
    auto pout = createOutputPort("grid_out", "converted to or approximated by triangles");
    linkPorts(pin, pout);

    p_transformSpheres =
        addIntParameter("transform_spheres", "also generate triangles for sphere impostors", false, Parameter::Boolean);

    addResultCache(m_resultCache);
}

ToTriangles::~ToTriangles()
{}

template<int Dim>
struct ReplicateData {
    DataBase::const_ptr object;
    DataBase::ptr &result;
    Index multiplicity = 0;
    Index nConn = 0;
    const Index *const cl = nullptr;
    Index nElem = 0;
    const Index *const el = nullptr;
    Index numMult = 0;
    const Index *const mult = nullptr;
    Index numTri = 0;
    Index nStart = 0, nEnd = 0;
    ReplicateData(DataBase::const_ptr obj, DataBase::ptr &result, Index mult, Index nConn, const Index *cl, Index nElem,
                  const Index *el, Index nStart, Index nEnd)
    : object(obj)
    , result(result)
    , multiplicity(mult)
    , nConn(nConn)
    , cl(cl)
    , nElem(nElem)
    , el(el)
    , nStart(nStart)
    , nEnd(nEnd)
    {
        assert(nElem == 0 || el);
    }
    ReplicateData(DataBase::const_ptr obj, DataBase::ptr &result, Index numMult, const Index *mult, Index numTri)
    : object(obj), result(result), numMult(numMult), mult(mult), numTri(numTri)
    {}
    template<typename S>
    void operator()(S)
    {
        typedef Vec<S, Dim> V;
        typename V::const_ptr in(V::as(object));
        if (!in)
            return;

        auto mapping = in->guessMapping();
        Index numIn = cl ? nConn : in->getSize();
        Index sz = numIn * multiplicity + nElem * (nStart + nEnd);
        if (mult) {
            if (mapping == DataBase::Element)
                sz = numTri;
            else
                sz = numMult;
        }
        typename V::ptr out(new V(sz));
        for (int i = 0; i < Dim; ++i) {
            auto din = &in->x(i)[0];
            auto dout = out->x(i).data();

            if (mult) {
                if (mapping == DataBase::Vertex) {
                    assert(nElem <= in->getSize());
                    for (Index j = 0; j < nElem; ++j) {
                        Index idx = j;
                        for (Index k = 0; k < mult[j]; ++k) {
                            assert(dout - out->x(i).data() < out->getSize());
                            *dout++ = din[idx];
                        }
                    }
                } else if (mapping == DataBase::Element) {
                    for (Index e = 0; e < numMult; ++e) {
                        Index idx = e;
                        for (Index k = 0; k < mult[e]; ++k) {
                            assert(dout - out->x(i).data() < out->getSize());
                            *dout++ = din[idx];
                        }
                    }
                }
            } else if (el) {
                if (mapping == DataBase::Vertex) {
                    for (Index e = 0; e < nElem; ++e) {
                        const Index start = el[e], end = el[e + 1];
                        Index idx = cl ? cl[start] : start;
                        for (Index k = 0; k < nStart; ++k) {
                            *dout++ = din[idx];
                        }
                        for (Index i = start; i < end; ++i) {
                            idx = cl ? cl[i] : i;
                            for (Index k = 0; k < multiplicity; ++k) {
                                *dout++ = din[idx];
                            }
                        }
                        // reuse idx
                        for (Index k = 0; k < nEnd; ++k) {
                            *dout++ = din[idx];
                        }
                    }
                } else if (mapping == DataBase::Element) {
                    for (Index e = 0; e < nElem; ++e) {
                        const Index start = el[e], end = el[e + 1];
                        for (Index k = 0; k < nStart + end - start + nEnd; ++k) {
                            *dout++ = din[e];
                        }
                    }
                }
            } else {
                for (Index j = 0; j < numIn; ++j) {
                    Index idx = cl ? cl[j] : j;
                    for (Index k = 0; k < multiplicity; ++k) {
                        *dout++ = din[idx];
                    }
                }
            }
        }
        result = out;
    }
};

DataBase::ptr replicateData(DataBase::const_ptr src, Index mult, Index nConn = 0, const Index *cl = nullptr,
                            Index nElem = 0, const Index *el = nullptr, Index nStart = 0, Index nEnd = 0)
{
    DataBase::ptr result;
    boost::mpl::for_each<Scalars>(ReplicateData<1>(src, result, mult, nConn, cl, nElem, el, nStart, nEnd));
    boost::mpl::for_each<Scalars>(ReplicateData<3>(src, result, mult, nConn, cl, nElem, el, nStart, nEnd));
    if (auto tex = Texture1D::as(src)) {
        auto vec1 = Vec<Scalar, 1>::as(Object::ptr(result));
        assert(vec1);
        auto result2 = tex->clone();
        result2->d()->x[0] = vec1->d()->x[0];
        result = result2;
    }
    return result;
}

DataBase::ptr replicateData(DataBase::const_ptr src, Index nnelem, const Index *mult, Index numTri)
{
    DataBase::ptr result;
    boost::mpl::for_each<Scalars>(ReplicateData<1>(src, result, nnelem, mult, numTri));
    boost::mpl::for_each<Scalars>(ReplicateData<3>(src, result, nnelem, mult, numTri));
    if (auto tex = Texture1D::as(src)) {
        auto vec1 = Vec<Scalar, 1>::as(Object::ptr(result));
        assert(vec1);
        auto result2 = tex->clone();
        result2->d()->x[0] = vec1->d()->x[0];
        result = result2;
    }
    return result;
}

bool ToTriangles::compute()
{
    auto container = expect<Object>("grid_in");
    auto split = splitContainerObject(container);
    auto data = split.mapped;
    auto obj = split.geometry;
    if (!obj) {
        return true;
    }

    vistle::Object::ptr result;
    if (auto entry = m_resultCache.getOrLock(container->getName(), result)) {
        bool perElement = data && data->guessMapping() == DataBase::Element;
        assert(!data || perElement || data->guessMapping() == DataBase::Vertex);

        bool radiusPerElement = false;
        vistle::Vec<Scalar>::const_ptr radius;
        auto lines = Lines::as(obj);
        if (lines)
            radius = lines->radius();
        auto points = Points::as(obj);
        if (points)
            radius = points->radius();
        if (radius) {
            radiusPerElement = radius->guessMapping(obj) == DataBase::Element;
        }

        // transform the rest, if possible
        Triangles::ptr tri;
        DataBase::ptr ndata;
        std::vector<Index> mult;
        bool useMultiplicity = false;

        // pass through triangles
        if (auto tri = Triangles::as(obj)) {
            if (data) {
                auto ndata = data->clone();
                updateMeta(ndata);
                result = ndata;
            } else {
                auto nobj = obj->clone();
                updateMeta(nobj);
                result = nobj;
            }
            return true;
        }

        if (auto poly = Polygons::as(obj)) {
            Index nelem = poly->getNumElements();
            Index nvert = poly->getNumCorners();
            Index ntri = nvert - 2 * nelem;

            if (perElement) {
                mult.reserve(ntri);
                useMultiplicity = true;
            }

            tri.reset(new Triangles(3 * ntri, 0));
            for (int i = 0; i < 3; ++i)
                tri->d()->x[i] = poly->d()->x[i];

            Index i = 0;
            auto el = &poly->el()[0];
            auto cl = &poly->cl()[0];
            auto tcl = &tri->cl()[0];
            for (Index e = 0; e < nelem; ++e) {
                const Index begin = el[e], end = el[e + 1];
                const Index N = end - begin;
                for (Index v = 0; v < N - 2; ++v) {
                    tcl[i++] = cl[begin];
                    tcl[i++] = cl[begin + v + 1];
                    tcl[i++] = cl[begin + v + 2];
                }
                if (perElement)
                    mult.push_back(N - 2);
            }
            assert(i == 3 * ntri);
        } else if (auto quads = Quads::as(obj)) {
            Index nelem = quads->getNumElements();
            Index nvert = quads->getNumCorners();
            Index ntri = nvert - 2 * nelem;
            assert(ntri == nelem * 2);

            tri.reset(new Triangles(3 * ntri, 0));
            for (int i = 0; i < 3; ++i)
                tri->d()->x[i] = quads->d()->x[i];

            const Index N = 4;
            Index i = 0;
            auto cl = &quads->cl()[0];
            auto tcl = &tri->cl()[0];
            for (Index e = 0; e < nelem; ++e) {
                const Index begin = e * N;
                for (Index v = 0; v < N - 2; ++v) {
                    tcl[i++] = cl[begin];
                    tcl[i++] = cl[begin + v + 1];
                    tcl[i++] = cl[begin + v + 2];
                }
            }
            assert(i == 3 * ntri);

            if (data && data->guessMapping() == DataBase::Element) {
                ndata = replicateData(data, 2);
            }
        } else if (points && radius && p_transformSpheres->getValue()) {
            const int NumLat = 8;
            const int NumLong = 13;
            static_assert(NumLat >= 3, "too few vertices");
            static_assert(NumLong >= 3, "too few vertices");
            Index TriPerSphere = NumLong * (NumLat - 2) * 2;
            Index CoordPerSphere = NumLong * (NumLat - 2) + 2;

            Index n = points->getNumPoints();
            auto x = &points->x()[0];
            auto y = &points->y()[0];
            auto z = &points->z()[0];
            auto r = &radius->x()[0];

            tri.reset(new Triangles(n * 3 * TriPerSphere, n * CoordPerSphere));
            auto tx = tri->x().data();
            auto ty = tri->y().data();
            auto tz = tri->z().data();
            auto ti = tri->cl().data();

            Normals::ptr norm(new Normals(n * CoordPerSphere));
            auto nx = norm->x().data();
            auto ny = norm->y().data();
            auto nz = norm->z().data();

            const float psi = M_PI / (NumLat - 1);
            const float phi = M_PI * 2 / NumLong;
            for (Index i = 0; i < n; ++i) {
                // create normals
                {
                    Index ci = i * CoordPerSphere;
                    // south pole
                    nx[ci] = ny[ci] = 0.f;
                    nz[ci] = -1.f;
                    ++ci;

                    float Psi = -M_PI * 0.5 + psi;
                    for (Index j = 0; j < NumLat - 2; ++j) {
                        float Phi = j * 0.5f * phi;
                        for (Index k = 0; k < NumLong; ++k) {
                            nx[ci] = sin(Phi) * cos(Psi);
                            ny[ci] = cos(Phi) * cos(Psi);
                            nz[ci] = sin(Psi);
                            ++ci;
                            Phi += phi;
                        }
                        Psi += psi;
                    }
                    // north pole
                    nx[ci] = ny[ci] = 0.f;
                    nz[ci] = 1.f;
                    ++ci;
                    assert(ci == (i + 1) * CoordPerSphere);
                }

                // create coordinates from normals
                for (Index ci = i * CoordPerSphere; ci < (i + 1) * CoordPerSphere; ++ci) {
                    tx[ci] = nx[ci] * r[i] + x[i];
                    ty[ci] = ny[ci] * r[i] + y[i];
                    tz[ci] = nz[ci] * r[i] + z[i];
                }

                // create index list
                {
                    Index ii = i * 3 * TriPerSphere;
                    // indices for ring around south pole
                    Index ci = i * CoordPerSphere + 1;
                    for (Index k = 0; k < NumLong; ++k) {
                        ti[ii++] = ci + k;
                        ti[ii++] = ci + (k + 1) % NumLong;
                        ti[ii++] = i * CoordPerSphere;
                    }

                    ci = i * CoordPerSphere + 1;
                    for (Index j = 0; j < NumLat - 3; ++j) {
                        for (Index k = 0; k < NumLong; ++k) {
                            ti[ii++] = ci + k;
                            ti[ii++] = ci + k + NumLong;
                            ti[ii++] = ci + (k + 1) % NumLong;
                            ti[ii++] = ci + (k + 1) % NumLong;
                            ti[ii++] = ci + k + NumLong;
                            ti[ii++] = ci + NumLong + (k + 1) % NumLong;
                        }
                        ci += NumLong;
                    }
                    assert(ci == i * CoordPerSphere + 1 + NumLong * (NumLat - 3));
                    assert(ci + NumLong + 1 == (i + 1) * CoordPerSphere);

                    // indices for ring around north pole
                    for (Index k = 0; k < NumLong; ++k) {
                        ti[ii++] = ci + (k + 1) % NumLong;
                        ti[ii++] = ci + k;
                        ti[ii++] = (i + 1) * CoordPerSphere - 1;
                    }
                    assert(ii == (i + 1) * 3 * TriPerSphere);

                    for (Index j = 0; j < 3 * TriPerSphere; ++j) {
                        assert(ti[i * 3 * TriPerSphere + j] >= i * CoordPerSphere);
                        assert(ti[i * 3 * TriPerSphere + j] < (i + 1) * CoordPerSphere);
                    }
                }
            }
            norm->setMeta(obj->meta());
            updateMeta(norm);
            tri->setNormals(norm);

            if (data) {
                ndata = replicateData(data, CoordPerSphere);
            }
        } else if (lines && radius) {
            const int NumSect = 7;
            static_assert(NumSect >= 3, "too few sectors");
            Index TriPerSection = NumSect * 2;

            const Index *cl = nullptr;
            Index numEl = lines->getNumElements();
            Index numPoint = lines->getNumCoords();
            Index numConn = lines->getNumCorners();
            if (numConn == 0) {
                numConn = numPoint;
            } else {
                cl = &lines->cl()[0];
            }
            auto x = &lines->x()[0];
            auto y = &lines->y()[0];
            auto z = &lines->z()[0];
            auto r = &radius->x()[0];
            auto el = &lines->el()[0];
            // we ignore connection style altogether and simplify start and end style
            auto startStyle = lines->startStyle();
            if (startStyle != Lines::Open) {
                startStyle = Lines::Flat;
            }
            auto endStyle = lines->endStyle();
            if (endStyle != Lines::Arrow && endStyle != Lines::Open) {
                endStyle = Lines::Flat;
            }

            Index numCoordStart = 0, numCoordEnd = 0;
            Index numIndStart = 0, numIndEnd = 0;
            if (startStyle == Lines::Flat) {
                numCoordStart = 1 + NumSect;
                numIndStart = 3 * NumSect;
            }
            if (endStyle == Lines::Arrow) {
                numCoordEnd = 3 * NumSect;
                numIndEnd = 3 * 3 * NumSect;
            } else if (endStyle == Lines::Flat) {
                numCoordEnd = 1 + NumSect;
                numIndEnd = 3 * NumSect;
            }

            const Index numSeg = (numConn - numEl);
            const Index numVert = numSeg * 3 * TriPerSection + numEl * (numIndStart + numIndEnd);
            const Index numCoord = numVert > 0 ? numConn * NumSect + numEl * (numCoordStart + numCoordEnd) : 0;

            if (perElement) {
                mult.reserve(numEl);
                useMultiplicity = true;
            }

            tri.reset(new Triangles(numVert, numCoord));
            auto tx = tri->x().data();
            auto ty = tri->y().data();
            auto tz = tri->z().data();
            auto ti = tri->cl().data();

            Normals::ptr norm(new Normals(numCoord));
            assert(norm->getSize() == tri->getSize());
            auto nx = norm->x().data();
            auto ny = norm->y().data();
            auto nz = norm->z().data();

            Index ci = 0; // coord index
            Index ii = 0; // index index
            if (numCoord > 0) {
                for (Index i = 0; i < numEl; ++i) {
                    const Index begin = el[i], end = el[i + 1];
                    if (perElement) {
                        Index ntri = numIndStart / 3 + (end - begin - 1) * TriPerSection + numIndEnd / 3;
                        mult.push_back(ntri);
                    }

                    Vector3 normal, dir;
                    for (Index k = begin; k < end; ++k) {
                        Index idx = cl ? cl[k] : k;
                        Index nidx = (cl && k + 1 < end) ? cl[k + 1] : k + 1;
                        Index pidx = (cl && k > 0) ? cl[k - 1] : k - 1;
                        auto curRad = radiusPerElement ? r[i] : r[idx];
                        Vector3 cur(x[idx], y[idx], z[idx]);
                        Vector3 next = k + 1 < end ? Vector3(x[nidx], y[nidx], z[nidx]) : cur;

                        Vector3 l1 = next - cur;
                        auto len1 = l1.norm(), len2 = Scalar();
                        bool first = false, last = false;
                        if (k == begin) {
                            first = true;
                            dir = l1.normalized();
                        } else if (k + 1 == end) {
                            last = true;
                            // keep previous direction for final segment
                        } else {
                            Vector3 l2(x[idx] - x[pidx], y[idx] - y[pidx], z[idx] - z[pidx]);
                            len2 = l2.norm();
                            if (len2 > 100 * len1) {
                                dir = l2.normalized();
                            } else if (len1 > 100 * len2) {
                                dir = l1.normalized();
                            } else {
                                dir = (l1.normalized() + l2.normalized()).normalized();
                            }
                        }

                        if (first || normal.norm() < Scalar(0.5)) {
                            normal = dir.cross(Vector3(0, 0, 1)).normalized();
                            if (normal.norm() < Scalar(0.5)) {
                                // try another direction
                                normal = dir.cross(Vector3(0, 1, 0)).normalized();
                            }
                        } else if (len1 > 1e-4 || len2 > 1e-4) {
                            normal = (normal - dir.dot(normal) * dir).normalized();
                        }

                        Quaternion qrot(AngleAxis(2. * M_PI / NumSect, dir));
                        const auto rot = qrot.toRotationMatrix();
                        const auto rot2 = Quaternion(AngleAxis(M_PI / NumSect, dir)).toRotationMatrix();

                        // start cap
                        if (first && startStyle == Lines::Flat) {
                            tx[ci] = cur[0];
                            ty[ci] = cur[1];
                            tz[ci] = cur[2];
                            nx[ci] = dir[0];
                            ny[ci] = dir[1];
                            nz[ci] = dir[2];
                            ++ci;

                            for (Index l = 0; l < NumSect; ++l) {
                                ti[ii++] = ci - 1;
                                ti[ii++] = ci + l;
                                ti[ii++] = ci + (l + 1) % NumSect;
                            }

                            Vector3 rad = normal;
                            for (Index l = 0; l < NumSect; ++l) {
                                nx[ci] = dir[0];
                                ny[ci] = dir[1];
                                nz[ci] = dir[2];
                                Vector3 p = cur + curRad * rad;
                                rad = rot * rad;
                                tx[ci] = p[0];
                                ty[ci] = p[1];
                                tz[ci] = p[2];
                                ++ci;
                            }
                        }

                        // indices
                        if (!last) {
                            for (Index l = 0; l < NumSect; ++l) {
                                ti[ii++] = ci + l;
                                ti[ii++] = ci + (l + 1) % NumSect;
                                ti[ii++] = ci + (l + 1) % NumSect + NumSect;
                                ti[ii++] = ci + l;
                                ti[ii++] = ci + (l + 1) % NumSect + NumSect;
                                ti[ii++] = ci + l + NumSect;
                            }
                        }

                        // coordinates and normals
                        auto n = normal;
                        for (Index l = 0; l < NumSect; ++l) {
                            nx[ci] = n[0];
                            ny[ci] = n[1];
                            nz[ci] = n[2];
                            Vector3 p = cur + curRad * n;
                            n = rot * n;
                            tx[ci] = p[0];
                            ty[ci] = p[1];
                            tz[ci] = p[2];
                            ++ci;
                        }

                        // end cap/arrow
                        if (last && endStyle != Lines::Open) {
                            if (endStyle == Lines::Arrow) {
                                Index tipStart = ci;
                                for (Index l = 0; l < NumSect; ++l) {
                                    tx[ci] = tx[ci - NumSect];
                                    ty[ci] = ty[ci - NumSect];
                                    tz[ci] = tz[ci - NumSect];
                                    nx[ci] = dir[0];
                                    ny[ci] = dir[1];
                                    nz[ci] = dir[2];
                                    ++ci;
                                }

                                Scalar tipSize = 2.0;

                                Vector3 n = normal;
                                Vector3 tip = cur + tipSize * dir * curRad;
                                for (Index l = 0; l < NumSect; ++l) {
                                    Vector3 norm = (n + dir).normalized();
                                    Vector3 p = cur + tipSize * curRad * n;
                                    n = rot * n;

                                    nx[ci] = norm[0];
                                    ny[ci] = norm[1];
                                    nz[ci] = norm[2];
                                    tx[ci] = p[0];
                                    ty[ci] = p[1];
                                    tz[ci] = p[2];
                                    ++ci;
                                }

                                n = rot2 * normal;
                                for (Index l = 0; l < NumSect; ++l) {
                                    Vector3 norm = (n + dir).normalized();
                                    n = rot * n;

                                    nx[ci] = norm[0];
                                    ny[ci] = norm[1];
                                    nz[ci] = norm[2];
                                    tx[ci] = tip[0];
                                    ty[ci] = tip[1];
                                    tz[ci] = tip[2];
                                    ++ci;
                                }

                                for (Index l = 0; l < NumSect; ++l) {
                                    ti[ii++] = tipStart + l;
                                    ti[ii++] = tipStart + (l + 1) % NumSect;
                                    ti[ii++] = tipStart + NumSect + (l + 1) % NumSect;

                                    ti[ii++] = tipStart + NumSect + (l + 1) % NumSect;
                                    ti[ii++] = tipStart + NumSect + l;
                                    ti[ii++] = tipStart + l;

                                    ti[ii++] = tipStart + NumSect + l;
                                    ti[ii++] = tipStart + NumSect + (l + 1) % NumSect;
                                    ti[ii++] = tipStart + 2 * NumSect + l;
                                }
                            } else if (endStyle == Lines::Flat) {
                                for (Index l = 0; l < NumSect; ++l) {
                                    tx[ci] = tx[ci - NumSect];
                                    ty[ci] = ty[ci - NumSect];
                                    tz[ci] = tz[ci - NumSect];
                                    nx[ci] = dir[0];
                                    ny[ci] = dir[1];
                                    nz[ci] = dir[2];
                                    ++ci;
                                }

                                tx[ci] = cur[0];
                                ty[ci] = cur[1];
                                tz[ci] = cur[2];
                                nx[ci] = dir[0];
                                ny[ci] = dir[1];
                                nz[ci] = dir[2];
                                for (Index l = 0; l < NumSect; ++l) {
                                    ti[ii++] = ci - NumSect + l;
                                    ti[ii++] = ci - NumSect + (l + 1) % NumSect;
                                    ti[ii++] = ci;
                                }
                                ++ci;
                            }
                        }
                    }
                }
            }
            assert(ci == numCoord);
            assert(ii == numVert);

            norm->setMeta(obj->meta());
            updateMeta(norm);
            tri->setNormals(norm);

            if (data && !useMultiplicity) {
                ndata = replicateData(data, NumSect, numConn, cl, numEl, el, numCoordStart, numCoordEnd);
            }
        } else if (auto unstr = UnstructuredGrid::as(obj)) {
            Index nelem = unstr->getNumElements();
            auto el = &unstr->el()[0];
            auto cl = &unstr->cl()[0];
            auto tl = &unstr->tl()[0];

            if (perElement) {
                mult.reserve(nelem);
                useMultiplicity = true;
            }
            Index ntri = 0;
            for (Index e = 0; e < nelem; ++e) {
                if (tl[e] == UnstructuredGrid::TRIANGLE) {
                    ++ntri;
                    if (perElement)
                        mult.push_back(1);
                } else if (tl[e] == UnstructuredGrid::QUAD) {
                    ntri += 2;
                    if (perElement)
                        mult.push_back(2);
                } else {
                    mult.push_back(0);
                }
            }

            tri.reset(new Triangles(3 * ntri, 0));
            for (int i = 0; i < 3; ++i)
                tri->d()->x[i] = unstr->d()->x[i];

            Index i = 0;
            auto tcl = &tri->cl()[0];
            for (Index e = 0; e < nelem; ++e) {
                const Index begin = el[e], end = el[e + 1];
                if (tl[e] == UnstructuredGrid::TRIANGLE) {
                    assert(end - begin == 3);
                    for (Index v = begin; v < end; ++v) {
                        tcl[i++] = cl[v];
                    }
                } else if (tl[e] == UnstructuredGrid::QUAD) {
                    assert(end - begin == 4);
                    for (Index v = begin; v < begin + 3; ++v) {
                        tcl[i++] = cl[v];
                    }
                    tcl[i++] = cl[begin];
                    tcl[i++] = cl[begin + 2];
                    tcl[i++] = cl[begin + 3];
                }
            }
            assert(i == 3 * ntri);
        }

        if (tri) {
            tri->copyAttributes(obj);
            updateMeta(tri);

            if (data) {
                if (useMultiplicity) {
                    ndata = replicateData(data, mult.size(), mult.data(), tri->getNumElements());
                }
                if (!ndata) {
                    ndata = data->clone();
                }
                ndata->copyAttributes(data);
                ndata->setMapping(data->guessMapping());
                ndata->setGrid(tri);
                updateMeta(ndata);
                result = ndata;
            } else {
                result = tri;
            }
        } else {
            auto out = obj->clone();
            updateMeta(out);
            if (data) {
                auto ndata = data->clone();
                ndata->setGrid(out);
                updateMeta(ndata);
                result = ndata;
            } else {
                result = out;
            }
        }

        m_resultCache.storeAndUnlock(entry, result);
    }

    addObject("grid_out", result);

    return true;
}

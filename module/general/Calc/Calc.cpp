#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/normals.h>
#include <vistle/core/coords.h>
#include <vistle/alg/objalg.h>

#include "Calc.h"
#include <vistle/util/enum.h>

#define exprtk_enable_debugging
#define exprtk_disable_caseinsensitivity
#define exprtk_disable_string_capabilities
#define exprtk_disable_rtl_io_file
#include "exprtk/exprtk.hpp"


DEFINE_ENUM_WITH_STRING_CONVERSIONS(OutputType, (AsInput)(AsInputGrid)(Vec1)(Vec3))

typedef double Precision;

template<typename S>
class CalcExpression {
public:
    typedef exprtk::symbol_table<S> symbol_table_t;
    typedef exprtk::expression<S> expression_t;
    typedef exprtk::parser<S> parser_t;

    //! Empty constructor
    CalcExpression()
    {
        m_symbols.add_variable("i", m_index);
        m_symbols.add_variable("idx", m_index);
        m_symbols.add_variable("index", m_index);

        m_symbols.add_variable("x", m_position[0]);
        m_symbols.add_variable("y", m_position[1]);
        m_symbols.add_variable("z", m_position[2]);
        m_symbols.add_variable("p.x", m_position[0]);
        m_symbols.add_variable("p.y", m_position[1]);
        m_symbols.add_variable("p.z", m_position[2]);
        m_symbols.add_vector("p", &m_position[0], 3);

        m_resultDim = 1;
        m_symbols.add_vector("result", m_result.data(), m_resultDim);
        m_symbols.add_variable("result.x", m_result[0]);
    }

    void setExpr(const std::string &expr)
    {
        m_compiled = false;
        m_expr = expr;
    }

    void setConstant(const std::string &name, const S value)
    {
        m_compiled = false;
        m_symbols.add_constant(name, value);
    }

    bool addVector(const std::string &name, std::vector<S> &val)
    {
        m_compiled = false;
        m_vector[name] = &val;
        auto dim = val.size();
        m_symbols.add_vector(name, &val[0], dim);
        if (dim >= 1)
            m_symbols.add_variable(name + ".x", val[0]);
        if (dim >= 2)
            m_symbols.add_variable(name + ".y", val[1]);
        if (dim >= 3)
            m_symbols.add_variable(name + ".z", val[2]);
        if (dim >= 4)
            m_symbols.add_variable(name + ".w", val[3]);
        return true;
    }

    void addAlias(const std::string &name, const std::string &alias)
    {
        m_compiled = false;
        auto *val = m_vector[name];
        unsigned dim = val->size();
        m_symbols.add_vector(alias, val->data(), dim);
        if (dim >= 1)
            m_symbols.add_variable(alias + ".x", val->at(0));
        if (dim >= 2)
            m_symbols.add_variable(alias + ".y", val->at(1));
        if (dim >= 3)
            m_symbols.add_variable(alias + ".z", val->at(2));
        if (dim >= 4)
            m_symbols.add_variable(alias + ".w", val->at(3));
    }

    void setIndex(vistle::Index i) { m_index = i; }
    void setPosition(const vistle::Vector3 &v) { std::copy(v.begin(), v.end(), m_position.begin()); }
    void setResultDimension(size_t dim)
    {
        m_compiled = false;
        m_resultDim = dim;
        m_symbols.remove_vector("result");
        m_symbols.add_vector("result", m_result.data(), m_resultDim);
        if (dim >= 1)
            m_symbols.add_variable("result.x", m_result[0]);
        else
            m_symbols.remove_variable("result.x");
        if (dim >= 2)
            m_symbols.add_variable("result.y", m_result[1]);
        else
            m_symbols.remove_variable("result.y");
        if (dim >= 3)
            m_symbols.add_variable("result.z", m_result[2]);
        else
            m_symbols.remove_variable("result.z");
    }


    bool compile()
    {
        m_compiled = false;
        m_error.clear();
        m_expression.register_symbol_table(m_symbols);
        if (m_parser.compile(m_expr, m_expression)) {
            m_compiled = true;
        } else {
            m_error = m_parser.error();
            std::cerr << "expression error: " << m_error << std::endl;
        }
        return m_compiled;
    }

    bool evaluate()
    {
        m_returned.clear();

        if (!m_compiled)
            compile();
        if (!m_compiled)
            return false;
        std::fill(m_result.begin(), m_result.end(), std::numeric_limits<S>::signaling_NaN());
        S val = m_expression.value();
        if (std::isnan(m_result[0])) {
            m_result[0] = val;
            for (unsigned d = 1; d < m_result.size(); ++d)
                m_result[d] = S(0);
        }
        if (m_expression.results().count() > 0) {
            typedef exprtk::results_context<S> results_context_t;
            typedef typename results_context_t::type_store_t type_t;
            typedef typename type_t::scalar_view scalar_t;
            typedef typename type_t::vector_view vector_t;
            typedef typename type_t::string_view string_t;

            const results_context_t &results = m_expression.results();
            bool onlyScalar = true;
            for (std::size_t i = 0; i < results.count(); ++i) {
                type_t t = results[i];
                if (t.type != type_t::e_scalar)
                    onlyScalar = false;
            }

            size_t count = 1;
            if (onlyScalar) {
                count = results.count();
            } else if (results.count() > 1) {
                m_error = "ignoring non-scalar results";
            }
            m_returned.resize(count);
            for (std::size_t i = 0; i < count; ++i) {
                type_t t = results[i];

                switch (t.type) {
                case type_t::e_scalar:
                    m_returned[i] = scalar_t(t)();
                    break;

                case type_t::e_vector: {
                    auto vec = vector_t(t);
                    m_returned.resize(vec.size());
                    for (size_t i = 0; i < m_returned.size(); ++i) {
                        m_returned[i] = vec[i];
                    }
                } break;

                case type_t::e_string: {
                    auto str = string_t(t);
                    m_error = "string result: ";
                    for (size_t i = 0; i < str.size(); ++i) {
                        m_error += str[i];
                    }
                    return false;
                }

                default:
                    m_error = "unknown result type";
                    return false;
                }
            }
        }
        return m_compiled;
    }

    std::string error() { return m_error; }

    std::map<std::string, S> m_constants;
    std::map<const S *, S> m_arrays;
    std::map<std::string, std::vector<S> *> m_vector;
    std::array<S, 3> m_result;
    std::vector<S> m_returned;


    size_t m_resultDim = 1;
    const std::array<S, 3> &result() { return m_result; }
    std::string m_expr;
    std::string m_error;
    S m_index = 0;
    std::array<S, 3> m_position;

    bool m_compiled = false;
    symbol_table_t m_symbols;
    expression_t m_expression;
    parser_t m_parser;
};


using namespace vistle;

Calc::Calc(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    for (int i = 0; i < NumPorts; ++i) {
        m_dataIn[i] = createInputPort("data_in" + std::to_string(i), "input data");
    }
    m_dataOut = createOutputPort("data_out", "output data");

    m_dataTerm = addStringParameter(
        "formula", "formula for computing data with exprtk (use result := {x,y,z} for vector output)", "d0");
    m_gridTerm = addStringParameter("grid_formula", "formula for computing grid coordinates", "");
    m_normTerm = addStringParameter("normal_formula", "formula for computing grid normals", "");
    m_outputType = addIntParameter("output_type", "type of output", AsInput, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_outputType, OutputType);
    m_species = addStringParameter("species", "species of output data", "computed");
}

Calc::~Calc()
{}

bool Calc::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr oin[NumPorts];
    DataBase::const_ptr din[NumPorts];
    Object::const_ptr grid;
    Normals::const_ptr normals;

    int timestep = -1;
    std::string attr;
    Index nvert = InvalidIndex;
    double time = 0.;

    const GeometryInterface *geomInterface = nullptr;

    DataBase::Mapping mapping = DataBase::Unspecified;
    for (int i = 0; i < NumPorts; ++i) {
        if (!m_dataIn[i]->isConnected())
            continue;

        oin[i] = task->expect<Object>(m_dataIn[i]);
        auto split = splitContainerObject(oin[i]);
        din[i] = split.mapped;
        if (din[i]) {
            auto m = din[i]->guessMapping();
            if (mapping == DataBase::Unspecified) {
                mapping = m;
            } else if (m != DataBase::Unspecified && mapping != m) {
                sendError("Grids have inconsistent mapping");
                return true;
            }
            if (din[i]->guessMapping() != DataBase::Vertex) {
                din[i] = nullptr;
                continue;
            }

            if (time == 0.) {
                time = getRealTime(din[i]);
            }
        }
        if (timestep == -1) {
            timestep = split.timestep;
        }
        Object::const_ptr g = split.geometry;
        if (grid) {
            if (*g != *grid) {
                sendError("Grids on input ports do not match");
                return true;
            }
        } else if (!g) {
            sendError("Input does not have a grid on port %s", m_dataIn[i]->getName().c_str());
            return true;
        } else {
            grid = g;
            normals = split.normals;
            geomInterface = grid->getInterface<GeometryInterface>();
            if (geomInterface) {
                nvert = geomInterface->getNumVertices();
            }
        }
    }

    if (nvert == InvalidIndex) {
        sendError("Could not determine number of input vertices");
        return true;
    }
    assert(geomInterface);

    typedef CalcExpression<Precision> CE;
    std::array<std::vector<Precision>, NumPorts> data;
    for (unsigned p = 0; p < NumPorts; ++p) {
        if (din[p])
            data[p].resize(din[p]->dimension());
    }
    auto setUpExpr = [&](const std::string &name, const std::string &term, int outdim) -> std::unique_ptr<CE> {
        if (term.empty())
            return nullptr;

        auto expr = std::make_unique<CE>();
        expr->setExpr(term);

        expr->setConstant("timestep", timestep);
        expr->setConstant("step", timestep);
        expr->setConstant("t", time);
        expr->setConstant("time", time);
        expr->setConstant("rank", rank());
        expr->setConstant("block", din[0]->getBlock());

        for (unsigned p = 0; p < NumPorts; ++p) {
            if (!din[p])
                continue;
            const std::string name = "d" + std::to_string(p);
            expr->addVector(name, data[p]);
            if (p == 0) {
                expr->addAlias(name, "d");
            }
        }

        expr->setConstant("outdim", outdim);
        expr->setResultDimension(outdim);

        if (!expr->compile()) {
            sendError("Syntax error in %s: %s", name.c_str(), expr->error().c_str());
            return nullptr;
        }

        return expr;
    };

    Normals::ptr normOut;
    std::string nterm = m_normTerm->getValue();
    std::unique_ptr<CE> normExpr;
    if (!nterm.empty()) {
        auto N = normals->clone();
        N->resetArrays();
        N->setSize(nvert);
        normOut = N;

        normExpr = setUpExpr("normals term", nterm, 3);
    }

    Object::const_ptr gridOut = grid;
    Coords::ptr coordsOut;
    std::string gterm = m_gridTerm->getValue();
    std::vector<std::vector<Precision> *> gdata(NumPorts);
    std::unique_ptr<CE> gridExpr;
    if (!gterm.empty() && Coords::as(grid)) {
        auto cin = Coords::as(grid);
        coordsOut = cin->clone();
        coordsOut->resetArrays();
        coordsOut->setSize(nvert);
        gridOut = coordsOut;
        if (normOut) {
            coordsOut->setNormals(normOut);
        } else {
            coordsOut->setNormals(normals);
        }

        gridExpr = setUpExpr("coordinate term", gterm, 3);
    } else if (normOut) {
        auto G = grid->clone();
        if (auto C = Coords::as(G)) {
            C->setNormals(normOut);
            gridOut = G;
        } else {
            sendInfo("Ignoring computed normals: no Coords in grid");
            gridOut = grid;
        }
    } else {
        gridOut = grid;
    }

    std::string dterm = m_dataTerm->getValue();
    std::unique_ptr<CE> dataExpr;
    int outdim = 0;
    DataBase::ptr dout;
    if (mapping == DataBase::Vertex && !dterm.empty()) {
        Object::ptr out;
        switch (m_outputType->getValue()) {
        case AsInput:
        case AsInputGrid:
            if (m_outputType->getValue() == AsInput) {
                out = oin[0]->clone();
            } else {
                out = grid->clone();
                outdim = 3;
            }
            if (auto dout = DataBase::as(out)) {
                dout->resetArrays();
                dout->setSize(nvert);
                outdim = dout->dimension();
            }
            break;
        case Vec1:
            outdim = 1;
            out.reset(new Vec<Scalar, 1>(nvert));
            break;
        case Vec3:
            outdim = 3;
            out.reset(new Vec<Scalar, 3>(nvert));
            break;
        }

        dout = DataBase::as(out);
        if (dout) {
            dout->setMapping(DataBase::Vertex);
            outdim = dout->dimension();
            dataExpr = setUpExpr("data term", dterm, outdim);
        }
    }

    struct E {
        CE *e;
        DataBase *o;
    };
    E eg{gridExpr.get(), coordsOut.get()};
    E ed{dataExpr.get(), dout.get()};
    E en{normExpr.get(), normOut.get()};
    if (dataExpr || normExpr || gridExpr) {
        for (Index i = 0; i < nvert; ++i) {
            auto v = geomInterface->getVertex(i);

            for (unsigned p = 0; p < NumPorts; ++p) {
                for (unsigned d = 0; d < data[p].size(); ++d) {
                    data[p][d] = din[p]->value(i, d);
                }
            }

            for (auto &e: {ed, eg, en}) {
                if (!e.e)
                    continue;

                for (unsigned p = 0; p < NumPorts; ++p) {
                    if (!din[p])
                        continue;
                }

                e.e->setIndex(i);
                e.e->setPosition(v);

                if (e.e->evaluate()) {
                    auto outdim = e.o->dimension();
                    for (unsigned c = 0; c < outdim; ++c) {
                        if (e.e->result().size() > c)
                            e.o->setValue(i, c, e.e->result()[c]);
                        else
                            e.o->setValue(i, c, 0);
                    }
                }
            }
        }
    }

    if (dout) {
        dout->setGrid(gridOut);
        dout->copyAttributes(din[0]);
        dout->addAttribute("_species", m_species->getValue());

        updateMeta(dout);
        task->addObject("data_out", dout);
    } else {
        auto nobj = din[0]->clone();
        nobj->setGrid(gridOut);

        updateMeta(nobj);
        task->addObject("data_out", nobj);
    }

    return true;
}

bool Calc::prepare()
{
    return true;
}

bool Calc::reduce(int t)
{
    return true;
}

MODULE_MAIN(Calc)

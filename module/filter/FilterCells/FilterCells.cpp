#include <sstream>
#include <iomanip>


#include "FilterCells.h"

#include <vistle/core/object.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>
#include <vistle/util/enum.h>
#include <vistle/alg/objalg.h>
#include <vistle/util/enum.h>

using namespace vistle;

Symbols::Symbols()
{
    for (size_t i = UnstructuredGrid::NONE; i < UnstructuredGrid::NUM_TYPES; i++) {
        auto name = UnstructuredGrid::toString(static_cast<UnstructuredGrid::Type>(i));
        if (strcmp(name, "") == 0)
            continue;
        m_symbolTable.add_constant(name, i);
        name = UnstructuredGrid::toString(static_cast<UnstructuredGrid::Type>(i), true);
        m_symbolTable.add_constant(name, i);
    }
    for (const auto &var: SUPPORTED_VARIABLES) {
        m_symbolTable.add_variable(var.name.data(), m_symbolValues[&var - &SUPPORTED_VARIABLES[0]]);
    }
}

std::vector<std::string> Symbols::getSupportedSymbols()
{
    std::vector<std::string> symbols;
    m_symbolTable.get_variable_list(symbols);
    return symbols;
}

std::vector<std::string> Symbols::getSupportedVariables()
{
    auto vars = getSupportedSymbols();
    for (auto it = vars.begin(); it != vars.end();) {
        if (m_symbolTable.is_constant_node(*it)) {
            it = vars.erase(it);
        } else {
            ++it;
        }
    }
    return vars;
}
std::vector<std::string> Symbols::getSupportedConstants()
{
    auto constants = getSupportedSymbols();
    for (auto it = constants.begin(); it != constants.end();) {
        if (!m_symbolTable.is_constant_node(*it)) {
            it = constants.erase(it);
        } else {
            ++it;
        }
    }
    return constants;
}

bool Symbols::update(const std::string &expression, std::string &error)
{
    m_expression.register_symbol_table(m_symbolTable);
    if (!m_parser.compile(expression, m_expression)) {
        error = m_parser.error();
        return false;
    }

    std::vector<std::string> usedSymbols;
    exprtk::collect_variables(expression, usedSymbols);

    for (size_t i = 0; i < SUPPORTED_VARIABLES.size(); i++) {
        m_symbolUsed[i] =
            std::find(usedSymbols.begin(), usedSymbols.end(), SUPPORTED_VARIABLES[i].name) != usedSymbols.end();
    }

    return true;
}


bool Symbols::filterCell(const vistle::UnstructuredGrid::const_ptr grid, const vistle::DataBase::const_ptr &data,
                         vistle::Index elementIndex)
{
    assert(grid);
    assert(elementIndex < grid->getNumElements());
    for (size_t i = 0; i < SUPPORTED_VARIABLES.size(); i++) {
        if (m_symbolUsed[i]) {
            m_symbolValues[i] = SUPPORTED_VARIABLES[i].var({grid, data, elementIndex});
        }
    }
    return m_expression.value() == 0.0;
}

// Ghost : set the filtered elements to ghost
// Elements : remove the filtered elements
// Vertices : remove the filtered elements and the vertices that are not used by any element
DEFINE_ENUM_WITH_STRING_CONVERSIONS(FilterMode, (Ghost)(Elements)(Vertices))

FilterCells::FilterCells(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in", "unstructured grid with or without data");
    createOutputPort("grid_out", "filtered grid with or without data");

    m_expression = addStringParameter(
        "filter_expression",
        "elements for which this expression is true are filtered out, for supported symbols see selection lists",
        "type == HEXA");

    m_filterMode = addIntParameter("filterType", "filter type", 0, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_filterMode, FilterMode);

    //the lists only serve as a reference for the user
    addIntParameter("SupportedVariables", "Supported Variables", 0, Parameter::Choice);
    setParameterChoices("SupportedVariables", m_symbols.getSupportedVariables());
    addIntParameter("SupportedConstants", "Supported Constants", 0, Parameter::Choice);
    setParameterChoices("SupportedConstants", m_symbols.getSupportedConstants());
}

vistle::UnstructuredGrid::ptr clone(const vistle::UnstructuredGrid::const_ptr &grid, FilterMode filterMode)
{
    auto out = grid->clone();
    switch (filterMode) {
    case FilterMode::Vertices:
        out->resetCoords();
        //fall through
    case FilterMode::Elements: {
        out->resetCorners();
        out->resetElements();
    } break;
    case FilterMode::Ghost: {
        out->d()->ghost = ShmVector<Byte>();
        out->d()->ghost.construct();
        out->d()->ghost->resize(grid->getNumElements(), 0);
        std::copy(grid->d()->ghost->begin(), grid->d()->ghost->end(), out->d()->ghost->begin());
    } break;
    }
    out->copyAttributes(grid);
    return out;
}

bool FilterCells::changeParameter(const Parameter *p)
{
    if (p == m_expression) {
        std::string error;
        if (!m_symbols.update(m_expression->getValue(), error))
            sendError("failed to compile expression: %s", error.c_str());
    }
    return Module::changeParameter(p);
}

bool FilterCells::compute()
{
    auto obj = expect<Object>("grid_in");
    auto split = splitContainerObject(obj);
    if (!split.geometry) {
        sendError("no input geometry");
        return true;
    }

    if (auto data = split.mapped) {
        if (data->guessMapping() != DataBase::Vertex) {
            sendError("data has to be mapped per vertex");
            return true;
        }
    }
    auto grid = UnstructuredGrid::as(split.geometry);
    if (!grid) {
        sendError("cannot get underlying unstructured grid");
        return true;
    }
    grid->check(std::cerr);
    auto numVertices = grid->getNumVertices();


    auto out = clone(grid, (FilterMode)m_filterMode->getValue());

    std::vector<Index> vertexKept;
    if (m_filterMode->getValue() == FilterMode::Vertices)
        vertexKept.resize(numVertices, 0);
    for (size_t i = 0; i < grid->getNumElements(); i++) {
        bool filterCell = m_symbols.filterCell(grid, split.mapped, i);
        if (filterCell && m_filterMode->getValue() == FilterMode::Ghost) {
            out->setGhost(i, true);
        } else if (!filterCell && (m_filterMode->getValue() == FilterMode::Elements ||
                                   m_filterMode->getValue() == FilterMode::Vertices)) {
            for (size_t j = grid->el()[i]; j < grid->el()[i + 1]; j++) {
                // per vertex
                out->cl().push_back(grid->cl()[j]);
                if (m_filterMode->getValue() == FilterMode::Vertices) {
                    vertexKept[grid->cl()[j]] = 1;
                }
            }
            //per element
            out->d()->ghost->push_back(grid->isGhost(i));
            out->el().push_back(out->cl().size());
            out->tl().push_back(grid->tl()[i]);
        }
    }
    DataBase::ptr data;
    if (split.mapped) {
        data = split.mapped->clone();
        updateMeta(data);
        data->copyAttributes(split.mapped);
    }
    if (m_filterMode->getValue() == FilterMode::Vertices) {
        vistle::Index totalNumVertsRemaining = std::count(vertexKept.begin(), vertexKept.end(), 1);

        if (data) {
            data->resetArrays();
            data->setSize(totalNumVertsRemaining);
        }
        out->setSize(totalNumVertsRemaining);
        Index numVertsFiltered = 0;
        //add kept vertices to new grid
        for (size_t i = 0; i < vertexKept.size(); i++) {
            if (vertexKept[i]) {
                auto end = i - numVertsFiltered;
                out->copyEntry(end, grid, i);
                // out->x()[end] = (grid->x()[i]);
                // out->y()[end] = (grid->y()[i]);
                // out->z()[end] = (grid->z()[i]);
                if (data) {
                    data->copyEntry(end, split.mapped, i);
                }
            } else
                ++numVertsFiltered;
            vertexKept[i] = numVertsFiltered;
        }
        //adjust connectivity indices
        for (size_t i = 0; i < out->cl().size(); i++) {
            out->cl()[i] -= vertexKept[out->cl()[i]];
            assert(out->cl()[i] < out->getNumVertices());
        }
    }

    if (out->el().size() == 0) {
        sendError("No elements left after filtering");
        return true;
    }

    if (data)
        data->setGrid(out);
    updateMeta(out);
    addObject("grid_out", data ? data : out);

    return true;
}

MODULE_MAIN(FilterCells)

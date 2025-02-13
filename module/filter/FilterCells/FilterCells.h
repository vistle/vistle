#ifndef VISTLE_FILTERCELLS_FILTERCELLS_H
#define VISTLE_FILTERCELLS_FILTERCELLS_H

#include "SupportedVariables.h"
#include <vistle/module/module.h>
#include "../../general/Calc/exprtk/exprtk.hpp"


class Symbols {
public:
    Symbols();
    // update the expression and check if all symbols are supported
    bool update(const std::string &expression, std::string &error);
    //  true if the cell should be filtered out
    bool filterCell(const vistle::UnstructuredGrid::const_ptr grid, const vistle::DataBase::const_ptr &data,
                    vistle::Index elementIndex);
    std::vector<std::string> getSupportedSymbols();
    std::vector<std::string> getSupportedVariables();
    std::vector<std::string> getSupportedConstants();

private:
    double &getSymbol(const std::string_view &symbol);

    std::array<double, SUPPORTED_VARIABLES.size()> m_symbolValues;
    std::array<bool, SUPPORTED_VARIABLES.size()> m_symbolUsed;
    exprtk::symbol_table<double> m_symbolTable;
    exprtk::expression<double> m_expression;
    exprtk::parser<double> m_parser;
};

class FilterCells: public vistle::Module {
public:
    FilterCells(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool changeParameter(const vistle::Parameter *p) override;
    bool compute() override;
    vistle::StringParameter *m_expression;
    vistle::IntParameter *m_filterMode;
    Symbols m_symbols;
};


#endif

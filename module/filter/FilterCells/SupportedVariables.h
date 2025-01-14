#ifndef VISTLE_FILTER_CELLS_SUPPORTED_VARIABLES_H
#define VISTLE_FILTER_CELLS_SUPPORTED_VARIABLES_H

#include <vistle/core/vector.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>

struct Args {
    const vistle::UnstructuredGrid::const_ptr &grid;
    const vistle::DataBase::const_ptr &data;
    vistle::Index elementIndex;
};

typedef double (*VariableGetter)(const Args &args);

struct Variable {
    const std::string_view name;
    VariableGetter var;
};

constexpr std::array<Variable, 19> checkedVars(const std::array<Variable, 19> &vars)
{
    for (const auto &var: vars) {
        if (var.var == nullptr) {
            throw std::runtime_error("Variable " + std::string(var.name) + " has no source");
        }
    }
    return vars;
}


constexpr std::array<Variable, 19> SUPPORTED_VARIABLES = {
    Variable{"cellDiameter",
             [](const Args &args) {
                 auto t = args.grid->tl()[args.elementIndex];
                 return static_cast<double>(vistle::UnstructuredGrid::Dimensionality[t]);
             }},
    Variable{"datavalue",
             [](const Args &args) {
                 if (!args.data)
                     return 0.0;
                 if (args.data->mapping() == vistle::DataBase::Vertex)
                     return args.data->value(args.elementIndex);
                 else {
                     const vistle::Index begin = args.grid->el()[args.elementIndex],
                                         end = args.grid->el()[args.elementIndex + 1];

                     double avg = 0;
                     for (size_t i = begin; i < end; ++i) {
                         avg += args.data->value(args.grid->cl()[i]);
                     }
                     return avg / (end - begin);
                 }
             }},
    Variable{"dimensionality",
             [](const Args &args) {
                 auto t = args.grid->tl()[args.elementIndex];
                 return static_cast<double>(vistle::UnstructuredGrid::Dimensionality[t]);
             }},
    Variable{"edgeLength",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellEdgeLength(args.elementIndex));
             }},
    Variable{"elem",
             [](const Args &args) {
                 return static_cast<double>(args.elementIndex);
             }},
    Variable{"numFaces",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellNumFaces(args.elementIndex));
             }},
    Variable{"numVertices",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellNumVertices(args.elementIndex));
             }},
    Variable{"surface",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellSurface(args.elementIndex));
             }},
    Variable{"type",
             [](const Args &args) {
                 return static_cast<double>(args.grid->tl()[args.elementIndex]);
             }},
    Variable{"volume",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellVolume(args.elementIndex));
             }},
    Variable{"x_center",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellCenter(args.elementIndex).x());
             }},
    Variable{"x_max",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellBounds(args.elementIndex).second.x());
             }},
    Variable{"x_min",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellBounds(args.elementIndex).first.x());
             }},
    Variable{"y_center",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellCenter(args.elementIndex).y());
             }},
    Variable{"y_max",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellBounds(args.elementIndex).second.y());
             }},
    Variable{"y_min",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellBounds(args.elementIndex).first.y());
             }},
    Variable{"z_center",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellCenter(args.elementIndex).z());
             }},
    Variable{"z_max",
             [](const Args &args) {
                 return static_cast<double>(args.grid->cellBounds(args.elementIndex).second.z());
             }},
    Variable{"z_min", [](const Args &args) {
                 return static_cast<double>(args.grid->cellBounds(args.elementIndex).first.z());
             }}};

#endif // VISTLE_FILTER_CELLS_SUPPORTED_VARIABLES_H

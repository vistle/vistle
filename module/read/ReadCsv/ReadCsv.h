#ifndef READCSV_H
#define READCSV_H

#include <vistle/module/reader.h>
#include <array>
#include <vistle/core/points.h>
#include <vistle/core/scalar.h>
#include <vistle/core/vec.h>
class ReadCsv: public vistle::Reader {
public:
    ReadCsv(const std::string &name, int moduleID, mpi::communicator comm);

private:
    static const size_t NUM_DATA_FIELDS = 4;
    static const size_t NUM_COORD_FIELDS = 3;
    static const size_t NUM_FIELDS = NUM_COORD_FIELDS + NUM_DATA_FIELDS;

    bool examine(const vistle::Parameter *param = nullptr) override;
    template<char delimiter>
    bool readFile(Token &token, int timestep, int block);

    template<char Delimiter>
    void readLayer(size_t layer, vistle::Points::ptr &points,
                   std::array<vistle::Vec<vistle::Scalar>::ptr, NUM_DATA_FIELDS> &dataFields);
    bool read(Token &token, int timestep = -1, int block = -1) override;

    vistle::StringParameter *m_directory = nullptr;
    vistle::IntParameter *m_filename = nullptr;

    vistle::IntParameter *m_layerMode = nullptr;
    vistle::IntParameter *m_layersInOneObject = nullptr;
    vistle::FloatParameter *m_layerOffset = nullptr;


    std::array<vistle::IntParameter *, NUM_FIELDS> m_selectionParams;
    std::vector<std::string> m_choices;
    char m_delimiter = ',';

    vistle::Integer getCoordSelection(size_t i);
    vistle::Integer getDataSelection(size_t i);
};

#endif // READCSV_H

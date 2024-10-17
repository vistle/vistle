#include <vistle/module/module.h>
#include <vistle/core/points.h>

#include <fstream>
#include <ostream>
#include <sstream>
#include <iomanip>
#include <cctype>

using namespace vistle;

struct Color {
    int r = 0, g = 0, b = 0, a = 255;
    std::string str() const
    {
        std::stringstream s;
        s << "#" << std::hex << std::setfill('0') << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b;
        if (a != 255) {
            s << a;
        }
        return s.str();
    }
};

class ColorMetapostPart: public vistle::Module {
public:
    ColorMetapostPart(const std::string &name, int moduleID, mpi::communicator comm);
    ~ColorMetapostPart();

private:
    bool prepare() override;
    bool compute() override;

    Color m_defaultColor;
    std::map<Index, Color> m_partColors;

    StringParameter *p_colorfile;
};

using namespace vistle;

ColorMetapostPart::ColorMetapostPart(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    Port *din = createInputPort("data_in", "input data", Port::MULTI);
    Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
    din->link(dout);

    p_colorfile = addStringParameter("metapost", "filename of metapost file", "", Parameter::ExistingFilename);
    setParameterFilters(p_colorfile, "METApost (*.ses)");
}

ColorMetapostPart::~ColorMetapostPart()
{}

bool ColorMetapostPart::prepare()
{
    //std::cerr << "ColorMetapostPart: compute: generation=" << m_generation << std::endl;

    auto filename = p_colorfile->getValue();
    std::ifstream f(filename, std::ifstream::in);

    std::string line;
    for (std::getline(f, line); f.good(); std::getline(f, line)) {
        auto begin = line.begin(), end = line.end();
        while (begin != end && std::isspace(*begin))
            ++begin;
        if (*begin == '$')
            continue;

        std::istringstream str(line);
        typedef std::istream_iterator<std::string> iter;
        std::vector<std::string> words{iter(str), iter()};

        m_defaultColor.r = 255;
        m_defaultColor.g = 0;
        m_defaultColor.b = 255;

        if (words.size() >= 4 && words[0] == "col" && words[1] == "pid") {
            Index pid = atol(words[3].c_str());
            std::istringstream colstr(words[2]);
            auto &c = m_partColors[pid];
            colstr >> c.r;
            colstr.ignore();
            colstr >> c.g;
            colstr.ignore();
            colstr >> c.b;
        } else if (words.size() >= 5 && words[0] == "style" && words[1] == "transparency" && words[2] == "pid") {
            Index pid = atol(words[4].c_str());
            Scalar transp = atof(words[3].c_str());
            auto &c = m_partColors[pid];
            c.a = (1. - transp) * 255.99;
        }
    }

    return true;
}

bool ColorMetapostPart::compute()
{
    //std::cerr << "ColorMetapostPart: compute: generation=" << m_generation << std::endl;

    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    auto part = obj->getAttribute("_part");

    auto db = DataBase::as(obj);
    if (db) {
        if (part.empty()) {
            auto grid = db->grid();
            part = grid->getAttribute("_part");
        }
    }

    Object::ptr out = obj->clone();
    Index pid = InvalidIndex;
    std::stringstream str(part);
    str >> pid;
    auto it = m_partColors.find(pid);
    std::string color;
    if (it == m_partColors.end())
        color = m_defaultColor.str();
    else
        color = it->second.str();

    out->addAttribute("_color", color);
    updateMeta(out);
    addObject("data_out", out);

    return true;
}

MODULE_MAIN(ColorMetapostPart)

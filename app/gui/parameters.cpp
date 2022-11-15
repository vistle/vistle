/*********************************************************************************/
/** \file parameters.cpp
 *
 * Qt property browser for module parameters
 */
/**********************************************************************************/

#include <cmath>
#include <QTextEdit>
#include <QLayout>
#include <QDebug>

#include <vistle/userinterface/vistleconnection.h>

#include "parameters.h"
#include "vistleobserver.h"
#include <vistle/core/message.h>

#include <QtGroupPropertyManager>
#include <QtBoolPropertyManager>
#include <QtIntPropertyManager>
#include <QtDoublePropertyManager>
#include <QtStringPropertyManager>
#include <QtEnumPropertyManager>

#include "propertybrowser/qtlongpropertymanager.h"
#include "propertybrowser/qtlongvectorpropertymanager.h"
#include "propertybrowser/vistledoublepropertymanager.h"
#include "propertybrowser/qtvectorpropertymanager.h"
#include "propertybrowser/vistlebrowserpropertymanager.h"

#include "propertybrowser/qtlongeditorfactory.h"
#include "propertybrowser/vistledoubleeditorfactory.h"
#include "propertybrowser/vistlelineeditfactory.h"
#include "propertybrowser/vistlebrowserfactory.h"

namespace gui {

const int NumDec = 15;

static QString displayName(QString parameterName)
{
    return parameterName.replace("_", " ").trimmed();
}

template<class M>
static M *addPropertyManager(Parameters *p)
{
    M *m = new M(p);
    QObject::connect(m, SIGNAL(propertyChanged(QtProperty *)), p, SLOT(propertyChanged(QtProperty *)));
    return m;
}

Parameters::Parameters(QWidget *parent, Qt::WindowFlags f)
: Parameters::PropertyBrowser(parent)
, m_moduleId(0)
, m_vistle(nullptr)
, m_groupManager(nullptr)
, m_boolManager(nullptr)
, m_intManager(nullptr)
, m_floatManager(nullptr)
, m_stringManager(nullptr)
, m_browserManager(nullptr)
, m_stringChoiceManager(nullptr)
, m_intChoiceManager(nullptr)
, m_vectorManager(nullptr)
, m_intVectorManager(nullptr)
{
    if (layout())
        layout()->setMargin(3);

    m_groupManager = new QtGroupPropertyManager(this); // no change notifications

    m_boolManager = addPropertyManager<QtBoolPropertyManager>(this);
    m_intManager = addPropertyManager<QtLongPropertyManager>(this);
    m_intChoiceManager = addPropertyManager<QtEnumPropertyManager>(this);
    m_floatManager = addPropertyManager<VistleDoublePropertyManager>(this);
    m_stringManager = addPropertyManager<QtStringPropertyManager>(this);
    m_browserManager = addPropertyManager<VistleBrowserPropertyManager>(this);
    m_stringChoiceManager = addPropertyManager<QtEnumPropertyManager>(this);
    m_vectorManager = addPropertyManager<QtVectorPropertyManager>(this);
    m_intVectorManager = addPropertyManager<QtLongVectorPropertyManager>(this);

    QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
    setFactoryForManager(m_boolManager, checkBoxFactory);

    QtLongSpinBoxFactory *spinBoxFactory = new QtLongSpinBoxFactory(this);
    setFactoryForManager(m_intManager, spinBoxFactory);
    setFactoryForManager(m_intVectorManager->subLongPropertyManager(), spinBoxFactory);

    VistleDoubleSpinBoxFactory *doubleSpinBoxFactory = new VistleDoubleSpinBoxFactory(this);
    setFactoryForManager(m_floatManager, doubleSpinBoxFactory);
    setFactoryForManager(m_vectorManager->subDoublePropertyManager(), doubleSpinBoxFactory);

    VistleLineEditFactory *lineEditFactory = new VistleLineEditFactory(this);
    setFactoryForManager(m_stringManager, lineEditFactory);

    m_browserFactory = new VistleBrowserFactory(this);
    setFactoryForManager(m_browserManager, m_browserFactory);

    QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(this);
    setFactoryForManager(m_stringChoiceManager, comboBoxFactory);
    setFactoryForManager(m_intChoiceManager, comboBoxFactory);

    auto itemToName = [this](QtBrowserItem *item) -> QString {
        auto propIt = m_itemToProp.find(item);
        if (propIt == m_itemToProp.end()) {
            std::cerr << "property for item not found: " << item << std::endl;
            return QString();
        }
        auto prop = propIt->second;
        auto name = propertyToName(prop);
        return name;
    };

    connect(this, &Parameters::expanded, [this, itemToName](QtBrowserItem *item) {
        auto name = itemToName(item);
        if (name.isEmpty())
            return;
        //std::cerr << "Parameters: storing expanded state for " << name.toStdString() << ": true" << std::endl;
        m_expandedState[m_moduleId][name] = true;
    });
    connect(this, &Parameters::collapsed, [this, itemToName](QtBrowserItem *item) {
        auto name = itemToName(item);
        if (name.isEmpty())
            return;
        //std::cerr << "Parameters: storing expanded state for " << name.toStdString() << ": false" << std::endl;
        m_expandedState[m_moduleId][name] = false;
    });
}

void Parameters::setVistleObserver(VistleObserver *observer)
{
    connect(observer, SIGNAL(newParameter_s(int, QString)), this, SLOT(newParameter(int, QString)));
    connect(observer, SIGNAL(deleteParameter_s(int, QString)), this, SLOT(deleteParameter(int, QString)));
    connect(observer, SIGNAL(parameterValueChanged_s(int, QString)), this, SLOT(parameterValueChanged(int, QString)));
    connect(observer, SIGNAL(parameterChoicesChanged_s(int, QString)), this,
            SLOT(parameterChoicesChanged(int, QString)));
}

void Parameters::setVistleConnection(vistle::VistleConnection *conn)
{
    m_vistle = conn;
    m_browserFactory->setUi(&conn->ui());
}

void Parameters::setModule(int id)
{
    clear();
    m_itemToProp.clear();
    m_paramToProp.clear();
    m_propToParam.clear();
    m_groups.clear();
    m_propToGroup.clear();

    m_moduleId = id;

    //std::cerr << "Parameters: showing for " << id << std::endl;
    if (m_vistle) {
        auto params = m_vistle->getParameters(id);
        for (auto &p: params)
            newParameter(id, QString::fromStdString(p));
    }
}

QString Parameters::propertyToName(QtProperty *prop) const
{
    auto propIt = m_propToParam.find(prop);
    if (propIt == m_propToParam.end()) {
        propIt = m_propToGroup.find(prop);
        if (propIt == m_propToGroup.end())
            return QString();
    }
    auto name = propIt->second;
    return name;
}

void Parameters::setParameterEnabled(QtProperty *prop, bool state)
{
    if (!prop)
        return;
    const auto subs = prop->subProperties();
    for (auto p: subs) {
        p->setEnabled(state);
    }
    if (subs.empty())
        prop->setEnabled(state);
    else
        prop->setEnabled(true);
}

bool Parameters::getExpandedState(QString name, bool &state)
{
    if (name.isEmpty())
        return false;

    auto &expanded = m_expandedState[m_moduleId];
    auto it = expanded.find(name);
    if (it == expanded.end())
        return false;

    state = it->second;
    //std::cerr << "Parameters: found expanded state for " << name.toStdString() << ": " << state << std::endl;
    return true;
}

void Parameters::addItemWithProperty(QtBrowserItem *item, QtProperty *prop)
{
    m_itemToProp[item] = prop;

    bool expanded = false;
    if (getExpandedState(propertyToName(prop), expanded))
        setExpanded(item, expanded);
}

void Parameters::newParameter(int moduleId, QString parameterName)
{
    if (m_moduleId != moduleId)
        return;

    auto p = m_vistle->getParameter(moduleId, parameterName.toStdString());
    if (!p)
        return;

    m_ignorePropertyChanges = true;

    const auto it = m_paramToProp.find(parameterName);
    if (vistle::message::Id::isModule(moduleId)) {
        assert(it == m_paramToProp.end());
        if (it != m_paramToProp.end()) {
            qDebug() << "duplicate parameter " << parameterName << " for module " << m_moduleId;
        }
    }

    QtProperty *prop = nullptr;
    if (auto ip = std::dynamic_pointer_cast<vistle::IntParameter>(p)) {
        if (ip->presentation() == vistle::Parameter::Boolean) {
            prop = m_boolManager->addProperty(displayName(parameterName));
        } else if (ip->presentation() == vistle::Parameter::Choice) {
            prop = m_intChoiceManager->addProperty(displayName(parameterName));
        } else {
            prop = m_intManager->addProperty(displayName(parameterName));
        }
    } else if (auto fp = std::dynamic_pointer_cast<vistle::FloatParameter>(p)) {
        prop = m_floatManager->addProperty(displayName(parameterName));
        m_floatManager->setDecimals(prop, NumDec);
    } else if (auto sp = std::dynamic_pointer_cast<vistle::StringParameter>(p)) {
        if (sp->presentation() == vistle::Parameter::Choice) {
            prop = m_stringChoiceManager->addProperty(displayName(parameterName));
        } else if (sp->presentation() == vistle::Parameter::Filename ||
                   sp->presentation() == vistle::Parameter::Directory ||
                   sp->presentation() == vistle::Parameter::ExistingDirectory ||
                   sp->presentation() == vistle::Parameter::ExistingFilename) {
            prop = m_browserManager->addProperty(displayName(parameterName));
            m_browserManager->setModuleId(prop, moduleId);
            QString module = QString::fromStdString(m_vistle->ui().state().getModuleName(m_moduleId));
            QString title =
                "Vistle - " + module + "_" + QString::number(m_moduleId) + ": " + displayName(parameterName);
            m_browserManager->setTitle(prop, title);
            if (sp->presentation() == vistle::Parameter::Filename)
                m_browserManager->setFileMode(prop, VistleBrowserEdit::File);
            else if (sp->presentation() == vistle::Parameter::Directory)
                m_browserManager->setFileMode(prop, VistleBrowserEdit::Directory);
            else if (sp->presentation() == vistle::Parameter::ExistingDirectory)
                m_browserManager->setFileMode(prop, VistleBrowserEdit::ExistingDirectory);
            else if (sp->presentation() == vistle::Parameter::ExistingFilename)
                m_browserManager->setFileMode(prop, VistleBrowserEdit::ExistingFile);
        } else {
            prop = m_stringManager->addProperty(displayName(parameterName));
        }
    } else if (auto vp = std::dynamic_pointer_cast<vistle::VectorParameter>(p)) {
        vistle::ParamVector value = vp->getValue();
        prop = m_vectorManager->addProperty(displayName(parameterName));
        m_vectorManager->setDecimals(prop, NumDec);
        m_vectorManager->setDimension(prop, value.dim);
    } else if (auto vp = std::dynamic_pointer_cast<vistle::IntVectorParameter>(p)) {
        vistle::IntParamVector value = vp->getValue();
        prop = m_intVectorManager->addProperty(displayName(parameterName));
        m_intVectorManager->setDimension(prop, value.dim);
    } else {
        std::cerr << "parameter type not handled in Parameters::newParameter" << std::endl;
    }

    if (prop) {
        prop->setStatusTip(QString::fromStdString(p->description()));
        prop->setWhatsThis(QString::fromStdString(p->description()));
        m_paramToProp[parameterName] = prop;
        m_propToParam[prop] = parameterName;
        QString group = QString::fromStdString(p->group());
        bool expanded = false;
        if (parameterName.startsWith("_")) {
            group = "System Parameters";
        } else {
            expanded = p->isGroupExpanded();
        }
        if (!group.isEmpty()) {
            auto it = m_groups.find(group);
            QtProperty *g = nullptr;
            if (it == m_groups.end()) {
                g = m_groupManager->addProperty(group);
                auto item = addProperty(g);
                m_groups[group] = g;
                m_propToGroup[g] = group;
                addItemWithProperty(item, g);
            } else {
                g = it->second;
            }
            g->addSubProperty(prop);
            if (auto *item = topLevelItem(g)) {
                if (getExpandedState(group, expanded)) {
                    setExpanded(item, expanded);
                } else if (expanded) {
                    setExpanded(item, expanded);
                }
            }
        } else {
            auto *item = addProperty(prop);
            addItemWithProperty(item, prop);
            bool expanded = false;
            if (getExpandedState(propertyToName(prop), expanded)) {
                setExpanded(item, expanded);
            }
        }
    }

    parameterChoicesChanged(moduleId, parameterName);
    parameterValueChanged(moduleId, parameterName);

    m_ignorePropertyChanges = false;
}

void Parameters::deleteParameter(int moduleId, QString parameterName)
{
    if (m_moduleId != moduleId)
        return;

    auto p = m_vistle->getParameter(moduleId, parameterName.toStdString());
    if (!p)
        return;

    const auto it = m_paramToProp.find(parameterName);
    assert(it != m_paramToProp.end());
    if (it == m_paramToProp.end()) {
        qDebug() << "Parameters::deleteParameter: did not find parameter " << m_moduleId << ":" << parameterName;
        return;
    }

    QtProperty *prop = it->second;
    QString group = QString::fromStdString(p->group());
    QtProperty *g = nullptr;
    if (!group.isEmpty()) {
        auto it = m_groups.find(group);
        if (it != m_groups.end()) {
            g = it->second;
        }
    }
    if (g) {
        g->removeSubProperty(prop);
    } else {
        removeProperty(prop);
    }
}

void Parameters::parameterValueChanged(int moduleId, QString parameterName)
{
    if (m_moduleId != moduleId)
        return;

    auto p = m_vistle->getParameter(moduleId, parameterName.toStdString());
    if (!p)
        return;

    QtProperty *prop = nullptr;
    const auto it = m_paramToProp.find(parameterName);
    if (it != m_paramToProp.end()) {
        prop = it->second;
    }

    if (!prop)
        return;

    setParameterEnabled(prop, !p->isReadOnly());

    if (auto ip = std::dynamic_pointer_cast<vistle::IntParameter>(p)) {
        if (ip->presentation() == vistle::Parameter::Boolean) {
            m_boolManager->setValue(prop, ip->getValue() != 0);
        } else if (ip->presentation() == vistle::Parameter::Choice) {
            m_intChoiceManager->setValue(prop, ip->getValue());
        } else {
            m_intManager->setValue(prop, ip->getValue());
            m_intManager->setRange(prop, ip->minimum(), ip->maximum());

            QString tip = QString("%1 (default: %2, %3 – %4)")
                              .arg(QString::fromStdString(p->description()), QString::number(ip->getDefaultValue()),
                                   QString::number(ip->minimum()), QString::number(ip->maximum()));
            prop->setStatusTip(tip);
        }
    } else if (auto fp = std::dynamic_pointer_cast<vistle::FloatParameter>(p)) {
        m_floatManager->setValue(prop, fp->getValue());
        m_floatManager->setRange(prop, fp->minimum(), fp->maximum());
        m_floatManager->setDecimals(prop, NumDec);
        double range = fp->maximum() - fp->minimum();
        double step = 1.;
        if (range > 0. && !std::isinf(range)) {
            if (range > 300.) {
                while (step * 300. < range) {
                    step *= 10.;
                }
            } else {
                while (step * 300. > range) {
                    step /= 10.;
                }
            }
            m_floatManager->setSingleStep(prop, step);
        } else {
            m_floatManager->setSingleStep(prop, 1.);
        }

        QString tip = QString("%1 (default: %2, %3 – %4)")
                          .arg(QString::fromStdString(p->description()), QString::number(fp->getDefaultValue()),
                               QString::number(fp->minimum()), QString::number(fp->maximum()));
        prop->setStatusTip(tip);
    } else if (auto sp = std::dynamic_pointer_cast<vistle::StringParameter>(p)) {
        if (sp->presentation() == vistle::Parameter::Choice) {
            QStringList choices = m_stringChoiceManager->enumNames(prop);
            QString val = QString::fromStdString(sp->getValue());
            int choice = choices.indexOf(val);
            if (choice < 0)
                choice = 0;
            m_stringChoiceManager->setValue(prop, choice);
        } else if (sp->presentation() == vistle::Parameter::Filename ||
                   sp->presentation() == vistle::Parameter::Directory ||
                   sp->presentation() == vistle::Parameter::ExistingDirectory ||
                   sp->presentation() == vistle::Parameter::ExistingFilename) {
            m_browserManager->setValue(prop, QString::fromStdString(sp->getValue()));
            m_browserManager->setFilters(prop, QString::fromStdString(sp->minimum()));
        } else {
            m_stringManager->setValue(prop, QString::fromStdString(sp->getValue()));
        }
    } else if (auto vp = std::dynamic_pointer_cast<vistle::VectorParameter>(p)) {
        vistle::ParamVector value = vp->getValue();
        if (!prop) {
            prop = m_vectorManager->addProperty(displayName(parameterName));
            m_vectorManager->setDecimals(prop, NumDec);
            m_vectorManager->setDimension(prop, value.dim);
        }
        m_vectorManager->setValue(prop, value);
        m_vectorManager->setRange(prop, vp->minimum(), vp->maximum());
        QString tip;
        switch (value.dim) {
        case 1:
            tip = QString("%1 (%2)").arg(QString::fromStdString(p->description()), QString::number(value[0]));
            break;
        case 2:
            tip = QString("%1 (%2, %3)")
                      .arg(QString::fromStdString(p->description()), QString::number(value[0]),
                           QString::number(value[1]));
            break;
        case 3:
            tip = QString("%1 (%2, %3, %4)")
                      .arg(QString::fromStdString(p->description()), QString::number(value[0]),
                           QString::number(value[1]), QString::number(value[2]));
            break;
        case 4:
            tip = QString("%1 (%2, %3, %4, %5)")
                      .arg(QString::fromStdString(p->description()), QString::number(value[0]),
                           QString::number(value[1]), QString::number(value[2]), QString::number(value[3]));
            break;
        }
        prop->setStatusTip(tip);
    } else if (auto vp = std::dynamic_pointer_cast<vistle::IntVectorParameter>(p)) {
        vistle::IntParamVector value = vp->getValue();
        if (!prop) {
            prop = m_intVectorManager->addProperty(displayName(parameterName));
            m_intVectorManager->setDimension(prop, value.dim);
        }
        m_intVectorManager->setValue(prop, value);
        m_intVectorManager->setRange(prop, vp->minimum(), vp->maximum());
    } else {
        std::cerr << "parameter type not handled in Parameters::parameterValueChanged" << std::endl;
    }
}

void Parameters::parameterChoicesChanged(int moduleId, QString parameterName)
{
    //std::cerr << "choices changed for " << moduleId << ":" << parameterName.toStdString() << std::endl;
    if (m_moduleId != moduleId)
        return;

    auto p = m_vistle->getParameter(moduleId, parameterName.toStdString());
    if (!p)
        return;

    if (p->presentation() != vistle::Parameter::Choice)
        return;

    const auto it = m_paramToProp.find(parameterName);
    if (it == m_paramToProp.end()) {
        return;
    }

    QtProperty *prop = it->second;

    //std::cerr << "property " << parameterName.toStdString() << (insert?" to insert":"") << std::endl;

    QStringList choices;
    for (auto &choice: p->choices()) {
        choices << QString::fromStdString(choice);
    }
    if (std::dynamic_pointer_cast<vistle::IntParameter>(p)) {
        m_intChoiceManager->setEnumNames(prop, choices);
    } else if (std::dynamic_pointer_cast<vistle::StringParameter>(p)) {
        m_stringChoiceManager->setEnumNames(prop, choices);
    } else {
    }
}

void Parameters::propertyChanged(QtProperty *prop)
{
    if (m_ignorePropertyChanges)
        return;

    auto it = m_propToParam.find(prop);
    if (it == m_propToParam.end()) {
        std::cerr << "parameter for property not found: " << prop->propertyName().toStdString() << std::endl;
        return;
    }
    std::string paramName = it->second.toStdString();
    auto p = m_vistle->getParameter(m_moduleId, paramName);
    assert(p);
    if (!p)
        return;

    bool changed = false;
    if (auto ip = std::dynamic_pointer_cast<vistle::IntParameter>(p)) {
        if (ip->presentation() == vistle::Parameter::Boolean) {
            vistle::Integer value = m_boolManager->value(prop) ? 1 : 0;
            if (ip->getValue() != value) {
                changed = true;
                ip->setValue(value);
            }
        } else if (ip->presentation() == vistle::Parameter::Choice) {
            vistle::Integer value = m_intChoiceManager->value(prop);
            //std::cerr << "Choice: value=" << value << std::endl;
            if (ip->getValue() != value) {
                changed = true;
                ip->setValue(value);
            }
        } else {
            if (ip->getValue() != m_intManager->value(prop)) {
                changed = true;
                ip->setValue(m_intManager->value(prop));
            }
        }
    } else if (auto fp = std::dynamic_pointer_cast<vistle::FloatParameter>(p)) {
        if (fp->getValue() != m_floatManager->value(prop)) {
            changed = true;
            fp->setValue(m_floatManager->value(prop));
        }
    } else if (auto sp = std::dynamic_pointer_cast<vistle::StringParameter>(p)) {
        std::string value;
        if (sp->presentation() == vistle::Parameter::Choice) {
            int choice = m_stringChoiceManager->value(prop);
            //std::cerr << "choice index: " << choice << std::endl;
            QStringList choices = m_stringChoiceManager->enumNames(prop);
            value = choices[choice].toStdString();
            //std::cerr << "choice value: " << value << std::endl;
        } else if (sp->presentation() == vistle::Parameter::Filename ||
                   sp->presentation() == vistle::Parameter::Directory ||
                   sp->presentation() == vistle::Parameter::ExistingFilename ||
                   sp->presentation() == vistle::Parameter::ExistingDirectory) {
            value = m_browserManager->value(prop).toStdString();
        } else {
            value = m_stringManager->value(prop).toStdString();
        }
        if (sp->getValue() != value) {
            changed = true;
            sp->setValue(value);
        }
    } else if (auto vp = std::dynamic_pointer_cast<vistle::VectorParameter>(p)) {
        if (vp->getValue() != m_vectorManager->value(prop)) {
            changed = true;
            vp->setValue(m_vectorManager->value(prop));
        }
    } else if (auto vp = std::dynamic_pointer_cast<vistle::IntVectorParameter>(p)) {
        if (vp->getValue() != m_intVectorManager->value(prop)) {
            changed = true;
            vp->setValue(m_intVectorManager->value(prop));
        }
    } else {
        std::cerr << "property type not handled for " << paramName << std::endl;
        return;
    }
    if (changed) {
        std::cerr << "sending new value for " << paramName << std::endl;
        m_vistle->sendParameter(p);
    }
}

} //namespace gui

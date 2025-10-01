#include "TransformInteraction.h"
#include "TransformPlugin.h"
#include <config/CoviseConfig.h>
#include <cover/coInteractor.h>
#include <cover/coVRPluginSupport.h>
#include <cover/RenderObject.h>
#include <cover/ui/Action.h>
#include <cover/ui/Button.h>
#include <cover/ui/Slider.h>
#include <cover/ui/Menu.h>
#include <osg/Matrix>
#include <osg/Vec3>
#include <PluginUtil/coVR3DTransInteractor.h>
#include <PluginUtil/coVR3DTransformInteractor.h>

using namespace opencover;

TransformInteraction::TransformInteraction(const RenderObject *container, coInteractor *inter, const char *pluginName,
                                           TransformPlugin *plugin)
: ModuleInteraction(container, inter, pluginName), m_plugin(plugin)
{
    if (cover->debugLevel(2))
        fprintf(stderr, "\nTransformInteraction::TransformInteraction\n");

    float interSize = -1.f;
    interSize = covise::coCoviseConfig::getFloat("COVER.IconSize", interSize / 2);
    // Use the coVRIntersectionInteractor constructor parameters
    m_interactor = std::make_unique<coVR3DTransformInteractor>(interSize, vrui::coInteraction::ButtonA, "hand",
                                                               "TransformInteractor", vrui::coInteraction::Medium);
    m_interactor->disableIntersection();
    m_interactor->hide();

    // Create reset scale button
    m_resetScaleButton = new opencover::ui::Action(menu_, "ResetScale");
    m_resetScaleButton->setText("Reset Scale");
    m_resetScaleButton->setCallback([this]() {
        m_interactor->updateScale(osg::Vec3(1.0f, 1.0f, 1.0f));
        sendTransformToModule();
    });

    // Slider to change interactor size (uniform scale)
    m_sizeSlider = new opencover::ui::Slider(menu_, "InteractorSize");
    m_sizeSlider->setText("Interactor size");
    // default based on icon size or 1.0
    m_sizeSlider->setValue(1);
    m_sizeSlider->setBounds(0.1, 10.0);
    m_sizeSlider->setCallback([this](double value, bool moving) {
        if (m_interactor) {
            m_interactor->setInteractorSize(value * value);
        }
    });

    updateInteractorTransform();
}

TransformInteraction::~TransformInteraction()
{
    if (m_interactor) {
        m_interactor->hide();
        m_interactor->disableIntersection();
    }
}

void TransformInteraction::preFrame()
{
    m_interactor->preFrame();
    if (m_interactor && m_interactor->wasStopped()) {
        sendTransformToModule();
    }
}

void TransformInteraction::update(const RenderObject *container, coInteractor *inter)
{
    ModuleInteraction::update(container, inter);
    updateInteractorTransform();
}

void TransformInteraction::updatePickInteractors(bool show)
{
    if (m_interactor) {
        if (show && !hideCheckbox_->state()) {
            m_interactor->show();
            m_interactor->enableIntersection();
        } else {
            m_interactor->hide();
            m_interactor->disableIntersection();
        }
    }
}

void TransformInteraction::updateDirectInteractors(bool show)
{
    // For now, same as pick interactors
    updatePickInteractors(show);
}

void TransformInteraction::updateInteractorTransform()
{
    if (!m_interactor || !inter_)
        return;

    // Get current parameters from the module
    osg::Vec3 translate;
    osg::Vec3 rotAxis;
    float rotAngle = 0;
    osg::Vec3 scale(1.0f, 1.0f, 1.0f); // Default scale

    // Get translate parameter (now in world coordinates)
    int numElem;
    float *translateParam = nullptr;
    inter_->getFloatVectorParam("translate", numElem, translateParam);
    if (translateParam && numElem >= 3) {
        translate[0] = translateParam[0];
        translate[1] = translateParam[1];
        translate[2] = translateParam[2];
    }

    // Get rotation_axis_angle parameter (4 elements: x, y, z, angle)
    float *rotParam = nullptr;
    inter_->getFloatVectorParam("rotation_axis_angle", numElem, rotParam);
    if (rotParam && numElem >= 4) {
        rotAxis[0] = rotParam[0];
        rotAxis[1] = rotParam[1];
        rotAxis[2] = rotParam[2];
        rotAngle = rotParam[3] / 180.0f * osg::PI;
    }

    // Get scale parameter (3 elements: x, y, z)
    float *scaleParam = nullptr;
    inter_->getFloatVectorParam("scale", numElem, scaleParam);
    if (scaleParam && numElem >= 3) {
        scale[0] = scaleParam[0];
        scale[1] = scaleParam[1];
        scale[2] = scaleParam[2];
    }

    osg::Matrix transform;
    transform.makeIdentity();

    osg::Matrix rotMatrix;
    rotMatrix.makeRotate(osg::Quat(rotAngle, rotAxis));
    transform *= rotMatrix;

    osg::Matrix transMat;
    transMat.makeTranslate(translate);
    transform *= transMat;

    m_interactor->updateTransform(transform);
    m_interactor->updateScale(scale);
}

void TransformInteraction::sendTransformToModule()
{
    if (!m_interactor || !inter_)
        return;

    // Get the current transformation from the interactor
    const auto matrix = m_interactor->getMatrix();

    // Extract rotation as axis-angle
    auto quat = matrix.getRotate();
    osg::Vec3 rotAxis;
    osg::Quat::value_type angle = 0.0f;
    quat.getRotate(angle, rotAxis);

    // Handle edge case for very small angles
    if (angle < 1e-6) {
        rotAxis = osg::Vec3(0, 0, 1);
        angle = 0.0;
    }

    // Extract translation directly (now in world coordinates)
    osg::Vec3 translation = matrix.getTrans();

    float translate[3] = {static_cast<float>(translation.x()), static_cast<float>(translation.y()),
                          static_cast<float>(translation.z())};

    float rotAxisAngle[4] = {static_cast<float>(rotAxis.x()), static_cast<float>(rotAxis.y()),
                             static_cast<float>(rotAxis.z()), static_cast<float>(angle * (180.0f / osg::PI))};

    // Extract scale
    osg::Vec3 scaleVec = m_interactor->getScale();
    float scale[3] = {static_cast<float>(scaleVec.x()), static_cast<float>(scaleVec.y()),
                      static_cast<float>(scaleVec.z())};


    m_plugin->getSyncInteractors(inter_);

    // Set the parameters
    m_plugin->setVectorParam("translate", 3, translate);
    m_plugin->setVectorParam("rotation_axis_angle", 4, rotAxisAngle);
    m_plugin->setVectorParam("scale", 3, scale);

    // Execute the module
    m_plugin->executeModule();
}

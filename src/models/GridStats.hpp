#pragma once
#include <memory>
#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "models/Logic.hpp"

template<typename REAL> class Project;
namespace render {
    class GridTess;
} // of namespace render

namespace models {

class GridStats
        : public tinia::model::StateListener
{
public:
    GridStats( std::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic );

    ~GridStats();

    void
    stateElementModified( tinia::model::StateElement * stateElement );

    tinia::model::gui::Element*
    guiFactory() const;

    float
    zScale() const { return m_zscale; }

    void
    update( );

    void
    update( std::shared_ptr<Project<float> > project,
            std::shared_ptr<render::GridTess> tessellation );

protected:
    std::shared_ptr<tinia::model::ExposedModel> m_model;
    Logic&                                      m_logic;
    float                                       m_zscale;
};


} // of namespace models

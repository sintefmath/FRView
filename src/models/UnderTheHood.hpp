#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "models/Logic.hpp"
#include "render/TimerQuery.hpp"
#include "PerfTimer.hpp"

namespace models {

class UnderTheHood
        : public tinia::model::StateListener
{
public:
    UnderTheHood( std::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic );

    ~UnderTheHood();

    render::TimerQuery&
    proxyGenerateTimer() { return m_proxy_gen; }

    render::TimerQuery&
    surfaceGenerateTimer() { return m_surface_gen; }

    render::TimerQuery&
    surfaceRenderTimer() { return m_surface_render; }

    const std::string&
    titleKey() const;

    bool
    profilingEnabled() const { return m_profiling_enabled; }

    void
    update( bool force=false );

    void
    stateElementModified( tinia::model::StateElement * stateElement );

    tinia::model::gui::Element*
    guiFactory() const;


protected:
    std::shared_ptr<tinia::model::ExposedModel> m_model;
    Logic&                                      m_logic;
    bool                                        m_profiling_enabled;

    unsigned int                                m_frames;
    PerfTimer                                   m_update_timer;
    render::TimerQuery                          m_proxy_gen;
    render::TimerQuery                          m_surface_gen;
    render::TimerQuery                          m_surface_render;



};


} // of namespace models

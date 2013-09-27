/* Copyright STIFTELSEN SINTEF 2013
 * 
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "models/Logic.hpp"
#include "render/TimerQuery.hpp"
#include "utils/PerfTimer.hpp"

namespace models {

class UnderTheHood
        : public tinia::model::StateListener
{
public:
    UnderTheHood( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic );

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
    boost::shared_ptr<tinia::model::ExposedModel> m_model;
    Logic&                                      m_logic;
    bool                                        m_profiling_enabled;

    unsigned int                                m_frames;
    PerfTimer                                   m_update_timer;
    render::TimerQuery                          m_proxy_gen;
    render::TimerQuery                          m_surface_gen;
    render::TimerQuery                          m_surface_render;



};


} // of namespace models

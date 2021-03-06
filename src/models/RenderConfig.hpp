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
#include "models/Logic.hpp"
#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>

namespace models {

class RenderConfig
        : public tinia::model::StateListener
{
public:

    typedef int Revision;
    
    typedef int Theme;

    typedef enum {
        Solid = 0,
        Diffuse = 1,
        DiffuseSpecular = 2
    } ShadingModel;
    
    RenderConfig( boost::shared_ptr<tinia::model::ExposedModel>& model,
                  Logic& logic );

    ~RenderConfig();

    Revision
    revision() const { return m_revision; }
    
    void
    setLightTheme();

    void
    setDarkTheme();

    ShadingModel
    shadingModel() const
    { return m_shading_model; }
    
    void
    stateElementModified( tinia::model::StateElement * stateElement );

    tinia::model::gui::Element*
    guiFactory() const;

    int
    proxyResolution() const { return m_proxy_resolution; }
    
    bool
    renderGrid() const { return m_render_grid; }

    bool
    renderWells() const { return m_render_wells; }

    bool
    createNonindexedSurfaces() const { return m_create_nonindexed_surfaces; }
    
    int
    renderQuality() const { return m_render_quality; }

    const std::string&
    titleKey() const;

    const std::string&
    lightThemeKey() const;

    const std::string&
    renderWellsKey() const;

    const std::string&
    renderQualityStringKey() const;

    float
    lineThickness() const { return m_line_thickness; }

    const glm::vec4&
    backgroundColor() const { return m_background_color; }


    bool
    clipPlaneVisible() const { return m_clip_plane_visible; }

    const glm::vec4&
    clipPlaneColor() const { return m_clip_plane_color; }

    Theme
    theme() const { return m_theme; }

protected:
    boost::shared_ptr<tinia::model::ExposedModel>   m_model;
    Logic&                                          m_logic;
    Revision                                        m_revision;
    bool                                        m_reload;
    int                                             m_proxy_resolution;
    int                                         m_render_quality;
    ShadingModel                                m_shading_model;
    Theme                                       m_theme;
    bool                                        m_render_grid;
    bool                                        m_render_wells;
    bool                                        m_create_nonindexed_surfaces;

    float                                       m_line_thickness;
    glm::vec4                                   m_background_color;
    bool                                        m_clip_plane_visible;
    glm::vec4                                   m_clip_plane_color;

    void
    bumpRevision();
    
};









} // of namespace models

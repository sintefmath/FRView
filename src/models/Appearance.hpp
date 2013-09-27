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

namespace models {

class Appearance
        : public tinia::model::StateListener
{
public:
    typedef enum {
        VISIBILITY_MASK_NONE            = 0x0,
        VISIBILITY_MASK_SUBSET          = 0x1,
        VISIBILITY_MASK_BOUNDARY        = 0x2,
        VISIBILITY_MASK_FAULTS_INSIDE   = 0x4,
        VISIBILITY_MASK_FAULTS_OUTSIDE  = 0x8,
        VISIBILITY_MASK_FAULTS          = 0xc,
        VISIBILITY_MASK_ALL             = 0xf
    } VisibilityMask;

    typedef int Theme;

    Appearance( boost::shared_ptr<tinia::model::ExposedModel>& model,
                bool& reload );

    ~Appearance();

    VisibilityMask
    visibilityMask() const;

    void
    setLightTheme();

    void
    setDarkTheme();

    void
    stateElementModified( tinia::model::StateElement * stateElement );

    tinia::model::gui::Element*
    guiFactory() const;

    bool
    renderGrid() const { return m_render_grid; }

    bool
    renderWells() const { return m_render_wells; }

    int
    renderQuality() const { return m_render_quality; }

    const std::string&
    titleKey() const;

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

    const glm::vec4&
    subsetFillColor() const { return m_subset_fill_color; }

    const glm::vec4&
    subsetOutlineColor() const { return m_subset_outline_color; }

    const glm::vec4&
    boundaryFillColor() const { return m_boundary_fill_color; }

    const glm::vec4&
    boundaryOutlineColor() const { return m_boundary_outline_color; }

    const glm::vec4&
    faultsFillColor() const { return m_faults_fill_color; }

    const glm::vec4&
    faultsOutlineColor() const { return m_faults_outline_color; }

    Theme
    theme() const { return m_theme; }

protected:
    boost::shared_ptr<tinia::model::ExposedModel> m_model;
    bool&                                       m_reload;
    int                                         m_render_quality;
    Theme                                       m_theme;
    bool                                        m_render_grid;
    bool                                        m_render_wells;

    float                                       m_line_thickness;
    glm::vec4                                   m_background_color;
    bool                                        m_clip_plane_visible;
    glm::vec4                                   m_clip_plane_color;
    glm::vec4                                   m_subset_fill_color;
    glm::vec4                                   m_subset_outline_color;
    glm::vec4                                   m_boundary_fill_color;
    glm::vec4                                   m_boundary_outline_color;
    glm::vec4                                   m_faults_fill_color;
    glm::vec4                                   m_faults_outline_color;

};









} // of namespace models

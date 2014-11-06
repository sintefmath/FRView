#pragma once
/* Copyright STIFTELSEN SINTEF 2014
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

#include <glm/glm.hpp>
#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "models/Logic.hpp"

// fwd decl
struct SourceItem;

namespace models {

class AppearanceData
{
    friend class Appearance;
public:
    enum ColorMapType {
        COLORMAP_LINEAR     = 0,
        COLORMAP_LOGARITMIC = 1
    };
    typedef enum {
        VISIBILITY_MASK_NONE            = 0x0,
        VISIBILITY_MASK_SUBSET          = 0x1,
        VISIBILITY_MASK_BOUNDARY        = 0x2,
        VISIBILITY_MASK_FAULTS_INSIDE   = 0x4,
        VISIBILITY_MASK_FAULTS_OUTSIDE  = 0x8,
        VISIBILITY_MASK_FAULTS          = 0xc,
        VISIBILITY_MASK_ALL             = 0xf
    } VisibilityMask;

    ColorMapType
    colorMapType() const { return m_colormap_type; }

    bool
    colorMapFixed() const { return m_colormap_fixed; }
    
    double
    colorMapFixedMin() const { return m_colormap_fixed_min; }
    
    double
    colorMapFixedMax() const { return m_colormap_fixed_max; }

    bool
    flipOrientation() const { return m_flip_orientation; }
    
    VisibilityMask
    visibilityMask() const;

    glm::vec3
    subsetColor() const;

    float
    subsetFillAlpha() const { return m_subset_fill_alpha; }

    float
    subsetOutlineAlpha() const { return m_subset_outline_alpha; }


    glm::vec3
    boundaryColor() const;

    float
    boundaryFillAlpha() const { return m_boundary_fill_alpha; }

    float
    boundaryOutlineAlpha() const { return m_boundary_outline_alpha; }


    glm::vec3
    faultsColor() const;

    float
    faultsFillAlpha() const { return m_faults_fill_alpha; }

    float
    faultsOutlineAlpha() const { return m_faults_outline_alpha; }



protected:
    bool            m_visible;
    ColorMapType    m_colormap_type;
    bool            m_colormap_fixed;
    double          m_colormap_fixed_min;
    double          m_colormap_fixed_max;
    bool            m_flip_orientation;
    int             m_subset_color;
    float           m_subset_fill_alpha;
    float           m_subset_outline_alpha;
    int             m_boundary_color;
    float           m_boundary_fill_alpha;
    float           m_boundary_outline_alpha;
    int             m_faults_color;
    float           m_faults_fill_alpha;
    float           m_faults_outline_alpha;
};


class Appearance
        : public tinia::model::StateListener
{
public:
    Appearance( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic  );

    const std::string&
    titleKey() const;
    
    void
    stateElementModified( tinia::model::StateElement * stateElement );

    const std::string&
    flipOrientationKey() const;

    tinia::model::gui::Element*
    guiFactory() const;

    void
    update( boost::shared_ptr<SourceItem> source_item, size_t index );
    
protected:
    boost::shared_ptr<tinia::model::ExposedModel>   m_model;
    Logic&                                          m_logic;
    boost::shared_ptr<SourceItem>                   m_source_item;
};

}

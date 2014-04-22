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

#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "job/SourceItem.hpp"
#include "models/Logic.hpp"

namespace models {

class AppearanceData
{
    friend class Appearance;
public:
    enum ColorMapType {
        COLORMAP_LINEAR     = 0,
        COLORMAP_LOGARITMIC = 1
    };

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
    
protected:
    ColorMapType    m_colormap_type;
    bool            m_colormap_fixed;
    double          m_colormap_fixed_min;
    double          m_colormap_fixed_max;
    bool            m_flip_orientation;
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

    tinia::model::gui::Element*
    guiFactory() const;

    void
    update( boost::shared_ptr<SourceItem> source_item );
    
protected:
    boost::shared_ptr<tinia::model::ExposedModel>   m_model;
    Logic&                                          m_logic;
    boost::shared_ptr<SourceItem>                   m_source_item;
};

}

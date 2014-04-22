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

#include <string>
#include "render/GridField.hpp"
#include "models/Appearance.hpp"

namespace {
    const std::string appearance_title_key      = "apperance_title";
    const std::string colormap_label_key        = "colormap_label";
    const std::string colormap_type_key         = "colormap_type";
    const std::string field_range_enable_key    = "field_range_enable";
    const std::string field_range_min_key       = "field_range_min";
    const std::string field_range_max_key       = "field_range_max";
    const std::string tessellation_label_key    = "tessellation_label";
    const std::string tess_flip_orientation_key = "tess_flip_orientation";

    const std::string package                   = "models.Appearance";
    const double dmax                           = std::numeric_limits<double>::max();
    const char* colormap_types[]             =   { "Linear", "Logarithmic" };

}


namespace models {


Appearance::Appearance( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic  )
    : m_model( model ),
      m_logic( logic )
{
    m_model->addElement<bool>( appearance_title_key, true, "Appearance" );

    m_model->addElement<bool>( colormap_label_key, true, "Color map" );
    m_model->addElementWithRestriction<std::string>( colormap_type_key,
                                                    colormap_types[0],
                                                    &colormap_types[0],
                                                    &colormap_types[2] );
    m_model->addAnnotation( colormap_type_key, "Type" );
    m_model->addStateListener( colormap_type_key, this);  

    m_model->addElement<bool>( field_range_enable_key, false, "Lock min and max" );
    m_model->addStateListener( field_range_enable_key, this);    

    m_model->addConstrainedElement<double>( field_range_min_key, 0.0, -dmax, dmax, "Min" );
    m_model->addStateListener( field_range_min_key, this);    

    m_model->addConstrainedElement<double>( field_range_max_key, 0.0, -dmax, dmax, "Max" );
    m_model->addStateListener( field_range_max_key, this);    

    m_model->addElement<bool>( tessellation_label_key, true, "Tessellation" );

    m_model->addElement<bool>( tess_flip_orientation_key, false, "Flip orientation" );
    m_model->addStateListener( tess_flip_orientation_key, this);    

}

const std::string&
Appearance::titleKey() const
{
    return appearance_title_key;
}

void
Appearance::stateElementModified( tinia::model::StateElement * stateElement )
{
    //Logger log = getLogger( package + "stateElementModified" );
    using std::string;   

    if( !m_source_item || !m_source_item->m_appearance_data ) {
        return;
    }
    const std::string& key = stateElement->getKey();
    AppearanceData& ap = *m_source_item->m_appearance_data;
    
    if( key == field_range_enable_key ) {
        stateElement->getValue( ap.m_colormap_fixed );
        if( ap.m_colormap_fixed ) {
            // fixing just set, default to full range.
            if( m_source_item->m_grid_field ) {
                m_model->updateElement<double>( field_range_min_key, m_source_item->m_grid_field->minValue() );
                m_model->updateElement<double>( field_range_max_key, m_source_item->m_grid_field->maxValue() );
            }
        }
    }
    else if( key == colormap_type_key ) {
        std::string value;
        stateElement->getValue( value );
        if( value.length() >= 3 && value.substr(0, 3) == "Log" ) {
            ap.m_colormap_type = AppearanceData::COLORMAP_LOGARITMIC;
        }
        else {
            ap.m_colormap_type = AppearanceData::COLORMAP_LINEAR;
        }
    }
    else if( key == field_range_min_key ) {
        stateElement->getValue( ap.m_colormap_fixed_min );
    }
    else if( key == field_range_max_key ) {
        stateElement->getValue( ap.m_colormap_fixed_max );
    }
    else if( key == tess_flip_orientation_key ) {
        stateElement->getValue( ap.m_flip_orientation );
        m_source_item->m_do_update_subset = true;
        // Still needed?
        //if( m_renderlist_state == RENDERLIST_SENT ) {
        //    m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        //}
    }
}

tinia::model::gui::Element*
Appearance::guiFactory() const
{
    using namespace tinia::model::gui;
    

    Grid* colormap_range_grid = new Grid( 2, 3 );
    colormap_range_grid->setVisibilityKey( "field_range_enable" );
    colormap_range_grid->setChild( 0, 0, new Label( "field_range_min" ) );
    colormap_range_grid->setChild( 0, 1, new DoubleSpinBox( "field_range_min" ) );
    colormap_range_grid->setChild( 1, 0, new Label( "field_range_max" ) );
    colormap_range_grid->setChild( 1, 1, new DoubleSpinBox( "field_range_max" ) );
    colormap_range_grid->setChild( 0, 2, new HorizontalExpandingSpace );
    VerticalLayout* colormap_layout = new VerticalLayout;
    colormap_layout->addChild( new RadioButtons("colormap_type") );
    colormap_layout->addChild( new CheckBox("field_range_enable") );
    colormap_layout->addChild( colormap_range_grid);
    colormap_layout->addChild( new VerticalExpandingSpace );
    PopupButton* colormap_popup = new PopupButton( "colormap_type", true );
    colormap_popup->setChild( colormap_layout );
    //colormap_popup->setEnabledKey( "has_field" );
    
    
    Grid* surf_details_grid = new Grid( 4, 4 );
    surf_details_grid->setChild( 0, 1, new HorizontalSpace );
    surf_details_grid->setChild( 0, 3, new HorizontalExpandingSpace );
    surf_details_grid->setChild( 1, 0, new Label( "tessellation_label" ) );
    surf_details_grid->setChild( 1, 2, new CheckBox( "tess_flip_orientation" ) );
    surf_details_grid->setChild( 2, 0, new Label( "colormap_label" ) );
    surf_details_grid->setChild( 2, 2, colormap_popup );
    
    ElementGroup* surf_details_group = new ElementGroup( "surface_tab" );
    surf_details_group->setChild( surf_details_grid );
    
    VerticalLayout* layout = new VerticalLayout;
    layout->addChild( surf_details_group );
    return layout;
}

void
Appearance::update( boost::shared_ptr<SourceItem> source_item )
{
    m_source_item = source_item;
    if( !source_item ) {
        return;
    }
    
    if( !source_item->m_appearance_data ) {
        // If first time, set defaults
        m_source_item->m_appearance_data.reset( new AppearanceData );
        m_source_item->m_appearance_data->m_colormap_type = AppearanceData::COLORMAP_LINEAR;
        m_source_item->m_appearance_data->m_colormap_fixed = false;
        m_source_item->m_appearance_data->m_colormap_fixed_min = 0.f;
        m_source_item->m_appearance_data->m_colormap_fixed_max = 0.f;
        m_source_item->m_appearance_data->m_flip_orientation = false;
    }

    // Propagate from source item to GUI
    const AppearanceData& ap = *m_source_item->m_appearance_data;
    m_model->updateElement<std::string>( colormap_type_key, colormap_types[ ap.m_colormap_type ] );
    m_model->updateElement<bool>( field_range_enable_key, ap.m_colormap_fixed );
    m_model->updateElement<double>( field_range_min_key, ap.m_colormap_fixed_min );
    m_model->updateElement<double>( field_range_max_key, ap.m_colormap_fixed_max );
    m_model->updateElement<bool>( tess_flip_orientation_key, ap.m_flip_orientation );
}


} // of namespace models

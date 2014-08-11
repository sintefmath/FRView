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
#include "job/SourceItem.hpp"
#include "render/GridField.hpp"
#include "models/Appearance.hpp"

namespace {
    const std::string appearance_title_key      = "apperance_title";
    const std::string source_visible_key        = "source_visible";
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

    const std::string subset_label_key               = "subset_label";
    const std::string subset_color_key               = "subset_color";
    const std::string subset_opacity_fill_key        = "subset_fill_opacity";
    const std::string subset_opacity_outline_key     = "subset_outline_opacity";

    const std::string boundary_label_key             = "boundary_label";
    const std::string boundary_color_key             = "boundary_color";
    const std::string boundary_opacity_fill_key      = "boundary_fill_opacity";
    const std::string boundary_opacity_outline_key   = "boundary_outline_opacity";

    const std::string faults_label_key               = "faults_label";
    const std::string faults_color_key               = "faults_color";
    const std::string faults_opacity_fill_key        = "faults_fill_opacity";
    const std::string faults_opacity_outline_key     = "faults_outline_opacity";

    struct {
        std::string  m_name;
        unsigned char   m_red;
        unsigned char   m_green;
        unsigned char   m_blue;
    } colors[14] = {
        {"Dark grey",   0x44, 0x44, 0x44},
        {"Red",         0x68, 0x37, 0x2B},
        {"Light blue",  0x6C, 0x5E, 0xB5},
        {"Light green", 0x9A, 0xD2, 0x84},
        {"Light grey",  0x95, 0x95, 0x95},
        {"Light red",   0x9A, 0x67, 0x59},
        {"Blue",        0x35, 0x28, 0x79},
        {"Green",       0x58, 0x8D, 0x43},
        {"Yellow",      0xB8, 0xC7, 0x6F},
        {"Grey",        0x6C, 0x6C, 0x6C},
        {"Cyan",        0x70, 0xA4, 0xB2},
        {"Purple",      0x6F, 0x3D, 0x86},
        {"Orange",      0x6F, 0x4F, 0x25},
        {"Brown",       0x43, 0x39, 0x00}
    };

}


namespace models {

AppearanceData::VisibilityMask
AppearanceData::visibilityMask() const
{
    if( m_visible ) {
        int mask =
                (m_subset_fill_alpha      > 0.f  ? VISIBILITY_MASK_SUBSET : VISIBILITY_MASK_NONE ) |
                (m_subset_outline_alpha   > 0.f  ? VISIBILITY_MASK_SUBSET : VISIBILITY_MASK_NONE ) |
                (m_boundary_fill_alpha    > 0.f  ? VISIBILITY_MASK_BOUNDARY : VISIBILITY_MASK_NONE ) |
                (m_boundary_outline_alpha > 0.f  ? VISIBILITY_MASK_BOUNDARY : VISIBILITY_MASK_NONE ) |
                ((m_faults_fill_alpha     > 0.f) && (m_subset_fill_alpha < 1.f) ? VISIBILITY_MASK_FAULTS_INSIDE : VISIBILITY_MASK_NONE ) |
                ((m_faults_outline_alpha  > 0.f) && (m_subset_fill_alpha < 1.f) ? VISIBILITY_MASK_FAULTS_INSIDE : VISIBILITY_MASK_NONE ) |
                (m_faults_fill_alpha      > 0.f  ? VISIBILITY_MASK_FAULTS_OUTSIDE : VISIBILITY_MASK_NONE ) |
                (m_faults_outline_alpha   > 0.f  ? VISIBILITY_MASK_FAULTS_OUTSIDE : VISIBILITY_MASK_NONE );
        return static_cast<VisibilityMask>( mask );
    }
    return VISIBILITY_MASK_NONE;
}

glm::vec3
AppearanceData::subsetColor() const
{
    return glm::vec3( (1.f/255.f)*colors[ m_subset_color ].m_red,
                      (1.f/255.f)*colors[ m_subset_color ].m_green,
                      (1.f/255.f)*colors[ m_subset_color ].m_blue );
}


glm::vec3
AppearanceData::boundaryColor() const
{
    return glm::vec3( (1.f/255.f)*colors[ m_boundary_color ].m_red,
                      (1.f/255.f)*colors[ m_boundary_color ].m_green,
                      (1.f/255.f)*colors[ m_boundary_color ].m_blue );
}


glm::vec3
AppearanceData::faultsColor() const
{
    return glm::vec3( (1.f/255.f)*colors[ m_faults_color ].m_red,
                      (1.f/255.f)*colors[ m_faults_color ].m_green,
                      (1.f/255.f)*colors[ m_faults_color ].m_blue );
}


Appearance::Appearance( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic  )
    : m_model( model ),
      m_logic( logic )
{
    m_model->addElement<bool>( appearance_title_key, true, "Appearance" );

    m_model->addElement<bool>( source_visible_key, true, "Visible" );
    m_model->addStateListener( source_visible_key, this );

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


    std::vector<std::string> color_names;
    for(unsigned int i=0; i<sizeof(colors)/sizeof(colors[0]); i++ ) {
        color_names.push_back( colors[i].m_name );
    }

    m_model->addElement<bool>( subset_label_key, true, "Subset" );

    m_model->addElementWithRestriction<std::string>( subset_color_key,
                                                     color_names[0],
                                                     color_names.begin(),
                                                     color_names.end() );
    m_model->addAnnotation( subset_color_key, "Color" );
    m_model->addStateListener( subset_color_key, this );
    m_model->addConstrainedElement<int>( subset_opacity_fill_key,
                                         0, 0, 100, "Fill" );
    m_model->addStateListener( subset_opacity_fill_key, this );
    m_model->addConstrainedElement<int>( subset_opacity_outline_key,
                                         0, 0, 100, "Outlines");
    m_model->addStateListener( subset_opacity_outline_key, this );

    m_model->addElement<bool>( faults_label_key, true, "Faults" );
    m_model->addElementWithRestriction<std::string>( faults_color_key,
                                                     color_names[0],
                                                     color_names.begin(),
                                                     color_names.end() );
    m_model->addAnnotation( faults_color_key, "Color" );
    m_model->addStateListener( faults_color_key, this );
    m_model->addConstrainedElement<int>( faults_opacity_fill_key,
                                         0, 0, 100, "Fill" );
    m_model->addStateListener( faults_opacity_fill_key, this );
    m_model->addConstrainedElement<int>( faults_opacity_outline_key,
                                         0, 0, 100, "Outlines" );
    m_model->addStateListener( faults_opacity_outline_key, this );

    m_model->addElement<bool>( boundary_label_key, true, "Grid boundary" );
    m_model->addElementWithRestriction<std::string>( boundary_color_key,
                                                     color_names[0],
                                                     color_names.begin(),
                                                     color_names.end() );
    m_model->addAnnotation( boundary_color_key, "Color" );
    m_model->addStateListener( boundary_color_key, this );
    m_model->addConstrainedElement<int>( boundary_opacity_fill_key,
                                         0, 0, 100, "Fill" );
    m_model->addStateListener( boundary_opacity_fill_key, this );
    m_model->addConstrainedElement<int>( boundary_opacity_outline_key,
                                         0, 0, 100, "Outlines" );
    m_model->addStateListener( boundary_opacity_outline_key, this );

    m_model->addElement<bool>( "surface_tab", true, "Surfaces" );

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
    
    if( key == source_visible_key ) {
        stateElement->getValue( ap.m_visible );
        m_source_item->m_do_update_subset = true;
    }
    else if( key == field_range_enable_key ) {
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
    else if( key == subset_color_key ) {
        std::string value;
        stateElement->getValue( value );
        for(unsigned int i=0; i<sizeof(colors)/sizeof(colors[0]); i++ ) {
            if( colors[i].m_name == value ) {
                ap.m_subset_color = i;
                break;
            }
        }
    }
    else if( key == subset_opacity_fill_key ) {
        int val;
        stateElement->getValue( val );
        ap.m_subset_fill_alpha = 0.01f*val;
    }
    else if( key == subset_opacity_outline_key ) {
        int val;
        stateElement->getValue( val );
        ap.m_subset_outline_alpha = 0.01f*val;
    }
    else if( key == boundary_color_key ) {
        std::string value;
        stateElement->getValue( value );
        for(unsigned int i=0; i<sizeof(colors)/sizeof(colors[0]); i++ ) {
            if( colors[i].m_name == value ) {
                ap.m_boundary_color = i;
                break;
            }
        }
    }
    else if( key == boundary_opacity_fill_key ) {
        int val;
        stateElement->getValue( val );
        ap.m_boundary_fill_alpha = 0.01f*val;
    }
    else if( key == boundary_opacity_outline_key ) {
        int val;
        stateElement->getValue( val );
        ap.m_boundary_outline_alpha = 0.01f*val;
    }

    else if( key == faults_color_key ) {
        std::string value;
        stateElement->getValue( value );
        for(unsigned int i=0; i<sizeof(colors)/sizeof(colors[0]); i++ ) {
            if( colors[i].m_name == value ) {
                ap.m_faults_color = i;
                break;
            }
        }
    }
    else if( key == faults_opacity_fill_key ) {
        int val;
        stateElement->getValue( val );
        ap.m_faults_fill_alpha = 0.01f*val;
    }
    else if( key == faults_opacity_outline_key ) {
        int val;
        stateElement->getValue( val );
        ap.m_faults_outline_alpha = 0.01f*val;
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
    
    
    Grid* surf_details_grid = new Grid( 2, 4 );
    surf_details_grid->setChild( 0, 1, new HorizontalSpace );
    surf_details_grid->setChild( 0, 3, new HorizontalExpandingSpace );
    surf_details_grid->setChild( 1, 0, (new Label( "colormap_label" ))->setVisibilityKey( "has_field" ) );
    surf_details_grid->setChild( 1, 2, (colormap_popup)->setVisibilityKey( "has_field" ) );
    
    VerticalLayout* options_layout = new VerticalLayout;
    options_layout->addChild( new CheckBox( tess_flip_orientation_key ) );
    options_layout->addChild( new CheckBox( source_visible_key ) );
    options_layout->addChild( surf_details_grid );
    
    ElementGroup* surf_details_group = new ElementGroup( "surface_tab" );
    surf_details_group->setChild( options_layout );
    
    VerticalLayout* layout = new VerticalLayout;
    layout->addChild( surf_details_group );

    // -------------------------------------------------------------------------
    ElementGroup* alpha_subset_group = new ElementGroup( subset_label_key, true );
    layout->addChild( alpha_subset_group );
    Grid* alpha_subset_layout = new Grid(4,2);
    alpha_subset_group->setChild( alpha_subset_layout );
    alpha_subset_layout->setChild( 0, 0, (new Label( subset_color_key ))->setVisibilityKey( "has_field", true ) );
    alpha_subset_layout->setChild( 0, 1, (new ComboBox( subset_color_key ))->setVisibilityKey( "has_field", true ) );
    alpha_subset_layout->setChild( 1, 0, new Label( subset_opacity_fill_key ) );
    alpha_subset_layout->setChild( 1, 1, new HorizontalSlider( subset_opacity_fill_key ) );
    alpha_subset_layout->setChild( 2, 0, new Label( subset_opacity_outline_key ) );
    alpha_subset_layout->setChild( 2, 1, new HorizontalSlider( subset_opacity_outline_key ) );
    alpha_subset_layout->setChild( 3, 1, new HorizontalExpandingSpace );

    // -------------------------------------------------------------------------
    {
        ElementGroup* alpha_boundary_group = new ElementGroup( boundary_label_key, true );
        layout->addChild( alpha_boundary_group );
        Grid* alpha_boundary_layout = new Grid(4,2);
        alpha_boundary_group->setChild( alpha_boundary_layout );

        alpha_boundary_layout->setChild( 0, 0, (new Label( boundary_color_key ))->setVisibilityKey( "has_field", true ) );
        alpha_boundary_layout->setChild( 0, 1, (new ComboBox( boundary_color_key))->setVisibilityKey( "has_field", true ) );
        alpha_boundary_layout->setChild( 1, 0, new Label( boundary_opacity_fill_key ) );
        alpha_boundary_layout->setChild( 1, 1, new HorizontalSlider( boundary_opacity_fill_key ) );
        alpha_boundary_layout->setChild( 2, 0, new Label( boundary_opacity_outline_key ) );
        alpha_boundary_layout->setChild( 2, 1, new HorizontalSlider( boundary_opacity_outline_key ) );
        alpha_boundary_layout->setChild( 3, 1, new HorizontalExpandingSpace );
    }

    // -------------------------------------------------------------------------
    ElementGroup* alpha_faults_group = new ElementGroup( faults_label_key, true );
    layout->addChild( alpha_faults_group );
    Grid* alpha_faults_layout = new Grid(4,2);
    alpha_faults_group->setChild( alpha_faults_layout );
    alpha_faults_layout->setChild( 0, 0, new Label( faults_color_key ) );
    alpha_faults_layout->setChild( 0, 1, new ComboBox( faults_color_key ) );
    alpha_faults_layout->setChild( 1, 0, new Label( faults_opacity_fill_key ) );
    alpha_faults_layout->setChild( 1, 1, new HorizontalSlider( faults_opacity_fill_key ) );
    alpha_faults_layout->setChild( 2, 0, new Label( faults_opacity_outline_key ) );
    alpha_faults_layout->setChild( 2, 1, new HorizontalSlider( faults_opacity_outline_key ) );
    alpha_faults_layout->setChild( 3, 1, new HorizontalExpandingSpace );


    return layout;
}

void
Appearance::update(boost::shared_ptr<SourceItem> source_item , size_t index)
{
    m_source_item = source_item;
    if( !source_item ) {
        return;
    }
    
    if( !source_item->m_appearance_data ) {
        // If first time, set defaults
        m_source_item->m_appearance_data.reset( new AppearanceData );
        m_source_item->m_appearance_data->m_visible = true;
        m_source_item->m_appearance_data->m_colormap_type = AppearanceData::COLORMAP_LINEAR;
        m_source_item->m_appearance_data->m_colormap_fixed = false;
        m_source_item->m_appearance_data->m_colormap_fixed_min = 0.f;
        m_source_item->m_appearance_data->m_colormap_fixed_max = 0.f;
        m_source_item->m_appearance_data->m_flip_orientation = false;


        m_source_item->m_appearance_data->m_subset_color = std::min( sizeof(colors)/sizeof(colors[0])-1,
                                                                     index+2 );
        m_source_item->m_appearance_data->m_subset_fill_alpha = 1.f;
        m_source_item->m_appearance_data->m_subset_outline_alpha = 1.f;
        m_source_item->m_appearance_data->m_boundary_color = 0;
        m_source_item->m_appearance_data->m_boundary_fill_alpha = 0.f;
        m_source_item->m_appearance_data->m_boundary_outline_alpha = 0.f;
        m_source_item->m_appearance_data->m_faults_color = 1;
        m_source_item->m_appearance_data->m_faults_fill_alpha = 1.f;
        m_source_item->m_appearance_data->m_faults_outline_alpha = 1.f;

    }

    // Propagate from source item to GUI
    const AppearanceData& ap = *m_source_item->m_appearance_data;
    m_model->updateElement<bool>( source_visible_key, ap.m_visible );
    m_model->updateElement<std::string>( colormap_type_key, colormap_types[ ap.m_colormap_type ] );
    m_model->updateElement<bool>( field_range_enable_key, ap.m_colormap_fixed );
    m_model->updateElement<double>( field_range_min_key, ap.m_colormap_fixed_min );
    m_model->updateElement<double>( field_range_max_key, ap.m_colormap_fixed_max );
    m_model->updateElement<bool>( tess_flip_orientation_key, ap.m_flip_orientation );

    m_model->updateElement<std::string>(subset_color_key, colors[ap.m_subset_color].m_name );
    m_model->updateElement<int>( subset_opacity_fill_key, static_cast<int>(100*ap.m_subset_fill_alpha ) );
    m_model->updateElement<int>( subset_opacity_outline_key, static_cast<int>(100*ap.m_subset_outline_alpha ) );

    m_model->updateElement<std::string>(boundary_color_key, colors[ap.m_boundary_color].m_name );
    m_model->updateElement<int>( boundary_opacity_fill_key, static_cast<int>(100*ap.m_boundary_fill_alpha ) );
    m_model->updateElement<int>( boundary_opacity_outline_key, static_cast<int>(100*ap.m_boundary_outline_alpha ) );

    m_model->updateElement<std::string>(faults_color_key, colors[ap.m_faults_color].m_name );
    m_model->updateElement<int>( faults_opacity_fill_key, static_cast<int>(100*ap.m_faults_fill_alpha ) );
    m_model->updateElement<int>( faults_opacity_outline_key, static_cast<int>(100*ap.m_faults_outline_alpha ) );
}


} // of namespace models

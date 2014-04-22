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
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include "utils/Logger.hpp"
#include "models/SubsetSelector.hpp"
#include "render/ClipPlane.hpp"
#include "render/GridField.hpp"
#include "dataset/AbstractDataSource.hpp"
#include "dataset/CellLayoutInterface.hpp"

namespace {

    const std::string surface_subset_field_range_key = "surface_subset_field_range";

    const std::string field_select_min_key = "field_select_min";
    const std::string field_select_max_key = "field_select_max";
    const std::string field_select_solution_override_key = "field_select_solution_override";
    const std::string field_select_solution_key = "field_select_solution";

    const std::string field_select_report_step_override_key = "field_select_report_step_override";

    const std::string field_select_report_step_key = "field_select_report_step";


    const std::string index_range_select_min_i_key = "index_range_select_min_i";
    const std::string index_range_select_max_i_key = "index_range_select_max_i";
    const std::string index_range_select_min_j_key = "index_range_select_min_j";
    const std::string index_range_select_max_j_key = "index_range_select_max_j";
    const std::string index_range_select_min_k_key = "index_range_select_min_k";
    const std::string index_range_select_max_k_key = "index_range_select_max_k";
    
    const std::string plane_select_NX_key = "plane_select_NX";
    const std::string plane_select_PX_key = "plane_select_PX";
    const std::string plane_select_NY_key = "plane_select_NY";
    const std::string plane_select_PY_key = "plane_select_PY";
    const std::string plane_select_NZ_key = "plane_select_NZ";
    const std::string plane_select_PZ_key = "plane_select_PZ";
    
    const std::string plane_select_shift_key = "plane_select_shift";
    const std::string surface_subset_plane_key = "surface_subset_plane";
    const std::string surface_subset_index_range_key = "surface_subset_index_range";
    
    const std::string surface_subset_key = "surface_subset";
    
    const std::string details_label_key = "details_label";

    const std::string package = "models.SubsetSelectorData";

    const char* subsets[models::SubsetSelectorData::SELECTOR_N] = { "all",
                                                                    "subset_field",
                                                                    "subset_index",
                                                                    "subset_plane",
                                                                    "subset_halfplane" };
    
}

namespace models {



SubsetSelector::SubsetSelector( boost::shared_ptr<tinia::model::ExposedModel>& model,
                                Logic& logic  )
    : m_model( model ),
      m_logic( logic )
{
    std::list<std::string> solutions = { "[none]" };
    const double dmax = std::numeric_limits<double>::max();

    // --- field variables -----------------------------------------------------
    m_model->addElement<bool>( surface_subset_field_range_key, false );
    
    m_model->addConstrainedElement<double>( field_select_min_key, 0.0, -dmax, dmax, "Min" );
    m_model->addStateListener( field_select_min_key, this);

    m_model->addConstrainedElement<double>( field_select_max_key, 0.0, -dmax, dmax, "Max" );
    m_model->addStateListener( field_select_max_key, this);

    // --- plane variables -----------------------------------------------------
    m_model->addElement<bool>( surface_subset_plane_key, false );

    m_model->addElement<bool>( field_select_solution_override_key, false );
    m_model->addElementWithRestriction<std::string>( field_select_solution_key,
                                                     solutions.front(),
                                                     solutions.begin(),
                                                     solutions.end() );
    
    m_model->addElement<bool>( plane_select_PY_key, false, "PY" );
    m_model->addStateListener( plane_select_PY_key, this);

    m_model->addElement<bool>( plane_select_NY_key, false, "NY" );
    m_model->addStateListener( plane_select_NY_key, this);

    m_model->addElement<bool>( plane_select_PX_key, false, "PX" );
    m_model->addStateListener( plane_select_PX_key, this);

    m_model->addElement<bool>( plane_select_NX_key, false, "NX" );
    m_model->addStateListener( plane_select_NX_key, this);

    m_model->addConstrainedElement( plane_select_shift_key, 0, -500, 500, "Shift" );
    m_model->addStateListener( plane_select_shift_key, this);
    

    // --- logical index variables ---------------------------------------------
    m_model->addElement<bool>( surface_subset_index_range_key, false );
    
    m_model->addConstrainedElement<int>( index_range_select_min_i_key, 0, 0, 0, "I" );
    m_model->addStateListener( index_range_select_min_i_key, this);

    m_model->addConstrainedElement<int>( index_range_select_max_i_key, 0, 0, 0, "I" );
    m_model->addStateListener( index_range_select_max_i_key, this);

    m_model->addConstrainedElement<int>( index_range_select_min_j_key, 0, 0, 0, "J" );
    m_model->addStateListener( index_range_select_min_j_key, this);

    m_model->addConstrainedElement<int>( index_range_select_max_j_key, 0, 0, 0, "J" );
    m_model->addStateListener( index_range_select_max_j_key, this);
    
    m_model->addConstrainedElement<int>( index_range_select_min_k_key, 0, 0, 0, "k" );
    m_model->addStateListener( index_range_select_min_k_key, this);

    m_model->addConstrainedElement<int>( index_range_select_max_k_key, 0, 0, 0, "K" );
    m_model->addStateListener( index_range_select_max_k_key, this);
    
    // --- chooser -------------------------------------------------------------
    m_model->addElementWithRestriction<std::string>( surface_subset_key,
                                                    subsets[0],
                                                    &subsets[0],
                                                    &subsets[5] );
    m_model->addStateListener( surface_subset_key, this);

}
    
void
SubsetSelector::stateElementModified( tinia::model::StateElement * stateElement )
{
    Logger log = getLogger( package + "stateElementModified" );
    using std::string;   
    
    if( !m_source_item || !m_source_item->m_subset_selector_data) {
        return;
    }
    SubsetSelectorData& ssd = *m_source_item->m_subset_selector_data;
    
    const std::string& key = stateElement->getKey();

    // --- change of selector --------------------------------------------------
    if( key == surface_subset_key ) {
        string value;
        stateElement->getValue<string>( value );

        SubsetSelectorData::SelectorType type = SubsetSelectorData::SELECTOR_N;
        for(int i=0; i<SubsetSelectorData::SELECTOR_N; i++ ) {
            if( value == subsets[i] ) {
                type = static_cast<SubsetSelectorData::SelectorType>(i);
            }
        }
        switch( type ) {
        case SubsetSelectorData::SELECTOR_ALL:
            m_model->updateElement( surface_subset_field_range_key, false );
            m_model->updateElement( surface_subset_index_range_key, false );
            m_model->updateElement( surface_subset_plane_key, false );
            break;
            
        case SubsetSelectorData::SELECTOR_FIELD:
            m_model->updateElement( surface_subset_field_range_key, true );
            m_model->updateElement( surface_subset_index_range_key, false );
            m_model->updateElement( surface_subset_plane_key, false );
            break;
            
        case SubsetSelectorData::SELECTOR_INDEX:
            m_model->updateElement( surface_subset_field_range_key, false );
            m_model->updateElement( surface_subset_index_range_key, true );
            m_model->updateElement( surface_subset_plane_key, false );
            break;
            
        case SubsetSelectorData::SELECTOR_PLANE:
            m_model->updateElement( surface_subset_field_range_key, false );
            m_model->updateElement( surface_subset_index_range_key, false );
            m_model->updateElement( surface_subset_plane_key, true );
            break;
            
        case SubsetSelectorData::SELECTOR_HALFPLANE:
            m_model->updateElement( surface_subset_field_range_key, false );
            m_model->updateElement( surface_subset_index_range_key, false );
            m_model->updateElement( surface_subset_plane_key, true );
            break;
            
        case SubsetSelectorData::SELECTOR_N:
            // this should never happen.
            LOGGER_FATAL( log, "element '" << surface_subset_key << "' got illegal value '" << value << "'." );
            break;
        }
        
        m_source_item->m_subset_selector_data->m_selector_type = type;
        m_source_item->m_do_update_subset = true;
    }
    else if( key == plane_select_PX_key ) {
        bool value;
        stateElement->getValue( value );
        if( value ) {
            m_model->updateElement( key, false );
            m_source_item->m_clip_plane->rotate( glm::quat( glm::vec3( 0.f, 0.f, -0.1*M_PI_4 ) ) );
            m_source_item->m_do_update_subset = true;
        }
    }
    else if( key == plane_select_NX_key ) {
        bool value;
        stateElement->getValue( value );
        if( value ) {
            m_model->updateElement( key, false );
            m_source_item->m_clip_plane->rotate( glm::quat( glm::vec3( 0.f, 0.f, 0.1*M_PI_4 ) ) );
            m_source_item->m_do_update_subset = true;
        }
    }
    else if( key == plane_select_PY_key ) {
        bool value;
        stateElement->getValue( value );
        if( value ) {
            m_model->updateElement( key, false );
            m_source_item->m_clip_plane->rotate( glm::quat( glm::vec3( 0.1*M_PI_4, 0.f, 0.f ) ) );
            m_source_item->m_do_update_subset = true;
        }
    }
    else if( key == plane_select_NY_key ) {
        bool value;
        stateElement->getValue( value );
        if( value ) {
            m_model->updateElement( key, false );
            m_source_item->m_clip_plane->rotate( glm::quat( glm::vec3( -0.1*M_PI_4, 0.f, 0.f ) ) );
            m_source_item->m_do_update_subset = true;
        }
    }
    else if( key == plane_select_shift_key ) {
        int value;
        stateElement->getValue<int>( value );
        m_source_item->m_clip_plane->setOffset( (1.7f/1000.f)*(value) );
        m_source_item->m_do_update_subset = true;
    }
    else if( key == index_range_select_min_i_key ) {
        stateElement->getValue<int>( ssd.m_nx_min );
        if( ssd.m_nx_max < ssd.m_nx_min ) {
            ssd.m_nx_max = ssd.m_nx_min;
            m_model->updateElement( index_range_select_max_i_key, ssd.m_nx_max );
        }
        m_source_item->m_do_update_subset = true;
    }
    else if( key == index_range_select_max_i_key ) {
        stateElement->getValue<int>( ssd.m_nx_max );
        if( ssd.m_nx_max < ssd.m_nx_min ) {
            ssd.m_nx_min = ssd.m_nx_max;
            m_model->updateElement( index_range_select_min_i_key, ssd.m_nx_min );
        }
        m_source_item->m_do_update_subset = true;
    }
    else if( key == index_range_select_min_j_key ) {
        stateElement->getValue<int>( ssd.m_ny_min );
        if( ssd.m_ny_max < ssd.m_ny_min ) {
            ssd.m_ny_max = ssd.m_ny_min;
            m_model->updateElement( index_range_select_max_j_key, ssd.m_ny_max );
        }
        m_source_item->m_do_update_subset = true;
    }
    else if( key == index_range_select_max_j_key ) {
        stateElement->getValue<int>( ssd.m_ny_max );
        if( ssd.m_ny_max < ssd.m_ny_min ) {
            ssd.m_ny_min = ssd.m_ny_max;
            m_model->updateElement( index_range_select_min_j_key, ssd.m_ny_min );
        }
        m_source_item->m_do_update_subset = true;
    }
    else if( key == index_range_select_min_k_key ) {
        stateElement->getValue<int>( ssd.m_nz_min );
        if( ssd.m_nz_max < ssd.m_nz_min ) {
            ssd.m_nz_max = ssd.m_nz_min;
            m_model->updateElement( index_range_select_max_k_key, ssd.m_nz_max );
        }
        m_source_item->m_do_update_subset = true;
    }
    else if( key == index_range_select_max_k_key ) {
        stateElement->getValue<int>( ssd.m_nz_max );
        if( ssd.m_nz_max < ssd.m_nz_min ) {
            ssd.m_nz_min = ssd.m_nz_max;
            m_model->updateElement( index_range_select_min_k_key, ssd.m_nz_min );
        }
        m_source_item->m_do_update_subset = true;
    }
    else if( key == field_select_min_key ) {
        stateElement->getValue( ssd.m_field_select_min );
        m_source_item->m_do_update_subset = true;
    }
    else if( key == field_select_max_key ) {
        stateElement->getValue( ssd.m_field_select_max );
        m_source_item->m_do_update_subset = true;
    }

}

tinia::model::gui::Element*
SubsetSelector::guiFactory() const
{
    using namespace tinia::model::gui;

    // subset field range
    Grid* subset_field_minmax_layout = new Grid( 3, 2 );
    subset_field_minmax_layout->setChild( 0, 0, new Label(field_select_min_key) );
    subset_field_minmax_layout->setChild( 0, 1, new DoubleSpinBox(field_select_min_key) );
    subset_field_minmax_layout->setChild( 1, 0, new Label(field_select_max_key) );
    subset_field_minmax_layout->setChild( 1, 1, new DoubleSpinBox(field_select_max_key) );
    subset_field_minmax_layout->setChild( 2, 1, new HorizontalExpandingSpace );
    VerticalLayout* subset_field_layout = new VerticalLayout;
    subset_field_layout->addChild( subset_field_minmax_layout );
/*
    subset_field_layout->addChild( new CheckBox( field_select_solution_override_key ) );
    subset_field_layout->addChild( (new ComboBox( field_select_solution_key ))
                                   ->setVisibilityKey( field_select_solution_override_key ) );
    subset_field_layout->addChild( new CheckBox( field_select_report_step_override_key ) );
    subset_field_layout->addChild( (new HorizontalSlider( field_select_report_step_key ))
                                   ->setVisibilityKey(field_select_report_step_override_key) );
*/
    //subset_field_layout->addChild( new VerticalExpandingSpace );
    subset_field_layout->setVisibilityKey( "surface_subset_field_range" );

    
    //PopupButton* subset_field_popup = new PopupButton( details_label_key, false );
    //subset_field_popup->setChild( subset_field_layout );
    //subset_field_popup->setVisibilityKey( "surface_subset_field_range" );

    // subset index
    Grid* subset_index_grid = new Grid( 4, 4 );
    subset_index_grid->setChild( 0, 3, new HorizontalExpandingSpace );
    subset_index_grid->setChild( 0, 0, new Label( index_range_select_min_i_key ));
    subset_index_grid->setChild( 0, 1, new SpinBox( index_range_select_min_i_key ));
    subset_index_grid->setChild( 0, 2, new SpinBox( index_range_select_max_i_key ));
    subset_index_grid->setChild( 1, 0, new Label( index_range_select_min_j_key ));
    subset_index_grid->setChild( 1, 1, new SpinBox( index_range_select_min_j_key ));
    subset_index_grid->setChild( 1, 2, new SpinBox( index_range_select_max_j_key ));
    subset_index_grid->setChild( 2, 0, new Label( index_range_select_min_k_key ));
    subset_index_grid->setChild( 2, 1, new SpinBox( index_range_select_min_k_key ));
    subset_index_grid->setChild( 2, 2, new SpinBox( index_range_select_max_k_key ));
    subset_index_grid->setVisibilityKey( surface_subset_index_range_key );

    // --- subset plane ---
    Grid* geo_subset_plane_rot_grid = new Grid( 3, 3 );
    geo_subset_plane_rot_grid->setChild( 1, 0, new Button( plane_select_NX_key ) );
    geo_subset_plane_rot_grid->setChild( 1, 2, new Button( plane_select_PX_key ) );
    geo_subset_plane_rot_grid->setChild( 0, 1, new Button( plane_select_PY_key ) );
    geo_subset_plane_rot_grid->setChild( 2, 1, new Button( plane_select_NY_key ) );
    VerticalLayout* subset_plane_layout = new VerticalLayout;
    subset_plane_layout->addChild( geo_subset_plane_rot_grid );
    subset_plane_layout->addChild( new HorizontalSlider( plane_select_shift_key ) );
    subset_plane_layout->setVisibilityKey( surface_subset_plane_key );

    
    VerticalLayout* subsets_layout = new VerticalLayout;
    subsets_layout->addChild( new tinia::model::gui::ComboBox( surface_subset_key ) );
    subsets_layout->addChild( subset_field_layout );
    subsets_layout->addChild( subset_index_grid);
    subsets_layout->addChild( subset_plane_layout );
    
    
    return subsets_layout;
}

void
SubsetSelector::update(boost::shared_ptr<SourceItem> source_item )
{
    using std::string;
    using boost::shared_ptr;
    using boost::dynamic_pointer_cast;
    using dataset::CellLayoutInterface;
    m_source_item = source_item;

    // --- no valid source item, set defaults ----------------------------------
    if( !m_source_item ) {
        std::list<std::string> solutions = {"none"};
        m_model->updateRestrictions( "field_select_solution", solutions.front(), solutions );
        m_model->updateConstraints<int>("field_report_step", 0, 0, 0 );
        m_model->updateConstraints<int>( "field_select_report_step", 0, 0, 0 );
        m_model->updateConstraints<int>( "index_range_select_min_i", 0, 0, 0 );
        m_model->updateConstraints<int>( "index_range_select_max_i", 0, 0, 0 );
        m_model->updateConstraints<int>( "index_range_select_min_j", 0, 0, 0 );
        m_model->updateConstraints<int>( "index_range_select_max_j", 0, 0, 0 );
        m_model->updateConstraints<int>( "index_range_select_min_k", 0, 0, 0 );
        m_model->updateConstraints<int>( "index_range_select_max_k", 0, 0, 0 );
        return;
    }
    
    shared_ptr<CellLayoutInterface> cl = dynamic_pointer_cast<CellLayoutInterface>( m_source_item->m_source );

    // --- initialize data for this source if first time it is encountered -----
    if( !m_source_item->m_subset_selector_data ) {
        m_source_item->m_subset_selector_data.reset( new SubsetSelectorData );
        
        m_source_item->m_subset_selector_data->m_selector_type = SubsetSelectorData::SELECTOR_ALL;
        m_source_item->m_subset_selector_data->m_dim = 0;
        m_source_item->m_subset_selector_data->m_field_select_min = 0.0;
        m_source_item->m_subset_selector_data->m_field_select_max = 0.0;
        m_source_item->m_subset_selector_data->m_nx_min = 0;
        m_source_item->m_subset_selector_data->m_nx_max = 0;
        m_source_item->m_subset_selector_data->m_ny_min = 0;
        m_source_item->m_subset_selector_data->m_ny_max = 0;
        m_source_item->m_subset_selector_data->m_nz_min = 0;
        m_source_item->m_subset_selector_data->m_nz_max = 0;
        
        if( m_source_item->m_grid_field ) {
            m_source_item->m_subset_selector_data->m_field_select_min = m_source_item->m_grid_field->minValue();
            m_source_item->m_subset_selector_data->m_field_select_max = m_source_item->m_grid_field->maxValue();
        }
        
        if( cl ) {
            int dim = cl->indexDim();
            m_source_item->m_subset_selector_data->m_dim = dim;
            if( dim > 0 ) {
                m_source_item->m_subset_selector_data->m_nx_min = cl->minIndex(0);
                m_source_item->m_subset_selector_data->m_nx_max = std::max( 1, cl->maxIndex(0) ) - 1u;
                if( dim > 1 ) {
                    m_source_item->m_subset_selector_data->m_ny_min = cl->minIndex(1);
                    m_source_item->m_subset_selector_data->m_ny_max = std::max( 1, cl->maxIndex(1) ) - 1u;
                    if( dim > 2 ) {
                        m_source_item->m_subset_selector_data->m_nz_min = cl->minIndex(2);
                        m_source_item->m_subset_selector_data->m_nz_max = std::max( 1, cl->maxIndex(2) ) - 1u;
                    }
                }
            }
        }
    }

    // --- push restrictions from newly selected source to GUI -----------------
    if( m_source_item->m_subset_selector_data->m_selector_type < SubsetSelectorData::SELECTOR_N ) {
        m_model->updateElement<string>( surface_subset_key,
                                        subsets[ m_source_item->m_subset_selector_data->m_selector_type ] );
    }

    if( cl && cl->indexDim() > 0 ) {
        m_model->updateConstraints<int>( index_range_select_min_i_key,
                                         m_source_item->m_subset_selector_data->m_nx_min,
                                         cl->minIndex(0),
                                         std::max( 1, cl->maxIndex(0) ) - 1u );
        m_model->updateConstraints<int>( index_range_select_max_i_key,
                                         m_source_item->m_subset_selector_data->m_nx_max,
                                         cl->minIndex(0),
                                         std::max( 1, cl->maxIndex(0) ) - 1u );
        
        if( cl->indexDim() > 1 ) {
            m_model->updateConstraints<int>( index_range_select_min_j_key,
                                             m_source_item->m_subset_selector_data->m_ny_min,
                                             cl->minIndex(1),
                                             std::max( 1, cl->maxIndex(1) ) - 1u );
            m_model->updateConstraints<int>( index_range_select_max_j_key,
                                             m_source_item->m_subset_selector_data->m_ny_max,
                                             cl->minIndex(1),
                                             std::max( 1, cl->maxIndex(1) ) - 1u );
        }
        else {
            m_model->updateConstraints<int>( index_range_select_min_j_key, 0, 0, 0 );
            m_model->updateConstraints<int>( index_range_select_max_j_key, 0, 0, 0 );
        }
        
        if( cl->indexDim() > 2 ) {
            m_model->updateConstraints<int>( index_range_select_min_k_key,
                                             m_source_item->m_subset_selector_data->m_nz_min,
                                             cl->minIndex(2),
                                             std::max( 1, cl->maxIndex(2) ) - 1u );
            m_model->updateConstraints<int>( index_range_select_max_k_key,
                                             m_source_item->m_subset_selector_data->m_nz_max,
                                             cl->minIndex(2),
                                             std::max( 1, cl->maxIndex(2) ) - 1u );
            
        }
        else {
            m_model->updateConstraints<int>( index_range_select_min_k_key, 0, 0, 0 );
            m_model->updateConstraints<int>( index_range_select_max_k_key, 0, 0, 0 );
        }
    }
    else {
        m_model->updateConstraints<int>( index_range_select_min_i_key, 0, 0, 0 );
        m_model->updateConstraints<int>( index_range_select_max_i_key, 0, 0, 0 );
        m_model->updateConstraints<int>( index_range_select_min_j_key, 0, 0, 0 );
        m_model->updateConstraints<int>( index_range_select_max_j_key, 0, 0, 0 );
        m_model->updateConstraints<int>( index_range_select_min_k_key, 0, 0, 0 );
        m_model->updateConstraints<int>( index_range_select_max_k_key, 0, 0, 0 );
    }
    
    m_model->updateElement<double>( field_select_min_key, m_source_item->m_subset_selector_data->m_field_select_min );
    m_model->updateElement<double>( field_select_max_key, m_source_item->m_subset_selector_data->m_field_select_max );
}

void
SubsetSelector::sourceFieldHasChanged( boost::shared_ptr<SourceItem> source_item )
{
    if( !source_item || !source_item->m_appearance_data ) {
        return;
    }
}


void
SubsetSelector::bumpRevision()
{
}


} // of namespace models

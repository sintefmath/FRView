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

#include "models/RenderConfig.hpp"

namespace {
    const char* shading_model_strings[] = { "Solid",
                                            "Diffuse",
                                            "Diffuse and Specular" };
}


namespace models {
    using std::string;
    using namespace tinia::model::gui;

static const string light_theme_key = "light_theme";
static const string render_options_label_key = "_render_options";
static const string render_clipplane_key = "render_clip_plane";
static const string render_grid_key = "_render_grid";
static const string render_wells_key = "_render_wells";
static const string line_thickness_key = "_line_thickness";

static const string shading_model_key = "shading_model_key";

static const std::string renderconfig_title_key       = "renderconfig_title";
static const std::string render_quality_key             = "render_quality";
static const std::string render_quality_string_key      = "render_quality_string";




RenderConfig::RenderConfig(boost::shared_ptr<tinia::model::ExposedModel>& model)
    : m_model( model ),
      m_revision( 1 ),
      //m_reload( reload ),
      m_render_quality( 2 ),
      m_shading_model( Diffuse ),
      m_render_grid( false ),
      m_render_wells( false ),
      m_line_thickness( 0.5f ),
      m_clip_plane_visible( true )
{
    setDarkTheme();
    m_model->addElement<bool>( renderconfig_title_key, true, "Rendering" );

    m_model->addConstrainedElement<int>( line_thickness_key,
                                         static_cast<int>(50.f*m_line_thickness), 1, 100, "Line thickness" );

    m_model->addElement<bool>( render_grid_key, m_render_grid, "Render grid environemt" );
    m_model->addElement<bool>( render_wells_key, m_render_wells, "Render wells" );

    m_model->addElement<bool>( render_options_label_key, false, "Rendering options" );


    m_model->addConstrainedElement<int>( render_quality_key, m_render_quality, 0, 3, "Rendering quality" );
    m_model->addElement<std::string>( render_quality_string_key, "high", "Details" );

    m_model->addElement<bool>( light_theme_key, false, "Light theme" );
    m_model->addElement<bool>( render_clipplane_key, true, "Render clip plane" );

    
    m_model->addElementWithRestriction<std::string>( shading_model_key,
                                                     shading_model_strings[ m_shading_model ],
                                                    &shading_model_strings[0],
                                                    &shading_model_strings[3] );
    m_model->addAnnotation( shading_model_key, "Shading Model" );
    m_model->addStateListener( shading_model_key, this );

    
    m_model->addStateListener( render_grid_key, this );
    m_model->addStateListener( render_wells_key, this );
    m_model->addStateListener( line_thickness_key, this );
    m_model->addStateListener( render_quality_key, this );
    m_model->addStateListener( light_theme_key, this );
    m_model->addStateListener( render_clipplane_key, this );
}

RenderConfig::~RenderConfig()
{

}
void
RenderConfig::bumpRevision()
{
    m_revision++;
}

const std::string&
RenderConfig::titleKey() const
{
    return renderconfig_title_key;
}

const std::string&
RenderConfig::renderQualityStringKey() const
{
    return render_quality_string_key;
}


void
RenderConfig::setLightTheme()
{
    m_theme = 1;
    m_background_color          = glm::vec4( 1.f, 1.f, 1.f, 0.f );
    m_clip_plane_color          = glm::vec4( 0.3f, 0.3f, 0.0f, 1.f );
}

void
RenderConfig::setDarkTheme()
{
    m_theme = 2;
    m_background_color          = glm::vec4( 0.f, 0.f, 0.f, 0.f );
    m_clip_plane_color          = glm::vec4( 1.0f, 1.0f, 0.2f, 1.f );
}


//RenderConfig::VisibilityMask
//RenderConfig::visibilityMask() const
//{
//    int mask =
//            (m_subset_fill_color.w > 0.f        ? VISIBILITY_MASK_SUBSET : VISIBILITY_MASK_NONE ) |
//            (m_subset_outline_color.w > 0.f     ? VISIBILITY_MASK_SUBSET : VISIBILITY_MASK_NONE ) |
//            (m_boundary_fill_color.w > 0.f      ? VISIBILITY_MASK_BOUNDARY : VISIBILITY_MASK_NONE ) |
//            (m_boundary_outline_color.w > 0.f   ? VISIBILITY_MASK_BOUNDARY : VISIBILITY_MASK_NONE ) |
//            ((m_faults_fill_color.w > 0.f) && (m_subset_fill_color.w < 1.f) ? VISIBILITY_MASK_FAULTS_INSIDE : VISIBILITY_MASK_NONE ) |
//            ((m_faults_outline_color.w > 0.f) && (m_subset_fill_color.w < 1.f) ? VISIBILITY_MASK_FAULTS_INSIDE : VISIBILITY_MASK_NONE ) |
//            (m_faults_fill_color.w > 0.f        ? VISIBILITY_MASK_FAULTS_OUTSIDE : VISIBILITY_MASK_NONE ) |
//            (m_faults_outline_color.w > 0.f     ? VISIBILITY_MASK_FAULTS_OUTSIDE : VISIBILITY_MASK_NONE );

//    return static_cast<VisibilityMask>( mask );
//}


void
RenderConfig::stateElementModified( tinia::model::StateElement * stateElement )
{
    const std::string& key = stateElement->getKey();
    if( key == render_quality_key ) {
        stateElement->getValue( m_render_quality );
        switch( m_render_quality ) {
        case 0:
            m_model->updateElement<std::string>( render_quality_string_key, "crappy" );
            break;
        case 1:
            m_model->updateElement<std::string>( render_quality_string_key, "low" );
            break;
        case 2:
            m_model->updateElement<std::string>( render_quality_string_key, "medium" );
            break;
        case 3:
            m_model->updateElement<std::string>( render_quality_string_key, "high" );
            break;
        }
    }
    else if( key == render_grid_key ) {
        stateElement->getValue( m_render_grid );
    }
    else if( key == render_wells_key ) {
        stateElement->getValue( m_render_wells );
        m_reload = true;
    }
    else if( key == line_thickness_key ) {
        int val;
        stateElement->getValue( val );
        m_line_thickness = 0.02f*val;
    }
    else if( key == light_theme_key ) {
        bool value;
        stateElement->getValue( value );
        if( value ) {
            setLightTheme();
        }
        else {
            setDarkTheme();
        }
    }
    else if( key == render_clipplane_key ) {
        stateElement->getValue( m_clip_plane_visible );
    }
    else if( key == shading_model_key ) {
        std::string value;
        stateElement->getValue( value );
        for( int i=0; i<3; i++ ) {
            if( value == shading_model_strings[i] ) {
                m_shading_model  = (ShadingModel)i;
                break;
            }
        }
    }
    bumpRevision();
}

tinia::model::gui::Element*
RenderConfig::guiFactory() const
{
    VerticalLayout* root = new VerticalLayout;

    // -------------------------------------------------------------------------
    ElementGroup* render_options_group = new ElementGroup( render_options_label_key, true );
    root->addChild( render_options_group );
    VerticalLayout* render_options_layout = new VerticalLayout;
    render_options_group->setChild( render_options_layout );
    render_options_layout->addChild( new tinia::model::gui::CheckBox( light_theme_key ) );
    render_options_layout->addChild( new tinia::model::gui::CheckBox( render_clipplane_key ) );
    render_options_layout->addChild( new tinia::model::gui::CheckBox( render_grid_key ) );
    render_options_layout->addChild( new tinia::model::gui::CheckBox( render_wells_key ) );

    // -------------------------------------------------------------------------
    ElementGroup* rendering_quality_group = new ElementGroup( render_quality_key, true );
    root->addChild( rendering_quality_group );
    Grid* alpha_quality_layout = new Grid(3,2);
    rendering_quality_group->setChild( alpha_quality_layout );
    alpha_quality_layout->setChild( 0, 0, new Label( render_quality_string_key, true ) );
    alpha_quality_layout->setChild( 0, 1, new HorizontalSlider( render_quality_key, true ) );

    alpha_quality_layout->setChild( 1, 0, new Label( shading_model_key ) );
    alpha_quality_layout->setChild( 1, 1, new ComboBox( shading_model_key ) );
    
    // -------------------------------------------------------------------------
    ElementGroup* line_thickness_group = new ElementGroup( line_thickness_key, true );
    root->addChild( line_thickness_group );
    line_thickness_group->setChild( new HorizontalSlider( line_thickness_key, true ) );


    return root;
}


} // of namespace models

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

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <stdexcept>
#include <string>

#include <tinia/model/GUILayout.hpp>
//#include <tinia/model/PolicyLock.hpp>

#include "dataset/CornerpointGrid.hpp"
#include "dataset/FieldDataInterface.hpp"
#include "job/FRViewJob.hpp"
#include "utils/Logger.hpp"
#include "ASyncReader.hpp"
#include "utils/PerfTimer.hpp"
#include "render/mesh/AbstractMeshGPUModel.hpp"
#include "render/mesh/BoundingBoxInterface.hpp"
#include "render/GridField.hpp"
#include "render/ClipPlane.hpp"
#include "render/GridCubeRenderer.hpp"
#include "render/TextRenderer.hpp"
#include "render/wells/Representation.hpp"
#include "render/wells/Renderer.hpp"
#include "render/CoordSysRenderer.hpp"
#include "render/subset/BuilderSelectAll.hpp"
#include "render/subset/BuilderSelectByFieldValue.hpp"
#include "render/subset/BuilderSelectByIndex.hpp"
#include "render/subset/BuilderSelectInsideHalfplane.hpp"
#include "render/subset/BuilderSelectOnPlane.hpp"
#include "render/subset/Representation.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/surface/GridTessSurfBuilder.hpp"
#include "render/rlgen/VoxelGrid.hpp"
#include "render/rlgen/VoxelSurface.hpp"

using std::string;
using std::stringstream;

using boost::shared_ptr;
using boost::dynamic_pointer_cast;

namespace {
    const std::string package = "FRViewJob";
}


FRViewJob::FRViewJob( const std::list<string>& files )
    : tinia::jobcontroller::OpenGLJob(),
      m_current_item( ~0 ),
      m_source_selector( m_model, *this ),
      m_subset_selector( m_model, *this ),
      m_appearance( m_model, *this ),
      m_under_the_hood( m_model, *this ),
      m_renderconfig( m_model, *this ),
      m_theme( 0 ),
      m_grid_stats( m_model, *this ),
      m_has_context( false ),
      m_async_reader( new ASyncReader( m_model ) ),
      m_check_async_reader( false ),
      m_enable_gl_debug( false ),
      m_renderlist_initialized( false ),
      m_renderlist_update_revision( true ),
      m_has_pipeline( false ),
      m_query_primitives( false ),
      m_numprimitives( 0 ),
      m_show_wells( m_renderconfig.renderWells() )
{
    
    // Triggers release of all sources.
    
    
    m_render_clip_plane = false;
    m_care_about_updates = true;

    m_create_nonindexed_geometry = false;

    
    float identity[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };


    // fetch data that is needed to populate the policy
    tinia::model::Viewer viewer;
    viewer.height = 640;
    viewer.width = 360;
    for(unsigned int i=0; i<16; i++) {
        viewer.modelviewMatrix[i] = identity[i];
        viewer.projectionMatrix[i] = identity[i];
    }
    std::list<string> solutions = { "[none]" };

    using namespace tinia::model::gui;

    /* 
     * +- root ----------------------------------------------------------------+
     * | +-- left_right_wrapper ---------------------------------------------+ |
     * | | +- main ---------------------------+ +- source -----------------+ | |
     * | | | +- tab_buttons ----------------+ | |
     * | | | |                              | | |
     * | | | +------------------------------+ | |
     * | | | +- canvas ---------------------+ | |
     * | | | |                              | | |
     * | | | |                              | | |
     * | | | +------------------------------+ | |
     * | | +----------------------------------+ +--------------------------+ | |
     * | +-------------------------------------------------------------------+ |
     * | +-- asyncreader_working --------------------------------------------+ |
     * | |                                                                   | |
     * | +-------------------------------------------------------------------+ |
     * + ----------------------------------------------------------------------+
     */
    
    VerticalLayout* root = new VerticalLayout;
    HorizontalLayout* left_right_wrapper = new HorizontalLayout;
    VerticalLayout* main  = new VerticalLayout;
    main->setPadContents( false );

    m_model->addElement<string>( "source_element_group", "n/a", "Options" );
    m_model->addElement<string>( "experimental_element_group", "n/a", "Experimental" );


    root->addChild( left_right_wrapper );


    // --- main ----------------------------------------------------------------
    {
        // --- Canvas --------------------------------------------------------------
        {   m_model->addElement("viewer", viewer );
            m_model->addElement<string>( "boundingbox", "-0.1 -0.1 -0.1 1.1 1.1 1.1" );
            m_model->addElement<int>( "renderlist", 0, "RL timestamp" );
            Canvas* canvas = new Canvas("viewer", "renderlist", "boundingbox" );
            canvas->boundingBoxKey( "boundingbox" );
            //canvas->setViewerType( std::string( "MouseClickResponder" ) );

            main->addChild( canvas );
        }

        left_right_wrapper->addChild( main );
    }

    // --- Async reader info bar -----------------------------------------------
    {
        ElementGroup* preprocess_group = new ElementGroup( "asyncreader_working", true );
        preprocess_group->setVisibilityKey( "asyncreader_working" );

        HorizontalLayout* progress_layout = new HorizontalLayout;
        progress_layout->addChild( new Label( "asyncreader_what", true) );
        //progress_layout->addChild( new Label( "asyncreader_progress", true ) );
        progress_layout->addChild( new HorizontalExpandingSpace );
        preprocess_group->setChild( progress_layout );
        root->addChild( preprocess_group );
    }




    m_model->addElement<bool>( "project_tab", true, "Project" );
    m_model->addElement<bool>( "has_field", false );
    m_model->addElement<bool>( "field_select_report_step_override", false, "Specific report step" );
    m_model->addConstrainedElement<int>( "field_select_report_step", 0, 0, 0, "Field select report step" );
    m_model->addAnnotation( "field_select_solution_override", "Specific solution");
    m_model->addElement<bool>( "field_range_label", true, "Range" );
    m_model->addElement<bool>( "details_label", true, "Details" );
    m_model->addStateListener( "asyncreader_ticket", this);

    TabLayout* outer_tabs = new TabLayout;
    left_right_wrapper->addChild( outer_tabs );

        m_model->addElement<bool>( "Source", true );

    // --- options tab ----------------------------------------------------------
    {   


        Tab* source_tab = new Tab( "source_element_group" );

        VerticalLayout* options = new VerticalLayout;
        source_tab->setChild( options );
        outer_tabs->addChild( source_tab );



        m_model->addElement<bool>( "options_label_key", true, "Options" );
        ElementGroup* optionsRoot = new ElementGroup( "options_label_key", true );
        Grid* optionsGrid = new Grid( 2, 2 );
        optionsGrid->setChild( 0, 0, new Label( m_grid_stats.zScaleKey() ) );
        optionsGrid->setChild( 0, 1, new DoubleSpinBox( m_grid_stats.zScaleKey() ) );
        optionsGrid->setChild( 1, 0, new CheckBox( m_renderconfig.lightThemeKey() ) );
        optionsGrid->setChild( 1, 1, new CheckBox( m_renderconfig.renderWellsKey() ) );
        optionsRoot->setChild(optionsGrid);
        options->addChild( optionsRoot );

        //options->addChild( m_source_selector.guiFactory() );

        m_model->addElement<bool>( "field_info_enable", true, "Source and Field" );


        ElementGroup* field_details_group = new ElementGroup( "field_info_enable", true );

        Grid* field_details_grid = new Grid( 8, 4 );
        field_details_grid->setChild( 0, 1, new HorizontalSpace );
        field_details_grid->setChild( 0, 3, new HorizontalExpandingSpace );

        field_details_grid->setChild( 1, 0, new Button(m_source_selector.getFileLoadKey()) );
        field_details_grid->setChild( 1, 1, new TextInput( m_source_selector.getFileNameKey()) );

        field_details_grid->setChild( 2, 0, new Label("Source") );
        field_details_grid->setChild( 2, 1, new Button(m_source_selector.getCloneKey()) );
        field_details_grid->setChild( 2, 2, new ComboBox(m_source_selector.getSourceSelectorKey() ));
        field_details_grid->setChild( 2, 3, new Button(m_source_selector.getDeleteKey()) );

        m_model->addElementWithRestriction<string>( "field_solution",
                                                    solutions.front(),
                                                    solutions.begin(),
                                                    solutions.end() );
        m_model->addAnnotation( "field_solution", "Solution" );
        m_model->addStateListener( "field_solution", this);
        field_details_grid->setChild( 3, 0, new Label( "field_solution" ) );
        field_details_grid->setChild( 3, 2, new ComboBox( "field_solution" ) );

        m_model->addConstrainedElement<int>("field_report_step", 0, 0, 0, "Report step" );
        m_model->addStateListener( "field_report_step", this);
        field_details_grid->setChild( 4, 0, new Label( "field_report_step" ) );
        field_details_grid->setChild( 4, 2, new HorizontalSlider( "field_report_step" ) );

        m_model->addElement<string>( "field_info_calendar", "n/a", "Date" );
        field_details_grid->setChild( 5, 0, new Label( "field_info_calendar" ));
        field_details_grid->setChild( 5, 2, (new Label( "field_info_calendar", true ))->setEnabledKey( "has_field") );

        m_model->addElement<string>( "field_info_range", "n/a", "Range" );
        field_details_grid->setChild( 6, 0, new Label( "field_info_range" ) );
        field_details_grid->setChild( 6, 2, (new Label( "field_info_range", true ))->setEnabledKey( "has_field" ) );

        field_details_grid->setChild( 7, 0, new CheckBox( m_appearance.flipOrientationKey() ) );

        field_details_group->setChild( field_details_grid );

        options->addChild( field_details_group );

        options->addChild( m_subset_selector.guiFactory() );
        options->addChild( m_appearance.guiFactory() );

        options->addChild( new VerticalExpandingSpace );
    }

    // --- experimental tab -==---------------------------------------------------
    {   Tab* experimental_tab = new Tab( "experimental_element_group" );
        outer_tabs->addChild( experimental_tab );
        VerticalLayout* experimental = new VerticalLayout;

        m_model->addElement<bool>( "details_label_key", true, "Details" );
        m_model->addElement<bool>( "primitives_key", m_query_primitives, "# Primitives");
        m_model->addStateListener( "primitives_key", this);
        m_model->addElement<int>( "num_prim_key", m_numprimitives, "Number Primitives" );
        ElementGroup* detailsRoot = new ElementGroup( "details_label_key", true );
        Grid* detailsStats = m_grid_stats.guiFactory();
        detailsStats->setChild( 6, 0, new CheckBox( "primitives_key") );
        detailsStats->setChild( 6, 2, new Label( "num_prim_key", true) );
        detailsRoot->setChild(detailsStats);
        experimental->addChild( detailsRoot );
        experimental->addChild( m_renderconfig.guiFactory() );
        experimental_tab->setChild(experimental);
    }

    //// --- under the hood tab --------------------------------------------------
    //{  Tab* uth_tab = new Tab( m_under_the_hood.titleKey() );
    //    outer_tabs->addChild( uth_tab );
    //    uth_tab->setChild( m_under_the_hood.guiFactory() );
    //}


    std::vector<tinia::model::StateSchemaElement> elements;
    m_model->getFullStateSchema(elements);

    m_model->setGUILayout( root, DESKTOP);

    for(auto it=files.begin(); it!=files.end(); ++it ) {
        if( (*it) == "--gl-debug-log" ) {
            m_enable_gl_debug = true;
        }
        
        if( (*it).find('.') != std::string::npos ) {
            loadFile( *it );
            //break;
        }
    }

}


bool
FRViewJob::init()
{
    return true;
}

FRViewJob::~FRViewJob()
{
   m_model->removeStateListener(this);
}



void
FRViewJob::updateCurrentFieldData()
{
    bool has_field = false;
    if( m_current_item < m_source_items.size() ) {
        boost::shared_ptr<SourceItem> si = m_source_items[ m_current_item ];
        
        if( si->m_grid_field ) {
            has_field = true;
            
            if( si->m_grid_tess_subset ) {
                
                boost::shared_ptr<dataset::FieldDataInterface> fielddata =
                        boost::dynamic_pointer_cast<dataset::FieldDataInterface>( si->m_source );
                
                if( fielddata ) {
                    has_field = true;
                    std::stringstream o;
                    o << "[ " << si->m_grid_field->minValue()
                      << ", " << si->m_grid_field->maxValue() << " ]";
                    m_model->updateElement( "field_info_range", o.str() );
                    o.str("");
                    o << "[not implemented]";
                    m_model->updateElement( "field_info_calendar", fielddata->timestepDescription( si->m_timestep_current ) );
                }
                
                
            }
        }
    }
    else {
        
    }
    m_model->updateElement( "has_field", has_field );
    return;
    if( !has_field ) {
        m_model->updateElement( "field_info_range", "[not available]" );
        m_model->updateElement( "field_info_calendar", "[not available]" );
    }
}


void
FRViewJob::loadFile( const std::string& filename )
{
    m_source_selector.file().setFileName( filename );

    m_async_reader->issueOpenSource( filename,
                                     m_source_selector.file().refineI(),
                                     m_source_selector.file().refineJ(),
                                     m_source_selector.file().refineK(),
                                     m_source_selector.file().triangulate() );
}

void
FRViewJob::stateElementModified( tinia::model::StateElement *stateElement )
{

    if( !m_care_about_updates ) {
        return;
    }
    m_care_about_updates = false;

    Logger log = getLogger( "CPViewJob.stateElementModified" );


    const string& key = stateElement->getKey();




    if( key == "asyncreader_ticket" ) { // Async job is finished
        m_check_async_reader = true;
    }
    
    else if( currentSourceItemValid() && (key == "field_solution") ) {
        string value;
        stateElement->getValue( value );

        shared_ptr<SourceItem> si = currentSourceItem();
        int field = -1;
        for( int i=0; i<(int)si->m_field_names.size(); i++ ) {
            if( si->m_field_names[ i ] == value ) {
                field = i;
                break;
            }
        }
        if( (field != -1) && (si->m_field_current != field) ) {
            si->m_field_current = field;
            issueFieldFetch();
        }
    }
    else if( currentSourceItemValid() && (key == "field_report_step") ) {
        int value;
        stateElement->getValue( value );
        shared_ptr<SourceItem> si = currentSourceItem();
        if( si->m_timestep_current != value ) {
            si->m_timestep_current = value;
            issueFieldFetch();
        }
    }

    if (stateElement->getKey() == "primitives_key") {
        stateElement->getValue(m_query_primitives);
    }
    
    

    m_care_about_updates = true;
    doLogic();
}

void
FRViewJob::triggerRedraw( const string& viewer_key )
{
    tinia::model::Viewer viewer;
    m_model->getElementValue( viewer_key, viewer );
    m_model->updateElement( viewer_key, viewer );
}

void
FRViewJob::updateModelMatrices()
{
    using boost::shared_ptr;
    using boost::dynamic_pointer_cast;
    using render::mesh::BoundingBoxInterface;
    using dataset::ZScaleInterface;
    
    
    glm::vec3 bbmin( 0.f );
    glm::vec3 bbmax( 1.f );
    float scale_z  = 0.f;

    bool first = true;
    for(size_t i=0; i<m_source_items.size(); i++ ) {
        boost::shared_ptr<SourceItem> source_item = m_source_items[i];
        shared_ptr<const BoundingBoxInterface> bbox = dynamic_pointer_cast<const BoundingBoxInterface>( source_item->m_grid_tess );
        if( bbox ) {
            glm::vec3 bbmin_( bbox->minBBox()[0], bbox->minBBox()[1], bbox->minBBox()[2] );
            glm::vec3 bbmax_( bbox->maxBBox()[0], bbox->maxBBox()[1], bbox->maxBBox()[2] );
            if( first ) {
                bbmin = bbmin_;
                bbmax = bbmax_;
                first = false;
            }
            else {
                bbmin = glm::min( bbmin, bbmin_ );
                bbmax = glm::max( bbmax, bbmax_ );
            }
        }
        
        shared_ptr<ZScaleInterface> zscale_src = dynamic_pointer_cast<ZScaleInterface>( source_item->m_source );
        if( zscale_src ) {
            scale_z = glm::max( scale_z, zscale_src->cornerPointZScale()/zscale_src->cornerPointXYScale() );
        }
    }
    if( !first ) {
        
        glm::vec3 shift = -0.5f*( bbmin + bbmax );
        float scale_xy = std::max( (bbmax.x-bbmin.x),
                                   (bbmax.y-bbmin.y) );
        
        if( scale_z < std::numeric_limits<float>::epsilon() ) {
            scale_z = 10.f;
        }
        scale_z = m_grid_stats.zScale()*scale_z;
        
        m_local_to_world = glm::mat4();
        m_local_to_world = glm::translate( m_local_to_world, glm::vec3(0.5f ) );
        m_local_to_world = glm::scale( m_local_to_world, glm::vec3( 1.f/scale_xy, 1.f/scale_xy, scale_z/scale_xy) );
        m_local_to_world = glm::translate( m_local_to_world, shift );
        
        m_local_from_world = glm::mat4();
        m_local_from_world = glm::translate( m_local_from_world, -shift );
        m_local_from_world = glm::scale( m_local_from_world, glm::vec3( scale_xy, scale_xy, scale_xy/scale_z ) );
        m_local_from_world = glm::translate( m_local_from_world, -glm::vec3(0.5f ) );
        
        m_bbox_to_world = glm::translate( glm::mat4(), glm::vec3( -0.1f ) );
        m_bbox_to_world = glm::scale( m_bbox_to_world, glm::vec3( 1.2f ) );
        m_bbox_from_world = glm::inverse( m_bbox_to_world );
    }
    else {
        m_local_to_world = glm::mat4();
        m_local_from_world = glm::mat4();
        m_bbox_to_world = glm::mat4();
        m_bbox_from_world = glm::mat4();
    }
}

void
FRViewJob::updateProxyMatrices()
{
    if( m_has_pipeline ) {
        const float* t = m_proxy_transform;
        m_proxy_to_world =
                m_local_to_world *
                glm::mat4( t[0],  t[1],  t[2],  t[3],
                           t[4],  t[5],  t[6],  t[7],
                           t[8],  t[9],  t[10], t[11],
                           t[12], t[13], t[14], t[15] );
        m_proxy_to_world = glm::translate( m_proxy_to_world, glm::vec3( m_proxy_box_min[0],
                                           m_proxy_box_min[1],
                                           m_proxy_box_min[2] ) );
        m_proxy_to_world = glm::scale( m_proxy_to_world, glm::vec3( m_proxy_box_max[0]-m_proxy_box_min[0],
                                       m_proxy_box_max[1]-m_proxy_box_min[1],
                                       m_proxy_box_max[2]-m_proxy_box_min[2] ) );
        m_proxy_from_world = glm::inverse( m_proxy_to_world );
    }
    else {
        m_proxy_to_world = glm::mat4();
        m_proxy_from_world = glm::mat4();
    }
}

#ifndef APIENTRY
#define APIENTRY
#endif

static void APIENTRY debugLogger( GLenum source,
                                  GLenum type,
                                  GLuint /*id*/,
                                  GLenum severity,
                                  GLsizei /*length*/,
                                  const GLchar* message,
#ifdef GL_VERSION_4_5
                                  const GLvoid* /*data*/
#else
                                  void* /*data*/
#endif
                                )
{
    if( strncmp( "Texture state usage warning", message, 27 ) == 0 ) {
        return;
    }

    const char* source_str = "---";
    switch( source ) {
    case GL_DEBUG_SOURCE_API: source_str = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: source_str = "WSY"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "SCM"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: source_str = "3PY"; break;
    case GL_DEBUG_SOURCE_APPLICATION: source_str = "APP"; break;
    case GL_DEBUG_SOURCE_OTHER: source_str = "OTH"; break;
    }

    const char* type_str = "---";
    switch( type ) {
    case GL_DEBUG_TYPE_ERROR: type_str = "error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "deprecated"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_str = "undef"; break;
    case GL_DEBUG_TYPE_PORTABILITY: type_str = "portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE: type_str = "performance"; break;
    case GL_DEBUG_TYPE_OTHER: type_str = "other"; break;
    }

    Logger log = getLogger( std::string("GL_KHR_debug.") + source_str + "." + type_str );
    // const char* severity_str = "---";
    switch( severity ) {
    case GL_DEBUG_SEVERITY_HIGH:
        LOGGER_FATAL( log, message );
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        LOGGER_ERROR( log, message );
        break;
    case GL_DEBUG_SEVERITY_LOW:
        LOGGER_DEBUG( log, message );
        break;
    }
}

bool
FRViewJob::initGL()
{
    Logger log = getLogger( "CPViewJob.initGL" );
    glewInit();

    if( m_enable_gl_debug && glewIsSupported( "GL_KHR_debug" ) ) {
        LOGGER_DEBUG( log, "Enabling OpenGL debug logging" );
        glEnable( GL_DEBUG_OUTPUT );
        glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
        glDebugMessageCallback( debugLogger, NULL );
        glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE );
        glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_FALSE );

    }
    
    setupPipeline();
    m_has_context = true;
    return true;
}

void
FRViewJob::releasePipeline()
{
    if( !m_has_context ) {
        return;
    }
    m_grid_tess_surf_builder = boost::shared_ptr<render::surface::GridTessSurfBuilder>();
    m_all_selector = boost::shared_ptr<render::subset::BuilderSelectAll>();
    m_field_selector = boost::shared_ptr<render::subset::BuilderSelectByFieldValue>();
    m_index_selector = boost::shared_ptr<render::subset::BuilderSelectByIndex>();
    m_plane_selector = boost::shared_ptr<render::subset::BuilderSelectOnPlane>();
    m_half_plane_selector = boost::shared_ptr<render::subset::BuilderSelectInsideHalfplane>();
    m_grid_cube_renderer = boost::shared_ptr<render::GridCubeRenderer>();
    m_coordsys_renderer = boost::shared_ptr<render::CoordSysRenderer>();
    m_voxel_grid = boost::shared_ptr<render::rlgen::GridVoxelization>();
    m_voxel_surface = boost::shared_ptr<render::rlgen::VoxelSurface>();
    m_color_maps.reset();
    m_has_pipeline = false;
}

bool FRViewJob::setupPipeline()
{
    if( !m_has_context ) {
        return false;
    }
    try {
        m_grid_tess_surf_builder.reset( new render::surface::GridTessSurfBuilder );
        m_all_selector.reset( new render::subset::BuilderSelectAll );
        m_field_selector.reset( new render::subset::BuilderSelectByFieldValue );
        m_index_selector.reset( new render::subset::BuilderSelectByIndex );
        m_plane_selector.reset( new render::subset::BuilderSelectOnPlane );
        m_half_plane_selector.reset( new render::subset::BuilderSelectInsideHalfplane );
        m_grid_cube_renderer.reset( new render::GridCubeRenderer );
        m_coordsys_renderer.reset( new render::CoordSysRenderer );
        m_voxel_surface.reset( new render::rlgen::VoxelSurface );

        // --- create color map texture --------------------------------------------
        m_color_maps.reset( new render::GLTexture() );
        std::vector<glm::vec3> ramp( 256 );
        for(size_t i=0; i<ramp.size(); i++ ) {
            float x = static_cast<float>(i)/static_cast<float>(ramp.size()-1);
            ramp[i].x = (x < 0.50f ? 0.f   : (x < 0.75f ? 4.f*(x-0.5f)        : 1.f                 ));
            ramp[i].y = (x < 0.25f ? 4.f*x : (x < 0.75f ? 1.f                 : 1.f - 4.f*(x-0.75f) ));
            ramp[i].z = (x < 0.25f ? 1.f   : (x < 0.50f ? 1.f - 4.f*(x-0.25f) : 0.f                 ));
        }
        glBindTexture( GL_TEXTURE_1D, m_color_maps->get() );
        glTexImage1D( GL_TEXTURE_1D, 0, GL_RGB, 256, 0, GL_RGB, GL_FLOAT, ramp.data() );
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0 );
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, 0 );
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture( GL_TEXTURE_1D, 0 );
        
        m_has_pipeline = true;
    }
    catch( std::runtime_error& e ) {
        Logger log = getLogger( "CPViewJob.setupPipeline" );
        LOGGER_ERROR( log, "Failed to set up pipeline: " << e.what() );
        releasePipeline();
    }

// ------------ for counting # primitives generated -------------- //
// ----- happens in Render.cpp ----------------------------------- //
    glGenQueries(1, &m_primitives);

    return m_has_pipeline;
}



void
FRViewJob::doLogic()
{
    
    if( m_create_nonindexed_geometry != m_renderconfig.createNonindexedSurfaces() ) {
        m_create_nonindexed_geometry = m_renderconfig.createNonindexedSurfaces();

        for( size_t i=0; i<m_source_items.size(); i++ ) {
            SourceItem& si = *m_source_items[i];
            si.m_faults_surface.reset();
            si.m_faults_surface_soup.reset();
            si.m_subset_surface.reset();
            si.m_subset_surface_soup.reset();
            si.m_boundary_surface.reset();
            si.m_boundary_surface_soup.reset();
            si.m_do_update_subset = true;
        }
    }
        

    
    if( m_zscale != m_grid_stats.zScale() ) {
        m_zscale = m_grid_stats.zScale();
        
        // If the z-scale changes and we use a clip-plane, the subsets will
        // change (since the model-clipplane relation changes).
        if( m_render_clip_plane ) {
            for( size_t i=0; i<m_source_items.size(); i++ ) {
                m_source_items[i]->m_do_update_subset = true;
                m_source_items[i]->m_do_update_renderlist = true;
            }
            m_renderlist_update_revision = true;
        }
    }
    
    if( m_voxel_grid && !m_voxel_grid->hasDimension( m_renderconfig.proxyResolution() ) ) {
        for( size_t i=0; i<m_source_items.size(); i++ ) {
            m_source_items[i]->m_do_update_renderlist = true;
        }
        m_renderlist_update_revision = true;
    }
    
    for( size_t i=0; i<m_source_items.size(); i++ ) {
        SourceItem& si = *m_source_items[i];
        if( si.m_appearance_data == NULL ) {
            continue;
        }

        models::AppearanceData::VisibilityMask new_mask = si.m_appearance_data->visibilityMask();
        if( ( (si.m_visibility_mask != new_mask) || m_under_the_hood.profilingEnabled() ) ) {
            si.m_visibility_mask = new_mask;
            si.m_do_update_subset = true;
            si.m_do_update_renderlist = true;
            m_renderlist_update_revision = true;
        }
    }


    // If subset changes, so do render lists
    for( size_t i=0; i<m_source_items.size(); i++ ) {
        if( m_source_items[i]->m_do_update_subset ) {
            m_renderlist_update_revision = true;
        }
    }
    
    
    // If we clients have asked for renderlists at least once, we inform them
    // that they should query for a new one. 
    if( /*m_renderlist_initialized &&*/  m_renderlist_update_revision ) {
        m_renderlist_update_revision = false;

        //int val;
        //m_model->getElementValue( "renderlist", val );
        //m_model->updateElement( "renderlist", val+1 );
        m_model->updateElement<int>( "renderlist", m_renderlist_db.bump() );
    }

    if( m_show_wells != m_renderconfig.renderWells() ) {
        m_show_wells = m_renderconfig.renderWells();
        issueFieldFetch();
    }

}

bool
FRViewJob::renderFrame( const string&  session,
                        const string&  key,
                        unsigned int        fbo,
                        const size_t        width,
                        const size_t        height )
{
    Logger log = getLogger( "CPViewJob.renderFrame" );
    if( !m_has_context ) {
        LOGGER_ERROR( log, "renderFrame invoked before initGL" );
        return false;
    }
    doLogic();
    m_under_the_hood.update();

    fetchData();
    updateModelMatrices();
    doCompute();

    tinia::model::Viewer viewer;
    m_model->getElementValue( "viewer", viewer );
    render( viewer.projectionMatrix.data(),
            viewer.modelviewMatrix.data(),
            fbo,
            width,
            height );

    if (m_query_primitives) {
        m_model->updateElement( "num_prim_key", (int)m_numprimitives);
    }


    return true;
}


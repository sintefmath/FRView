/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <stdexcept>
#include <string>

#include <tinia/model/GUILayout.hpp>
//#include <tinia/model/PolicyLock.hpp>

#include "dataset/Project.hpp"
#include "job/FRViewJob.hpp"
#include "utils/Logger.hpp"
#include "ASyncReader.hpp"
#include "utils/PerfTimer.hpp"
#include "render/GridTess.hpp"
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
#include "render/rlgen/GridVoxelization.hpp"
#include "render/rlgen/VoxelSurface.hpp"

using std::string;
using std::stringstream;


FRViewJob::FRViewJob( const std::list<string>& files )
    : tinia::jobcontroller::OpenGLJob(),
      m_file( m_model, *this ),
      m_under_the_hood( m_model, *this ),
      m_appearance( m_model, m_load_color_field ),
      m_visibility_mask( models::Appearance::VISIBILITY_MASK_NONE ),
      m_theme( 0 ),
      m_grid_stats( m_model, *this ),
      m_has_context( false ),
      m_async_reader( new ASyncReader( m_model ) ),
      m_renderlist_initialized( false ),
      m_renderlist_state( RENDERLIST_SENT ),
      m_has_pipeline( false ),
      m_load_geometry( false )
{

    m_load_color_field = false;
    m_has_color_field = false;
    m_render_clip_plane = false;
    m_care_about_updates = true;
    m_report_step_index = 0;
    m_solution_index = 0;


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

   const char* colormap_types[] = { "Linear", "Logarithmic" };

    const char* subsets[] = { "all",
                              "subset_field",
                              "subset_index",
                              "subset_plane",
                              "subset_halfplane" };



    const double dmax = std::numeric_limits<double>::max();
    using namespace tinia::model::gui;

    VerticalLayout* root = new VerticalLayout;
    HorizontalLayout* tab_buttons = new HorizontalLayout;
    //tab_buttons->setEnabledKey( "asyncreader_working", true );

    root->addChild( tab_buttons );

    // File dialogue
    PopupButton* file_popup = new PopupButton( m_file.titleKey(), false );
    file_popup->setChild( m_file.guiFactory() );
    tab_buttons->addChild( file_popup );

    // Appearance
    PopupButton* appearance_popup = new PopupButton( m_appearance.titleKey(), false );
    appearance_popup->setChild( m_appearance.guiFactory() );
    tab_buttons->addChild( appearance_popup );

    // Profile dialogue
    PopupButton* under_the_hood_popup = new PopupButton( m_under_the_hood.titleKey(), false );
    under_the_hood_popup->setChild( m_under_the_hood.guiFactory() );
    tab_buttons->addChild( under_the_hood_popup );
    tab_buttons->addChild( new HorizontalExpandingSpace );

    HorizontalLayout* app_pane = new HorizontalLayout;
    root->addChild( app_pane );
    m_model->addElement<bool>("has_project", false );
    //app_pane->setEnabledKey( "has_project" );

    VerticalLayout* right_master = new VerticalLayout;
    VerticalLayout* right_column = new VerticalLayout;


    // Preprocess dialogue
    if( 1 ) {
        ElementGroup* preprocess_group = new ElementGroup( "asyncreader_working", true );
        preprocess_group->setVisibilityKey( "asyncreader_working" );

        HorizontalLayout* progress_layout = new HorizontalLayout;
        progress_layout->addChild( new Label( "asyncreader_what", true) );
        //progress_layout->addChild( new tinia::model::gui::Label( "asyncreader_progress", true ) );
        progress_layout->addChild( new HorizontalExpandingSpace );
        preprocess_group->setChild( progress_layout );
        root->addChild( preprocess_group );
    }



    // Viewer
    m_model->addElement("viewer", viewer );
    m_model->addElement<string>( "boundingbox", "-0.1 -0.1 -0.1 1.1 1.1 1.1" );
    m_model->addElement<int>( "renderlist", 0 );
    Canvas* canvas = new Canvas("viewer", "renderlist", "boundingbox" );
    canvas->boundingBoxKey( "boundingbox" );
    canvas->setViewerType( std::string( "MouseClickResponder" ) );
    app_pane->addChild( canvas );
    app_pane->addChild( right_master );
    right_master->addChild( right_column );



    m_model->addElement<bool>( "field_info_enable", true, "Field" );
    m_model->addElement<string>( "field_info_calendar", "n/a", "Date" );
    m_model->addElement<string>( "field_info_range", "n/a", "Range" );
    m_model->addElement<bool>( "project_tab", true, "Project" );
    m_model->addElement<bool>( "transparency_label", true, "Transparency" );
    m_model->addElement<bool>( "surface_tab", true, "Surfaces" );
    m_model->addElement<bool>( "well_tab", true, "Wells" );
    m_model->addElement<bool>("field_group_enable", true, "Field" );
    m_model->addConstrainedElement<int>("field_report_step", 0,0, 0, "Report step" );
    m_model->addElementWithRestriction<string>("field_solution",
                                                        solutions.front(),
                                                        solutions.begin(),
                                                        solutions.end() );
    m_model->addAnnotation( "field_solution", "Solution" );
    m_model->addElement<bool>( "has_field", false );

    m_model->addElement<bool>( "colormap_label", true, "Color map" );
    m_model->addElementWithRestriction<string>( "colormap_type",
                                                    colormap_types[0],
                                                    &colormap_types[0],
                                                    &colormap_types[2] );
    m_model->addAnnotation( "colormap_type", "Type" );
    m_model->addElement<bool>( "field_range_enable", false, "Lock min and max" );
    m_model->addConstrainedElement<double>( "field_range_min", 0.0, -dmax, dmax, "Min" );
    m_model->addConstrainedElement<double>( "field_range_max", 0.0, -dmax, dmax, "Max" );
    m_model->addElement<bool>("grid_subset", true, "Subset" );
    m_model->addElementWithRestriction<string>( "surface_subset",
                                                    subsets[0],
                                                    &subsets[0],
                                                    &subsets[5] );

    m_model->addElement<bool>( "surface_subset_field_range", false );
    m_model->addElement<bool>( "surface_subset_index_range", false );
    m_model->addElement<bool>( "surface_subset_plane", false );
    m_model->addElement<bool>( "tessellation_label", true, "Tessellation" );
    m_model->addElement<bool>( "tess_flip_orientation", false, "Flip orientation" );
    m_model->addElement<bool>( "plane_select_PY", false, "PY" );
    m_model->addElement<bool>( "plane_select_NY", false, "NY" );
    m_model->addElement<bool>( "plane_select_PX", false, "PX" );
    m_model->addElement<bool>( "plane_select_NX", false, "NX" );
    m_model->addConstrainedElement( "plane_select_shift", 0, -500, 500, "Shift" );
    m_model->addConstrainedElement<int>( "index_range_select_min_i", 0, 0, 0, "I" );
    m_model->addConstrainedElement<int>( "index_range_select_max_i", 0, 0, 0, "I" );
    m_model->addConstrainedElement<int>( "index_range_select_min_j", 0, 0, 0, "J" );
    m_model->addConstrainedElement<int>( "index_range_select_max_j", 0, 0, 0, "J" );
    m_model->addConstrainedElement<int>( "index_range_select_min_k", 0, 0, 0, "k" );
    m_model->addConstrainedElement<int>( "index_range_select_max_k", 0, 0, 0, "K" );
    m_model->addConstrainedElement<double>( "field_select_min", 0.0, -dmax, dmax, "Min" );
    m_model->addConstrainedElement<double>( "field_select_max", 0.0, -dmax, dmax, "Max" );
    m_model->addElement<bool>( "field_select_report_step_override", false, "Specific report step" );
    m_model->addConstrainedElement<int>( "field_select_report_step", 0, 0, 0, "Field select report step" );
    m_model->addElement<bool>( "field_select_solution_override", false );
    m_model->addElementWithRestriction<string>( "field_select_solution",
                                                solutions.front(),
                                                solutions.begin(),
                                                solutions.end() );
    m_model->addAnnotation( "field_select_solution_override", "Specific solution");
    m_model->addElement<bool>( "field_range_label", true, "Range" );
    m_model->addElement<bool>( "color_label", true, "Color" );
    m_model->addElement<bool>( "details_label", true, "Details" );


    m_model->addStateListener( "asyncreader_ticket", this);
    m_model->addStateListener( "field_solution", this);
    m_model->addStateListener( "field_report_step", this);
    m_model->addStateListener( "plane_select_PX", this);
    m_model->addStateListener( "plane_select_NX", this);
    m_model->addStateListener( "plane_select_PY", this);
    m_model->addStateListener( "plane_select_NY", this);
    m_model->addStateListener( "plane_select_shift", this);
    m_model->addStateListener( "index_range_select_min_i", this);
    m_model->addStateListener( "index_range_select_max_i", this);
    m_model->addStateListener( "index_range_select_min_j", this);
    m_model->addStateListener( "index_range_select_max_j", this);
    m_model->addStateListener( "index_range_select_min_k", this);
    m_model->addStateListener( "index_range_select_max_k", this);
    m_model->addStateListener( "field_select_min", this);
    m_model->addStateListener( "field_select_max", this);
    m_model->addStateListener( "surface_subset", this);
    m_model->addStateListener( "field_range_enable", this);
    m_model->addStateListener( "tess_flip_orientation", this);

    // --- info
    right_column->addChild( m_grid_stats.guiFactory() );


    tinia::model::gui::ElementGroup* field_details_group = new tinia::model::gui::ElementGroup( "field_info_enable", true );
    tinia::model::gui::Grid* field_details_grid = new tinia::model::gui::Grid( 4, 4 );
    field_details_grid->setChild( 0, 1, new tinia::model::gui::HorizontalSpace );
    field_details_grid->setChild( 0, 3, new tinia::model::gui::HorizontalExpandingSpace );
    field_details_grid->setChild( 0, 0, new tinia::model::gui::Label( "field_solution" ) );
    field_details_grid->setChild( 0, 2, new tinia::model::gui::ComboBox( "field_solution" ) );
    field_details_grid->setChild( 1, 0, new tinia::model::gui::Label( "field_report_step" ) );
    field_details_grid->setChild( 1, 2, new tinia::model::gui::HorizontalSlider( "field_report_step" ) );
    field_details_grid->setChild( 2, 0, new tinia::model::gui::Label( "field_info_calendar" ));
    field_details_grid->setChild( 2, 2, (new tinia::model::gui::Label( "field_info_calendar", true ))->setEnabledKey( "has_field") );
    field_details_grid->setChild( 3, 0, new tinia::model::gui::Label( "field_info_range" ) );
    field_details_grid->setChild( 3, 2, (new tinia::model::gui::Label( "field_info_range", true ))->setEnabledKey( "has_field" ) );
    field_details_group->setChild( field_details_grid );
    right_column->addChild( field_details_group );


    // -- surface

    // subset field range
    Grid* subset_field_minmax_layout = new Grid( 3, 2 );
    subset_field_minmax_layout->setChild( 0, 0, new Label("field_select_min") );
    subset_field_minmax_layout->setChild( 0, 1, new DoubleSpinBox("field_select_min") );
    subset_field_minmax_layout->setChild( 1, 0, new Label("field_select_max") );
    subset_field_minmax_layout->setChild( 1, 1, new DoubleSpinBox("field_select_max") );
    subset_field_minmax_layout->setChild( 2, 1, new HorizontalExpandingSpace );
    VerticalLayout* subset_field_layout = new VerticalLayout;
    subset_field_layout->addChild( subset_field_minmax_layout );
    subset_field_layout->addChild( new CheckBox( "field_select_solution_override" ) );
    subset_field_layout->addChild( (new ComboBox( "field_select_solution" ))
                                   ->setVisibilityKey( "field_select_solution_override" ) );
    subset_field_layout->addChild( new CheckBox( "field_select_report_step_override" ) );
    subset_field_layout->addChild( (new HorizontalSlider( "field_select_report_step" ))
                                   ->setVisibilityKey("field_select_report_step_override") );
    subset_field_layout->addChild( new VerticalExpandingSpace );
    PopupButton* subset_field_popup = new PopupButton( "details_label", false );
    subset_field_popup->setChild( subset_field_layout );
    subset_field_popup->setVisibilityKey( "surface_subset_field_range" );

    // subset index
    Grid* subset_index_grid = new Grid( 4, 4 );
    subset_index_grid->setChild( 0, 3, new HorizontalExpandingSpace );
    subset_index_grid->setChild( 0, 0, new Label("index_range_select_min_i"));
    subset_index_grid->setChild( 0, 1, new SpinBox("index_range_select_min_i"));
    subset_index_grid->setChild( 0, 2, new SpinBox("index_range_select_max_i"));
    subset_index_grid->setChild( 1, 0, new Label("index_range_select_min_j"));
    subset_index_grid->setChild( 1, 1, new SpinBox("index_range_select_min_j"));
    subset_index_grid->setChild( 1, 2, new SpinBox("index_range_select_max_j"));
    subset_index_grid->setChild( 2, 0, new Label("index_range_select_min_k"));
    subset_index_grid->setChild( 2, 1, new SpinBox("index_range_select_min_k"));
    subset_index_grid->setChild( 2, 2, new SpinBox("index_range_select_max_k"));
    PopupButton* subset_index_popup = new PopupButton( "details_label", false );
    subset_index_popup->setChild( subset_index_grid );
    subset_index_popup->setVisibilityKey( "surface_subset_index_range" );

    // --- subset plane ---
    Grid* geo_subset_plane_rot_grid = new Grid( 3, 3 );
    geo_subset_plane_rot_grid->setChild( 1, 0, new Button( "plane_select_NX" ) );
    geo_subset_plane_rot_grid->setChild( 1, 2, new Button( "plane_select_PX" ) );
    geo_subset_plane_rot_grid->setChild( 0, 1, new Button( "plane_select_PY" ) );
    geo_subset_plane_rot_grid->setChild( 2, 1, new Button( "plane_select_NY" ) );
    VerticalLayout* subset_plane_layout = new VerticalLayout;
    subset_plane_layout->addChild( geo_subset_plane_rot_grid );
    subset_plane_layout->addChild( new HorizontalSlider( "plane_select_shift" ) );
    PopupButton* subset_plane_popup = new PopupButton( "details_label", false );
    subset_plane_popup->setChild( subset_plane_layout );
    subset_plane_popup->setVisibilityKey( "surface_subset_plane" );


    HorizontalLayout* subsets_layout = new HorizontalLayout;
    subsets_layout->addChild( new tinia::model::gui::ComboBox( "surface_subset" ) );
    subsets_layout->addChild( subset_field_popup );
    subsets_layout->addChild( subset_index_popup );
    subsets_layout->addChild( subset_plane_popup );


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

    ElementGroup* surf_details_group = new ElementGroup( "surface_tab" );
    Grid* surf_details_grid = new Grid( 4, 4 );
    surf_details_grid->setChild( 0, 1, new HorizontalSpace );
    surf_details_grid->setChild( 0, 3, new HorizontalExpandingSpace );
    surf_details_grid->setChild( 0, 0, new Label( "subset_label" ) );
    surf_details_grid->setChild( 0, 2, subsets_layout );
    surf_details_grid->setChild( 1, 0, new Label( "tessellation_label" ) );
    surf_details_grid->setChild( 1, 2, new CheckBox( "tess_flip_orientation" ) );
    surf_details_grid->setChild( 2, 0, new Label( "colormap_label" ) );
    surf_details_grid->setChild( 2, 2, colormap_popup );

    surf_details_group->setChild( surf_details_grid );
    right_column->addChild( surf_details_group );


    right_master->addChild( new tinia::model::gui::VerticalExpandingSpace );

    std::vector<tinia::model::StateSchemaElement> elements;
    m_model->getFullStateSchema(elements);

    m_model->setGUILayout( root, tinia::model::gui::DESKTOP);

    for(auto it=files.begin(); it!=files.end(); ++it ) {
        if( (*it).find('.') != std::string::npos ) {
            loadFile( *it, 1, 1, 1, false );
            break;
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

bool
FRViewJob::handleButtonClick( tinia::model::StateElement *stateElement )
{
    bool value;
    stateElement->getValue<bool>( value );
    if( value ) {
        m_model->updateElement( stateElement->getKey(), false );
        return true;
    }
    return false;
}

void
FRViewJob::loadFile( const std::string& filename,
                     int refine_i,
                     int refine_j,
                     int refine_k,
                     bool triangulate )
{
    m_file.setFileName( filename );

    m_load_geometry = false;
    m_load_color_field = false;
    m_project = boost::shared_ptr< dataset::Project<float> >();
    std::list<std::string> solutions = {"none"};
    m_model->updateRestrictions( "field_solution", solutions.front(), solutions );
    m_model->updateRestrictions( "field_select_solution", solutions.front(), solutions );
    m_model->updateConstraints<int>("field_report_step", 0, 0, 0 );
    m_model->updateConstraints<int>( "field_select_report_step", 0, 0, 0 );
    m_model->updateConstraints<int>( "index_range_select_min_i", 0, 0, 0 );
    m_model->updateConstraints<int>( "index_range_select_max_i", 0, 0, 0 );
    m_model->updateConstraints<int>( "index_range_select_min_j", 0, 0, 0 );
    m_model->updateConstraints<int>( "index_range_select_max_j", 0, 0, 0 );
    m_model->updateConstraints<int>( "index_range_select_min_k", 0, 0, 0 );
    m_model->updateConstraints<int>( "index_range_select_max_k", 0, 0, 0 );
    m_grid_stats.update();

    releasePipeline();
    m_async_reader->issueReadProject( filename,
                                      refine_i,
                                      refine_j,
                                      refine_k,
                                      triangulate );
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
        if( !m_project ) {
            m_load_geometry = true;
        }
        else {
            m_load_color_field = true;
        }
    }
    else if( key == "field_solution" && m_project ) {
        string value;
        stateElement->getValue<string>( value );
        for(unsigned int i=0; i<m_project->solutions(); i++ ) {
            if( value == m_project->solutionName(i) ) {
                m_solution_index = i;
                break;
            }
        }
        dataset::Project<float>::Solution sol;
        if( m_project->solution( sol, m_solution_index, m_report_step_index ) ) {
            if( m_async_reader->issueReadSolution( sol ) ) {
            }
        }
    }
    else if( key == "field_report_step" && m_project ) {
        int value;
        stateElement->getValue<int>( value );
        m_report_step_index = value;
        dataset::Project<float>::Solution sol;
        if( m_project->solution( sol, m_solution_index, m_report_step_index ) ) {
            if( m_async_reader->issueReadSolution( sol ) ) {
            }
        }
    }
    else if( key == "plane_select_PX" && handleButtonClick(stateElement) ) {
        if( m_clip_plane.get() != NULL ) {
            m_clip_plane->rotate( glm::quat( glm::vec3( 0.f, 0.f, -0.1*M_PI_4 ) ) );
            m_do_update_subset = true;
            if( m_renderlist_state == RENDERLIST_SENT ) {
                m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
            }
        }
    }
    else if( key == "plane_select_NX"  && handleButtonClick(stateElement) ) {
        if( m_clip_plane.get() != NULL ) {
            m_clip_plane->rotate( glm::quat( glm::vec3( 0.f, 0.f, 0.1*M_PI_4 ) ) );
            m_do_update_subset = true;
            if( m_renderlist_state == RENDERLIST_SENT ) {
                m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
            }
        }
    }
    else if( key == "plane_select_PY"  && handleButtonClick(stateElement) ) {
        if( m_clip_plane.get() != NULL ) {
            m_clip_plane->rotate( glm::quat( glm::vec3( 0.1*M_PI_4, 0.f, 0.f ) ) );
            m_do_update_subset = true;
            if( m_renderlist_state == RENDERLIST_SENT ) {
                m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
            }
        }
    }
    else if( key == "plane_select_NY"  && handleButtonClick(stateElement) ) {
        if( m_clip_plane.get() != NULL ) {
            m_clip_plane->rotate( glm::quat( glm::vec3( -0.1*M_PI_4, 0.f, 0.f ) ) );
            m_do_update_subset = true;
            if( m_renderlist_state == RENDERLIST_SENT ) {
                m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
            }
        }
    }
    else if( key == "plane_select_shift" ) {
        int value;
        stateElement->getValue<int>( value );
        if( m_clip_plane.get() != NULL ) {
            m_clip_plane->setOffset( (1.7f/1000.f)*(value) );
            m_do_update_subset = true;
            if( m_renderlist_state == RENDERLIST_SENT ) {
                m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
            }
        }
    }
    else if( key == "index_range_select_min_i" ) {
        int min;
        stateElement->getValue<int>( min );
        int max;
        m_model->getElementValue( "index_range_select_max_i", max );
        if( max < min ) {
            m_model->updateElement( "index_range_select_max_i", min );
        }
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
    }
    else if( key == "index_range_select_max_i" ) {
        int max;
        stateElement->getValue<int>( max );
        int min;
        m_model->getElementValue( "index_range_select_min_i", min );
        if( max < min ) {
            m_model->updateElement( "index_range_select_min_i", max );
        }
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
    }
    else if( key == "index_range_select_min_j" ) {
        int min;
        stateElement->getValue<int>( min );
        int max;
        m_model->getElementValue( "index_range_select_max_j", max );
        if( max < min ) {
            m_model->updateElement( "index_range_select_max_j", min );
        }
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
    }
    else if( key == "index_range_select_max_j" ) {
        int max;
        stateElement->getValue<int>( max );
        int min;
        m_model->getElementValue( "index_range_select_min_j", min );
        if( max < min ) {
            m_model->updateElement( "index_range_select_min_j", max );
        }
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
    }
    else if( key == "index_range_select_min_k" ) {
        int min;
        stateElement->getValue<int>( min );
        int max;
        m_model->getElementValue( "index_range_select_max_k", max );
        if( max < min ) {
            m_model->updateElement( "index_range_select_max_k", min );
        }
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
    }
    else if( key == "index_range_select_max_k" ) {
        int max;
        stateElement->getValue<int>( max );
        int min;
        m_model->getElementValue( "index_range_select_min_k", min );
        if( max < min ) {
            m_model->updateElement( "index_range_select_min_k", max );
        }
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
    }
    else if( key == "field_select_min" ) {
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
    }
    else if( key == "field_select_max" ) {
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
    }
    else if( key == "surface_subset" ) {
        string value;
        stateElement->getValue<string>( value );
        if( value == "all" ) {
            m_model->updateElement( "surface_subset_field_range", false );
            m_model->updateElement( "surface_subset_index_range", false );
            m_model->updateElement( "surface_subset_plane", false );
        }
        else if( value == "subset_field" ) {
            m_model->updateElement( "surface_subset_field_range", true );
            m_model->updateElement( "surface_subset_index_range", false );
            m_model->updateElement( "surface_subset_plane", false );
        }
        else if( value == "subset_index" ) {
            m_model->updateElement( "surface_subset_field_range", false );
            m_model->updateElement( "surface_subset_index_range", true );
            m_model->updateElement( "surface_subset_plane", false );
        }
        else if( value == "subset_plane" ) {
            m_model->updateElement( "surface_subset_field_range", false );
            m_model->updateElement( "surface_subset_index_range", false );
            m_model->updateElement( "surface_subset_plane", true );

        }
        else if( value == "subset_halfplane" ) {
            m_model->updateElement( "surface_subset_field_range", false );
            m_model->updateElement( "surface_subset_index_range", false );
            m_model->updateElement( "surface_subset_plane", true );
        }
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
    }
    else if( key == "field_range_enable" ) {
        bool value;
        stateElement->getValue( value );
        if( !value && m_grid_field.get() != NULL ) {
            m_model->updateElement<double>( "field_range_min", m_grid_field->minValue() );
            m_model->updateElement<double>( "field_range_max", m_grid_field->maxValue() );
        }
    }
    else if( key == "tess_flip_orientation" ) {
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
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
    if( m_has_pipeline ) {
        glm::vec3 bbmin( m_grid_tess->minBBox()[0], m_grid_tess->minBBox()[1], m_grid_tess->minBBox()[2] );
        glm::vec3 bbmax( m_grid_tess->maxBBox()[0], m_grid_tess->maxBBox()[1], m_grid_tess->maxBBox()[2] );
        glm::vec3 shift = -0.5f*( bbmin + bbmax );
        float scale_xy = std::max( (bbmax.x-bbmin.x),
                                   (bbmax.y-bbmin.y) );

        float scale_z = m_grid_stats.zScale();
        if( m_project.get() != NULL ) {
            scale_z = scale_z*(m_project->cornerPointZScale()/m_project->cornerPointXYScale());
        }
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
                                  void* /*data*/ )
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

    if( false && glewIsSupported( "GL_KHR_debug" ) ) {
        glEnable( GL_DEBUG_OUTPUT );
        glDebugMessageCallback( debugLogger, NULL );
        glDebugMessageControl( GL_DONT_CARE,
                               GL_DONT_CARE,
                               GL_DEBUG_SEVERITY_LOW,
                               0, NULL, GL_TRUE );

    }
    m_has_context = true;
    if( m_project.get() != NULL ) {
        setupPipeline();
    }
    return true;
}

void
FRViewJob::releasePipeline()
{
    if( !m_has_context ) {
        return;
    }
    m_clip_plane = boost::shared_ptr<render::ClipPlane>();
    m_grid_tess = boost::shared_ptr<render::GridTess>();
    m_faults_surface = boost::shared_ptr<render::surface::GridTessSurf>();
    m_subset_surface = boost::shared_ptr<render::surface::GridTessSurf>();
    m_boundary_surface = boost::shared_ptr<render::surface::GridTessSurf>();
    m_grid_tess_surf_builder = boost::shared_ptr<render::surface::GridTessSurfBuilder>();
    m_grid_field = boost::shared_ptr<render::GridField>();
    m_all_selector = boost::shared_ptr<render::subset::BuilderSelectAll>();
    m_field_selector = boost::shared_ptr<render::subset::BuilderSelectByFieldValue>();
    m_index_selector = boost::shared_ptr<render::subset::BuilderSelectByIndex>();
    m_plane_selector = boost::shared_ptr<render::subset::BuilderSelectOnPlane>();
    m_half_plane_selector = boost::shared_ptr<render::subset::BuilderSelectInsideHalfplane>();
    m_grid_tess_subset = boost::shared_ptr<render::subset::Representation>();
    m_grid_cube_renderer = boost::shared_ptr<render::GridCubeRenderer>();
    m_wells = boost::shared_ptr<render::wells::Representation>();
    m_coordsys_renderer = boost::shared_ptr<render::CoordSysRenderer>();
    m_grid_voxelizer = boost::shared_ptr<render::rlgen::GridVoxelization>();
    m_voxel_surface = boost::shared_ptr<render::rlgen::VoxelSurface>();
    m_has_pipeline = false;
}

bool FRViewJob::setupPipeline()
{
    if( !m_has_context ) {
        return false;
    }
    try {
        m_clip_plane.reset( new render::ClipPlane( glm::vec3( -0.1f ) , glm::vec3( 1.1f ), glm::vec4(0.f, 1.f, 0.f, 0.f ) ) );
        m_grid_tess.reset( new render::GridTess );
        m_faults_surface.reset( new render::surface::GridTessSurf );
        m_subset_surface.reset( new render::surface::GridTessSurf );
        m_boundary_surface.reset( new render::surface::GridTessSurf );
        m_grid_tess_surf_builder.reset( new render::surface::GridTessSurfBuilder );
        m_grid_field.reset(  new render::GridField( m_grid_tess ) );
        m_all_selector.reset( new render::subset::BuilderSelectAll );
        m_field_selector.reset( new render::subset::BuilderSelectByFieldValue );
        m_index_selector.reset( new render::subset::BuilderSelectByIndex );
        m_plane_selector.reset( new render::subset::BuilderSelectOnPlane );
        m_half_plane_selector.reset( new render::subset::BuilderSelectInsideHalfplane );
        m_grid_tess_subset.reset( new render::subset::Representation );
        m_grid_cube_renderer.reset( new render::GridCubeRenderer );
        m_wells.reset( new render::wells::Representation );
        m_coordsys_renderer.reset( new render::CoordSysRenderer );
        m_grid_voxelizer.reset( new render::rlgen::GridVoxelization );
        m_voxel_surface.reset( new render::rlgen::VoxelSurface );
        m_has_pipeline = true;
    }
    catch( std::runtime_error& e ) {
        Logger log = getLogger( "CPViewJob.setupPipeline" );
        LOGGER_ERROR( log, "Failed to set up pipeline: " << e.what() );
        releasePipeline();
    }
    return m_has_pipeline;
}

void
FRViewJob::doLogic()
{
    if( m_zscale != m_grid_stats.zScale() ) {
        m_zscale = m_grid_stats.zScale();
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
//        if( !m_do_update_renderlist ) {
//            m_do_update_renderlist = true;
            // trigger render list update
//        }

        if( m_render_clip_plane ) {
            m_do_update_subset = true;
        }
    }

    models::Appearance::VisibilityMask new_mask = m_appearance.visibilityMask();
    if( (m_visibility_mask != new_mask) || m_under_the_hood.profilingEnabled() ) {
        m_do_update_subset = true;
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
//        m_do_update_renderlist = true;
        m_visibility_mask = new_mask;
    }


    if( m_renderlist_state == RENDERLIST_CHANGED_NOTIFY_CLIENTS ) {
        m_renderlist_state = RENDERLIST_CHANGED_CLIENTS_NOTIFIED;
        int val;
        m_model->getElementValue( "renderlist", val );
        m_model->updateElement( "renderlist", val+1 );
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


    return true;
}


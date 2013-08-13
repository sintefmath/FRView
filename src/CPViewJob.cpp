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
#include <stdexcept>
#include <string>

#include <tinia/policy/GUILayout.hpp>
#include <tinia/policy/PolicyLock.hpp>

#include "CPViewJob.hpp"
#include "Logger.hpp"
#include "Project.hpp"
#include "GridTess.hpp"
#include "GridTessSubset.hpp"
#include "GridTessSurf.hpp"
#include "GridTessSurfBuilder.hpp"
#include "GridTessSurfRenderer.hpp"
#include "GridField.hpp"
#include "CellSelector.hpp"
#include "ClipPlane.hpp"
#include "GridTessSurfBBoxFinder.hpp"
#include "GridCubeRenderer.hpp"
#include "TextRenderer.hpp"
#include "WellRenderer.hpp"
#include "TikZExporter.hpp"

using std::string;
using std::stringstream;




CPViewJob::CPViewJob( const std::list<string>& files )
    : tinia::jobobserver::OpenGLJob(),
      m_project( new Project<float>(files) ),
      m_cares_about_renderlists( false ),
      m_policies_changed( true ),
      m_renderlist_rethink( false ),
      m_clip_plane( NULL ),
      m_grid_tess( NULL ),
      m_grid_tess_subset( NULL ),
      m_subset_surface( NULL ),
      m_grid_tess_surf_builder( NULL ),
      m_grid_field( NULL ),
      m_tess_renderer( NULL ),
      m_half_plane_selector( NULL ),
      m_grid_cube_renderer( NULL ),
      m_well_renderer( NULL )
{


    m_load_geometry = true;
    m_load_color_field = false;
    m_has_color_field = false;
    m_render_clip_plane = false;
    m_care_about_updates = true;
    m_report_step_index = 0;
    m_solution_index = 0;
    m_policy->addStateListener(this);

    m_faults_surface_tainted = true;
    m_subset_surface_tainted = true;
    m_boundary_surface_tainted = true;

    float identity[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };


    // fetch data that is needed to populate the policy
    tinia::policy::Viewer viewer;
    viewer.height = 640;
    viewer.width = 360;
    for(unsigned int i=0; i<16; i++) {
        viewer.modelviewMatrix[i] = identity[i];
        viewer.projectionMatrix[i] = identity[i];
    }
    std::list<string> solutions;
    if( m_project->solutions() == 0 ) {
        solutions.push_back( "[none]" );
    }
    else {
        for(unsigned int i=0; i<m_project->solutions(); i++ ) {
            solutions.push_back( m_project->solutionName(i) );
        }
    }
    int reportstep_max = std::max( 1u, m_project->reportSteps() )-1u;
    const char* colormap_types[] = { "Linear", "Logarithmic" };

    const char* subsets[] = { "all",
                              "subset_field",
                              "subset_index",
                              "subset_plane",
                              "subset_halfplane" };

    int nx_max = std::max( 1u, m_project->nx() ) - 1u;
    int ny_max = std::max( 1u, m_project->ny() ) - 1u;
    int nz_max = std::max( 1u, m_project->nz() ) - 1u;

    const double dmax = std::numeric_limits<double>::max();



    m_policy->addElement("viewer", viewer );
    m_policy->addElement<bool>("myTab", true);

//    m_policy->addElement<string>( "renderlist", "" );
    m_policy->addElement<string>( "boundingbox", "-0.1 -0.1 -0.1 1.1 1.1 1.1" );
    m_policy->addElement<int>( "renderlist", 0 );
    m_policy->addElement<bool>( "grid_details_enable", true, "Grid" );
    m_policy->addElement<bool>( "export", false, "Export");
    m_policy->addElement<bool>( "export_tikz", false, "TikZ");
    m_policy->addElement<string>( "grid_dim", "foo", "Dimensions" );
    m_policy->addElement<string>( "grid_total_cells", "baz", "Cells" );
    m_policy->addElement<string>( "grid_active_cells", "baz", "Active cells" );
    m_policy->addElement<string>( "grid_faces", "bar", "Faces" );
    m_policy->addElement<bool>( "field_info_enable", true, "Field" );
    m_policy->addConstrainedElement<double>( "z_scale", 10.0, 0.01, 1000.0, "Z-scale" );
//    m_policy->addConstrainedElement<double>( "z_scale", 1.0, 0.01, 1000.0, "Z-scale" );
    m_policy->addElement<string>( "field_info_calendar", "n/a", "Date" );
    m_policy->addElement<string>( "field_info_range", "n/a", "Range" );
    m_policy->addElement<bool>( "project_tab", true, "Project" );
    m_policy->addElement<bool>( "rendering_label", true, "Rendering" );
    m_policy->addElement<bool>( "transparency_label", true, "Transparency" );
    m_policy->addElement<bool>( "surface_tab", true, "Surfaces" );
    m_policy->addElement<bool>( "well_tab", true, "Wells" );
    m_policy->addElement<bool>("field_group_enable", true, "Field" );
    m_policy->addConstrainedElement<int>("field_report_step", 0,0,  reportstep_max, "Report step" );
    m_policy->addElementWithRestriction<string>("field_solution",
                                                        solutions.front(),
                                                        solutions.begin(),
                                                        solutions.end() );
    m_policy->addAnnotation( "field_solution", "Solution" );
    m_policy->addElement<bool>( "has_field", false );

    m_policy->addElement<bool>( "colormap_label", true, "Color map" );
    m_policy->addElementWithRestriction<string>( "colormap_type",
                                                    colormap_types[0],
                                                    &colormap_types[0],
                                                    &colormap_types[2] );
    m_policy->addAnnotation( "colormap_type", "Type" );
    m_policy->addElement<bool>( "field_range_enable", false, "Lock min and max" );
    m_policy->addConstrainedElement<double>( "field_range_min", 0.0, -dmax, dmax, "Min" );
    m_policy->addConstrainedElement<double>( "field_range_max", 0.0, -dmax, dmax, "Max" );
    m_policy->addElement<bool>("grid_subset", true, "Subset" );
    m_policy->addElementWithRestriction<string>( "surface_subset",
                                                    subsets[0],
                                                    &subsets[0],
                                                    &subsets[5] );
    m_policy->addElement<bool>("faults_label", true, "Faults" );
    m_policy->addConstrainedElement<int>( "faults_fill_opacity", 0, 0, 100, "Fill" );
    m_policy->addConstrainedElement<int>( "faults_outline_opacity", 0, 0, 100, "Opacity" );
    m_policy->addElement<bool>( "rendering_quality_group", true, "Rendering quality" );
    m_policy->addConstrainedElement<int>( "rendering_quality", 3, 0, 3, "Rendering quality" );
    m_policy->addElement<string>( "rendering_quality_string", "high", "Details" );
//    m_policy->addConstrainedElement<int>( "subset_fill_opacity", 0, 0, 100, "Fill" );
//    m_policy->addConstrainedElement<int>( "subset_outline_opacity", 20, 0, 100, "Outlines");
    m_policy->addConstrainedElement<int>( "subset_fill_opacity", 100, 0, 100, "Fill" );
    m_policy->addConstrainedElement<int>( "subset_outline_opacity", 100, 0, 100, "Outlines");
    m_policy->addElement<bool>( "surface_subset_field_range", false );
    m_policy->addElement<bool>( "surface_subset_index_range", false );
    m_policy->addElement<bool>( "surface_subset_plane", false );
    m_policy->addElement<bool>( "subset_label", true, "Subset" );
    m_policy->addElement<bool>( "tessellation_label", true, "Tessellation" );
    m_policy->addElement<bool>( "tess_flip_orientation", false, "Flip orientation" );
    m_policy->addElement<bool>( "plane_select_PY", false, "PY" );
    m_policy->addElement<bool>( "plane_select_NY", false, "NY" );
    m_policy->addElement<bool>( "plane_select_PX", false, "PX" );
    m_policy->addElement<bool>( "plane_select_NX", false, "NX" );
    m_policy->addConstrainedElement( "plane_select_shift", 0, -500, 500, "Shift" );
    m_policy->addConstrainedElement<int>( "index_range_select_min_i", 0, 0, nx_max, "I" );
    m_policy->addConstrainedElement<int>( "index_range_select_max_i", nx_max, 0, nx_max, "I" );
    m_policy->addConstrainedElement<int>( "index_range_select_min_j", 0, 0, ny_max, "J" );
    m_policy->addConstrainedElement<int>( "index_range_select_max_j", ny_max, 0, ny_max, "J" );
    m_policy->addConstrainedElement<int>( "index_range_select_min_k", 0, 0, nz_max, "k" );
    m_policy->addConstrainedElement<int>( "index_range_select_max_k", nz_max, 0, nz_max, "K" );
    m_policy->addConstrainedElement<double>( "field_select_min", 0.0, -dmax, dmax, "Min" );
    m_policy->addConstrainedElement<double>( "field_select_max", 0.0, -dmax, dmax, "Max" );
    m_policy->addElement<bool>( "field_select_report_step_override", false, "Specific report step" );
    m_policy->addConstrainedElement<int>( "field_select_report_step", 0, 0, reportstep_max, "Field select report step" );
    m_policy->addElement<bool>( "field_select_solution_override", false );
    m_policy->addElementWithRestriction<string>( "field_select_solution",
                                                         solutions.front(),
                                                         solutions.begin(),
                                                         solutions.end() );
    m_policy->addAnnotation( "field_select_solution_override", "Specific solution");
    m_policy->addElement<bool>("grid_boundary", true, "Grid boundary" );
    m_policy->addConstrainedElement<int>( "boundary_fill_opacity", 5, 0, 100, "Fill" );
    m_policy->addConstrainedElement<int>( "boundary_outline_opacity", 50, 0, 100, "Outlines" );
    m_policy->addElement<bool>( "field_range_label", true, "Range" );
    m_policy->addElement<bool>( "z_scale_label", true, "Z-scale" );
    m_policy->addElement<bool>( "color_label", true, "Color" );
    m_policy->addElement<bool>( "details_label", true, "Details" );

    using namespace tinia::policy::gui;

    tinia::policy::gui::HorizontalLayout* root = new tinia::policy::gui::HorizontalLayout;
    tinia::policy::gui::VerticalLayout* right_column = new tinia::policy::gui::VerticalLayout;
    tinia::policy::gui::Canvas* canvas = new tinia::policy::gui::Canvas("viewer", "renderlist", "boundingbox" );
    root->addChild( canvas );
    root->addChild( right_column );

    // --- info
    tinia::policy::gui::ElementGroup* grid_details_group = new tinia::policy::gui::ElementGroup( "grid_details_enable", true );
    tinia::policy::gui::Grid* grid_details_grid = new tinia::policy::gui::Grid( 6, 4 );
    grid_details_grid->setChild( 0, 1, new tinia::policy::gui::HorizontalSpace );
    grid_details_grid->setChild( 0, 3, new tinia::policy::gui::HorizontalExpandingSpace );
    grid_details_grid->setChild( 0, 0, new tinia::policy::gui::Label( "grid_dim" ) );
    grid_details_grid->setChild( 0, 2, new tinia::policy::gui::Label( "grid_dim", true ) );
    grid_details_grid->setChild( 1, 0, new tinia::policy::gui::Label( "grid_total_cells" ) );
    grid_details_grid->setChild( 1, 2, new tinia::policy::gui::Label( "grid_total_cells", true ) );
    grid_details_grid->setChild( 2, 0, new tinia::policy::gui::Label( "grid_active_cells" ) );
    grid_details_grid->setChild( 2, 2, new tinia::policy::gui::Label( "grid_active_cells", true ) );
    grid_details_grid->setChild( 3, 0, new tinia::policy::gui::Label( "grid_faces" ) );
    grid_details_grid->setChild( 3, 2, new tinia::policy::gui::Label( "grid_faces", true ) );
    grid_details_grid->setChild( 4, 0, new tinia::policy::gui::Label( "z_scale" ) );
    grid_details_grid->setChild( 4, 2, new tinia::policy::gui::DoubleSpinBox( "z_scale" ) );
    grid_details_grid->setChild( 5, 0, new tinia::policy::gui::Label( "export" ) );
    grid_details_grid->setChild( 5, 2, new tinia::policy::gui::Button( "export_tikz" ) );
    grid_details_group->setChild( grid_details_grid );
    right_column->addChild( grid_details_group );

    tinia::policy::gui::ElementGroup* field_details_group = new tinia::policy::gui::ElementGroup( "field_info_enable", true );
    tinia::policy::gui::Grid* field_details_grid = new tinia::policy::gui::Grid( 4, 4 );
    field_details_grid->setChild( 0, 1, new tinia::policy::gui::HorizontalSpace );
    field_details_grid->setChild( 0, 3, new tinia::policy::gui::HorizontalExpandingSpace );
    field_details_grid->setChild( 0, 0, new tinia::policy::gui::Label( "field_solution" ) );
    field_details_grid->setChild( 0, 2, new tinia::policy::gui::ComboBox( "field_solution" ) );
    field_details_grid->setChild( 1, 0, new tinia::policy::gui::Label( "field_report_step" ) );
    field_details_grid->setChild( 1, 2, new tinia::policy::gui::HorizontalSlider( "field_report_step" ) );
    field_details_grid->setChild( 2, 0, new tinia::policy::gui::Label( "field_info_calendar" ));
    field_details_grid->setChild( 2, 2, (new tinia::policy::gui::Label( "field_info_calendar", true ))->setEnabledKey( "has_field") );
    field_details_grid->setChild( 3, 0, new tinia::policy::gui::Label( "field_info_range" ) );
    field_details_grid->setChild( 3, 2, (new tinia::policy::gui::Label( "field_info_range", true ))->setEnabledKey( "has_field" ) );
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
    subsets_layout->addChild( new tinia::policy::gui::ComboBox( "surface_subset" ) );
    subsets_layout->addChild( subset_field_popup );
    subsets_layout->addChild( subset_index_popup );
    subsets_layout->addChild( subset_plane_popup );




    Grid* alpha_faults_layout = new Grid(2,3);
    alpha_faults_layout->setChild( 0, 0, new Label( "faults_fill_opacity" ) );
    alpha_faults_layout->setChild( 0, 1, new HorizontalSlider( "faults_fill_opacity" ) );
    alpha_faults_layout->setChild( 1, 0, new Label( "faults_outline_opacity" ) );
    alpha_faults_layout->setChild( 1, 1, new HorizontalSlider( "faults_outline_opacity" ) );
    alpha_faults_layout->setChild( 2, 1, new HorizontalExpandingSpace );
    ElementGroup* alpha_faults_group = new ElementGroup( "faults_label", true );
    alpha_faults_group->setChild( alpha_faults_layout );

    Grid* alpha_subset_layout = new Grid(3,2);
    alpha_subset_layout->setChild( 0, 0, new Label( "subset_fill_opacity" ) );
    alpha_subset_layout->setChild( 0, 1, new HorizontalSlider( "subset_fill_opacity" ) );
    alpha_subset_layout->setChild( 1, 0, new Label( "subset_outline_opacity" ) );
    alpha_subset_layout->setChild( 1, 1, new HorizontalSlider( "subset_outline_opacity" ) );
    alpha_subset_layout->setChild( 2, 1, new HorizontalExpandingSpace );
    ElementGroup* alpha_subset_group = new ElementGroup( "subset_label", true );
    alpha_subset_group->setChild( alpha_subset_layout );

    Grid* alpha_boundary_layout = new Grid(3,2);
    alpha_boundary_layout->setChild( 0, 0, new Label( "boundary_fill_opacity" ) );
    alpha_boundary_layout->setChild( 0, 1, new HorizontalSlider( "boundary_fill_opacity" ) );
    alpha_boundary_layout->setChild( 1, 0, new Label( "boundary_outline_opacity" ) );
    alpha_boundary_layout->setChild( 1, 1, new HorizontalSlider( "boundary_outline_opacity" ) );
    alpha_boundary_layout->setChild( 2, 1, new HorizontalExpandingSpace );
    ElementGroup* alpha_boundary_group = new ElementGroup( "grid_boundary", true );
    alpha_boundary_group->setChild( alpha_boundary_layout );

    HorizontalLayout* alpha_quality_layout = new HorizontalLayout;
    alpha_quality_layout->addChild( new HorizontalSlider( "rendering_quality", true ) );
    alpha_quality_layout->addChild( new Label( "rendering_quality_string", true ) );
    ElementGroup* alpha_quality_group = new ElementGroup( "rendering_quality_group", true );
    alpha_quality_group->setChild( alpha_quality_layout );


    VerticalLayout* transparency_layout = new VerticalLayout;
    transparency_layout->addChild( alpha_quality_group );
    transparency_layout->addChild( alpha_faults_group );
    transparency_layout->addChild( alpha_subset_group );
    transparency_layout->addChild( alpha_boundary_group );

    PopupButton* transparency_popup = new PopupButton( "rendering_quality_string", false );
    transparency_popup->setChild( transparency_layout );

    HorizontalLayout* transparency_popup_c = new HorizontalLayout;
    transparency_popup_c->addChild( new Label( "rendering_quality_string", true ) );
    transparency_popup_c->addChild( transparency_popup );


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
    surf_details_grid->setChild( 2, 0, new Label( "transparency_label" ) );
    surf_details_grid->setChild( 2, 2, transparency_popup_c );
    surf_details_grid->setChild( 3, 0, new Label( "colormap_label" ) );
    surf_details_grid->setChild( 3, 2, colormap_popup );

    surf_details_group->setChild( surf_details_grid );
    right_column->addChild( surf_details_group );
    right_column->addChild( new tinia::policy::gui::VerticalExpandingSpace );

    std::vector<tinia::policy::StateSchemaElement> elements;
    m_policy->getFullStateSchema(elements);

    m_policy->setGUILayout( root, tinia::policy::gui::DESKTOP);

}

bool
CPViewJob::init()
{
    return true;
}

CPViewJob::~CPViewJob()
{
   m_policy->removeStateListener(this);
}

bool
CPViewJob::handleButtonClick( tinia::policy::StateElement *stateElement )
{
    bool value;
    stateElement->getValue<bool>( value );
    if( value ) {
        m_policy->updateElement( stateElement->getKey(), false );
        return true;
    }
    return false;
}


void
CPViewJob::stateElementModified( tinia::policy::StateElement *stateElement )
{
    if( !m_care_about_updates ) {
        return;
    }
    m_care_about_updates = false;

    Logger log = getLogger( "CPViewJob.stateElementModified" );


    const string& key = stateElement->getKey();
    if( key == "export_tikz" && handleButtonClick(stateElement) ) {
        tinia::policy::Viewer viewer;
        m_policy->getElementValue( "viewer", viewer );
        glm::mat4 mv = glm::make_mat4( viewer.modelviewMatrix.data() );
        glm::mat4 p = glm::make_mat4( viewer.projectionMatrix.data() );

        float s = std::max( viewer.width, viewer.height );
        glm::mat4 vpt( viewer.width/s, 0.f, 0.f, 0.f,
                      0.0f, viewer.height/s, 0.f, 0.f,
                      0.f, 0.f, 1.f, 0.f,
                      0.f, 0.f, 0.f, 1.f );

        TikZExporter exporter( m_grid_tess );
        exporter.run( "dump.tex",
                      vpt*p*mv*m_local_to_world,
                      glm::transpose( glm::inverse( mv*m_local_to_world ) ) );

//        m_grid_tess->exportTikZ( "dump.tex",  vpt*p*mv*m_local_to_world );
    }
    if( key == "field_solution" ) {
        string value;
        stateElement->getValue<string>( value );
        for(unsigned int i=0; i<m_project->solutions(); i++ ) {
            if( value == m_project->solutionName(i) ) {
                m_solution_index = i;
                m_load_color_field = true;
                break;
            }
        }
        m_load_color_field = true;
        m_do_update_subset = true;
    }
    else if( key == "field_report_step" ) {
        int value;
        stateElement->getValue<int>( value );
        m_report_step_index = value;
        m_load_color_field = true;
        m_do_update_subset = true;
    }
    else if( key == "plane_select_PX" && handleButtonClick(stateElement) ) {
        if( m_clip_plane != NULL ) {
            m_clip_plane->rotate( glm::quat( glm::vec3( 0.f, 0.f, -0.1*M_PI_4 ) ) );
            m_do_update_subset = true;
        }
    }
    else if( key == "plane_select_NX"  && handleButtonClick(stateElement) ) {
        if( m_clip_plane != NULL ) {
            m_clip_plane->rotate( glm::quat( glm::vec3( 0.f, 0.f, 0.1*M_PI_4 ) ) );
            m_do_update_subset = true;
        }
    }
    else if( key == "plane_select_PY"  && handleButtonClick(stateElement) ) {
        if( m_clip_plane != NULL ) {
            m_clip_plane->rotate( glm::quat( glm::vec3( 0.1*M_PI_4, 0.f, 0.f ) ) );
            m_do_update_subset = true;
        }
    }
    else if( key == "plane_select_NY"  && handleButtonClick(stateElement) ) {
        if( m_clip_plane != NULL ) {
            m_clip_plane->rotate( glm::quat( glm::vec3( -0.1*M_PI_4, 0.f, 0.f ) ) );
            m_do_update_subset = true;
        }
    }
    else if( key == "plane_select_shift" ) {
        int value;
        stateElement->getValue<int>( value );
        if( m_clip_plane != NULL ) {
            m_clip_plane->setOffset( (1.7f/1000.f)*(value) );
            m_do_update_subset = true;
        }
    }
    else if( key == "index_range_select_min_i" ) {
        int min;
        stateElement->getValue<int>( min );
        int max;
        m_policy->getElementValue( "index_range_select_max_i", max );
        if( max < min ) {
            m_policy->updateElement( "index_range_select_max_i", min );
        }
        m_do_update_subset = true;
    }
    else if( key == "index_range_select_max_i" ) {
        int max;
        stateElement->getValue<int>( max );
        int min;
        m_policy->getElementValue( "index_range_select_min_i", min );
        if( max < min ) {
            m_policy->updateElement( "index_range_select_min_i", max );
        }
        m_do_update_subset = true;
    }
    else if( key == "index_range_select_min_j" ) {
        int min;
        stateElement->getValue<int>( min );
        int max;
        m_policy->getElementValue( "index_range_select_max_j", max );
        if( max < min ) {
            m_policy->updateElement( "index_range_select_max_j", min );
        }
        m_do_update_subset = true;
    }
    else if( key == "index_range_select_max_j" ) {
        int max;
        stateElement->getValue<int>( max );
        int min;
        m_policy->getElementValue( "index_range_select_min_j", min );
        if( max < min ) {
            m_policy->updateElement( "index_range_select_min_j", max );
        }
        m_do_update_subset = true;
    }
    else if( key == "index_range_select_min_k" ) {
        int min;
        stateElement->getValue<int>( min );
        int max;
        m_policy->getElementValue( "index_range_select_max_k", max );
        if( max < min ) {
            m_policy->updateElement( "index_range_select_max_k", min );
        }
        m_do_update_subset = true;
    }
    else if( key == "index_range_select_max_k" ) {
        int max;
        stateElement->getValue<int>( max );
        int min;
        m_policy->getElementValue( "index_range_select_min_k", min );
        if( max < min ) {
            m_policy->updateElement( "index_range_select_min_k", max );
        }
        m_do_update_subset = true;
    }
    else if( key == "field_select_min" ) {
        m_do_update_subset = true;
    }
    else if( key == "field_select_max" ) {
        m_do_update_subset = true;
    }
    else if( key == "surface_subset" ) {
        string value;
        stateElement->getValue<string>( value );
        if( value == "all" ) {
            m_policy->updateElement( "surface_subset_field_range", false );
            m_policy->updateElement( "surface_subset_index_range", false );
            m_policy->updateElement( "surface_subset_plane", false );
        }
        else if( value == "subset_field" ) {
            m_policy->updateElement( "surface_subset_field_range", true );
            m_policy->updateElement( "surface_subset_index_range", false );
            m_policy->updateElement( "surface_subset_plane", false );
        }
        else if( value == "subset_index" ) {
            m_policy->updateElement( "surface_subset_field_range", false );
            m_policy->updateElement( "surface_subset_index_range", true );
            m_policy->updateElement( "surface_subset_plane", false );
        }
        else if( value == "subset_plane" ) {
            m_policy->updateElement( "surface_subset_field_range", false );
            m_policy->updateElement( "surface_subset_index_range", false );
            m_policy->updateElement( "surface_subset_plane", true );

        }
        else if( value == "subset_halfplane" ) {
            m_policy->updateElement( "surface_subset_field_range", false );
            m_policy->updateElement( "surface_subset_index_range", false );
            m_policy->updateElement( "surface_subset_plane", true );
        }
        m_do_update_subset = true;
    }
    else if( key == "field_range_enable" ) {
        bool value;
        stateElement->getValue( value );
        if( !value && m_grid_field != NULL ) {
            m_policy->updateElement<double>( "field_range_min", m_grid_field->minValue() );
            m_policy->updateElement<double>( "field_range_max", m_grid_field->maxValue() );
        }
    }
    else if( key == "tess_flip_orientation" ) {
        m_do_update_subset = true;
    }
    else if( key == "z_scale" && m_render_clip_plane ) {
        m_do_update_subset = true;
    }
    else if( key == "rendering_quality" ) {
        int value;
        stateElement->getValue( value );
        switch( value ) {
        case 0:
            m_policy->updateElement<string>( "rendering_quality_string", "crappy" );
            break;
        case 1:
            m_policy->updateElement<string>( "rendering_quality_string", "low" );
            break;
        case 2:
            m_policy->updateElement<string>( "rendering_quality_string", "medium" );
            break;
        case 3:
            m_policy->updateElement<string>( "rendering_quality_string", "high" );
            break;
        }
    }

    if( m_load_color_field ) {
        LOGGER_DEBUG( log, "report_step_index=" << m_report_step_index << ", solution=" << m_solution_index );
    }
    m_policies_changed = true;
    m_care_about_updates = true;
}

void
CPViewJob::triggerRedraw( const string& viewer_key )
{
    tinia::policy::Viewer viewer;
    m_policy->getElementValue( viewer_key, viewer );
    m_policy->updateElement( viewer_key, viewer );
}


void
CPViewJob::updateModelMatrices()
{
    glm::vec3 bbmin( m_grid_tess->minBBox()[0], m_grid_tess->minBBox()[1], m_grid_tess->minBBox()[2] );
    glm::vec3 bbmax( m_grid_tess->maxBBox()[0], m_grid_tess->maxBBox()[1], m_grid_tess->maxBBox()[2] );

    glm::vec3 shift = -0.5f*( bbmin + bbmax );
    float scale_xy = std::max( (bbmax.x-bbmin.x),
                               (bbmax.y-bbmin.y) );
    double scale_z;
    m_policy->getElementValue( "z_scale", scale_z );

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

void
CPViewJob::updateProxyMatrices()
{
    const float* t = m_proxy_transform;

    m_proxy_to_world =
            m_local_to_world *
            glm::mat4( t[0], t[1], t[2], t[3],
                       t[4], t[5], t[6], t[7],
                       t[8], t[9], t[10], t[11],
                       t[12], t[13], t[14], t[15] );

    m_proxy_to_world = glm::translate( m_proxy_to_world, glm::vec3( m_proxy_box_min[0],
                                                                    m_proxy_box_min[1],
                                                                    m_proxy_box_min[2] ) );
    m_proxy_to_world = glm::scale( m_proxy_to_world, glm::vec3( m_proxy_box_max[0]-m_proxy_box_min[0],
                                                                m_proxy_box_max[1]-m_proxy_box_min[1],
                                                                m_proxy_box_max[2]-m_proxy_box_min[2] ) );
    m_proxy_from_world = glm::inverse( m_proxy_to_world );

}

bool
CPViewJob::initGL()
{
    Logger log = getLogger( "CPViewJob.initGL" );
    if( m_grid_tess != NULL ) {
        LOGGER_ERROR( log, "initGL invoked multiple times." );
        return false;
    }
    glewInit();
    m_well_labels = new TextRenderer();
    m_clip_plane = new ClipPlane( glm::vec3( -0.1f ) , glm::vec3( 1.1f ), glm::vec4(0.f, 1.f, 0.f, 0.f ) );
    m_grid_tess = new GridTess;
    m_faults_surface = new GridTessSurf;
    m_subset_surface = new GridTessSurf;
    m_boundary_surface = new GridTessSurf;
    m_grid_tess_surf_builder = new GridTessSurfBuilder;
    m_grid_field = new GridField;
    m_tess_renderer = new GridTessSurfRenderer;
    m_all_selector = new AllSelector;
    m_field_selector = new FieldSelector;
    m_index_selector = new IndexSelector;
    m_plane_selector = new PlaneSelector;
    m_half_plane_selector = new HalfPlaneSelector;
    m_grid_tess_subset = new GridTessSubset;
    m_bbox_finder = new GridTessSurfBBoxFinder;
    m_grid_cube_renderer = new GridCubeRenderer;
    m_well_renderer = new WellRenderer();
    return true;
}



bool CPViewJob::renderFrame( const string&  session,
                             const string&  key,
                             unsigned int        fbo,
                             const size_t        width,
                             const size_t        height )
{
    Logger log = getLogger( "CPViewJob.renderFrame" );

    if( m_grid_tess == NULL ) {
        LOGGER_ERROR( log, "renderFrame invoked before initGL" );
        return false;
    }
    fetchData();
    updateModelMatrices();
    doCompute();

    tinia::policy::Viewer viewer;
    m_policy->getElementValue( "viewer", viewer );
    render( viewer.projectionMatrix.data(),
            viewer.modelviewMatrix.data(),
            fbo,
            width,
            height );





    return true;
}


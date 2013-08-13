#include "Project.hpp"
#include "models/GridStats.hpp"
#include "render/GridTess.hpp"

namespace models {
    using std::string;
    using namespace tinia::model::gui;

static const string zscale_key = "grid_z_scale";
static const string zscale_label_key = "zscale_label";
static const string grid_details_label_key = "grid_details";
static const string grid_dim_key = "_grid_dim";
static const string grid_total_cells_key = "_grid_total_cells";
static const string grid_active_cells_key = "_grid_active_cells";
static const string grid_faces_key = "_grid_faces";
static const string grid_triangles_key = "_grid_triangles";
static const string grid_max_poly_key = "_grid_max_poly";

GridStats::GridStats( std::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic )
    : m_model( model ),
      m_logic( logic ),
      m_zscale( 1.f )
{
    m_model->addElement<bool>( grid_details_label_key, true, "Grid" );
    m_model->addElement<bool>( zscale_label_key, true, "Z-scale" );
    m_model->addElement<string>( grid_dim_key, "", "Dimensions" );
    m_model->addElement<string>( grid_total_cells_key, "", "Cells" );
    m_model->addElement<string>( grid_active_cells_key, "", "Active cells" );
    m_model->addElement<string>( grid_faces_key, "", "Faces" );
    m_model->addElement<string>( grid_triangles_key, "", "Triangles" );
    m_model->addElement<string>( grid_max_poly_key, "", "Polygon max" );
    m_model->addConstrainedElement<double>( zscale_key, m_zscale, 0.01, 1000.0, "Z-scale" );
    m_model->addStateListener( zscale_key, this );

    update( );
}

GridStats::~GridStats()
{

}

void
GridStats::update( )
{
    std::shared_ptr<Project<float> > project;
    std::shared_ptr<render::GridTess> tessellation;
    update( project, tessellation );
}

void
GridStats::update( std::shared_ptr<Project<float> > project,
                   std::shared_ptr<render::GridTess> tessellation )
{
    int nx = 0;
    int ny = 0;
    int nz = 0;
    int na = 0;
    int pc = 0;
    int tc = 0;
    int mv = 0;
    if( project.get() != NULL ) {
        nx = project->nx();
        ny = project->ny();
        nz = project->nz();
    }
    if( tessellation.get() != NULL ) {
        na = tessellation->cellCount();
        pc = tessellation->polygonCount();
        tc = tessellation->polygonTriangulatedCount();
        mv = tessellation->polygonMaxPolygonSize();
    }

    std::stringstream o;
    o << "[ " << nx
      << " x " << ny
      << " x " << nz
      << " ]";
    m_model->updateElement( grid_dim_key, o.str() );

    o.str("");
    o << (nx*ny*nz);
    m_model->updateElement( grid_total_cells_key, o.str() );

    o.str("");
    o << na << " (";
    if( na == 0 ) {
        o << "n/a";
    }
    else {
        o << ((100u*na)/(nx*ny*nz) );
    }
    o << "%)";
    m_model->updateElement( grid_active_cells_key, o.str() );

    o.str("");
    o << pc << " polygons";
    m_model->updateElement( grid_faces_key, o.str() );

    o.str("");
    o << tc << " triangles";
    m_model->updateElement( grid_triangles_key, o.str() );

    o.str("");
    o << mv  << " corners";
    m_model->updateElement( grid_max_poly_key, o.str() );
}

void
GridStats::stateElementModified( tinia::model::StateElement * stateElement )
{
    const std::string& key = stateElement->getKey();
    if( key == zscale_key ) {
        double scale;
        stateElement->getValue( scale );
        m_zscale = scale;
        m_logic.doLogic();
    }
}

tinia::model::gui::Element*
GridStats::guiFactory() const
{
    tinia::model::gui::ElementGroup* root = new tinia::model::gui::ElementGroup( grid_details_label_key, true );
    tinia::model::gui::Grid* grid = new tinia::model::gui::Grid( 7, 4 );
    grid->setChild( 0, 1, new tinia::model::gui::HorizontalSpace );
    grid->setChild( 0, 3, new tinia::model::gui::HorizontalExpandingSpace );
    grid->setChild( 0, 0, new tinia::model::gui::Label( grid_dim_key ) );
    grid->setChild( 0, 2, new tinia::model::gui::Label( grid_dim_key, true ) );
    grid->setChild( 1, 0, new tinia::model::gui::Label( grid_total_cells_key ) );
    grid->setChild( 1, 2, new tinia::model::gui::Label( grid_total_cells_key, true ) );
    grid->setChild( 2, 0, new tinia::model::gui::Label( grid_active_cells_key ) );
    grid->setChild( 2, 2, new tinia::model::gui::Label( grid_active_cells_key, true ) );
    grid->setChild( 3, 0, new tinia::model::gui::Label( grid_faces_key ) );
    grid->setChild( 3, 2, new tinia::model::gui::Label( grid_faces_key, true ) );
    grid->setChild( 4, 0, new tinia::model::gui::Label( grid_triangles_key ) );
    grid->setChild( 4, 2, new tinia::model::gui::Label( grid_triangles_key, true ) );
    grid->setChild( 5, 0, new tinia::model::gui::Label( grid_max_poly_key ) );
    grid->setChild( 5, 2, new tinia::model::gui::Label( grid_max_poly_key, true ) );
    grid->setChild( 6, 0, new tinia::model::gui::Label( zscale_key ) );
    grid->setChild( 6, 2, new tinia::model::gui::DoubleSpinBox( zscale_key ) );
    root->setChild( grid );

    return root;
}


} // of namespace models
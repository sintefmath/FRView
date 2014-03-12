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

#include "dataset/CornerpointGrid.hpp"
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


GridStats::GridStats( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic )
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
    boost::shared_ptr<dataset::CornerpointGrid > project;
    boost::shared_ptr<render::GridTess> tessellation;
    update( project, tessellation );
}

void
GridStats::update(boost::shared_ptr<dataset::AbstractDataSource> project,
                   boost::shared_ptr<render::GridTess> tessellation )
{
    int na = 0;
    int nn = 0;
    int pc = 0;
    int tc = 0;
    int mv = 0;

    std::stringstream o;
    if( project ) {
        boost::shared_ptr<dataset::CellLayoutInterface> cell_layout =
                boost::dynamic_pointer_cast<dataset::CellLayoutInterface>( project );
        if( cell_layout ) {
            int n = cell_layout->maxIndex(0) - cell_layout->minIndex(0);
            nn = n;
            o << "[ " << n;
            if( cell_layout->indexDim() > 1 ) {
                int n = cell_layout->maxIndex(1) - cell_layout->minIndex(1);
                nn = nn*n;
                o << " x " << n;
                if( cell_layout->indexDim() > 2 ) {
                    int n = cell_layout->maxIndex(2) - cell_layout->minIndex(2);
                    nn = nn*n;
                    o << " x " << n;
                    if( cell_layout->indexDim() > 3 ) {
                        int n = cell_layout->maxIndex(3) - cell_layout->minIndex(3);
                        nn = nn*n;
                        o << " x " << n;
                    }
                }
            }
            o << " ]";
        }
    }
    m_model->updateElement( grid_dim_key, o.str() );

    o.str("");
    o << (nn);
    m_model->updateElement( grid_total_cells_key, o.str() );
    
    
    if( tessellation.get() != NULL ) {
        na = tessellation->cellCount();
        pc = tessellation->polygonCount();
        tc = tessellation->polygonTriangulatedCount();
        mv = tessellation->polygonMaxPolygonSize();
    }


    o.str("");
    o << na;
    if( nn != 0 ) {
        o << " (" << ((100*na)/nn ) << " %)";
    }
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

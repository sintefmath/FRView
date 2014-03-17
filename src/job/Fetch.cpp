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

#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "dataset/CornerpointGrid.hpp"
#include "dataset/PolyhedralDataInterface.hpp"
#include "job/FRViewJob.hpp"
#include "utils/Logger.hpp"
#include "ASyncReader.hpp"
#include "eclipse/EclipseReader.hpp"
#include "render/mesh/PolyhedralRepresentation.hpp"
#include "render/GridField.hpp"
#include "bridge/PolyhedralMeshBridge.hpp"
#include "render/TextRenderer.hpp"
#include "render/wells/Renderer.hpp"
#include "render/wells/Representation.hpp"
#include "render/GridField.hpp"
#include "render/ClipPlane.hpp"
#include "render/GridCubeRenderer.hpp"
#include "render/TextRenderer.hpp"
#include "render/wells/Representation.hpp"
#include "render/wells/Renderer.hpp"
#include "render/CoordSysRenderer.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/surface/GridTessSurfBuilder.hpp"
#include "render/subset/Representation.hpp"

namespace {
const std::string package = "FRViewJob";
}

void
FRViewJob::handleFetchSource()
{
    Logger log = getLogger( package + ".handleFetchSource" );
    // Try to get source
    boost::shared_ptr< dataset::AbstractDataSource > source;
    boost::shared_ptr< bridge::AbstractMeshBridge > mesh_bridge;
    if( !m_async_reader->getSource( source, mesh_bridge ) ) {
        return; // Nothing.
    }
    
    SourceItem source_item;
    source_item.m_source = source;

    boost::shared_ptr<bridge::PolyhedralMeshBridge> polyhedral_bridge =
            boost::dynamic_pointer_cast<bridge::PolyhedralMeshBridge>( mesh_bridge );
    if( polyhedral_bridge ) {
        boost::shared_ptr<render::mesh::PolyhedralRepresentation> gpu_mesh( new render::mesh::PolyhedralRepresentation );
        
        
        source_item.m_clip_plane.reset( new render::ClipPlane( glm::vec3( -0.1f ) , glm::vec3( 1.1f ), glm::vec4(0.f, 1.f, 0.f, 0.f ) ) );
        source_item.m_grid_tess = gpu_mesh;
        source_item.m_faults_surface.reset( new render::surface::GridTessSurf );
        source_item.m_subset_surface.reset( new render::surface::GridTessSurf );
        source_item.m_boundary_surface.reset( new render::surface::GridTessSurf );
        source_item.m_grid_tess_subset.reset( new render::subset::Representation );
        source_item.m_wells.reset( new render::wells::Representation );
        source_item.m_grid_field.reset(  new render::GridField( gpu_mesh ) );
        
        gpu_mesh->update( *polyhedral_bridge );
        LOGGER_DEBUG( log, "Updated grid tess" );

        addSourceItem( source_item );
        if( !m_has_pipeline ) {
            if(!setupPipeline()) {
                return;
            }
        }
    }
    
    
    LOGGER_DEBUG( log, "currentSourceItemValid()=" << currentSourceItemValid() );
    
    // update state variables
    m_do_update_subset = true;
    m_load_color_field = true;
    m_visibility_mask = models::Appearance::VISIBILITY_MASK_NONE;
    
    // --- update model variables --------------------------------------
    m_model->updateElement<bool>("has_project", true );
    
    boost::shared_ptr< dataset::PolyhedralDataInterface > polydata =
            boost::dynamic_pointer_cast< dataset::PolyhedralDataInterface >( source_item.m_source );
    
    std::list<std::string> solutions;
    int timestep_max = 0;
    if( polydata ) {
        for(unsigned int i=0; i<polydata->fields(); i++ ) {
            solutions.push_back( polydata->fieldName(i) );
        }
        timestep_max = std::max( (int)1, (int)polydata->timesteps() )-1;
    }
    
    if( solutions.empty() ) {
        solutions.push_back( "[none]" );
    }
    m_model->updateRestrictions( "field_solution", solutions.front(), solutions );
    m_model->updateRestrictions( "field_select_solution", solutions.front(), solutions );
    m_model->updateConstraints<int>("field_report_step", 0,0,  timestep_max );
    m_model->updateConstraints<int>( "field_select_report_step", 0, 0, timestep_max );
    
    int nx_min = 0;
    int nx_max = 0;
    int ny_min = 0;
    int ny_max = 0;
    int nz_min = 0;
    int nz_max = 0;
    boost::shared_ptr< dataset::CellLayoutInterface > cell_layout =
            boost::dynamic_pointer_cast< dataset::CellLayoutInterface >( source_item.m_source );
    if( cell_layout ) {
        int dim = cell_layout->indexDim();
        if( dim > 0 ) {
            nx_min = cell_layout->minIndex(0);
            nx_max = std::max( 1, cell_layout->maxIndex(0) ) - 1u;
            if( dim > 1 ) {
                ny_min = cell_layout->minIndex(1);
                ny_max = std::max( 1, cell_layout->maxIndex(1) ) - 1u;
                if( dim > 2 ) {
                    nz_min = cell_layout->minIndex(2);
                    nz_max = std::max( 1, cell_layout->maxIndex(2) ) - 1u;
                }
            }
        }
    }
    m_model->updateConstraints<int>( "index_range_select_min_i", nx_min, nx_min, nx_max );
    m_model->updateConstraints<int>( "index_range_select_max_i", nx_max, nx_min, nx_max );
    m_model->updateConstraints<int>( "index_range_select_min_j", ny_min, ny_min, ny_max );
    m_model->updateConstraints<int>( "index_range_select_max_j", ny_max, ny_min, ny_max );
    m_model->updateConstraints<int>( "index_range_select_min_k", nz_min, nz_min, nz_max );
    m_model->updateConstraints<int>( "index_range_select_max_k", nz_max, nz_min, nz_max );
    
    m_grid_stats.update( source_item.m_source, source_item.m_grid_tess );
    
    m_do_update_subset = true;
    
    if( m_renderlist_state == RENDERLIST_SENT ) {
        m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
    }
    //m_do_update_renderlist = true;
    //m_renderlist_rethink = true;

}

void
FRViewJob::handleFetchField()
{
    m_has_color_field = false;

    boost::shared_ptr<dataset::AbstractDataSource> source;
    boost::shared_ptr<bridge::FieldBridge> bridge;
    if( !m_async_reader->getField( source, bridge ) ) {
        return; // Nothing
    }
    
    for( size_t i=0; i<m_source_items.size(); i++ ) {
        
        // find the corresponding source and update
        if( m_source_items[i].m_source == source ) {
            SourceItem& source_item = m_source_items[i];
            
            if( bridge ) {
                source_item.m_grid_field->import( *bridge );
                m_has_color_field = true;
            }
            else {
                m_has_color_field = false;
            }
            m_load_color_field = false;
            
            //m_visibility_mask = models::Appearance::VISIBILITY_MASK_NONE;
            
            source_item.m_wells->clear();
            if( m_appearance.renderWells() ) {
                boost::shared_ptr< dataset::WellDataInterace > well_source =
                        boost::dynamic_pointer_cast< dataset::WellDataInterace >( source_item.m_source );
                if( well_source ) {
                    std::vector<float> colors;
                    std::vector<float> positions;
                    for( unsigned int w=0; w<well_source->wellCount(); w++ ) {
                        if( !well_source->wellDefined( m_report_step_index, w ) ) {
                            continue;
                        }
                        source_item.m_wells->addWellHead( well_source->wellName(w),
                                              well_source->wellHeadPosition( m_report_step_index, w ) );
                        
                        positions.clear();
                        colors.clear();
                        const unsigned int bN = well_source->wellBranchCount( m_report_step_index, w );
                        for( unsigned int b=0; b<bN; b++ ) {
                            const std::vector<float>& p = well_source->wellBranchPositions( m_report_step_index, w, b );
                            if( p.empty() ) {
                                continue;
                            }
                            for( size_t i=0; i<p.size(); i+=3 ) {
                                positions.push_back( p[i+0] );
                                positions.push_back( p[i+1] );
                                positions.push_back( p[i+2] );
                                colors.push_back( ((i & 0x1) == 0) ? 1.f : 0.5f );
                                colors.push_back( ((i & 0x2) == 0) ? 1.f : 0.5f );
                                colors.push_back( ((i & 0x4) == 0) ? 1.f : 0.5f );
                            }
                            source_item.m_wells->addSegments( positions, colors );
                        }
                    }
                }
            }
            updateCurrentFieldData();
            
            if( m_renderlist_state == RENDERLIST_SENT ) {
                m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
            }
            //            m_renderlist_rethink = true;
            
        }
    }
}


void
FRViewJob::fetchData()
{
    Logger log = getLogger( package + ".fetchData" );
    if( !m_has_context ) {
        return;
    }
    
    if( m_check_async_reader ) {    // replace with while 
        
        ASyncReader::ResponseType response = m_async_reader->checkForResponse();
        switch( response ) {
        case ASyncReader::RESPONSE_NONE:
            m_check_async_reader = false;
            break;
        case ASyncReader::RESPONSE_SOURCE:
            
            handleFetchSource();
            break;
        case ASyncReader::RESPONSE_FIELD:
            handleFetchField();
            break;
        }
    }
}

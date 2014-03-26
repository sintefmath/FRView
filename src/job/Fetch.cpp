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
#include "render/mesh/PolygonMeshGPUModel.hpp"
#include "render/mesh/PolyhedralMeshGPUModel.hpp"
#include "render/GridField.hpp"
#include "bridge/PolygonMeshBridge.hpp"
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
#include "models/SubsetSelector.hpp"

using boost::shared_ptr;
using boost::dynamic_pointer_cast;

using dataset::AbstractDataSource;
using bridge::AbstractMeshBridge;
using bridge::PolygonMeshBridge;
using bridge::PolyhedralMeshBridge;
using render::mesh::PolygonMeshGPUModel;
using render::mesh::PolyhedralMeshGPUModel;

namespace {
const std::string package = "FRViewJob";
}

void
FRViewJob::handleFetchSource()
{
    Logger log = getLogger( package + ".handleFetchSource" );
    // Try to get source
    shared_ptr< AbstractDataSource > source;
    shared_ptr< AbstractMeshBridge > mesh_bridge;
    if( !m_async_reader->getSource( source, mesh_bridge ) ) {
        return; // Nothing.
    }
    
    SourceItem source_item;
    source_item.m_source = source;

    shared_ptr<PolyhedralMeshBridge> polyhedral_bridge =
            dynamic_pointer_cast<PolyhedralMeshBridge>( mesh_bridge );

    shared_ptr<PolygonMeshBridge> polygon_bridge =
            dynamic_pointer_cast<PolygonMeshBridge>( mesh_bridge );

    
    if( polyhedral_bridge ) {
        
        shared_ptr<PolyhedralMeshGPUModel> gpu_polyhedronmesh( new PolyhedralMeshGPUModel );
        
        
        source_item.m_clip_plane.reset( new render::ClipPlane( glm::vec3( -0.1f ) , glm::vec3( 1.1f ), glm::vec4(0.f, 1.f, 0.f, 0.f ) ) );
        source_item.m_grid_tess = gpu_polyhedronmesh;
        source_item.m_faults_surface.reset( new render::surface::GridTessSurf );
        source_item.m_subset_surface.reset( new render::surface::GridTessSurf );
        source_item.m_boundary_surface.reset( new render::surface::GridTessSurf );
        source_item.m_grid_tess_subset.reset( new render::subset::Representation );
        source_item.m_wells.reset( new render::wells::Representation );
        source_item.m_do_update_subset = true;
        source_item.m_load_color_field = true;
        source_item.m_subset_selector_data.reset( new models::SubsetSelectorData );
        
        gpu_polyhedronmesh->update( *polyhedral_bridge );
        LOGGER_DEBUG( log, "Updated polyhedral mesh" );

        size_t index = m_source_items.size();
        m_source_items.push_back( boost::shared_ptr<SourceItem>( new SourceItem( source_item ) ) );
        setSource( index );

        if( !m_has_pipeline ) {
            if(!setupPipeline()) {
                return;
            }
        }
    }
    else if( polygon_bridge ) {
        shared_ptr<PolygonMeshGPUModel> gpu_polygonmesh( new PolygonMeshGPUModel );
        
        source_item.m_clip_plane.reset( new render::ClipPlane( glm::vec3( -0.1f ) , glm::vec3( 1.1f ), glm::vec4(0.f, 1.f, 0.f, 0.f ) ) );
        source_item.m_grid_tess = gpu_polygonmesh;
        source_item.m_faults_surface.reset( new render::surface::GridTessSurf );
        source_item.m_subset_surface.reset( new render::surface::GridTessSurf );
        source_item.m_boundary_surface.reset( new render::surface::GridTessSurf );
        source_item.m_grid_tess_subset.reset( new render::subset::Representation );
        source_item.m_wells.reset( new render::wells::Representation );
        source_item.m_do_update_subset = true;
        source_item.m_load_color_field = true;
        source_item.m_subset_selector_data.reset( new models::SubsetSelectorData );


        
        gpu_polygonmesh->update( polygon_bridge );
        LOGGER_DEBUG( log, "Updated polygon mesh" );

        size_t index = m_source_items.size();
        m_source_items.push_back( boost::shared_ptr<SourceItem>( new SourceItem( source_item ) ) );
        setSource( index );

        if( !m_has_pipeline ) {
            if(!setupPipeline()) {
                return;
            }
        }
    }
    
    std::vector<std::string> sources;
    for( size_t i=0; i<m_source_items.size(); i++ ) {
        sources.push_back( m_source_items[i]->m_source->name() );
    }
    m_source_selector.updateSources( sources );
    
    
    
    
    LOGGER_DEBUG( log, "currentSourceItemValid()=" << currentSourceItemValid() );
    
    
    
    
    // update state variables
    m_visibility_mask = models::Appearance::VISIBILITY_MASK_NONE;
    
        
    
    if( m_renderlist_state == RENDERLIST_SENT ) {
        m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
    }
    //m_do_update_renderlist = true;
    //m_renderlist_rethink = true;

}

void
FRViewJob::handleFetchField()
{
    using boost::shared_ptr;
    using boost::dynamic_pointer_cast;
    using render::GridField;
    using render::mesh::CellSetInterface;
    

    boost::shared_ptr<dataset::AbstractDataSource> source;
    boost::shared_ptr<bridge::FieldBridge> bridge;
    if( !m_async_reader->getField( source, bridge ) ) {
        return; // Nothing
    }
    
    for( size_t i=0; i<m_source_items.size(); i++ ) {
        
        // find the corresponding source and update
        if( m_source_items[i]->m_source == source ) {
            boost::shared_ptr<SourceItem> source_item = m_source_items[i];
            
            source_item->m_grid_field.reset();
            if( bridge ) {
                source_item->m_grid_field.reset(  new render::GridField( boost::dynamic_pointer_cast<render::mesh::CellSetInterface>( source_item->m_grid_tess ) ) );
                source_item->m_grid_field->import( *bridge );
            }
            
            //m_visibility_mask = models::Appearance::VISIBILITY_MASK_NONE;
            
            source_item->m_wells->clear();
            if( m_appearance.renderWells() ) {
                boost::shared_ptr< dataset::WellDataInterace > well_source =
                        boost::dynamic_pointer_cast< dataset::WellDataInterace >( source_item->m_source );
                if( well_source ) {
                    std::vector<float> colors;
                    std::vector<float> positions;
                    for( unsigned int w=0; w<well_source->wellCount(); w++ ) {
                        if( !well_source->wellDefined( m_report_step_index, w ) ) {
                            continue;
                        }
                        source_item->m_wells->addWellHead( well_source->wellName(w),
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
                            source_item->m_wells->addSegments( positions, colors );
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

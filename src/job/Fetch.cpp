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
#include "dataset/PolygonDataInterface.hpp"
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
    std::string source_file;
    shared_ptr< AbstractMeshBridge > mesh_bridge;
    if( !m_async_reader->getSource( source, source_file, mesh_bridge ) ) {
        return; // Nothing.
    }


    shared_ptr<PolyhedralMeshBridge> polyhedral_bridge =
            dynamic_pointer_cast<PolyhedralMeshBridge>( mesh_bridge );

    shared_ptr<PolygonMeshBridge> polygon_bridge =
            dynamic_pointer_cast<PolygonMeshBridge>( mesh_bridge );

    
    if( polyhedral_bridge ) {
        LOGGER_DEBUG( log, "Adding polyhedral mesh (source " << m_source_items.size() << ")." );
        
        shared_ptr<PolyhedralMeshGPUModel> gpu_polyhedronmesh( new PolyhedralMeshGPUModel );
        gpu_polyhedronmesh->update( *polyhedral_bridge );

        addSource( source, source_file, gpu_polyhedronmesh );
    }

    else if( polygon_bridge ) {
        LOGGER_DEBUG( log, "Adding polygon mesh (source " << m_source_items.size() << ")." );

        shared_ptr<PolygonMeshGPUModel> gpu_polygonmesh( new PolygonMeshGPUModel );
        gpu_polygonmesh->update( polygon_bridge );

        addSource( source, source_file, gpu_polygonmesh );
    }
    

}

void
FRViewJob::issueFieldFetch()
{
    if( currentSourceItemValid() ) {
        boost::shared_ptr<SourceItem> si = currentSourceItem();

        shared_ptr<dataset::PolyhedralDataInterface> pdi = dynamic_pointer_cast<dataset::PolyhedralDataInterface>( si->m_source );
        if( pdi ) {

            // TODO: check for mismatch with current field

            if( pdi->validFieldAtTimestep( si->m_field_current,
                                           si->m_timestep_current ) )
            {
                m_async_reader->issueFetchField( si->m_source,
                                                 si->m_field_current,
                                                 si->m_timestep_current );
            }
        }
        else {
            shared_ptr<dataset::PolygonDataInterface> polygons = dynamic_pointer_cast<dataset::PolygonDataInterface>( si->m_source );
            if( polygons ) {
                if( polygons->validFieldAtTimestep( si->m_field_current,
                                                    si->m_timestep_current ) )
                {
                    m_async_reader->issueFetchField( si->m_source,
                                                     si->m_field_current,
                                                     si->m_timestep_current );
                }
            }
        }
    }
}


void
FRViewJob::handleFetchField()
{
    using render::GridField;
    using render::mesh::CellSetInterface;
    

    shared_ptr<const dataset::AbstractDataSource> source;
    shared_ptr<bridge::FieldBridge> bridge;
    size_t field_index, timestep_index;
    if( !m_async_reader->getField( source, field_index, timestep_index, bridge ) ) {
        return; // Nothing
    }
    
    for( size_t i=0; i<m_source_items.size(); i++ ) {
        
        // --- find matching source item ---------------------------------------
        if( (m_source_items[i]->m_source == source)
                && (m_source_items[i]->m_field_current == (int)field_index )
                && (m_source_items[i]->m_timestep_current == (int)timestep_index ) )
        {
            boost::shared_ptr<SourceItem> si = m_source_items[i];

            si->m_do_update_renderlist = true;

            if( bridge ) {
                si->m_grid_field.reset(  new render::GridField( boost::dynamic_pointer_cast<render::mesh::CellSetInterface>( si->m_grid_tess ) ) );
                si->m_grid_field->import( bridge, field_index, timestep_index );
            }
            else {
                si->m_grid_field.reset();   // no data
            }

            si->m_wells->clear();
            if( m_renderconfig.renderWells() ) {
                shared_ptr<dataset::WellDataInterace> well_source =
                        dynamic_pointer_cast<dataset::WellDataInterace>( si->m_source );
                if( well_source ) {
                    std::vector<float> colors;
                    std::vector<float> positions;
                    for( unsigned int w=0; w<well_source->wellCount(); w++ ) {
                        if( !well_source->wellDefined( si->m_timestep_current, w ) ) {
                            continue;
                        }
                        si->m_wells->addWellHead( well_source->wellName(w),
                                                  well_source->wellHeadPosition( si->m_timestep_current, w ) );
                        
                        positions.clear();
                        colors.clear();
                        const unsigned int bN = well_source->wellBranchCount( si->m_timestep_current, w );
                        for( unsigned int b=0; b<bN; b++ ) {
                            const std::vector<float>& p = well_source->wellBranchPositions( si->m_timestep_current, w, b );
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
                            si->m_wells->addSegments( positions, colors );
                        }
                    }
                }

                m_subset_selector.sourceFieldHasChanged( m_source_items[i] );
            }
            
            updateCurrentFieldData();
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



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
#include <boost/shared_ptr.hpp>
#include "utils/Logger.hpp"
#include "dataset/AbstractDataSource.hpp"
#include "job/SourceItem.hpp"
#include "render/ClipPlane.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/subset/Representation.hpp"
#include "render/wells/Representation.hpp"
#include "dataset/AbstractDataSource.hpp"
#include "dataset/PolyhedralDataInterface.hpp"
#include "dataset/PolygonDataInterface.hpp"

SourceItem::SourceItem( boost::shared_ptr< dataset::AbstractDataSource > source,
                        const std::string& source_file,
                        boost::shared_ptr<render::mesh::AbstractMeshGPUModel> gpu_mesh,
                        boost::shared_ptr<render::GLTexture> color_map,
                        const  std::vector<boost::shared_ptr<SourceItem> >& sources )
    : m_source( source ),
      m_source_file( source_file ),
      m_clip_plane( new render::ClipPlane( glm::vec3( -0.1f ),
                                           glm::vec3( 1.1f ),
                                           glm::vec4(0.f, 1.f, 0.f, 0.f ) ) ),
      m_grid_tess( gpu_mesh ),
      m_grid_tess_subset( new render::subset::Representation ),
      m_faults_surface( new render::surface::GridTessSurf ),
      m_subset_surface( new render::surface::GridTessSurf ),
      m_boundary_surface( new render::surface::GridTessSurf ),
      m_wells( new render::wells::Representation ),
      m_color_map( color_map ),
      m_visibility_mask( models::AppearanceData::VISIBILITY_MASK_NONE ),
      m_load_color_field( true ),
      m_do_update_subset( true ),
      m_do_update_renderlist( true ),
      m_field_num( 0 ),
      m_field_current( 0 ),
      m_timestep_num( 0 ),
      m_timestep_current( 0 )
{
    using boost::shared_ptr;
    using boost::dynamic_pointer_cast;
    using dataset::PolyhedralDataInterface;
    using dataset::PolygonDataInterface;

    // --- extract list of files & number of timesteps -------------------------
    shared_ptr<PolyhedralDataInterface> polyhedra = dynamic_pointer_cast<PolyhedralDataInterface >( m_source );

    if( polyhedra ) {
        m_field_num = polyhedra->fields();
        for(int i=0; i<m_field_num; i++ ) {
            m_field_names.push_back( polyhedra->fieldName(i) );
        }
        m_timestep_num = (int)polyhedra->timesteps();
    }
    else {
        shared_ptr<PolygonDataInterface> polygons = dynamic_pointer_cast<PolygonDataInterface>( m_source );
        if( polygons ) {
            m_field_num = polygons->fields();
            for(int i=0; i<m_field_num; i++ ) {
                m_field_names.push_back( polygons->fieldName(i) );
            }
            m_timestep_num = (int)polygons->timesteps();
        }
    }

    if( m_field_names.empty() ) {
        m_field_names.push_back( "[none]" );
    }


    // --- make sure that the source has a unique name -------------------------
    std::string stem = m_source->name();
    if( stem.empty() ) {
        stem = "unnamed";
    }

    std::string candidate = stem;

    bool done = false;
    for(int instance=1; instance<1000 && !done; instance++) {

        done = true;
        for( size_t i=0; i<sources.size(); i++ ) {
            if( sources[i]->m_name == candidate ) {
                std::stringstream o;
                o << stem << " (" << (instance+1) << ")";
                candidate = o.str();
                done = false;
                break;
            }
        }
    }
    if( done ) {
        m_name = candidate;
    }
    else {
        Logger log = getLogger( "SourceItem.setName" );
        LOGGER_FATAL( log, "Failed to set name of source item '" << stem << "'." );
    }

}




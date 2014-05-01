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

#include "job/FRViewJob.hpp"
#include "utils/Logger.hpp"
#include "render/ClipPlane.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/subset/Representation.hpp"
#include "render/wells/Representation.hpp"
#include "dataset/AbstractDataSource.hpp"
#include "dataset/PolyhedralDataInterface.hpp"


namespace {
    using std::string;

    const std::string package = "FRViewJob.Source";

} // of anonymous namespace

void
FRViewJob::setSource( size_t index )
{
    Logger log = getLogger( package + ".setSource" );

    boost::shared_ptr<SourceItem> source_item;

    if( !m_source_items.empty() ) {
        index = std::min( m_source_items.size()-1, index );

        m_current_item = index;
        source_item = m_source_items[index];

        // --- update model variables --------------------------------------

        boost::shared_ptr< dataset::PolyhedralDataInterface > polydata =
                boost::dynamic_pointer_cast< dataset::PolyhedralDataInterface >( source_item->m_source );

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


        m_grid_stats.update( source_item->m_source, source_item->m_grid_tess );

        LOGGER_DEBUG( log, "current source is " << m_current_item << ": " << source_item->m_name )
    }
    else {
        std::list<std::string> solutions = {"none"};
        m_model->updateRestrictions( "field_solution", solutions.front(), solutions );

        m_grid_stats.update();
    }
    m_subset_selector.update( source_item );
    m_appearance.update( source_item, index );
    updateCurrentFieldData();
}


void
FRViewJob::addSource( boost::shared_ptr< dataset::AbstractDataSource > source,
                      boost::shared_ptr<render::mesh::AbstractMeshGPUModel> gpu_mesh )
{
    Logger log = getLogger( package + ".addSource" );
    SourceItem si;

    si.m_source = source;
    si.m_clip_plane.reset( new render::ClipPlane( glm::vec3( -0.1f ) , glm::vec3( 1.1f ), glm::vec4(0.f, 1.f, 0.f, 0.f ) ) );
    si.m_grid_tess = gpu_mesh;
    si.m_faults_surface.reset( new render::surface::GridTessSurf );
    si.m_subset_surface.reset( new render::surface::GridTessSurf );
    si.m_boundary_surface.reset( new render::surface::GridTessSurf );
    si.m_grid_tess_subset.reset( new render::subset::Representation );
    si.m_wells.reset( new render::wells::Representation );
    si.m_color_map = m_color_maps;
    si.setName( m_source_items );

    size_t index = m_source_items.size();
    m_source_items.push_back( boost::shared_ptr<SourceItem>( new SourceItem( si ) ) );
    setSource( index );

    // Update list of sources in exposedmodel/GUI
    std::vector<std::string> sources;
    for( size_t i=0; i<m_source_items.size(); i++ ) {
        sources.push_back( m_source_items[i]->m_name );
        LOGGER_DEBUG( log, "source " << i << ": " << sources.back() );
    }
    m_source_selector.updateSources( sources );
    m_renderlist_update_revision = true;
}

void
FRViewJob::cloneSource()
{
    if( m_current_item < m_source_items.size() ) {
        addSource( m_source_items[ m_current_item ]->m_source,
                   m_source_items[ m_current_item ]->m_grid_tess );

    }
}

void
FRViewJob::deleteSource()
{
    if( m_source_items.size() <= 1 ) {
        deleteAllSources();
    }
    else if( m_current_item < m_source_items.size() ) {
        Logger log = getLogger( package + ".deleteSource" );

        m_source_items.erase( m_source_items.begin() + m_current_item );

        std::vector<std::string> sources;
        for( size_t i=0; i<m_source_items.size(); i++ ) {
            sources.push_back( m_source_items[i]->m_name );
            LOGGER_DEBUG( log, "source " << i << ": " << sources.back() );
        }
        m_source_selector.updateSources( sources );
        setSource( m_current_item );
    }
}

void
FRViewJob::deleteAllSources()
{
    m_source_items.clear();
    std::vector<std::string> sources;
    m_source_selector.updateSources( sources );

    setSource( 0 );

    releasePipeline();
}


// Returns true if there is a valid source item
bool
FRViewJob::currentSourceItemValid() const
{
    return m_current_item < m_source_items.size();
}


boost::shared_ptr<SourceItem>
FRViewJob::currentSourceItem()
{
    if( !currentSourceItemValid() ) {
        throw std::runtime_error( "No selected item" );
    }
    return m_source_items[ m_current_item ];
}

const boost::shared_ptr<SourceItem>
FRViewJob::currentSourceItem() const
{
    if( !currentSourceItemValid() ) {
        throw std::runtime_error( "No selected item" );
    }
    return m_source_items[ m_current_item ];
}

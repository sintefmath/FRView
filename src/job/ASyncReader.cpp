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

#include "utils/Logger.hpp"
#include "ASyncReader.hpp"
#include "cornerpoint/Tessellator.hpp"
#include "dataset/PolyhedralMeshSource.hpp"
#include "eclipse/EclipseReader.hpp"
#include "utils/PerfTimer.hpp"

namespace {
    const std::string package = "ASyncReader";
    const std::string progress_description_key = "asyncreader_what";
    const std::string progress_counter_key     = "asyncreader_progress";
}

ASyncReader::ASyncReader( boost::shared_ptr<tinia::model::ExposedModel> model )
    : m_ticket_counter(1),
      m_model( model ),
      m_worker( worker, this )
{
    Logger log = getLogger( package + ".ASyncReader" );
    m_model->addElement<bool>( "asyncreader_working", false, "Loading and preprocessing" );
    m_model->addElement<std::string>( progress_description_key, "Idle" );
    m_model->addConstrainedElement<int>( progress_counter_key, 0, 0, 100, "Progress" );
    m_model->addElement<int>( "asyncreader_ticket", 0 );
}

bool
ASyncReader::issueReadProject( const std::string& file,
                               const int refine_i,
                               const int refine_j,
                               const int refine_k,
                               const bool triangulate)
{
    Logger log = getLogger( package + ".read" );
    Command cmd;
    cmd.m_type = Command::READ_PROJECT;
    cmd.m_project_file = file;
    cmd.m_refine_i = refine_i;
    cmd.m_refine_j = refine_j;
    cmd.m_refine_k = refine_k;
    cmd.m_triangulate = triangulate;
    postCommand( cmd );
    return true;
}

bool
ASyncReader::issueReadSolution( const boost::shared_ptr< dataset::Project > project,
                                const dataset::Project::Solution& solution_location )
{
    Command cmd;
    cmd.m_type = Command::READ_SOLUTION;
    cmd.m_project = project;
    cmd.m_solution_location = solution_location;
    postCommand( cmd );
    return true;
}


bool
ASyncReader::getProject( boost::shared_ptr< dataset::Project >& project,
                         boost::shared_ptr< render::GridTessBridge>&  tess_bridge )
{
    std::unique_lock<std::mutex> lock( m_rsp_queue_lock );
    for(auto it = m_rsp_queue.begin(); it!=m_rsp_queue.end(); ++it ) {
        if( it->m_type == Response::PROJECT ) {
            project = it->m_project;
            tess_bridge = it->m_project_grid;
            m_rsp_queue.erase( it );
            return true;
        }
    }
    return false;
}

bool
ASyncReader::getSolution( boost::shared_ptr< render::GridFieldBridge >& field_bridge )
{
    // We kill of all but the latest request of correct type
    bool found_any = false;
    std::list<Response> keep;
    std::unique_lock<std::mutex> lock( m_rsp_queue_lock );
    for(auto it = m_rsp_queue.begin(); it!=m_rsp_queue.end(); ++it ) {
        if( it->m_type == Response::SOLUTION ) {
            field_bridge = it->m_solution;
            found_any = true;
        }
        else {
            keep.push_back( *it );
        }
    }
    if( found_any ) {
        m_rsp_queue.swap( keep );
        return true;
    }
    else {
        return false;
    }
}



bool
ASyncReader::getCommand( Command& cmd )
{
    std::unique_lock< std::mutex > lock( m_cmd_queue_lock );
    while( m_cmd_queue.empty() ) {
        m_cmd_queue_wait.wait( lock );
    }
    cmd = m_cmd_queue.front();
    m_cmd_queue.pop_front();
    return true;
}

void
ASyncReader::handleReadProject( const Command& cmd )
{
    Logger log = getLogger( package + ".handleReadProject" );
    m_model->updateElement<bool>( "asyncreader_working", true );
    try {
        m_model->updateElement<std::string>( progress_description_key, "Indexing files..." );
        m_model->updateElement<int>( progress_counter_key, 0 );

        boost::shared_ptr< dataset::Project > project( new dataset::Project( cmd.m_project_file,
                                                                       cmd.m_refine_i,
                                                                       cmd.m_refine_j,
                                                                       cmd.m_refine_k ) );
        if( project->geometryType() == dataset::Project::GEOMETRY_CORNERPOINT_GRID ) {

            boost::shared_ptr< render::GridTessBridge > tess_bridge( new render::GridTessBridge( cmd.m_triangulate ) );
            m_field_remap = project->fieldRemap();

            project->geometry( *tess_bridge,
                               m_model,
                               progress_description_key,
                               progress_counter_key );
            
            m_model->updateElement<std::string>( progress_description_key, "Organizing data..." );
            m_model->updateElement<int>( progress_counter_key, 0 );
            tess_bridge->process();

            Response rsp;
            rsp.m_type = Response::PROJECT;
            rsp.m_project = project;
            rsp.m_project_grid = tess_bridge;
            postResponse( cmd, rsp );
        }
        else if( project->geometryType() == dataset::Project::GEOMETRY_POLYHEDRAL_MESH ) {
            boost::shared_ptr< render::GridTessBridge > tess_bridge( new render::GridTessBridge( cmd.m_triangulate ) );
            m_field_remap = project->fieldRemap();
            
            project->source()->geometry( *tess_bridge,
                                         m_model,
                                         progress_description_key,
                                         progress_counter_key );
            
            m_model->updateElement<std::string>( progress_description_key, "Organizing data..." );
            m_model->updateElement<int>( progress_counter_key, 0 );
            tess_bridge->process();
            
            Response rsp;
            rsp.m_type = Response::PROJECT;
            rsp.m_project = project;
            rsp.m_project_grid = tess_bridge;
            postResponse( cmd, rsp );
        }
        
        else {
            m_model->updateElement<std::string>( progress_description_key, "Unsupported geometry type" );
            sleep(2);
        }
    }
    catch( std::runtime_error& e ) {
        m_model->updateElement<std::string>( progress_description_key, e.what() );
        sleep(2);
    }
    m_model->updateElement<bool>( "asyncreader_working", false );
}

void
ASyncReader::handleReadSolution( const Command& cmd )
{
    Logger log = getLogger( package + ".handleReadSolution" );
    m_model->updateElement<std::string>( progress_description_key, "Reading solution..." );

    Response rsp;
    rsp.m_type = Response::SOLUTION;
    if( cmd.m_solution_location.m_reader == dataset::Project::READER_UNFORMATTED_ECLIPSE ) {

        if( !m_field_remap.empty() ) {
            rsp.m_solution.reset( new render::GridFieldBridge( m_field_remap.size() ) );

            std::vector<float> tmp( cmd.m_solution_location.m_location.m_unformatted_eclipse.m_size );
            eclipse::Reader reader( cmd.m_solution_location.m_path );
            reader.blockContent( tmp.data(),
                                 rsp.m_solution->minimum(),
                                 rsp.m_solution->maximum(),
                                 cmd.m_solution_location.m_location.m_unformatted_eclipse );
            for(size_t i=0; i<m_field_remap.size(); i++ ) {
                rsp.m_solution->values()[i] = tmp[ m_field_remap[i] ];
            }

        }
        else {
            rsp.m_solution.reset( new render::GridFieldBridge( cmd.m_solution_location.m_location.m_unformatted_eclipse.m_size ) );

            eclipse::Reader reader( cmd.m_solution_location.m_path );
            reader.blockContent( rsp.m_solution->values(),
                                 rsp.m_solution->minimum(),
                                 rsp.m_solution->maximum(),
                                 cmd.m_solution_location.m_location.m_unformatted_eclipse );
        }
    }
    else if( cmd.m_solution_location.m_reader == dataset::Project::READER_FROM_SOURCE ) {
        try {
            rsp.m_solution.reset( new render::GridFieldBridge( cmd.m_project->source()->activeCells() ) );
            cmd.m_project->source()->field( *rsp.m_solution.get(),
                                             cmd.m_solution_location.m_location.m_source_index,
                                             0 );
            rsp.m_type = Response::SOLUTION;
        }
        catch( std::runtime_error& e ) {
            LOGGER_ERROR( log, "Caught runtime error: " << e.what() );
        }
    }
    else {
        LOGGER_ERROR( log, "Unsupported solution format" );
    }
    postResponse( cmd, rsp );
}


void
ASyncReader::worker( ASyncReader* that )
{
    Logger log = getLogger( package + ".worker" );

    bool keep_going = true;
    while( keep_going ) {
        Command cmd;
        if( that->getCommand( cmd ) ) {
            switch( cmd.m_type ) {
            case Command::READ_PROJECT:
                that->handleReadProject( cmd );
                break;
            case Command::READ_SOLUTION:
                that->handleReadSolution( cmd );
                break;
            case Command::DIE:
                keep_going = false;
                break;
            }
        }
    }
    LOGGER_DEBUG( log, "weee!" );
}

void
ASyncReader::postCommand( Command& cmd , bool wipe)
{
    Logger log = getLogger( package + ".postCommand" );
    std::unique_lock<std::mutex> lock( m_cmd_queue_lock );
    cmd.m_ticket = m_ticket_counter++;
    if( wipe ) {
        for(auto it=m_cmd_queue.begin(); it!=m_cmd_queue.end(); ++it ) {
            if( it->m_type == cmd.m_type ) {
                LOGGER_DEBUG( log, "Wiped old command" );
                *it = cmd;
                return;
            }
        }
    }
    m_cmd_queue.push_back( cmd );
    m_cmd_queue_wait.notify_one();
}

void
ASyncReader::postResponse( const Command& cmd, const Response& rsp )
{
    std::unique_lock<std::mutex> lock( m_rsp_queue_lock );
    m_rsp_queue.push_back( rsp );
    lock.unlock();

    // Signal that a response is ready
    m_model->updateElement<int>( "asyncreader_ticket", cmd.m_ticket );
}

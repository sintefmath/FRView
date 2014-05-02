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
#include "bridge/PolygonMeshBridge.hpp"
#include "dataset/VTKXMLSourceFactory.hpp"
#include "dataset/CornerpointGrid.hpp"
#include "dataset/PolygonDataInterface.hpp"
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
ASyncReader::issueOpenSource( const std::string& file,
                              int refine_i,
                              int refine_j,
                              int refine_k,
                              bool triangulate)
{
    Logger log = getLogger( package + ".read" );
    Command cmd;
    cmd.m_type = COMMAND_OPEN_SOURCE;
    cmd.m_source_file = file;
    cmd.m_refine_i = refine_i;
    cmd.m_refine_j = refine_j;
    cmd.m_refine_k = refine_k;
    cmd.m_triangulate = triangulate;
    postCommand( cmd, false );
    return true;
}

bool
ASyncReader::issueFetchField(boost::shared_ptr<dataset::AbstractDataSource> source,
                              size_t                                                field_index,
                              size_t                                                timestep_index )
{
    Command cmd;
    cmd.m_type = COMMAND_FETCH_FIELD;
    cmd.m_source = source;
    cmd.m_field_index = field_index;
    cmd.m_timestep_index = timestep_index;
    postCommand( cmd );
    return true;
}

ASyncReader::ResponseType
ASyncReader::checkForResponse()
{
    std::unique_lock<std::mutex> lock( m_rsp_queue_lock );
    if( m_rsp_queue.empty() ) {
        return RESPONSE_NONE;
    }
    else {
        return m_rsp_queue.begin()->m_type;
    }
}


bool
ASyncReader::getSource( boost::shared_ptr<dataset::AbstractDataSource>&  source,
                        std::string&                                     source_file,
                        boost::shared_ptr<bridge::AbstractMeshBridge>&   mesh_bridge )
{
    std::unique_lock<std::mutex> lock( m_rsp_queue_lock );
    for(auto it = m_rsp_queue.begin(); it!=m_rsp_queue.end(); ++it ) {
        if( it->m_type == RESPONSE_SOURCE ) {
            source      = it->m_source;
            source_file = it->m_source_file;
            mesh_bridge = it->m_mesh_bridge;
            m_rsp_queue.erase( it );
            return true;
        }
    }
    return false;
}

bool
ASyncReader::getField( boost::shared_ptr<const dataset::AbstractDataSource>&  source,
                       size_t&                                                field_index,
                       size_t&                                                timestep_index,
                       boost::shared_ptr< bridge::FieldBridge >&              field_bridge )
{
    // We kill of all but the latest request of correct type
    bool found_any = false;
    std::list<Response> keep;
    std::unique_lock<std::mutex> lock( m_rsp_queue_lock );
    for(auto it = m_rsp_queue.begin(); it!=m_rsp_queue.end(); ++it ) {
        if( it->m_type == RESPONSE_FIELD ) {

            source         = it->m_source;
            field_index    = it->m_field_index;
            timestep_index = it->m_timestep_index;
            field_bridge   = it->m_field_bridge;

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
ASyncReader::handleOpenSource( const Command& cmd )
{
    Logger log = getLogger( package + ".handleReadProject" );
    m_model->updateElement<bool>( "asyncreader_working", true );
    try {
        m_model->updateElement<std::string>( progress_description_key, "Indexing files..." );
        m_model->updateElement<int>( progress_counter_key, 0 );

        if( cmd.m_source_file.empty() ) {
            throw std::runtime_error( "Empty file name" );
        }
        
        //struct stat stat_buffer;
        //stat( cmd.m_project_file.c_str(), &stat_buffer );
        //if( )
        

        // Extract suffix in upper case
        size_t dot = cmd.m_source_file.find_last_of( '.' );
        if( dot == std::string::npos ) {
            throw std::runtime_error( "Filename has no suffix" );
        }
        std::string suffix = cmd.m_source_file.substr( dot + 1u );
        for( auto it=suffix.begin(); it!=suffix.end(); ++it ) {
          *it = std::toupper( *it );
        }
        

        boost::shared_ptr<dataset::AbstractDataSource> source;
        if( suffix == "VTU" ) {
            source = dataset::VTKXMLSourceFactory::FromVTUFile( cmd.m_source_file );
            
//            source.reset( new dataset::VTKXMLSource( cmd.m_project_file ) );
        }
        else if( suffix == "GTXT" ) {
            source.reset( new dataset::CornerpointGrid( cmd.m_source_file,
                                                cmd.m_refine_i,
                                                cmd.m_refine_j,
                                                cmd.m_refine_k ) );            
        }
        else if( suffix == "GEOMETRY" ) {
            source.reset( new dataset::CornerpointGrid( cmd.m_source_file,
                                                cmd.m_refine_i,
                                                cmd.m_refine_j,
                                                cmd.m_refine_k ) );            
        }
        else if( suffix == "EGRID" ) {
            source.reset( new dataset::CornerpointGrid( cmd.m_source_file,
                                                cmd.m_refine_i,
                                                cmd.m_refine_j,
                                                cmd.m_refine_k ) );            
        }

        if( source ) {
            boost::shared_ptr<dataset::PolyhedralDataInterface> polyhedron_source =
                    boost::dynamic_pointer_cast<dataset::PolyhedralDataInterface>( source );

            boost::shared_ptr<dataset::PolygonDataInterface> polygon_source =
                    boost::dynamic_pointer_cast<dataset::PolygonDataInterface>( source );
            
            if( polyhedron_source ) {
                boost::shared_ptr< bridge::PolyhedralMeshBridge > bridge( new bridge::PolyhedralMeshBridge( cmd.m_triangulate ) );
                
                polyhedron_source->geometry( *bridge,
                                             m_model,
                                             progress_description_key,
                                             progress_counter_key );
                
                m_model->updateElement<std::string>( progress_description_key, "Organizing data..." );
                m_model->updateElement<int>( progress_counter_key, 0 );
                bridge->process();
                
                Response rsp;
                rsp.m_type = RESPONSE_SOURCE;
                rsp.m_source = source;
                rsp.m_mesh_bridge = bridge;
                postResponse( cmd, rsp );
            }
            else if( polygon_source ) {
                
                boost::shared_ptr< bridge::PolygonMeshBridge > bridge( new bridge::PolygonMeshBridge( cmd.m_triangulate ) );
                
                polygon_source->geometry( bridge,
                                          m_model,
                                          progress_description_key,
                                          progress_counter_key );
                m_model->updateElement<std::string>( progress_description_key, "Organizing data..." );
                m_model->updateElement<int>( progress_counter_key, 0 );
                bridge->process();
                
                Response rsp;
                rsp.m_type = RESPONSE_SOURCE;
                rsp.m_source = source;
                rsp.m_mesh_bridge = bridge;
                rsp.m_source_file = cmd.m_source_file;
                postResponse( cmd, rsp );
            }
            else {
                m_model->updateElement<std::string>( progress_description_key, "Unhandled source type" );
                sleep(2);
            }
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

    
    boost::shared_ptr<dataset::PolyhedralDataInterface> polydata =
            boost::dynamic_pointer_cast<dataset::PolyhedralDataInterface>( cmd.m_source );

    Response rsp;
    if( polydata != NULL ) {
        try {
            rsp.m_type = RESPONSE_FIELD;
            rsp.m_source = cmd.m_source;
            rsp.m_field_bridge.reset( new bridge::FieldBridge( ) );
            rsp.m_field_index = cmd.m_field_index;
            rsp.m_timestep_index = cmd.m_timestep_index;

            polydata->field( rsp.m_field_bridge,
                             cmd.m_field_index,
                             cmd.m_timestep_index );

            postResponse( cmd, rsp );
        }
        catch( std::exception& e ) {
            rsp.m_field_bridge.reset();
            LOGGER_ERROR( log, "Caught error: " << e.what() );
        }
    }
    else {
        LOGGER_ERROR( log, "Current data source does not support fields." );
    }
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
            case COMMAND_OPEN_SOURCE:
                that->handleOpenSource( cmd );
                break;
            case COMMAND_FETCH_FIELD:
                that->handleReadSolution( cmd );
                break;
            case COMMAND_DIE:
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

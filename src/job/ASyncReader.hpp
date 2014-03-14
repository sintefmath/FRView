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

#pragma once
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <tinia/model/ExposedModel.hpp>
#include "dataset/AbstractDataSource.hpp"
#include "bridge/PolyhedralMeshBridge.hpp"
#include "bridge/FieldBridge.hpp"

class ASyncReader
{
public:
    typedef uint    Ticket;
    static const Ticket IllegalTicket = ~0u;

    ASyncReader( boost::shared_ptr<tinia::model::ExposedModel> model );

    /** Asynchronously read files and create project object.
     *
     * creates and updates 'asyncreader_progress' which is used to notify
     * progress and that reading is finished.
     */
    bool
    issueReadProject( const std::string& file,
                      const int refine_i = 1,
                      const int refine_j = 1,
                      const int refine_k = 1,
                      const bool triangulate = false );

    bool
    issueReadSolution( const boost::shared_ptr< dataset::AbstractDataSource > project,
                       size_t solution_index, size_t report_step_index );

    bool
    getProject( boost::shared_ptr< dataset::AbstractDataSource >& project,
                boost::shared_ptr< bridge::PolyhedralMeshBridge>&  tess_bridge );


    bool
    getSolution( boost::shared_ptr< bridge::FieldBridge >& field_bridge );


protected:

    struct Command
    {
        enum {
            READ_PROJECT,
            READ_SOLUTION,
            DIE
        }                                       m_type;
        Ticket                                  m_ticket;
        std::string                             m_project_file;
        int                                     m_refine_i;
        int                                     m_refine_j;
        int                                     m_refine_k;
        bool                                    m_triangulate;
        boost::shared_ptr< dataset::AbstractDataSource >       m_project;
        size_t                                  m_field_index;
        size_t                                  m_timestep_index;
    };

    struct Response
    {
        enum {
            PROJECT,
            SOLUTION
            // Should have error as well?
        }                                       m_type;
        boost::shared_ptr< dataset::AbstractDataSource >       m_project;
        boost::shared_ptr< bridge::PolyhedralMeshBridge >       m_project_grid;
        boost::shared_ptr< bridge::FieldBridge >      m_solution;
    };

    Ticket                                      m_ticket_counter;
    boost::shared_ptr<tinia::model::ExposedModel> m_model;
    std::list<Command>                          m_cmd_queue;
    std::mutex                                  m_cmd_queue_lock;
    std::condition_variable                     m_cmd_queue_wait;

    std::list<Response>                         m_rsp_queue;
    std::mutex                                  m_rsp_queue_lock;

    std::thread                                 m_worker;

    //std::vector<int>                            m_field_remap;

    void
    handleReadProject( const Command& cmd );

    void
    handleReadSolution( const Command& cmd );

    bool
    getCommand( Command& cmd );

    void
    postCommand( Command& cmd, bool wipe = true );

    void
    postResponse( const Command& cmd, const Response& rsp );

    static
    void
    worker( ASyncReader* that );

};

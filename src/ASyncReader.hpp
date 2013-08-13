/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <tinia/model/ExposedModel.hpp>
#include "Project.hpp"
#include "GridTessBridge.hpp"
#include "GridFieldBridge.hpp"

class ASyncReader
{
public:
    typedef uint    Ticket;
    static const Ticket IllegalTicket = ~0u;

    ASyncReader( std::shared_ptr<tinia::model::ExposedModel> model );

    /** Asynchronously read files and create project object.
     *
     * creates and updates 'asyncreader_progress' which is used to notify
     * progress and that reading is finished.
     */
    bool
    issueReadProject(const std::list<std::string> &files );

    bool
    issueReadSolution( const Project<float>::Solution& solution_location );

    bool
    getProject( std::shared_ptr< Project<float> >& project,
                std::shared_ptr< GridTessBridge>&  tess_bridge );


    bool
    getSolution( std::shared_ptr< GridFieldBridge >& field_bridge );


protected:

    struct Command
    {
        enum {
            READ_PROJECT,
            READ_SOLUTION,
            DIE
        }                                       m_type;
        Ticket                                  m_ticket;
        std::list<std::string>                  m_project_files;
        Project<float>::Solution                m_solution_location;
    };

    struct Response
    {
        enum {
            PROJECT,
            SOLUTION
        }                                       m_type;
        std::shared_ptr< Project<float> >       m_project;
        std::shared_ptr< GridTessBridge >       m_project_grid;
        std::shared_ptr< GridFieldBridge >      m_solution;
    };

    Ticket                                      m_ticket_counter;
    std::shared_ptr<tinia::model::ExposedModel> m_model;
    std::list<Command>                          m_cmd_queue;
    std::mutex                                  m_cmd_queue_lock;
    std::condition_variable                     m_cmd_queue_wait;

    std::list<Response>                         m_rsp_queue;
    std::mutex                                  m_rsp_queue_lock;

    std::thread                                 m_worker;

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

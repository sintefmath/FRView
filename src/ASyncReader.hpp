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
#include "render/GridTessBridge.hpp"
#include "render/GridFieldBridge.hpp"

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
    issueReadSolution( const Project<float>::Solution& solution_location );

    bool
    getProject( boost::shared_ptr< Project<float> >& project,
                boost::shared_ptr< render::GridTessBridge>&  tess_bridge );


    bool
    getSolution( boost::shared_ptr< render::GridFieldBridge >& field_bridge );


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
        Project<float>::Solution                m_solution_location;
    };

    struct Response
    {
        enum {
            PROJECT,
            SOLUTION
        }                                       m_type;
        boost::shared_ptr< Project<float> >       m_project;
        boost::shared_ptr< render::GridTessBridge >       m_project_grid;
        boost::shared_ptr< render::GridFieldBridge >      m_solution;
    };

    Ticket                                      m_ticket_counter;
    boost::shared_ptr<tinia::model::ExposedModel> m_model;
    std::list<Command>                          m_cmd_queue;
    std::mutex                                  m_cmd_queue_lock;
    std::condition_variable                     m_cmd_queue_wait;

    std::list<Response>                         m_rsp_queue;
    std::mutex                                  m_rsp_queue_lock;

    std::thread                                 m_worker;

    std::vector<int>                            m_field_remap;

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

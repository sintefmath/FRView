/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <sys/times.h>
#include <unistd.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "siut2/dsrv/FreeglutWindow.h"

#include "Logger.hpp"
#include "Project.hpp"
#include "GridTess.hpp"
#include "GridTessSubset.hpp"
#include "GridTessSurf.hpp"
#include "GridTessSurfBuilder.hpp"
#include "GridTessSurfRenderer.hpp"
#include "GridField.hpp"
#include "CellSelector.hpp"
#include "ClipPlane.hpp"
#include "GridTessBridge.hpp"


class TestApp : public siut2::dsrv::FreeglutWindow
{
public:
    TestApp( int* argc, char** argv )
    : FreeglutWindow(),
      m_wireframe( true ),
      m_flip( false ),
      m_wells( false ),
      m_rethink( true ),
      m_magic( false ),
      m_clip_plane( NULL ),
      m_grid_tess( NULL ),
      m_grid_tess_subset( NULL ),
      m_selected_surface( NULL ),
      m_grid_tess_surf_builder( NULL ),
      m_tess_renderer( NULL ),
      m_half_plane_selector( NULL ),
      m_solution( 0u ),
      m_report_step( 0u ),
      m_rendering_quality( 3u )
    {
        Logger log = getLogger( "TestApp.TestApp" );

        struct tms start;
        times( &start );

        std::list<std::string> files;
        for(int i=1; i<*argc; i++ ) {
            files.push_back( argv[i] );
        }
        m_project = new Project<float>( files );

        glm::vec3 bb_min( -0.1f );
        glm::vec3 bb_max(  1.1f );

        setUpMixedModeContext( argc, argv, "cpview", GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH );
        FreeglutWindow::init( bb_min, bb_max );

        // create GL context
        //init( siut::simd::BSphere3f( siut::simd::Vec3f( 0.5f, 0.5f, 0.5f ), 1.0f ) );

        m_clip_plane = new ClipPlane( bb_min, bb_max, glm::vec4(0.f, 1.f, 0.f, 0.f ) );
        m_grid_tess = new GridTess;
        m_selected_surface = new GridTessSurf;
        m_boundary_surface = new GridTessSurf;
        m_grid_tess_surf_builder = new GridTessSurfBuilder;
        m_grid_field = new GridField;
        m_tess_renderer = new GridTessSurfRenderer;
        m_half_plane_selector = new HalfPlaneSelector();

        try {
            GridTessBridge bridge( *m_grid_tess );
            m_project->geometry( bridge );
        }
        catch( std::runtime_error&e ) {
            LOGGER_ERROR( log, "Failed to create geometry: " << e.what() );
        }
        //exit( EXIT_SUCCESS );

        m_grid_tess_subset = new GridTessSubset;

        fetchField();

        struct tms finish;
        times( &finish );

        long tics_per_sec = sysconf( _SC_CLK_TCK );

        double user_time = (double)(finish.tms_utime-start.tms_utime)/tics_per_sec;
        double system_time = (double)(finish.tms_stime-start.tms_stime)/tics_per_sec;


        LOGGER_DEBUG( log, "setup, utime=" << user_time <<
                           "s, st=" << system_time <<
                           "s, tot=" << (user_time+system_time ) << "s." );
        setFpsVisibility(true);
        setCoordSysVisibility(true);

    }

protected:
    bool                    m_wireframe;
    bool                    m_flip;
    bool                    m_wells;
    bool                    m_rethink;
    bool                    m_magic;
    Project<float>*         m_project;
    ClipPlane*              m_clip_plane;
    GridTess*               m_grid_tess;
    GridTessSubset*         m_grid_tess_subset;
    GridTessSurf*           m_selected_surface;
    GridTessSurf*           m_boundary_surface;
    GridTessSurfBuilder*    m_grid_tess_surf_builder;
    GridField*              m_grid_field;
    GridTessSurfRenderer*   m_tess_renderer;
    HalfPlaneSelector*      m_half_plane_selector;
    unsigned int            m_solution;
    unsigned int            m_report_step;
    unsigned int            m_rendering_quality;

    std::string
    info()
    {
        std::stringstream o;
        o << m_grid_tess->activeCells() << " active cells, ";
        if( m_solution < m_project->solutions() ) {
            o << m_project->solutionName( m_solution );
        }
        if( m_report_step < m_project->reportSteps() ) {
            o << ", report step " << m_report_step;
        }
        o << ", quality=" << m_rendering_quality;


        /*
        if( tess != NULL ) {
            o << "[" << tess->nx()
              << "x" << tess->ny()
              << "x" << tess->nz()
              << "]=" << tess->cells()
              << " cells, " << tess->activeCells()
              << " active";
        }
*/
        return o.str();
    }


    void
    keyboard(unsigned char key)
    {
        switch( key ) {
        case '8':
            m_clip_plane->rotate( glm::quat( glm::vec3( 0.1*M_PI_4, 0.f, 0.f ) ) );
            break;
        case '2':
            m_clip_plane->rotate( glm::quat( glm::vec3( -0.1*M_PI_4, 0.f, 0.f ) ) );
            break;
        case '4':
            m_clip_plane->rotate( glm::quat( glm::vec3( 0.f, 0.f, 0.1*M_PI_4 ) ) );
            break;
        case '6':
            m_clip_plane->rotate( glm::quat( glm::vec3( 0.f, 0.f, -0.1*M_PI_4 ) ) );
            break;
        case '+':
            m_clip_plane->shift( 0.01f );
            break;
        case '-':
            m_clip_plane->shift( -0.01f );
            break;
        case 'w':
            m_wireframe = !m_wireframe;
            break;
        case 'W':
            m_wells = !m_wells;
            break;
        case 'c':
            m_rendering_quality = 0;
            break;
        case 'l':
            m_rendering_quality = 1;
            break;
        case 'm':
            m_rendering_quality = 2;
            break;
        case 'h':
            m_rendering_quality = 3;
            break;
        case 'x':
            m_clip_plane->setDirection( glm::vec3( 1.f, 0.f, 0.f ) );
            break;
        case 'y':
            m_clip_plane->setDirection( glm::vec3( 0.f, 1.f, 0.f ) );
            break;
        case 'z':
            m_clip_plane->setDirection( glm::vec3( 0.f, 0.f, 1.f ) );
            break;
        case 'X':
            m_clip_plane->setDirection( glm::vec3( -1.f, 0.f, 0.f ) );
            break;
        case 'Y':
            m_clip_plane->setDirection( glm::vec3( 0.f, -1.f, 0.f ) );
            break;
        case 'Z':
            m_clip_plane->setDirection( glm::vec3( 0.f, 0.f, -1.f ) );
            break;
        case 'f':
            m_flip = !m_flip;
            break;
        case 'M':
            m_magic = !m_magic;
            break;
        }
        m_rethink = true;
    }

    void
    special(int key, int x, int y)
    {
        switch( key ) {
        case GLUT_KEY_LEFT:
            m_report_step = std::max( 1u, m_report_step ) - 1u;
            fetchField();
            break;
        case GLUT_KEY_RIGHT:
            m_report_step = std::min( std::max( 1u, m_project->reportSteps() ) -1u , m_report_step + 1u );
            fetchField();
            break;
        case GLUT_KEY_UP:
            m_solution = std::max( 1u, m_solution ) - 1u;
            fetchField();
            break;
        case GLUT_KEY_DOWN:
            m_solution = std::min( std::max( 1u, m_project->solutions() ) - 1u, m_solution + 1u );
            fetchField();
            break;
        }
    }

    void
    fetchField()
    {
        if( m_report_step >= m_project->reportSteps() ) {
            return;
        }
        if( m_solution >= m_project->solutions() ) {
            return;
        }

        try {
            GridFieldBridge bridge( *m_grid_field, *m_grid_tess );
            m_project->field( bridge, m_solution, m_report_step );
            m_rethink = true;
        }
        catch( std::runtime_error& e ) {
            Logger log = getLogger( "main.fetchField" );
            LOGGER_ERROR( log, e.what() );
        }
    }

    void
    render()
    {
        Logger log = getLogger( "main.render" );

        m_clip_plane->render( glm::value_ptr( m_viewer->getProjectionMatrix() ),
                              glm::value_ptr( m_viewer->getModelviewMatrix() ) );


        if( m_tess_renderer == NULL || m_grid_tess == NULL ) {
            return;
        }

        if( m_rethink ) {
            if( m_magic ) {
                m_grid_tess_surf_builder->buildFaultSurface( m_selected_surface, m_grid_tess, false );
                m_grid_tess_surf_builder->buildBoundarySurface( m_boundary_surface, m_grid_tess, false );
            }
            else {
                m_half_plane_selector->apply( m_grid_tess_subset,
                                              m_grid_tess,
                                              glm::value_ptr(m_clip_plane->plane()) );

                m_grid_tess_surf_builder->buildSubsetSurface( m_selected_surface,
                                                              m_grid_tess_subset,
                                                              m_grid_tess,
                                                              false );
                m_grid_tess_surf_builder->buildSubsetBoundarySurface( m_boundary_surface,
                                                                      m_grid_tess_subset,
                                                                      m_grid_tess,
                                                                      false );
            }
            m_rethink = false;
        }


        glm::vec2 size = m_viewer->getWindowSize();
        CHECK_GL;


        try {
            glBindFramebuffer( GL_FRAMEBUFFER, 0 );
            glUseProgram( 0 );
            glColor4f( 1.f, 1.f, 0.f, 1.f );
            m_clip_plane->render( glm::value_ptr( m_viewer->getProjectionMatrix() ),
                                  glm::value_ptr( m_viewer->getModelviewMatrix() ) );

            std::vector<GridTessSurfRenderer::RenderItem> items;
            items.resize( items.size() + 1 );
            items.back().m_surf = m_selected_surface;
            items.back().m_opacity = 1.0f;
            items.back().m_edge_opacity = m_wireframe ? 1.0f : 0.f;
            items.back().m_field = m_grid_field->hasData();
            items.back().m_field_log_map = false;
            items.back().m_field_min= m_grid_field->minValue();
            items.back().m_field_max= m_grid_field->maxValue();
            items.back().m_solid_color[0] = 1.0f;
            items.back().m_solid_color[1] = 0.5f;
            items.back().m_solid_color[2] = 1.0f;

            /*
            items.resize( items.size() + 1 );
            items.back().m_surf = m_boundary_surface;
            items.back().m_opacity = 0.25f;
            items.back().m_edge_opacity = m_wireframe ? 0.5f : 0.f;
            items.back().m_field = m_grid_field->hasData();
            items.back().m_field_log_map = false;
            items.back().m_field_min= m_grid_field->minValue();
            items.back().m_field_max= m_grid_field->maxValue();
            items.back().m_solid_color[0] = 0.5f;
            items.back().m_solid_color[1] = 1.0f;
            items.back().m_solid_color[2] = 0.5f;
*/
            m_tess_renderer->renderCells( 0,
                                          size[0],
                                          size[1],
                                          glm::value_ptr( m_viewer->getModelviewMatrix() ),
                                          glm::value_ptr( m_viewer->getProjectionMatrix() ),
                                          m_grid_tess,
                                          m_grid_field,
                                          items,
                                          m_rendering_quality );

        }
        catch( std::runtime_error& e ) {
            LOGGER_ERROR( log, "While rendering: " << e.what() );
        }
        glDisable( GL_CULL_FACE );

        if( m_wells ) {

            if( m_report_step < m_project->reportSteps() ) {
                const std::vector<Project<float>::Well>& wells = m_project->wells( m_report_step );

                glUseProgram( 0 );
                glMatrixMode( GL_PROJECTION );
                glLoadMatrixf( glm::value_ptr( m_viewer->getProjectionMatrix() ) );
                glMatrixMode( GL_MODELVIEW );
                glLoadMatrixf( glm::value_ptr( m_viewer->getModelviewMatrix() ) );

                glScalef( m_grid_tess->scale()[0],
                          m_grid_tess->scale()[1],
                          m_grid_tess->scale()[2] );

                glTranslatef( m_grid_tess->shift()[0],
                              m_grid_tess->shift()[1],
                              m_grid_tess->shift()[2] );

                // Pillars

#if 0
                glColor3f( 0.f, 1.f, 0.f );
                glBegin( GL_LINES );
                for(unsigned int i=0; i<m_project->m_cornerpoint_geometry.m_nx+1; i++ ) {
                    for(unsigned int j=0; j<m_project->m_cornerpoint_geometry.m_ny+1; j++ ) {

                        int cix = 6*( i  +
                                      (m_project->m_cornerpoint_geometry.m_nx+1)*(j) +
                                      (m_project->m_cornerpoint_geometry.m_nx+1)*(m_project->m_cornerpoint_geometry.m_ny+1)*(0) );
                        float x1 = m_project->m_cornerpoint_geometry.m_coord[ cix + 0 ];
                        float y1 = m_project->m_cornerpoint_geometry.m_coord[ cix + 1 ];
                        float z1 = m_project->m_cornerpoint_geometry.m_coord[ cix + 2 ];
                        float x2 = m_project->m_cornerpoint_geometry.m_coord[ cix + 3 ];
                        float y2 = m_project->m_cornerpoint_geometry.m_coord[ cix + 4 ];
                        float z2 = m_project->m_cornerpoint_geometry.m_coord[ cix + 5 ];

                        float z = 2575.f;
                        float a = (z-z1)/(z2-z1);
                        float b = 1.f-a;
                        glVertex3f( b*x1 + a*x2, b*y1 + a*y2, z );

                        z = 2600.f;
                        a = (z-z1)/(z2-z1);
                        b = 1.f-a;
                        glVertex3f( b*x1 + a*x2, b*y1 + a*y2, z );
                    }
                }
                glEnd();
#endif

                for(size_t i=0; i<wells.size(); i++ ) {
                    float p[3] = { wells[i].m_points[0],
                                   wells[i].m_points[1],
                                   m_grid_tess->minBBox()[2] - 0.1f*( m_grid_tess->maxBBox()[2] - m_grid_tess->minBBox()[2])
                                 };

                    glColor3f( 1.f, 1.f, 1.f );
                    glRasterPos3fv( p );
                    glutBitmapString( GLUT_BITMAP_8_BY_13, (const unsigned char*)(wells[i].m_name.c_str()) );

/*
                    glBegin( GL_LINES );
                    glVertex3fv( p );
                    glVertex3fv( wells[i].m_points.data() );
                    glEnd();
*/
                    glColor3f( 1.f, 0.f, 0.f );
                    glPointSize( 5.0 );
                    glBegin( GL_POINTS );
                    glVertex3fv( wells[i].m_points.data() );
                    glEnd();


                    if( wells[i].m_points.size() < 6 ) {


                    }
                    else {


                        glLineWidth( 2.f );
                        glBegin( GL_LINES );
                        for(unsigned int k=0; k<std::max((size_t)3u, wells[i].m_points.size()-3u); k+=3 ) {
                            glColor3f( 0.f, 0.f, 1.f );
                            glVertex3fv( wells[i].m_points.data() + k );
                            glColor3f( 0.f, 1.f, 1.f );
                            glVertex3fv( wells[i].m_points.data() + k + 3 );
                        }
                        glEnd();
                        glLineWidth( 1.f );
                    }
                }

            }
        }
    }

};

int
main( int argc, char** argv )
{
    initializeLoggingFramework( &argc, argv );
    TestApp app( &argc, argv );

    glutReshapeWindow( 640, 360 );

    app.run();
}

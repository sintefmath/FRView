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

#include "Logger.hpp"
#include "EclipseReader.hpp"
#include "EclipseParser.hpp"
#include "FooBarParser.hpp"
#include "ClipPlane.hpp"
#include "GridTess.hpp"
#include "GridTessBridge.hpp"
#include "GridTessRenderer.hpp"
#include "GridField.hpp"
#include "GridFieldBridge.hpp"
#include "Project.hpp"


#include "CellSelector.hpp"




#include "siut/dsrv/FreeglutWindow.hpp"
#include "siut/dsrv/DSRViewer.hpp"
#include "siut/gl_utils/GLSLtools.hpp"
#include "siut/simd/BSphere3f.hpp"

#include "siut/simd/QuatIO.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>



class TestApp : public siut::dsrv::FreeglutWindow
{
public:
    TestApp( int* argc, char** argv )
    : FreeglutWindow( argc, argv, false, argv[0] ),
      m_wireframe( true ),
      m_flip( false ),
      m_wells( false ),
      m_rethink( true ),
      m_clip_plane( glm::vec3( -0.1f ) , glm::vec3( 1.1f ), glm::vec4(0.f, 1.f, 0.f, 0.f ) ),
      m_project( NULL ),
      m_grid_tess( NULL ),
      m_tess_renderer( NULL ),
      m_solution( 0u ),
      m_report_step( 0u )
    {
        Logger log = getLogger( "TestApp.TestApp" );

        struct tms start;
        times( &start );

        std::list<std::string> files;
        for(int i=1; i<*argc; i++ ) {
            files.push_back( argv[i] );
        }
        m_project = new Project<float>( files );


        // create GL context
        init( siut::simd::BSphere3f( siut::simd::Vec3f( 0.5f, 0.5f, 0.5f ), 1.0f ) );

        m_grid_tess = new GridTess();
        m_grid_field = new GridField();
        m_tess_renderer = new GridTessRenderer();
        m_half_plane_selector = new HalfPlaneSelector();

        try {
            GridTessBridge bridge( *m_grid_tess );
            m_project->geometry( bridge );
        }
        catch( std::runtime_error&e ) {
            LOGGER_ERROR( log, "Failed to create geometry: " << e.what() );
        }
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
    bool                m_wireframe;
    bool                m_flip;
    bool                m_wells;
    bool                m_rethink;
    ClipPlane           m_clip_plane;
    Project<float>*     m_project;
    GridTess*           m_grid_tess;
    GridField*          m_grid_field;
    GridTessRenderer*   m_tess_renderer;
    HalfPlaneSelector*  m_half_plane_selector;

    unsigned int        m_solution;
    unsigned int        m_report_step;


    std::string
    info()
    {
        std::stringstream o;
        o << m_grid_tess->cellCount() << " active cells, ";
        if( m_solution < m_project->solutions() ) {
            o << m_project->solutionName( m_solution );
        }
        if( m_report_step < m_project->reportSteps() ) {
            o << ", report step " << m_report_step;
        }


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
            m_clip_plane.rotate( glm::quat( glm::vec3( 0.1*M_PI_4, 0.f, 0.f ) ) );
            break;
        case '2':
            m_clip_plane.rotate( glm::quat( glm::vec3( -0.1*M_PI_4, 0.f, 0.f ) ) );
            break;
        case '4':
            m_clip_plane.rotate( glm::quat( glm::vec3( 0.f, 0.f, 0.1*M_PI_4 ) ) );
            break;
        case '6':
            m_clip_plane.rotate( glm::quat( glm::vec3( 0.f, 0.f, -0.1*M_PI_4 ) ) );
            break;
        case '+':
            m_clip_plane.shift( 0.01f );
            break;
        case '-':
            m_clip_plane.shift( -0.01f );
            break;
        case 'w':
            m_wireframe = !m_wireframe;
            break;
        case 'W':
            m_wells = !m_wells;
            break;
        case 'x':
            m_clip_plane.setDirection( glm::vec3( 1.f, 0.f, 0.f ) );
            break;
        case 'y':
            m_clip_plane.setDirection( glm::vec3( 0.f, 1.f, 0.f ) );
            break;
        case 'z':
            m_clip_plane.setDirection( glm::vec3( 0.f, 0.f, 1.f ) );
            break;
        case 'X':
            m_clip_plane.setDirection( glm::vec3( -1.f, 0.f, 0.f ) );
            break;
        case 'Y':
            m_clip_plane.setDirection( glm::vec3( 0.f, -1.f, 0.f ) );
            break;
        case 'Z':
            m_clip_plane.setDirection( glm::vec3( 0.f, 0.f, -1.f ) );
            break;
        case 'm':
            m_tess_renderer->magic();
            break;
        case 'f':
            m_flip = !m_flip;
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

        m_clip_plane.render( viewer_->getProjectionMatrix().c_ptr(),
                             viewer_->getModelviewMatrix().c_ptr() );


        if( m_tess_renderer == NULL || m_grid_tess == NULL ) {
            return;
        }

        if( m_rethink ) {
            glm::vec4 clip_plane = m_clip_plane.plane();
            unsigned int cells = m_half_plane_selector->select( *m_grid_tess, glm::value_ptr(clip_plane));
            m_grid_tess->update( cells );
            m_rethink = false;
        }


        siut::simd::Vec2f size = viewer_->getWindowSize();
        if( m_flip ) {
            glCullFace( GL_FRONT );
        }
        else {
            glCullFace( GL_BACK );
        }
        CHECK_GL;

        glEnable( GL_CULL_FACE );
        if( m_grid_field->hasData() ) {
            m_tess_renderer->renderCells( 0u,
                                          (GLsizei)(size[0]),
                                          (GLsizei)(size[1]),
                                          viewer_->getModelviewMatrix().c_ptr(),
                                          viewer_->getProjectionMatrix().c_ptr(),
                                          *m_grid_tess,
                                          *m_grid_field,
                                          m_wireframe );
        }
        else {
            m_tess_renderer->renderCells( 0u,
                                          (GLsizei)(size[0]),
                                          (GLsizei)(size[1]),
                                          viewer_->getModelviewMatrix().c_ptr(),
                                          viewer_->getProjectionMatrix().c_ptr(),
                                          *m_grid_tess,
                                          m_wireframe );
        }

        glDisable( GL_CULL_FACE );

        if( m_wells ) {

            if( m_report_step < m_project->reportSteps() ) {
                const std::vector<Project<float>::Well>& wells = m_project->wells( m_report_step );

                glUseProgram( 0 );
                glMatrixMode( GL_PROJECTION );
                glLoadMatrixf( viewer_->getProjectionMatrix().c_ptr() );
                glMatrixMode( GL_MODELVIEW );
                glLoadMatrixf( viewer_->getModelviewMatrix().c_ptr() );

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

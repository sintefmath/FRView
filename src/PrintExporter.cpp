#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <tuple>
#include <glm/gtc/type_ptr.hpp>
#include "Logger.hpp"
#include "GridTess.hpp"
#include "PrintExporter.hpp"

PrintExporter::PrintExporter(GridTess *tess)
    : m_tess( tess )
{}

void
PrintExporter::run( const std::string& filename,
                    const glm::mat4& transform_position,
                    const glm::mat4& transform_normal )
{
    Logger log = getLogger( "PrintExporter.run" );

    m_vertices.clear();

    m_vertices.resize( m_tess->vertexCount() );
    const auto v = m_tess->vertexPositionsInHostMemory();
    for(size_t i=0; i<m_tess->vertexCount(); i++ ) {
        glm::vec4 h = transform_position * glm::make_vec4( v.data() + 4*i );
        m_vertices[i] = glm::vec3( h.x/h.w, h.y/h.w, h.z/h.w );
    }


    m_shapes.clear();
    const auto tv = m_tess->triangleVertexIndexInHostMemory();
    const auto tn = m_tess->triangleNormalIndexInHostMemory();
    const auto nd = m_tess->normalVectorsInHostMemory();
    const auto ti = m_tess->triangleCellIndexInHostMemory();
    std::map< std::tuple<uint,uint>, bool > edges;

    for(size_t i=0; i<m_tess->triangleCount(); i++ ) {
        bool flip;
        uint info;
        if( (ti[ 2*i+0 ] == ~0u) && (ti[ 2*i+1 ] != ~0u ) ) {
            flip = true;
            info = ti[2*i+1];
        }
        else if( (ti[ 2*i+0 ] != ~0u) && (ti[ 2*i+1 ] == ~0u ) ) {
            flip = false;
            info = ti[2*i+0];
        }
        else {
            continue;
        }
        const uint i0 = tv[ 3*i+0 ];
        const uint i1 = tv[ 3*i+1 ];
        const uint i2 = tv[ 3*i+2 ];

        glm::vec3 n = glm::normalize( (flip?-1.f:1.f)*glm::cross( m_vertices[i1] - m_vertices[i0],
                                                                  m_vertices[i2] - m_vertices[i0] ) );
        if( n.z < 0.f ) {
            continue;
        }



        Shape tri;
        tri.m_type = Shape::TYPE_TRIANGLE;
        tri.m_pen_size = Shape::PEN_NONE;
        tri.m_lightness = n.z > 0.f ? n.z : 0.f;
        tri.m_indices[0] = i0;
        tri.m_indices[1] = i1;
        tri.m_indices[2] = i2;
        tri.m_depth = std::min( m_vertices[i0].z, std::min( m_vertices[i1].z, m_vertices[i2].z));

//                ( m_vertices[i0].z + m_vertices[i1].z + m_vertices[i2].z )/3.0;
        m_shapes.push_back( tri );

        Shape edge;
        edge.m_type = Shape::TYPE_LINE;
        edge.m_pen_size = Shape::PEN_NORMAL;
        edge.m_lightness = 0.f;
        if( (info & 0x40000000u) != 0u ) {
            addEdge( i0, i1, Shape::PEN_NORMAL, tri.m_depth - 1e-6 );
        }
        if( (info & 0x20000000u) != 0u ) {
            addEdge( i1, i2, Shape::PEN_NORMAL, tri.m_depth - 1e-6 );
        }
        if( (info & 0x10000000u) != 0u ) {
            addEdge( i2, i0, Shape::PEN_NORMAL, tri.m_depth - 1e-6 );
        }
    }

    std::sort( m_shapes.begin(), m_shapes.end(), []( const Shape& a, const Shape& b )
    {
        return a.m_depth > b.m_depth;
    });


    std::vector<char> buffer;
    try {
        format( buffer );
    }
    catch( std::exception& e ) {
        LOGGER_ERROR( log, "Format-dependent code failed: " << e.what() );
        throw e;
    }

    std::ofstream out( filename );
    if( !out.is_open() ) {
        LOGGER_ERROR( log, "Unable to open '" << filename << "'." );
        throw std::runtime_error( "Failed to open output file: " + filename );
    }
    out.write( buffer.data(), buffer.size() );
    out.close();
}

void
PrintExporter::addEdge( uint a, uint b, PrintExporter::Shape::PenSize pen_size, float depth )
{
    Shape edge;
    edge.m_type = Shape::TYPE_LINE;
    edge.m_pen_size = pen_size;
    edge.m_lightness = 0.f;
    edge.m_indices[0] = a;
    edge.m_indices[1] = b;
    edge.m_depth = depth;//std::min( m_vertices[a].z, m_vertices[b].z )-1e-6;
    m_shapes.push_back( edge );
}

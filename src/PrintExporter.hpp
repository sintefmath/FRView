#pragma once
#include <vector>
#include <string>
#include <boost/utility.hpp>
#include <glm/glm.hpp>

class GridTess;

class PrintExporter : public boost::noncopyable
{
public:
    PrintExporter( GridTess* tess );

    void
    run( const std::string& filename,
         const glm::mat4& transform_position,
         const glm::mat4& transform_normal );

protected:
    struct Shape {
        enum {
            TYPE_POINT,
            TYPE_LINE,
            TYPE_TRIANGLE
        }       m_type;
        enum PenSize {
            PEN_NONE,
            PEN_THIN,
            PEN_NORMAL,
            PEN_THICK
        };

        PenSize m_pen_size;
        float   m_lightness;
        uint    m_indices[3];
        float   m_depth;
    };
    GridTess*                   m_tess;
    std::vector<glm::vec3>      m_vertices;
    std::vector<Shape>          m_shapes;


    virtual
    void
    format( std::vector<char>& buffer ) = 0;

private:

    void
    addEdge( uint a, uint b, Shape::PenSize pen_size, float depth );

};

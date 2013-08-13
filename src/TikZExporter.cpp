#include "TikZExporter.hpp"
#include <sstream>
#include <iomanip>

TikZExporter::TikZExporter(GridTess *tess)
    : PrintExporter( tess )
{}

void
TikZExporter::format(std::vector<char> &buffer)
{
    std::stringstream o;
    o << std::setiosflags(std::ios::fixed) << std::setprecision(6);
    for(size_t i=0; i<m_vertices.size(); i++ ) {
        o << "\\coordinate (c" << i
          << ") at (" << m_vertices[i].x
          << ", " << m_vertices[i].y
          << ");\n";
    }
    for(size_t i=0; i<m_shapes.size(); i++ ) {
        const Shape& s = m_shapes[i];
        if( s.m_type == Shape::TYPE_LINE ) {
            o << "\\draw ";
            o << "(c" << s.m_indices[0] << ") -- (c" << s.m_indices[1] << ");\n";
        }
        else if( s.m_type == Shape::TYPE_TRIANGLE ) {
//            o << "\\fill[blue!"<<int(s.m_lightness*100)<<"!white] ";
            o << "\\fill[blue!50!white,draw=blue!50!white] ";
            o << "(c" << s.m_indices[0]
              << ") -- (c" << s.m_indices[1]
              << ") -- (c" << s.m_indices[2]
              << ") -- cycle; % " << s.m_depth << "\n";
        }
    }
    std::string str = o.str();
    buffer.resize( str.size() );
    std::copy( str.begin(), str.end(), buffer.begin() );
}

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

#include <vector>
#include <GL/glew.h>
#include "ClipPlane.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace render {

ClipPlane::ClipPlane( const glm::vec3&  aabbox_min,
                      const glm::vec3&  aabbox_max,
                      const glm::vec4&  plane_eq )
    : m_tainted(true),
      m_aabbox_min( aabbox_min ),
      m_aabbox_max( aabbox_max ),
      m_shift(0.f)
{
    setBBox( aabbox_min, aabbox_max );
    m_plane_n = glm::vec3( plane_eq.x, plane_eq.y, plane_eq.z );
    m_plane_d = -glm::dot( m_aabbox_center, m_plane_n );
}

void
ClipPlane::setBBox( const glm::vec3& min, const glm::vec3& max )
{
    m_aabbox_min = min;
    m_aabbox_max = max;
    m_aabbox_center = 0.5f*( m_aabbox_min + m_aabbox_max );
    for(int i=0; i<8; i++) {
        m_aabbox_corners[i] = glm::vec3( (i&1)==0 ? m_aabbox_min.x : m_aabbox_max.x,
                                         (i&2)==0 ? m_aabbox_min.y : m_aabbox_max.y,
                                         (i&4)==0 ? m_aabbox_min.z : m_aabbox_max.z );
    }
}

void
ClipPlane::setDirection( const glm::vec3& n )
{
    m_plane_n = glm::normalize( n );
    m_plane_d = -glm::dot( m_aabbox_center, m_plane_n ) - m_shift;
    m_tainted = true;
}


void
ClipPlane::rotate( const glm::quat& r )
{
    m_plane_n = r * m_plane_n;
    m_plane_d = -glm::dot( m_aabbox_center, m_plane_n ) - m_shift;
    m_tainted = true;
}

void
ClipPlane::shift( const float delta )
{
    m_shift += delta;
    m_plane_d = -glm::dot( m_aabbox_center, m_plane_n ) - m_shift;
    m_tainted = true;
}

void
ClipPlane::setOffset( const float offset )
{
    m_shift = offset;
    m_plane_d = -glm::dot( m_aabbox_center, m_plane_n ) - m_shift;
    m_tainted = true;
}

void
ClipPlane::getLineLoop( std::vector<float>& vertices ) const
{
    bool inside[8];
    for(int i=0; i<8; i++) {
        inside[i] = glm::dot( m_aabbox_corners[i], m_plane_n ) + m_plane_d >= 0.f;
    }
    static int edges[12][2] = {
        {0,1},
        {1,3},
        {3,2},
        {2,0},
        {6,4},
        {4,5},
        {5,7},
        {7,6},
        {4,0},
        {6,2},
        {7,3},
        {5,1}
    };

    // find intersections
    std::vector<glm::vec3> isec;
    for(unsigned int i=0u; i<12u; i++ ) {
        const int a = edges[i][0];
        const int b = edges[i][1];

        if( inside[a] != inside[b] ) {
            glm::vec3 l = m_aabbox_corners[ b ] - m_aabbox_corners[ a ];
            float t = -( glm::dot( m_aabbox_corners[a], m_plane_n ) + m_plane_d)/glm::dot(l, m_plane_n );

            glm::vec3 i = m_aabbox_corners[ a ] + l*t;
            isec.push_back( glm::vec3( i.x, i.y, i.z ) );
        }
    }

    if( isec.size() > 2u ) {
        // basically bubble-sort
        for( unsigned int k=0; k<isec.size()-2u; k++ ) {
            bool swap = false;
            for(unsigned int i=0; i<isec.size()-2; i++ ) {
                glm::vec3 a = isec[i+1] - isec[0];
                glm::vec3 b = isec[i+2] - isec[0];
                glm::vec3 t = glm::cross( a, b );
                if( glm::dot( t, m_plane_n ) < 0.f ) {
                    std::swap( isec[i+1], isec[i+2] );
                    swap = true;
                }
            }
            if(!swap) {
                break;
            }
        };
    }

    vertices.resize( 3*isec.size() );
    for(size_t i=0; i<isec.size(); i++ ) {
        vertices[ 3*i + 0 ] = isec[i].x;
        vertices[ 3*i + 1 ] = isec[i].y;
        vertices[ 3*i + 2 ] = isec[i].z;
    }
}


void
ClipPlane::render( const float* projection,
                   const float* modelview )
{
/*
    bool inside[8];

    for(int i=0; i<8; i++) {
        inside[i] = glm::dot( m_aabbox_corners[i], m_plane_n ) + m_plane_d >= 0.f;
    }

    static int edges[12][2] = {
        {0,1},
        {1,3},
        {3,2},
        {2,0},
        {6,4},
        {4,5},
        {5,7},
        {7,6},
        {4,0},
        {6,2},
        {7,3},
        {5,1}
    };

    // find intersections
    std::vector<glm::vec3> isec;
    for(unsigned int i=0u; i<12u; i++ ) {
        const int a = edges[i][0];
        const int b = edges[i][1];

        if( inside[a] != inside[b] ) {
            glm::vec3 l = m_aabbox_corners[ b ] - m_aabbox_corners[ a ];
            float t = -( glm::dot( m_aabbox_corners[a], m_plane_n ) + m_plane_d)/glm::dot(l, m_plane_n );

            glm::vec3 i = m_aabbox_corners[ a ] + l*t;
            isec.push_back( glm::vec3( i.x, i.y, i.z ) );
        }
    }

    if( isec.size() > 2u ) {
        // basically bubble-sort
        for( unsigned int k=0; k<isec.size()-2u; k++ ) {
            bool swap = false;
            for(unsigned int i=0; i<isec.size()-2; i++ ) {
                glm::vec3 a = isec[i+1] - isec[0];
                glm::vec3 b = isec[i+2] - isec[0];
                glm::vec3 t = glm::cross( a, b );
                if( glm::dot( t, m_plane_n ) < 0.f ) {
                    std::swap( isec[i+1], isec[i+2] );
                    swap = true;
                }
            }
            if(!swap) {
                break;
            }
        };
    }





    glColor3f( 1.f, 1.f, 0.5f );
    glBegin( GL_LINE_LOOP );
    for(size_t i=0; i<isec.size(); i++ ) {
        glVertex3fv( glm::value_ptr( isec[i]) );
    }
    glEnd();
*/

    std::vector<float> vertices;
    getLineLoop( vertices );

    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( projection );
    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( modelview );

//    glColor3f( 1.f, 1.f, 0.5f );
    glBegin( GL_LINE_LOOP );
    for(size_t i=0; i<vertices.size(); i+=3 ) {
        glVertex3fv( vertices.data() + i );
    }
    glEnd();

}
} // of namespace render


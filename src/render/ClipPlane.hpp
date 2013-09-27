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

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace render {

class ClipPlane
{
public:
    ClipPlane( const glm::vec3&  aabbox_min,
               const glm::vec3&  aabbox_max,
               const glm::vec4&  plane );

    void
    setBBox( const glm::vec3& min, const glm::vec3& max );

    void
    rotate( const glm::quat& r );

    void
    setDirection( const glm::vec3& n );

    void
    shift( const float delta );

    void
    setOffset( const float offset );

    const glm::vec4
    plane() const { return glm::vec4( m_plane_n.x,
                                      m_plane_n.y,
                                      m_plane_n.z,
                                      m_plane_d ); }

    void
    getLineLoop( std::vector<float>& vertices ) const;

    void
    render( const float* projection,
            const float* modelview );

protected:
    bool        m_tainted;
    glm::vec3   m_aabbox_min;
    glm::vec3   m_aabbox_max;
    glm::vec3   m_aabbox_corners[8];
    glm::vec3   m_plane_n;
    float       m_plane_d;
    glm::vec3   m_aabbox_center;
    float       m_shift;


};

} // of namespace render

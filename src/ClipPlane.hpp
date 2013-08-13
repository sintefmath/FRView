/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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


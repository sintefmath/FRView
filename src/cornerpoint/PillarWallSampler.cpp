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

#include "PillarWallSampler.hpp"
#include "render/GridTessBridge.hpp"

namespace cornerpoint {

template<typename Tessellation>
template<typename SrcReal>
PillarWallSampler<Tessellation>::PillarWallSampler( const SrcReal*  coord_0,
                                                    const SrcReal*  coord_1 )
{
    m_c_x[0] = coord_0[0]; m_c_x[1] = coord_1[0]; m_c_x[2] = coord_0[3]; m_c_x[3] = coord_1[3];
    m_c_y[0] = coord_0[1]; m_c_y[1] = coord_1[1]; m_c_y[2] = coord_0[4]; m_c_y[3] = coord_1[4];
    m_c_z[0] = coord_0[2]; m_c_z[1] = coord_1[2]; m_c_z[2] = coord_0[5]; m_c_z[3] = coord_1[5];

    m_s[0] = 1.f/(m_c_z[2] - m_c_z[0]);
    m_s[1] = 1.f/(m_c_z[3] - m_c_z[1]);

    m_dl[0] = m_s[0]*(m_c_x[2]-m_c_x[0]);
    m_dl[1] = m_s[1]*(m_c_x[3]-m_c_x[1]);
    m_dl[2] = m_s[0]*(m_c_y[2]-m_c_y[0]);
    m_dl[3] = m_s[1]*(m_c_y[3]-m_c_y[1]);
}


template<typename Tessellation>
typename PillarWallSampler<Tessellation>::Real4
PillarWallSampler<Tessellation>::intersect( const Real line_a_z0,
                                            const Real line_a_z1,
                                            const Real line_b_z0,
                                            const Real line_b_z1 )
{
    Real lz[4] = {line_a_z0, line_a_z1, line_b_z0, line_b_z1 };

    Real m = 0.25*( lz[0] + lz[1] + lz[2] + lz[3] );    // center problem for better stability
    for( int i=0; i<4; i++ ) {
        lz[i] = lz[i]-m;
    }

    // find parameter value of intersection
    Real u = (lz[2] - lz[0])/( (lz[1]-lz[0]) - (lz[3]-lz[2]) );
    if( u < 0.f ) { u = 0.f; }
    if( u > 1.f ) { u = 1.f; }
    Real z = (1.f-u)*lz[0] + u*lz[1] + m;

    // position on pillars for the intersection's z-value
    const Real t[2] = { (z - m_c_z[0])/(m_c_z[2] - m_c_z[0]),
                        (z - m_c_z[1])/(m_c_z[3] - m_c_z[1]) };
    const Real px[2] = { (1.f-t[0])*m_c_x[0] + t[0]*m_c_x[2],
                         (1.f-t[1])*m_c_x[1] + t[1]*m_c_x[3] };
    const Real py[2] = { (1.f-t[0])*m_c_y[0] + t[0]*m_c_y[2],
                         (1.f-t[1])*m_c_y[1] + t[1]*m_c_y[3] };

    // Linear interpolation between positions on pillars
    const Real p[2] = { (1.f-u)*px[0] + u*px[1],
                        (1.f-u)*py[0] + u*py[1] };

    // Return position, cache u in w-coordinate
    return Real4( p[0], p[1], z, u );
}

template<typename Tessellation>
typename PillarWallSampler<Tessellation>::Real4
PillarWallSampler<Tessellation>::normal( const Real u, const Real w )
{
    const Real a[2] = { m_s[0]*(w-m_c_z[0]), m_s[1]*(w-m_c_z[1]) };
    const Real pxy[4] = { (1.f-a[0])*m_c_x[0] + a[0]*m_c_x[2],
                          (1.f-a[1])*m_c_x[1] + a[1]*m_c_x[3],
                          (1.f-a[0])*m_c_y[0] + a[0]*m_c_y[2],
                          (1.f-a[1])*m_c_y[1] + a[1]*m_c_y[3] };

    const Real dv_dv[2] = { pxy[1]-pxy[0], pxy[3]-pxy[2] };
    const Real dv_dw[2] = { m_dl[0]*(1.f-u)  +  m_dl[1]*u,
                            m_dl[2]*(1.f-u)  +  m_dl[3]*u  };
    return Real4( -dv_dv[1],
                   dv_dv[0],
                  -dv_dv[0]*dv_dw[1] + dv_dv[1]*dv_dw[0],
                   3.f );
}



template class PillarWallSampler< render::GridTessBridge >;
template PillarWallSampler< render::GridTessBridge >::PillarWallSampler( const float*, const float*);

} // of namespace cornerpoint

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

namespace cornerpoint {

template<typename Tessellation>
class PillarFloorSampler
{
public:
    typedef typename Tessellation::Real   Real;
    typedef typename Tessellation::Real4  Real4;

    template<typename SrcReal>
    PillarFloorSampler( const SrcReal*  coord_00,
                        const SrcReal*  coord_10,
                        const SrcReal*  coord_01,
                        const SrcReal*  coord_11 );

    void
    setCellFloor( const Real cell_z00,
                  const Real cell_z01,
                  const Real cell_z10,
                  const Real cell_z11 );

    Real4
    normal( const Real u, const Real v, const Real w );

protected:
    Real    m_c_x0[4];
    Real    m_c_y0[4];
    Real    m_c_z0[4];
    Real    m_c_x1[4];
    Real    m_c_y1[4];
    Real    m_c_z1[4];
    Real    m_s[4];
    Real    m_dlx[4];
    Real    m_dly[4];
    Real    m_dd[4];
};

} // of namespace cornerpoint

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
class PillarWallSampler
{
public:
    typedef typename Tessellation::Real   Real;
    typedef typename Tessellation::Real4  Real4;

    template<typename SrcReal>
    PillarWallSampler( const SrcReal*  coord_0,
                       const SrcReal*  coord_1 );

    Real4
    intersect( const Real line_a0,
               const Real line_a1,
               const Real line_b0,
               const Real line_b1 );
    Real4
    normal( const Real u, const Real w );

protected:
    Real    m_c_x[4];
    Real    m_c_y[4];
    Real    m_c_z[4];
    Real    m_s[2];
    Real    m_dl[4];
};

} // of namespace cornerpoint

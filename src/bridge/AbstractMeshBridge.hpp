#pragma once
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
#include <boost/utility.hpp>

namespace bridge {

class AbstractMeshBridge
        : public boost::noncopyable
{
public:
    /** Basic floating-point type for triangulation. */
    typedef float Real;
    
    /** Type for describing points in R^3. */
    struct Real4 {
        inline Real4() {}
        inline Real4( const Real x, const Real y, const Real z, const Real w=1.f ) { v[0]=x; v[1]=y; v[2]=z; v[3]=w; }
        inline const Real& x() const { return v[0]; }
        inline const Real& y() const { return v[1]; }
        inline const Real& z() const { return v[2]; }
        inline const Real& w() const { return v[3]; }
        inline Real& x() { return v[0]; }
        inline Real& y() { return v[1]; }
        inline Real& z() { return v[2]; }
        inline Real& w() { return v[3]; }
        Real v[4];
    } __attribute__((aligned(16)));

    
    /** Basic index type for triangulation. */
    typedef unsigned int Index;

    /** Helper struct used for polygon tessellation.
      *
      * Everything is currently encoded as an uint32, where the two upper bits
      * are used to signal the presence of a cell boundary, used for line
      * drawing. Maximum vertex index is thus restricted to 2^30-1.
      */
    struct Segment {
        inline Segment() {}
        inline Segment( Index normal, Index vertex, Index flags )
            : m_normal( normal ), m_value( vertex | (flags<<30u) )
        {}

        inline void clearEdges() { m_value = m_value & (~(3u<<30u)); }
        inline void setEdges(bool edge_a, bool edge_b ) { m_value = (m_value & (~(3u<<30u))) | (edge_a?(1u<<30u):0) | (edge_b?(1u<<31u):0); }
        inline Index normal() const { return m_normal; }
        inline Index vertex() const { return m_value & (~(3u<<30u)); }
        inline bool edgeA() const { return (m_value & (1u<<30u)) != 0u; }
        inline bool edgeB() const { return (m_value & (1u<<31u)) != 0u; }
        unsigned int    m_normal;
        unsigned int    m_value;
    };    
    
    virtual
    ~AbstractMeshBridge();


    
};


} // of namespace bridge

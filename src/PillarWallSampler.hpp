#pragma once

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

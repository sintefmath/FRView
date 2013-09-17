#include "PillarFloorSampler.hpp"
#include "render/GridTessBridge.hpp"

namespace cornerpoint {

template<typename Tessellation>
template<typename SrcReal>
PillarFloorSampler<Tessellation>::PillarFloorSampler( const SrcReal*  coord_00,
                                                      const SrcReal*  coord_10,
                                                      const SrcReal*  coord_01,
                                                      const SrcReal*  coord_11 )
{
    m_c_x0[0] = coord_00[0]; m_c_x0[1] = coord_10[0]; m_c_x0[2] = coord_01[0]; m_c_x0[3] = coord_11[0];
    m_c_y0[0] = coord_00[1]; m_c_y0[1] = coord_10[1]; m_c_y0[2] = coord_01[1]; m_c_y0[3] = coord_11[1];
    m_c_z0[0] = coord_00[2]; m_c_z0[1] = coord_10[2]; m_c_z0[2] = coord_01[2]; m_c_z0[3] = coord_11[2];
    m_c_x1[0] = coord_00[3]; m_c_x1[1] = coord_10[3]; m_c_x1[2] = coord_01[3]; m_c_x1[3] = coord_11[3];
    m_c_y1[0] = coord_00[4]; m_c_y1[1] = coord_10[4]; m_c_y1[2] = coord_01[4]; m_c_y1[3] = coord_11[4];
    m_c_z1[0] = coord_00[5]; m_c_z1[1] = coord_10[5]; m_c_z1[2] = coord_01[5]; m_c_z1[3] = coord_11[5];
    for(unsigned int i=0; i<4; i++) {
        m_s[i] = 1.f/(m_c_z1[i]-m_c_z0[i]);
        m_dlx[i] = m_s[i]*(m_c_x1[i]-m_c_x0[i]);
        m_dly[i] = m_s[i]*(m_c_y1[i]-m_c_y1[i]);
    }
}


template<typename Tessellation>
void
PillarFloorSampler<Tessellation>::setCellFloor( const Real cell_z00,
                                                const Real cell_z01,
                                                const Real cell_z10,
                                                const Real cell_z11 )
{
    m_dd[0] = cell_z10 - cell_z00;
    m_dd[1] = cell_z01 - cell_z00;
    m_dd[2] = cell_z11 - cell_z01;
    m_dd[3] = cell_z11 - cell_z10;
}

template<typename Tessellation>
typename PillarFloorSampler<Tessellation>::Real4
PillarFloorSampler<Tessellation>::normal( const Real u, const Real v, const Real w )
{

    const Real a[4] = { m_s[0]*(w-m_c_z0[0]),
                        m_s[1]*(w-m_c_z0[1]),
                        m_s[2]*(w-m_c_z0[2]),
                        m_s[3]*(w-m_c_z0[3]) };
    const Real px[4] = { (1.f-a[0])*m_c_x0[0] + a[0]*m_c_x1[0],
                         (1.f-a[1])*m_c_x0[1] + a[1]*m_c_x1[1],
                         (1.f-a[2])*m_c_x0[2] + a[2]*m_c_x1[2],
                         (1.f-a[3])*m_c_x0[3] + a[3]*m_c_x1[3] };
    const Real py[4] = { (1.f-a[0])*m_c_y0[0] + a[0]*m_c_y1[0],
                         (1.f-a[1])*m_c_y0[1] + a[1]*m_c_y1[1],
                         (1.f-a[2])*m_c_y0[2] + a[2]*m_c_y1[2],
                         (1.f-a[3])*m_c_y0[3] + a[3]*m_c_y1[3] };

    const Real dv_du[2] = { (px[1]-px[0])*(1.f-v) + (px[3]-px[2])*v,
                            (py[1]-py[0])*(1.f-v) + (py[3]-py[2])*v };
    const Real dv_dv[2] = { (px[2]-px[0])*(1.f-u) + (px[3]-px[1])*u,
                            (py[2]-py[0])*(1.f-u) + (py[3]-py[1])*u };
    const Real dv_dw[2] = { m_dlx[0]*(1.f-u)*(1.f-v)  +
                            m_dlx[1]*u*(1.f-v)  +
                            m_dlx[2]*(1.f-u)*v  +
                            m_dlx[3]*u*v,
                            m_dly[0]*(1.f-u)*(1.f-v)  +
                            m_dly[1]*u*(1.f-v)  +
                            m_dly[2]*(1.f-u)*v  +
                            m_dly[3]*u*v };
    const Real dphi_dst[2] = { m_dd[0]*(1.f-v) + m_dd[2]*v,
                               m_dd[1]*(1.f-u) + m_dd[3]*u };
    const Real vu[2] = { dv_du[0] + dv_dw[0] * dphi_dst[0],
                         dv_du[1] + dv_dw[1] * dphi_dst[0] };
    const Real vv[2] = { dv_dv[0] + dv_dw[0] * dphi_dst[1],
                         dv_dv[1] + dv_dw[1] * dphi_dst[1] };

    return Real4( vu[1]*dphi_dst[1] - dphi_dst[0]*vv[1],
                  dphi_dst[0]*vv[0] - vu[0]*dphi_dst[1],
                  vu[0]*vv[1] -        vu[1]*vv[0],
                  2.f );
}

template class PillarFloorSampler< render::GridTessBridge >;
template PillarFloorSampler< render::GridTessBridge >::PillarFloorSampler( const float*, const float*, const float*, const float*);

} // of namespace cornerpoint

#pragma once
#include <boost/shared_ptr.hpp>

namespace render {
    namespace surface {
        class GridTessSurf;
    }

struct RenderItem {

    enum {
        RENDERER_SURFACE
    }                                       m_renderer;
    boost::shared_ptr<const surface::GridTessSurf>   m_surf;
    bool                                    m_field;
    bool                                    m_field_log_map;
    float                                   m_field_min;
    float                                   m_field_max;
    float                                   m_line_thickness;
    float                                   m_edge_color[4];
    float                                   m_face_color[4];
};

} // of namespace Render

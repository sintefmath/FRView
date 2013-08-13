/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <GL/glew.h>
#include <boost/utility.hpp>
#include <vector>

namespace render {

/** Render a set of text strings specified by 3D position.
 *
 * For 2D renderering, use 2D coordinates and a orthogonal projection.
 *
 * \todo This class should be split into two classes, one with the model (text
 * layout etc.) and a view (renderer class with shaders and font textures).
 */
class TextRenderer : public boost::noncopyable
{
public:
    enum Font {
        FONT_8X12
    };

    enum Anchor {
        ANCHOR_C,
        ANCHOR_N,
        ANCHOR_NE,
        ANCHOR_E,
        ANCHOR_SE,
        ANCHOR_S,
        ANCHOR_SW,
        ANCHOR_W,
        ANCHOR_NW
    };

    TextRenderer();

    ~TextRenderer();

    /** Render the current set of text strings. */
    void
    render( GLsizei           width,
            GLsizei           height,
            const GLfloat*    modelview_projection );

    /** Remove all text strings. */
    void
    clear();

    /** Add a text string to the current set of text strings. */
    void
    add( const std::string& text,
         const Font         font,
         const GLfloat*     anchor_pos,
         const GLfloat      anchor_spacing = 0.f,
         const Anchor       anchor_type = ANCHOR_C );


private:

    struct Image {
        unsigned int    m_width;
        unsigned int    m_height;
        unsigned int    m_bbp;
        const char*     m_data;
    };
    static Image        m_font_8x12;
    GLuint              m_tex_8x12;

    GLuint              m_text_prog;

    /** Host copy of glyph data
      * 8 floats per char:
      * - anchor position x,y,z.
      * - glyph width
      * - glyph screen offset x,y
      * - glyph texture offset x,y.
      */
    std::vector<float>              m_glyphs;
    bool                            m_glyphs_taint;
    GLuint                          m_glyphs_buf;
    GLuint                          m_glyphs_tex;



    std::vector<unsigned int>        m_widths;
    std::vector<GLfloat>    m_vertex;
    std::vector<GLfloat>    m_texcoords;
    std::vector<GLfloat>    m_anchor_pos;

};

} // of namespace render

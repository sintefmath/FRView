/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <siut2/gl_utils/GLSLtools.hpp>
#include "TextRenderer.hpp"

namespace resources {
    extern const std::string text_vs;
    extern const std::string text_fs;
}

using siut2::gl_utils::compileShader;
using siut2::gl_utils::linkProgram;

TextRenderer::TextRenderer()
{
    glGenTextures( 1, &m_tex_8x12 );
    glBindTexture( GL_TEXTURE_2D, m_tex_8x12 );
    glTexImage2D( GL_TEXTURE_2D, 0,
                  GL_LUMINANCE,
                  m_font_8x12.m_width, m_font_8x12.m_height, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, m_font_8x12.m_data );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glBindTexture( GL_TEXTURE_2D, 0 );

    GLuint text_vs = compileShader( resources::text_vs, GL_VERTEX_SHADER );
    GLuint text_fs = compileShader( resources::text_fs, GL_FRAGMENT_SHADER );

    m_text_prog = glCreateProgram();
    glAttachShader( m_text_prog, text_vs );
    glAttachShader( m_text_prog, text_fs );
    linkProgram( m_text_prog );

    glDeleteShader( text_vs );
    glDeleteShader( text_fs );


    glGenBuffers( 1, &m_glyphs_buf );
    glGenTextures( 1, &m_glyphs_tex );
    m_glyphs_taint = true;


    m_widths.resize( 256 );
    for( unsigned j=0; j<12*16; j++ ) {
        for( unsigned i=0; i<8*16; i++ ) {
            if( m_font_8x12.m_data[ 3*(8*16*j + i) ] != 0 ) {
                int character = 16*(j/12)+(i/8);
                m_widths[ character ] = std::max( m_widths[character], (i%8)+1u );
            }
        }
    }

    /*
    std::string foo = "The quick brown fox jumps over the lazy dog\n0123456789.987654321\n"
            "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG";

    unsigned int pos_i = 0;
    unsigned int pos_j = 0;
    unsigned int shifts[12] = {
        0, 0,
        1, 0,
        1, 1,
        0, 0,
        1, 1,
        0, 1
    };

    for( size_t i=0; i<foo.length(); i++ ) {
        if( foo[i] == '\n' ) {
            pos_i = 0;
            pos_j += 12;
            continue;
        }
        unsigned int w = m_widths[ (unsigned int)foo[i] ];
        if( w == 0 ) {
            pos_i += 3;
            continue;
        }
        unsigned int tex_i = ((unsigned int)foo[i])%16u;
        unsigned int tex_j = ((unsigned int)foo[i])/16u;
        for( unsigned int k=0; k<6; k++) {
            m_texcoords.push_back(  8.f*(tex_i+shifts[2*k+0]) );
            m_texcoords.push_back( 12.f*(tex_j+shifts[2*k+1]) );
            m_vertex.push_back(   pos_i + 8.f*(shifts[2*k+0]) );
            m_vertex.push_back(-( pos_j + 12.f*shifts[2*k+1]) );
        }

        m_glyphs.push_back( 0.5f );
        m_glyphs.push_back( 0.5f );
        m_glyphs.push_back( 0.8f );
        m_glyphs.push_back( w );
        m_glyphs.push_back( pos_i );
        m_glyphs.push_back( pos_j );
        m_glyphs.push_back( tex_i );
        m_glyphs.push_back( tex_j );

        pos_i += w + 1;
    }
*/
}

TextRenderer::~TextRenderer()
{
    glDeleteTextures( 1, &m_tex_8x12 );
    glDeleteProgram( m_text_prog );
    glDeleteBuffers( 1, &m_glyphs_buf );
    glDeleteTextures( 1, &m_glyphs_tex );
}

void
TextRenderer::clear()
{
    m_vertex.clear();
    m_texcoords.clear();
    m_anchor_pos.clear();
    m_glyphs.clear();
    m_glyphs_taint = true;
}

void
TextRenderer::add( const std::string& text,
                   const Font         font,
                   const GLfloat*     anchor_pos,
                   const GLfloat      anchor_spacing,
                   const Anchor       anchor_type )
{

    unsigned int pos_i = 0;
    unsigned int pos_j = 0;
    unsigned int shifts[12] = {
        0, 0,
        1, 0,
        1, 1,
        0, 0,
        1, 1,
        0, 1
    };


    size_t start = m_vertex.size();

    unsigned int max_i = 0;
    unsigned int max_j = 0;
    for( size_t i=0; i<text.length(); i++ ) {
        unsigned int symbol = text[i];

        if( symbol == '\n' ) {
            pos_i = 0;
            pos_j += 12;
            continue;
        }
        unsigned int w = m_widths[ symbol ];
        if( w == 0 ) {
            pos_i += 3;
            continue;
        }

        max_i = std::max( max_i, pos_i + w );
        max_j = std::max( max_j, pos_j + 12 );

        unsigned int tex_i = (symbol)%16u;
        unsigned int tex_j = (symbol)/16u;
        for( unsigned int k=0; k<6; k++) {
            m_texcoords.push_back(  8.f*(tex_i+shifts[2*k+0]) );
            m_texcoords.push_back( 12.f*(tex_j+shifts[2*k+1]) );
            m_vertex.push_back(   pos_i + 8.f*(shifts[2*k+0]) );
            m_vertex.push_back(-( pos_j + 12.f*shifts[2*k+1]) );

            m_anchor_pos.push_back( anchor_pos[0] );
            m_anchor_pos.push_back( anchor_pos[1] );
            m_anchor_pos.push_back( anchor_pos[2] );
        }

        m_glyphs.push_back( 0.5f );
        m_glyphs.push_back( 0.5f );
        m_glyphs.push_back( 0.8f );
        m_glyphs.push_back( w );
        m_glyphs.push_back( pos_i );
        m_glyphs.push_back( pos_j );
        m_glyphs.push_back( tex_i );
        m_glyphs.push_back( tex_j );

        pos_i += w + 1;
    }

    float shift_i = 0.f;
    float shift_j = 0.f;

    switch( anchor_type ) {
    case ANCHOR_C:
        shift_i = -(max_i/2.f);
        shift_j = -(max_j/2.f);
        break;

    case ANCHOR_S:
        shift_i = -(max_i/2.f);
        shift_j = +(max_j/1.f) + anchor_spacing;
        break;
    default:
        break;

    }



    for( size_t i = start; i<m_vertex.size(); i+=2 ) {
        m_vertex[ i+0 ] += shift_i;
        m_vertex[ i+1 ] += shift_j;
    }


    m_glyphs_taint = true;
}


void
TextRenderer::render( GLsizei           width,
                      GLsizei           height,
                      const GLfloat*    modelview_projection )
{
    if( m_glyphs.empty() ) {
        return;
    }

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_tex_8x12 );

    glActiveTexture( GL_TEXTURE1 );
    if( m_glyphs_taint ) {
        m_glyphs_taint = false;

        glBindBuffer( GL_TEXTURE_BUFFER, m_glyphs_buf );
        glBufferData( GL_TEXTURE_BUFFER,
                      sizeof(GLfloat)*m_glyphs.size(),
                      m_glyphs.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );

        glBindTexture( GL_TEXTURE_BUFFER, m_glyphs_tex );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_glyphs_buf );
    }
    else {
        glBindTexture( GL_TEXTURE_BUFFER, m_glyphs_tex );
    }


    glUseProgram( m_text_prog );

    glUniformMatrix4fv( glGetUniformLocation( m_text_prog, "mvp" ),
                        1, GL_FALSE, modelview_projection );
//    glUniform3f( glGetUniformLocation( m_text_prog, "position"),
//                 0.5f, 0.5f, 0.8f );
    glUniform2f( glGetUniformLocation( m_text_prog, "size" ),
                 0.5f*width, 0.5f*height );
    glUniform2f( glGetUniformLocation( m_text_prog, "scale" ),
                 2.f/width, 2.f/height );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

    glBegin( GL_TRIANGLES );
    for( unsigned int i=0; i<m_vertex.size()/2; i++ ) {
//        glVertexAttrib3f( 2, 0.f, 0.f, 0.f );
        glVertexAttrib3f( 2, m_anchor_pos[3*i+0], m_anchor_pos[3*i+1], m_anchor_pos[3*i+2] );
        glVertexAttrib2f( 1, m_texcoords[2*i+0], m_texcoords[2*i+1] );
        glVertex2f( m_vertex[2*i+0], m_vertex[2*i+1] );
    }

    glEnd( );
    glUseProgram( 0 );
    glDisable( GL_BLEND );
    glActiveTexture( GL_TEXTURE1 ); glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_2D, 0 );
    CHECK_GL;
}


#pragma once
#include <GL/glew.h>
#include <string>
#include "GridTess.hpp"
#include <GridFieldBridge.hpp>


class GridField
{
    friend class GridFieldBridge;
public:
    GridField();


    GLuint
    texture() const
    { return m_texture; }

    const float
    minValue() const
    { return m_min_value; }

    const float
    maxValue() const
    { return m_max_value; }

    bool
    hasData() const { return m_has_data; }

protected:
    bool        m_has_data;
    GLuint      m_buffer;
    GLuint      m_texture;
    GLsizei     m_count;
    float       m_min_value;
    float       m_max_value;

    void
    import( GridFieldBridge& bridge );

};

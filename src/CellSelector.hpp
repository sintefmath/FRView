#pragma once
#include <GL/glew.h>

class GridTess;
class GridField;

class CellSelector
{
public:



protected:
    GLuint  m_prog;
    GLuint  m_vs;
    GLuint  m_query;
};


class HalfPlaneSelector : public CellSelector
{
public:
    HalfPlaneSelector();

    unsigned int
    select( GridTess& tess, const float* equation );

protected:
    GLint   m_loc_halfplane_eq;


};

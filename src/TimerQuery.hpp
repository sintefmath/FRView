#pragma once
#include <GL/glew.h>

class TimerQuery
{
public:
    TimerQuery();

    ~TimerQuery();

    void
    reset();

    double
    samples();

    double
    average();

    void
    update();

    void
    beginQuery();

    void
    endQuery();

protected:
    bool        m_has_data;
    GLuint      m_query;
    GLsizei     m_samples;
    GLuint64    m_sample_sum;

};

#include <glm/glm.hpp>
#include "TimerQuery.hpp"


TimerQuery::TimerQuery()
    : m_query(0), m_has_data( false )
{
    reset();
}

TimerQuery::~TimerQuery()
{
    glDeleteQueries( 1, &m_query );
}

void
TimerQuery::reset()
{
    m_has_data = false;
    m_samples = 0;
    m_sample_sum = 0;
}

double
TimerQuery::samples()
{
    return m_samples;
}

double
TimerQuery::average()
{
    if( m_samples == 0 ) {
        return -1.f;
    }
    else {
        return glm::floor(static_cast<double>(m_sample_sum)/static_cast<double>(m_samples));
    }
}

void
TimerQuery::update()
{
    if(!m_has_data ) {
        return;
    }
    if( m_query == 0 ) {
        glGenQueries( 1, &m_query );
    }

    GLuint64 result;
    glGetQueryObjectui64v( m_query,
                           GL_QUERY_RESULT,
                           &result );
    m_sample_sum += result;
    m_samples++;
    m_has_data = false;
}

void
TimerQuery::beginQuery()
{
    if( m_query == 0 ) {
        glGenQueries( 1, &m_query );
    }
    glBeginQuery( GL_TIME_ELAPSED, m_query );
}

void
TimerQuery::endQuery()
{
    glEndQuery( GL_TIME_ELAPSED );
    m_has_data = true;
}


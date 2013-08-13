#pragma once
#include <vector>

class CellSanityChecker
{
public:

    void
    addTriangle( unsigned int id,
                 unsigned int v0, unsigned int v1, unsigned int v2 );


    bool
    checkTopology();

protected:
    unsigned int                m_id;
    std::vector<unsigned int>   m_triangles;


};

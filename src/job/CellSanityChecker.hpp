#pragma once
#include <vector>

class CellSanityChecker
{
public:
    CellSanityChecker();

    void
    addTriangle( unsigned int id,
                 unsigned int v0, unsigned int v1, unsigned int v2 );


    void
    addPolygon( unsigned int id,
                const std::vector<unsigned int>& vertices );

    void
    addPolygonReverse( unsigned int id,
                       const std::vector<unsigned int>& vertices );

    bool
    checkTriangleTopology();

    bool
    checkPolygonTopology();

protected:
    unsigned int                m_id;
    std::vector<unsigned int>   m_triangles;

    std::vector<unsigned int>   m_polygon_offsets;
    std::vector<unsigned int>   m_polygon_vertices;


};

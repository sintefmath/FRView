#pragma once

namespace models {

class Logic
{
public:

    virtual
    void
    doLogic() = 0;

    virtual
    void
    loadFile( const std::string& filename,
              int refine_i,
              int refine_j,
              int refine_k,
              bool triangulate ) = 0;


protected:

};

} // of namespace models

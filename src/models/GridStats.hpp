/* Copyright STIFTELSEN SINTEF 2013
 * 
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <memory>
#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "models/Logic.hpp"

namespace dataset {
    class AbstractDataSource;
}
namespace render {
    namespace mesh {
        class AbstractMesh;
    }
} // of namespace render

namespace models {

class GridStats
        : public tinia::model::StateListener
{
public:
    GridStats( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic );

    ~GridStats();

    void
    stateElementModified( tinia::model::StateElement * stateElement );

    tinia::model::gui::Element*
    guiFactory() const;

    float
    zScale() const { return m_zscale; }

    void
    update( );

    void
    update( boost::shared_ptr<dataset::AbstractDataSource > project,
            boost::shared_ptr<render::mesh::AbstractMesh> tessellation );

protected:
    boost::shared_ptr<tinia::model::ExposedModel> m_model;
    Logic&                                      m_logic;
    float                                       m_zscale;
};


} // of namespace models

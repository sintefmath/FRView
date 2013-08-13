#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "models/Logic.hpp"

namespace models {

class File
        : public tinia::model::StateListener
{
public:
    File( std::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic );

    ~File();

    void
    setFileName( const std::string& filename );

    const std::string&
    titleKey() const;

    void
    stateElementModified( tinia::model::StateElement * stateElement );

    tinia::model::gui::Element*
    guiFactory() const;

protected:
    std::shared_ptr<tinia::model::ExposedModel> m_model;
    Logic&                                      m_logic;


};



} // of namespace models

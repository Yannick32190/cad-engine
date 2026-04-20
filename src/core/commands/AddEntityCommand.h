#ifndef ADDENTITYCOMMAND_H
#define ADDENTITYCOMMAND_H

#include "Command.h"
#include "../../features/sketch/SketchEntity.h"
#include "../../features/sketch/Sketch2D.h"
#include <memory>

namespace CADEngine {

/**
 * @brief Commande pour ajouter une entité au sketch
 */
class AddEntityCommand : public Command {
public:
    AddEntityCommand(std::shared_ptr<Sketch2D> sketch, 
                     std::shared_ptr<SketchEntity> entity)
        : m_sketch(sketch)
        , m_entity(entity)
    {
    }
    
    void execute() override {
        if (m_sketch && m_entity) {
            m_sketch->addEntity(m_entity);
        }
    }
    
    void undo() override {
        if (m_sketch && m_entity) {
            m_sketch->removeEntity(m_entity);
        }
    }
    
    std::string getDescription() const override {
        return "Add Entity";
    }
    
private:
    std::shared_ptr<Sketch2D> m_sketch;
    std::shared_ptr<SketchEntity> m_entity;
};

} // namespace CADEngine

#endif // ADDENTITYCOMMAND_H

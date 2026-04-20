#ifndef DELETEENTITYCOMMAND_H
#define DELETEENTITYCOMMAND_H

#include "Command.h"
#include "../../features/sketch/SketchEntity.h"
#include "../../features/sketch/Sketch2D.h"
#include <memory>

namespace CADEngine {

/**
 * @brief Commande pour supprimer une entité du sketch
 */
class DeleteEntityCommand : public Command {
public:
    DeleteEntityCommand(std::shared_ptr<Sketch2D> sketch, 
                        std::shared_ptr<SketchEntity> entity)
        : m_sketch(sketch)
        , m_entity(entity)
    {
    }
    
    void execute() override {
        if (m_sketch && m_entity) {
            m_sketch->removeEntity(m_entity);
        }
    }
    
    void undo() override {
        // Undo de suppression = rajouter
        if (m_sketch && m_entity) {
            m_sketch->addEntity(m_entity);
        }
    }
    
    std::string getDescription() const override {
        return "Delete Entity";
    }
    
private:
    std::shared_ptr<Sketch2D> m_sketch;
    std::shared_ptr<SketchEntity> m_entity;
};

} // namespace CADEngine

#endif // DELETEENTITYCOMMAND_H

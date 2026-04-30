#ifndef ADDDIMENSIONCOMMAND_H
#define ADDDIMENSIONCOMMAND_H

#include "Command.h"
#include "../../features/sketch/Dimension.h"
#include "../../features/sketch/Sketch2D.h"
#include <memory>

namespace CADEngine {

/**
 * @brief Commande pour ajouter une dimension au sketch
 */
class AddDimensionCommand : public Command {
public:
    AddDimensionCommand(std::shared_ptr<Sketch2D> sketch, 
                        std::shared_ptr<Dimension> dimension)
        : m_sketch(sketch)
        , m_dimension(dimension)
    {
    }
    
    void execute() override {
        if (m_sketch && m_dimension) {
            m_sketch->addDimension(m_dimension);
        }
    }
    
    void undo() override {
        if (m_sketch && m_dimension) {
            m_sketch->removeDimension(m_dimension);
        }
    }
    
    std::string getDescription() const override {
        return "Add Dimension";
    }
    
private:
    std::shared_ptr<Sketch2D> m_sketch;
    std::shared_ptr<Dimension> m_dimension;
};

} // namespace CADEngine

#endif // ADDDIMENSIONCOMMAND_H

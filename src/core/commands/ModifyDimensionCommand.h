#ifndef MODIFYDIMENSIONCOMMAND_H
#define MODIFYDIMENSIONCOMMAND_H

#include "Command.h"
#include "../../features/sketch/Dimension.h"
#include <memory>
#include <functional>

namespace CADEngine {

/**
 * @brief Commande pour modifier la valeur d'une dimension (undo/redo)
 */
class ModifyDimensionCommand : public Command {
public:
    ModifyDimensionCommand(std::shared_ptr<Dimension> dimension, 
                           double oldValue, 
                           double newValue,
                           std::function<void(double)> applyCallback = nullptr)
        : m_dimension(dimension)
        , m_oldValue(oldValue)
        , m_newValue(newValue)
        , m_applyCallback(applyCallback)
    {
    }
    
    void execute() override {
        if (m_dimension) {
            m_dimension->setValue(m_newValue);
            if (m_applyCallback) {
                m_applyCallback(m_newValue);
            }
        }
    }
    
    void undo() override {
        if (m_dimension) {
            m_dimension->setValue(m_oldValue);
            if (m_applyCallback) {
                m_applyCallback(m_oldValue);
            }
        }
    }
    
    std::string getDescription() const override {
        return "Modify Dimension";
    }
    
private:
    std::shared_ptr<Dimension> m_dimension;
    double m_oldValue;
    double m_newValue;
    std::function<void(double)> m_applyCallback;
};

} // namespace CADEngine

#endif // MODIFYDIMENSIONCOMMAND_H

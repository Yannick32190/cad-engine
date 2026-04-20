#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <memory>

namespace CADEngine {

/**
 * @brief Classe de base pour le système Undo/Redo (Command Pattern)
 * 
 * Chaque action utilisateur (ajouter entité, modifier dimension, etc.)
 * hérite de cette classe et implémente execute() et undo().
 */
class Command {
public:
    virtual ~Command() = default;
    
    /**
     * @brief Exécuter la commande (faire l'action)
     */
    virtual void execute() = 0;
    
    /**
     * @brief Annuler la commande (défaire l'action)
     */
    virtual void undo() = 0;
    
    /**
     * @brief Refaire la commande (après un undo)
     * Par défaut, refaire = exécuter à nouveau
     */
    virtual void redo() { execute(); }
    
    /**
     * @brief Description de la commande (pour debug/UI)
     */
    virtual std::string getDescription() const { return "Command"; }
    
protected:
    Command() = default;
};

} // namespace CADEngine

#endif // COMMAND_H

#ifndef FEATURE_H
#define FEATURE_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <TopoDS_Shape.hxx>
#include <Standard_GUID.hxx>

namespace CADEngine {

// Types de features disponibles
enum class FeatureType {
    Unknown,
    // Primitives
    Box,
    Cylinder,
    Sphere,
    Cone,
    // Sketch
    Sketch,
    // Opérations booléennes
    BooleanUnion,
    BooleanDifference,
    BooleanIntersection,
    // Modifications
    Fillet,
    Chamfer,
    // Sketch-based
    Extrude,
    Revolve,
    // Patterns
    LinearPattern,
    CircularPattern,
    // Modification de forme
    Shell
};

// Type de paramètre
enum class ParameterType {
    Double,
    Integer,
    String,
    Boolean,
    Point,
    Vector,
    Reference  // Référence vers autre feature
};

// Paramètre individuel
struct Parameter {
    std::string name;
    ParameterType type;
    
    // Valeurs possibles selon le type
    double doubleValue;
    int intValue;
    std::string stringValue;
    bool boolValue;
    
    // Constructeurs
    Parameter() : type(ParameterType::Double), doubleValue(0.0), intValue(0), boolValue(false) {}
    
    static Parameter makeDouble(const std::string& name, double value) {
        Parameter p;
        p.name = name;
        p.type = ParameterType::Double;
        p.doubleValue = value;
        return p;
    }
    
    static Parameter makeInt(const std::string& name, int value) {
        Parameter p;
        p.name = name;
        p.type = ParameterType::Integer;
        p.intValue = value;
        return p;
    }
    
    static Parameter makeString(const std::string& name, const std::string& value) {
        Parameter p;
        p.name = name;
        p.type = ParameterType::String;
        p.stringValue = value;
        return p;
    }
    
    static Parameter makeBool(const std::string& name, bool value) {
        Parameter p;
        p.name = name;
        p.type = ParameterType::Boolean;
        p.boolValue = value;
        return p;
    }
};

/**
 * @brief Classe de base pour toutes les features CAO
 * 
 * Une Feature représente une opération géométrique paramétrique :
 * - Primitives (Box, Cylinder...)
 * - Opérations (Boolean, Fillet...)
 * - Sketch-based (Extrude, Revolve...)
 * 
 * Chaque feature :
 * - A un type et un nom unique
 * - Possède des paramètres modifiables
 * - Génère un TopoDS_Shape (résultat)
 * - Peut dépendre d'autres features
 * - Peut être recalculée si ses paramètres changent
 */
class Feature {
public:
    Feature(FeatureType type, const std::string& name);
    virtual ~Feature() = default;
    
    // Identification
    FeatureType getType() const { return m_type; }
    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }
    
    // Gestion des paramètres
    void setParameter(const std::string& name, const Parameter& param);
    bool hasParameter(const std::string& name) const;
    Parameter getParameter(const std::string& name) const;
    std::map<std::string, Parameter> getParameters() const { return m_parameters; }
    
    // Raccourcis pour paramètres courants
    void setDouble(const std::string& name, double value);
    void setInt(const std::string& name, int value);
    void setString(const std::string& name, const std::string& value);
    void setBool(const std::string& name, bool value);
    
    double getDouble(const std::string& name) const;
    int getInt(const std::string& name) const;
    std::string getString(const std::string& name) const;
    bool getBool(const std::string& name) const;
    
    // Dépendances (pour arbre paramétrique)
    void addDependency(int featureId);
    void removeDependency(int featureId);
    std::vector<int> getDependencies() const { return m_dependencies; }
    bool dependsOn(int featureId) const;
    
    // Résultat géométrique
    TopoDS_Shape getShape() const { return m_shape; }
    bool isValid() const { return !m_shape.IsNull(); }
    
    // État
    bool isUpToDate() const { return m_upToDate; }
    void setUpToDate(bool state) { m_upToDate = state; }
    void invalidate() { m_upToDate = false; }
    
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    // Méthodes virtuelles à implémenter par les sous-classes
    virtual bool compute() = 0;  // Calcule la géométrie
    virtual std::string getTypeName() const = 0;
    
    // Sérialisation (pour sauvegarde/chargement)
    virtual void toJSON(std::ostream& os) const;
    virtual bool fromJSON(const std::string& json);

protected:
    // Membres protégés accessibles aux sous-classes
    FeatureType m_type;
    std::string m_name;
    int m_id;
    
    std::map<std::string, Parameter> m_parameters;
    std::vector<int> m_dependencies;
    
    TopoDS_Shape m_shape;  // Résultat géométrique
    
    bool m_upToDate;  // Le shape est-il à jour ?
    bool m_visible;   // Feature visible dans le viewer ?
    
    // Helper pour validation
    bool validateParameters() const;
};

} // namespace CADEngine

#endif // FEATURE_H

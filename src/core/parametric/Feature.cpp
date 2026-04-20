#include "Feature.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace CADEngine {

Feature::Feature(FeatureType type, const std::string& name)
    : m_type(type)
    , m_name(name)
    , m_id(-1)
    , m_upToDate(false)
    , m_visible(true)
{
}

// Gestion des paramètres

void Feature::setParameter(const std::string& name, const Parameter& param) {
    m_parameters[name] = param;
    m_upToDate = false;  // Invalide le résultat
}

bool Feature::hasParameter(const std::string& name) const {
    return m_parameters.find(name) != m_parameters.end();
}

Parameter Feature::getParameter(const std::string& name) const {
    auto it = m_parameters.find(name);
    if (it == m_parameters.end()) {
        throw std::runtime_error("Parameter '" + name + "' not found");
    }
    return it->second;
}

// Raccourcis pour paramètres courants

void Feature::setDouble(const std::string& name, double value) {
    setParameter(name, Parameter::makeDouble(name, value));
}

void Feature::setInt(const std::string& name, int value) {
    setParameter(name, Parameter::makeInt(name, value));
}

void Feature::setString(const std::string& name, const std::string& value) {
    setParameter(name, Parameter::makeString(name, value));
}

void Feature::setBool(const std::string& name, bool value) {
    setParameter(name, Parameter::makeBool(name, value));
}

double Feature::getDouble(const std::string& name) const {
    Parameter p = getParameter(name);
    if (p.type != ParameterType::Double) {
        throw std::runtime_error("Parameter '" + name + "' is not a Double");
    }
    return p.doubleValue;
}

int Feature::getInt(const std::string& name) const {
    Parameter p = getParameter(name);
    if (p.type != ParameterType::Integer) {
        throw std::runtime_error("Parameter '" + name + "' is not an Integer");
    }
    return p.intValue;
}

std::string Feature::getString(const std::string& name) const {
    Parameter p = getParameter(name);
    if (p.type != ParameterType::String) {
        throw std::runtime_error("Parameter '" + name + "' is not a String");
    }
    return p.stringValue;
}

bool Feature::getBool(const std::string& name) const {
    Parameter p = getParameter(name);
    if (p.type != ParameterType::Boolean) {
        throw std::runtime_error("Parameter '" + name + "' is not a Boolean");
    }
    return p.boolValue;
}

// Dépendances

void Feature::addDependency(int featureId) {
    if (!dependsOn(featureId)) {
        m_dependencies.push_back(featureId);
        m_upToDate = false;
    }
}

void Feature::removeDependency(int featureId) {
    auto it = std::find(m_dependencies.begin(), m_dependencies.end(), featureId);
    if (it != m_dependencies.end()) {
        m_dependencies.erase(it);
        m_upToDate = false;
    }
}

bool Feature::dependsOn(int featureId) const {
    return std::find(m_dependencies.begin(), m_dependencies.end(), featureId) 
           != m_dependencies.end();
}

// Validation

bool Feature::validateParameters() const {
    // Vérification basique - à surcharger dans les sous-classes
    return !m_parameters.empty();
}

// Sérialisation

void Feature::toJSON(std::ostream& os) const {
    os << "{\n";
    os << "  \"type\": \"" << getTypeName() << "\",\n";
    os << "  \"name\": \"" << m_name << "\",\n";
    os << "  \"id\": " << m_id << ",\n";
    os << "  \"visible\": " << (m_visible ? "true" : "false") << ",\n";
    os << "  \"parameters\": {\n";
    
    bool first = true;
    for (const auto& [name, param] : m_parameters) {
        if (!first) os << ",\n";
        first = false;
        
        os << "    \"" << name << "\": ";
        switch (param.type) {
            case ParameterType::Double:
                os << param.doubleValue;
                break;
            case ParameterType::Integer:
                os << param.intValue;
                break;
            case ParameterType::String:
                os << "\"" << param.stringValue << "\"";
                break;
            case ParameterType::Boolean:
                os << (param.boolValue ? "true" : "false");
                break;
            default:
                os << "null";
        }
    }
    
    os << "\n  },\n";
    os << "  \"dependencies\": [";
    for (size_t i = 0; i < m_dependencies.size(); ++i) {
        if (i > 0) os << ", ";
        os << m_dependencies[i];
    }
    os << "]\n";
    os << "}";
}

bool Feature::fromJSON(const std::string& json) {
    // TODO: Implémenter parsing JSON complet
    // Pour l'instant, juste un placeholder
    return false;
}

} // namespace CADEngine

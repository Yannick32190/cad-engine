/**
 * @file ViewportScaling.h
 * @brief Système centralisé de scaling adaptatif au zoom
 * 
 * Toutes les tailles (snap, texte, flèches, grilles) s'adaptent automatiquement
 * au niveau de zoom actuel pour maintenir une expérience utilisateur constante.
 */

#ifndef VIEWPORT_SCALING_H
#define VIEWPORT_SCALING_H

namespace CADEngine {

/**
 * @brief Classe utilitaire pour calculer les tailles adaptatives
 */
class ViewportScaling {
public:
    /**
     * @brief Calcule la grille de snap adaptative
     * @param zoomLevel Niveau de zoom actuel (m_sketch2DZoom)
     * @return Taille de grille en mm — toujours nombres entiers ou sous-multiples propres
     */
    static double getSnapGridSize(double zoomLevel) {
        // Séquence stricte : que des valeurs qui donnent des positions entières en mm
        if (zoomLevel < 8)   return 0.25;
        if (zoomLevel < 20)  return 0.5;
        if (zoomLevel < 50)  return 1.0;
        if (zoomLevel < 120) return 2.0;
        if (zoomLevel < 300) return 5.0;
        if (zoomLevel < 600) return 10.0;
        if (zoomLevel < 1500) return 25.0;
        return 50.0;
    }
    
    /**
     * @brief Calcule la tolérance de snap aux entités
     * @param zoomLevel Niveau de zoom actuel
     * @return Tolérance en mm (toujours ~15px à l'écran)
     */
    static double getSnapTolerance(double zoomLevel) {
        return zoomLevel * 0.05;  // 5% du zoom = ~15px constant
    }
    
    /**
     * @brief Calcule la tolérance de clic/sélection
     * @param zoomLevel Niveau de zoom actuel
     * @return Tolérance en mm (toujours ~10px à l'écran)
     */
    static double getClickTolerance(double zoomLevel) {
        return zoomLevel * 0.03;  // 3% du zoom = ~10px constant
    }
    
    /**
     * @brief Calcule la taille de texte pour les cotations
     * @param zoomLevel Niveau de zoom actuel
     * @return Taille de texte en mm
     */
    static double getDimensionTextSize(double zoomLevel) {
        // Texte adaptatif : entre 1mm (zoom max) et 30mm (vue ensemble)
        double size = zoomLevel * 0.15;  // 15% du zoom
        if (size < 1.0) size = 1.0;      // Min 1mm (lisible au zoom max)
        if (size > 30.0) size = 30.0;    // Max 30mm
        return size;
    }
    
    /**
     * @brief Calcule la taille des flèches de cotation
     * @param zoomLevel Niveau de zoom actuel
     * @return Taille de flèche en mm
     */
    static double getDimensionArrowSize(double zoomLevel) {
        // Flèches fines - 8% du zoom
        double size = zoomLevel * 0.08;  // 8% pour être petit mais visible
        if (size < 0.15) size = 0.15;    // Min 0.15mm (3× plus petit qu'avant)
        if (size > 15.0) size = 15.0;    // Max 15mm
        return size;
    }
    
    /**
     * @brief Calcule l'offset par défaut des cotations
     * @param zoomLevel Niveau de zoom actuel
     * @return Offset en mm
     */
    static double getDimensionDefaultOffset(double zoomLevel) {
        // Offset adaptatif : entre 3mm et 100mm
        double offset = zoomLevel * 0.3;  // 30% du zoom
        if (offset < 3.0) offset = 3.0;
        if (offset > 100.0) offset = 100.0;
        return offset;
    }
    
    /**
     * @brief Calcule l'espacement de grille pour l'affichage
     * @param zoomLevel Niveau de zoom actuel
     * @return Espacement en mm — TOUJOURS diviseur entier de 1000mm
     * 
     * Les lignes de grille tombent TOUJOURS sur des coordonnées entières (en mm).
     * Séquence : 1, 2, 5, 10, 25, 50, 100, 250, 500
     */
    static double getGridDisplayStep(double zoomLevel) {
        if (zoomLevel < 20)  return 1.0;
        if (zoomLevel < 50)  return 2.0;
        if (zoomLevel < 120) return 5.0;
        if (zoomLevel < 300) return 10.0;
        if (zoomLevel < 600) return 25.0;
        if (zoomLevel < 1500) return 50.0;
        if (zoomLevel < 3000) return 100.0;
        return 250.0;
    }
    
    /**
     * @brief Calcule la taille de grille pour l'affichage
     * @param zoomLevel Niveau de zoom actuel
     * @return Taille totale en mm
     */
    static int getGridDisplaySize(double zoomLevel) {
        return static_cast<int>(zoomLevel * 3);  // 3× la zone visible
    }
    
    /**
     * @brief Calcule la vitesse de pan
     * @param zoomLevel Niveau de zoom actuel
     * @return Facteur de vitesse
     */
    static float getPanSpeed(double zoomLevel) {
        return static_cast<float>(zoomLevel / 300.0f);  // Référence 300mm
    }
    
    /**
     * @brief Affiche les niveaux de précision pour debug
     */
    static const char* getPrecisionLevel(double zoomLevel) {
        if (zoomLevel < 8) return "Ultra-Précision (0.25mm)";
        if (zoomLevel < 20) return "Très Haute Précision (0.5mm)";
        if (zoomLevel < 50) return "Haute Précision (1mm)";
        if (zoomLevel < 120) return "Précision Fine (2mm)";
        if (zoomLevel < 300) return "Standard (5mm)";
        if (zoomLevel < 600) return "Vue Normale (10mm)";
        if (zoomLevel < 1500) return "Vue Standard (25mm)";
        return "Vue d'Ensemble (50mm)";
    }
};

} // namespace CADEngine

#endif // VIEWPORT_SCALING_H

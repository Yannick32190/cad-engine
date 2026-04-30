#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QApplication>
#include <QPalette>
#include <QString>
#include <QStyle>
#include <QStyleFactory>

/**
 * @brief Gestionnaire de thèmes visuels pour l'application
 */
class ThemeManager {
public:
    enum Theme { Light, Dark, BlueDark, Graphite };
    
    static Theme currentTheme() { return s_current; }
    
    static void applyTheme(Theme theme, QApplication* app) {
        s_current = theme;
        
        switch (theme) {
            case Light:   applyLight(app); break;
            case Dark:    applyDark(app); break;
            case BlueDark: applyBlueDark(app); break;
            case Graphite: applyGraphite(app); break;
        }
    }
    
    static bool isDark() { return s_current != Light; }
    
private:
    static inline Theme s_current = Light;
    
    static void applyLight(QApplication* app) {
        app->setStyle(QStyleFactory::create("Fusion"));
        app->setStyleSheet(R"(
            QMainWindow { background: #f0f1f3; }
            QToolBar { 
                background: #e8eaed; 
                border-bottom: 1px solid #c8cad0;
                spacing: 3px; 
                padding: 3px;
            }
            QToolBar::separator { 
                background: #c0c4ca; 
                width: 1px; 
                margin: 3px 5px; 
            }
            QToolBar QToolButton {
                background: transparent;
                border: 1px solid transparent;
                border-radius: 4px;
                padding: 3px;
                margin: 1px;
                color: #303030;
            }
            QToolBar QToolButton:hover {
                background: #d0d4da;
                border: 1px solid #b0b4ba;
            }
            QToolBar QToolButton:checked {
                background: #c0d8f0;
                border: 1px solid #80a8d0;
            }
            QMenuBar { 
                background: #e0e2e6; 
                border-bottom: 1px solid #c0c2c6;
                color: #202020;
                font-weight: bold;
            }
            QMenuBar::item { 
                padding: 4px 8px;
                color: #202020;
            }
            QMenuBar::item:selected { background: #c8d0e0; color: #101010; }
            QMenu { background: #ffffff; border: 1px solid #c0c4ca; color: #202020; }
            QMenu::item { color: #202020; padding: 4px 20px; }
            QMenu::item:selected { background: #d0e0f8; color: #000000; }
            QStatusBar { 
                background: #e0e2e6; 
                border-top: 1px solid #c8cad0; 
                color: #404040;
            }
            QTreeView, QTreeWidget {
                background: #ffffff;
                color: #202020;
                border: 1px solid #d0d2d6;
                alternate-background-color: #f5f6f8;
                font-size: 13px;
            }
            QTreeView::item, QTreeWidget::item {
                padding: 3px 2px;
                color: #202020;
            }
            QTreeView::item:selected, QTreeWidget::item:selected {
                background: #c0d8f0;
                color: #000000;
            }
            QTreeView::item:hover, QTreeWidget::item:hover {
                background: #e0e8f0;
            }
            QHeaderView::section {
                background: #e8eaed;
                color: #202020;
                border: 1px solid #d0d2d6;
                padding: 4px;
                font-weight: bold;
            }
            QSplitter::handle { background: #d0d2d6; }
            QDialog { background: #f0f1f3; color: #202020; }
            QLabel { color: #202020; }
            QGroupBox { color: #202020; font-weight: bold; }
            QGroupBox::title { color: #202020; }
            QComboBox { color: #202020; background: #ffffff; border: 1px solid #c0c4ca; padding: 3px; }
            QComboBox::drop-down { border-left: 1px solid #c0c4ca; }
            QLineEdit, QSpinBox, QDoubleSpinBox {
                color: #202020; background: #ffffff; 
                border: 1px solid #c0c4ca; border-radius: 3px; padding: 3px;
            }
            QPushButton {
                color: #202020; background: #e8eaed;
                border: 1px solid #c0c4ca; border-radius: 4px; padding: 5px 12px;
            }
            QPushButton:hover { background: #d8dade; }
            QPushButton:pressed { background: #c0d8f0; }
            QCheckBox, QRadioButton { color: #202020; }
        )");
        
        QPalette pal;
        pal.setColor(QPalette::Window, QColor(240, 241, 243));
        pal.setColor(QPalette::WindowText, QColor(40, 40, 40));
        pal.setColor(QPalette::Base, QColor(255, 255, 255));
        pal.setColor(QPalette::AlternateBase, QColor(245, 246, 248));
        pal.setColor(QPalette::ToolTipBase, QColor(255, 255, 240));
        pal.setColor(QPalette::ToolTipText, QColor(40, 40, 40));
        pal.setColor(QPalette::Text, QColor(40, 40, 40));
        pal.setColor(QPalette::Button, QColor(232, 234, 237));
        pal.setColor(QPalette::ButtonText, QColor(40, 40, 40));
        pal.setColor(QPalette::Highlight, QColor(42, 130, 218));
        pal.setColor(QPalette::HighlightedText, Qt::white);
        app->setPalette(pal);
    }
    
    static void applyDark(QApplication* app) {
        app->setStyle(QStyleFactory::create("Fusion"));
        app->setStyleSheet(R"(
            QMainWindow { background: #2b2d30; }
            QToolBar { 
                background: #313335; 
                border-bottom: 1px solid #1e1f22;
                spacing: 3px; 
                padding: 3px;
            }
            QToolBar::separator { 
                background: #484a4d; 
                width: 1px; 
                margin: 3px 5px; 
            }
            QToolBar QToolButton {
                background: transparent;
                border: 1px solid transparent;
                border-radius: 4px;
                padding: 3px;
                margin: 1px;
                color: #bcc0c8;
            }
            QToolBar QToolButton:hover {
                background: #404347;
                border: 1px solid #505357;
            }
            QToolBar QToolButton:checked {
                background: #2d4f7c;
                border: 1px solid #3a6ba5;
            }
            QMenuBar { 
                background: #313335; 
                color: #bcc0c8;
                border-bottom: 1px solid #1e1f22;
            }
            QMenuBar::item:selected { background: #404347; }
            QMenu { background: #2b2d30; border: 1px solid #484a4d; color: #bcc0c8; }
            QMenu::item:selected { background: #2d4f7c; }
            QStatusBar { 
                background: #313335; 
                border-top: 1px solid #1e1f22; 
                color: #8c9098;
            }
            QTreeView, QTreeWidget {
                background: #2b2d30;
                color: #bcc0c8;
                border: 1px solid #3c3e42;
                alternate-background-color: #303234;
            }
            QTreeView::item:selected, QTreeWidget::item:selected { background: #2d4f7c; }
            QTreeView::item:hover, QTreeWidget::item:hover { background: #353739; }
            QHeaderView::section {
                background: #313335; color: #bcc0c8;
                border: 1px solid #3c3e42; padding: 4px;
            }
            QSplitter::handle { background: #1e1f22; }
            QDialog { background: #2b2d30; color: #bcc0c8; }
            QLabel { color: #bcc0c8; }
            QLineEdit, QSpinBox, QDoubleSpinBox { 
                background: #3c3e42; color: #d0d4dc; 
                border: 1px solid #505357; border-radius: 3px; padding: 3px;
            }
            QPushButton {
                background: #3c3e42; color: #d0d4dc;
                border: 1px solid #505357; border-radius: 4px; padding: 5px 12px;
            }
            QPushButton:hover { background: #484a4d; }
            QPushButton:pressed { background: #2d4f7c; }
            QInputDialog { background: #2b2d30; }
        )");
        
        QPalette pal;
        pal.setColor(QPalette::Window, QColor(43, 45, 48));
        pal.setColor(QPalette::WindowText, QColor(188, 192, 200));
        pal.setColor(QPalette::Base, QColor(43, 45, 48));
        pal.setColor(QPalette::AlternateBase, QColor(48, 50, 52));
        pal.setColor(QPalette::ToolTipBase, QColor(60, 62, 66));
        pal.setColor(QPalette::ToolTipText, QColor(200, 204, 212));
        pal.setColor(QPalette::Text, QColor(188, 192, 200));
        pal.setColor(QPalette::Button, QColor(60, 62, 66));
        pal.setColor(QPalette::ButtonText, QColor(188, 192, 200));
        pal.setColor(QPalette::Highlight, QColor(45, 79, 124));
        pal.setColor(QPalette::HighlightedText, QColor(220, 224, 230));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(100, 104, 110));
        app->setPalette(pal);
    }
    
    static void applyBlueDark(QApplication* app) {
        app->setStyle(QStyleFactory::create("Fusion"));
        app->setStyleSheet(R"(
            QMainWindow { background: #1a2332; }
            QToolBar { 
                background: #1e2a3a; 
                border-bottom: 1px solid #0f1820;
                spacing: 3px; padding: 3px;
            }
            QToolBar::separator { background: #2a3a4e; width: 1px; margin: 3px 5px; }
            QToolBar QToolButton {
                background: transparent; border: 1px solid transparent;
                border-radius: 4px; padding: 3px; margin: 1px; color: #8ca8c8;
            }
            QToolBar QToolButton:hover { background: #253548; border: 1px solid #3a5068; }
            QToolBar QToolButton:checked { background: #1a4070; border: 1px solid #2a60a0; }
            QMenuBar { background: #1e2a3a; color: #8ca8c8; border-bottom: 1px solid #0f1820; }
            QMenuBar::item:selected { background: #253548; }
            QMenu { background: #1a2332; border: 1px solid #2a3a4e; color: #8ca8c8; }
            QMenu::item:selected { background: #1a4070; }
            QStatusBar { background: #1e2a3a; border-top: 1px solid #0f1820; color: #607898; }
            QTreeView, QTreeWidget { background: #1a2332; color: #8ca8c8; border: 1px solid #2a3a4e; }
            QTreeView::item:selected, QTreeWidget::item:selected { background: #1a4070; }
            QTreeView::item:hover, QTreeWidget::item:hover { background: #222e3e; }
            QHeaderView::section { background: #1e2a3a; color: #8ca8c8; border: 1px solid #2a3a4e; padding: 4px; }
            QSplitter::handle { background: #0f1820; }
            QDialog, QLabel { background: #1a2332; color: #8ca8c8; }
            QLineEdit, QSpinBox, QDoubleSpinBox {
                background: #253548; color: #a0c0e0; border: 1px solid #3a5068; 
                border-radius: 3px; padding: 3px;
            }
            QPushButton { background: #253548; color: #a0c0e0; border: 1px solid #3a5068; 
                           border-radius: 4px; padding: 5px 12px; }
            QPushButton:hover { background: #2a4058; }
        )");
        
        QPalette pal;
        pal.setColor(QPalette::Window, QColor(26, 35, 50));
        pal.setColor(QPalette::WindowText, QColor(140, 168, 200));
        pal.setColor(QPalette::Base, QColor(26, 35, 50));
        pal.setColor(QPalette::Text, QColor(140, 168, 200));
        pal.setColor(QPalette::Button, QColor(37, 53, 72));
        pal.setColor(QPalette::ButtonText, QColor(140, 168, 200));
        pal.setColor(QPalette::Highlight, QColor(26, 64, 112));
        pal.setColor(QPalette::HighlightedText, QColor(180, 208, 240));
        app->setPalette(pal);
    }
    
    static void applyGraphite(QApplication* app) {
        app->setStyle(QStyleFactory::create("Fusion"));
        app->setStyleSheet(R"(
            QMainWindow { background: #3a3a3a; }
            QToolBar { 
                background: #444444; border-bottom: 1px solid #2a2a2a;
                spacing: 3px; padding: 3px;
            }
            QToolBar::separator { background: #555555; width: 1px; margin: 3px 5px; }
            QToolBar QToolButton {
                background: transparent; border: 1px solid transparent;
                border-radius: 4px; padding: 3px; margin: 1px; color: #c0c0c0;
            }
            QToolBar QToolButton:hover { background: #505050; border: 1px solid #666666; }
            QToolBar QToolButton:checked { background: #606060; border: 1px solid #808080; }
            QMenuBar { background: #444444; color: #c0c0c0; border-bottom: 1px solid #2a2a2a; }
            QMenuBar::item:selected { background: #555555; }
            QMenu { background: #3a3a3a; border: 1px solid #555555; color: #c0c0c0; }
            QMenu::item:selected { background: #606060; }
            QStatusBar { background: #444444; border-top: 1px solid #2a2a2a; color: #909090; }
            QTreeView, QTreeWidget { background: #3a3a3a; color: #c0c0c0; border: 1px solid #4a4a4a; }
            QTreeView::item:selected, QTreeWidget::item:selected { background: #606060; }
            QTreeView::item:hover, QTreeWidget::item:hover { background: #454545; }
            QHeaderView::section { background: #444444; color: #c0c0c0; border: 1px solid #4a4a4a; padding: 4px; }
            QSplitter::handle { background: #2a2a2a; }
            QDialog, QLabel { background: #3a3a3a; color: #c0c0c0; }
            QLineEdit, QSpinBox, QDoubleSpinBox {
                background: #4a4a4a; color: #d0d0d0; border: 1px solid #606060;
                border-radius: 3px; padding: 3px;
            }
            QPushButton { background: #4a4a4a; color: #d0d0d0; border: 1px solid #606060;
                           border-radius: 4px; padding: 5px 12px; }
            QPushButton:hover { background: #555555; }
        )");
        
        QPalette pal;
        pal.setColor(QPalette::Window, QColor(58, 58, 58));
        pal.setColor(QPalette::WindowText, QColor(192, 192, 192));
        pal.setColor(QPalette::Base, QColor(58, 58, 58));
        pal.setColor(QPalette::Text, QColor(192, 192, 192));
        pal.setColor(QPalette::Button, QColor(74, 74, 74));
        pal.setColor(QPalette::ButtonText, QColor(192, 192, 192));
        pal.setColor(QPalette::Highlight, QColor(96, 96, 96));
        pal.setColor(QPalette::HighlightedText, QColor(230, 230, 230));
        app->setPalette(pal);
    }
};

#endif // THEMEMANAGER_H

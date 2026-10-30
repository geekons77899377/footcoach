#pragma once

#include <QString>
#include <QtGlobal>

// Disposition « téléphone » : toujours sur Android ; sur PC, uniquement si la variable d'environnement
// FOOTCOACH_COMPACT vaut 1 (pour essayer la disposition compacte sans téléphone).
namespace Platform {

inline bool compactLayout()
{
#ifdef Q_OS_ANDROID
    return true;
#else
    static const bool forced = qEnvironmentVariableIntValue("FOOTCOACH_COMPACT") != 0;
    return forced;
#endif
}

// Feuille de style de la disposition téléphone : cibles tactiles confortables (environ 44 px) et menus aérés.
inline QString compactStyleSheet()
{
    return QStringLiteral(
        "QPushButton, QComboBox, QLineEdit, QSpinBox, QDateTimeEdit, QDateEdit, QTimeEdit { min-height: 44px; }"
        "QMenu::item { padding: 14px 32px; font-size: 18px; }"
        "QListWidget::item, QTableWidget::item { min-height: 36px; }"
        "QCheckBox { min-height: 40px; }");
}

} // namespace Platform

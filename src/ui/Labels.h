#pragma once

#include <QCoreApplication>
#include <QList>
#include <QPair>
#include <QString>

// Libellés affichés pour les codes stockés en base.
namespace Labels {

inline QList<QPair<QString, QString>> positions()   // code -> libellé
{
    return {
        {QStringLiteral("GK"), QCoreApplication::translate("Labels", "Gardien")},
        {QStringLiteral("DEF"), QCoreApplication::translate("Labels", "Défenseur")},
        {QStringLiteral("MID"), QCoreApplication::translate("Labels", "Milieu")},
        {QStringLiteral("FWD"), QCoreApplication::translate("Labels", "Attaquant")},
    };
}

inline QList<QPair<QString, QString>> feet()
{
    return {
        {QStringLiteral("R"), QCoreApplication::translate("Labels", "Droit")},
        {QStringLiteral("L"), QCoreApplication::translate("Labels", "Gauche")},
        {QStringLiteral("B"), QCoreApplication::translate("Labels", "Les deux")},
    };
}

inline QList<QPair<QString, QString>> categories()
{
    return {
        {QStringLiteral("warmup"), QCoreApplication::translate("Labels", "Échauffement")},
        {QStringLiteral("technical"), QCoreApplication::translate("Labels", "Technique")},
        {QStringLiteral("tactical"), QCoreApplication::translate("Labels", "Tactique")},
        {QStringLiteral("physical"), QCoreApplication::translate("Labels", "Physique")},
        {QStringLiteral("game"), QCoreApplication::translate("Labels", "Jeu")},
        {QStringLiteral("cooldown"), QCoreApplication::translate("Labels", "Retour au calme")},
    };
}

inline QList<QPair<QString, QString>> statuses()
{
    return {
        {QStringLiteral("planned"), QCoreApplication::translate("Labels", "Prévue")},
        {QStringLiteral("done"), QCoreApplication::translate("Labels", "Réalisée")},
        {QStringLiteral("cancelled"), QCoreApplication::translate("Labels", "Annulée")},
    };
}

inline QString label(const QList<QPair<QString, QString>> &list, const QString &code)
{
    for (const auto &entry : list) {
        if (entry.first == code)
            return entry.second;
    }
    return QString();
}

} // namespace Labels

#pragma once

#include "core/TacticalBoard.h"

#include <QString>
#include <QStringList>

class QIODevice;
class QPainter;

// Contenu de la fiche exercice (schéma 13).
struct ExerciseSheetData
{
    QString title;
    QString ageCategory;            // catégorie définie par l'éducateur (ex. U10-U11), pastille verte ; vide : masquée
    int durationMin = 0;            // 0 : pastille masquée
    QString objective;              // bande « Objectifs : … » ; vide : masquée
    QStringList organisation;       // terrain, joueurs, matériel
    QStringList instructions;       // Consignes (numérotées)
    QStringList variations;         // Évolutions
    QStringList criteria;           // Critères de réussite
    QString coachCue;               // Consigne éducateur, dans la bulle
    bool hasBoard = false;
    Tactics::Board board;           // schéma de référence de l'exercice
};

// Fiche exercice A4 paysage, en couleur, dessinée au QPainter (aucune image externe) : bandeau titre, catégorie,
// durée, objectif, schéma de l'exercice cadré sur la zone utilisée, cadres Organisation, Consignes, Évolutions,
// Critères de réussite et Consigne éducateur. Les textes trop longs sont réduits jusqu'à une taille minimale.
class ExerciseSheet
{
public:
    static constexpr qreal kWidth = 1536;      // repère logique, rapport A4 paysage
    static constexpr qreal kHeight = 1086;

    // Une ligne par point : lignes vides retirées, puces de début (-, *, •) enlevées.
    static QStringList splitLines(const QString &text);
    // Dessine la fiche dans le repère logique kWidth x kHeight (le peintre est déjà mis à l'échelle).
    static void paint(QPainter &p, const ExerciseSheetData &data);
    static bool writePdf(QIODevice *out, const ExerciseSheetData &data, QString *error);
    // Nom de fichier proposé : « fiche-<titre>.pdf », sans accents ni caractères spéciaux.
    static QString fileName(const QString &title);
};

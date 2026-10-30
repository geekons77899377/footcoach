#pragma once

#include "core/Models.h"

#include <QList>
#include <QString>

namespace SessionSheet {

// Schéma à joindre à la fiche : l'image elle-même est fournie au moteur PDF sous le nom imageName
// (QTextDocument::addResource). exerciseId >= 0 : schéma lié à un exercice du plan ;
// sinon il est lié à la séance elle-même. Tous sont affichés dans la section « Schémas » après le plan,
// ceux des exercices d'abord (dans l'ordre du plan, avec l'exercice en légende).
struct SheetBoard
{
    QString title;
    QString imageName;
    int exerciseId = -1;
};

// Fiche de séance imprimable, en HTML simple (sous-ensemble géré par QTextDocument) :
// en-tête, plan avec horaires calculés, matériel à prévoir, total comparé à la durée de la séance.
QString buildHtml(const Team &team, const Session &session, const QList<PlanEntry> &plan,
                  const QList<SheetBoard> &boards = {});

// Nom de fichier proposé, par exemple "Seance_2026-10-27_U15.pdf".
QString suggestedFileName(const Team &team, const Session &session);

} // namespace SessionSheet

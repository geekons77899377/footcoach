#include "SessionSheet.h"

#include "core/PlanSchedule.h"

#include <QLocale>
#include <QSet>
#include <QRegularExpression>
#include <QStringList>

namespace {

QString esc(const QString &s)
{
    return s.toHtmlEscaped();
}

// Conserve les retours à la ligne saisis par l'utilisateur.
QString escMultiline(const QString &s)
{
    return s.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
}

QString categoryLabel(const QString &code)
{
    if (code == QLatin1String("warmup"))   return QStringLiteral("Échauffement");
    if (code == QLatin1String("technical")) return QStringLiteral("Technique");
    if (code == QLatin1String("tactical")) return QStringLiteral("Tactique");
    if (code == QLatin1String("physical")) return QStringLiteral("Physique");
    if (code == QLatin1String("game"))     return QStringLiteral("Jeu");
    if (code == QLatin1String("cooldown")) return QStringLiteral("Retour au calme");
    return code;
}

QString players(const Exercise &e)
{
    if (e.playersMin > 0 && e.playersMax > 0)
        return e.playersMin == e.playersMax ? QString::number(e.playersMin)
                                            : QStringLiteral("%1 à %2").arg(e.playersMin).arg(e.playersMax);
    if (e.playersMin > 0)
        return QStringLiteral("%1 et plus").arg(e.playersMin);
    if (e.playersMax > 0)
        return QStringLiteral("jusqu'à %1").arg(e.playersMax);
    return QString();
}

// Matériel de toutes les lignes, sans doublon (comparaison insensible à la casse), dans l'ordre d'apparition.
QStringList equipmentList(const QList<PlanEntry> &plan)
{
    QStringList result;
    QStringList seen;
    for (const PlanEntry &p : plan) {
        for (const QString &part : p.exercise.equipment.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            const QString item = part.trimmed();
            if (item.isEmpty() || item.compare(QLatin1String("Aucun"), Qt::CaseInsensitive) == 0)
                continue;
            if (!seen.contains(item.toLower())) {
                seen << item.toLower();
                result << item;
            }
        }
    }
    return result;
}

} // namespace

namespace SessionSheet {

namespace {
constexpr int kBoardWidth = 420;   // px dans le document ; la largeur utile de la page est d'environ 525
}

QString buildHtml(const Team &team, const Session &session, const QList<PlanEntry> &plan,
                  const QList<SheetBoard> &boards)
{
    const QLocale fr(QLocale::French);
    const QString timeFmt = QStringLiteral("HH:mm");

    QList<int> durations;
    for (const PlanEntry &p : plan)
        durations << p.durationMin;
    const QList<PlanSchedule::Slot> timeline = PlanSchedule::schedule(session.startAt, durations);
    const int total = PlanSchedule::totalMinutes(durations);

    QString html;
    html += QStringLiteral("<html><body style=\"font-family: Arial, sans-serif; font-size: 10pt;\">");
    html += QStringLiteral("<h1 style=\"font-size: 18pt; margin-bottom: 2px;\">Séance d'entraînement</h1>");
    html += QStringLiteral("<p style=\"font-size: 12pt; margin-top: 0px;\"><b>%1</b>%2</p>")
                .arg(esc(team.name),
                     team.category.isEmpty() ? QString() : QStringLiteral(" — %1").arg(esc(team.category)));

    html += QStringLiteral("<table cellspacing=\"0\" cellpadding=\"3\">");
    html += QStringLiteral("<tr><td><b>Date</b></td><td>%1</td></tr>")
                .arg(esc(fr.toString(session.startAt.date(), QStringLiteral("dddd d MMMM yyyy"))));
    html += QStringLiteral("<tr><td><b>Horaire</b></td><td>%1 – %2 (%3 min)</td></tr>")
                .arg(session.startAt.time().toString(timeFmt), session.endAt().time().toString(timeFmt))
                .arg(session.durationMin);
    if (!session.location.isEmpty())
        html += QStringLiteral("<tr><td><b>Lieu</b></td><td>%1</td></tr>").arg(esc(session.location));
    if (!session.theme.isEmpty())
        html += QStringLiteral("<tr><td><b>Thème</b></td><td>%1</td></tr>").arg(esc(session.theme));
    html += QStringLiteral("</table>");

    if (!session.notes.isEmpty())
        html += QStringLiteral("<p><b>Notes :</b> %1</p>").arg(escMultiline(session.notes));

    html += QStringLiteral("<h2 style=\"font-size: 13pt;\">Plan de séance</h2>");
    if (plan.isEmpty()) {
        html += QStringLiteral("<p><i>Aucun exercice dans le plan.</i></p>");
    } else {
        html += QStringLiteral("<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\" width=\"100%\">");
        html += QStringLiteral("<tr bgcolor=\"#dddddd\"><th width=\"14%\">Horaire</th><th width=\"12%\">Durée</th>"
                               "<th>Exercice</th></tr>");
        for (int i = 0; i < plan.size(); ++i) {
            const PlanEntry &p = plan.at(i);
            const Exercise &e = p.exercise;

            QString details = QStringLiteral("<b>%1</b> <i>(%2)</i>").arg(esc(e.title), esc(categoryLabel(e.category)));
            if (!e.objective.isEmpty())
                details += QStringLiteral("<br/><u>Objectif</u> : %1").arg(escMultiline(e.objective));
            if (!e.description.isEmpty())
                details += QStringLiteral("<br/>%1").arg(escMultiline(e.description));
            const QString nb = players(e);
            if (!nb.isEmpty())
                details += QStringLiteral("<br/><u>Joueurs</u> : %1").arg(esc(nb));
            if (!e.equipment.isEmpty())
                details += QStringLiteral("<br/><u>Matériel</u> : %1").arg(esc(e.equipment));
            if (!p.notes.isEmpty())
                details += QStringLiteral("<br/><u>Note</u> : %1").arg(escMultiline(p.notes));

            html += QStringLiteral("<tr><td align=\"center\">%1<br/>%2</td><td align=\"center\" nowrap=\"nowrap\">%3&nbsp;min</td><td>%4</td></tr>")
                        .arg(timeline.at(i).start.time().toString(timeFmt), timeline.at(i).end.time().toString(timeFmt))
                        .arg(p.durationMin)
                        .arg(details);
        }
        html += QStringLiteral("</table>");

        QString totalLine = QStringLiteral("<p><b>Total des exercices : %1 min</b> pour une séance de %2 min.")
                                .arg(total)
                                .arg(session.durationMin);
        if (total > session.durationMin)
            totalLine += QStringLiteral(" <font color=\"#c00000\"><b>Attention : le plan dépasse la durée de la séance de %1 min.</b></font>")
                             .arg(total - session.durationMin);
        else if (total < session.durationMin)
            totalLine += QStringLiteral(" Temps libre : %1 min.").arg(session.durationMin - total);
        totalLine += QStringLiteral("</p>");
        html += totalLine;

        const QStringList equipment = equipmentList(plan);
        if (!equipment.isEmpty())
            html += QStringLiteral("<p><b>Matériel à prévoir :</b> %1</p>").arg(esc(equipment.join(QStringLiteral(", "))));
    }

    // Schémas : d'abord ceux des exercices du plan (dans l'ordre du plan), puis ceux liés à la séance.
    // Ils sont placés après le plan et non dans le tableau : une grande image dans une cellule écrase les
    // autres colonnes avec le moteur de mise en page de Qt.
    QString boardSection;
    QSet<QString> shown;
    auto addBoard = [&](const SheetBoard &b, const QString &exerciseTitle) {
        if (shown.contains(b.imageName))
            return;
        shown.insert(b.imageName);
        QString caption = QStringLiteral("<b>%1</b>").arg(esc(b.title));
        if (!exerciseTitle.isEmpty())
            caption += QStringLiteral(" — exercice : %1").arg(esc(exerciseTitle));
        // Légende sous l'image : elle ne peut pas se retrouver seule en bas d'une page.
        boardSection += QStringLiteral("<p><img src=\"%1\" width=\"%2\"/><br/>%3</p>")
                            .arg(esc(b.imageName)).arg(kBoardWidth).arg(caption);
    };
    for (const PlanEntry &p : plan) {
        for (const SheetBoard &b : boards) {
            if (b.exerciseId >= 0 && b.exerciseId == p.exercise.id)
                addBoard(b, p.exercise.title);
        }
    }
    for (const SheetBoard &b : boards) {
        if (b.exerciseId < 0)
            addBoard(b, QString());
    }
    if (!boardSection.isEmpty())
        // Les schémas commencent sur une nouvelle page : un titre ne reste pas seul en bas de la page du plan.
        html += QStringLiteral("<h2 style=\"font-size: 13pt; page-break-before: always;\">Schémas</h2>") + boardSection;

    html += QStringLiteral("</body></html>");
    return html;
}

QString suggestedFileName(const Team &team, const Session &session)
{
    // Garde lettres, chiffres, tiret et underscore : le résultat est valable comme nom de fichier Windows.
    QString teamPart = team.name;
    teamPart.replace(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}_-]+")), QStringLiteral("_"));
    teamPart = teamPart.left(40);
    while (teamPart.startsWith(QLatin1Char('_')))
        teamPart.remove(0, 1);
    while (teamPart.endsWith(QLatin1Char('_')))
        teamPart.chop(1);
    const QString date = session.startAt.date().toString(QStringLiteral("yyyy-MM-dd"));
    return teamPart.isEmpty() ? QStringLiteral("Seance_%1.pdf").arg(date)
                              : QStringLiteral("Seance_%1_%2.pdf").arg(date, teamPart);
}

} // namespace SessionSheet

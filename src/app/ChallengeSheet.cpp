#include "ChallengeSheet.h"

#include <QFont>
#include <QFontMetrics>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPen>

namespace {

// Unité de dessin : 0,1 mm (résolution 254 ppp). Page A4 paysage : 2970 × 2100.
constexpr int kPageW = 2970;
constexpr int kPageH = 2100;
constexpr int kMargin = 100;

constexpr int kBlockW = 1355;
constexpr int kBlockGap = 60;
constexpr int kBlockH = 810;
constexpr int kNameRowH = 70;
constexpr int kHeaderH = 55;
constexpr int kRowH = 48;
constexpr int kTopOfBlocks = 300;
constexpr int kRowGap = 40;

const int kColW[5] = {110, 360, 330, 300, 255};     // n°, Nom, Prénom, N° de licence, Blessure (somme = kBlockW)

QFont font(qreal points, bool bold = false)
{
    QFont f(QStringLiteral("sans-serif"));
    f.setPointSizeF(points);
    f.setBold(bold);
    return f;
}

void text(QPainter &p, const QRect &r, const QString &s, Qt::Alignment align, const QFont &f, int padding = 12)
{
    p.setFont(f);
    const QRect inner = r.adjusted(padding, 0, -padding, 0);
    p.drawText(inner, int(align | Qt::AlignVCenter | Qt::TextSingleLine),
               QFontMetrics(f, p.device()).elidedText(s, Qt::ElideRight, inner.width()));
}

// Texte d'une cellule : la police rétrécit (jusqu'à 6,5 pt) pour que le nom tienne en entier ; l'ellipse n'est que le
// dernier recours, pour un texte vraiment trop long.
void fitText(QPainter &p, const QRect &r, const QString &s, Qt::Alignment align, qreal points, bool bold, int padding)
{
    qreal pt = points;
    const int room = r.width() - 2 * padding;
    while (pt > 6.5 && QFontMetrics(font(pt, bold), p.device()).horizontalAdvance(s) > room)
        pt -= 0.5;
    text(p, r, s, align, font(pt, bold), padding);
}

void block(QPainter &p, int x, int y, int number, const QString &teamName, const QList<ChallengeEntry> &entries)
{
    const QPen thick(Qt::black, 4);
    const QPen thin(Qt::black, 2);

    // « ÉQUIPE n : » et cadre du nom de l'équipe.
    const QRect label(x, y, 250, kNameRowH);
    text(p, label, QStringLiteral("ÉQUIPE %1 :").arg(number), Qt::AlignLeft, font(11, true), 0);
    const QRect nameBox(x + 260, y, kBlockW - 260, kNameRowH);
    p.setPen(thick);
    p.drawRect(nameBox);
    if (!teamName.isEmpty())
        text(p, nameBox, teamName, Qt::AlignLeft, font(12, true));

    // Tableau : en-tête puis kRows lignes.
    const int top = y + kNameRowH + 25;
    const QStringList heads = {QString(), QStringLiteral("Nom"), QStringLiteral("Prénom"),
                               QStringLiteral("N° de licence"), QStringLiteral("Blessure")};
    int cx = x;
    for (int c = 0; c < 5; ++c) {
        text(p, QRect(cx, top, kColW[c], kHeaderH), heads.at(c), Qt::AlignHCenter, font(9, true), 4);
        cx += kColW[c];
    }
    for (int r = 0; r < ChallengeSheet::kRows; ++r) {
        const int ry = top + kHeaderH + r * kRowH;
        cx = x;
        for (int c = 0; c < 5; ++c) {
            QString s;
            Qt::Alignment al = Qt::AlignLeft;
            if (c == 0) {
                s = QString::number(r + 1);
                al = Qt::AlignHCenter;
            } else if (r < entries.size()) {
                const ChallengeEntry &e = entries.at(r);
                s = c == 1 ? e.lastName.toUpper() : c == 2 ? e.firstName : c == 3 ? e.licence : QString();
                al = c == 3 ? Qt::AlignHCenter : Qt::AlignLeft;
            }
            fitText(p, QRect(cx, ry, kColW[c], kRowH), s, al, 10, c == 1, 10);
            cx += kColW[c];
        }
    }
    // Quadrillage : filets fins, cadre et ligne d'en-tête plus épais.
    const int tableH = kHeaderH + ChallengeSheet::kRows * kRowH;
    p.setPen(thin);
    for (int r = 1; r <= ChallengeSheet::kRows; ++r)
        p.drawLine(x, top + kHeaderH + r * kRowH - kRowH, x + kBlockW, top + kHeaderH + r * kRowH - kRowH);
    cx = x;
    for (int c = 1; c < 5; ++c) {
        cx += kColW[c - 1];
        p.drawLine(cx, top, cx, top + tableH);
    }
    p.setPen(thick);
    p.drawRect(x, top, kBlockW, tableH);
    p.drawLine(x, top + kHeaderH, x + kBlockW, top + kHeaderH);

    // Responsable : à remplir à la main.
    const int ry = top + tableH + 25;
    text(p, QRect(x, ry, 330, 60), QStringLiteral("Nom Responsable"), Qt::AlignLeft, font(9), 0);
    p.setPen(thick);
    p.drawRect(x + 340, ry, 350, 60);
    text(p, QRect(x + 720, ry, 220, 60), QStringLiteral("N° de licence"), Qt::AlignLeft, font(9), 0);
    p.setPen(thick);
    p.drawRect(x + 950, ry, kBlockW - 950, 60);
}

} // namespace

bool ChallengeSheet::isValidLicence(const QString &licence)
{
    QString digits;
    for (const QChar c : licence) {
        if (c.isSpace())
            continue;
        if (c < QLatin1Char('0') || c > QLatin1Char('9'))
            return false;
        digits += c;
    }
    return digits.size() == kLicenceDigits;
}

bool ChallengeSheet::writePdf(QIODevice *out, const QString &teamName, const QList<ChallengeEntry> &entries, QString *error)
{
    if (!out || !out->isOpen() || !out->isWritable()) {
        if (error)
            *error = QStringLiteral("Écriture du PDF impossible.");
        return false;
    }
    QPdfWriter pdf(out);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageOrientation(QPageLayout::Landscape);
    pdf.setPageMargins(QMarginsF(0, 0, 0, 0));
    pdf.setResolution(254);
    pdf.setTitle(QStringLiteral("Feuille de challenge 2/2 - %1").arg(teamName));
    pdf.setCreator(QStringLiteral("FootCoach"));

    QPainter p(&pdf);
    if (!p.isActive()) {
        if (error)
            *error = QStringLiteral("Création du PDF impossible.");
        return false;
    }
    p.setBrush(Qt::NoBrush);                        // jamais d'aplat : traits noirs seulement
    p.setPen(QPen(Qt::black, 3));

    // Titre.
    text(p, QRect(kMargin, 90, kPageW - 2 * kMargin, 150), QStringLiteral("FEUILLE DE CHALLENGE 2/2"), Qt::AlignHCenter,
         font(24, true), 0);

    const QList<ChallengeEntry> shown = entries.mid(0, kRows);
    const int x0 = kMargin;
    const int x1 = kMargin + kBlockW + kBlockGap;
    block(p, x0, kTopOfBlocks, 1, teamName, shown);
    block(p, x1, kTopOfBlocks, 2, QString(), {});
    block(p, x0, kTopOfBlocks + kBlockH + kRowGap, 3, QString(), {});
    block(p, x1, kTopOfBlocks + kBlockH + kRowGap, 4, QString(), {});

    // Séparations en croix, comme la feuille papier.
    p.setPen(QPen(Qt::black, 2));
    const int midX = kMargin + kBlockW + kBlockGap / 2;
    const int midY = kTopOfBlocks + kBlockH + kRowGap / 2;
    p.drawLine(midX, kTopOfBlocks, midX, kTopOfBlocks + 2 * kBlockH + kRowGap);
    p.drawLine(kMargin, midY, kPageW - kMargin, midY);

    text(p, QRect(kMargin, kPageH - 90, kPageW - 2 * kMargin, 50),
         QStringLiteral("(Mettre le chiffre 1 dans la colonne « Blessure » - Préciser ensuite sa blessure)"), Qt::AlignRight,
         font(7), 0);
    p.end();
    return true;
}

#include "TacticalBoard.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QtGlobal>
#include <cmath>

namespace Tactics {

namespace {

constexpr int kVersion = 1;

QString kindToString(Kind k)
{
    switch (k) {
    case Kind::Player: return QStringLiteral("player");
    case Kind::Opponent: return QStringLiteral("opponent");
    case Kind::Ball: return QStringLiteral("ball");
    case Kind::Cone: return QStringLiteral("cone");
    case Kind::Goal: return QStringLiteral("goal");
    case Kind::Text: return QStringLiteral("text");
    }
    return QStringLiteral("player");
}

bool kindFromString(const QString &s, Kind *k)
{
    static const Kind all[] = {Kind::Player, Kind::Opponent, Kind::Ball, Kind::Cone, Kind::Goal, Kind::Text};
    for (Kind c : all) {
        if (kindToString(c) == s) {
            *k = c;
            return true;
        }
    }
    return false;
}

QString styleToString(ArrowStyle s)
{
    switch (s) {
    case ArrowStyle::Run: return QStringLiteral("run");
    case ArrowStyle::Pass: return QStringLiteral("pass");
    case ArrowStyle::Dribble: return QStringLiteral("dribble");
    case ArrowStyle::Cross: return QStringLiteral("cross");
    }
    return QStringLiteral("pass");
}

bool styleFromString(const QString &s, ArrowStyle *out)
{
    static const ArrowStyle all[] = {ArrowStyle::Run, ArrowStyle::Pass, ArrowStyle::Dribble, ArrowStyle::Cross};
    for (ArrowStyle c : all) {
        if (styleToString(c) == s) {
            *out = c;
            return true;
        }
    }
    return false;
}

double clamp01(double v)
{
    return qBound(0.0, v, 1.0);
}

bool readNumber(const QJsonObject &o, const char *key, double *out)
{
    const QJsonValue v = o.value(QLatin1String(key));
    if (!v.isDouble())
        return false;
    *out = v.toDouble();
    return true;
}

bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

Item player(int number, double x, double y)
{
    Item it;
    it.kind = Kind::Player;
    it.pos = QPointF(x, y);
    it.label = QString::number(number);
    return it;
}

} // namespace

QString toJson(const Board &board)
{
    QJsonObject root;
    root.insert(QStringLiteral("version"), kVersion);
    root.insert(QStringLiteral("pitch"), board.pitch == Pitch::Half ? QStringLiteral("half") : QStringLiteral("full"));

    QJsonArray items;
    for (const Item &it : board.items) {
        QJsonObject o;
        o.insert(QStringLiteral("kind"), kindToString(it.kind));
        o.insert(QStringLiteral("x"), it.pos.x());
        o.insert(QStringLiteral("y"), it.pos.y());
        if (!it.label.isEmpty())
            o.insert(QStringLiteral("label"), it.label);
        if (it.rotation != 0)
            o.insert(QStringLiteral("rot"), it.rotation);
        if (it.scale != 1.0)
            o.insert(QStringLiteral("scale"), it.scale);
        if (!it.color.isEmpty())
            o.insert(QStringLiteral("color"), it.color);
        items.append(o);
    }
    root.insert(QStringLiteral("items"), items);

    QJsonArray arrows;
    for (const Arrow &a : board.arrows) {
        QJsonObject o;
        o.insert(QStringLiteral("style"), styleToString(a.style));
        o.insert(QStringLiteral("x1"), a.from.x());
        o.insert(QStringLiteral("y1"), a.from.y());
        o.insert(QStringLiteral("x2"), a.to.x());
        o.insert(QStringLiteral("y2"), a.to.y());
        if (a.curve != 0)
            o.insert(QStringLiteral("curve"), a.curve);
        arrows.append(o);
    }
    root.insert(QStringLiteral("arrows"), arrows);

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool fromJson(const QString &json, Board *board, QString *error)
{
    Q_ASSERT(board);
    *board = Board();
    if (json.trimmed().isEmpty())
        return true;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return fail(error, QStringLiteral("Schéma illisible : %1").arg(parseError.errorString()));
    if (!doc.isObject())
        return fail(error, QStringLiteral("Schéma illisible : objet JSON attendu."));

    const QJsonObject root = doc.object();
    if (root.isEmpty())
        return true;   // "{}" : schéma vide

    if (root.value(QStringLiteral("version")).toInt(0) != kVersion)
        return fail(error, QStringLiteral("Schéma d'une version inconnue (%1).")
                               .arg(root.value(QStringLiteral("version")).toInt(0)));

    Board result;
    const QString pitch = root.value(QStringLiteral("pitch")).toString(QStringLiteral("full"));
    if (pitch == QLatin1String("half"))
        result.pitch = Pitch::Half;
    else if (pitch != QLatin1String("full"))
        return fail(error, QStringLiteral("Type de terrain inconnu : %1").arg(pitch));

    for (const QJsonValue &v : root.value(QStringLiteral("items")).toArray()) {
        const QJsonObject o = v.toObject();
        Item it;
        double x = 0, y = 0;
        if (!kindFromString(o.value(QStringLiteral("kind")).toString(), &it.kind)
            || !readNumber(o, "x", &x) || !readNumber(o, "y", &y))
            return fail(error, QStringLiteral("Schéma illisible : élément invalide."));
        it.pos = QPointF(clamp01(x), clamp01(y));
        it.label = o.value(QStringLiteral("label")).toString();
        if (o.value(QStringLiteral("rot")).isDouble() && isRotatable(it.kind)) {
            double rot = std::fmod(o.value(QStringLiteral("rot")).toDouble(), 360.0);
            if (rot < 0)
                rot += 360.0;
            it.rotation = rot;
        }
        if (o.value(QStringLiteral("scale")).isDouble() && isScalable(it.kind))
            it.scale = qBound(kMinScale, o.value(QStringLiteral("scale")).toDouble(), kMaxScale);
        const QString color = o.value(QStringLiteral("color")).toString();
        if (isColorable(it.kind) && QRegularExpression(QStringLiteral("^#[0-9a-fA-F]{6}$")).match(color).hasMatch())
            it.color = color.toLower();
        result.items.append(it);
    }

    for (const QJsonValue &v : root.value(QStringLiteral("arrows")).toArray()) {
        const QJsonObject o = v.toObject();
        Arrow a;
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        if (!styleFromString(o.value(QStringLiteral("style")).toString(), &a.style)
            || !readNumber(o, "x1", &x1) || !readNumber(o, "y1", &y1)
            || !readNumber(o, "x2", &x2) || !readNumber(o, "y2", &y2))
            return fail(error, QStringLiteral("Schéma illisible : flèche invalide."));
        a.from = QPointF(clamp01(x1), clamp01(y1));
        a.to = QPointF(clamp01(x2), clamp01(y2));
        if (a.style == ArrowStyle::Cross) {
            // Un centre sans courbure enregistrée (ancien fichier, valeur absente) prend la courbure par défaut.
            a.curve = o.value(QStringLiteral("curve")).isDouble()
                          ? qBound(-kMaxCurve, o.value(QStringLiteral("curve")).toDouble(), kMaxCurve)
                          : kDefaultCurve;
        }
        result.arrows.append(a);
    }

    *board = result;
    return true;
}

QList<QPair<QString, QString>> colorPalette()
{
    return {{QStringLiteral("Bleu"), QStringLiteral("#1565c0")},
            {QStringLiteral("Rouge"), QStringLiteral("#c62828")},
            {QStringLiteral("Jaune"), QStringLiteral("#fbc02d")},
            {QStringLiteral("Orange"), QStringLiteral("#ef6c00")},
            {QStringLiteral("Violet"), QStringLiteral("#7b1fa2")},
            {QStringLiteral("Blanc"), QStringLiteral("#fafafa")},
            {QStringLiteral("Noir"), QStringLiteral("#212121")}};
}

QStringList formationNames()
{
    return {QStringLiteral("4-4-2"), QStringLiteral("4-3-3"), QStringLiteral("3-5-2"), QStringLiteral("4-2-3-1")};
}

QList<Item> formation(const QString &name)
{
    // x : profondeur (0 = but défendu), y : largeur (0 = haut). Numéros classiques.
    if (name == QLatin1String("4-4-2")) {
        return {player(1, 0.06, 0.50),
                player(2, 0.22, 0.85), player(5, 0.20, 0.62), player(4, 0.20, 0.38), player(3, 0.22, 0.15),
                player(7, 0.46, 0.88), player(6, 0.42, 0.62), player(8, 0.42, 0.38), player(11, 0.46, 0.12),
                player(9, 0.70, 0.62), player(10, 0.70, 0.38)};
    }
    if (name == QLatin1String("4-3-3")) {
        return {player(1, 0.06, 0.50),
                player(2, 0.22, 0.85), player(5, 0.20, 0.62), player(4, 0.20, 0.38), player(3, 0.22, 0.15),
                player(6, 0.40, 0.50), player(8, 0.50, 0.70), player(10, 0.50, 0.30),
                player(7, 0.72, 0.85), player(9, 0.76, 0.50), player(11, 0.72, 0.15)};
    }
    if (name == QLatin1String("3-5-2")) {
        return {player(1, 0.06, 0.50),
                player(5, 0.20, 0.75), player(4, 0.18, 0.50), player(3, 0.20, 0.25),
                player(2, 0.44, 0.92), player(6, 0.38, 0.50), player(8, 0.46, 0.68), player(10, 0.46, 0.32), player(11, 0.44, 0.08),
                player(9, 0.72, 0.60), player(7, 0.72, 0.40)};
    }
    if (name == QLatin1String("4-2-3-1")) {
        return {player(1, 0.06, 0.50),
                player(2, 0.22, 0.85), player(5, 0.20, 0.62), player(4, 0.20, 0.38), player(3, 0.22, 0.15),
                player(6, 0.38, 0.62), player(8, 0.38, 0.38),
                player(7, 0.58, 0.85), player(10, 0.58, 0.50), player(11, 0.58, 0.15),
                player(9, 0.76, 0.50)};
    }
    return {};
}

QList<Item> mirroredAsOpponent(const QList<Item> &players)
{
    QList<Item> result;
    for (const Item &p : players) {
        Item o = p;
        o.kind = Kind::Opponent;
        o.pos = QPointF(1.0 - p.pos.x(), 1.0 - p.pos.y());
        result.append(o);
    }
    return result;
}

int nextFreePlayerNumber(const QList<Item> &items)
{
    QSet<int> used;
    for (const Item &it : items) {
        if (it.kind != Kind::Player)
            continue;
        bool ok = false;
        const int n = it.label.toInt(&ok);
        if (ok)
            used.insert(n);
    }
    for (int n = 1; n <= 99; ++n) {
        if (!used.contains(n))
            return n;
    }
    return 0;
}

} // namespace Tactics

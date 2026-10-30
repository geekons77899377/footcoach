#pragma once

#include <QList>
#include <QPair>
#include <QPointF>
#include <QString>
#include <QStringList>

// Contenu d'un schéma tactique, sans aucune dépendance graphique : c'est ce qui est stocké en JSON
// (colonne tactical_board.scene_json) et relu pour l'affichage, l'export PNG et la fiche PDF.
//
// Les coordonnées sont normalisées dans le terrain affiché : x et y vont de 0 à 1, dans le sens du jeu de
// l'équipe (qui attaque vers la droite de l'écran sur un terrain entier, vers le haut sur un demi-terrain).
//   x : profondeur. Terrain entier : 0 = ligne de but défendue, 1 = ligne de but visée.
//       Demi-terrain : 0 = ligne médiane, 1 = ligne de but visée.
//   y : largeur, de la gauche (0) vers la droite (1) du sens de jeu.
namespace Tactics {

enum class Pitch { Full, Half };          // terrain entier ou demi-terrain (moitié offensive, but visé en haut)
enum class Kind { Player, Opponent, Ball, Cone, Goal, Text };   // Text : note texte (le texte est dans label)
enum class ArrowStyle { Run, Pass, Dribble, Cross };   // course (pointillé), passe (trait plein), conduite (ondulé), centre (courbe)

struct Item
{
    Kind kind = Kind::Player;
    QPointF pos;                           // normalisée, 0..1
    QString label;                         // numéro ou texte court, peut être vide
    double rotation = 0;                   // degrés, sens horaire, de 0 inclus à 360 exclu (buts, plots, notes)
    double scale = 1.0;                    // taille relative, de kMinScale à kMaxScale (buts, plots, notes)
    QString color;                         // "#rrggbb", vide = couleur par défaut (joueurs, adversaires, plots, notes)
    bool operator==(const Item &o) const
    {
        return kind == o.kind && pos == o.pos && label == o.label && rotation == o.rotation && scale == o.scale
               && color == o.color;
    }
};

struct Arrow
{
    ArrowStyle style = ArrowStyle::Pass;
    QPointF from;                          // normalisée, 0..1
    QPointF to;
    // Centres seulement : flèche courbe, la courbure est la flèche du milieu du tracé par rapport à la corde,
    // en fraction de la longueur (de -kMaxCurve à kMaxCurve). Positif : bombée à gauche du sens de la flèche.
    double curve = 0;
    bool operator==(const Arrow &o) const
    {
        return style == o.style && from == o.from && to == o.to && curve == o.curve;
    }
};

struct Board
{
    Pitch pitch = Pitch::Full;
    QList<Item> items;
    QList<Arrow> arrows;
    bool operator==(const Board &o) const
    {
        return pitch == o.pitch && items == o.items && arrows == o.arrows;
    }
};

// Sérialisation. fromJson renvoie false (et remplit *error) si le texte n'est pas un schéma valide ;
// "{}" (valeur par défaut de la base) donne un schéma vide. Les coordonnées hors de [0, 1] sont ramenées dedans.
QString toJson(const Board &board);
bool fromJson(const QString &json, Board *board, QString *error = nullptr);

// Formations de départ (équipe qui attaque vers la droite, gardien à gauche, numéros 1 à 11).
QStringList formationNames();                       // "4-4-2", "4-3-3", "3-5-2", "4-2-3-1"
QList<Item> formation(const QString &name);         // vide si le nom est inconnu
// Même placement vu de l'autre côté du terrain (x et y inversés), type Opponent.
QList<Item> mirroredAsOpponent(const QList<Item> &players);

constexpr double kMaxCurve = 0.6;
constexpr double kDefaultCurve = 0.25;
constexpr double kMinScale = 0.3;
constexpr double kMaxScale = 4.0;

// Les buts, plots et notes peuvent pivoter et changer de taille ; les autres éléments ignorent ces champs.
inline bool isRotatable(Kind kind) { return kind == Kind::Goal || kind == Kind::Cone || kind == Kind::Text; }
inline bool isScalable(Kind kind) { return isRotatable(kind); }
// Joueurs, adversaires, plots et notes ont une couleur au choix ; ballons et buts gardent la leur.
inline bool isColorable(Kind kind)
{
    return kind == Kind::Player || kind == Kind::Opponent || kind == Kind::Cone || kind == Kind::Text;
}

// Couleurs proposées dans l'application : (nom, "#rrggbb").
QList<QPair<QString, QString>> colorPalette();

// Plus petit numéro (1 à 99) pas encore utilisé par un joueur de l'équipe ; 0 s'ils sont tous pris.
int nextFreePlayerNumber(const QList<Item> &items);

} // namespace Tactics

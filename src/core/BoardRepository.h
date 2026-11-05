#pragma once

#include <QList>
#include <QStringList>
#include <QSqlDatabase>
#include <QString>

// Un schéma tactique enregistré (table tactical_board). Le contenu est le JSON de Tactics::Board.
//
// Un schéma a un seul rôle :
//  - schéma de référence d'un exercice : exerciseId renseigné (au plus un par exercice) ;
//  - terrain d'un bloc de séance (une ligne du plan) : sessionExerciseId renseigné, exerciseId et sessionId vides ;
//    c'est une copie indépendante : le modifier ne touche jamais le schéma de référence ;
//  - schéma libre ou lié à une séance : sessionId éventuel.
struct BoardRecord
{
    int id = -1;
    QString title;
    QString formation;      // formation choisie en dernier (informatif), peut être vide
    QString sceneJson = QStringLiteral("{}");
    int exerciseId = -1;    // -1 : non lié
    int sessionId = -1;     // -1 : non lié
    int sessionExerciseId = -1;   // bloc de séance (session_exercise.id) dont c'est le terrain, -1 : aucun
    QString updatedAt;      // renseigné par la base (UTC, "yyyy-MM-dd HH:mm:ss")
};

class BoardRepository
{
public:
    explicit BoardRepository(const QSqlDatabase &db) : m_db(db) {}

    // Tous les schémas, triés par titre. Les terrains de blocs n'en font pas partie si includeBlockBoards est faux
    // (c'est la liste de la bibliothèque de schémas).
    QList<BoardRecord> list(QString *error = nullptr, bool includeBlockBoards = true) const;
    // Un schéma par son id. false (et message dans *error) s'il n'existe pas ou en cas d'erreur de la base.
    bool get(int id, BoardRecord *out, QString *error = nullptr) const;
    // Terrain d'un bloc. true s'il existe. false sans message : le bloc n'a pas de terrain ; false avec message :
    // erreur de la base.
    bool forBlock(int sessionExerciseId, BoardRecord *out, QString *error = nullptr) const;
    // Terrains de tous les blocs d'une séance, triés par id.
    QList<BoardRecord> forBlocksOfSession(int sessionId, QString *error = nullptr) const;
    // Schéma de référence d'un exercice. true s'il existe. Si d'anciennes données en comptent plusieurs (possible
    // avant le schéma 5), le plus récemment modifié est renvoyé. false sans message : pas de schéma de référence.
    bool referenceForExercise(int exerciseId, BoardRecord *out, QString *error = nullptr) const;
    // Schémas à joindre à la fiche d'une séance : ceux liés à la séance elle-même, ceux liés à un exercice de son
    // plan (schémas de référence) et les terrains de ses blocs. Triés par id, sans doublon.
    QList<BoardRecord> forSession(int sessionId, QString *error = nullptr) const;

    // renseigne board.id. Refus (rien n'est écrit, message dans *error) si :
    //  - le schéma est à la fois terrain de bloc et schéma de référence ou lié à une séance ;
    //  - le bloc a déjà un terrain ; l'exercice a déjà un schéma de référence.
    bool insert(BoardRecord &board, QString *error = nullptr);
    // Mêmes refus que insert(), mais seulement quand le lien (bloc ou exercice) change : on peut toujours enregistrer
    // le contenu d'un schéma ancien déjà lié.
    bool update(const BoardRecord &board, QString *error = nullptr);
    bool remove(int id, QString *error = nullptr);

    // Contrôle tous les terrains présents avec les règles de insert() (import de données). Un message par anomalie
    // dans *problems ; false s'il y en a au moins une, ou en cas d'erreur de la base. referenceUniqueness : vérifie
    // aussi « un seul schéma de référence par exercice » (faux pour les données d'avant le schéma 5, qui pouvaient
    // en compter plusieurs).
    bool validateAllLinks(bool referenceUniqueness, QStringList *problems, QString *error = nullptr) const;

private:
    bool checkLinks(const BoardRecord &board, int previousExerciseId, int previousBlockId, QString *error) const;

    QSqlDatabase m_db;
};

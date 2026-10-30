#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QString>

// Un schéma tactique enregistré (table tactical_board). Le contenu est le JSON de Tactics::Board.
struct BoardRecord
{
    int id = -1;
    QString title;
    QString formation;      // formation choisie en dernier (informatif), peut être vide
    QString sceneJson = QStringLiteral("{}");
    int exerciseId = -1;    // -1 : non lié
    int sessionId = -1;     // -1 : non lié
    QString updatedAt;      // renseigné par la base (UTC, "yyyy-MM-dd HH:mm:ss")
};

class BoardRepository
{
public:
    explicit BoardRepository(const QSqlDatabase &db) : m_db(db) {}

    // Tous les schémas, triés par titre.
    QList<BoardRecord> list(QString *error = nullptr) const;
    // Schémas à joindre à la fiche d'une séance : ceux liés à la séance elle-même et ceux liés à
    // un exercice de son plan. Triés par id, sans doublon.
    QList<BoardRecord> forSession(int sessionId, QString *error = nullptr) const;

    bool insert(BoardRecord &board, QString *error = nullptr);   // renseigne board.id
    bool update(const BoardRecord &board, QString *error = nullptr);
    bool remove(int id, QString *error = nullptr);

private:
    QSqlDatabase m_db;
};

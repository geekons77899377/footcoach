-- Schéma 6 : clubs (décision D1) et matchs (décision D2).
--  * Un club regroupe des équipes : team.club_id. Le club est obligatoire pour chaque équipe ; SQLite interdit d'ajouter
--    une colonne NOT NULL avec REFERENCES, la règle est donc appliquée par l'application (TeamRepository) et par
--    l'import de données. Les équipes déjà enregistrées sont rattachées à un club « Mon club », créé ici.
--  * Un match (football_match) est distinct d'une séance et d'un exercice : adversaire, lieu, domicile ou extérieur,
--    statut, score. Ses joueurs convoqués sont dans match_player. Son terrain tactique est un tactical_board dont
--    match_id est renseigné (au plus un par match ; ni schéma de référence, ni terrain de bloc, ni lié à une séance).
-- Contraintes du moteur de migration : un seul statement par ';', les lignes commençant par '--' sont ignorées.

CREATE TABLE club (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT    NOT NULL,
    created_at  TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO club(name) VALUES('Mon club');

ALTER TABLE team ADD COLUMN club_id INTEGER REFERENCES club(id) ON DELETE CASCADE;

UPDATE team SET club_id = (SELECT MIN(id) FROM club);

CREATE INDEX idx_team_club ON team(club_id);

CREATE TABLE football_match (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    team_id        INTEGER NOT NULL REFERENCES team(id) ON DELETE CASCADE,
    start_at       TEXT    NOT NULL,         -- ISO 8601 : YYYY-MM-DDTHH:MM
    opponent       TEXT    NOT NULL,
    location       TEXT,
    home_away      TEXT    NOT NULL DEFAULT 'home' CHECK (home_away IN ('home','away')),
    status         TEXT    NOT NULL DEFAULT 'planned' CHECK (status IN ('planned','played','cancelled')),
    score_for      INTEGER CHECK (score_for IS NULL OR score_for >= 0),
    score_against  INTEGER CHECK (score_against IS NULL OR score_against >= 0),
    notes          TEXT,
    created_at     TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_match_team_start ON football_match(team_id, start_at);

CREATE TABLE match_player (
    match_id   INTEGER NOT NULL REFERENCES football_match(id) ON DELETE CASCADE,
    player_id  INTEGER NOT NULL REFERENCES player(id) ON DELETE CASCADE,
    role       TEXT    NOT NULL DEFAULT 'starter' CHECK (role IN ('starter','substitute')),
    PRIMARY KEY (match_id, player_id)
);

CREATE INDEX idx_match_player_player ON match_player(player_id);

ALTER TABLE tactical_board
    ADD COLUMN match_id INTEGER
    REFERENCES football_match(id) ON DELETE CASCADE;

CREATE INDEX idx_board_match ON tactical_board(match_id);

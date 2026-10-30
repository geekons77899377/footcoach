-- FootCoach : schéma initial (version 1)
-- Contraintes d'écriture pour le moteur de migration :
--  * un seul statement par ';' (pas de TRIGGER ni de corps de statement contenant ';')
--  * les lignes de commentaire commençant par '--' sont ignorées

CREATE TABLE team (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT    NOT NULL,
    category    TEXT,                       -- ex. U15, Seniors
    season      TEXT,                       -- ex. 2026-2027
    created_at  TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE player (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    team_id         INTEGER NOT NULL REFERENCES team(id) ON DELETE CASCADE,
    first_name      TEXT    NOT NULL,
    last_name       TEXT    NOT NULL,
    birth_date      TEXT,                   -- ISO 8601 : YYYY-MM-DD
    jersey_number   INTEGER CHECK (jersey_number IS NULL OR jersey_number BETWEEN 1 AND 99),
    position        TEXT    CHECK (position IN ('GK','DEF','MID','FWD')),
    preferred_foot  TEXT    CHECK (preferred_foot IN ('R','L','B')),
    phone           TEXT,
    email           TEXT,
    notes           TEXT,
    active          INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0,1)),
    created_at      TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_player_team ON player(team_id);

CREATE TABLE training_session (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    team_id       INTEGER NOT NULL REFERENCES team(id) ON DELETE CASCADE,
    start_at      TEXT    NOT NULL,         -- ISO 8601 : YYYY-MM-DDTHH:MM
    duration_min  INTEGER NOT NULL DEFAULT 90 CHECK (duration_min > 0),
    location      TEXT,
    theme         TEXT,
    notes         TEXT,
    status        TEXT    NOT NULL DEFAULT 'planned'
                  CHECK (status IN ('planned','done','cancelled')),
    created_at    TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_session_team_start ON training_session(team_id, start_at);

CREATE TABLE attendance (
    session_id  INTEGER NOT NULL REFERENCES training_session(id) ON DELETE CASCADE,
    player_id   INTEGER NOT NULL REFERENCES player(id) ON DELETE CASCADE,
    status      TEXT    NOT NULL DEFAULT 'present'
                CHECK (status IN ('present','absent','excused','injured')),
    note        TEXT,
    PRIMARY KEY (session_id, player_id)
);

CREATE INDEX idx_attendance_player ON attendance(player_id);

CREATE TABLE exercise (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    title        TEXT    NOT NULL,
    category     TEXT    NOT NULL DEFAULT 'technical'
                 CHECK (category IN ('warmup','technical','tactical','physical','game','cooldown')),
    objective    TEXT,
    duration_min INTEGER CHECK (duration_min IS NULL OR duration_min > 0),
    players_min  INTEGER CHECK (players_min IS NULL OR players_min > 0),
    players_max  INTEGER CHECK (players_max IS NULL OR players_max > 0),
    equipment    TEXT,
    description  TEXT,
    created_at   TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE session_exercise (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id    INTEGER NOT NULL REFERENCES training_session(id) ON DELETE CASCADE,
    exercise_id   INTEGER NOT NULL REFERENCES exercise(id) ON DELETE RESTRICT,
    position      INTEGER NOT NULL,         -- ordre dans la séance, à partir de 1
    duration_min  INTEGER CHECK (duration_min IS NULL OR duration_min > 0),
    notes         TEXT,
    UNIQUE (session_id, position)
);

CREATE INDEX idx_session_exercise_exercise ON session_exercise(exercise_id);

-- Schéma tactique : l'état de la QGraphicsScene est sérialisé en JSON (scene_json).
CREATE TABLE tactical_board (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    title        TEXT    NOT NULL,
    formation    TEXT,                      -- ex. 4-3-3
    scene_json   TEXT    NOT NULL DEFAULT '{}',
    thumbnail    BLOB,                      -- aperçu PNG
    exercise_id  INTEGER REFERENCES exercise(id) ON DELETE SET NULL,
    session_id   INTEGER REFERENCES training_session(id) ON DELETE SET NULL,
    created_at   TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at   TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_board_exercise ON tactical_board(exercise_id);
CREATE INDEX idx_board_session  ON tactical_board(session_id);

CREATE TABLE injury (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    player_id    INTEGER NOT NULL REFERENCES player(id) ON DELETE CASCADE,
    start_date   TEXT    NOT NULL,
    end_date     TEXT,                      -- NULL = toujours indisponible
    description  TEXT
);

CREATE INDEX idx_injury_player ON injury(player_id);

-- Schéma 10 : créneaux d'entraînement réguliers, présence des convoqués d'un match, contacts des parents.
-- Contraintes du moteur de migration : ni point-virgule ni double tiret dans les textes, un statement par point-virgule.

-- Créneau régulier : règle qui a généré des séances (jours, heure, durée, lieu, thème, période).
-- weekdays : jours séparés par des virgules, 1 = lundi … 7 = dimanche (ex. « 2,4 »).
-- start_time : « HH:MM ». first_day et last_day : « YYYY-MM-DD », inclus.
CREATE TABLE training_slot (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    team_id       INTEGER NOT NULL REFERENCES team(id) ON DELETE CASCADE,
    weekdays      TEXT    NOT NULL,
    start_time    TEXT    NOT NULL,
    duration_min  INTEGER NOT NULL DEFAULT 90 CHECK (duration_min > 0),
    location      TEXT,
    theme         TEXT,
    first_day     TEXT    NOT NULL,
    last_day      TEXT    NOT NULL,
    created_at    TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_slot_team ON training_slot(team_id);

-- Séance générée par un créneau. Créneau supprimé : les séances conservées deviennent des séances seules.
ALTER TABLE training_session ADD COLUMN slot_id INTEGER REFERENCES training_slot(id) ON DELETE SET NULL;

CREATE INDEX idx_session_slot ON training_session(slot_id);

-- Présence d'un convoqué : réponse avant le match, puis appel le jour du match.
ALTER TABLE match_player ADD COLUMN presence TEXT NOT NULL DEFAULT 'unknown'
    CHECK (presence IN ('unknown','present','absent','excused','injured'));

-- Deux contacts au plus par joueur (parents) : rang 1 ou 2.
CREATE TABLE player_contact (
    player_id  INTEGER NOT NULL REFERENCES player(id) ON DELETE CASCADE,
    rank       INTEGER NOT NULL CHECK (rank IN (1, 2)),
    label      TEXT,
    phone      TEXT,
    email      TEXT,
    PRIMARY KEY (player_id, rank)
);

-- Les colonnes player.phone et player.email (schéma 1) deviennent le contact 1.
INSERT INTO player_contact(player_id, rank, phone, email)
    SELECT id, 1, NULLIF(trim(phone), ''), NULLIF(trim(email), '') FROM player
     WHERE NULLIF(trim(phone), '') IS NOT NULL OR NULLIF(trim(email), '') IS NOT NULL;

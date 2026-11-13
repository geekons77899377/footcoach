-- Schéma 11 : lieux de match (stades) d'un club.
--  * Un club a un département (code « 60 », « 2A »…) : il sert à charger les terrains de football de Data ES
--    (recensement des équipements sportifs du ministère des Sports) depuis les Paramètres.
--  * venue : un lieu du club. Origine « data_es » (identifiant equip_numero dans source_id) ou « manual » (saisi
--    à la main). Un lieu n'est jamais supprimé par une mise à jour : es_status passe à « removed » (absent de Data ES)
--    ou « closed » (installation hors service). unavailable : l'entraîneur l'a marqué indisponible lui-même.
--    is_home : stade du club pour les matchs à domicile (un seul par club, contrôlé par l'index unique).
--  * opponent_venue : stade retenu pour un adversaire (nom normalisé dans opponent_key).
--  * football_match.venue_id : lieu choisi. Le texte de football_match.location reste la copie de l'adresse, pour que
--    le match garde son lieu si le stade est supprimé.
-- Contraintes du moteur de migration : ni point-virgule ni double tiret dans les textes, un statement par point-virgule.

ALTER TABLE club ADD COLUMN department TEXT;

CREATE TABLE venue (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    club_id      INTEGER NOT NULL REFERENCES club(id) ON DELETE CASCADE,
    name         TEXT    NOT NULL,
    field        TEXT,
    address      TEXT,
    postcode     TEXT,
    city         TEXT,
    lat          REAL,
    lon          REAL,
    source       TEXT    NOT NULL DEFAULT 'manual' CHECK (source IN ('manual','data_es')),
    source_id    TEXT,
    es_status    TEXT    NOT NULL DEFAULT 'ok' CHECK (es_status IN ('ok','removed','closed')),
    unavailable  INTEGER NOT NULL DEFAULT 0 CHECK (unavailable IN (0,1)),
    is_home      INTEGER NOT NULL DEFAULT 0 CHECK (is_home IN (0,1)),
    updated_at   TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_venue_club ON venue(club_id);

CREATE UNIQUE INDEX idx_venue_source ON venue(club_id, source_id) WHERE source_id IS NOT NULL;

CREATE UNIQUE INDEX idx_venue_home ON venue(club_id) WHERE is_home = 1;

CREATE TABLE opponent_venue (
    club_id       INTEGER NOT NULL REFERENCES club(id) ON DELETE CASCADE,
    opponent_key  TEXT    NOT NULL,
    opponent      TEXT    NOT NULL,
    venue_id      INTEGER NOT NULL REFERENCES venue(id) ON DELETE CASCADE,
    PRIMARY KEY (club_id, opponent_key)
);

ALTER TABLE football_match ADD COLUMN venue_id INTEGER REFERENCES venue(id) ON DELETE SET NULL;

CREATE INDEX idx_match_venue ON football_match(venue_id);

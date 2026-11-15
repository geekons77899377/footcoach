-- Schéma 12 : éducateurs d'un club, éducateur responsable d'une équipe.
--  * educator : un éducateur du club (prénom, nom, n° de licence facultatif). Un éducateur peut suivre plusieurs équipes
--    du même club. La licence est unique dans le club (casse ignorée) quand elle est renseignée.
--  * team.educator_id : éducateur responsable de l'équipe, imprimé dans « Nom Responsable » et « N° de licence » de la
--    feuille de challenge. NULL : aucun (cases laissées vides). Supprimer l'éducateur vide la colonne.
-- Contraintes du moteur de migration : ni point-virgule ni double tiret dans les textes, un statement par point-virgule.

CREATE TABLE educator (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    club_id     INTEGER NOT NULL REFERENCES club(id) ON DELETE CASCADE,
    first_name  TEXT    NOT NULL,
    last_name   TEXT    NOT NULL,
    licence     TEXT,
    created_at  TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_educator_club ON educator(club_id);

CREATE UNIQUE INDEX idx_educator_club_licence ON educator(club_id, licence COLLATE NOCASE) WHERE licence IS NOT NULL;

ALTER TABLE team ADD COLUMN educator_id INTEGER REFERENCES educator(id) ON DELETE SET NULL;

CREATE INDEX idx_team_educator ON team(educator_id);

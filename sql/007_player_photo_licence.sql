-- Schéma 7 : numéro de licence et photo d'un joueur.
--  * licence : texte facultatif (lettres et chiffres, 20 caractères au plus), unique dans l'équipe sans tenir compte de
--    la casse. L'unicité est contrôlée par l'application (PlayerRepository::licenceTaken) et par l'import de données.
--  * photo : image JPEG réduite (côté le plus long 480 px), stockée dans la base ; NULL = pas de photo.
-- Contraintes du moteur de migration : un seul statement par ';', les lignes commençant par '--' sont ignorées.

ALTER TABLE player ADD COLUMN licence TEXT;

ALTER TABLE player ADD COLUMN photo BLOB;

-- Filet de sécurité de la règle « une licence par joueur dans l'équipe » (casse ignorée), valable aussi pour l'import.
CREATE UNIQUE INDEX idx_player_team_licence ON player(team_id, licence COLLATE NOCASE) WHERE licence IS NOT NULL;

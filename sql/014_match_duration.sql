-- Schéma 14 : durée d'un match (minutes). Décision du 15/11/2026 : un joueur peut jouer un autre match qui ne
-- chevauche pas la durée du premier ; durée par défaut 30 min pour les U10-U11, 90 min sinon.
-- Les matchs existants reçoivent la durée par défaut de la catégorie de leur équipe (U10, U11, U10-U11, U10U11…).
-- Contraintes du moteur de migration : ni point-virgule ni double tiret dans les textes, un statement par point-virgule.

ALTER TABLE football_match ADD COLUMN duration_min INTEGER NOT NULL DEFAULT 90 CHECK (duration_min BETWEEN 1 AND 600);

UPDATE football_match SET duration_min = 30
WHERE team_id IN (SELECT id FROM team WHERE upper(category) LIKE '%U10%' OR upper(category) LIKE '%U11%');

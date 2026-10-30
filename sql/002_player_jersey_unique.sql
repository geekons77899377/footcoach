-- Un numéro de maillot ne peut être porté que par un seul joueur actif d'une même équipe.
-- Index partiel : les joueurs désactivés libèrent leur numéro, et l'absence de numéro est autorisée.
CREATE UNIQUE INDEX idx_player_team_jersey
    ON player(team_id, jersey_number)
    WHERE jersey_number IS NOT NULL AND active = 1;

-- Schéma 5 : un terrain tactique propre à un bloc de séance (une ligne du plan, table session_exercise).
-- Le terrain d'un bloc est indépendant du schéma de référence de l'exercice (copié à l'ajout, jamais lié ensuite).
-- Supprimer un bloc, ou la séance qui le contient, supprime son terrain ; les schémas de référence ne sont pas touchés.
-- Les terrains déjà enregistrés gardent la valeur NULL : aucune donnée existante n'est modifiée.
ALTER TABLE tactical_board
    ADD COLUMN session_exercise_id INTEGER
    REFERENCES session_exercise(id) ON DELETE CASCADE;

CREATE INDEX idx_board_session_exercise
    ON tactical_board(session_exercise_id);

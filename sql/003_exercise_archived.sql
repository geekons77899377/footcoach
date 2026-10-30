-- Un exercice utilisé dans un plan de séance ne peut pas être supprimé (clé étrangère RESTRICT) :
-- on l'archive. Il disparaît de la bibliothèque mais reste visible dans les anciens plans.
ALTER TABLE exercise ADD COLUMN archived INTEGER NOT NULL DEFAULT 0 CHECK (archived IN (0,1));

-- Catégories d'exercices définies par l'utilisateur (ex. U10-U11), distinctes de l'activité (échauffement, technique...).
-- Contraintes du moteur de migration : ni point-virgule ni double tiret dans les textes.

CREATE TABLE exercise_category (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    name       TEXT    NOT NULL COLLATE NOCASE UNIQUE,
    created_at TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

ALTER TABLE exercise ADD COLUMN category_id INTEGER REFERENCES exercise_category(id) ON DELETE SET NULL;

CREATE INDEX idx_exercise_category ON exercise(category_id);

INSERT INTO exercise_category(name) VALUES ('U10-U11');

UPDATE exercise
   SET category_id = (SELECT id FROM exercise_category WHERE name = 'U10-U11'),
       title = substr(title, 11)
 WHERE title LIKE 'U10-U11 · %';

UPDATE tactical_board
   SET title = substr(title, 11)
 WHERE exercise_id IN (SELECT id FROM exercise WHERE category_id IS NOT NULL)
   AND title LIKE 'U10-U11 · %';

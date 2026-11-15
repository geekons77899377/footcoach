-- Schéma 13 : rubriques de la fiche exercice (consignes, évolutions, critères de réussite, consigne éducateur),
-- une ligne par point, imprimées sur la fiche exercice (PDF A4 paysage). Ajoute l'exercice « Le parcours des portes ».
-- Source de l'exercice : fiche fournie par l'éducateur le 15/11/2026. Placement des portes repris à l'œil de sa fiche.
-- Contraintes du moteur de migration : ni point-virgule ni double tiret dans les textes, un statement par point-virgule.

ALTER TABLE exercise ADD COLUMN instructions TEXT;

ALTER TABLE exercise ADD COLUMN variations TEXT;

ALTER TABLE exercise ADD COLUMN success_criteria TEXT;

ALTER TABLE exercise ADD COLUMN coach_cue TEXT;

INSERT OR IGNORE INTO exercise_category(name) VALUES ('U10-U11');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description,
                     category_id, instructions, variations, success_criteria, coach_cue) VALUES
('Le parcours des portes', 'technical', 'Conduite de balle + lever la tête pour choisir la prochaine porte', 17, 8, 12,
 '1 ballon par joueur, 10 à 12 portes (coupelles de couleurs différentes)',
 'Terrain : 20 x 20 m',
 (SELECT id FROM exercise_category WHERE name = 'U10-U11'),
 'Conduire librement son ballon.
Traverser un maximum de portes.
Lever la tête pour repérer la prochaine porte.
Ne pas passer deux fois de suite dans la même porte.
Garder le ballon proche des pieds.',
 'Passer uniquement dans les portes rouges.
L''éducateur annonce une couleur à rejoindre.
Accélérer après chaque porte.
Varier les directions.',
 'Lever régulièrement la tête.
Choisir une porte avant de s''y engager.
Garder la maîtrise du ballon.
Enchaîner les passages sans collision.',
 'Regarde devant toi, trouve ta prochaine porte et maîtrise ton ballon !');

INSERT INTO tactical_board(title, formation, scene_json, exercise_id) VALUES
('Le parcours des portes', NULL, '{"version":1,"pitch":"half","items":[{"kind":"cone","x":0.5,"y":0.353,"label":"","rot":0,"scale":1,"color":"#fafafa"},{"kind":"cone","x":0.5,"y":0.647,"label":"","rot":0,"scale":1,"color":"#fafafa"},{"kind":"cone","x":0.119,"y":0.647,"label":"","rot":0,"scale":1,"color":"#fafafa"},{"kind":"cone","x":0.119,"y":0.353,"label":"","rot":0,"scale":1,"color":"#fafafa"},{"kind":"cone","x":0.456,"y":0.41,"label":"","rot":0,"scale":1,"color":"#c62828"},{"kind":"cone","x":0.456,"y":0.44,"label":"","rot":0,"scale":1,"color":"#c62828"},{"kind":"cone","x":0.46,"y":0.574,"label":"","rot":0,"scale":1,"color":"#fbc02d"},{"kind":"cone","x":0.46,"y":0.603,"label":"","rot":0,"scale":1,"color":"#fbc02d"},{"kind":"cone","x":0.391,"y":0.599,"label":"","rot":0,"scale":1,"color":"#1565c0"},{"kind":"cone","x":0.391,"y":0.628,"label":"","rot":0,"scale":1,"color":"#1565c0"},{"kind":"cone","x":0.35,"y":0.412,"label":"","rot":0,"scale":1,"color":"#1565c0"},{"kind":"cone","x":0.35,"y":0.441,"label":"","rot":0,"scale":1,"color":"#1565c0"},{"kind":"cone","x":0.315,"y":0.485,"label":"","rot":0,"scale":1,"color":"#ef6c00"},{"kind":"cone","x":0.315,"y":0.515,"label":"","rot":0,"scale":1,"color":"#ef6c00"},{"kind":"cone","x":0.321,"y":0.559,"label":"","rot":0,"scale":1,"color":"#fbc02d"},{"kind":"cone","x":0.321,"y":0.588,"label":"","rot":0,"scale":1,"color":"#fbc02d"},{"kind":"cone","x":0.241,"y":0.406,"label":"","rot":0,"scale":1,"color":"#fbc02d"},{"kind":"cone","x":0.241,"y":0.435,"label":"","rot":0,"scale":1,"color":"#fbc02d"},{"kind":"cone","x":0.245,"y":0.521,"label":"","rot":0,"scale":1,"color":"#1565c0"},{"kind":"cone","x":0.245,"y":0.55,"label":"","rot":0,"scale":1,"color":"#1565c0"},{"kind":"cone","x":0.228,"y":0.604,"label":"","rot":0,"scale":1,"color":"#ef6c00"},{"kind":"cone","x":0.228,"y":0.634,"label":"","rot":0,"scale":1,"color":"#ef6c00"},{"kind":"cone","x":0.155,"y":0.496,"label":"","rot":0,"scale":1,"color":"#ef6c00"},{"kind":"cone","x":0.155,"y":0.525,"label":"","rot":0,"scale":1,"color":"#ef6c00"},{"kind":"cone","x":0.428,"y":0.516,"label":"","rot":0,"scale":1,"color":"#c62828"},{"kind":"cone","x":0.428,"y":0.546,"label":"","rot":0,"scale":1,"color":"#c62828"},{"kind":"player","x":0.169,"y":0.382,"label":"","color":"#1565c0"},{"kind":"ball","x":0.161,"y":0.393,"label":""},{"kind":"player","x":0.433,"y":0.531,"label":"","color":"#1565c0"},{"kind":"ball","x":0.426,"y":0.538,"label":""},{"kind":"text","x":0.138,"y":0.421,"label":"Départ","rot":0,"scale":1}],"arrows":[{"style":"dribble","x1":0.191,"y1":0.391,"x2":0.336,"y2":0.425,"curve":0},{"style":"dribble","x1":0.359,"y1":0.438,"x2":0.416,"y2":0.521,"curve":0},{"style":"dribble","x1":0.409,"y1":0.541,"x2":0.329,"y2":0.571,"curve":0},{"style":"dribble","x1":0.302,"y1":0.577,"x2":0.169,"y2":0.515,"curve":0}]}', (SELECT id FROM exercise WHERE title = 'Le parcours des portes' ORDER BY id DESC LIMIT 1));

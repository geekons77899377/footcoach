-- Exercices génériques de départ (football à 11). Modifiables, archivables ou supprimables.
-- Contraintes du moteur de migration : ni point-virgule ni double tiret dans les textes.

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Échauffement avec ballon', 'warmup', 'Monter progressivement en intensité et se mettre en rythme avec le ballon', 10, 8, 22, 'Ballons, plots',
 'Un ballon pour deux. Footing en conduite libre dans un carré de 30 m, puis passes à deux en mouvement, changements de rythme et de direction. Finir par quelques accélérations de 10 m.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Mobilité et activation dynamique', 'warmup', 'Préparer les articulations et les muscles avant l''effort', 10, 6, 25, 'Plots',
 'Parcours de 20 m à parcourir en enchaînant montées de genoux, talons-fesses, pas chassés, fentes marchées et rotations de bassin. Deux à trois allers-retours, sans ballon.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Rondo 4 contre 1', 'technical', 'Améliorer la qualité de passe, l''orientation du corps et la prise d''information', 12, 5, 10, 'Ballons, plots, chasubles',
 'Carré de 8 x 8 m. Quatre joueurs gardent le ballon, un défenseur le récupère. Celui qui perd le ballon ou dont la passe est interceptée prend la place du défenseur. Limiter à deux touches de balle.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Rondo 5 contre 2', 'technical', 'Circulation rapide du ballon sous pression et jeu en une touche', 15, 7, 14, 'Ballons, plots, chasubles',
 'Carré de 10 x 10 m. Cinq joueurs contre deux défenseurs. Défi : réussir dix passes consécutives. Variante : une touche obligatoire pour les joueurs extérieurs.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Conduite de balle en slalom', 'technical', 'Maîtriser la conduite, les changements de direction et le contrôle orienté', 10, 4, 20, 'Ballons, plots',
 'Deux files, un parcours de plots en slalom sur 15 m. Conduite pied droit, pied gauche, puis semelle. Retour en passe au partenaire suivant. Augmenter la vitesse à chaque passage.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Passes et appuis en triangle', 'technical', 'Enchaîner passe, déplacement et contrôle orienté', 12, 6, 18, 'Ballons, plots',
 'Trois plots en triangle de 10 m, un joueur à chaque sommet. Passe et suit son ballon, contrôle orienté puis passe. Progressions : une touche, passe longue, passe sur le côté faible.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Frappes après remise', 'technical', 'Travailler la précision et la puissance de frappe dans un enchaînement', 15, 6, 16, 'Ballons, buts, plots',
 'Départ d''une passe vers un attaquant qui remet d''une touche, frappe en première intention face au gardien. Alterner pied droit et pied gauche, puis changer les rôles.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Finition 1 contre 1 face au gardien', 'technical', 'Gagner son duel et conclure avec sang-froid', 15, 6, 16, 'Ballons, buts, chasubles',
 'Un attaquant reçoit une passe à 25 m du but, un défenseur part de sa hauteur. Duel jusqu''à la frappe, le gardien est actif. Compter les buts, rotation des rôles après chaque passage.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Centres et finition', 'tactical', 'Coordonner appels des attaquants et qualité du centre', 20, 8, 16, 'Ballons, buts, plots, chasubles',
 'Deux côtiers centrent alternativement, deux attaquants font des appels au premier et au second poteau, un milieu arrive en retrait. Un défenseur gêne en opposition progressive. Finir en une touche.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Jeu à 4 contre 4 sur petit terrain', 'game', 'Multiplier les duels et les situations de jeu en espace réduit', 20, 8, 16, 'Ballons, mini-buts, plots, chasubles',
 'Terrain de 30 x 20 m avec deux mini-buts. Matchs de quatre minutes, un point par but. Ballon sorti : remise en jeu au pied. Rotation des équipes à chaque match.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Possession 8 contre 8 avec deux jokers', 'tactical', 'Conserver le ballon et créer des supériorités', 20, 16, 22, 'Ballons, plots, chasubles de trois couleurs',
 'Terrain de 40 x 30 m. Deux équipes de huit et deux jokers qui jouent toujours avec l''équipe qui a le ballon. Objectif : enchaîner douze passes pour marquer un point. Limiter les touches si la possession est trop facile.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Sortie de balle contre pressing', 'tactical', 'Relancer proprement depuis le gardien face à un pressing haut', 20, 14, 22, 'Ballons, buts, plots, chasubles',
 'Demi-terrain. Gardien et quatre défenseurs, deux milieux contre cinq attaquants qui pressent. Objectif : franchir la ligne médiane avec le ballon. Consignes de placement : largeur, soutien du gardien, jeu vers l''espace libre.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Transition défense vers attaque', 'tactical', 'Réagir vite à la récupération du ballon et attaquer l''espace', 15, 12, 20, 'Ballons, buts, plots, chasubles',
 'Une équipe attaque, l''autre défend. À la récupération, l''équipe qui défendait doit marquer en moins de dix secondes dans le but opposé. Insister sur la première passe vers l''avant.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Circuit intermittent 30 secondes', 'physical', 'Développer l''endurance intermittente et la capacité à répéter les efforts', 15, 6, 25, 'Plots, chronomètre',
 'Course de 30 secondes à allure vive autour d''un carré de 25 m, puis 30 secondes de récupération active. Huit à dix répétitions en deux séries, trois minutes de repos entre les séries.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Relais de vitesse avec ballon', 'physical', 'Travailler la vitesse et la coordination balle au pied', 10, 8, 24, 'Ballons, plots',
 'Équipes de quatre en relais sur 20 m avec conduite de balle, retour en courant. Variantes : slalom, passe au partenaire en arrivant. L''équipe la plus rapide marque un point.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Match à thème 11 contre 11', 'game', 'Appliquer en situation réelle les principes travaillés pendant la séance', 30, 16, 22, 'Ballons, buts, chasubles',
 'Match sur terrain complet ou à 70 pour cent du terrain avec une consigne de jeu : par exemple relance courte, pressing après perte, ou jeu dans le dos de la défense. Arrêts de jeu brefs pour corriger.');

INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, equipment, description) VALUES
('Étirements et retour au calme', 'cooldown', 'Favoriser la récupération et le retour au calme', 10, 1, 30, 'Aucun',
 'Trot lent de deux minutes, puis étirements passifs de 20 à 30 secondes pour les quadriceps, ischio-jambiers, mollets, adducteurs et fessiers. Respiration lente, debrief rapide de la séance.');

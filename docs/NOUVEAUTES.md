# Nouveautés de la branche `dev`

Tout ce qui a été ajouté par rapport à `origin/dev` (dernier commit commun : « Port macOS + musique »), dans l'ordre où
c'est arrivé. Chaque fonctionnalité a été faite sur sa propre branche (colonne « Branche »), puis tout a été réuni dans
`dev` avec les nouveautés d'`origin/dev` (bouton MISE A JOUR, musique stéréo, CI Windows et release).

Pour chaque point : ce que ça change en jeu, comment le régler, où est le code, comment le vérifier.
Le détail technique complet est dans [`CODE.md`](CODE.md) (numéros de section entre parenthèses).

| # | Fonctionnalité | Branche |
|---|---|---|
| 1 | Rendu, jump pads, railgun, bots, aimant de visée | (avant les branches) |
| 2 | Mode équipes 1v1 à 4v4, éclairage, ombres, bloom | (avant les branches) |
| 3 | Réglages graphiques modifiables sans recompiler | `graphismes-modifiables` |
| 4 | Skins, effets du monde, néons, météo, ambiances, qualité | `skins-effets-ambiances` |
| 5 | Molette : un geste = un changement d'arme | `molette-changement-arme` |
| 6 | Code rangé en dossiers (`src/`) | `arborescence-fichiers` |
| 7 | Changement d'arme visible | `retour-changement-arme` |
| 8 | Barre de vie avec traînée des dégâts | `hud-barre-de-vie` |
| 9 | Vrai modèle de lance-roquettes | `modele-lance-roquettes` |
| 10 | Second écran : carte et statistiques en direct | `second-ecran-carte` |
| 11 | Carte : seulement soi et ses équipiers | `carte-equipiers` |
| 12 | Carte en 3D inclinée (ou de dessus) | `carte-vue-3d` |
| 13 | Courbe de vitesse | `courbe-vitesse` |
| 14 | Carte de chaleur de fin de match | `carte-chaleur` |
| 15 | Fonds sous les textes du HUD | `hud-fonds` |
| 16 | Caméra de mort | `camera-de-mort` |
| 17 | Réunion avec `origin/dev` (mise à jour, stéréo, CI) | `dev` |

---

## 1. Rendu, jump pads, railgun, bots, aimant de visée

- **Décor** : textures générées plus riches (sol et murs séparés, joints biseautés, boulons, salissures, grain fin) ;
  les faces cachées des blocs ne sont plus dessinées.
- **Menu principal** : fond en dégradé (le décor se voit en bas), halo sur le titre, panneau derrière les boutons.
- **Jump pads** : vrai bug corrigé. En bhop on arrive sur le pad en l'air, et le pad (0,3 m) se comportait comme un
  mur au lieu de propulser. Deux tests de régression (au sol et en l'air à grande vitesse).
- **Railgun en main** : canon octogonal, frein de bouche, bague d'énergie, lunette (générateur de prismes réutilisable,
  `prism_z`).
- **Bots** : le niveau facile est vraiment plus imprécis et réagit plus lentement ; son bhop n'est plus parfait.
- **Aimant de visée** (joueur humain seulement) : correction très douce vers une cible proche du viseur (cône d'environ
  4°, ligne de vue vérifiée), jamais de verrouillage.

## 2. Mode équipes 1v1 à 4v4, éclairage, ombres, bloom

- **Équipes** (§3.11) : l'hôte choisit 1v1 à 4v4 dans HEBERGER ; équipes équilibrées à l'arrivée et rééquilibrées entre
  deux matchs, pas de tir allié, score et victoire par équipe, couleurs bleue / rouge, radar des alliés.
- **Éclairage** (§3.12) : lightmaps précalculées avec occlusion ambiante, ombres dynamiques des joueurs, ciel dégradé
  avec soleil et étoiles, ville au loin, bloom autour de ce qui brille.

## 3. Réglages graphiques modifiables sans recompiler

- Tout l'aspect visuel se règle dans **`graphismes/rendu.ini`** : une trentaine de clés commentées (soleil, ombres,
  occlusion, brouillard, ciel, ville, bloom, textures…). `sol.bmp`, `mur.bmp`, `grain.bmp` (facultatifs) remplacent les
  textures générées.
- **F5 en jeu** relit tout et refait l'éclairage sans relancer (« GRAPHISMES RECHARGES »).
- Le gameplay et le réseau ne lisent jamais ce dossier : deux joueurs avec des réglages différents jouent ensemble.
- Code : `src/graphismes/rendu_ini.c` (§3.13). Mode d'emploi : `graphismes/README.md`.

## 4. Skins, effets du monde, néons, météo, ambiances, qualité

- **5 skins** (ROBOT, POUSSIN, NINJA, CHAT, CHEVALIER) : écran SKIN du menu avec aperçu 3D, couleur de l'équipe,
  animation de course, de saut, de glissade et de visée. Envoyé aux autres joueurs (`NET_VER` 13). Code :
  `src/perso/skins.c`.
- **Effets du monde** : lumières dynamiques (explosions, rail, roquettes en vol), marques d'impact, ondes au sol, fumée,
  éclats à la couleur de l'équipe au kill.
- **Néons** sur les plateformes, les murs de wall-run et le centre, avec leur lueur sur le décor ; **rebond coloré** de
  la lumière.
- **Ciel et météo** : nuages, poussière, pluie, orage (éclairs et tonnerre), reflets du soleil.
- **Ambiances** (PARAMETRES > CIEL) : couchant, nuit, jour, orage (`graphismes/ambiance_*.ini`).
- **Qualité** (PARAMETRES > QUALITE) : BASSE / MOYENNE / ULTRA pour les petites machines.
- **Contours** colorés autour des joueurs et **traînées de vitesse** au-delà de 16 m/s.
- Détails : §3.14.

## 5. Molette : un geste = un changement d'arme

- **Avant** : une rotation de molette envoie des dizaines d'événements (roue libre, inertie du pavé tactile ou de la
  Magic Mouse) et chacun inversait les deux armes : l'arme clignotait pendant environ 2 s.
- **Maintenant** : une rafale d'événements ne compte qu'une fois. Il faut 0,15 s de repos entre deux gestes
  (`WHEEL_QUIET`) et 0,3 s entre deux changements (`WHEEL_COOLDOWN`) ; sur macOS la phase d'inertie est ignorée.
- Code : `wheel_accept` dans `src/plateforme/fenetre.c`. Test : « molette : rafale de 2 s=1 ».

## 6. Code rangé en dossiers

- `main.c` (16 000 lignes) est devenu une **table des matières** qui inclut, dans l'ordre, les fichiers de `src/`
  rangés par thème : `armes/` (une arme = un fichier), `perso/` (skins), `reglages/` (tous les `#define`), `rendu/`,
  `interface/`, `reseau/`, `son/`, `plateforme/` (dont `mac/`), `tests/`…
- Le code compilé est **identique** (sortie du préprocesseur comparée avant / après). Les scripts de compilation ne
  compilent toujours que `main.c`.
- Arborescence complète et rôle de chaque fichier : §2 de `CODE.md` et `README.md`.

## 7. Changement d'arme visible

- Le **nom de l'arme** apparaît sous le viseur (à sa couleur, avec une barre qui se vide) puis s'efface en 0,9 s
  (`WP_FLASH_T`).
- L'**emplacement** choisi s'allume dans la barre des armes en bas à droite.
- L'**arme remonte en pivotant** au lieu d'apparaître d'un coup.
- Code : `weapon_select` (`wpFlashAt`), `draw_hud`, `draw_viewmodel`. Capture : `--shot changearme.bmp`.

## 8. Barre de vie avec traînée des dégâts

- Après un coup, la partie perdue reste affichée **en blanc**, devient rouge et **se vide** après 0,4 s
  (`HP_GHOST_HOLD`, `HP_GHOST_RATE`) : on voit combien le coup a retiré.
- **Éclat vert** quand on se soigne, **graduations** tous les 25 PV, reflet sur le haut de la barre.
- Code : `hp_ghost_tick` dans `src/interface/hud.c`. Capture : `--shot vie.bmp`.

## 9. Vrai modèle de lance-roquettes

- Ce n'était qu'une boîte verte. Il a maintenant un tube à facettes, une bouche évasée, une tuyère, des colliers, un
  viseur lumineux, des poignées, des bandes chaudes, et l'ogive de la roquette chargée dans la bouche.
- **Une LED par roquette** sur le dessus : on voit ses munitions sans regarder le HUD.
- Code : `draw_viewmodel` dans `src/rendu/viewmodel.c`.

## 10. Second écran : carte et statistiques en direct

- Si **un deuxième écran est branché**, une fenêtre plein écran s'y ouvre toute seule. Elle ne prend jamais la souris
  ni le clavier du jeu. Le jeu recompte les écrans chaque seconde : brancher ou débrancher en cours de partie est suivi.
- **À gauche, la carte** : blocs (plus clairs quand ils sont hauts), jump pads, recharges, pièces, roquettes en vol,
  ennemis en solo ; chaque joueur est une flèche à sa couleur, avec son nom, ses PV et 2 s de trajet ; toi, avec ton
  cône de vision.
- **À droite, les stats** : mode, chrono, limite de frags, scores d'équipe, classement (frags, morts, ratio), ta vitesse
  et ta vitesse max, chaîne de tir, arme et munitions, derniers kills. En solo : vague, score, kills, ennemis restants.
- Hors partie : « la carte de la partie s'affichera ici ».
- Réglage : PARAMETRES > 2e ECRAN (`second_ecran=` dans `velocity.ini`).
- Technique : contexte OpenGL partagé avec le jeu (police et textures communes), 30 images par seconde, sans synchro
  verticale pour ne pas ralentir le jeu. Code : `src/interface/carte_ecran.c` (dessin) et `src/plateforme/second_ecran.c`
  (fenêtre, Windows et macOS). Captures : `--shot carte.bmp`, `carte_dm.bmp`, `carte_dm_equipes.bmp` (§3.15).

## 11. Carte : seulement soi et ses équipiers

- La carte ne sert pas de radar contre l'adversaire. Pendant la partie on n'y voit que **soi et ses équipiers**, avec
  leurs projectiles et leurs pièces ; en chacun pour soi, seulement soi.
- **Tout le monde** apparaît en spectateur et une fois le match fini.
- Code : `maps_sees`. Test : « joueurs visibles en equipes=2, chacun pour soi=1, partie finie=4 ».

## 12. Carte en 3D inclinée (ou de dessus)

- Au choix, la carte se voit **de dessus** ou **en 3D**, vue en plongée depuis le sud.
- En 3D, les **hauteurs** se lisent : chaque joueur est un pion à sa couleur avec la flèche de son regard sur la tête,
  une **ombre au sol** sous lui et un **poteau pointillé** jusqu'au sol quand il est en l'air ; les traînées suivent les
  sauts. Les murs extérieurs sont rabotés pour ne rien cacher.
- Réglage : PARAMETRES > 2e ECRAN, qui fait défiler **NON → HAUT → 3D** (`carte_vue=` dans `velocity.ini`). Angle,
  recul et champ de vision : `carte_3d` dans `graphismes/rendu.ini`.
- Code : `maps_draw_3d`. Captures : `--shot carte3d.bmp`, `carte3d_dm.bmp`.

## 13. Courbe de vitesse

- Dans le panneau des stats, la **vitesse de chaque joueur visible** au fil du temps (toi en trait épais) : les
  60 dernières secondes pendant le match, **tout le match** une fois fini. La courbe s'interrompt pendant les morts.
- Un point toutes les 0,5 s ; remise à zéro à chaque nouveau match.
- Code : `maps_speed_tick`, `maps_draw_speed`. Test : « courbe de vitesse : points en 2 s=4 ».

## 14. Carte de chaleur de fin de match

- À la fin du match, la carte montre **où l'on est mort** (taches rouges) et **d'où l'on a tiré** (taches jaunes). Les
  taches se cumulent : les zones chaudes sautent aux yeux. Elle s'affiche toujours vue de dessus.
- Chaque kill est enregistré au moment où il arrive (`maps_heat_add`, appelé sur `EV_KILL`) ; rayon d'une tache :
  `MAPS_HEAT_R`.
- Capture : `--shot carte_dm_equipes_fin.bmp`. Test : « carte de chaleur : 2 points ».

## 15. Fonds sous les textes du HUD

- Des **plaques sombres** avec un liseré clair sous les infos de partie, la mêlée, la vitesse, la chaîne de tir, le
  panneau d'arme, et sous chaque ligne de killfeed : tout reste lisible sur un ciel clair ou une explosion.
- Opacité réglable sans recompiler : `hud_fonds` dans `graphismes/rendu.ini` (0 = aucun fond).
- Code : `hud_panel` dans `src/interface/texte.c`.

## 16. Caméra de mort

- Quand on est tué, la caméra **recule, monte et pivote vers le tueur** en zoomant, pendant 1,6 s (`KILLCAM_T`) ; elle
  ne traverse pas les murs.
- À l'écran : bandes noires de cinéma, « **ELIMINE PAR** Rival », l'**arme** utilisée, les **PV qu'il lui reste**, et
  des crochets autour de lui qui se resserrent. Le bas du HUD s'efface pour laisser voir la scène ; le choix du loadout
  reste disponible. Le respawn garde son délai (2,5 s).
- En solo, la caméra se tourne vers l'ennemi vivant le plus proche qui te voyait (« TUE PAR UN DRONE »).
- Pas de caméra pour un suicide ni une chute.
- Réglage : PARAMETRES > CAM. MORT (`camera_mort=` dans `velocity.ini`).
- Code : `src/effets/camera_mort.c`, `draw_killcam_hud` (§3.16). Capture : `--shot dm_mort.bmp`.

## 17. Réunion avec `origin/dev`

Les nouveautés d'`origin/dev` sont intégrées et rangées dans l'arborescence :

- **Bouton MISE A JOUR** : vérifie, télécharge (SHA-256 vérifié) et installe la dernière version de la branche
  `release`. Il a maintenant son fichier, `src/interface/mise_a_jour.c` (§3.17). Le menu principal affiche LOADOUT,
  SKIN et PARAMETRES sur une ligne, puis MISE A JOUR ; « MAJ AUTO » est dans PARAMETRES.
- **Musique stéréo** (`src/son/music.h`), **CI Windows** (build MSVC et `--test` à chaque push) et **CI release**
  (binaires Windows et macOS construits depuis `stable`).
- Le code fusionné a été vérifié de la même façon que pour le point 6 : sortie du préprocesseur identique entre le
  fichier fusionné à plat et l'arborescence.

## 18. Fusion avec le dépôt original (`origin/stable` : compétences et cartes)

Le dépôt original a avancé sur `stable` dans l'ancien `main.c` d'un seul bloc ; tout est porté dans l'arborescence `src/` :

- **Compétences** (emplacement générique, touche COMPETENCE) : PIECE, MARQUE, CHARGE, RICOCHET, ALL IN ONE, MIRAGE ;
  choix dans LOADOUT (`skill=` dans `velocity.ini`), HUD des charges sous l'arme. Nouveau fichier
  `src/combat/competences.c` ; détails dans `docs/COMPETENCES_MARKSMAN.md`.
- **Cartes** : `MapDef` et 4 nouvelles cartes (LE PUITS, LE CANYON, LA BOUCLE, LE DOJO), bandes d'accélération,
  ancres de grappin, pads visés ; **kill zone** (`W_VOID`, kill crédité au dernier attaquant), bots prudents au bord
  du vide (`src/jeu/kill_zone.c`). Choix de la carte et ROTATION dans le menu principal **et** dans HEBERGER UNE
  PARTIE ; `--map N` pour les bots et les captures (§3.18 de `docs/CODE.md`).
- **Raccords avec `dev`** : départs par camp sur toutes les cartes en mode équipes, annonce LAN avec le mode **et**
  la carte, lightmaps/ombres/bloom sur les nouvelles cartes (ciel et brouillard propres à chaque carte, `rendu.ini`
  et ses ambiances pour l'Arène), carte du second écran et radar calés sur l'emprise réelle de la carte, chute hors
  de la carte conservée sur les cartes fermées. `NET_VER` passe à **14** (les deux côtés avaient pris 13).
- Vérifié : compilation macOS sans avertissement, toutes les lignes `--test` de `dev` et de l'original présentes et
  conformes (+ une ligne : départs par camp sur LE CANYON), captures, matchs bot contre bot sur LE CANYON et LE DOJO
  (le client sans `--map` charge bien la carte de l'hôte). Pas encore vérifié sous Windows.

## 19. Bots qui savent se déplacer, jauge de style, course au grappin, nouvelles musiques

- **Navigation des bots** : chaque carte a son graphe calculé tout seul (sols, pads, ancres, grappin). Les bots
  passent les vides du Canyon (pad → îlot → grappin vers l'autre falaise), montent la spirale du Puits, contournent
  les piliers du Dojo, et tirent en route. Simulation de test : les 6 trajets d'essai arrivent, sans chute
  (avant : Canyon et Puits impossibles). Match de bots sur Le Canyon : 1 chute au lieu de 3 à 6.
- **Jauge de style D → S** (à droite du viseur) : la vitesse, le wall-run, le grappin et les beaux kills (en l'air,
  à pleine vitesse, au grappin, ricochet, enchaînés, parry) la remplissent ; les coups reçus la vident. Points du solo
  × 2 en S, musique plus intense en A et S.
- **Mode COURSE** (bouton à côté de SOLO) : course au grappin façon Spider-Man sur la nouvelle carte **LA TOILE**,
  quatre avenues de gratte-ciel au-dessus du vide, 13 portes, chrono, écart au record à chaque porte, **fantôme** de
  ton meilleur temps, chute = retour à la dernière porte, R pour recommencer. Record dans `velocity_course.txt`.
  Rendue plus facile : 31 gros **points de balancement** dorés et lumineux, un **aimant** qui envoie le grappin
  vers celui que tu vises à peu près (repère tournant dessus), une corde ramenée à 18 m pour un vrai balancier à
  hauteur des portes, de l'élan en avançant pendant le balancement, des portes plus grandes et plus de passage
  obligé sous une passerelle. Un pilote de test très simple finit le parcours en 41 s.
- **5 nouvelles musiques**, une par carte : Le Puits (dub techno sombre, cloches FM), Le Canyon (breakbeat du désert
  avec swing), La Boucle (synthwave, arpèges), Le Dojo (drum and bass, gamme japonaise), La Toile (trance qui monte
  avec les portes). Le moteur gère maintenant une gamme, un timbre de lead et un swing par morceau ; l'Arène est
  identique à l'échantillon près.
- Docs : §3.19 à §3.21 de `docs/CODE.md`. Tests : navigation, style, course, musiques.

## 20. LA CITADELLE et le décor maillé

- **Nouvelle carte LA CITADELLE**, dans l'esprit des shooters tactiques à l'architecture soignée (carte originale) :
  ville fortifiée en plein soleil, pierre, enduit, tuiles et coupoles turquoise. Deux spawns, trois couloirs, site A
  (cour de la fontaine sous une arcade), site B (marché couvert à colonnes), place du clocher au centre, remparts à
  chemin de ronde, terrasses accessibles par pads, paysage hors les murs (collines, mer, montagnes).
- **Beaucoup plus de polygones** : ~180 000 triangles de décor (avant : quelques centaines de boîtes). Nouveau
  système de **décor maillé** (colonnes, arches, coupoles, toits de tuiles en rangées, fenêtres à volets, auvents,
  arbres, rambardes, lanternes, amphores, fontaine...) éclairé par sommet avec la même lumière que les lightmaps
  (soleil, ombres, occlusion), et boîtes de collision invisibles derrière.
- **Éclairage et textures par carte** : plein soleil sur la Citadelle (le reste garde `rendu.ini`), textures de
  pierre et d'enduit, nouveau morceau « Citadelle » (groove à mi-tempo, sol dorien, lead façon guitare).
- Moteur : jusqu'à 768 boîtes par carte, lightmaps libérées au changement de carte, `--pos` pour les captures.
- Vérifié : tous les tests (aucune régression), match de bots sur la carte (4 kills, navigation, 0 chute).

## 21. Règles de partie, modes d'équipe, boutique, spécialisations, L'OASIS

- **Règles choisies par l'hôte** (HEBERGER > RÈGLES) : mode, grappin oui/non, munitions normales / ×2 / infinies,
  boutique, spécialisations, durée.
- **Modes** : CHRONO (le plus de kills dans le temps), **3 ZONES** (rester dedans pour la prendre, l'équipe qui en
  tient le plus marque), **DRAPEAU** (le porteur qui meurt le laisse sur place, un coéquipier peut le reprendre, un
  défenseur qui reste à côté sans être touché le rend), **BOUCLIER** (les ROUGES le surchargent, les BLEUS tiennent).
  Pas de bots dans ces trois modes pour l'instant.
- **Boutique** : une arme de départ au choix parmi 4, des pièces à chaque kill, mort et objectif, et à chaque mort on
  achète d'autres armes (gardées pour le match) ou des munitions.
- **Spécialisations** : ASSAUT, REMPART, ÉCLAIREUR, SOUTIEN, ARTIFICIER (LOADOUT).
- **Nouvelle carte L'OASIS** : tactique, sans grappin, une salle d'apparition fermée par équipe (l'adversaire y
  brûle), trois couloirs (ruines, temple, bazar), deux tours de guet.
- Corrigé au passage : le sol d'une carte pouvait disparaître quand ses coins et son centre étaient sous d'autres
  boîtes (test de face enfouie sur une grille 5 × 5).

---

## 22. Plafond d'images par seconde (MAX FPS)

- **Avant** : la synchro verticale ne bride pas toujours (fenêtre masquée, certains pilotes) ; le jeu montait alors à
  400–1000 images/s et occupait un cœur entier (mesuré sur Mac M4, toutes cartes).
- **Maintenant** : bouton **MAX FPS** dans PARAMÈTRES (à côté de PLEIN ÉCR.) : SANS, 30, 60, 90, 120, 144, 165, 240,
  360 ; 240 par défaut (`FPS_CAP_DEFAULT`), clé `max_fps=` de `velocity.ini` (0 = sans limite, borné 20–1000).
- Attente en fin d'image (`fps_cap_wait`, sommeil puis courte attente active ; `timeBeginPeriod(1)` sous Windows),
  échéances régulières sans rattrapage en rafale (`fps_cap_deadline`). La simulation (120 ticks/s) n'est pas touchée.
- Mesure (LA CITADELLE, Mac M4) : sans plafond 540 img/s et ~100 % d'un cœur ; à 60 : 59 img/s, ~45 %.
- Test : « plafond d'images : echeances 1.010 1.500 1.010… ».

## 23. GRILLE-PAIN : le jeu tourne par défaut sur les petites machines

- **Avant** : premier lancement en ULTRA (ombres, bloom, lightmaps fines, ciel complet) ; trois qualités BASSE,
  MOYENNE, ULTRA ; aucune détection d'un PC sans pilote graphique.
- **Maintenant** : quatre préréglages **GRILLE-PAIN** (par défaut), BASSE, MOYENNE, ULTRA (PARAMÈTRES > QUALITÉ,
  bouton pleine largeur). GRILLE-PAIN : ni ombres dynamiques ni bloom, 60 img/s, lightmaps grossières, ni décor
  purement décoratif (étoiles, nuages, poussière, ville au loin, reflets, lumières dynamiques, liserés, passe de grain).
  Contours et traînées des joueurs gardés. Les anciens `velocity.ini` (clé `quality=`) repartent en GRILLE-PAIN :
  le choix est maintenant dans `graphismes=`.
- **RES. 3D** (50 à 100 %) : la 3D est dessinée plus petite puis agrandie, le HUD reste net.
- **Sans pilote** (rendu logiciel de Windows) : message au démarrage, et 3D à 50 % au premier lancement.
  `--soft` simule ce cas ; `--qualite N` choisit le préréglage d'une capture.
- Mesures (`--shot bench_dm.bmp`, 1366×891) :

  | | ULTRA | GRILLE-PAIN |
  |---|---|---|
  | Arène, rendu logiciel (`--soft`) | 85,7 ms/image | 30,5 ms/image |
  | Citadelle, rendu logiciel | 98,2 ms | 40,1 ms |
  | Arène, Mac M4 | 1,98 ms | 1,16 ms |
  | Éclairage du décor au chargement, Arène | 224 ms | 23 ms |
  | Éclairage du décor au chargement, Citadelle | 865 ms | 282 ms |

  RES. 3D à 67 % ne gagnait rien sur ces deux rendus (l'agrandissement coûte autant qu'il rapporte) : 100 % par
  défaut, 50 % sans pilote (30 → 24 ms). À mesurer sur un vrai PC Windows à puce Intel avant d'aller plus loin.
- Test : « grille-pain : prereglage ombres 0 bloom 0 3D 100 % fps 60… ».

## 24. Monde en secteurs et en une seule passe, lightmaps en atlas

- **Avant** : le monde était une seule liste d'affichage dessinée en entier à chaque image, en 4 passes (sol, murs,
  lightmap mélangée ×2, grain), avec une texture de lightmap par face (des centaines de changements de texture).
- **Maintenant** (`src/rendu/monde_secteurs.c`) :
  - **Atlas** : toutes les lightmaps dans des pages de 1024² (1 page sur l'Arène comme sur LA CITADELLE).
  - **Passe unique** en multitexture (texture × lightmap × 2 × grain) dès que la carte le permet ; repli automatique
    sur les passes d'avant (rendu logiciel de Windows) ou forcé par `--multipasse`. Même image (écart < 1/255).
  - **Secteurs** de 24 m : seuls ceux dans le champ de vision sont dessinés (Arène 14 sur 39, Citadelle 73 sur 242
    dans la vue du banc).
- Mesures (`--shot bench_dm.bmp`, ms par image) :

  | | avant | après |
  |---|---|---|
  | Arène, Mac M4, GRILLE-PAIN | 1,12 | 0,77 |
  | Arène, Mac M4, ULTRA (dont monde) | 1,90 (0,85) | 1,60 (0,55) |
  | Citadelle, Mac M4, GRILLE-PAIN | 1,95 | 1,43 |
  | Citadelle, Mac M4, ULTRA (dont monde) | 2,58 (1,76) | 2,11 (1,10) |
  | Arène, rendu logiciel, monde seul | 20,8 | 17,6 |
  | Citadelle, rendu logiciel, monde seul | 36,2 | 32,0 |

  En rendu logiciel le coût est surtout le remplissage des pixels (sol plein écran), que les secteurs ne réduisent
  pas : l'image complète y reste à ~31 ms (Arène) et ~39 ms (Citadelle) en GRILLE-PAIN.
- Test : « secteurs : cases 0 10 3, boite dedans 1, a cheval 1, dehors 0 ».

## 25. GRILLE-PAIN : moins de pixels à calculer (textures, ciel, détails du décor)

- **Mesure d'abord** (rendu logiciel, monde de l'Arène : 21 ms) : la texture du sol et des murs filtrée coûtait ~14 ms,
  la lightmap ~4 ms, le brouillard ~1 ms. Le banc donne maintenant le détail par étape en ULTRA **et** en GRILLE-PAIN.
- **Sans textures en GRILLE-PAIN** (`textures_monde = 0`) : couleur unie × lumière, avec la clarté moyenne de la texture
  (mêmes tons qu'avant). BASSE et au-dessus gardent les textures.
- **Ciel dessiné après le monde** (toutes qualités) : il ne peint plus que les pixels vides, et son dégradé est une
  liste d'affichage au lieu de ~1 000 couleurs recalculées à chaque image.
- **Détails de LA CITADELLE** (`decor_details = 0` en GRILLE-PAIN) : tuiles, cadres et lames des volets, balustres,
  lanternes retirés (103 000 triangles sur 180 000), avec des versions plates pour les volets et rambardes.
- Mesures (`--shot bench_dm.bmp`, ms par image, GRILLE-PAIN) :

  | | avant | après |
  |---|---|---|
  | Arène, rendu logiciel | 31,7 | 17,1 |
  | Citadelle, rendu logiciel | 38,4 | 20,5 |
  | Arène, Mac M4 | 0,77 | 0,69 |
  | Citadelle, Mac M4 | 1,43 | 1,06 |
  | Chargement de l'éclairage, Citadelle | 280 ms | 177 ms |

- Tests : « decor (LA CITADELLE) GRILLE-PAIN : … triangles de details retires… ».

## 26. Page GRAPHISMES et manette PS4

- **PARAMÈTRES** en trois pages : RÉGLAGES, **GRAPHISMES** (QUALITÉ, ombres, bloom, **textures**, **détails**,
  résolution 3D, ciel, plein écran, MAX FPS ; à droite la carte graphique, le mode de dessin et l'aide des
  préréglages) et **MANETTE**. TEXTURES et DÉTAILS se changent maintenant seuls (avant : imposés par GRILLE-PAIN).
- **Manette** : DualShock 4 (et DualSense, Xbox) en USB ou Bluetooth, sans pilote ni dépendance (Windows : winmm ;
  macOS : GameController). Disposition : stick gauche bouger, stick droit viser, R2 tir, L2 grappin, R1 épée, L1
  dash, CROIX saut, ROND accroupi/slide, CARRÉ pièce, TRIANGLE changer d'arme, flèches armes et mines, pavé tactile
  scores, OPTIONS menu. Chaque bouton se réaffecte dans PARAMÈTRES > MANETTE ; vitesse de visée et axe Y réglables.
  Dans les menus : curseur au stick gauche, CROIX clique, ROND revient.
- À tester sur une vraie manette : la disposition winmm d'une DualShock 4 vient de la documentation (personne n'a
  encore branché de manette sur cette version). Si Steam ou DS4Windows tournent, ils peuvent la présenter comme une
  manette Xbox : c'est aussi géré.
- Tests : « manette : avance 1 saut 1 tir 1… », « reglages manette et graphismes… ».

## Vérifications faites

- Compilation macOS sans avertissement ; tous les tests `--test` conformes, y compris ceux d'`origin/dev` (stéréo,
  mise à jour, SHA-256) et une ligne de test par nouvelle mécanique.
- Captures de chaque nouveauté et match bot contre bot en réseau réel.
- Fenêtre du second écran essayée en vrai sur un Mac (contexte partagé, police, partie solo en direct).
- **Pas encore vérifié sous Windows** : la CI Windows d'`origin/dev` compilera et testera `dev` à chaque push ; reste
  à essayer le second écran avec deux moniteurs.

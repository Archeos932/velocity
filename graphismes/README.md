# Graphismes de VELOCITY — à modifier librement

Tout l'aspect visuel du jeu se règle ici, **sans toucher au code ni recompiler** :

| Fichier | Rôle |
|---|---|
| `rendu.ini` | soleil, ombres, lumière du décor, brouillard, ciel, ville au loin, bloom, textures générées |
| `sol.bmp` | texture du sol et des dessus de blocs (256 px) |
| `mur.bmp` | texture des murs (256 px) |
| `pierre.bmp` | pavés des sols de LA CITADELLE (256 px) |
| `enduit.bmp` | enduit des murs de LA CITADELLE (256 px) |
| `grain.bmp` | grain fin multiplié par-dessus le décor (64 px, gris clair à blanc) |
| `ambiance_nuit.ini`, `ambiance_jour.ini`, `ambiance_orage.ini` | ambiances choisies dans PARAMETRES > CIEL : elles remplacent une partie de `rendu.ini` (crée la tienne en copiant une de ces lignes) |

## Mode d'emploi

1. Ouvre `rendu.ini` dans un éditeur de texte. Chaque ligne est commentée.
2. Change une valeur, enregistre.
3. En jeu, appuie sur **F5** : le fichier et les textures sont relus, l'éclairage est recalculé
   (« GRAPHISMES RECHARGES » s'affiche). Pas besoin de relancer le jeu.

PARAMETRES > QUALITE (BASSE / MOYENNE / ULTRA) allège les réglages coûteux pour les petites machines.

Une ligne effacée reprend sa valeur d'origine. Une valeur hors limites est ramenée dans des bornes raisonnables.

**Le gameplay n'est jamais modifié** : collisions, dégâts, carte et réseau ne lisent pas ce dossier. Deux joueurs
avec des réglages différents peuvent jouer ensemble.

## Textures `.bmp`

Les cinq `.bmp` livrés ici sont **les textures actuelles du jeu**, telles que le code les calcule : les ouvrir dans
un logiciel de dessin, les retoucher, enregistrer, puis **F5** en jeu. Tant qu'elles ne sont pas modifiées, le jeu
est exactement le même qu'avec les textures générées.

- BMP **24 bits** (ou 32), non compressé, **carré**, côté 16, 32, 64, 128, 256, 512 ou 1024 px.
- Une image invalide est ignorée (la texture générée reste).
- La taille de répétition se règle avec `texture_echelle` (et l'échelle de `pierre` / `enduit`) dans `rendu.ini` :
  elle marche avec ou sans `.bmp`.

### Attention : un `.bmp` présent fige sa texture

Sans `.bmp`, chaque texture est **générée** par le jeu à partir de réglages de `rendu.ini`. Dès que le `.bmp` est
là, c'est lui qui est affiché et ces réglages **n'ont plus aucun effet** sur cette texture :

| Fichier | Réglages de `rendu.ini` ignorés tant qu'il existe |
|---|---|
| `sol.bmp` | `sol_texture` (taille des dalles, luminosité) |
| `mur.bmp` | `mur_texture` (taille des plaques, luminosité) |
| `pierre.bmp` | les deux premières valeurs de `pierre` (taille des pavés, clarté) |
| `enduit.bmp` | les deux premières valeurs de `enduit` (clarté, grain) |
| `grain.bmp` | `grain_force` |

- **Revenir à la texture générée et réglable** : supprimer (ou renommer) le `.bmp`, puis F5.
- **Régénérer les `.bmp`** après avoir changé ces réglages : `velocity.exe --export-textures graphismes`
  (macOS : `Velocity.app/Contents/MacOS/velocity --export-textures graphismes`). Le jeu lit `rendu.ini`, calcule les
  textures et les écrit dans le dossier donné (créé s'il manque) — **en écrasant** les `.bmp` qui s'y trouvent :
  exporter ailleurs pour garder tes retouches.
- Ces textures ne dépendent ni de la carte, ni de l'ambiance (CIEL), ni de la QUALITE : une seule image chacune.
  L'éclairage, les ombres et le brouillard sont appliqués par-dessus, à part.

## Où le jeu cherche ce dossier

Le premier trouvé gagne :

- **macOS** :
  1. `~/Library/Application Support/Velocity/graphismes/` (tes réglages perso) ;
  2. le dossier `graphismes/` à côté de `Velocity.app` (dans le projet) ;
  3. celui copié dans `Velocity.app` par `build_mac.sh`.
- **Windows** : `graphismes\` à côté de `velocity.exe`.
- Sinon : `graphismes/` dans le dossier courant. Sans dossier, le jeu garde son aspect par défaut.

## Et les shaders ?

Le jeu tourne en OpenGL 1.1 (pipeline fixe, sans shader) pour marcher partout sans dépendance. Ce que ferait
un shader (lumière, ombres, brouillard, halo) est précalculé ou réglé par les valeurs de `rendu.ini`.

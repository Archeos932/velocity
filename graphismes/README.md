# Graphismes de VELOCITY — à modifier librement

Tout l'aspect visuel du jeu se règle ici, **sans toucher au code ni recompiler** :

| Fichier | Rôle |
|---|---|
| `rendu.ini` | soleil, ombres, lumière du décor, brouillard, ciel, ville au loin, bloom, textures générées |
| `sol.bmp` *(facultatif)* | remplace la texture du sol et des dessus de blocs |
| `mur.bmp` *(facultatif)* | remplace la texture des murs |
| `grain.bmp` *(facultatif)* | remplace le grain fin (multiplié par-dessus, mettre du gris clair à blanc) |

## Mode d'emploi

1. Ouvre `rendu.ini` dans un éditeur de texte. Chaque ligne est commentée.
2. Change une valeur, enregistre.
3. En jeu, appuie sur **F5** : le fichier et les textures sont relus, l'éclairage est recalculé
   (« GRAPHISMES RECHARGES » s'affiche). Pas besoin de relancer le jeu.

Une ligne effacée reprend sa valeur d'origine. Une valeur hors limites est ramenée dans des bornes raisonnables.

**Le gameplay n'est jamais modifié** : collisions, dégâts, carte et réseau ne lisent pas ce dossier. Deux joueurs
avec des réglages différents peuvent jouer ensemble.

## Textures `.bmp`

- BMP **24 bits** (ou 32), non compressé, **carré**, côté 16, 32, 64, 128, 256, 512 ou 1024 px.
- Une image invalide est ignorée (la texture générée reste).
- La taille de répétition se règle avec `texture_echelle` dans `rendu.ini`.

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

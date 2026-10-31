# VELOCITY

Movement shooter 3D en C, sans aucune dépendance (Win32 / Cocoa + OpenGL 1.1, son synthétisé, réseau UDP).

- **Compiler** : `build.bat` (Windows, MSVC) ou `./build_mac.sh` (macOS).
- **Changer l'aspect sans recompiler** : `graphismes/` (puis F5 en jeu).
- **Documentation du code** : [`docs/CODE.md`](docs/CODE.md) — le §2 détaille chaque fichier.
- **Nouveautés** (détail de chaque fonctionnalité ajoutée) : [`docs/NOUVEAUTES.md`](docs/NOUVEAUTES.md).

## Où trouver quoi

```
main.c                 table des matieres : inclut src/ dans l'ordre (seul fichier compile)
src/
  reglages/            tous les chiffres d'equilibrage (#define)
  joueur/              mouvement : bhop, slide, dash, wall-run, grappin
  armes/               une arme = un fichier (enclume, lance, fleau, poings, egide, elan, eventail, tesla, scie, ventouse)
  combat/              degats, loadout, melee, pieces et railgun, roquettes, projectiles
  perso/               skins des personnages
  graphismes/          lecture de graphismes/rendu.ini (ambiances, qualite)
  rendu/               textures, eclairage, ciel, ombres, bloom, arme en main, image finale
  effets/              particules, impacts, neons, meteo
  interface/           menus, HUD, scores, police, velocity.ini, carte du 2e ecran
  son/                 sons synthetises et musique
  monde/               carte et collisions
  solo/                ennemis et vagues
  reseau/              LAN et parties en ligne (hote, client, lobby)
  bots/                bots
  entrees/             touches et actions
  jeu/                 boucle de simulation (game_tick)
  plateforme/          fenetre, point d'entree ; mac/ = port macOS
  tests/               --test et --shot
  noyau/               includes et maths
graphismes/            reglages visuels et textures modifiables
outils/                outil d'ecoute de la musique
worker/                lobby en ligne (Cloudflare)
docs/                  documentation, plans, fiches des armes
```

/* VELOCITY - TOUS les #define d'equilibrage : mouvement, armes, parry, PvP, points
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Reglages du mouvement (metres, secondes)                            */
/* ------------------------------------------------------------------ */
#define HW             0.4f   /* demi-largeur joueur */
#define H_STAND        1.8f
#define H_CROUCH       1.1f
#define EYE_STAND      1.6f
#define EYE_CROUCH     0.95f
#define STEP_H         0.55f
#define STAIR_SLOPE    0.45f   /* pente d'un escalier (marche 0,4 m / profondeur 0,84 m) : vy de rampe = vitesse x pente */
#define STAIR_RAMP_T   0.18f   /* un escalier agit comme une rampe pendant ce delai apres la derniere marche montee */
#define FRICTION_MOVE  0.12f   /* friction au sol x0,12 quand on tient une direction (sans sauter, la vitesse se perd moins) */
#define GRAVITY        24.0f
#define JUMP_V         8.4f
#define RUN_SPEED      9.5f
#define CROUCH_SPEED   4.5f
#define GROUND_ACCEL   12.0f
#define FRICTION       6.0f
#define STOPSPEED      3.0f
#define AIR_ACCEL      14.0f
#define AIR_CAP        1.0f
#define SLIDE_MIN      6.5f
#define SLIDE_END      3.0f
#define SLIDE_BOOST    4.5f
#define SLIDE_FRICTION 0.7f
#define SLIDE_CD       1.0f
#define DASH_SPEED     18.0f
#define DASH_RECHARGE  1.6f
#define WALLJUMP_PUSH  8.0f
#define WALLJUMP_UP    8.8f
#define WALLRUN_MIN    6.0f
#define WALLRUN_MAX    1.8f
#define BASE_HFOV      105.0f

#define AIMASSIST_CONE  0.07f  /* rayon (rad, ~4 deg) du cone pres du viseur ou l'aimant agit */
#define AIMASSIST_RATE  1.4f   /* vitesse de correction au centre du cone (s'annule au bord) ; faible expres */
#define AIMASSIST_RANGE 60.0f  /* portee max */
#define BOT_AIM_TOL     0.03f  /* bot : il ne tire (ou ne lance grappin, dash, piece, melee) qu'avec la vue a moins de
                                  0,03 rad (+ la taille de la cible) de la direction voulue */
#define BOT_PEND_T      0.6f   /* ...sinon l'action attend que la vue arrive, 0,6 s au plus */

#define GRAPPLE_RANGE    55.0f   /* portee max du crochet */
#define GRAPPLE_SPEED    120.0f  /* vitesse de vol du crochet */
#define GRAPPLE_PULL     70.0f   /* acceleration vers le point d'accroche */
#define GRAPPLE_MAXSPEED 38.0f   /* plus de traction au-dela de cette vitesse vers l'ancre */
#define GRAPPLE_CD       0.5f
#define GRAPPLE_JUMP     5.0f    /* boost vertical quand on lache en sautant */
#define GRAPPLE_ENEMY_PULL     95.0f  /* traction plus forte vers un ennemi */
#define GRAPPLE_ENEMY_MAXSPEED 46.0f
#define GRAPPLE_STRIKE_DMG     50.0f  /* degats de l'impact... */
#define GRAPPLE_STRIKE_PER_MS  3.0f   /* ...plus 3 par m/s de vitesse d'impact */

#define RAIL_CD        0.85f
#define PICKUP_RESPAWN  12.0f

/* Cartes (MapDef, docs/CODE.md) : bande d'acceleration de La Boucle, plan de mort des cartes ouvertes */
#define SPEED_ACCEL     30.0f   /* B_SPEED : poussee (m/s2) dans le sens de la bande, sans friction */
#define SPEED_MAX       28.0f   /* B_SPEED : vitesse jusqu'a laquelle la bande pousse */
#define VOID_CREDIT_T   4.0f    /* chute dans le vide moins de 4 s apres une touche : kill au dernier attaquant */
#define VOID_WARN_H     8.0f    /* vignette rouge sous killY + 8 m quand on tombe */

/* Pieces facon Marksman (Ultrakill) : on lance une piece, un tir de rail qui la touche ricoche
 * tout seul vers la cible la plus proche. Pense pour viser a pleine vitesse. */
#define COIN_MAX        4
#define COIN_RECHARGE   2.0f    /* secondes par piece */
#define COIN_RADIUS     0.9f    /* grosse zone de touche */
#define COIN_MULT       1.5f    /* chaque ricochet multiplie les degats */
#define COIN_GRAVITY    16.0f
#define COIN_SEEK_RANGE 90.0f

/* Competences : l'emplacement de la piece (touche A_SKILL) recoit une competence au choix du LOADOUT
 * (docs/COMPETENCES_MARKSMAN.md). Charges et recharge par competence : skillDef. */
enum { SK_COIN, SK_MARK, SK_CHARGE, SK_RICOCHET, SK_ALLIN, SK_DECOY, SK_COUNT };
#define MARK_T            4.0f    /* MARQUE : duree de la marque */
#define MARK_RANGE        90.0f
#define MARK_MULT         1.25f   /* toute touche du marqueur sur une cible marquee */
#define CHARGE_HOLD       0.6f    /* CHARGE : maintenir la touche */
#define CHARGE_WINDOW     2.0f    /* puis fenetre ouverte pour la prochaine touche */
#define CHARGE_MULT       1.5f
#define CHARGE_GRACE      0.05f   /* une fois declenchee, les autres touches du meme tir en profitent (perforation, plombs) */
#define RIC_BOUNCES       3       /* RICOCHET : rebonds du prochain projectile */
#define RIC_MAX_LEN       120.0f  /* portee totale d'un rail reflechi */
#define RIC_ROCKET_SLOW   0.8f    /* vitesse gardee par une roquette a chaque rebond */
#define ALLIN_SPREAD_BASE 0.8f    /* ALL IN ONE : demi-angle de l'eventail (degres) */
#define ALLIN_SPREAD_PER  0.7f    /* par munition au-dela de la premiere */
#define ALLIN_SPREAD_MAX  6.0f
#define DECOY_T           3.0f    /* MIRAGE : duree du leurre */
#define DECOY_MULT        1.0f    /* un de mes rails qui le traverse ricoche sans bonus */
#define RAIL_DMG       100.0f
#define ROCKET_CD      0.75f
#define ROCKET_SPEED   38.0f
#define ROCKET_DMG     110.0f
#define ROCKET_RADIUS  4.5f
#define ROCKET_KNOCK   13.0f

/* Elan (convertisseur cinetique) : la vitesse horizontale depensee devient des degats */
#define KIN_CD            0.5f    /* recharge de l'Elan (divisee par streak_rate) */
#define KIN_DMG_BASE      20.0f   /* degats a l'arret */
#define KIN_DMG_PER_MS    5.0f    /* degats en plus par m/s de vitesse depensee */
#define KIN_SPEED_CAP     15.0f   /* vitesse horizontale maximale convertie par tir */
#define KIN_KNOCK         3.0f    /* recul de la cible a l'arret (PvE ; moitie en PvP) */
#define KIN_KNOCK_PER_MS  0.4f    /* recul en plus par m/s depensee : 3 -> 9 m/s */
#define KIN_FULL          14.5f   /* au-dela : tir "plein" (hitstop, son et rayon max) */
#define KIN_DASH_WINDOW   0.10f   /* dash-tir : le tir doit partir dans les 12 ticks apres le dash */
#define KIN_DASH_FORGET   0.6f    /* jusque-la, hors fenetre, la vitesse du dash ne compte pas */

/* Fusil a pompe "Eventail" : 10 plombs, plein jusqu'a 10 m, x0.3 a 30 m */
#define SG_PELLETS        10
#define SG_HALF_ANGLE     0.0785f /* demi-angle du cone en radians (4.5 deg, cone total 9 deg) */
#define SG_SLIDE_SPREAD   0.5f    /* en slide : dispersion divisee par 2 */
#define SG_PELLET_DMG     8.0f    /* degats PvE par plomb */
#define SG_NEAR           10.0f   /* degats pleins jusqu'ici */
#define SG_FAR            30.0f   /* ... puis lineaire jusqu'a SG_FAR_MULT ici */
#define SG_FAR_MULT       0.3f
#define SG_RANGE          60.0f   /* portee max d'un plomb */
#define SG_CD             0.95f   /* recharge (divisee par streak_rate) */
#define SG_STREAK_MIN     3       /* plombs touches pour compter dans la chaine */
#define SG_PELLET_KNOCK   0.6f    /* recul par plomb sur la cible */
#define SG_RECOIL         7.0f    /* recul du tireur en l'air */
#define SG_RECOIL_VMAX    9.0f    /* le recul ne pousse pas vy au-dela (sauf si deja plus) */
#define SG_RECOIL_HCAP    16.0f   /* idem pour la vitesse horizontale */
#define SG_COIN_RADIUS    0.55f   /* zone de touche d'une piece pour un plomb (rail : 0.9) */
#define SG_BURST_RADIUS   3.0f    /* piece eclatee : souffle */
#define SG_BURST_DMG      45.0f   /* au centre, PvE (x PVP_SCALE contre un joueur) */
#define SG_BURST_KNOCK    8.0f
#define SG_BURST_CHAIN    3       /* une piece eclatee fait eclater les pieces voisines (profondeur max) */
#define SG_TRACER_SPEED   260.0f  /* vitesse visuelle d'un traceur */
#define MAXTR             64      /* traceurs de plombs */

/* Axe corps a corps : degats et portee montent avec la vitesse */
#define MELEE_CD          0.45f
#define MELEE_RANGE       2.4f
#define MELEE_RANGE_PER_MS 0.12f  /* +0.12 m de portee par m/s */
#define MELEE_DMG         35.0f
#define MELEE_DMG_PER_MS  4.0f    /* +4 degats par m/s */
#define MELEE_CONE        0.55f   /* cosinus du demi-angle du coup */
#define PARRY_SPEED       36.0f   /* renvoi esquivable (ou re-parable) */
#define PARRY_DMG         150.0f  /* orbes renvoyees en solo */
#define PARRY_HEAL        10.0f
#define PARRY_WINDOW      0.12f   /* le parry n'est actif qu'au tout debut du coup */
#define MELEE_WHIFF_CD    0.9f    /* coup qui ne touche ni ne pare rien : recharge plus longue */
#define PARRY_ROCKET_MULT 1.25f   /* roquette renvoyee : 1,25x les degats d'origine */
#define PARRY_PINGPONG    1.15f   /* chaque re-parry : +15 % de vitesse et de degats */
#define PARRY_RANGE_SHRINK 0.85f  /* ... et une zone de parry 15 % plus courte */
#define PARRY_AIM_COS     0.988f  /* aide a la visee du renvoi : ~9 degres */
#define PVP_HIT_CAP       60.0f   /* en PvP, aucune touche ne retire plus de 60 PV */

/* Jauge de style (src/jeu/style.c) : seuils des rangs C, B, A, S et plafond */
#define STYLE_C          60.0f
#define STYLE_B          130.0f
#define STYLE_A          220.0f
#define STYLE_S          330.0f
#define STYLE_MAX        420.0f
#define STYLE_KILL       45.0f    /* un kill */
#define STYLE_BONUS      30.0f    /* chaque bonus d'un kill (en l'air, vitesse, grappin, ricochet, enchaine...) */
#define STYLE_PARRY      55.0f    /* un parry reussi */
#define STYLE_FAST       20.0f    /* m/s : kill "PLEINE VITESSE" */
#define STYLE_CHAIN_T    3.0f     /* kills enchaines en moins de 3 s */
#define STYLE_SPEED_MIN  12.0f    /* au-dela, la vitesse remplit la jauge */
#define STYLE_SPEED_K    1.6f     /* points par seconde et par m/s au-dela de STYLE_SPEED_MIN */
#define STYLE_WALL_K     8.0f     /* points par seconde en wall-run ou au grappin a pleine vitesse */
#define STYLE_HURT_K     1.8f     /* points perdus par PV perdu */
#define STYLE_DECAY      10.0f    /* baisse par seconde (rang D) ... */
#define STYLE_DECAY_RANK 6.0f     /* ... plus 6 par rang au-dessus */

/* Modes de jeu (src/reseau/modes.c) */
#define ZONE_R           5.0f     /* 3 ZONES : rayon d'une zone (a l'horizontale) */
#define ZONE_CAP_T       6.0f     /* 3 ZONES : secondes pour prendre une zone seul (plus vite a plusieurs) */
#define ZONE_SCORE       200      /* 3 ZONES : points pour gagner (1 par seconde et par zone tenue) */
#define FLAG_TOUCH_R     2.2f     /* DRAPEAU : distance pour le prendre ou capturer */
#define FLAG_RETURN_R    3.5f     /* DRAPEAU : distance pour le rendre (rester a cote) */
#define FLAG_RETURN_T    5.0f     /* DRAPEAU : secondes a cote sans etre touche pour le rendre */
#define FLAG_RETURN_HIT  0.6      /* DRAPEAU : touche depuis moins de 0,6 s = le retour repart de zero */
#define FLAG_AUTO_RETURN 30.0f    /* DRAPEAU : tombe et oublie 30 s = rendu tout seul */
#define FLAG_CAPS        3        /* DRAPEAU : captures pour gagner */
#define SHIELD_ZONE_R    6.0f     /* BOUCLIER : rayon de la zone du generateur */
#define SHIELD_DRAIN     1.6f     /* BOUCLIER : % par seconde avec un attaquant seul dessus (+35 % par attaquant en plus) */
#define SPAWN_ROOM_DPS   40.0f    /* salle de depart adverse : degats par seconde */

/* Specialisations */
#define SPEC_ASSAUT_MUL  1.15f
#define SPEC_REMPART_MUL 0.8f
#define SPEC_REMPART_SPD 0.93f    /* REMPART : vitesse x0,93 */
#define SPEC_ECLAIR_SPD  1.12f    /* ECLAIREUR : vitesse x1,12, 3 charges de dash */
#define SPEC_REGEN_DELAY 3.0      /* SOUTIEN : secondes sans etre touche avant de regenerer */
#define SPEC_REGEN       6.0f     /* SOUTIEN : PV par seconde */
#define SPEC_HEAL_R      7.0f     /* SOUTIEN : rayon du soin des allies */
#define SPEC_HEAL        4.0f     /* SOUTIEN : PV par seconde aux allies proches */
#define SPEC_AMMO_MUL    1.5f     /* ARTIFICIER : jauges */

/* Economie et boutique (src/jeu/economie.c) */
#define ECO_START        400      /* pieces au debut du match */
#define ECO_KILL         200
#define ECO_DEATH        100      /* consolation a chaque mort */
#define ECO_OBJ          300      /* objectif personnel (zone prise, drapeau pris ou rendu, capture) */
#define ECO_TEAM_OBJ     100      /* objectif de l'equipe */
#define ECO_AMMO_PRICE   250      /* MUNITIONS + : jauges pleines a chaque respawn (pour le match) */
#define ECO_STARTER_T    10.0f    /* secondes pour choisir l'arme de depart */
#define LOADOUT_PICK_T   12.0f    /* sans boutique : secondes pour choisir son loadout au debut du match (ou en arrivant) */

/* Mode COURSE (src/jeu/course.c) */
#define RACE_COUNTDOWN   3.0f     /* decompte avant le depart */
#define SWING_HALF       1.5f     /* points de balancement : cubes de 3 m */
#define RACE_MAGNET_COS  0.70f    /* course : le grappin part vers le point de balancement a moins de ~45 deg du viseur */
#define RACE_SWING_PULL  0.25f    /* course : une fois la corde a RACE_ROPE, traction x0,25 (on se balance plus qu'on ne file) */
#define RACE_ROPE        18.0f    /* course : le grappin ramene la corde a 18 m (bas de l'arc ~ hauteur des portes), puis balancier */
#define RACE_SWING_PUMP  14.0f    /* course : avancer au grappin pousse de 14 m/s2 le long du balancement */
/* course facon Spider-Man : la toile s'accroche seule (on ne vise pas), tenir le bouton enchaine les balancements */
#define RACE_WEB_RANGE   48.0f    /* portee de la recherche d'accroche (decor et points de balancement) */
#define RACE_WEB_FWD     16.0f    /* accroche ideale : 16 m devant ... */
#define RACE_WEB_UP      18.0f    /* ... et 18 m au-dessus (sans decor : toile dans le ciel a cet endroit) */
#define RACE_ROPE_MAX    26.0f    /* corde plus longue : ramenee a 26 m (25 m/s) */
#define RACE_DROP        6.0f     /* ... et assez courte pour que le bas de l'arc reste au plus 6 m sous la hauteur d'accroche */
#define RACE_REEL        9.0f     /* sous l'altitude de la porte suivante, la toile se rembobine (m/s) : on remonte */
#define RACE_HIGH        8.0f     /* plus de 8 m au-dessus de la porte suivante : pas de toile, on redescend vers elle */
#define RACE_STALL_T     1.2f     /* toile qui ne mene nulle part (< 2,5 m/s pendant 1,2 s) : lachee toute seule */
#define RACE_MIN_UP      10.0f    /* la toile ne se prend que 10 m au moins au-dessus des yeux */
#define RACE_CHAIN_K     0.45f    /* tenu : nouvelle toile quand on a depasse l'ancre de 0,45 x la corde en remontant */
#define RACE_REL_FWD     4.0f     /* lacher (ou enchainer) : elan vers l'avant (m/s) ... */
#define RACE_REL_UP      6.0f     /* ... et vers le haut */
#define RACE_MAXSPEED    40.0f    /* vitesse horizontale maxi en course (m/s) */
#define RACE_SWING_STEER 1.6f     /* au bout de la toile, la trajectoire suit le regard (rad/s) */
#define RACE_AIR_STEER   1.0f     /* en l'air sans toile */
#define RACE_GATE_PULL   1.8f     /* a moins de 35 m d'une porte vers laquelle on va : aimant (rad/s) */
#define MELEE_PARRIED_CD  1.0f    /* attaquant dont l'epee est paree : epee bloquee */
#define MELEE_PARRY_GRACE 0.15f   /* tolerance de latence pour l'hote */

/* --- Enclume (marteau-pilon) : la melee verticale, elle recompense la hauteur --- */
/* coup au sol (ou en l'air trop pres du sol) */
#define ANVIL_CD           0.8f    /* recharge du coup au sol */
#define ANVIL_WHIFF_CD     1.2f    /* coup au sol dans le vide */
#define ANVIL_SWING_T      0.3f    /* duree de l'animation du coup */
#define ANVIL_RANGE        2.6f    /* portee de base */
#define ANVIL_RANGE_PER_MS 0.10f   /* portee en plus par m/s relatif */
#define ANVIL_DMG          45.0f   /* degats de base */
#define ANVIL_DMG_PER_MS   3.0f    /* degats en plus par m/s relatif */
#define ANVIL_CONE         0.45f   /* cosinus du demi-angle (plus large que l'epee) */
#define ANVIL_KNOCK        14.0f   /* recul vers l'avant */
#define ANVIL_KNOCK_UP     6.0f    /* recul vers le haut */
#define ANVIL_PARRY_WINDOW 0.10f   /* fenetre de parry, plus courte que l'epee */
/* pique et onde de choc */
#define SLAM_VY            30.0f   /* vitesse de chute minimale du pique */
#define SLAM_VY_MAX        45.0f   /* plafond de |vy| */
#define SLAM_MIN_H         1.2f    /* hauteur minimale sous les pieds pour piquer (sinon : coup au sol) */
#define SLAM_FULL_H        4.0f    /* hauteur de chute pour 100% des degats (0 = desactive) */
#define SLAM_RADIUS        2.5f    /* rayon de base de l'onde */
#define SLAM_RADIUS_PER_MS 0.06f   /* rayon en plus par m/s de chute */
#define SLAM_DMG           25.0f   /* degats de base de l'onde */
#define SLAM_DMG_PER_MS    1.5f    /* degats en plus par m/s de chute */
#define SLAM_REACH_UP      1.0f    /* l'onde ne touche que les pieds a moins de 1 m du sol : un saut simple l'esquive */
#define SLAM_REACH_DOWN    1.0f    /* ni ce qui est plus bas que le sol d'impact */
#define SLAM_KNOCK_UP      12.0f   /* recul vertical des cibles */
#define SLAM_KNOCK_OUT     8.0f    /* recul horizontal au centre */
#define SLAM_BOUNCE        9.0f    /* rebond si l'onde touche */
#define SLAM_STUN          0.6f    /* immobilisation si l'onde rate */
#define SLAM_CD            0.8f    /* recharge apres l'atterrissage */
#define SLAM_BREAK_VY      -15.0f  /* poussee vers le haut qui casse le pique */
#define SLAM_PARRY_GRACE   0.2f    /* tolerance de l'hote pour parer l'onde */
#define SLAM_PARRIED_UP    12.0f   /* attaquant pare : renvoye en l'air */
#define ST_SLAM            0x10    /* NetPState.pad : pique en cours */
enum { ANVIL_IDLE, ANVIL_DIVE, ANVIL_STUN };

/* --- Scie : disque-boomerang (une seule, ni jauge ni recharge : la scie est sa munition) --- */
#define SAW_SPEED        32.0f   /* vitesse de lancer, plus la vitesse du joueur dans l'axe */
#define SAW_MIN_SPEED    20.0f   /* plancher si on lance en reculant */
#define SAW_GRAVITY      4.5f    /* gravite legere : ~1 m de chute sur 22 m */
#define SAW_SPAWN_UP     0.25f   /* la scie part au-dessus des yeux (elle partait trop bas) */
#define SAW_DMG          30.0f   /* degats de base... */
#define SAW_DMG_PER_MS   1.5f    /* ...plus 1,5 par m/s de vitesse relative au-dela de l'offset */
#define SAW_REL_OFFSET   12.0f   /* cible immobile (rel 32) : 60 PvE / 42 PvP, 3 touches (choix du 27/10) */
#define SAW_CD           0.4f    /* delai minimal entre deux lancers */
#define SAW_RETURN_SPEED 36.0f   /* retour guide apres une touche */
#define SAW_RECALL_SPEED 45.0f   /* rappel au grappin */
#define SAW_HOME_MAX     2.5f    /* retour ou rappel plus long : rattrapee d'office */
#define SAW_CATCH_R      1.5f    /* distance de ramassage / rattrapage */
#define SAW_HOOK_R       0.6f    /* sphere que le crochet doit toucher */
#define SAW_RADIUS       0.35f   /* rayon du disque (rendu) */
#define SAW_EMBED        0.12f   /* profondeur d'enfoncement dans la face */
#define SAW_HIT_GROW     0.25f   /* AABB des joueurs agrandie */
#define SAW_DASH         0.5f    /* dash rendu au rattrapage apres une touche */
#define SAW_COIN_R       0.8f    /* frolement d'une piece */
#define SAW_COIN_KICK    11.0f   /* vitesse verticale donnee a la piece */
#define SAW_COIN_LIFE    2.0f    /* vie ajoutee a la piece */
#define SAW_PARRY_MULT   1.25f   /* scie paree par un adversaire : 1,25x sa vitesse (au moins PARRY_SPEED) */
#define SAW_BOUNCE_MULT  1.5f    /* renvoi : 1,5x la vitesse de lancer (48 m/s), plus la vitesse du joueur */
#define SAW_BOUNCE_MAX   70.0f   /* vitesse maximale d'un renvoi */
#define SAW_BOUNCE_AIM_COS 0.9925f /* aide a la visee du renvoi : 7 degres (parry adverse : 0.988) */
#define SAW_BOUNCE_AIM_RANGE 40.0f /* portee de l'aide a la visee */
#define SAW_BOUNCE_CHAIN 3       /* renvois consecutifs max sans rattraper la scie */
#define SAW_RETURN_PARRY_DMG_MULT 0.9f /* renvoi par parry : lancer + renvoi ne tue plus une cible immobile (choix du 27/10) */
#define SAW_LOST_T       3.0f    /* sous le plan de mort : revient apres 3 s */
#define SAW_LOST_Y       -20.0f  /* plan de mort des cartes fermees (sinon killY de la carte, map_lost_y) */
#define SAW_FLY_MAX      12.0f   /* garde-fou : vol trop long = perdue */
#define SAW_RESYNC       2.0f    /* reemission periodique de l'etat plante (joueurs arrives en cours) */
#define SAW_WHIRR        0.18f   /* vrombissement rejoue en vol */
#define SAW_RECALL_HURTS 0       /* option : la scie rappelee blesse le premier adversaire traverse */
enum { SAW_FLY, SAW_PARRIED, SAW_PLANTED, SAW_RETURN, SAW_RECALL, SAW_LOST };
enum { SAWE_THROW, SAWE_PLANT, SAWE_HIT, SAWE_CATCH, SAWE_RECALL, SAWE_BOUNCE };       /* EV_SAW.sub */
enum { SAW_HOW_RETURN, SAW_HOW_FOOT, SAW_HOW_RECALL, SAW_HOW_LOST, SAW_HOW_PARRIED };   /* EV_SAW.actor (rattrapage) */

/* --- Tesla : arc electrique, faisceau continu, chaine, relais sur piece --- */
#define TESLA_PULSE        0.1f    /* 10 impulsions par seconde (P.fireCd) */
#define TESLA_COST         2       /* batterie par impulsion : 20 par seconde, aucune recharge passive */
#define TESLA_RANGE        14.0f   /* portee de base */
#define TESLA_RANGE_PER_MS 0.3f    /* portee en plus par m/s de vitesse (vitesse totale) */
#define TESLA_RANGE_MAX    28.0f
#define TESLA_AIM_PAD      0.4f    /* aide a la visee : les cibles grossissent de 0,4 m (cylindre, pas un cone) */
#define TESLA_DMG          7.0f    /* par impulsion, cible principale */
#define TESLA_CHAIN_N      2       /* sauts en plus */
#define TESLA_CHAIN_R      6.0f    /* distance max d'un saut */
#define TESLA_CHAIN_MULT   0.6f
#define TESLA_RELAY_T      2.0f    /* duree du relais, puis la piece grille */
#define TESLA_RELAY_R      6.0f
#define TESLA_RELAY_DMG    5.0f
#define TESLA_RELAY_MAX    3       /* relais actifs par joueur */
#define TESLA_AGG          0.2f    /* cumul : un envoi (net_hit / damage_enemy) par cible toutes les 0,2 s */
#define TESLA_KNOCK_PVE    0.3f    /* petit recul par impulsion sur les drones (aucun en PvP) */
#define TESLA_STUN         0.4f    /* tireur dont la decharge est paree : plus de tir a distance */
#define TESLA_REFLECT_DMG  15.0f   /* PvP direct, ni multiplie ni plafonne */
#define TESLA_PARRY_GRACE  0.25f   /* tolerance de l'hote : fenetre de cumul + latence */
#define TESLA_LOCK_T       0.3f    /* victime : l'arc dessine finit sur moi apres un EV_HIT W_TESLA */
#define ST_ZAP             0x80    /* NetPState.pad : arc Tesla actif */
enum { TK_NONE, TK_MUZZLE, TK_HAND, TK_ENEMY, TK_PLAYER, TK_COIN, TK_POINT };   /* extremites d'un lien d'arc */
/* --- fin Tesla --- */
/* --- fin Scie --- */

/* --- Lance de joute : estoc a l'appui, lance couchee si la touche reste tenue ; la joute part toute seule --- */
#define LANCE_TAP          0.15f   /* relache avant : estoc seul ; tenu apres : lance couchee */
#define LANCE_HOLD_MAX     2.0f    /* duree maximale de la lance couchee (depuis l'appui) */
#define LANCE_REACH        3.5f    /* portee de la lance couchee (bord de la cible) */
#define LANCE_CONE         0.92f   /* cos du demi-angle (~23 degres), autour de la vue */
#define LANCE_VC_MIN       10.0f   /* vitesse de rapprochement minimale pour toucher */
#define LANCE_VC_CAP       30.0f   /* plafond de v_c pour les degats (160 PvE) */
#define LANCE_DMG          10.0f
#define LANCE_DMG_PER_MS   5.0f    /* degats en plus par m/s de rapprochement */
#define LANCE_GIVE         0.6f    /* part de ta vitesse transmise a la cible */
#define LANCE_KEEP         0.5f    /* part de ta vitesse que tu gardes */
#define LANCE_KNOCK_MAX    28.0f   /* norme maximale du recul transmis */
#define LANCE_HIT_CD       1.0f    /* recharge apres une joute reussie */
#define LANCE_MISS_CD      0.5f    /* recharge apres une lance couchee sans touche */
#define LANCE_PARRIED_CD   1.5f    /* lance paree ou joute perdue */
#define LANCE_THRUST_DMG   30.0f   /* estoc */
#define LANCE_THRUST_PER_MS 2.0f
#define LANCE_THRUST_RANGE 3.0f
#define LANCE_THRUST_CONE  0.80f   /* estoc : cone plus etroit que l'epee */
#define LANCE_THRUST_CD    0.5f
#define LANCE_CLASH_DIFF   2.0f    /* ecart de vitesse pour gagner un choc de joute */
#define LANCE_CLASH_COS    0.85f   /* hote : la lance adverse doit viser l'attaquant (tolerance) */
#define LANCE_CLASH_GRACE  0.15f   /* hote : tolerance de latence sur ST_LANCE */
#define LANCE_JOUST_LOCK   0.3f    /* hote : une joute resolue ignore la touche reciproque */
#define LANCE_UNHORSE      11.0f   /* recul des deux joueurs desarconnes */
#define LANCE_PREVIEW      30.0f   /* HUD : distance max de l'apercu de v_c */
#define ST_LANCE           0x20    /* NetPState.pad : lance couchee */
enum { LS_IDLE, LS_THRUST, LS_COUCH };
/* --- fin Lance --- */

/* --- Ventouse : mines collantes, detonation a distance (deux commandes : toutes, ou la plus ancienne) --- */
#define MINE_SPEED         24.0f   /* vitesse de lancer, ajoutee a la vitesse du joueur */
#define MINE_GRAVITY       16.0f   /* comme les pieces */
#define MINE_CD            0.55f   /* entre deux lancers */
#define MINE_MAX_ACTIVE    2       /* la 3e fait sauter la plus ancienne */
#define MINE_FLY_LIFE      3.0f    /* une mine qui ne colle a rien explose en l'air */
#define MINE_FUSE          3.0f    /* explosion automatique, comptee depuis le collage */
#define MINE_ARM_FLY       0.10f   /* detonation possible apres 0,1 s de vol */
#define MINE_ARM_PLAYER    0.6f    /* collee sur un joueur : temps laisse pour parer */
#define MINE_RADIUS        4.0f
#define MINE_DMG           90.0f   /* PvE au centre, 30 % au bord */
#define MINE_KNOCK         15.0f   /* recul (roquette : 13) */
#define MINE_BOOST_MULT    1.5f    /* rail ou piece sur une mine */
#define MINE_BOOST_RADIUS  5.0f
#define MINE_HIT_R         0.45f   /* rayon de touche d'une mine (rail, plomb, arc) */
#define MINE_STICK_GROW    0.15f   /* AABB des joueurs et rayon des drones agrandis pour coller */
#define MINE_SKIN          0.06f   /* decollement de la surface */
#define MINE_BEEP_SLOW     0.5f    /* intervalle du bip au collage */
#define MINE_BEEP_FAST     0.08f   /* intervalle du bip juste avant l'explosion */
enum { MS_NONE, MS_FLY, MS_WORLD, MS_ENEMY, MS_PLAYER };
enum { MINE_EV_THROW, MINE_EV_WORLD, MINE_EV_PLAYER, MINE_EV_BOOM, MINE_EV_TORN, MINE_EV_CANCEL };   /* EV_MINE.sub */
enum { MX_REMOTE, MX_FUSE, MX_THIRD, MX_SHOT, MX_FLY, MX_COUNT };   /* origine des explosions (journal) */
/* --- fin Ventouse --- */
/* --- Fleau a chaine : controle a longue portee, attire la cible vers soi (lanceur de combos) --- */
#define FLAIL_RANGE         6.0f    /* portee de base */
#define FLAIL_RANGE_PER_MS  0.05f   /* +0.05 m par m/s de vitesse relative */
#define FLAIL_ARC_COS       0.34f   /* cos du demi-angle horizontal (70 deg, arc de 140) */
#define FLAIL_VSLAB         0.6f    /* ecart vertical max autour du pitch (rad) */
#define FLAIL_CD            0.6f
#define FLAIL_WHIFF_CD      0.9f    /* coup dans le vide */
#define FLAIL_DMG           18.0f
#define FLAIL_DMG_PER_MS    1.5f
#define FLAIL_PULL          12.0f   /* traction vers le lanceur (recul negatif) */
#define FLAIL_PULL_UP       3.0f
#define FLAIL_DRONE_STOP    2.5f    /* un drone attire s'arrete a cette distance */
#define FLAIL_ANIM          0.30f   /* duree de l'animation de la chaine */
#define FLAIL_PARRY_WINDOW  0.10f
#define FLAIL_PARRY_RANGE   6.0f    /* portee de parry des projectiles */
#define FLAIL_PARRY_SPEED   0.8f    /* renvoi plus lent */
#define FLAIL_PARRIED_PULL  16.0f   /* fleau pare : le lanceur est tire vers le pareur */
#define FLAIL_PARRIED_UP    4.0f
#define FLAIL_COIN_SPEED    45.0f   /* piece frappee */
#define FLAIL_COIN_DMG      40.0f   /* x PVP_SCALE = 28 en PvP */
#define FLAIL_COIN_LIFE     1.5f    /* 67 m de vol */
#define FLAIL_COIN_SEEK     60.0f   /* portee de recherche de cible */
#define FLAIL_COIN_HIT_R    0.4f    /* marge de touche de la piece frappee */
#define FLAIL_COMBO_T       1.0f    /* fenetre du combo fleau -> rail (visuel seulement) */
/* --- fin Fleau --- */
/* --- Poings a impulsion : combo de 3 coups (le 3e est un uppercut), punch-jump sur le decor --- */
#define FIST_GAP            0.22f   /* entre deux coups du combo */
#define FIST_END_CD         0.70f   /* recharge apres le 3e coup */
#define FIST_COMBO_WIN      0.45f   /* au-dela, sans nouveau coup, le combo revient au coup 1 */
#define FIST_WHIFF_CD       0.60f   /* combo sans touche ni parry : recharge en plus */
#define FIST_BUFFER         0.12f   /* un appui un peu en avance est memorise */
#define FIST_ANIM           0.16f   /* duree de l'animation d'un coup */
#define FIST_RANGE          1.9f
#define FIST_RANGE_PER_MS   0.06f
#define FIST_CONE           0.6f    /* cosinus du demi-angle (~53 degres), une seule cible */
#define FIST_DMG            14.0f   /* coups 1 et 2 */
#define FIST_DMG_PER_MS     1.5f
#define FIST_UP_DMG         20.0f   /* coup 3 : uppercut */
#define FIST_UP_DMG_PER_MS  2.5f
#define FIST_KNOCK          2.5f    /* recul faible : la cible reste a portee */
#define FIST_UP_KNOCK       11.0f   /* uppercut : recul vertical */
#define FIST_AIR_HANG       2.0f    /* touche en l'air sur une cible en l'air : on reste suspendu */
#define FIST_DASH           0.35f   /* dash rendu par coup qui touche */
#define FIST_PARRY_WIN      0.06f   /* fenetre de parry de chaque coup */
#define FIST_PARRY_GRACE    0.05f   /* tolerance de l'hote quand la victime joue les Poings */
#define FIST_STAGGER_KNOCK  8.0f    /* un recul recu au moins aussi fort casse le combo */
#define FIST_STAGGER_CD     0.3f
#define FIST_PJ_RANGE       1.9f    /* coup dans le decor : distance maximale */
#define FIST_PJ_FLOOR_RANGE 1.9f    /* idem pour le sol (2.6 propose, voir fiche) */
#define FIST_PJ_FLOOR_DELAY 0.1f    /* pas de coup au sol dans les 0,1 s qui suivent le decollage (super-saut) */
#define FIST_PJ_PUSH        11.0f   /* composante normale minimale apres l'impulsion */
#define FIST_PJ_HCAP        20.0f   /* le punch-jump n'accelere plus au-dela (il reoriente) */
#define FIST_PJ_CD          0.25f   /* partage avec P.wallJumpCd */
#define FIST_UP_COMBO_T     0.5f    /* bot : apres un uppercut qui touche, le rail suit */
/* --- fin Poings --- */
/* --- Egide (bouclier) : maintenir = leve, cone frontal, jauge tenue par l'hote, parry parfait qui renvoie le rail --- */
#define SHIELD_COS           -0.34f  /* bloque si dot(dir du coup, face) < -0.34 : demi-angle 70 deg */
#define SHIELD_COS_HOST      -0.17f  /* tolerance de l'hote pour confirmer un blocage : demi-angle 80 deg */
#define SHIELD_GAUGE_MAX     120.0f  /* jauge : degats PvP absorbables */
#define SHIELD_REGEN         30.0f   /* regeneration par seconde, bouclier baisse */
#define SHIELD_BROKEN_T      3.0f    /* bouclier brise : indisponible 3 s */
#define SHIELD_BLAST_FRAC    0.5f    /* part d'un souffle bloquee de face */
#define SHIELD_KNOCK_MULT    0.5f    /* recul d'un coup bloque */
#define SHIELD_GROUND_SPEED  7.0f    /* vitesse au sol bouclier leve */
#define SHIELD_BRAKE         40.0f   /* freinage au sol au-dessus de 7 m/s */
#define SHIELD_AIR_MULT      0.5f    /* acceleration aerienne bouclier leve */
#define SHIELD_TAP           0.15f   /* relache avant : coup de bouclier */
#define SHIELD_PARRY_WIN     0.15f   /* fenetre du parry parfait apres la levee */
#define SHIELD_REARM         0.3f    /* baisse au moins 0.3 s pour rouvrir la fenetre parfaite */
#define SHIELD_GRACE         0.15f   /* tolerance de latence de l'hote (comme MELEE_PARRY_GRACE) */
#define SHIELD_REFLECT_PVP   60.0f   /* rail renvoye par un parry parfait */
#define SHIELD_RIP_T         1.0f    /* bouclier arrache par un grappin adverse */
#define SHIELD_BASH_DMG      25.0f
#define SHIELD_BASH_PER_MS   3.0f
#define SHIELD_BASH_RANGE    2.2f
#define SHIELD_BASH_KNOCK    14.0f
#define SHIELD_BASH_CD       0.5f
#define SHIELD_BASH_WHIFF_CD 0.8f    /* coup de bouclier dans le vide */
#define SHIELD_BASH_ANIM     0.18f   /* animation de poussee */
#define SHIELD_HOST_CONVERT  1       /* l'hote convertit en bloc un coup direct qu'il voit arriver de face (favorise le defenseur) */
#define ST_SHIELD            0x40    /* NetPState.pad : bouclier leve */
enum { SHS_DOWN, SHS_UP, SHS_BROKEN };
enum { SHE_CLAIM, SHE_CLAIM_BLAST, SHE_BLOCK, SHE_BREAK, SHE_PERFECT, SHE_DENIED, SHE_RIP };   /* EV_SHIELD.sub */
enum { SHB_DIRECT, SHB_BLAST, SHB_NEVER };                /* hit_player : coup direct, souffle (50 %), jamais bloque */
/* --- fin Egide --- */

/* Axe tir : chaque touche successive augmente degats et cadence, un tir rate remet a zero */
#define STREAK_MAX        8
#define STREAK_DMG        0.15f   /* +15% de degats par touche */
#define STREAK_RATE       0.05f   /* +5% de cadence par touche */

/* Loadout : 1 arme de melee + 2 armes a distance. Aucune recharge passive : chaque arme a une jauge
 * de points, remplie par la melee et les cellules ; jauge pleine = 1 munition. */
enum { MK_SWORD, MK_ANVIL, MK_LANCE, MK_FLAIL, MK_FIST, MK_SHIELD, MK_COUNT };
enum { WP_RAIL, WP_ROCKET, WP_SHOTGUN, WP_KINETIC, WP_SAW, WP_MINE, WP_TESLA, WP_COUNT };
enum { AP_HIT, AP_KILL, AP_PARRY, AP_CELL, AP_LOST, AP_COUNT };   /* origine des points (journal) */
#define PTS_MELEE_KILL     100     /* kill en melee, toutes armes */
#define PTS_PARRY          50      /* parry reussi, une fois par coup */
#define PTS_LANCE_JOUST    100     /* joute de la Lance (l'estoc = meleePts) */
#define PTS_FIST_COMBO_MAX 50      /* Poings : au plus 50 points par combo */
#define PTS_SHIELD_PERFECT 100     /* Egide : parry parfait (remplace PTS_PARRY) */
#define PICKUP_SMALL       100     /* petite cellule : points a chaque arme du loadout */
#define PICKUP_BIG         150     /* grande cellule (endroits exposes) */
#define REFUND_UNDO_T      0.5f    /* coup declare pare par l'hote : on rend ses points s'il a moins de 0,5 s */
#define SWITCH_TIME        0.22f   /* changement d'arme */
#define WHEEL_QUIET        0.15    /* molette : repos (s) qui separe deux gestes ; une rafale = un seul changement */
#define WHEEL_COOLDOWN     0.30    /* molette : delai minimal (s) entre deux changements */
#define WP_FLASH_T         0.9     /* nom de l'arme affiche (s) apres un changement */
#define SCREEN_POLL_T      1.0f    /* second ecran : on recompte les moniteurs toutes les secondes */
#define SCREEN_MAP_FPS     30.0f   /* images par seconde de la carte sur le second ecran */
#define FPS_CAP_DEFAULT    240     /* plafond d'images par seconde des prereglages BASSE a ULTRA (0 = sans limite ; max_fps=) */
#define FPS_CAP_TOASTER    60      /* ...du prereglage GRILLE-PAIN (celui par defaut) : moins de chauffe, moins de baisse de frequence */
enum { GFX_Q_TOASTER, GFX_Q_LOW, GFX_Q_MID, GFX_Q_ULTRA, GFX_Q_COUNT };   /* prereglages QUALITE (graphismes= de velocity.ini) */
#define PAD_SENS_DEFAULT   3.2f    /* manette : visee au stick droit, vitesse maxi (rad/s, ~180 deg/s) */
#define PAD_PITCH_K        0.7f    /* ...l'axe vertical tourne moins vite */
#define PAD_DEADZONE       0.12f   /* zone morte des sticks (part de la course) */
#define PAD_STICK_PRESS    0.4f    /* stick gauche : pousse au-dela = touche K_PAD_LS_* appuyee */
#define PAD_TRIGGER_PRESS  0.35f   /* L2 / R2 : enfonce au-dela = appuye */
#define PAD_CURSOR_SPEED   1100.0f /* menus : curseur au stick gauche (pixels/s a fond) */
#define PAD_SCAN_T         2.0f    /* sans manette : on recherche toutes les 2 s */
#define MENU_SHOT_T        9.0f    /* menu principal : duree d'un plan de camera (s) */
#define MENU_FADE_T        0.6f    /* ...fondu au noir a chaque changement de plan (s) */
#define WORLD_SECTOR_M     24.0f   /* cote d'un secteur du monde (m) : seuls ceux dans le champ de vision sont dessines */
#define RENDER_SCALE_TOASTER 100   /* resolution de la 3D (% de la fenetre) des prereglages : l'agrandissement coute ~0,2 ms
                                      (carte) a ~5 ms (logiciel), mesure : 67 % ne gagnait rien (Mac M4, rendu logiciel) */
#define RENDER_SCALE_SOFT  50      /* ...quand le pilote graphique manque (rendu logiciel) : 30 -> 24 ms/image mesure */
#define MAPS_HEAT_R        9.0f    /* carte de chaleur : rayon d'une tache (m) */
#define KILLCAM_T          1.6f    /* camera de mort : duree (s), avant l'ecran de respawn (RESPAWN_TIME 2,5 s) */
#define KILLCAM_TURN       0.45f   /* ...temps pour pivoter vers le tueur (s) */
#define KILLCAM_BACK       2.5f    /* ...recul (m) et montee (m) de la camera */
#define KILLCAM_UP         1.2f
#define KILLCAM_ZOOM       0.35f   /* ...zoom sur le tueur (part du champ de vision retiree) */
#define KILLCAM_SOLO_R     60.0f   /* solo : ennemi le plus proche qui me voit, dans ce rayon (m) */
#define HP_GHOST_HOLD      0.4     /* barre de vie : la partie perdue reste affichee (s)... */
#define HP_GHOST_RATE      80.0f   /* ...puis se vide a cette vitesse (PV/s) */
#define DRYFIRE_CD         0.35f   /* delai entre deux clics a sec */
#define ST_MASK            0xF0    /* NetPState.pad : bits 4-7 = etats d'arme, bits 0-3 = meleeKind */

/* points de melee par coup qui touche (Lance : estoc ; Egide : coup de bouclier) */
static const int meleePts[MK_COUNT] = { 50, 75, 50, 35, 25, 50 };

/* gauge : points pour gagner 1 munition (0 = pas de jauge). Tesla : points par unite de batterie */
typedef struct { const char *name; int start, max; float gauge; float r, g, b; } AmmoDef;
static const AmmoDef ammoDef[WP_COUNT] = {
    { "RAILGUN",   2,  2,   250, 0.40f, 0.85f, 1.00f },   /* tres haut degats : 2 tirs, tres lent */
    { "ROQUETTES", 4,  8,   50,  1.00f, 0.60f, 0.20f },   /* 1 coup d'epee = 1 roquette */
    { "EVENTAIL",  3,  6,   100, 1.00f, 0.85f, 0.35f },
    { "ELAN",      3,  6,   80,  0.55f, 1.00f, 0.55f },
    { "SCIE",      1,  1,   0,   0.90f, 0.90f, 0.95f },   /* objet unique : aucune jauge */
    { "VENTOUSE",  1,  3,   150, 1.00f, 0.35f, 0.35f },
    { "TESLA",     60, 100, 2.5f, 0.60f, 0.70f, 1.00f },  /* 1 point = 0,4 de batterie */
};
static const char *meleeNames[MK_COUNT] = { "EPEE", "ENCLUME", "LANCE", "YOYO", "IMPULSION", "EGIDE" };
static unsigned char wpReady[WP_COUNT] = { 1, 1, 1, 1, 1, 1, 1 };   /* chaque arme codee passe sa case a 1 */
static unsigned char mkReady[MK_COUNT] = { 1, 1, 1, 1, 1, 1 };
/* competences : nom, charges max, secondes par charge, couleur, description (LOADOUT) */
typedef struct { const char *name; int max; float recharge; float r, g, b; const char *desc; } SkillDef;
static const SkillDef skillDef[SK_COUNT] = {
    { "PIECE",      COIN_MAX, COIN_RECHARGE, 1.00f, 0.82f, 0.30f, "lance une piece : le rail qui la touche ricoche (x1,5)" },
    { "MARQUE",     2, 6,  0.85f, 0.35f, 1.00f, "marque une cible 4 s, visible a travers les murs : tes touches x1,25" },
    { "CHARGE",     1, 8,  1.00f, 0.55f, 0.20f, "maintenir 0,6 s : ta prochaine touche dans les 2 s fait x1,5 (perdue si touche)" },
    { "RICOCHET",   1, 15, 0.40f, 1.00f, 0.70f, "le prochain rail / elan / roquette rebondit 3 fois sur le decor" },
    { "ALL IN ONE", 1, 20, 1.00f, 0.30f, 0.30f, "tire toute la munition de l'arme en main d'un coup, en eventail" },
    { "MIRAGE",     2, 7,  0.40f, 0.85f, 1.00f, "leurre immobile 3 s : absorbe un tir adverse, ton rail y ricoche" },
};
static int wantSkill = SK_COIN;
static int botSkillSet;                                   /* --skill / --loadout a,b,m,s : competence imposee au bot */
static const float wpCd[WP_COUNT] = { RAIL_CD, ROCKET_CD, SG_CD, KIN_CD, 0.4f, MINE_CD, 0.1f };  /* barre de recharge du HUD */

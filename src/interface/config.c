/* VELOCITY - velocity.ini : reglages et touches du joueur
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Interface : menus, tableau des scores, joueurs distants             */
/* ------------------------------------------------------------------ */
enum { ACT_NONE, ACT_SOLO, ACT_HOST, ACT_BROWSE, ACT_QUIT, ACT_BACK, ACT_JOIN_FOUND, ACT_JOIN_IP, ACT_RESUME,
       ACT_RESTART, ACT_LEAVE, ACT_FULLSCREEN, ACT_QUICK, ACT_SPEC_FOUND, ACT_VSBOT, ACT_RACE };
static int settingsOpen, settingsPage, waitAct = -1, waitSlot, uiDown, uiDrag;   /* page : 0 reglages, 1 graphismes, 2 manette */
static int fullscreen;
static char cfgPath[MAX_PATH];

/* plafond d'images par seconde (bouton MAX FPS, max_fps= de velocity.ini) : 0 = sans limite.
   La synchro verticale ne bride pas toujours (fenetre masquee, pilote) : sans plafond le jeu
   peut monter a 1000 images/s et occuper un coeur entier pour rien. */
static int fpsCap = FPS_CAP_TOASTER;                     /* GRILLE-PAIN par defaut (gfx_preset) */
static int gfxSeen, gfxTexSeen;                                       /* velocity.ini avait graphismes= (sinon : premier lancement ou ancien fichier) */
static const int fpsCapList[] = { 0, 30, 60, 90, 120, 144, 165, 240, 360 };

/* prereglage QUALITE : ombres, bloom, resolution de la 3D et plafond d'images vont avec. Le reste (lightmaps, ciel,
   decor) est plafonne par gfx_quality_apply a la prochaine relecture (gfx_reload). */
static void gfx_preset(int q) {
    static const int scale[GFX_Q_COUNT] = { RENDER_SCALE_TOASTER, 100, 100, 100 };
    gfxQuality = q < 0 ? 0 : q >= GFX_Q_COUNT ? GFX_Q_COUNT - 1 : q;
    gfxShadows = gfxQuality >= GFX_Q_MID;
    gfxBloom = gfxQuality == GFX_Q_ULTRA;
    gfxScale = scale[gfxQuality];
    gfxTexOn = gfxDetailOn = gfxQuality != GFX_Q_TOASTER; /* GRILLE-PAIN : sol et murs sans texture, decor sans details */
    fpsCap = gfxQuality == GFX_Q_TOASTER ? FPS_CAP_TOASTER : FPS_CAP_DEFAULT;
}

/* bouton RESOLUTION : 50, 67, 75, 85, 100 % puis retour a 50 */
static int render_scale_cycle(int cur) {
    static const int list[] = { 50, 67, 75, 85, 100 };
    for (int i = 0; i < (int)(sizeof list / sizeof list[0]); i++)
        if (list[i] > cur) return list[i];
    return list[0];
}

static int fps_cap_cycle(int cur) {                       /* valeur suivante du bouton (apres 360 : sans limite) */
    for (int i = 1; i < (int)(sizeof fpsCapList / sizeof fpsCapList[0]); i++)
        if (fpsCapList[i] > cur) return fpsCapList[i];
    return 0;
}

/* echeance de l'image suivante (s) : une periode apres la precedente, pour garder une cadence reguliere ;
   trop en retard (chargement, image lente), on repart de maintenant au lieu de rattraper en rafale */
static double fps_cap_deadline(double prev, double now, int cap) {
    if (cap <= 0) return now;
    double t = prev + 1.0 / cap;
    return t < now - 0.5 / cap ? now : t;
}

static void key_name(int code, char *out, int n) {
    static const char *mouseNames[] = { "Souris G", "Souris D", "Clic molette", "Souris 4", "Souris 5", "Molette" };
    static const char *padNames[] = { "Croix", "Rond", "Carre", "Triangle", "L1", "R1", "L2", "R2", "Share", "Options",
                                      "L3", "R3", "PS", "Pave tactile", "Fleche haut", "Fleche bas", "Fleche gauche",
                                      "Fleche droite", "Stick G haut", "Stick G bas", "Stick G gauche", "Stick G droite" };
    char buf[64];
    if (code <= 0) { snprintf(out, n, "-"); return; }
    if (code >= K_PAD_FIRST && code < K_COUNT) { snprintf(out, n, "%s", padNames[code - K_PAD_FIRST]); return; }
    if (code >= K_LMB) { snprintf(out, n, "%s", mouseNames[code - K_LMB]); return; }
#ifdef _WIN32
    if (GetKeyNameTextA((LONG)code << 16, buf, sizeof buf) > 0) snprintf(out, n, "%s", buf);   /* nom selon le clavier */
    else snprintf(out, n, "touche %d", code);
#else
    mac_key_name(code, buf, sizeof buf);
    snprintf(out, n, "%s", buf);
#endif
}

static const char *bind_text(int a) {                    /* "Z / Fleche haut" : pour l'aide et le HUD */
    static char out[4][64];
    static int slot;
    char k0[32], k1[32];
    slot = (slot + 1) % 4;
    key_name(binds[a][0], k0, sizeof k0);
    key_name(binds[a][1], k1, sizeof k1);
    if (binds[a][1]) snprintf(out[slot], sizeof out[slot], "%s / %s", k0, k1);
    else snprintf(out[slot], sizeof out[slot], "%s", k0);
    return out[slot];
}

static void cfg_save(void) {
    if (botMode || shotMode || !cfgPath[0]) return;
    FILE *f = fopen(cfgPath, "w");
    if (!f) return;
    fprintf(f, "# VELOCITY - reglages (modifiable a la main)" "\n");
    fprintf(f, "name=%s" "\n" "sens=%.5f" "\n" "fov=%.0f" "\n" "volume=%.2f" "\n" "music=%.2f" "\n" "invert_y=%d" "\n" "show_fps=%d" "\n" "fullscreen=%d" "\n",
            myName, sens, fovBase, masterVol, musicVol, invertY, showFps, fullscreen);
    fprintf(f, "impact_frames=%d" "\n" "screen_fx=%d" "\n" "shadows=%d" "\n" "bloom=%d" "\n", impactFrames, screenFx, gfxShadows, gfxBloom);
    fprintf(f, "second_ecran=%d" "\n" "carte_vue=%d" "\n" "camera_mort=%d" "\n", secondScreen, mapView3d, killcamOn);
    fprintf(f, "graphismes=%d" "\n" "resolution=%d" "\n" "ambiance=%d" "\n" "max_fps=%d" "\n", gfxQuality, gfxScale, gfxAmbiance, fpsCap);
    fprintf(f, "textures=%d" "\n" "details=%d" "\n" "pad_sens=%.2f" "\n" "pad_invert_y=%d" "\n", gfxTexOn, gfxDetailOn, padSens, padInvertY);
    fprintf(f, "online=%d" "\n" "teams=%d" "\n" "update_check=%d" "\n", onlineOn, wantTeams, updCheck);
    fprintf(f, "loadout=%d,%d" "\n" "melee=%d" "\n" "skill=%d" "\n" "skin=%d" "\n", wantLoadout[0], wantLoadout[1], wantMelee, wantSkill, wantSkin);
    fprintf(f, "map=%d" "\n" "map_rotate=%d" "\n", menuMap, mapRotate);
    fprintf(f, "regles=%d,%d,%d,%d,%d,%d" "\n" "spec=%d" "\n", wantRules.mode, wantRules.hook, wantRules.ammo, wantRules.shop,
            wantRules.specs, wantRules.minutes, wantSpec);
    if (strcmp(lobbyAddr, LOBBY_DEFAULT)) fprintf(f, "lobby=%s" "\n", lobbyAddr);   /* le defaut peut changer avec une mise a jour */
    for (int a = 0; a < A_COUNT; a++) fprintf(f, "bind_%s=%d,%d" "\n", actId[a], binds[a][0], binds[a][1]);
    for (int a = 0; a < A_COUNT; a++) fprintf(f, "manette_%s=%d" "\n", actId[a], padBinds[a]);
    fclose(f);
}

static int cfg_load(void) {                               /* renvoie 1 si le plein ecran est demande */
    char line[256];
    int fs = 0;
    FILE *f = cfgPath[0] ? fopen(cfgPath, "r") : NULL;
    gfxSeen = gfxTexSeen = 0;
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '='), *v;
        if (!eq || line[0] == '#') continue;
        *eq = 0; v = eq + 1;
        v[strcspn(v, "\r" "\n")] = 0;
        if (!strcmp(line, "name") && v[0]) snprintf(myName, sizeof myName, "%s", v);
        else if (!strcmp(line, "sens")) sens = clampf((float)atof(v), 0.0003f, 0.01f);
        else if (!strcmp(line, "fov")) fovBase = clampf((float)atof(v), 80, 130);
        else if (!strcmp(line, "volume")) masterVol = clampf((float)atof(v), 0, 1);
        else if (!strcmp(line, "music")) musicVol = clampf((float)atof(v), 0, 1);
        else if (!strcmp(line, "invert_y")) invertY = atoi(v) != 0;
        else if (!strcmp(line, "show_fps")) showFps = atoi(v) != 0;
        else if (!strcmp(line, "max_fps")) fpsCap = atoi(v) <= 0 ? 0 : atoi(v) < 20 ? 20 : atoi(v) > 1000 ? 1000 : atoi(v);
        else if (!strcmp(line, "impact_frames")) impactFrames = atoi(v) != 0;
        else if (!strcmp(line, "screen_fx")) screenFx = atoi(v) != 0;
        else if (!strcmp(line, "second_ecran")) secondScreen = atoi(v) != 0;
        else if (!strcmp(line, "carte_vue")) mapView3d = atoi(v) != 0;
        else if (!strcmp(line, "camera_mort")) killcamOn = atoi(v) != 0;
        else if (!strcmp(line, "shadows")) gfxShadows = atoi(v) != 0;
        else if (!strcmp(line, "bloom")) gfxBloom = atoi(v) != 0;
        else if (!strcmp(line, "graphismes")) { gfxQuality = atoi(v) < 0 ? 0 : atoi(v) >= GFX_Q_COUNT ? GFX_Q_COUNT - 1 : atoi(v); gfxSeen = 1; }
        else if (!strcmp(line, "textures")) { gfxTexOn = atoi(v) != 0; gfxTexSeen = 1; }
        else if (!strcmp(line, "details")) gfxDetailOn = atoi(v) != 0;
        else if (!strcmp(line, "pad_sens")) padSens = clampf((float)atof(v), 0.5f, 10);
        else if (!strcmp(line, "pad_invert_y")) padInvertY = atoi(v) != 0;
        else if (!strncmp(line, "manette_", 8)) {
            for (int a = 0; a < A_COUNT; a++)
                if (!strcmp(line + 8, actId[a]) && (atoi(v) == 0 || (atoi(v) >= K_PAD_FIRST && atoi(v) < K_COUNT))) padBinds[a] = atoi(v);
        }
        else if (!strcmp(line, "resolution")) gfxScale = atoi(v) < 50 ? 50 : atoi(v) > 100 ? 100 : atoi(v);
        else if (!strcmp(line, "ambiance")) gfxAmbiance = atoi(v) < 0 ? 0 : atoi(v) > 3 ? 3 : atoi(v);
        else if (!strcmp(line, "online")) onlineOn = atoi(v) != 0;
        else if (!strcmp(line, "teams")) wantTeams = atoi(v) < 0 ? 0 : atoi(v) > TEAM_MAX ? TEAM_MAX : atoi(v);
        else if (!strcmp(line, "update_check")) updCheck = atoi(v) != 0;
        else if (!strcmp(line, "lobby") && v[0]) snprintf(lobbyAddr, sizeof lobbyAddr, "%s", v);
        else if (!strcmp(line, "fullscreen")) fs = atoi(v) != 0;
        else if (!strcmp(line, "loadout")) sscanf(v, "%d,%d", &wantLoadout[0], &wantLoadout[1]);
        else if (!strcmp(line, "melee")) wantMelee = atoi(v);
        else if (!strcmp(line, "skin")) wantSkin = atoi(v) < 0 || atoi(v) >= SKN_COUNT ? 0 : atoi(v);
        else if (!strcmp(line, "map")) menuMap = atoi(v) >= 0 && atoi(v) < NMAPS ? atoi(v) : 0;
        else if (!strcmp(line, "map_rotate")) mapRotate = atoi(v) != 0;
        else if (!strcmp(line, "regles")) {
            MatchRules r = wantRules;
            if (sscanf(v, "%d,%d,%d,%d,%d,%d", &r.mode, &r.hook, &r.ammo, &r.shop, &r.specs, &r.minutes) == 6 &&
                r.mode >= 0 && r.mode < MODE_COUNT && r.ammo >= 0 && r.ammo < AMMO_COUNT && r.minutes > 0 && r.minutes <= 60) wantRules = r;
        }
        else if (!strcmp(line, "spec")) wantSpec = atoi(v) >= 0 && atoi(v) < SPEC_COUNT ? atoi(v) : 0;
        else if (!strcmp(line, "skill")) wantSkill = atoi(v) >= 0 && atoi(v) < SK_COUNT ? atoi(v) : SK_COIN;
        else if (!strncmp(line, "bind_", 5))
            for (int a = 0; a < A_COUNT; a++)
                if (!strcmp(line + 5, actId[a])) {
                    int b0 = 0, b1 = 0;
                    if (sscanf(v, "%d,%d", &b0, &b1) >= 1) {
                        binds[a][0] = (b0 >= 0 && b0 < K_COUNT) ? b0 : 0;
                        binds[a][1] = (b1 >= 0 && b1 < K_COUNT) ? b1 : 0;
                    }
                }
    }
    fclose(f);
    /* ancien fichier (quality= de 0 a 2, sans graphismes=) : on repart du prereglage GRILLE-PAIN, ombres et bloom compris ;
       le joueur remonte dans PARAMETRES > QUALITE */
    if (!gfxSeen) gfx_preset(GFX_Q_TOASTER);
    else if (!gfxTexSeen) gfxTexOn = gfxDetailOn = gfxQuality != GFX_Q_TOASTER;   /* fichier d'avant TEXTURES / DETAILS */
    if (!loadout_valid(wantLoadout, wantMelee)) { wantLoadout[0] = WP_RAIL; wantLoadout[1] = WP_ROCKET; wantMelee = MK_SWORD; }
    return fs;
}

static void assign_bind(int code) {
    if (waitAct < 0) return;
    if (code > 0)                                         /* une touche ne sert qu'a une seule commande */
        for (int a = 0; a < A_COUNT; a++)
            for (int k = 0; k < 2; k++)
                if (binds[a][k] == code) binds[a][k] = 0;
    binds[waitAct][waitSlot] = code;
    waitAct = -1;
    play(S_HIT, 0.4f, 1.5f);
    cfg_save();
}
static int uiMX, uiMY, uiClick, uiFocus, pendingAction, pendingArg;   /* uiFocus : 1 = pseudo, 2 = IP */
static char ipField[32];

static const float pcol[MAXP][3] = {
    { 0.3f, 0.8f, 1.0f }, { 1.0f, 0.45f, 0.3f }, { 0.5f, 1.0f, 0.4f }, { 1.0f, 0.85f, 0.3f },
    { 0.8f, 0.45f, 1.0f }, { 1.0f, 0.4f, 0.7f }, { 0.4f, 1.0f, 0.9f }, { 0.9f, 0.9f, 0.9f },
};
/* couleur d'un joueur : la sienne en chacun pour soi ; en equipes, celle de l'equipe, eclaircie selon le rang */
static const float *pcolor(int i) {
    static float c[MAXP + 1][3];
    if (i < 0 || i > MAXP) i = MAXP;
    if (teamSize <= 0 || i >= MAXP) return pcol[i < MAXP ? i : MAXP - 1];
    int t = NP[i].team & 1, rank = 0;
    for (int j = 0; j < i; j++) rank += NP[j].active && NP[j].team == t;
    float k = 0.18f * rank;
    for (int a = 0; a < 3; a++) c[i][a] = teamCol[t][a] + (1 - teamCol[t][a]) * k * 0.6f;
    return c[i];
}
static const char *weaponNames[W_COUNT] = { "RAIL", "ROQUETTE", "EPEE", "GRAPPIN", "PARRY", "SOLO", "RICOCHET",
                                            "ENCLUME", "LANCE", "YOYO", "POINGS", "EGIDE", "EVENTAIL", "ELAN",
                                            "SCIE", "VENTOUSE", "TESLA", "VIDE" };

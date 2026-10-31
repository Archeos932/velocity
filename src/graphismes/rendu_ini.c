/* VELOCITY - lecture de graphismes/rendu.ini, ambiances, qualite
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ---- reglages graphiques modifiables : graphismes/rendu.ini ---------------------------------
 * Tout ce qui fait l'aspect du jeu (soleil, ombres, brouillard, ciel, ville, bloom, textures) est
 * ici, avec la valeur par defaut. Le fichier graphismes/rendu.ini les remplace (cle = valeurs) ;
 * F5 en jeu le relit et reconstruit textures et lightmaps. Une cle absente garde son defaut.
 * Les textures sol.bmp, mur.bmp, grain.bmp du meme dossier remplacent les textures generees. */
static float sunCol[3] = { 1.00f, 0.84f, 0.66f }, skyCol[3] = { 0.52f, 0.54f, 0.74f },
             bounceCol[3] = { 0.16f, 0.13f, 0.12f }, padGlowCol[3] = { 0.25f, 0.85f, 1.0f };
static float gxSunDir[3] = { 0.55f, 0.66f, -0.82f }, gxSunPow = 1.3f, gxPad[2] = { 0.9f, 9 };
static float gxLmDens = 3, gxAoRays = 12, gxAoRange = 4.5f, gxAoMin = 0.15f, gxLmBlur = 2;
static float gxObjAmb[3] = { 0.40f, 0.40f, 0.52f }, gxObjSun[2] = { 0.25f, 0.65f };
static float gxVmAmb[3] = { 0.42f, 0.42f, 0.52f }, gxVmSun[2] = { 0.30f, 0.45f };
static float gxShadowCol[3] = { 0.02f, 0.02f, 0.06f }, gxShadowAlpha = 0.5f, gxShadowMaxH = 30;
static float gxFogCol[3] = { 0.34f, 0.30f, 0.44f }, gxFog[2] = { 50, 240 };
/* couleurs de la carte : l'Arene suit rendu.ini (et ses ambiances), les autres cartes ont leur ciel (MapDef) */
static const float *map_fog_col(void) { return curMap == 0 ? gxFogCol : maps[curMap].fog; }
static float gxSkyZen[3] = { 0.025f, 0.035f, 0.11f }, gxSkyMid[3] = { 0.13f, 0.12f, 0.27f }, gxSkyHor[3] = { 0.36f, 0.30f, 0.45f };
static float gxSunGlow[3] = { 0.30f, 0.55f, 0.42f }, gxStars = 260, gxSunSize[3] = { 8, 30, 120 };
static float gxCity[5] = { 70, 260, 120, 30, 85 }, gxCityCol[3] = { 0.16f, 0.14f, 0.26f }, gxCityWin = 0.35f;
static float gxBloom[4] = { 0.7f, 3, 2, 1.6f };
static float gxTexScale[3] = { 0.5f, 0.5f, 2.37f }, gxFloorTex[2] = { 64, 0.80f }, gxWallTex[3] = { 128, 64, 0.62f };
static float gxGrain = 0.22f, gxEdges = 0.25f;
static float gxTrail[3] = { 16, 0.55f, 0.35f }, gxOutline = 3;
static float gxMap3d[3] = { 30, 2.0f, 52 };                /* carte du 2e ecran en 3D : champ (deg), recul (x taille), inclinaison (deg) */
static float gxHudPanel = 0.45f;                         /* opacite des fonds sous les textes du HUD (0 = aucun) */
static float gxNeonC[3] = { 0.2f, 0.95f, 1.0f }, gxNeonM[3] = { 1.0f, 0.25f, 0.85f }, gxNeonO[3] = { 1.0f, 0.6f, 0.15f };
static float gxNeon[3] = { 1.0f, 0.55f, 5.0f }, gxBleed = 0.35f;   /* neons : eclat, lueur sur le decor, portee ; rebond colore */
static float gxLights = 1.0f;                             /* lumieres dynamiques (armes, explosions), 0 = aucune */
static float gxClouds[2] = { 18, 0.35f }, gxCloudCol[3] = { 0.62f, 0.5f, 0.66f }, gxDust = 120, gxFlare = 0.5f;
static float gxRain = 0, gxStorm = 0;                     /* pluie (gouttes), orage (eclairs par minute) */   /* trainees : vitesse min, opacite, largeur ; contours */
typedef struct { const char *key; float *v; int n; } GfxKey;
static const GfxKey gfxKeys[] = {
    { "soleil_direction", gxSunDir, 3 }, { "soleil_couleur", sunCol, 3 }, { "soleil_force", &gxSunPow, 1 },
    { "ciel_lumiere", skyCol, 3 }, { "rebond_sol", bounceCol, 3 },
    { "pads_couleur", padGlowCol, 3 }, { "pads_lueur", gxPad, 2 },
    { "lightmap_densite", &gxLmDens, 1 }, { "occlusion_rayons", &gxAoRays, 1 }, { "occlusion_portee", &gxAoRange, 1 },
    { "occlusion_min", &gxAoMin, 1 }, { "lightmap_flou", &gxLmBlur, 1 },
    { "objets_ambiant", gxObjAmb, 3 }, { "objets_soleil", gxObjSun, 2 },
    { "arme_ambiant", gxVmAmb, 3 }, { "arme_soleil", gxVmSun, 2 },
    { "ombre_couleur", gxShadowCol, 3 }, { "ombre_opacite", &gxShadowAlpha, 1 }, { "ombre_hauteur_max", &gxShadowMaxH, 1 },
    { "brouillard_couleur", gxFogCol, 3 }, { "brouillard_distance", gxFog, 2 },
    { "ciel_zenith", gxSkyZen, 3 }, { "ciel_milieu", gxSkyMid, 3 }, { "ciel_horizon", gxSkyHor, 3 },
    { "soleil_halo", gxSunGlow, 3 }, { "etoiles", &gxStars, 1 }, { "soleil_taille", gxSunSize, 3 },
    { "ville", gxCity, 5 }, { "ville_couleur", gxCityCol, 3 }, { "ville_fenetres", &gxCityWin, 1 },
    { "bloom", gxBloom, 4 },
    { "texture_echelle", gxTexScale, 3 }, { "sol_texture", gxFloorTex, 2 }, { "mur_texture", gxWallTex, 3 },
    { "grain_force", &gxGrain, 1 }, { "aretes_opacite", &gxEdges, 1 },
    { "trainees", gxTrail, 3 }, { "contours", &gxOutline, 1 }, { "carte_3d", gxMap3d, 3 }, { "hud_fonds", &gxHudPanel, 1 },
    { "neon_cyan", gxNeonC, 3 }, { "neon_magenta", gxNeonM, 3 }, { "neon_orange", gxNeonO, 3 }, { "neon", gxNeon, 3 },
    { "rebond_couleur", &gxBleed, 1 }, { "lumieres_dynamiques", &gxLights, 1 },
    { "nuages", gxClouds, 2 }, { "nuages_couleur", gxCloudCol, 3 }, { "poussiere", &gxDust, 1 }, { "reflets_soleil", &gxFlare, 1 },
    { "pluie", &gxRain, 1 }, { "orage", &gxStorm, 1 },
};
#define GFX_NKEYS ((int)(sizeof gfxKeys / sizeof gfxKeys[0]))
static char gfxDir[512];                                  /* dossier graphismes trouve ("" : aucun, valeurs par defaut) */

/* une ligne "cle = v1 v2 v3" (# commentaire) ; renvoie 1 si la cle est connue */
static int gfx_parse_line(char *line) {
    char *h = strchr(line, '#');
    if (h) *h = 0;
    char *eq = strchr(line, '=');
    if (!eq) return 0;
    *eq = 0;
    char key[64];
    if (sscanf(line, " %63s", key) != 1) return 0;
    for (int k = 0; k < GFX_NKEYS; k++) {
        if (strcmp(key, gfxKeys[k].key)) continue;
        char *p = eq + 1;
        for (int i = 0; i < gfxKeys[k].n; i++) {
            char *end;
            float v = strtof(p, &end);
            if (end == p) break;                          /* moins de valeurs : les suivantes gardent leur valeur */
            gfxKeys[k].v[i] = v;
            p = end;
        }
        return 1;
    }
    return 0;
}

static void gfx_clamp(void) {                             /* valeurs absurdes ramenees dans des bornes jouables */
    gxLmDens = clampf(gxLmDens, 0.5f, 8); gxAoRays = clampf(gxAoRays, 0, 48); gxAoRange = clampf(gxAoRange, 0.5f, 12);
    gxAoMin = clampf(gxAoMin, 0, 1); gxLmBlur = clampf(gxLmBlur, 0, 6);
    gxBloom[1] = clampf(gxBloom[1], 1, 4); gxBloom[2] = clampf(gxBloom[2], 0, 6); gxStars = clampf(gxStars, 0, 2000);
    gxCity[0] = clampf(gxCity[0], 0, 400); gxFog[1] = fmaxf(gxFog[1], gxFog[0] + 1);
    gxClouds[0] = clampf(gxClouds[0], 0, 80); gxDust = clampf(gxDust, 0, 600); gxRain = clampf(gxRain, 0, 2000);
    gxNeon[2] = clampf(gxNeon[2], 0.5f, 12); gxOutline = clampf(gxOutline, 0, 8);
    gxMap3d[0] = clampf(gxMap3d[0], 10, 80); gxMap3d[1] = clampf(gxMap3d[1], 1, 5); gxMap3d[2] = clampf(gxMap3d[2], 15, 89);
    gxHudPanel = clampf(gxHudPanel, 0, 1);
    if (vlen(v3(gxSunDir[0], gxSunDir[1], gxSunDir[2])) < 1e-3f || gxSunDir[1] < 0.05f) gxSunDir[1] = 0.05f;   /* soleil au-dessus */
    int fp = (int)gxFloorTex[0], wx = (int)gxWallTex[0], wy = (int)gxWallTex[1];   /* motifs : diviseurs de 256 */
    if (fp < 8 || 256 % fp) gxFloorTex[0] = 64;
    if (wx < 8 || 256 % wx) gxWallTex[0] = 128;
    if (wy < 8 || 256 % wy) gxWallTex[1] = 64;
}

static int gfx_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[512];
    while (fgets(line, sizeof line, f)) gfx_parse_line(line);
    fclose(f);
    gfx_clamp();
    return 1;
}

/* dossier graphismes : celui de l'utilisateur d'abord, puis a cote du jeu, puis le dossier courant */
static void gfx_find_dir(void) {
    char cand[4][512];
    int n = 0;
#ifdef _WIN32
    GetModuleFileNameA(NULL, cand[n], sizeof cand[n]);
    { char *sl = strrchr(cand[n], '\\'); if (sl) { strcpy(sl + 1, "graphismes"); n++; } }
#else
    const char *home = getenv("HOME");
    if (home) snprintf(cand[n++], sizeof cand[0], "%s/Library/Application Support/Velocity/graphismes", home);
    {
        char exe[512];
        uint32_t len = sizeof exe;
        if (_NSGetExecutablePath(exe, &len) == 0) {      /* .../Velocity.app/Contents/MacOS/velocity */
            char *sl = strrchr(exe, '/');
            if (sl) {
                *sl = 0;
                snprintf(cand[n++], sizeof cand[0], "%s/../../../graphismes", exe);   /* dossier du projet, a cote du .app */
                snprintf(cand[n++], sizeof cand[0], "%s/../Resources/graphismes", exe);
            }
        }
    }
#endif
    snprintf(cand[n++], sizeof cand[0], "graphismes");
    gfxDir[0] = 0;
    for (int i = 0; i < n; i++) {
        char p[600];
        snprintf(p, sizeof p, "%s/rendu.ini", cand[i]);
        FILE *f = fopen(p, "r");
        if (f) { fclose(f); snprintf(gfxDir, sizeof gfxDir, "%s", cand[i]); return; }
    }
}

static float gfxDefaults[160];                            /* valeurs d'origine, pour qu'une cle effacee revienne a son defaut */
static int gfxDefaultsSaved;
static void gfx_defaults(int restore) {
    int o = 0;
    for (int k = 0; k < GFX_NKEYS; k++)
        for (int i = 0; i < gfxKeys[k].n && o < 160; i++, o++) {
            if (restore) gfxKeys[k].v[i] = gfxDefaults[o]; else gfxDefaults[o] = gfxKeys[k].v[i];
        }
    gfxDefaultsSaved = 1;
}

/* qualite : plafonne ce qui coute (precalcul, particules, decor lointain) ; ULTRA garde les valeurs du fichier */
static void gfx_quality_apply(void) {
    if (gfxQuality >= 2) return;
    int low = gfxQuality == 0;
    gxLmDens = fminf(gxLmDens, low ? 1.5f : 2.2f); gxAoRays = fminf(gxAoRays, low ? 4.0f : 8.0f);
    gxLmBlur = fminf(gxLmBlur, 1); gxStars = fminf(gxStars, low ? 100.0f : 200.0f);
    gxClouds[0] = fminf(gxClouds[0], low ? 6.0f : 10.0f); gxDust = fminf(gxDust, low ? 0.0f : 60.0f);
    gxRain = fminf(gxRain, low ? 80.0f : 200.0f); gxCity[0] = fminf(gxCity[0], low ? 25.0f : 45.0f);
    gxBloom[2] = fminf(gxBloom[2], 1);
    if (low) gxFlare = 0;
}

static void gfx_load_all(void) {
    char p[600];
    if (!gfxDefaultsSaved) gfx_defaults(0); else gfx_defaults(1);
    gfx_find_dir();
    if (gfxDir[0]) {
        snprintf(p, sizeof p, "%s/rendu.ini", gfxDir);
        gfx_load(p);
        if (gfxAmbiance > 0 && gfxAmbiance < 4) {         /* l'ambiance remplace une partie des valeurs */
            char nm[32];
            int k = 0;
            for (const char *q = ambianceNames[gfxAmbiance]; *q && k < 31; q++) nm[k++] = (char)(*q - 'A' + 'a');
            nm[k] = 0;
            snprintf(p, sizeof p, "%s/ambiance_%s.ini", gfxDir, nm);
            gfx_load(p);
        }
    }
    gfx_quality_apply();
}

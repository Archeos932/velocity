/* VELOCITY - boutons, menus, parametres, loadout, ecran SKIN
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

static int button(float x, float y, float w, float h, const char *label) {
    int hover = uiMX >= x && uiMX < x + w && uiMY >= y && uiMY < y + h;
    rect(x, y, w, h, hover ? 0.2f : 0.1f, hover ? 0.42f : 0.12f, hover ? 0.62f : 0.2f, 0.9f);
    rect(x, y + h - 3, w, 3, 0.4f, 0.85f, 1.0f, hover ? 1.0f : 0.45f);
    text_c(1, x + w * 0.5f, y + h * 0.5f + fontH[1] * 0.3f, label, 1, 1, 1, 1);
    if (hover && uiClick) { uiClick = 0; play(S_HIT, 0.4f, 1.5f); return 1; }
    return 0;
}

static void field(float x, float y, float w, float h, const char *label, const char *val, int id) {
    int hover = uiMX >= x && uiMX < x + w && uiMY >= y && uiMY < y + h, focus = uiFocus == id;
    char buf[64];
    text(0, x, y - 8, label, 0.7f, 0.75f, 0.85f, 1);
    rect(x, y, w, h, 0.05f, 0.06f, 0.1f, 0.9f);
    rect(x, y + h - 2, w, 2, focus ? 1.0f : 0.4f, focus ? 0.8f : 0.4f, focus ? 0.35f : 0.5f, 1);
    snprintf(buf, sizeof buf, "%s%s", val, (focus && ((GetTickCount() / 400) & 1)) ? "_" : "");
    text(1, x + 10, y + h * 0.5f + fontH[1] * 0.3f, buf, 1, 1, 1, 1);
    if (hover && uiClick) { uiClick = 0; uiFocus = id; }
}

/* aide : les commandes actuelles (reaffectables dans PARAMETRES) et les astuces de jeu */
static void draw_controls(float x, float y) {
    static const char *tips[] = {
        "air-strafe : tourne la souris + gauche/droite en l'air",
        "epee : degats et portee selon la vitesse RELATIVE ; touche = +1 dash",
        "PARRY (debut du coup d'epee) : roquettes, orbes, kamikazes ET epees",
        "piece + tir dedans au rail = ricochet auto (x1,5) ; vole celles des autres",
        "F2/F3 sensibilite   F5 recharger graphismes   F11 plein ecran   ECHAP menu",
    };
    char buf[128];
    text(1, x, y, "COMMANDES", 0.5f, 0.9f, 1.0f, 1);
    for (int a = 0; a < A_COUNT; a++) {
        text(0, x, y + 34 + a * 21.0f, actLabel[a], 0.75f, 0.78f, 0.88f, 1);
        text(0, x + 285, y + 34 + a * 21.0f, bind_text(a), 1, 0.9f, 0.55f, 1);
    }
    float ty = y + 44 + A_COUNT * 21.0f;
    for (int i = 0; i < (int)(sizeof tips / sizeof tips[0]); i++)
        text(0, x, ty + i * 21.0f, tips[i], 0.6f, 0.75f, 0.9f, 1);
    snprintf(buf, sizeof buf, "les commandes se changent dans PARAMETRES");
    text(0, x, ty + 5 * 21.0f + 6, buf, 0.5f, 0.55f, 0.65f, 1);
}

static float slider(int id, float x, float y, float w, const char *label, const char *valText, float v, float lo, float hi) {
    int hover = uiMX >= x - 6 && uiMX < x + w + 6 && uiMY >= y - 4 && uiMY < y + 26;
    text(0, x, y - 8, label, 0.75f, 0.78f, 0.88f, 1);
    text(0, x + w - fontW[0] * (float)strlen(valText), y - 8, valText, 1, 0.9f, 0.55f, 1);
    if (hover && uiClick) { uiClick = 0; uiDrag = id; }
    if (uiDrag == id && uiDown) v = lo + clampf((uiMX - x) / w, 0, 1) * (hi - lo);
    float k = (v - lo) / (hi - lo);
    rect(x, y + 8, w, 6, 0.12f, 0.14f, 0.22f, 0.9f);
    rect(x, y + 8, w * k, 6, 0.4f, 0.85f, 1.0f, 0.9f);
    rect(x + w * k - 6, y + 2, 12, 18, hover || uiDrag == id ? 1.0f : 0.8f, 1, 1, 1);
    return v;
}

static void draw_settings(float cx, float cy) {
    char buf[64];
    rect(0, 0, (float)winW, (float)winH, 0.02f, 0.02f, 0.06f, 0.82f);
    text_c(2, cx, cy - 330, "PARAMETRES", 0.5f, 0.9f, 1.0f, 1);
    float lx = cx - 660, lw = 440, y = cy - 250;

    text(1, lx, y - 20, "REGLAGES", 0.5f, 0.9f, 1.0f, 1);
    y += 30;
    snprintf(buf, sizeof buf, "%.2f", sens * 1000);
    sens = slider(1, lx, y, lw, "Sensibilite de la souris", buf, sens, 0.0003f, 0.008f);
    y += 62;
    snprintf(buf, sizeof buf, "%.0f deg", fovBase);
    fovBase = slider(2, lx, y, lw, "Champ de vision (horizontal)", buf, fovBase, 80, 130);
    y += 62;
    snprintf(buf, sizeof buf, "%.0f %%", masterVol * 100);
    masterVol = slider(3, lx, y, lw, "Volume", buf, masterVol, 0, 1);
    y += 62;
    snprintf(buf, sizeof buf, "%.0f %%", musicVol * 100);
    musicVol = slider(4, lx, y, lw, "Musique", buf, musicVol, 0, 1);
    y += 50;
    snprintf(buf, sizeof buf, "AXE Y : %s", invertY ? "INVERSE" : "NORMAL");
    if (button(lx, y, lw * 0.5f - 5, 44, buf)) invertY = !invertY;
    snprintf(buf, sizeof buf, "FPS : %s", showFps ? "OUI" : "NON");
    if (button(lx + lw * 0.5f + 5, y, lw * 0.5f - 5, 44, buf)) showFps = !showFps;
    y += 50;
    snprintf(buf, sizeof buf, "Impact frames (flashs) : %s", impactFrames ? "OUI" : "NON");
    if (button(lx, y, lw, 44, buf)) impactFrames = !impactFrames;
    y += 50;
    snprintf(buf, sizeof buf, "EFFETS : %s", screenFx ? "OUI" : "NON");
    if (button(lx, y, lw * 0.5f - 5, 44, buf)) screenFx = !screenFx;
    snprintf(buf, sizeof buf, "2e ECRAN:%s", !secondScreen ? "NON" : mapView3d ? "3D" : "HAUT");   /* carte : NON, de haut, 3D */
    if (button(lx + lw * 0.5f + 5, y, lw * 0.5f - 5, 44, buf)) {
        if (!secondScreen) { secondScreen = 1; mapView3d = 0; }
        else if (!mapView3d) mapView3d = 1;
        else secondScreen = 0;
    }
    y += 50;
    snprintf(buf, sizeof buf, "OMBRES : %s", gfxShadows ? "OUI" : "NON");   /* graphismes : deux demi-boutons */
    if (button(lx, y, lw * 0.5f - 5, 44, buf)) gfxShadows = !gfxShadows;
    snprintf(buf, sizeof buf, "BLOOM : %s", gfxBloom ? "OUI" : "NON");
    if (button(lx + lw * 0.5f + 5, y, lw * 0.5f - 5, 44, buf)) gfxBloom = !gfxBloom;
    y += 50;
    snprintf(buf, sizeof buf, "QUALITE:%s", qualityNames[gfxQuality]);   /* preregle : refait l'eclairage du decor */
    if (button(lx, y, lw * 0.5f - 5, 44, buf)) {
        gfxQuality = (gfxQuality + 1) % 3;
        gfxShadows = gfxQuality > 0; gfxBloom = gfxQuality == 2;
        gfx_reload(); popT = 0;
    }
    snprintf(buf, sizeof buf, "CIEL:%s", ambianceNames[gfxAmbiance]);
    if (button(lx + lw * 0.5f + 5, y, lw * 0.5f - 5, 44, buf)) { gfxAmbiance = (gfxAmbiance + 1) % 4; gfx_reload(); popT = 0; }
    y += 50;
    snprintf(buf, sizeof buf, "Plein ecran : %s", fullscreen ? "OUI" : "NON");
    if (button(lx, y, lw, 44, buf)) pendingAction = ACT_FULLSCREEN;
    y += 50;
    snprintf(buf, sizeof buf, "Parties en ligne : %s", onlineOn ? "OUI" : "NON");
    if (button(lx, y, lw, 44, buf)) { onlineOn = !onlineOn; cfg_save(); }
    y += 70;
    if (button(lx, y, 210, 50, "RETOUR")) { settingsOpen = 0; waitAct = -1; cfg_save(); }
    if (button(lx + 230, y, 210, 50, "PAR DEFAUT")) { memcpy(binds, bindDefault, sizeof binds); cfg_save(); }

    float rx = cx - 150, cw = 230, ry = cy - 270;
    text(1, rx, ry, "COMMANDES", 0.5f, 0.9f, 1.0f, 1);
    text(0, rx + 200, ry, waitAct >= 0 ? "appuie sur une touche ou un bouton (ECHAP : annuler, SUPPR : effacer)"
                                       : "clique une case pour la changer", 0.7f, 0.75f, 0.85f, 1);
    for (int a = 0; a < A_COUNT; a++) {
        float yy = ry + 22 + a * 36.0f;
        text(0, rx, yy + 24, actLabel[a], 0.85f, 0.87f, 0.95f, 1);
        for (int k = 0; k < 2; k++) {
            char kn[32];
            float bx = rx + 270 + k * (cw + 12);
            if (waitAct == a && waitSlot == k) snprintf(kn, sizeof kn, "...");
            else key_name(binds[a][k], kn, sizeof kn);
            int hover = uiMX >= bx && uiMX < bx + cw && uiMY >= yy && uiMY < yy + 32;
            int waiting = waitAct == a && waitSlot == k;
            rect(bx, yy, cw, 32, waiting ? 0.45f : hover ? 0.2f : 0.08f, waiting ? 0.35f : hover ? 0.4f : 0.1f,
                 waiting ? 0.1f : hover ? 0.6f : 0.18f, 0.9f);
            text_c(0, bx + cw * 0.5f, yy + 22, kn, 1, 1, 1, 1);
            if (hover && uiClick) { uiClick = 0; waitAct = a; waitSlot = k; }
        }
    }
    if (uiClick) uiClick = 0;
}

static void draw_status(float x, float y) {
    if (netStatus[0] && nowT - netStatusT < 8) text(0, x, y, netStatus, 1, 0.65f, 0.3f, 1);
}

/* bouton de choix : cadre de la couleur de l'arme si elle est choisie, grise si l'arme n'est pas encore codee */
static int sel_button(float x, float y, float w, float h, const char *label, int chosen, int ready, V3 col) {
    int hover = ready && uiMX >= x && uiMX < x + w && uiMY >= y && uiMY < y + h;
    if (chosen) rect(x - 3, y - 3, w + 6, h + 6, col.x, col.y, col.z, 1);
    rect(x, y, w, h, hover ? 0.2f : 0.08f, hover ? 0.42f : 0.1f, hover ? 0.62f : 0.18f, 0.95f);
    float k = ready ? 1.0f : 0.4f;
    text_c(0, x + w * 0.5f, y + h * 0.5f + 5, ready ? label : "BIENTOT", k, k, k, 1);
    if (hover && uiClick) { uiClick = 0; play(S_HIT, 0.4f, 1.5f); return 1; }
    return 0;
}

static void loadout_pick(int slot, int w) {               /* choisir pour un emplacement l'arme de l'autre les echange */
    if (w == wantLoadout[slot ^ 1]) wantLoadout[slot ^ 1] = wantLoadout[slot];
    wantLoadout[slot] = w;
}

static void draw_loadout(float cx, float cy) {
    char buf[128];
    rect(0, 0, (float)winW, (float)winH, 0.02f, 0.02f, 0.06f, 0.85f);
    text_c(2, cx, cy - 300, "LOADOUT", 0.5f, 0.9f, 1.0f, 1);
    text_c(0, cx, cy - 266, "1 arme de melee + 2 armes a distance ; la melee remplit les jauges de munitions",
           0.7f, 0.75f, 0.85f, 1);
    float bw = winW < 1300 ? 118.0f : 150.0f, step = bw + 10, x0 = cx - step * WP_COUNT * 0.5f;
    text(1, x0, cy - 215, "MELEE", 0.5f, 0.9f, 1.0f, 1);
    for (int m = 0; m < MK_COUNT; m++)
        if (sel_button(x0 + m * step, cy - 195, bw, 44, meleeNames[m], m == wantMelee, mkReady[m], v3(1, 0.85f, 0.4f)))
            wantMelee = m;
    for (int s = 0; s < 2; s++) {
        float y = cy - 70 + s * 112.0f;
        text(1, x0, y - 22, s ? "ARME 2" : "ARME 1", 0.5f, 0.9f, 1.0f, 1);
        for (int w = 0; w < WP_COUNT; w++)
            if (sel_button(x0 + w * step, y, bw, 44, ammoDef[w].name, w == wantLoadout[s], wpReady[w],
                           v3(ammoDef[w].r, ammoDef[w].g, ammoDef[w].b)))
                loadout_pick(s, w);
    }
    for (int s = 0; s < 2; s++) {
        const AmmoDef *d = &ammoDef[wantLoadout[s]];
        if (d->gauge <= 0) snprintf(buf, sizeof buf, "%s : objet unique, a recuperer", d->name);
        else snprintf(buf, sizeof buf, "%s : %d au spawn, %d max, %.0f pts par tir (%.1f coups d'epee)", d->name, d->start,
                      d->max, d->gauge, d->gauge / meleePts[MK_SWORD]);
        text(0, x0, cy + 120 + s * 24.0f, buf, 0.8f, 0.82f, 0.9f, 1);
    }
    snprintf(buf, sizeof buf, "%s : %d pts par coup qui touche%s, +%d par kill, +%d par parry ; cellules : %d ou %d pts",
             meleeNames[wantMelee], meleePts[wantMelee], wantMelee == MK_FIST ? " (50 max par combo)" : "",   /* Poings */
             PTS_MELEE_KILL, PTS_PARRY, PICKUP_SMALL, PICKUP_BIG);
    text(0, x0, cy + 168, buf, 1, 0.85f, 0.5f, 1);
    if (appState == APP_GAME) text(0, x0, cy + 194, "pris en compte au prochain respawn", 0.6f, 0.6f, 0.7f, 1);
    if (button(cx - 105, cy + 220, 210, 50, "VALIDER")) { loadoutOpen = 0; cfg_save(); }
    if (uiClick) uiClick = 0;
}

/* ecran de mort : les touches d'arme et de melee font defiler le loadout du prochain respawn */
static void draw_death_loadout(float cx, float y) {
    char buf[160];
    snprintf(buf, sizeof buf, "[%s] < %s >    [%s] < %s >    [%s] < %s >", bind_text(A_WEAP1), ammoDef[wantLoadout[0]].name,
             bind_text(A_WEAP2), ammoDef[wantLoadout[1]].name, bind_text(A_MELEE), meleeNames[wantMelee]);
    text_c(0, cx, y, "LOADOUT DU PROCHAIN RESPAWN", 0.5f, 0.9f, 1.0f, 1);
    text_c(1, cx, y + 32, buf, 1, 0.9f, 0.6f, 1);
}

/* ecran SKIN : 5 apparences, apercu 3D qui tourne ; la couleur reelle sera celle de l'equipe */
static void draw_skin_menu(float cx, float cy) {
    static const char *prevNames[3] = { "BLEU", "ROUGE", "LIBRE" };
    char buf[96];
    rect(0, 0, (float)winW, (float)winH, 0.02f, 0.02f, 0.06f, 0.82f);
    text_c(2, cx, cy - 300, "SKIN", 0.5f, 0.9f, 1.0f, 1);
    text_c(0, cx, cy - 266, "la couleur de ton perso est celle de ton equipe (ou la tienne en chacun pour soi)", 0.7f, 0.75f, 0.85f, 1);
    float bx = cx - 520, y = cy - 220;
    for (int k = 0; k < SKN_COUNT; k++) {
        if (button(bx, y, 320, 54, skinNames[k])) { wantSkin = k; cfg_save(); }
        if (wantSkin == k) {
            rect(bx, y, 320, 3, 1, 0.85f, 0.4f, 1); rect(bx, y + 51, 320, 3, 1, 0.85f, 0.4f, 1);
            rect(bx, y, 3, 54, 1, 0.85f, 0.4f, 1); rect(bx + 317, y, 3, 54, 1, 0.85f, 0.4f, 1);
        }
        y += 64;
    }
    y += 16;
    text(0, bx, y, "APERCU DE LA COULEUR", 0.7f, 0.75f, 0.85f, 1);
    y += 14;
    for (int k = 0; k < 3; k++) if (button(bx + k * 108, y, 100, 44, prevNames[k])) skinPrevTeam = k;
    y += 70;
    if (button(bx, y, 320, 52, "VALIDER")) { skinOpen = 0; cfg_save(); }
    float px = cx - 140, py = cy - 230, pw = 640, ph = 520;   /* apercu 3D */
    rect(px, py, pw, ph, 0.05f, 0.06f, 0.1f, 0.7f);
    rect(px, py + ph - 3, pw, 3, 0.5f, 0.9f, 1.0f, 0.5f);
    {
        static const float freeCol[3] = { 1.0f, 0.85f, 0.3f };
        const float *c = skinPrevTeam < 2 ? teamCol[skinPrevTeam] : freeCol;
        SkinAnim an;
        an.spd = 0.6f; an.run = (float)nowT * 7; an.air = 0; an.slide = 0; an.pitch = 0; an.t = (float)nowT;
        glViewport((int)px, winH - (int)(py + ph), (int)pw, (int)ph);
        set_frustum(32, pw / ph, 0.1f, 50);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND);
        set_dyn_light(1);
        glTranslatef(0, -1.0f, -4.6f);
        glRotatef(-10, 1, 0, 0);
        glRotatef(180 + (float)nowT * 40, 0, 1, 0);      /* de face au depart */
        glEnable(GL_LIGHTING);
        draw_skin(wantSkin, H_STAND, c, &an);
        glDisable(GL_LIGHTING);
        glViewport(0, 0, winW, winH);                     /* retour au HUD */
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, winW, winH, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    snprintf(buf, sizeof buf, "%s", skinNames[wantSkin]);
    text_c(1, px + pw * 0.5f, py + ph - 20, buf, 1, 0.9f, 0.6f, 1);
    if (uiClick) uiClick = 0;
}

/* ecran HEBERGER UNE PARTIE : nombre de joueurs (chacun pour soi ou equipes 1v1 a 4v4), bots pour completer */
static void draw_host_menu(float cx, float cy) {
    static const char *modeNames[TEAM_MAX + 1] = { "LIBRE", "1V1", "2V2", "3V3", "4V4" };
    static const char *botNames[4] = { "AUCUN", "FACILE", "NORMAL", "DIFFICILE" };
    char buf[128];
    rect(0, 0, (float)winW, (float)winH, 0.02f, 0.02f, 0.06f, 0.78f);
    text_c(2, cx, cy - 300, "HEBERGER UNE PARTIE", 0.5f, 0.9f, 1.0f, 1);
    float bw = 620, bx = cx - bw * 0.5f, y = cy - 210;
    rect(bx - 30, y - 40, bw + 60, 470, 0.03f, 0.04f, 0.08f, 0.6f);
    rect(bx - 30, y - 40, bw + 60, 2, 0.5f, 0.9f, 1.0f, 0.5f);
    text(1, bx, y, "NOMBRE DE JOUEURS", 0.5f, 0.9f, 1.0f, 1);
    y += 18;
    for (int m = 0; m <= TEAM_MAX; m++) {                 /* chacun pour soi, puis 1v1 a 4v4 */
        float mx = bx + m * 126.0f;
        if (button(mx, y, 116, 50, modeNames[m])) wantTeams = m;
        if (wantTeams == m) {                             /* mode choisi : cadre lumineux */
            rect(mx, y, 116, 3, 1, 0.85f, 0.4f, 1); rect(mx, y + 47, 116, 3, 1, 0.85f, 0.4f, 1);
            rect(mx, y, 3, 50, 1, 0.85f, 0.4f, 1); rect(mx + 113, y, 3, 50, 1, 0.85f, 0.4f, 1);
        }
    }
    y += 82;
    if (wantTeams > 0)
        snprintf(buf, sizeof buf, "2 equipes de %d  -  %d joueurs max  -  premiere equipe a %d frags", wantTeams, wantTeams * 2,
                 team_frag_limit(wantTeams));
    else snprintf(buf, sizeof buf, "chacun pour soi  -  %d joueurs max  -  premier a %d frags", MAXP, FRAG_LIMIT);
    text(0, bx, y, buf, 0.85f, 0.85f, 0.95f, 1);
    y += 50;
    text(1, bx, y, "COMPLETER AVEC DES BOTS", 0.5f, 0.9f, 1.0f, 1);
    y += 18;
    for (int k = 0; k < 4; k++) {                         /* -1 = aucun bot, sinon leur niveau */
        float mx = bx + k * 157.0f;
        if (button(mx, y, 147, 50, botNames[k])) hostBots = k - 1;
        if (hostBots == k - 1) {
            rect(mx, y, 147, 3, 1, 0.85f, 0.4f, 1); rect(mx, y + 47, 147, 3, 1, 0.85f, 0.4f, 1);
            rect(mx, y, 3, 50, 1, 0.85f, 0.4f, 1); rect(mx + 144, y, 3, 50, 1, 0.85f, 0.4f, 1);
        }
    }
    y += 82;
    if (hostBots < 0) snprintf(buf, sizeof buf, "les autres joueurs te trouvent en LAN ou en ligne");
    else if (wantTeams > 0)
        snprintf(buf, sizeof buf, "%d bot%s : toi + %d allie%s contre %d", wantTeams * 2 - 1, wantTeams * 2 - 1 > 1 ? "s" : "",
                 wantTeams - 1, wantTeams > 2 ? "s" : "", wantTeams);
    else snprintf(buf, sizeof buf, "1 bot, la partie reste ouverte en LAN");
    text(0, bx, y, buf, 0.85f, 0.85f, 0.95f, 1);
    y += 46;
    if (button(bx, y, bw, 56, "LANCER LA PARTIE")) {
        hostOpen = 0;
        if (hostBots < 0) pendingAction = ACT_HOST;
        else { pendingAction = ACT_VSBOT; pendingArg = hostBots; }
    }
    y += 70;
    if (button(bx, y, 240, 48, "RETOUR")) { hostOpen = 0; cfg_save(); }
    draw_status(bx, y + 90);
    if (uiClick) uiClick = 0;
}

static void draw_main_menu(float cx, float cy) {
    /* assombri surtout en haut (lisibilite du titre) ; vers le bas, le decor (sol, arene) reste visible */
    rect_vgrad(0, 0, (float)winW, (float)winH, 0.02f, 0.02f, 0.06f, 0.78f, 0.30f);
    for (int k = 0; k < 7; k++) {                             /* halo doux derriere le titre (passes decalees, faux flou) */
        float t = (float)nowT;                                /* gameTime est gele hors partie : horloge murale */
        float a = k * (2 * PI / 7) + t * 0.3f, rr = 3.5f + 1.5f * sinf(t * 0.7f + k);
        text_c(2, cx + cosf(a) * rr, cy - 300 + sinf(a) * rr, "V E L O C I T Y", 0.4f, 0.85f, 1.0f, 0.05f);
    }
    text_c(2, cx, cy - 300, "V E L O C I T Y", 0.5f, 0.9f, 1.0f, 1);
    text_c(0, cx, cy - 268, "movement shooter en C", 0.7f, 0.7f, 0.8f, 1);
    rect(cx - 220, cy - 252, 440, 2, 0.5f, 0.9f, 1.0f, 0.5f);   /* trait d'accent sous le sous-titre */
    float bx = cx - 620, bw = 420, y = cy - 205;
    rect(bx - 30, y - 30, bw + 60, 556, 0.03f, 0.04f, 0.08f, 0.55f);        /* panneau derriere les boutons */
    rect(bx - 30, y - 30, bw + 60, 2, 0.5f, 0.9f, 1.0f, 0.5f);
    rect(bx - 30, y + 524, bw + 60, 2, 0.5f, 0.9f, 1.0f, 0.25f);
    field(bx, y, bw, 48, "PSEUDO", myName, 1);
    y += 76;
    if (button(bx, y, bw, 52, "SOLO  -  VAGUES")) pendingAction = ACT_SOLO;
    y += 66;
    text(0, bx, y + 2, "JOUEUR CONTRE BOT  (duel)", 0.7f, 0.75f, 0.85f, 1);
    y += 14;
    for (int lv = 0; lv < 3; lv++)                        /* un duel contre un bot : trois niveaux */
        if (button(bx + lv * 142, y, 136, 46, botLevelNames[lv])) { pendingAction = ACT_VSBOT; pendingArg = lv; hostFfa = 1; }
    y += 54;
    if (button(bx, y, bw, 46, "PARTIE RAPIDE  -  EN LIGNE")) pendingAction = ACT_QUICK;
    y += 54;
    if (button(bx, y, bw, 46, "HEBERGER UNE PARTIE")) { hostOpen = 1; uiFocus = 0; }
    y += 54;
    if (button(bx, y, bw, 46, "REJOINDRE / REGARDER")) pendingAction = ACT_BROWSE;
    y += 54;
    if (button(bx, y, bw * 0.5f - 5, 46, "LOADOUT")) loadoutOpen = 1;
    if (button(bx + bw * 0.5f + 5, y, bw * 0.5f - 5, 46, "SKIN")) skinOpen = 1;
    y += 54;
    if (button(bx, y, bw, 46, "PARAMETRES")) settingsOpen = 1;
    y += 54;
    if (button(bx, y, bw, 46, "QUITTER")) pendingAction = ACT_QUIT;
    draw_status(bx, y + 72);
    draw_controls(cx - 130, cy - 200);
    if (uiClick) { uiClick = 0; uiFocus = 0; }
}

static void draw_join_menu(float cx, float cy) {
    char buf[128];
    rect(0, 0, (float)winW, (float)winH, 0.02f, 0.02f, 0.06f, 0.7f);
    text_c(2, cx, cy - 300, "REJOINDRE / REGARDER UNE PARTIE", 0.5f, 0.9f, 1.0f, 1);
    float bx = cx - 420, bw = 840, y = cy - 250;
    snprintf(buf, sizeof buf, "%s   |   %s", joining ? "connexion en cours..." : "parties du reseau local et en ligne",
             lb_status_text());
    text(0, bx, y, buf, 0.7f, 0.75f, 0.85f, 1);
    y += 14;
    int shown = 0;
    for (int i = 0; i < nFound && shown < 7; i++) {
        int dup = 0;                                      /* meme partie vue en LAN et en ligne : on garde la LAN */
        if (found[i].online)
            for (int j = 0; j < nFound; j++) if (!found[j].online && !strcmp(found[j].name, found[i].name)) dup = 1;
        if (dup) continue;
        if (found[i].online)
            snprintf(buf, sizeof buf, "[EN LIGNE]  %s    %d/%d joueurs%s", found[i].name, found[i].players, found[i].maxp,
                     found[i].spec ? "  (spectateurs)" : "");
        else {
            char mode[16];
            if (found[i].teams > 0) snprintf(mode, sizeof mode, "%dV%d", found[i].teams, found[i].teams); else snprintf(mode, sizeof mode, "LIBRE");
            snprintf(buf, sizeof buf, "[LAN %s]  %s    %d/%d joueurs    %s", mode, found[i].name, found[i].players, found[i].maxp,
                     inet_ntoa(found[i].addr.sin_addr));
        }
        if (button(bx, y, bw - 170, 46, buf)) { pendingAction = ACT_JOIN_FOUND; pendingArg = i; }
        if (button(bx + bw - 160, y, 160, 46, "REGARDER")) { pendingAction = ACT_SPEC_FOUND; pendingArg = i; }
        y += 54;
        shown++;
    }
    if (!shown) {
        text(0, bx, y + 30, "aucune partie pour l'instant - lance une PARTIE RAPIDE ou heberge la tienne", 0.6f, 0.6f, 0.7f, 1);
        y += 54;
    }
    y += 24;
    if (button(bx, y, 300, 46, "PARTIE RAPIDE")) pendingAction = ACT_QUICK;
    y += 60;
    field(bx, y, 520, 48, "OU TAPER L'IP DE L'HOTE", ipField, 2);
    if (button(bx + 540, y, 300, 48, "REJOINDRE")) pendingAction = ACT_JOIN_IP;
    draw_status(bx, y + 90);
    if (button(bx, y + 130, 300, 50, "RETOUR")) pendingAction = ACT_BACK;
    if (uiClick) { uiClick = 0; uiFocus = 0; }
}

static void draw_pause(float cx, float cy) {
    char buf[96];
    rect(0, 0, (float)winW, (float)winH, 0.02f, 0.02f, 0.06f, 0.72f);
    text_c(2, cx, cy - 300, dmMode ? "MENU" : "PAUSE", 0.5f, 0.9f, 1.0f, 1);
    if (dmMode) text_c(0, cx, cy - 268, "la partie continue pendant que tu es ici", 0.8f, 0.6f, 0.4f, 1);
    float bx = cx - 620, bw = 420, y = cy - 200;
    if (netMode == NET_HOST) {
        text(0, bx, y, "TU HEBERGES - les autres te trouvent tout seuls,", 0.7f, 0.75f, 0.85f, 1);
        text(0, bx, y + 22, "ou peuvent taper ton IP :", 0.7f, 0.75f, 0.85f, 1);
        text(1, bx, y + 56, hostIpText, 1, 0.85f, 0.4f, 1);
        snprintf(buf, sizeof buf, "%s   |   %d spectateur%s", lb_status_text(), spec_count(), spec_count() > 1 ? "s" : "");
        text(0, bx, y + 84, buf, 0.5f, 0.85f, 0.6f, 1);
        y += 112;
    }
    if (button(bx, y, bw, 54, "REPRENDRE")) pendingAction = ACT_RESUME;
    y += 68;
    if (button(bx, y, bw, 54, "LOADOUT")) loadoutOpen = 1;
    y += 68;
    if (!dmMode) {
        if (button(bx, y, bw, 54, "RECOMMENCER")) pendingAction = ACT_RESTART;
        y += 68;
    }
    if (button(bx, y, bw, 54, "PARAMETRES")) settingsOpen = 1;
    y += 68;
    if (button(bx, y, bw, 54, dmMode ? "QUITTER LA PARTIE" : "MENU PRINCIPAL")) pendingAction = ACT_LEAVE;
    draw_controls(cx - 130, cy - 200);
    if (uiClick) { uiClick = 0; pendingAction = ACT_RESUME; }  /* clic ailleurs = reprendre */
}

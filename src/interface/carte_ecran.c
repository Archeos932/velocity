/* VELOCITY - second ecran : carte vue de dessus en direct + statistiques de la partie
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* Dessine tout l'ecran de carte dans le contexte OpenGL courant (w x h pixels). Ne depend d'aucun etat laisse par
   render() : le second ecran a son propre contexte (textures et listes partagees avec le principal). La fenetre
   elle-meme est geree par plateforme/second_ecran.c ; --shot carte.bmp dessine cet ecran dans la fenetre du jeu. */

#define MAPS_TRAIL 28                                     /* points de trainee par joueur (un tous les MAPS_TRAIL_DT) */
#define MAPS_TRAIL_DT 0.07
static V3 mapsTrail[MAXP + 1][MAPS_TRAIL];                /* MAXP : le joueur local en solo */
static int mapsTrailN[MAXP + 1], mapsTrailHead[MAXP + 1];
static double mapsTrailT;
static float mapsMaxSpeed;                                /* vitesse max atteinte pendant la partie en cours */
static int shotMap;                                       /* --shot carte*.bmp : render() dessine cet ecran a la place du jeu */

typedef struct { float x0, y0, k; } MapView;              /* monde (x, z) -> ecran : x0 + (x - mn) * k */
static float mapsMnX, mapsMnZ;

static void maps_pt(const MapView *v, V3 p, float *sx, float *sy) {
    *sx = v->x0 + (p.x - mapsMnX) * v->k;
    *sy = v->y0 + (p.z - mapsMnZ) * v->k;
}

/* La carte ne doit pas servir de radar contre l'adversaire : pendant une partie on n'y voit que soi et ses equipiers
   (en chacun pour soi : soi seul). Tout le monde apparait en spectateur et une fois la partie finie. */
static int maps_sees(int i) {
    if (!dmMode || spectating || matchOver) return 1;
    return i == myId || same_team(i, myId);
}

/* un joueur affiche : position, cap, vivant, couleur, nom, PV */
typedef struct { V3 pos; float yaw, hp; int alive, self, id; const float *col; const char *name; } MapsPl;

static int maps_players(MapsPl *out) {
    static const float selfCol[3] = { 1, 1, 1 };
    int n = 0;
    if (!dmMode) {                                        /* solo : moi seulement */
        out[n].pos = P.pos; out[n].yaw = P.yaw; out[n].hp = P.hp; out[n].alive = !P.dead; out[n].self = 1;
        out[n].id = MAXP; out[n].col = selfCol; out[n].name = myName; n++;
        return n;
    }
    for (int i = 0; i < MAXP; i++) {
        if (!NP[i].active || !maps_sees(i)) continue;
        int me = i == myId && !spectating;
        out[n].pos = me ? P.pos : NP[i].disp; out[n].yaw = me ? P.yaw : NP[i].st.yaw;
        out[n].hp = me ? P.hp : NP[i].hp; out[n].alive = me ? !P.dead : NP[i].alive; out[n].self = me;
        out[n].id = i; out[n].col = pcolor(i); out[n].name = NP[i].name; n++;
    }
    return n;
}

static void maps_trail_tick(const MapsPl *pl, int n) {
    if (nowT - mapsTrailT < MAPS_TRAIL_DT && nowT >= mapsTrailT) return;
    mapsTrailT = nowT;
    for (int k = 0; k < n; k++) {
        int id = pl[k].id;
        if (!pl[k].alive) { mapsTrailN[id] = 0; continue; }
        int last = (mapsTrailHead[id] + MAPS_TRAIL - 1) % MAPS_TRAIL;
        if (mapsTrailN[id] && vlen(vsub(mapsTrail[id][last], pl[k].pos)) > 12) mapsTrailN[id] = 0;   /* respawn : pas de trait */
        mapsTrail[id][mapsTrailHead[id]] = pl[k].pos;
        mapsTrailHead[id] = (mapsTrailHead[id] + 1) % MAPS_TRAIL;
        if (mapsTrailN[id] < MAPS_TRAIL) mapsTrailN[id]++;
    }
}

static void maps_disc(float x, float y, float r, int seg) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int s = 0; s <= seg; s++) glVertex2f(x + r * cosf(s * 2 * PI / seg), y + r * sinf(s * 2 * PI / seg));
    glEnd();
}

static void maps_ring(float x, float y, float r, int seg) {
    glBegin(GL_LINE_LOOP);
    for (int s = 0; s < seg; s++) glVertex2f(x + r * cosf(s * 2 * PI / seg), y + r * sinf(s * 2 * PI / seg));
    glEnd();
}

static void maps_draw_map(const MapView *v, float S, const MapsPl *pl, int n) {
    float sx, sy, ex, ey;
    rect(v->x0 - 10, v->y0 - 10, S + 20, S + 20, 0.02f, 0.025f, 0.06f, 1);   /* cadre */
    rect(v->x0, v->y0, S, S, 0.07f, 0.08f, 0.13f, 1);                          /* sol */
    glLineWidth(1);
    glColor4f(0.3f, 0.4f, 0.6f, 0.12f);                   /* quadrillage tous les 10 m */
    glBegin(GL_LINES);
    for (float g = ceilf(mapsMnX / 10) * 10; (g - mapsMnX) * v->k <= S; g += 10) {
        glVertex2f(v->x0 + (g - mapsMnX) * v->k, v->y0); glVertex2f(v->x0 + (g - mapsMnX) * v->k, v->y0 + S);
    }
    for (float g = ceilf(mapsMnZ / 10) * 10; (g - mapsMnZ) * v->k <= S; g += 10) {
        glVertex2f(v->x0, v->y0 + (g - mapsMnZ) * v->k); glVertex2f(v->x0 + S, v->y0 + (g - mapsMnZ) * v->k);
    }
    glEnd();

    int ord[MAXB], no = 0;                                /* blocs, du plus bas au plus haut : le haut recouvre le bas */
    for (int i = 1; i < nb; i++) ord[no++] = i;
    for (int a = 1; a < no; a++)
        for (int b = a; b > 0 && boxes[ord[b]].mx.y < boxes[ord[b - 1]].mx.y; b--) { int t = ord[b]; ord[b] = ord[b - 1]; ord[b - 1] = t; }
    for (int o = 0; o < no; o++) {
        const Box *b = &boxes[ord[o]];
        maps_pt(v, b->mn, &sx, &sy); maps_pt(v, b->mx, &ex, &ey);
        float w = ex - sx, h = ey - sy;
        if (b->kind == B_PAD) {                           /* jump pad : disque cyan qui pulse */
            float pl2 = 0.6f + 0.4f * sinf((float)nowT * 5 + b->mn.x);
            glColor4f(0.2f, 0.95f, 1.0f, 0.25f + 0.35f * pl2);
            maps_disc(sx + w * 0.5f, sy + h * 0.5f, fmaxf(w, h) * 0.5f + 4, 20);
            glColor4f(0.6f, 1, 1, 0.9f);
            maps_ring(sx + w * 0.5f, sy + h * 0.5f, fmaxf(w, h) * 0.5f + 4 + 3 * pl2, 20);
            continue;
        }
        float hk = clampf(b->mx.y / 16, 0, 1), sh = 0.45f + 0.55f * hk;   /* plus c'est haut, plus c'est clair */
        if (b->mn.y > 0.5f) rect(sx + 5, sy + 5, w, h, 0, 0, 0, 0.35f);   /* plateforme flottante : ombre portee */
        rect(sx, sy, w, h, b->col.x * sh, b->col.y * sh, b->col.z * sh, 1);
        glColor4f(0.75f + 0.25f * hk, 0.85f + 0.15f * hk, 1, 0.35f + 0.3f * hk);
        glBegin(GL_LINE_LOOP);
        glVertex2f(sx, sy); glVertex2f(ex, sy); glVertex2f(ex, ey); glVertex2f(sx, ey);
        glEnd();
    }

    for (int i = 0; i < npu; i++) {                       /* recharges : losange dore si disponible */
        maps_pt(v, PU[i].pos, &sx, &sy);
        float a = PU[i].t > 0 ? 0.2f : 0.95f, r = PU[i].pts >= PICKUP_BIG ? 8.0f : 6.0f;
        glColor4f(1, 0.82f, 0.3f, a);
        glBegin(GL_QUADS);
        glVertex2f(sx, sy - r); glVertex2f(sx + r, sy); glVertex2f(sx, sy + r); glVertex2f(sx - r, sy);
        glEnd();
    }
    for (int i = 0; i < MAXCOIN; i++)                     /* pieces en vol */
        if (CO[i].alive && maps_sees(CO[i].owner)) { maps_pt(v, CO[i].pos, &sx, &sy); glColor4f(1, 0.9f, 0.4f, 1); maps_disc(sx, sy, 3, 8); }
    for (int i = 0; i < MAXPR; i++) {                     /* projectiles : roquettes oranges, le reste blanc */
        if (!PR[i].alive || !maps_sees(PR[i].owner)) continue;
        maps_pt(v, PR[i].pos, &sx, &sy);
        if (PR[i].kind == PR_ROCKET) glColor4f(1, 0.55f, 0.15f, 1); else glColor4f(0.9f, 0.9f, 1, 0.9f);
        maps_disc(sx, sy, PR[i].kind == PR_ROCKET ? 4.5f : 3.5f, 10);
    }
    if (!dmMode)
        for (int i = 0; i < MAXE; i++) {                  /* solo : ennemis en carres rouges */
            if (!E[i].alive) continue;
            maps_pt(v, E[i].pos, &sx, &sy);
            rect(sx - 5, sy - 5, 10, 10, 1, 0.2f + 0.6f * clampf(E[i].flash, 0, 1), 0.25f, 0.95f);
        }

    glLineWidth(3);                                       /* trainees : 2 s de trajet, a la couleur du joueur */
    for (int k = 0; k < n; k++) {
        int id = pl[k].id, cnt = mapsTrailN[id];
        if (cnt < 2) continue;
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j < cnt; j++) {
            V3 p = mapsTrail[id][(mapsTrailHead[id] - cnt + j + 2 * MAPS_TRAIL) % MAPS_TRAIL];
            maps_pt(v, p, &sx, &sy);
            glColor4f(pl[k].col[0], pl[k].col[1], pl[k].col[2], 0.55f * j / cnt);
            glVertex2f(sx, sy);
        }
        glEnd();
    }
    glLineWidth(1);

    for (int k = 0; k < n; k++) {                         /* joueurs : fleche orientee, plus grosse en hauteur */
        const MapsPl *p = &pl[k];
        maps_pt(v, p->pos, &sx, &sy);
        const float *c = p->col;
        if (!p->alive) {                                  /* mort : croix */
            glLineWidth(3); glColor4f(c[0], c[1], c[2], 0.5f);
            glBegin(GL_LINES);
            glVertex2f(sx - 7, sy - 7); glVertex2f(sx + 7, sy + 7); glVertex2f(sx + 7, sy - 7); glVertex2f(sx - 7, sy + 7);
            glEnd();
            glLineWidth(1);
            continue;
        }
        float s1 = sinf(p->yaw), c1 = -cosf(p->yaw), sz = 11 * (1 + clampf(p->pos.y / 20, 0, 0.8f));
        if (p->self) {                                    /* moi : cone de vision */
            float half = 0.6f, L = 70;
            glColor4f(1, 1, 1, 0.10f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(sx, sy);
            for (int s = 0; s <= 8; s++) {
                float a = p->yaw - half + 2 * half * s / 8;
                glVertex2f(sx + sinf(a) * L, sy - cosf(a) * L);
            }
            glEnd();
        }
        glColor4f(0, 0, 0, 0.6f);
        maps_disc(sx, sy, sz * 0.9f, 16);
        glColor4f(c[0], c[1], c[2], 1);
        glBegin(GL_TRIANGLES);
        glVertex2f(sx + s1 * sz, sy + c1 * sz);
        glVertex2f(sx - s1 * sz * 0.6f + c1 * sz * 0.65f, sy - c1 * sz * 0.6f - s1 * sz * 0.65f);
        glVertex2f(sx - s1 * sz * 0.6f - c1 * sz * 0.65f, sy - c1 * sz * 0.6f + s1 * sz * 0.65f);
        glEnd();
        if (p->self) { glLineWidth(2); glColor4f(1, 1, 1, 0.9f); maps_ring(sx, sy, sz + 4, 20); glLineWidth(1); }
        float tw = fontW[0] * (float)strlen(p->name);
        text(0, sx - tw * 0.5f, sy - sz - 26, p->name, c[0] * 0.5f + 0.5f, c[1] * 0.5f + 0.5f, c[2] * 0.5f + 0.5f, 1);
        rect(sx - 16, sy + sz + 4, 32, 4, 0.1f, 0.1f, 0.15f, 0.8f);
        float hk = clampf(p->hp / 100, 0, 1);
        rect(sx - 16, sy + sz + 4, 32 * hk, 4, 1 - hk * 0.7f, 0.25f + hk * 0.6f, 0.35f, 1);
    }
}

static float maps_title(float x, float y, const char *s) {   /* intitule de rubrique, renvoie la ligne suivante */
    text(0, x, y, s, 0.5f, 0.75f, 1, 0.9f);
    rect(x, y + 8, 60, 2, 0.5f, 0.75f, 1, 0.6f);
    return y + 40;
}

static void maps_draw_panel(float x, float y, float w, float h) {
    char buf[96];
    float yMax = y + h;
    rect(x - 20, y - 20, w + 40, h + 40, 0.03f, 0.035f, 0.07f, 1);
    text(2, x, y + 44, "VELOCITY", 1, 1, 1, 1);
    y += 90;
    if (dmMode) {                                         /* mode, temps, limite */
        if (teamSize > 0) snprintf(buf, sizeof buf, "EQUIPES %dv%d  -  %d frags", teamSize, teamSize, team_frag_limit(teamSize));
        else snprintf(buf, sizeof buf, "DEATHMATCH  -  %d frags", fragLimit);
        text(1, x, y, buf, 0.85f, 0.88f, 0.95f, 1);
        int left = (int)fmaxf(0, MATCH_TIME - matchT);
        snprintf(buf, sizeof buf, "%d:%02d", left / 60, left % 60);
        text(2, x + w - fontW[2] * (float)strlen(buf), y - 46, buf, left < 60 ? 1.0f : 0.9f, left < 60 ? 0.45f : 0.9f, 0.4f, 1);   /* sur la ligne du titre */
        y += 50;
        if (matchOver) {
            winner_text(buf, (int)sizeof buf);
            text(1, x, y, buf, 1, 0.85f, 0.3f, 1);
            y += 40;
        }
        if (teamSize > 0) {                               /* scores d'equipe en grand */
            for (int t = 0; t < 2; t++) {
                const float *c = teamCol[t];
                float bx = x + t * (w * 0.5f + 6), bw = w * 0.5f - 6;
                rect(bx, y, bw, 70, c[0] * 0.25f, c[1] * 0.25f, c[2] * 0.25f, 1);
                rect(bx, y, bw * clampf(team_score(t) / (float)team_frag_limit(teamSize), 0, 1), 6, c[0], c[1], c[2], 1);
                snprintf(buf, sizeof buf, "%s  %d", teamNames[t], team_score(t));
                text(1, bx + 14, y + 48, buf, c[0], c[1], c[2], 1);
            }
            y += 96;
        }
        y = maps_title(x, y, "CLASSEMENT");
        int order[MAXP], n = 0;
        for (int i = 0; i < MAXP; i++) if (NP[i].active) order[n++] = i;
        for (int a = 1; a < n; a++)
            for (int b = a; b > 0 && NP[order[b]].kills > NP[order[b - 1]].kills; b--) { int t = order[b]; order[b] = order[b - 1]; order[b - 1] = t; }
        text(0, x + w * 0.52f, y, "FRAGS", 0.6f, 0.65f, 0.75f, 1);
        text(0, x + w * 0.68f, y, "MORTS", 0.6f, 0.65f, 0.75f, 1);
        text(0, x + w * 0.84f, y, "RATIO", 0.6f, 0.65f, 0.75f, 1);
        y += 34;
        int lim = teamSize > 0 ? team_frag_limit(teamSize) : fragLimit;
        for (int k = 0; k < n && y < yMax - 260; k++) {   /* garder la place pour MOI */
            int i = order[k];
            const float *c = pcolor(i);
            float al = NP[i].alive ? 1.0f : 0.5f;
            if (i == myId) rect(x - 8, y - 26, w + 16, 36, 1, 1, 1, 0.07f);
            rect(x - 8, y + 6, (w + 16) * clampf(NP[i].kills / (float)(lim > 0 ? lim : 1), 0, 1), 3, c[0], c[1], c[2], 0.7f);
            snprintf(buf, sizeof buf, "%d. %s", k + 1, NP[i].name);
            text(1, x, y, buf, c[0], c[1], c[2], al);
            snprintf(buf, sizeof buf, "%d", NP[i].kills);  text(1, x + w * 0.52f, y, buf, 1, 1, 1, al);
            snprintf(buf, sizeof buf, "%d", NP[i].deaths); text(1, x + w * 0.68f, y, buf, 0.8f, 0.8f, 0.85f, al);
            snprintf(buf, sizeof buf, "%.1f", NP[i].kills / (float)(NP[i].deaths > 0 ? NP[i].deaths : 1));
            text(1, x + w * 0.84f, y, buf, 0.8f, 0.8f, 0.85f, al);
            y += 40;
        }
        y += 16;
    } else {                                              /* solo : vague, score */
        snprintf(buf, sizeof buf, "SOLO  -  VAGUE %d", wave);
        text(1, x, y, buf, 0.85f, 0.88f, 0.95f, 1);
        y += 50;
        y = maps_title(x, y, "PARTIE");
        int alive = 0;
        for (int i = 0; i < MAXE; i++) alive += E[i].alive;
        snprintf(buf, sizeof buf, "score %d   (record %d)", score, bestScore); text(1, x, y, buf, 1, 1, 1, 1); y += 40;
        snprintf(buf, sizeof buf, "kills %d   ennemis %d", kills, alive); text(1, x, y, buf, 0.85f, 0.85f, 0.9f, 1); y += 56;
    }

    if (!spectating) {                                    /* mes chiffres */
        y = maps_title(x, y, "MOI");
        float sp = hlen(P.vel);
        snprintf(buf, sizeof buf, "%.1f m/s", sp);
        text(2, x, y + 16, buf, 0.6f, 0.95f, 1, 1);
        y += 44;
        snprintf(buf, sizeof buf, "max %.1f m/s", mapsMaxSpeed);
        text(0, x, y, buf, 0.75f, 0.8f, 0.9f, 1);
        snprintf(buf, sizeof buf, "chaine x%.2f", streak_mult());
        text(0, x + w - fontW[0] * (float)strlen(buf), y, buf, 1, 0.85f, 0.4f, 1);
        y += 18;
        rect(x, y, w, 6, 0.1f, 0.1f, 0.15f, 1);
        rect(x, y, w * clampf(sp / 40, 0, 1), 6, 0.4f, 0.9f, 1, 1);
        y += 40;
        const AmmoDef *ad = &ammoDef[P.weapon];
        snprintf(buf, sizeof buf, "%s  %d/%d", ad->name, P.ammo[P.weapon], ad->max);
        text(1, x, y, buf, ad->r, ad->g, ad->b, 1);
        snprintf(buf, sizeof buf, "%d PV", (int)ceilf(P.hp));
        text(1, x + w - fontW[1] * (float)strlen(buf), y, buf, P.hp < 35 ? 1.0f : 0.8f, P.hp < 35 ? 0.35f : 1.0f, 0.5f, 1);
        y += 56;
    }

    if (dmMode) {                                         /* derniers evenements */
        y = maps_title(x, y, "DERNIERS KILLS");
        for (int k = 0; k < 6; k++) {
            KillFeed *f = &kfeed[k];
            if (f->t <= 0) continue;
            if (f->info) snprintf(buf, sizeof buf, "%s %s", pname(f->victim), f->info == 1 ? "a rejoint" : "a quitte");
            else if (f->killer == f->victim) snprintf(buf, sizeof buf, "%s s'est fait sauter", pname(f->victim));
            else snprintf(buf, sizeof buf, "%s  [%s]  %s", pname(f->killer), weaponNames[f->kind % W_COUNT], pname(f->victim));
            const float *c = f->killer >= 0 && f->killer < MAXP && !f->info ? pcolor(f->killer) : NULL;
            text(0, x, y, buf, c ? c[0] * 0.5f + 0.5f : 0.85f, c ? c[1] * 0.5f + 0.5f : 0.85f, c ? c[2] * 0.5f + 0.5f : 0.85f,
                 clampf(f->t, 0.3f, 1));
            y += 28;
        }
    }
}

static void draw_map_screen(int w, int h) {
    glViewport(0, 0, w, h);
    glClearColor(0.015f, 0.018f, 0.04f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_CULL_FACE); glDisable(GL_TEXTURE_2D); glDisable(GL_FOG);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (appState != APP_GAME || nb < 1) {                 /* hors partie : ecran d'attente */
        mapsMaxSpeed = 0;
        for (int i = 0; i <= MAXP; i++) mapsTrailN[i] = 0;
        text_c(2, w * 0.5f, h * 0.5f, "VELOCITY", 1, 1, 1, 1);
        text_c(0, w * 0.5f, h * 0.5f + 50, "la carte de la partie s'affichera ici", 0.6f, 0.7f, 0.85f, 0.6f + 0.4f * sinf((float)nowT * 2));
        return;
    }
    if (!P.dead) mapsMaxSpeed = fmaxf(mapsMaxSpeed, hlen(P.vel));

    MapsPl pl[MAXP + 1];
    int n = maps_players(pl);
    maps_trail_tick(pl, n);

    float m = 40;                                         /* carte carree a gauche, panneau a droite (ou dessous si l'ecran est etroit) */
    int wide = w >= h * 1.25f;
    float S = wide ? fminf(h - 2 * m, w * 0.62f - 2 * m) : fminf(w - 2 * m, h * 0.58f - 2 * m);
    mapsMnX = boxes[0].mn.x; mapsMnZ = boxes[0].mn.z;
    float span = fmaxf(boxes[0].mx.x - boxes[0].mn.x, boxes[0].mx.z - boxes[0].mn.z);
    MapView v = { m, wide ? (h - S) * 0.5f : m, S / fmaxf(span, 1) };
    maps_draw_map(&v, S, pl, n);
    if (wide) maps_draw_panel(m + S + 60, m, w - S - 3 * m - 60, h - 2 * m);
    else maps_draw_panel(m, m + S + 60, w - 2 * m, h - S - 3 * m - 60);
}

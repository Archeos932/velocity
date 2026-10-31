/* VELOCITY - modes de jeu : objectifs dans le monde et dans le HUD, boutique, choix de l'arme de depart
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

static const float noTeamCol[3] = { 0.75f, 0.75f, 0.8f };
static const float *obj_team_col(int t) { return t == 0 || t == 1 ? teamCol[t] : noTeamCol; }
static void ring3(V3 c, float r, float y, int seg) {
    glBegin(GL_LINE_LOOP);
    for (int s = 0; s < seg; s++) glVertex3f(c.x + cosf(2 * PI * s / seg) * r, y, c.z + sinf(2 * PI * s / seg) * r);
    glEnd();
}
static void disc3(V3 c, float r, float y, float a0, float a1, int seg) {   /* secteur plein (progression) */
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(c.x, y, c.z);
    for (int s = 0; s <= seg; s++) { float a = a0 + (a1 - a0) * s / seg; glVertex3f(c.x + cosf(a) * r, y, c.z + sinf(a) * r); }
    glEnd();
}
/* drapeau : hampe et toile qui ondule, a la position p (pieds) */
static void draw_flag3(V3 p, const float *c, float t) {
    glColor4f(0.85f, 0.85f, 0.9f, 1);
    glBegin(GL_QUADS);
    glVertex3f(p.x - 0.05f, p.y, p.z); glVertex3f(p.x + 0.05f, p.y, p.z); glVertex3f(p.x + 0.05f, p.y + 3.2f, p.z); glVertex3f(p.x - 0.05f, p.y + 3.2f, p.z);
    glEnd();
    glBegin(GL_QUAD_STRIP);
    for (int k = 0; k <= 8; k++) {
        float u = k / 8.0f, w = sinf(t * 5 + u * 4) * 0.25f * u;
        float sh = 0.75f + 0.25f * sinf(t * 5 + u * 4 + 1);
        glColor4f(c[0] * sh, c[1] * sh, c[2] * sh, 1);
        glVertex3f(p.x + u * 1.8f, p.y + 3.15f, p.z + w);
        glVertex3f(p.x + u * 1.8f, p.y + 2.05f - u * 0.1f, p.z + w);
    }
    glEnd();
}

/* --- monde : zones, drapeaux, bouclier, barrieres des salles de depart --- */
static void draw_modes_world(void) {
    if (!dmMode) return;
    const MapDef *md = &maps[curMap];
    float t = (float)nowT;
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if (md->spawnRooms && teamSize > 0)                   /* salles de depart : voile de la couleur de l'equipe */
        for (int r = 0; r < 2; r++) {
            const float *b = md->spawnRooms[r], *c = teamCol[r];
            float a = 0.10f + 0.04f * sinf(t * 2 + r);
            glColor4f(c[0], c[1], c[2], a);
            glBegin(GL_QUADS);
            float y0 = b[1], y1 = b[1] + 4.5f;
            glVertex3f(b[0], y0, b[2]); glVertex3f(b[3], y0, b[2]); glVertex3f(b[3], y1, b[2]); glVertex3f(b[0], y1, b[2]);
            glVertex3f(b[0], y0, b[5]); glVertex3f(b[0], y1, b[5]); glVertex3f(b[3], y1, b[5]); glVertex3f(b[3], y0, b[5]);
            glVertex3f(b[0], y0, b[2]); glVertex3f(b[0], y1, b[2]); glVertex3f(b[0], y1, b[5]); glVertex3f(b[0], y0, b[5]);
            glVertex3f(b[3], y0, b[2]); glVertex3f(b[3], y0, b[5]); glVertex3f(b[3], y1, b[5]); glVertex3f(b[3], y1, b[2]);
            glEnd();
        }
    if (rules.mode == MODE_ZONES && md->zones) {
        for (int z = 0; z < NZONES; z++) {
            V3 c = zone_pos(z);
            const float *oc = obj_team_col(obj.z[z].owner);
            float pulse = 0.6f + 0.4f * sinf(t * 3 + z);
            glColor4f(oc[0], oc[1], oc[2], 0.16f);
            disc3(c, ZONE_R, c.y + 0.04f, 0, 2 * PI, 40);
            if (obj.z[z].prog > 0 && obj.z[z].capTeam < 2) {   /* progression : secteur de la couleur qui prend */
                const float *cc = teamCol[obj.z[z].capTeam];
                glColor4f(cc[0], cc[1], cc[2], 0.45f);
                disc3(c, ZONE_R * 0.98f, c.y + 0.06f, -PI / 2, -PI / 2 + 2 * PI * obj.z[z].prog / 255.0f, 40);
            }
            glLineWidth(4);
            glColor4f(oc[0], oc[1], oc[2], 0.6f + 0.35f * pulse);
            ring3(c, ZONE_R, c.y + 0.08f, 48);
            glLineWidth(2);
            glBegin(GL_LINES);                            /* colonne de lumiere */
            glColor4f(oc[0], oc[1], oc[2], 0.5f); glVertex3f(c.x, c.y, c.z);
            glColor4f(oc[0], oc[1], oc[2], 0.0f); glVertex3f(c.x, c.y + 30, c.z);
            glEnd();
        }
    }
    if (rules.mode == MODE_DRAPEAU && md->flags) {
        glDepthMask(GL_TRUE);
        for (int f = 0; f < 2; f++) {
            V3 h = flag_home(f);
            glLineWidth(3);
            glColor4f(teamCol[f][0], teamCol[f][1], teamCol[f][2], 0.7f);
            ring3(h, FLAG_TOUCH_R, h.y + 0.06f, 32);
            V3 p = flag_pos(f);
            if (obj.fl[f].state == FLAG_CARRIED) {
                int c = obj.fl[f].carrier;
                if (c == myId) continue;                  /* le mien : rien devant la camera */
                p = vadd(np_feet(c), v3(-0.4f, 1.2f, 0));
            }
            draw_flag3(p, teamCol[f], t + f);
            if (obj.fl[f].state == FLAG_DROPPED) {        /* retour en cours : cercle qui se remplit */
                glDepthMask(GL_FALSE);
                glColor4f(teamCol[f][0], teamCol[f][1], teamCol[f][2], 0.35f);
                disc3(p, FLAG_RETURN_R, p.y + 0.05f, -PI / 2, -PI / 2 + 2 * PI * obj.fl[f].ret / 255.0f, 32);
                glLineWidth(2); ring3(p, FLAG_RETURN_R, p.y + 0.07f, 32);
                glDepthMask(GL_TRUE);
            }
        }
        glDepthMask(GL_FALSE);
    }
    if (rules.mode == MODE_BOUCLIER && md->shield) {      /* generateur : dome qui palpite, plus rouge quand il faiblit */
        V3 c = shield_pos();
        float k = obj.shield / 100.0f;
        float col[3] = { 0.35f + 0.65f * (1 - k), 0.65f * k + 0.2f, 1.0f * k + 0.2f };
        glColor4f(col[0], col[1], col[2], 0.14f + 0.06f * sinf(t * 4));
        for (int j = 0; j < 6; j++) {
            float a0 = PI * 0.5f * j / 6, a1 = PI * 0.5f * (j + 1) / 6, R = SHIELD_ZONE_R;
            glBegin(GL_QUAD_STRIP);
            for (int s = 0; s <= 24; s++) {
                float b = 2 * PI * s / 24;
                glVertex3f(c.x + cosf(b) * cosf(a0) * R, c.y + sinf(a0) * R * 0.7f, c.z + sinf(b) * cosf(a0) * R);
                glVertex3f(c.x + cosf(b) * cosf(a1) * R, c.y + sinf(a1) * R * 0.7f, c.z + sinf(b) * cosf(a1) * R);
            }
            glEnd();
        }
        glLineWidth(4);
        glColor4f(col[0], col[1], col[2], 0.85f);
        ring3(c, SHIELD_ZONE_R, c.y + 0.08f, 48);
        glColor4f(col[0], col[1], col[2], 0.9f);         /* coeur */
        glBegin(GL_QUADS);
        for (int f = 0; f < 4; f++) {
            float a = f * PI / 2 + t, ca = cosf(a) * 0.6f, sa = sinf(a) * 0.6f;
            glVertex3f(c.x + ca, c.y + 0.5f, c.z + sa); glVertex3f(c.x - sa, c.y + 1.4f + 0.3f * sinf(t * 3), c.z + ca);
            glVertex3f(c.x - ca, c.y + 2.3f, c.z - sa); glVertex3f(c.x + sa, c.y + 1.4f + 0.3f * sinf(t * 3), c.z - ca);
        }
        glEnd();
    }
    glLineWidth(1);
    glDepthMask(GL_TRUE);
}

/* --- HUD : bandeau des objectifs, temps restant, marqueurs des objectifs a l'ecran --- */
static void obj_marker(V3 p, const char *s, const float *c) {
    float sx, sy;
    if (!project(vadd(p, v3(0, 3.5f, 0)), &sx, &sy)) return;
    float d = vlen(vsub(p, eye_pos()));
    char buf[32];
    snprintf(buf, sizeof buf, "%s %.0fm", s, d);
    text_c(1, sx, sy, buf, c[0], c[1], c[2], 0.9f);
}
static void draw_modes_hud(void) {
    if (!dmMode || matchOver) return;
    const MapDef *md = &maps[curMap];
    float cx = winW * 0.5f, y = 86;
    char buf[96];
    if (rules.mode != MODE_FRAGS) {                       /* temps restant */
        float left = fmaxf(0, match_time_limit() - matchT);
        snprintf(buf, sizeof buf, "%d:%02d", (int)left / 60, (int)left % 60);
        text_c(1, cx, y, buf, left < 30 ? 1.0f : 0.95f, left < 30 ? 0.4f : 0.95f, left < 30 ? 0.35f : 1.0f, 1);
        y += 30;
    }
    int me = myId < MAXP ? NP[myId].team & 1 : 0;
    if (rules.mode == MODE_ZONES && md->zones) {
        static const char *zl[NZONES] = { "A", "B", "C" };
        for (int z = 0; z < NZONES; z++) {
            float bx = cx - 75 + z * 60, by = y;
            const float *oc = obj_team_col(obj.z[z].owner);
            rect(bx - 22, by, 44, 44, oc[0] * 0.35f, oc[1] * 0.35f, oc[2] * 0.35f, 0.85f);
            rect(bx - 22, by + 40, 44, 4, oc[0], oc[1], oc[2], 1);
            if (obj.z[z].prog > 0 && obj.z[z].capTeam < 2) {
                const float *cc = teamCol[obj.z[z].capTeam];
                rect(bx - 22, by + 44, 44 * obj.z[z].prog / 255.0f, 4, cc[0], cc[1], cc[2], 1);
            }
            text_c(1, bx, by + 31, zl[z], 1, 1, 1, 1);
            obj_marker(zone_pos(z), zl[z], oc);
        }
        snprintf(buf, sizeof buf, "%d  -  %d   (%d)", obj.pts[0], obj.pts[1], ZONE_SCORE);
        text_c(0, cx, y + 70, buf, 0.9f, 0.9f, 1, 1);
    }
    if (rules.mode == MODE_DRAPEAU && md->flags) {
        for (int f = 0; f < 2; f++) {
            const char *st = obj.fl[f].state == FLAG_HOME ? "A LA BASE" : obj.fl[f].state == FLAG_DROPPED ? "TOMBE" : "PORTE";
            if (obj.fl[f].state == FLAG_CARRIED && obj.fl[f].carrier < MAXP) st = obj.fl[f].carrier == myId ? "TU LE PORTES !" : pname(obj.fl[f].carrier);
            snprintf(buf, sizeof buf, "DRAPEAU %s : %s", teamNames[f], st);
            text_c(0, cx + (f ? 170.0f : -170.0f), y + 14, buf, teamCol[f][0], teamCol[f][1], teamCol[f][2], 1);
            if (obj.fl[f].state != FLAG_HOME || f != me) obj_marker(flag_pos(f), f == me ? "TON DRAPEAU" : "DRAPEAU", teamCol[f]);
        }
        snprintf(buf, sizeof buf, "%d  -  %d   (%d)", obj.pts[0], obj.pts[1], FLAG_CAPS);
        text_c(1, cx, y + 20, buf, 1, 1, 1, 1);
        if (obj.fl[1 - me].state == FLAG_CARRIED && obj.fl[1 - me].carrier == myId)
            text_c(1, cx, winH * 0.5f + 90, obj.fl[me].state == FLAG_HOME ? "RAMENE-LE A TON DRAPEAU" : "TON DRAPEAU DOIT ETRE A LA BASE", 1, 0.85f, 0.3f, 0.9f);
    }
    if (rules.mode == MODE_BOUCLIER && md->shield) {
        float k = obj.shield / 100.0f;
        rect(cx - 150, y, 300, 16, 0.08f, 0.08f, 0.12f, 0.85f);
        rect(cx - 150, y, 300 * k, 16, 0.35f + 0.65f * (1 - k), 0.65f * k + 0.2f, 1.0f * k + 0.2f, 1);
        snprintf(buf, sizeof buf, "BOUCLIER %d %%  -  %s", obj.shield, me == 0 ? "DEFENDS-LE" : "SURCHARGE-LE");
        text_c(0, cx, y + 36, buf, 0.9f, 0.9f, 1, 1);
        obj_marker(shield_pos(), "BOUCLIER", teamCol[0]);
    }
    if (md->spawnRooms && teamSize > 0 && myId < MAXP && !P.dead && in_room(P.pos, md->spawnRooms[1 - me]))
        text_c(1, cx, winH * 0.5f + 60, "SALLE ADVERSE : TU BRULES", 1, 0.3f, 0.25f, 0.6f + 0.4f * sinf((float)nowT * 10));
    if (eco_on()) {                                       /* pieces */
        snprintf(buf, sizeof buf, "%d PIECES", credits);
        text(1, winW - 30.0f - fontW[1] * (float)strlen(buf), 70, buf, 1, 0.85f, 0.3f, 1);
    }
}

/* --- boutique : arme de depart (debut de match), achats a l'ecran de mort --- */
static void draw_shop(void) {
    if (!eco_on() || spectating || matchOver) return;
    float cx = winW * 0.5f, cy = winH * 0.5f;
    char buf[96];
    if (ecoNeedStarter && !P.dead) {
        rect(0, 0, (float)winW, (float)winH, 0.02f, 0.02f, 0.06f, 0.55f);
        text_c(2, cx, cy - 170, "CHOISIS TON ARME", 1, 0.85f, 0.3f, 1);
        snprintf(buf, sizeof buf, "les autres s'achetent a la boutique, a chaque mort  -  %.0f s", fmaxf(0, ecoStarterT));
        text_c(0, cx, cy - 135, buf, 0.8f, 0.82f, 0.9f, 1);
        float w = 250, x0 = cx - 2 * w - 15;
        for (int k = 0; k < 4; k++) {
            int wp = ecoStarters[k];
            const AmmoDef *d = &ammoDef[wp];
            float x = x0 + k * (w + 10);
            if (button(x, cy - 90, w, 130, d->name)) { eco_pick_starter(wp); loadout_apply(); }
            rect(x, cy - 90, w, 5, d->r, d->g, d->b, 1);
            snprintf(buf, sizeof buf, "%d au spawn, %d max", d->start, d->max);
            text_c(0, x + w * 0.5f, cy + 25, buf, 0.8f, 0.82f, 0.9f, 1);
        }
        if (uiClick) uiClick = 0;
        return;
    }
    if (!P.dead) return;
    float pw = 760, px = cx - pw * 0.5f, py = cy - 40;
    rect(px - 20, py - 50, pw + 40, 330, 0.02f, 0.03f, 0.07f, 0.88f);
    rect(px - 20, py - 50, pw + 40, 3, 1, 0.8f, 0.3f, 1);
    snprintf(buf, sizeof buf, "BOUTIQUE  -  %d PIECES", credits);
    text(1, px, py - 12, buf, 1, 0.85f, 0.3f, 1);
    text(0, px + 420, py - 14, "pris en compte au respawn", 0.6f, 0.65f, 0.75f, 1);
    float w = (pw - 30) / 4, h = 82;
    for (int i = 0, n = 0; i < WP_COUNT; i++) {
        if (!wpReady[i]) continue;
        const AmmoDef *d = &ammoDef[i];
        float x = px + (n % 4) * (w + 10), yy = py + 10 + (n / 4) * (h + 12);
        int own = (ecoOwned >> i) & 1, eq = ecoSlot[0] == i || ecoSlot[1] == i, can = own || credits >= eco_price(i);
        if (own) snprintf(buf, sizeof buf, eq ? "%s" : "%s  (A TOI)", d->name);   /* equipee : le cadre dore suffit */
        else snprintf(buf, sizeof buf, "%s  %d", d->name, eco_price(i));
        if (sel_button(x, yy, w, h, buf, eq, can, v3(d->r, d->g, d->b))) eco_buy(i);
        n++;
    }
    float yy = py + 10 + 2 * (h + 12);
    snprintf(buf, sizeof buf, ecoAmmoBoost ? "MUNITIONS + (A TOI)" : "MUNITIONS +  %d", ECO_AMMO_PRICE);
    if (sel_button(px, yy, w * 2 + 10, 56, buf, ecoAmmoBoost, ecoAmmoBoost || credits >= ECO_AMMO_PRICE, v3(1, 0.85f, 0.3f))) eco_buy_ammo();
    if (ecoSlot[1] >= 0 && button(px + 2 * (w + 10), yy, w * 2 + 10, 56, "INVERSER LES ARMES")) { int t = ecoSlot[0]; ecoSlot[0] = ecoSlot[1]; ecoSlot[1] = t; }
    snprintf(buf, sizeof buf, "EQUIPEES : %s%s%s", ecoSlot[0] >= 0 ? ammoDef[ecoSlot[0]].name : "-", ecoSlot[1] >= 0 ? " + " : "",
             ecoSlot[1] >= 0 ? ammoDef[ecoSlot[1]].name : "");
    text(0, px, yy + 82, buf, 0.85f, 0.88f, 0.95f, 1);
    if (uiClick) uiClick = 0;
}

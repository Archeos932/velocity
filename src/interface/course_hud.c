/* VELOCITY - mode COURSE : affichage (chrono, portes en 3D, fantome) et sauvegarde du record
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* --- record : velocity_course.txt a cote de velocity.ini (jamais en --bot / --shot / --test) --- */
static int race_file(char *out, int n) {
    if (!cfgPath[0]) return 0;
    snprintf(out, n, "%s", cfgPath);
    char *s = strrchr(out, '/'), *b = strrchr(out, '\\');
    if (b > s) s = b;
    if (!s) return 0;
    snprintf(s + 1, n - (int)(s + 1 - out), "velocity_course.txt");
    return 1;
}
static void race_save(void) {
    char path[MAX_PATH + 32];
    if (botMode || shotMode || !race_file(path, sizeof path)) return;
    FILE *f = fopen(path, "w");
    if (!f) return;
    const MapDef *md = &maps[curMap];
    fprintf(f, "carte=%s\nrecord=%.4f\nportes=%d\n", md->name, raceBest, md->ngates);
    for (int i = 0; i < md->ngates; i++) fprintf(f, "%.4f\n", raceBestSplit[i]);
    fprintf(f, "fantome=%d\n", ghostBestN);
    for (int i = 0; i < ghostBestN; i++) fprintf(f, "%.2f %.2f %.2f %.3f\n", ghostBest[i][0], ghostBest[i][1], ghostBest[i][2], ghostBest[i][3]);
    fclose(f);
}
static void race_load(void) {
    char path[MAX_PATH + 32], line[128], name[32] = "";
    if (botMode || shotMode || !race_file(path, sizeof path)) return;
    FILE *f = fopen(path, "r");
    if (!f) return;
    int ng = 0, gn = 0;
    float best = 0;
    if (fgets(line, sizeof line, f)) sscanf(line, "carte=%31[^\n]", name);
    if (fgets(line, sizeof line, f)) sscanf(line, "record=%f", &best);
    if (fgets(line, sizeof line, f)) sscanf(line, "portes=%d", &ng);
    const MapDef *md = &maps[race_map()];
    if (strcmp(name, md->name) || ng != md->ngates || ng > RACE_GATES || best <= 0) { fclose(f); return; }   /* autre carte, autre trace */
    for (int i = 0; i < ng && fgets(line, sizeof line, f); i++) raceBestSplit[i] = (float)atof(line);
    if (fgets(line, sizeof line, f)) sscanf(line, "fantome=%d", &gn);
    if (gn < 0 || gn > RACE_GMAX) gn = 0;
    int k = 0;
    while (k < gn && fgets(line, sizeof line, f))
        if (sscanf(line, "%f %f %f %f", &ghostBest[k][0], &ghostBest[k][1], &ghostBest[k][2], &ghostBest[k][3]) == 4) k++;
    fclose(f);
    raceBest = best; ghostBestN = k;
}

static void race_time_str(char *b, int n, float t) {      /* 1:02.37 */
    int ms = (int)(t * 100 + 0.5f);
    snprintf(b, n, "%d:%02d.%02d", ms / 6000, (ms / 100) % 60, ms % 100);
}

/* --- monde : portes (anneaux), colonne de lumiere sur la suivante, fantome du record --- */
static void draw_race_world(void) {
    if (!raceOn) return;
    const MapDef *md = &maps[curMap];
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glDisable(GL_FOG);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    for (int i = raceGate; i < md->ngates && i < raceGate + 3; i++) {
        V3 c = race_gate(i), prev = i > 0 ? race_gate(i - 1) : md->start;
        float r = md->gates[i][3];
        V3 n = vnorm(vsub(c, prev)), u = vnorm(vcross(n, fabsf(n.y) > 0.9f ? v3(1, 0, 0) : v3(0, 1, 0))), v = vcross(n, u);
        int next = i == raceGate, fin = i == md->ngates - 1;
        float pulse = 0.75f + 0.25f * sinf((float)nowT * 6), a = next ? 0.95f : 0.35f;
        float col[3] = { fin ? 1.0f : 0.3f, fin ? 0.8f : 0.85f, fin ? 0.25f : 1.0f };
        for (int ring = 0; ring < (next ? 3 : 1); ring++) {   /* la porte suivante : trois anneaux qui respirent */
            float rr = r * (1 - ring * 0.08f) * (next ? pulse * 0.1f + 0.95f : 1);
            glLineWidth(next ? 5.0f - ring : 2.5f);
            glColor4f(col[0], col[1], col[2], a * (1 - ring * 0.3f));
            glBegin(GL_LINE_LOOP);
            for (int s = 0; s < 48; s++) {
                float t = s * 2 * PI / 48;
                V3 p = vadd(c, vadd(vmul(u, cosf(t) * rr), vmul(v, sinf(t) * rr)));
                glVertex3f(p.x, p.y, p.z);
            }
            glEnd();
        }
        if (next) {                                       /* voile leger dans l'anneau + colonne de lumiere */
            glColor4f(col[0], col[1], col[2], 0.08f * pulse);
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(c.x, c.y, c.z);
            for (int s = 0; s <= 48; s++) {
                float t = s * 2 * PI / 48;
                V3 p = vadd(c, vadd(vmul(u, cosf(t) * r), vmul(v, sinf(t) * r)));
                glVertex3f(p.x, p.y, p.z);
            }
            glEnd();
            glLineWidth(3);
            glBegin(GL_LINES);
            glColor4f(col[0], col[1], col[2], 0.0f); glVertex3f(c.x, c.y + r + 60, c.z);
            glColor4f(col[0], col[1], col[2], 0.35f); glVertex3f(c.x, c.y + r, c.z);
            glEnd();
        }
    }
    {                                                     /* points de balancement : contour dore qui brille, plus fort sur la cible de l'aimant */
        V3 at; int tgt = -1;
        if (P.hook == HOOK_NONE && race_anchor(eye_pos(), &at) > 0)   /* la ou partira la toile */
            for (int i = 0; i < nb; i++) if (swing_box(i) && vlen(vsub(vmul(vadd(boxes[i].mn, boxes[i].mx), 0.5f), at)) < 0.5f) tgt = i;
        float pulse = 0.6f + 0.4f * sinf((float)nowT * 4);
        for (int i = 0; i < nb; i++) {
            if (!swing_box(i)) continue;
            V3 c = vmul(vadd(boxes[i].mn, boxes[i].mx), 0.5f);
            if (vlen(vsub(c, camEye)) > 140) continue;
            int hot = i == tgt || (P.hook == HOOK_ATTACHED && vlen(vsub(P.anchor, c)) < 2.5f);
            for (int k = 0; k < 2; k++) {
                float g = k ? 0.9f + 0.3f * pulse : 0.05f;
                glLineWidth(k ? 2.5f : (hot ? 5.0f : 3.0f));
                glColor4f(1, 0.8f, 0.3f, hot ? 0.95f : k ? 0.35f + 0.25f * pulse : 0.5f);
                box_edges(vsub(boxes[i].mn, v3(g, g, g)), vadd(boxes[i].mx, v3(g, g, g)));
            }
        }
    }
    glLineWidth(1);
    V3 gp; float gy;
    if (raceCount <= 0 && race_ghost_at(raceT, &gp, &gy)) {   /* fantome : silhouette bleutee, additive */
        SkinAnim an;
        V3 gp2; float gy2;
        race_ghost_at(raceT + 0.1f, &gp2, &gy2);
        float sp = hlen(vsub(gp2, gp)) * 10;
        an.spd = clampf(sp / 12, 0, 1); an.run = raceT * 9; an.air = 1; an.slide = 0; an.pitch = 0; an.t = raceT;
        skinFlat = 1; skinFlatCol[0] = 0.25f; skinFlatCol[1] = 0.6f; skinFlatCol[2] = 0.9f;
        static const float gc[3] = { 0.3f, 0.7f, 1.0f };
        glPushMatrix();
        glTranslatef(gp.x, gp.y, gp.z);
        glRotatef(-gy * 180 / PI, 0, 1, 0);
        draw_skin(wantSkin, H_STAND, gc, &an);
        glPopMatrix();
        skinFlat = 0;
    }
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_FOG);
}

/* --- HUD : chrono, portes, ecart au record, decompte, fleche vers la porte, arrivee --- */
static void draw_race_hud(void) {
    if (!raceOn) return;
    const MapDef *md = &maps[curMap];
    float cx = winW * 0.5f, cy = winH * 0.5f;
    char buf[64], tb[24];
    race_time_str(tb, sizeof tb, raceT);
    hud_panel(cx - 130, 14, 260, 92);
    text_c(2, cx, 66, tb, 1, 1, 1, 1);
    snprintf(buf, sizeof buf, "PORTE %d / %d", raceGate < md->ngates ? raceGate + 1 : md->ngates, md->ngates);
    text_c(0, cx, 94, buf, 0.5f, 0.85f, 1, 0.9f);
    if (raceBest > 0) {
        race_time_str(tb, sizeof tb, raceBest);
        snprintf(buf, sizeof buf, "RECORD %s", tb);
        text(0, cx + 140, 40, buf, 1, 0.8f, 0.3f, 0.85f);
    }
    if (raceFalls) { snprintf(buf, sizeof buf, "chutes %d", raceFalls); text(0, cx + 140, 62, buf, 0.8f, 0.5f, 0.5f, 0.8f); }
    if (raceDeltaT > 0 && raceBest > 0 && !raceDone) {    /* ecart au record a la porte */
        snprintf(buf, sizeof buf, "%+.2f", raceDelta);
        float a = clampf(raceDeltaT, 0, 1);
        if (raceDelta <= 0) text_c(1, cx, 132, buf, 0.4f, 1, 0.5f, a); else text_c(1, cx, 132, buf, 1, 0.4f, 0.35f, a);
    }
    if (raceGateFlash > 0) rect(0, 0, (float)winW, (float)winH, 0.3f, 0.85f, 1, raceGateFlash * 0.12f);
    if (raceCount > 0) {                                  /* decompte */
        snprintf(buf, sizeof buf, "%d", (int)ceilf(raceCount));
        float k = raceCount - floorf(raceCount);
        text_c(2, cx, cy - 60, buf, 1, 0.85f, 0.3f, 0.4f + 0.6f * k);
        text_c(0, cx, cy + 10, "MAINTIENS TIR ou GRAPPIN pour te balancer, REGARDE ou tu veux aller", 0.8f, 0.85f, 0.95f, 0.9f);
        text_c(0, cx, cy + 34 + 24, "la toile s'accroche toute seule ; lache dans la remontee pour un elan", 0.6f, 0.7f, 0.85f, 0.9f);
        snprintf(buf, sizeof buf, "[R] recommencer   %s", ghostBestN ? "le fantome bleu = ton record" : "");
        text_c(0, cx, cy + 34, buf, 0.6f, 0.65f, 0.75f, 0.9f);
    } else if (raceT < 0.7f) text_c(2, cx, cy - 60, "GO !", 0.4f, 1, 0.5f, 1 - raceT / 0.7f);
    if (!raceDone && raceCount <= 0 && P.hook == HOOK_NONE && !P.dead) {   /* aimant : le point que prendra le grappin */
        V3 at; float sx, sy;
        if (race_anchor(eye_pos(), &at) > 0 && project(at, &sx, &sy)) {   /* la toile partira ici (sans viser) */
            float r = 22 + 4 * sinf((float)nowT * 9), a0 = (float)nowT * 2;
            glLineWidth(3);
            glColor4f(1, 0.85f, 0.3f, 0.95f);
            glBegin(GL_LINES);                            /* quatre crochets qui tournent autour du cube */
            for (int k = 0; k < 4; k++) {
                float a = a0 + k * PI / 2;
                glVertex2f(sx + cosf(a) * r, sy + sinf(a) * r);
                glVertex2f(sx + cosf(a + 0.5f) * r, sy + sinf(a + 0.5f) * r);
            }
            glEnd();
            glLineWidth(1);
            snprintf(buf, sizeof buf, "[%s] maintenir", bind_text(A_FIRE));
            text_c(0, sx, sy + r + 18, buf, 1, 0.85f, 0.3f, 0.9f);
        }
    }
    if (!raceDone && raceGate < md->ngates) {             /* fleche au bord de l'ecran si la porte est hors champ */
        float sx, sy;
        V3 g = race_gate(raceGate);
        int on = project(g, &sx, &sy) && sx > 40 && sx < winW - 40 && sy > 40 && sy < winH - 40;
        if (!on) {
            V3 d = vsub(g, eye_pos());
            float rel = atan2f(d.x, -d.z) - P.yaw;
            while (rel > PI) rel -= 2 * PI;
            while (rel < -PI) rel += 2 * PI;
            float ax = cx + sinf(rel) * (winH * 0.36f), ay = cy - cosf(rel) * (winH * 0.36f);
            glColor4f(0.3f, 0.85f, 1, 0.9f);
            glBegin(GL_TRIANGLES);
            glVertex2f(ax + sinf(rel) * 18, ay - cosf(rel) * 18);
            glVertex2f(ax + cosf(rel) * 10, ay + sinf(rel) * 10);
            glVertex2f(ax - cosf(rel) * 10, ay - sinf(rel) * 10);
            glEnd();
        }
    }
    if (raceDone) {                                       /* arrivee */
        rect(cx - 260, cy - 130, 520, 220, 0.02f, 0.03f, 0.07f, 0.85f);
        rect(cx - 260, cy - 130, 520, 3, 1, 0.8f, 0.3f, 1);
        text_c(2, cx, cy - 70, "ARRIVEE", 1, 0.85f, 0.3f, 1);
        race_time_str(tb, sizeof tb, raceT);
        text_c(2, cx, cy - 10, tb, 1, 1, 1, 1);
        if (raceNewBest == 2) text_c(1, cx, cy + 34, "NOUVEAU RECORD !", 0.4f, 1, 0.5f, 0.6f + 0.4f * sinf((float)nowT * 8));
        else if (raceNewBest == 1) text_c(1, cx, cy + 34, "PREMIER TEMPS : ton fantome t'attend", 0.5f, 0.85f, 1, 1);
        else {
            snprintf(buf, sizeof buf, "+%.2f sur le record", raceT - raceBest);
            text_c(1, cx, cy + 34, buf, 1, 0.5f, 0.4f, 1);
        }
        snprintf(buf, sizeof buf, "chutes %d   -   style max %s", raceFalls, styleName[styleTopRank]);
        text_c(0, cx, cy + 62, buf, 0.7f, 0.75f, 0.85f, 1);
        text_c(0, cx, cy + 82, "[R] recommencer        [ECHAP] menu", 0.9f, 0.9f, 1, 1);
    }
}

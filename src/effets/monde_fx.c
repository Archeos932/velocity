/* VELOCITY - effets du monde : neons, lumieres, marques, fumee, meteo
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ---- effets du monde : neons, lumieres dynamiques, marques, ondes, fumee, meteo, reflets du soleil ---- */
static void surf_axes(V3 n, V3 *t1, V3 *t2) {             /* deux axes dans le plan de normale n */
    *t1 = fabsf(n.y) > 0.9f ? v3(1, 0, 0) : vnorm(vcross(v3(0, 1, 0), n));
    *t2 = vcross(n, *t1);
}
static void surf_quad(V3 p, V3 n, float s, float rot) {   /* quad texture (texDot) pose sur une surface */
    V3 t1, t2;
    surf_axes(n, &t1, &t2);
    float c = cosf(rot) * s, sn = sinf(rot) * s;
    V3 u = vadd(vmul(t1, c), vmul(t2, sn)), v = vadd(vmul(t1, -sn), vmul(t2, c));
    glTexCoord2f(0, 0); glVertex3f(p.x - u.x - v.x, p.y - u.y - v.y, p.z - u.z - v.z);
    glTexCoord2f(1, 0); glVertex3f(p.x + u.x - v.x, p.y + u.y - v.y, p.z + u.z - v.z);
    glTexCoord2f(1, 1); glVertex3f(p.x + u.x + v.x, p.y + u.y + v.y, p.z + u.z + v.z);
    glTexCoord2f(0, 1); glVertex3f(p.x - u.x + v.x, p.y - u.y + v.y, p.z - u.z + v.z);
}
/* lumiere ponctuelle posee sur le sol et les murs proches (5 rayons), en additif */
static void light_splat(V3 p, V3 col, float r, float inten) {
    static const float dirs[5][3] = { { 0, -1, 0 }, { 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    for (int k = 0; k < 5; k++) {
        V3 d = v3(dirs[k][0], dirs[k][1], dirs[k][2]), n;
        float t = ray_world(p, d, r, &n);
        if (t >= r) continue;
        float f = (1 - t / r) * inten;
        glColor4f(col.x * f, col.y * f, col.z * f, 1);
        surf_quad(vadd(vadd(p, vmul(d, t)), vmul(n, 0.03f)), n, r * (1 - t / r * 0.5f), 0);
    }
}

static void fx_world_tick(float dt) {                     /* temps reel : les effets s'eteignent meme en pause */
    for (int i = 0; i < MAXDL; i++) if (DL[i].life > 0) DL[i].life -= dt;
    for (int i = 0; i < MAXDC; i++) if (DC[i].life > 0) DC[i].life -= dt;
    for (int i = 0; i < MAXRG; i++) if (RG[i].life > 0) RG[i].life -= dt;
    for (int i = 0; i < MAXSM; i++) {
        Smoke *m = &SMK[i];
        if (m->life <= 0) continue;
        m->life -= dt;
        m->p = vadd(m->p, vmul(m->v, dt));
        m->v = vmul(m->v, expf(-dt * 1.2f));
        m->v.y += dt * 0.6f;
    }
}

/* passe opaque/melangee apres le monde : marques d'impact puis fumee */
static void draw_world_marks(void) {
    glDisable(GL_LIGHTING); glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texDot);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); glDepthFunc(GL_LEQUAL);
    glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(-1, -3);
    glBegin(GL_QUADS);
    for (int i = 0; i < MAXDC; i++) {
        Decal *d = &DC[i];
        if (d->life <= 0) continue;
        glColor4f(d->col.x, d->col.y, d->col.z, 0.85f * clampf(d->life / 2.0f, 0, 1));
        surf_quad(d->p, d->n, d->s, d->rot);
    }
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBegin(GL_QUADS);                                    /* fumee : billboards gris, melanges */
    for (int i = 0; i < MAXSM; i++) {
        Smoke *m = &SMK[i];
        if (m->life <= 0) continue;
        float k = m->life / m->max, sz = m->size * (1.6f - 0.8f * k);
        glColor4f(m->shade, m->shade * 0.95f, m->shade * 1.05f, 0.42f * k * (1 - k) * 4 * 0.6f);
        billboard(m->p, sz);
    }
    glEnd();
    glDepthFunc(GL_LESS); glDepthMask(GL_TRUE);
    glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
}

/* passe additive : neons, lumieres dynamiques (explosions, rail, roquettes en vol), ondes de choc */
static void draw_world_glow(void) {
    if (gxNeon[0] > 0 && nneon) {                         /* bandes de neon : un coeur clair et un halo */
        glDisable(GL_TEXTURE_2D);
        for (int pass = 0; pass < 2; pass++) {
            float w = pass ? 0.05f : 0.14f, a = (pass ? 1.0f : 0.35f) * gxNeon[0];
            glBegin(GL_QUADS);
            for (int i = 0; i < nneon; i++) {
                const float *c = neon_col(NEO[i].kind);
                V3 A = NEO[i].a, B = NEO[i].b;
                glColor4f(fminf(1, c[0] + pass * 0.4f), fminf(1, c[1] + pass * 0.4f), fminf(1, c[2] + pass * 0.4f), a);
                glVertex3f(A.x, A.y + w, A.z); glVertex3f(B.x, B.y + w, B.z); glVertex3f(B.x, B.y - w, B.z); glVertex3f(A.x, A.y - w, A.z);
            }
            glEnd();
        }
    }
    if (gxLights > 0) {
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texDot);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(-1, -4);
        glBegin(GL_QUADS);
        for (int i = 0; i < MAXDL; i++) {
            DynLight *l = &DL[i];
            if (l->life <= 0) continue;
            float k = l->life / l->max;
            light_splat(l->p, l->col, l->r, k * k * 0.9f * gxLights);
        }
        for (int i = 0; i < MAXPR; i++) {                 /* roquettes, projectiles renvoyes, orbes : eclairent en vol */
            if (!PR[i].alive || PR[i].kind == PR_SAW || PR[i].kind == PR_MINE) continue;
            V3 c = PR[i].kind == PR_ORB ? v3(1, 0.3f, 0.9f) : v3(1, 0.6f, 0.25f);
            light_splat(PR[i].pos, c, 4.5f, 0.45f * gxLights);
        }
        glEnd();
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDepthFunc(GL_LESS);
        glDisable(GL_TEXTURE_2D);
    }
    for (int i = 0; i < MAXRG; i++) {                     /* ondes de choc : anneau qui s'elargit au sol */
        Ring *g = &RG[i];
        if (g->life <= 0) continue;
        float k = 1 - g->life / g->max, r0 = g->r * k, r1 = r0 + 0.25f + 0.5f * k, a = (1 - k) * 0.8f;
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= 32; s++) {
            float an = s * (2 * PI / 32), cs = cosf(an), sn = sinf(an);
            glColor4f(g->col.x, g->col.y, g->col.z, 0);
            glVertex3f(g->p.x + cs * r0, g->p.y, g->p.z + sn * r0);
            glColor4f(g->col.x, g->col.y, g->col.z, a);
            glVertex3f(g->p.x + cs * r1, g->p.y, g->p.z + sn * r1);
        }
        glEnd();
    }
}

/* poussiere en suspension et pluie autour de la camera (passe additive, positions deterministes qui defilent) */
static void draw_weather(V3 eye) {
    float t = (float)nowT;
    if (gxDust > 0) {
        glPointSize(2);
        glBegin(GL_POINTS);
        for (int i = 0; i < (int)gxDust; i++) {
            unsigned int h = (unsigned)i * 2654435761u;
            float fx = (float)(h & 1023) / 1023, fy = (float)((h >> 10) & 1023) / 1023, fz = (float)((h >> 20) & 1023) / 1023;
            V3 p = v3(fx * 24 + sinf(t * 0.13f + i) * 1.5f + t * 0.25f, fy * 10 + sinf(t * 0.21f + i * 0.7f) * 0.6f, fz * 24 + t * 0.15f);
            p.x = eye.x - 12 + fmodf(fmodf(p.x - eye.x + 12, 24) + 24, 24);   /* cube de 24 m qui suit la camera */
            p.y = eye.y - 4 + fmodf(fmodf(p.y - eye.y + 4, 10) + 10, 10);
            p.z = eye.z - 12 + fmodf(fmodf(p.z - eye.z + 12, 24) + 24, 24);
            float d = vlen(vsub(p, eye)), a = clampf(1 - d / 12, 0, 1) * 0.5f;
            glColor4f(sunCol[0], sunCol[1], sunCol[2], a);
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }
    if (gxRain > 0) {
        glLineWidth(1);
        glBegin(GL_LINES);
        for (int i = 0; i < (int)gxRain; i++) {
            unsigned int h = (unsigned)(i + 7) * 2246822519u;
            float fx = (float)(h & 1023) / 1023, fz = (float)((h >> 10) & 1023) / 1023, ph = (float)((h >> 20) & 1023) / 1023;
            float y = fmodf(ph * 30 - t * 24 + 3000, 30) - 12;
            V3 p = v3(eye.x - 15 + fmodf(fx * 30 - eye.x + 3000, 30), eye.y + y, eye.z - 15 + fmodf(fz * 30 - eye.z + 3000, 30));
            glColor4f(0.6f, 0.7f, 0.9f, 0.0f); glVertex3f(p.x + 0.05f, p.y + 0.7f, p.z);
            glColor4f(0.6f, 0.7f, 0.9f, 0.35f); glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }
}

/* orage : eclairs aleatoires (flash d'ecran + tonnerre) ; renvoie l'intensite du flash en cours */
static float stormFlash, stormNext = 4, thunderT = -1;
static void storm_tick(float dt) {
    if (gxStorm <= 0) { stormFlash = 0; return; }
    stormFlash = fmaxf(0, stormFlash - dt * 4);
    if ((stormNext -= dt) <= 0) {
        stormFlash = 1;
        stormNext = frange(0.3f, 2.0f) * 60 / gxStorm;
        thunderT = frange(0.4f, 1.6f);
    }
    if (thunderT > 0 && (thunderT -= dt) <= 0) play(S_EXPLO, 0.55f, 0.32f);
}

/* reflets du soleil dans l'objectif quand on le regarde (2D, additif) */
static void draw_flare(V3 eye) {
    if (gxFlare <= 0) return;
    float sx, sy;
    V3 sp = vadd(eye, vmul(sun_dir(), 250));
    if (vdot(vsub(sp, eye), view_dir()) <= 0 || !project(sp, &sx, &sy) || viewSun < 0.05f) return;
    float cx = winW * 0.5f, cy = winH * 0.5f, dx = cx - sx, dy = cy - sy;
    float on = clampf(1 - sqrtf(dx * dx + dy * dy) / (winW * 0.7f), 0, 1) * viewSun * gxFlare;
    if (on <= 0) return;
    static const float ghosts[6][5] = {                   /* position le long de l'axe, taille, couleur */
        { 0.0f, 140, 1.0f, 0.75f, 0.5f }, { 0.35f, 30, 0.6f, 0.9f, 1.0f }, { 0.6f, 55, 1.0f, 0.5f, 0.8f },
        { 0.9f, 18, 0.7f, 1.0f, 0.6f }, { 1.25f, 80, 0.5f, 0.6f, 1.0f }, { 1.6f, 36, 1.0f, 0.8f, 0.4f },
    };
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, winW, winH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texDot);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_QUADS);
    for (int g = 0; g < 6; g++) {
        float x = sx + dx * ghosts[g][0] * 2, y = sy + dy * ghosts[g][0] * 2, r = ghosts[g][1] * (winH / 900.0f);
        glColor4f(ghosts[g][2], ghosts[g][3], ghosts[g][4], (g ? 0.12f : 0.3f) * on);
        glTexCoord2f(0, 0); glVertex2f(x - r, y - r); glTexCoord2f(1, 0); glVertex2f(x + r, y - r);
        glTexCoord2f(1, 1); glVertex2f(x + r, y + r); glTexCoord2f(0, 1); glVertex2f(x - r, y + r);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);                                /* rayons du soleil */
    for (int k = 0; k < 12; k++) {
        float an = k * (PI / 6) + (float)nowT * 0.05f, w = 0.025f, len = winH * (0.25f + 0.1f * (k & 1));
        glColor4f(1, 0.85f, 0.6f, 0.12f * on); glVertex2f(sx, sy);
        glColor4f(1, 0.85f, 0.6f, 0);
        glVertex2f(sx + cosf(an - w) * len, sy + sinf(an - w) * len); glVertex2f(sx + cosf(an + w) * len, sy + sinf(an + w) * len);
    }
    glEnd();
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

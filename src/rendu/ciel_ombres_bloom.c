/* VELOCITY - ciel, ombres dynamiques, bloom
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ---- ciel, ombres dynamiques, bloom --------------------------------------------------------- */

/* couleur du ciel dans la direction d (unitaire) : zenith sombre, horizon violet, lueur autour du soleil */
static void sky_color(V3 d, float out[3]) {
    const float *zen = gxSkyZen, *mid = gxSkyMid, *hor = gxSkyHor;
    float mmid[3];
    if (curMap != 0) {                                    /* ciel de la carte : haut = sky, horizon = fog */
        zen = maps[curMap].sky; hor = maps[curMap].fog;
        for (int c = 0; c < 3; c++) mmid[c] = (zen[c] + hor[c]) * 0.5f;
        mid = mmid;
    }
    V3 S = sun_dir();
    float e = clampf(d.y, 0, 1), k1 = clampf(e / 0.25f, 0, 1), k2 = clampf((e - 0.25f) / 0.75f, 0, 1);
    for (int c = 0; c < 3; c++) out[c] = e < 0.25f ? hor[c] + (mid[c] - hor[c]) * k1 : mid[c] + (zen[c] - mid[c]) * sqrtf(k2);
    float g = fmaxf(0, vdot(d, S));
    V3 hd = vnorm(v3(d.x, 0, d.z)), hs = vnorm(v3(S.x, 0, S.z));
    float az = fmaxf(0, vdot(hd, hs)), low = powf(1 - fabsf(d.y), 5);
    float glow = powf(g, 6) * gxSunGlow[0] + powf(g, 40) * gxSunGlow[1], dusk = powf(az, 3) * low * gxSunGlow[2];
    out[0] += glow * 1.0f + dusk * 1.0f;
    out[1] += glow * 0.72f + dusk * 0.48f;
    out[2] += glow * 0.50f + dusk * 0.30f;
    if (d.y < 0) for (int c = 0; c < 3; c++) out[c] = hor[c];   /* sous l'horizon : couleur du brouillard */
}

/* degrade du ciel : ne depend que de la carte et de rendu.ini, donc liste d'affichage refaite avec le monde
   (dome de rayon 300 centre sur l'origine ; draw_sky_dome le place sur l'oeil) */
static GLuint skyDomeList;
static void build_sky_list(void) {
    if (skyDomeList) glDeleteLists(skyDomeList, 1);
    skyDomeList = glGenLists(1);
    glNewList(skyDomeList, GL_COMPILE);
    const int NA = 36, NE = 14;
    for (int j = 0; j < NE; j++) {                        /* bandes de -15 deg a +90 deg */
        float e0 = -0.26f + (PI * 0.5f + 0.26f) * j / NE, e1 = -0.26f + (PI * 0.5f + 0.26f) * (j + 1) / NE;
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= NA; i++) {
            float a = 2 * PI * i / NA;
            for (int k = 0; k < 2; k++) {
                float e = k ? e1 : e0, col[3];
                V3 d = v3(cosf(e) * cosf(a), sinf(e), cosf(e) * sinf(a));
                sky_color(d, col);
                glColor3f(col[0], col[1], col[2]);
                glVertex3f(d.x * 300, d.y * 300, d.z * 300);
            }
        }
        glEnd();
    }
    glEndList();
}

/* le ciel est dessine APRES le monde, avec le test de profondeur : seuls les pixels encore vides sont peints
   (avant, tout l'ecran etait rempli puis recouvert). Pour rester derriere le decor le plus lointain (montagnes
   de LA CITADELLE vers 400 m), tout le ciel est agrandi de SKY_SCALE autour de l'oeil (dome a 480 m, plan lointain
   a 500 m). */
#define SKY_SCALE 1.6f
static void draw_sky_dome(V3 eye) {
    glDisable(GL_FOG); glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glPushMatrix();
    glTranslatef(eye.x, eye.y, eye.z);
    glScalef(SKY_SCALE, SKY_SCALE, SKY_SCALE);
    if (skyDomeList) glCallList(skyDomeList);
    glTranslatef(-eye.x, -eye.y, -eye.z);                 /* le reste (etoiles, nuages, soleil) : autour de l'oeil, agrandi */
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    unsigned int seed = 99;                               /* etoiles, plus nombreuses loin du soleil */
    glPointSize(2);
    glBegin(GL_POINTS);
    for (int i = 0; i < (int)gxStars; i++) {
        float a, b, c;
        seed = seed * 1664525u + 1013904223u; a = (seed >> 8) / 16777216.0f * 2 - 1;
        seed = seed * 1664525u + 1013904223u; b = (seed >> 8) / 16777216.0f;
        seed = seed * 1664525u + 1013904223u; c = (seed >> 8) / 16777216.0f * 2 - 1;
        V3 d = vnorm(v3(a, b * 0.9f + 0.1f, c));
        float tw = 0.5f + 0.5f * sinf((float)nowT * 2 + i), away = clampf(1 - vdot(d, sun_dir()) * 1.4f, 0, 1);
        glColor4f(0.8f, 0.85f, 1.0f, (0.25f + 0.45f * tw) * d.y * away);
        glVertex3f(eye.x + d.x * 290, eye.y + d.y * 290, eye.z + d.z * 290);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texDot);
    if (gxClouds[0] > 0) {                                /* nuages : amas de taches douces qui derivent lentement */
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
        unsigned int cs = 0xC10Du;
        for (int k = 0; k < (int)gxClouds[0]; k++) {
            float az = tnoise(&cs) * 2 * PI + (float)nowT * 0.004f, el = 0.12f + tnoise(&cs) * 0.45f, sz = 18 + tnoise(&cs) * 26;
            V3 d = v3(cosf(el) * cosf(az), sinf(el), cosf(el) * sinf(az));
            V3 side = vnorm(v3(-d.z, 0, d.x));
            float lit = 0.6f + 0.4f * fmaxf(0, vdot(d, sun_dir()));
            for (int q = 0; q < 6; q++) {
                float o = (tnoise(&cs) - 0.5f) * sz * 2.2f, oy = (tnoise(&cs) - 0.5f) * sz * 0.35f, s2 = sz * (0.5f + tnoise(&cs) * 0.6f);
                V3 p = vadd(vadd(eye, vmul(d, 270)), vadd(vmul(side, o), v3(0, oy, 0)));
                glColor4f(gxCloudCol[0] * lit, gxCloudCol[1] * lit, gxCloudCol[2] * lit, gxClouds[1] * 0.5f);
                billboard(p, s2);
            }
        }
        glEnd();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    V3 sun = vadd(eye, vmul(sun_dir(), 280));
    glBegin(GL_QUADS);
    glColor4f(sunCol[0], sunCol[1] * 0.65f, sunCol[2] * 0.58f, 0.16f); billboard(sun, gxSunSize[2]);
    glColor4f(sunCol[0], sunCol[1] * 0.9f, sunCol[2] * 0.83f, 0.35f); billboard(sun, gxSunSize[1]);
    glColor4f(1.0f, 0.95f, 0.85f, 0.95f); billboard(sun, gxSunSize[0]);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glPopMatrix();
    glDepthMask(GL_TRUE);
    glEnable(GL_FOG);
}

/* silhouette de ville au loin, hors de l'arene (decor seulement, aucune collision) */
static void build_skyline_list(void) {
    unsigned int s = 0x51C7u;
    skylineList = glGenLists(1);
    glNewList(skylineList, GL_COMPILE);
    glDisable(GL_FOG); glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    for (int k = 0; k < (int)gxCity[0]; k++) {           /* ville = nombre, distance, ecart, hauteur min, hauteur en plus */
        float a = tnoise(&s) * 2 * PI, r = gxCity[1] + tnoise(&s) * gxCity[2], w = 7 + tnoise(&s) * 12, d = 7 + tnoise(&s) * 12;
        float h = gxCity[3] + tnoise(&s) * tnoise(&s) * gxCity[4], cx = cosf(a) * r, cz = sinf(a) * r, t = tnoise(&s);
        float base[3] = { gxCityCol[0] + 0.03f * t, gxCityCol[1] + 0.02f * t, gxCityCol[2] + 0.04f * t };   /* brume */
        glBegin(GL_QUADS);                                /* faces : plus claires en bas (brume), sombres en haut */
        V3 a0 = v3(cx - w, -2, cz - d), a1 = v3(cx + w, h, cz + d);
        for (int f = 0; f < 6; f++) {
            if (f == 3) continue;
            V3 c[4];
            face_corners(a0, a1, f, c);
            for (int q = 0; q < 4; q++) {
                float m = 1 - clampf(c[q].y / (h + 1), 0, 1);
                glColor3f(base[0] + 0.14f * m, base[1] + 0.11f * m, base[2] + 0.13f * m);
                glVertex3f(c[q].x, c[q].y, c[q].z);
            }
        }
        glEnd();
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);   /* fenetres allumees et feu rouge au sommet */
        glPointSize(2);
        glBegin(GL_POINTS);
        V3 in = vnorm(v3(-cx, 0, -cz));
        for (int q = 0; q < (int)(h * 0.35f); q++) {
            float u = tnoise(&s) * 2 - 1, y = 4 + tnoise(&s) * (h - 8), warm = tnoise(&s);
            V3 side = v3(-in.z, 0, in.x), p = vadd(v3(cx, y, cz), vadd(vmul(in, fmaxf(w, d) + 0.3f), vmul(side, u * w * 0.9f)));
            if (warm < 0.7f) glColor4f(1.0f, 0.78f, 0.45f, gxCityWin); else glColor4f(0.45f, 0.85f, 1.0f, gxCityWin * 1.15f);
            glVertex3f(p.x, p.y, p.z);
        }
        glColor4f(1, 0.2f, 0.2f, 0.9f); glVertex3f(cx, h + 1, cz);
        glEnd();
        glDisable(GL_BLEND);
    }
    glEnable(GL_FOG);
    glEndList();
}

/* ombres des objets mobiles : silhouettes aplaties sur le dessus de la boite qui les recoit, selon le soleil.
 * Le stencil garantit qu'un pixel n'est assombri qu'une fois (pas de double ombre entre deux boites). */
static void shadow_caster_boxes(V3 pos, float yaw, float h, float r, int round) {
    V3 L = sun_dir(), o;
    o = vadd(pos, v3(0, 0.15f, 0));
    float t = ray_world(o, vmul(L, -1), 60, 0);
    if (t >= 60 || rayBox < 0 || boxes[rayBox].mx.y > pos.y + 0.2f) return;
    Box *b = &boxes[rayBox];
    float gh = b->mx.y, alt = pos.y - gh, a = gxShadowAlpha * clampf(1 - alt / gxShadowMaxH, 0, 1);
    if (a <= 0.02f || sun_vis(vadd(pos, v3(0, h * 0.6f, 0))) < 0.5f) return;
    double cp[4][4] = { { 1, 0, 0, -b->mn.x }, { -1, 0, 0, b->mx.x }, { 0, 0, 1, -b->mn.z }, { 0, 0, -1, b->mx.z } };
    for (int k = 0; k < 4; k++) { glClipPlane(GL_CLIP_PLANE0 + k, cp[k]); glEnable(GL_CLIP_PLANE0 + k); }
    float kx = L.x / L.y, kz = L.z / L.y;                 /* projection sur le plan y = gh, le long de L */
    GLfloat M[16] = { 1, 0, 0, 0, -kx, 0, -kz, 0, 0, 0, 1, 0, kx * gh, gh + 0.03f, kz * gh, 1 };
    glColor4f(gxShadowCol[0], gxShadowCol[1], gxShadowCol[2], a);
    glPushMatrix();
    glMultMatrixf(M);
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(-yaw * 180 / PI, 0, 1, 0);
    if (round) {
        box_faces(v3(-r, -r * 0.7f, -r), v3(r, r * 0.7f, r));
        box_faces(v3(-r * 0.7f, -r, -r * 0.7f), v3(r * 0.7f, r, r * 0.7f));
    } else {
        box_faces(v3(-0.36f, 0, -0.24f), v3(0.36f, h - 0.48f, 0.24f));
        box_faces(v3(-0.23f, h - 0.46f, -0.23f), v3(0.23f, h, 0.23f));
        box_faces(v3(0.25f, h * 0.45f, -0.75f), v3(0.4f, h * 0.55f, 0.0f));   /* bras et arme */
    }
    glPopMatrix();
    for (int k = 0; k < 4; k++) glDisable(GL_CLIP_PLANE0 + k);
}

static void draw_dyn_shadows(void) {
    if (!gfxShadows) return;
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glDisable(GL_FOG); glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(-1, -2);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    if (appState == APP_GAME && !P.dead && !spectating) shadow_caster_boxes(P.pos, P.yaw, pheight(), 0, 0);
    for (int i = 0; dmMode && i < MAXP; i++)
        if (np_present(i)) shadow_caster_boxes(NP[i].disp, NP[i].st.yaw, np_height(i), 0, 0);
    for (int i = 0; i < MAXE; i++)
        if (E[i].alive) shadow_caster_boxes(E[i].pos, 0, 0, enemy_radius(E[i].type), 1);
    for (int i = 0; i < npu; i++)
        if (PU[i].t <= 0) shadow_caster_boxes(vadd(PU[i].pos, v3(0, 0.15f, 0)), (float)nowT * 1.5f + i, 0, 0.3f, 1);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_FOG);
}

/* ---- bloom : halo autour de ce qui est tres lumineux (copie d'ecran, seuil par carre, flou, ajout) ---- */
static GLuint bloomScene, bloomA, bloomB;
static int bloomTW, bloomTH, bloomSW, bloomSH;            /* tailles des textures (puissances de 2) */
static void bloom_tex(GLuint *t, int w, int h) {
    if (!*t) glGenTextures(1, t);
    glBindTexture(GL_TEXTURE_2D, *t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}
static void bloom_quad(float x, float y, float w, float h, float s1, float t1) {   /* repere : pixels, origine en bas */
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y);
    glTexCoord2f(s1, 0); glVertex2f(x + w, y);
    glTexCoord2f(s1, t1); glVertex2f(x + w, y + h);
    glTexCoord2f(0, t1); glVertex2f(x, y + h);
    glEnd();
}
static void bloom_blur(GLuint src, GLuint dst, int horiz, int sw, int sh) {
    static const float wgt[5] = { 0.07f, 0.24f, 0.38f, 0.24f, 0.07f };
    float s1 = (float)sw / bloomSW, t1 = (float)sh / bloomSH;
    glDisable(GL_BLEND);
    glColor3f(0, 0, 0);
    glDisable(GL_TEXTURE_2D);
    bloom_quad(0, 0, (float)sw, (float)sh, 0, 0);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    glBindTexture(GL_TEXTURE_2D, src);
    for (int k = 0; k < 5; k++) {
        float o = (k - 2) * gxBloom[3];
        glColor3f(wgt[k], wgt[k], wgt[k]);
        bloom_quad(horiz ? o : 0, horiz ? 0 : o, (float)sw, (float)sh, s1, t1);
    }
    glBindTexture(GL_TEXTURE_2D, dst);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sw, sh);
}
/* resolution de la 3D (PARAMETRES > RESOLUTION 3D) : la scene a ete dessinee dans le coin bas gauche, en sw x sh ;
   on la copie dans une texture et on l'etire sur toute la fenetre (filtre lineaire). Le HUD est dessine ensuite, net. */
static GLuint scaleTex;
static int scaleTW, scaleTH;
static void render_scale_apply(int sw, int sh) {
    static int npot = -1;                                 /* textures de taille quelconque (OpenGL 2.0) : pas d'arrondi a 2^n */
    if (npot < 0) { const char *e = (const char *)glGetString(GL_EXTENSIONS); npot = e && strstr(e, "GL_ARB_texture_non_power_of_two") != NULL; }
    int tw = npot ? sw : pow2up(sw), th = npot ? sh : pow2up(sh);
    if (tw != scaleTW || th != scaleTH) {
        scaleTW = tw; scaleTH = th;
        bloom_tex(&scaleTex, scaleTW, scaleTH);
    }
    glViewport(0, 0, winW, winH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, winW, 0, winH, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, scaleTex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sw, sh);
    glColor3f(1, 1, 1);
    bloom_quad(0, 0, (float)winW, (float)winH, (float)sw / scaleTW, (float)sh / scaleTH);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
}

static void bloom_apply(void) {
    if (!gfxBloom || winW < 64 || winH < 64) return;
    int sw = winW / 4, sh = winH / 4;
    if (pow2up(winW) != bloomTW || pow2up(winH) != bloomTH) {
        bloomTW = pow2up(winW); bloomTH = pow2up(winH); bloomSW = pow2up(sw); bloomSH = pow2up(sh);
        bloom_tex(&bloomScene, bloomTW, bloomTH);
        bloom_tex(&bloomA, bloomSW, bloomSH);
        bloom_tex(&bloomB, bloomSW, bloomSH);
    }
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, winW, 0, winH, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_FOG); glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bloomScene);             /* 1. l'image complete */
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, winW, winH);
    float ss = (float)winW / bloomTW, st = (float)winH / bloomTH;
    glDisable(GL_BLEND);                                  /* 2. reduite au quart, puis seuil doux : x^2, puis x^4, x^8 */
    glColor3f(1, 1, 1);
    bloom_quad(0, 0, (float)sw, (float)sh, ss, st);
    glEnable(GL_BLEND); glBlendFunc(GL_DST_COLOR, GL_ZERO);
    bloom_quad(0, 0, (float)sw, (float)sh, ss, st);
    glBindTexture(GL_TEXTURE_2D, bloomA);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sw, sh);
    for (int q = 1; q < (int)gxBloom[1]; q++) {          /* seuil : x^2, x^4, x^8... (bloom, 2e valeur) */
        bloom_quad(0, 0, (float)sw, (float)sh, (float)sw / bloomSW, (float)sh / bloomSH);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sw, sh);
    }
    for (int it = 0; it < (int)gxBloom[2]; it++) {       /* 3. flou separable (bloom, 3e valeur) */
        bloom_blur(bloomA, bloomB, 1, sw, sh);
        bloom_blur(bloomB, bloomA, 0, sw, sh);
    }
    glDisable(GL_BLEND);                                  /* 4. on remet l'image, puis le halo par-dessus */
    glColor3f(1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, bloomScene);
    bloom_quad(0, 0, (float)winW, (float)winH, ss, st);
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    glColor3f(gxBloom[0], gxBloom[0], gxBloom[0]);
    glBindTexture(GL_TEXTURE_2D, bloomA);
    bloom_quad(0, 0, (float)winW, (float)winH, (float)sw / bloomSW, (float)sh / bloomSH);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
}

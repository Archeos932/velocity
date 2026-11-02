/* VELOCITY - dessin du monde : secteurs (seul ce qui est dans le champ de vision est dessine)
   et passe unique en multitexture (texture x lightmap x grain), avec repli en plusieurs passes.
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ---- multitexture (OpenGL 1.3 / ARB_multitexture + ARB_texture_env_combine) -------------------
 * Avant : 4 passes sur tout le monde (sol, murs, lightmap en melange x2, grain en melange).
 * Maintenant, si la carte sait faire : une seule passe, unite 0 = texture du sol ou du mur (x couleur de la boite),
 * unite 1 = lightmap (COMBINE MODULATE, echelle 2 : 0,5 = inchange), unite 2 = grain (s'il y a une 3e unite).
 * Sans multitexture (rendu logiciel de Windows, GDI Generic), ou avec --multipasse : les passes d'avant. */
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif
#ifndef GL_MAX_TEXTURE_UNITS
#define GL_MAX_TEXTURE_UNITS 0x84E2
#endif
#ifndef GL_COMBINE
#define GL_COMBINE 0x8570
#endif
#ifndef GL_COMBINE_RGB
#define GL_COMBINE_RGB 0x8571
#endif
#ifndef GL_RGB_SCALE
#define GL_RGB_SCALE 0x8573
#endif
#ifndef GL_PREVIOUS
#define GL_PREVIOUS 0x8578
#endif
#ifndef GL_SOURCE0_RGB
#define GL_SOURCE0_RGB 0x8580
#endif
#ifndef GL_SOURCE1_RGB
#define GL_SOURCE1_RGB 0x8581
#endif
#ifndef GL_OPERAND0_RGB
#define GL_OPERAND0_RGB 0x8590
#endif
#ifndef GL_OPERAND1_RGB
#define GL_OPERAND1_RGB 0x8591
#endif
typedef void(WINAPI *MtActiveFn)(GLenum);
typedef void(WINAPI *MtCoordFn)(GLenum, GLfloat, GLfloat);
static MtActiveFn mtActive;
static MtCoordFn mtCoord;
static int mtUnits;                                       /* unites de texture utilisables (0 : pas de multitexture) */
static int worldForceMulti;                               /* --multipasse : force les passes d'avant (comparaison, tests) */

static void world_gl_caps(void) {
    static int done;
    if (done) return;
    done = 1;
    mtUnits = 0;
    if (worldForceMulti) return;
    const char *e = (const char *)glGetString(GL_EXTENSIONS), *v = (const char *)glGetString(GL_VERSION);
    int major = v ? atoi(v) : 1, minor = v && strchr(v, '.') ? atoi(strchr(v, '.') + 1) : 0;
    int gl13 = major > 1 || (major == 1 && minor >= 3);
    if (!gl13 && !(e && strstr(e, "GL_ARB_multitexture") && strstr(e, "GL_ARB_texture_env_combine"))) return;
#ifdef _WIN32
    mtActive = (MtActiveFn)wglGetProcAddress("glActiveTextureARB");
    mtCoord = (MtCoordFn)wglGetProcAddress("glMultiTexCoord2fARB");
#else
    mtActive = glActiveTexture;
    mtCoord = glMultiTexCoord2f;
#endif
    if (!mtActive || !mtCoord) return;
    GLint u = 1;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &u);
    mtUnits = u >= 2 ? u : 0;
}

/* ---- secteurs ------------------------------------------------------------------------------------
 * Le monde est coupe en cases de WORLD_SECTOR_M (au plus WSEC_N x WSEC_N) ; chaque face, liseré et triangle
 * de decor va dans la case de son centre. Une case = WL_N listes d'affichage et la boite qui les englobe ;
 * a chaque image, seules les cases dans la pyramide de vision sont appelees. */
#define WSEC_N 16
enum { WL_BASE, WL_LM, WL_GRAIN, WL_EDGES, WL_DECOR, WL_N };   /* passe unique : tout dans WL_BASE (+ WL_GRAIN si 2 unites) */
typedef struct { V3 mn, mx; GLuint list; int used; } WSec;
static WSec wsec[WSEC_N * WSEC_N];
static int wsGX, wsGZ, worldFused, worldGrainUnit, worldGrainPass, worldNoTex;
static float wsX0, wsZ0, wsCell;
static int worldSecDrawn, worldSecTotal, worldTris;     /* ... ; triangles du decor maille dans les listes */                  /* secteurs dessines / non vides a la derniere image (banc) */

static int wsec_of(float x, float z) {
    int gx = (int)floorf((x - wsX0) / wsCell), gz = (int)floorf((z - wsZ0) / wsCell);
    gx = gx < 0 ? 0 : gx >= wsGX ? wsGX - 1 : gx;
    gz = gz < 0 ? 0 : gz >= wsGZ ? wsGZ - 1 : gz;
    return gz * wsGX + gx;
}
static void wsec_grow(int s, V3 a, V3 b) {
    WSec *w = &wsec[s];
    if (!w->used) { w->mn = a; w->mx = b; w->used = 1; return; }
    w->mn = v3(fminf(w->mn.x, a.x), fminf(w->mn.y, a.y), fminf(w->mn.z, a.z));
    w->mx = v3(fmaxf(w->mx.x, b.x), fmaxf(w->mx.y, b.y), fmaxf(w->mx.z, b.z));
}

static void world_free(void) {                            /* listes et lightmaps de la carte precedente */
    for (int s = 0; s < WSEC_N * WSEC_N; s++) {
        if (wsec[s].list) glDeleteLists(wsec[s].list, WL_N);
        wsec[s].list = 0; wsec[s].used = 0;
    }
    lm_free();
}

/* coordonnees de texture d'un point d'une face (comme quad_tex) */
static void face_uv(V3 n, V3 p, float scale, float *u, float *v) {
    if (fabsf(n.x) > 0.5f) { *u = p.z; *v = p.y; }
    else if (fabsf(n.y) > 0.5f) { *u = p.x; *v = p.z; }
    else { *u = p.x; *v = p.y; }
    *u *= scale; *v *= scale;
}
/* une face en passe unique : texture de base, lightmap (atlas), grain */
static void face_fused(int bi, int f, float scale) {
    Box *b = &boxes[bi];
    LMap *m = &lmaps[bi][f];
    V3 c[4], n = face_normal(f);
    int ua = face_u(f), va = face_v(f);
    float u0 = *vc(&b->mn, ua), du = *vc(&b->mx, ua) - u0, v0 = *vc(&b->mn, va), dv = *vc(&b->mx, va) - v0;
    face_corners(b->mn, b->mx, f, c);
    for (int k = 0; k < 4; k++) {
        float u, v;
        face_uv(n, c[k], scale, &u, &v);
        mtCoord(GL_TEXTURE0, u, v);
        mtCoord(GL_TEXTURE1, m->u0 + (*vc(&c[k], ua) - u0) / du * m->su, m->v0 + (*vc(&c[k], va) - v0) / dv * m->sv);
        if (worldGrainUnit) { face_uv(n, c[k], gxTexScale[2], &u, &v); mtCoord(GL_TEXTURE2, u, v); }
        glVertex3f(c[k].x, c[k].y, c[k].z);
    }
}

/* faces triees par secteur, puis sol/murs, puis page de lightmap : peu de changements de texture */
static int wfKey[MAXB * 6];
static int wf_cmp(const void *a, const void *b) {
    int ka = wfKey[*(const int *)a], kb = wfKey[*(const int *)b];
    return ka < kb ? -1 : ka > kb;
}

static void build_world_list(void) {
    world_gl_caps();
    world_free();
    bake_lightmaps();
    decor_bake();                                         /* decor maille : eclairage par sommet */
    int stone = maps[curMap].style == 1;                  /* pierre et enduit (LA CITADELLE) ou metal */
    GLuint gtex[2] = { stone ? texStone : texGrid, stone ? texPlaster : texWall };
    float gsc[2] = { stone ? gxStoneTex[2] : gxTexScale[0], stone ? gxPlasterTex[2] : gxTexScale[1] };
    worldFused = mtUnits >= 2;
    int grain = gxGrain > 0.001f;
    worldGrainUnit = worldFused && grain && mtUnits >= 3;
    worldGrainPass = grain && !worldGrainUnit;
    worldNoTex = gxWorldTex < 0.5f;                       /* GRILLE-PAIN : couleur x moyenne de la texture, sans la texture */
    float gmean[2][3];
    for (int g = 0; g < 2; g++) {
        if (worldNoTex) tex_mean(gtex[g], gmean[g]);
        else gmean[g][0] = gmean[g][1] = gmean[g][2] = 1;
    }

    /* 1. etendue du monde et taille des cases */
    static int wfId[MAXB * 6], wfSec[MAXB * 6];
    int nf = 0, ntri = ndv / 3;
    float x0 = 1e9f, x1 = -1e9f, z0 = 1e9f, z1 = -1e9f;
    for (int i = 0; i < nb; i++) {
        if (boxes[i].kind == B_HIDDEN) continue;
        V3 c = vlerp(boxes[i].mn, boxes[i].mx, 0.5f);
        x0 = fminf(x0, c.x); x1 = fmaxf(x1, c.x); z0 = fminf(z0, c.z); z1 = fmaxf(z1, c.z);
    }
    for (int t = 0; t < ntri; t++) {
        V3 c = vmul(vadd(vadd(DV[t * 3].p, DV[t * 3 + 1].p), DV[t * 3 + 2].p), 1.0f / 3);
        x0 = fminf(x0, c.x); x1 = fmaxf(x1, c.x); z0 = fminf(z0, c.z); z1 = fmaxf(z1, c.z);
    }
    if (x1 < x0) { x0 = z0 = -1; x1 = z1 = 1; }
    wsCell = fmaxf(WORLD_SECTOR_M, fmaxf(x1 - x0, z1 - z0) / WSEC_N + 0.01f);
    wsX0 = x0; wsZ0 = z0;
    wsGX = (int)((x1 - x0) / wsCell) + 1; wsGZ = (int)((z1 - z0) / wsCell) + 1;
    wsGX = wsGX > WSEC_N ? WSEC_N : wsGX; wsGZ = wsGZ > WSEC_N ? WSEC_N : wsGZ;

    /* 2. faces visibles : secteur, puis cle de tri */
    for (int i = 0; i < nb; i++)
        for (int f = 0; f < 6; f++) {
            if (!lmaps[i][f].on) continue;
            V3 c[4];
            face_corners(boxes[i].mn, boxes[i].mx, f, c);
            V3 mid = vmul(vadd(vadd(c[0], c[1]), vadd(c[2], c[3])), 0.25f);
            int s = wsec_of(mid.x, mid.z), g = f / 2 == 1 ? 0 : 1;
            wsec_grow(s, v3(fminf(c[0].x, c[2].x), fminf(c[0].y, c[2].y), fminf(c[0].z, c[2].z)),   /* c[0], c[2] : coins */
                      v3(fmaxf(c[0].x, c[2].x), fmaxf(c[0].y, c[2].y), fmaxf(c[0].z, c[2].z)));        /* opposes de la face */
            wfId[nf] = i * 6 + f; wfSec[nf] = s;
            wfKey[nf] = (s * 2 + g) * LM_PAGES + lmaps[i][f].page;
            nf++;
        }
    static int ord[MAXB * 6];
    for (int k = 0; k < nf; k++) ord[k] = k;
    qsort(ord, nf, sizeof ord[0], wf_cmp);
    int *tsec = ntri ? (int *)malloc(sizeof(int) * ntri) : NULL;
    int lodSkip = gxDecorDetail >= 0.5f ? DEC_SIMPLE : DEC_DETAIL;   /* GRILLE-PAIN : versions simples du decor */
    worldTris = 0;
    for (int t = 0; t < ntri; t++) {
        if (DVlod[t * 3] == lodSkip) { tsec[t] = -1; continue; }
        worldTris++;
        V3 a = DV[t * 3].p, b = DV[t * 3 + 1].p, c = DV[t * 3 + 2].p;
        V3 m = vmul(vadd(vadd(a, b), c), 1.0f / 3);
        tsec[t] = wsec_of(m.x, m.z);
        wsec_grow(tsec[t], v3(fminf(a.x, fminf(b.x, c.x)), fminf(a.y, fminf(b.y, c.y)), fminf(a.z, fminf(b.z, c.z))),
                  v3(fmaxf(a.x, fmaxf(b.x, c.x)), fmaxf(a.y, fmaxf(b.y, c.y)), fmaxf(a.z, fmaxf(b.z, c.z))));
    }
    for (int i = 1; i < nb; i++)                          /* liseres : la boite entiere compte dans son secteur */
        if (boxes[i].kind != B_HIDDEN) {
            V3 c = vlerp(boxes[i].mn, boxes[i].mx, 0.5f);
            if (wsec[wsec_of(c.x, c.z)].used) wsec_grow(wsec_of(c.x, c.z), boxes[i].mn, boxes[i].mx);
        }

    /* 3. listes de chaque secteur */
    int k0 = 0;
    worldSecTotal = 0;
    for (int s = 0; s < wsGX * wsGZ; s++) {
        int k1 = k0;
        while (k1 < nf && wfSec[ord[k1]] == s) k1++;
        WSec *w = &wsec[s];
        if (!w->used) { k0 = k1; continue; }
        worldSecTotal++;
        w->list = glGenLists(WL_N);
        glNewList(w->list + WL_BASE, GL_COMPILE);         /* texture de base (+ lightmap et grain en passe unique) */
        for (int k = k0; k < k1;) {
            int q = ord[k], bi = wfId[q] / 6, f = wfId[q] % 6, g = f / 2 == 1 ? 0 : 1, pg = lmaps[bi][f].page;
            int ke = k;
            while (ke < k1 && wfKey[ord[ke]] == wfKey[q]) ke++;   /* meme groupe et meme page */
            if (worldFused) { mtActive(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, lmPage[pg]); mtActive(GL_TEXTURE0); }
            glBindTexture(GL_TEXTURE_2D, gtex[g]);
            glBegin(GL_QUADS);
            for (; k < ke; k++) {
                int qq = ord[k], b2 = wfId[qq] / 6, f2 = wfId[qq] % 6;
                glColor3f(boxes[b2].col.x * gmean[g][0], boxes[b2].col.y * gmean[g][1], boxes[b2].col.z * gmean[g][2]);
                if (worldFused) face_fused(b2, f2, gsc[g]);
                else {
                    V3 c[4];
                    face_corners(boxes[b2].mn, boxes[b2].mx, f2, c);
                    quad_tex(face_normal(f2), c[0], c[1], c[2], c[3], gsc[g]);
                }
            }
            glEnd();
        }
        glEndList();
        glNewList(w->list + WL_LM, GL_COMPILE);           /* passes d'avant : lightmaps (melange x2) */
        if (!worldFused) {
            glColor3f(1, 1, 1);
            for (int k = k0; k < k1;) {
                int pg = lmaps[wfId[ord[k]] / 6][wfId[ord[k]] % 6].page, ke = k;
                while (ke < k1 && lmaps[wfId[ord[ke]] / 6][wfId[ord[ke]] % 6].page == pg) ke++;
                glBindTexture(GL_TEXTURE_2D, lmPage[pg]);
                glBegin(GL_QUADS);
                for (; k < ke; k++) lm_face(wfId[ord[k]] / 6, wfId[ord[k]] % 6);
                glEnd();
            }
        }
        glEndList();
        glNewList(w->list + WL_GRAIN, GL_COMPILE);        /* grain fin (passe a part s'il n'a pas son unite) */
        if (worldGrainPass) {
            glColor3f(1, 1, 1);
            glBindTexture(GL_TEXTURE_2D, texGrime);
            glBegin(GL_QUADS);
            for (int k = k0; k < k1; k++) {
                int bi = wfId[ord[k]] / 6, f = wfId[ord[k]] % 6;
                V3 c[4];
                face_corners(boxes[bi].mn, boxes[bi].mx, f, c);
                quad_tex(face_normal(f), c[0], c[1], c[2], c[3], gxTexScale[2]);
            }
            glEnd();
        }
        glEndList();
        glNewList(w->list + WL_EDGES, GL_COMPILE);        /* liseres des blocs, pads et bandes d'acceleration */
        for (int i = 1; i < nb; i++) {
            if (boxes[i].kind == B_HIDDEN) continue;
            V3 c = vlerp(boxes[i].mn, boxes[i].mx, 0.5f);
            if (wsec_of(c.x, c.z) != s) continue;
            if (boxes[i].kind == B_PAD) glColor4f(0.6f, 1.0f, 1.0f, 0.9f);
            else if (boxes[i].kind == B_SPEED) glColor4f(1.0f, 0.5f, 0.9f, 0.9f);
            else if (stone) continue;                     /* pierre : pas de liseres sombres, les aretes viennent de la lumiere */
            else if (gxEdges > 0.001f) glColor4f(0.05f, 0.05f, 0.1f, gxEdges);
            else continue;
            box_edges(boxes[i].mn, boxes[i].mx);
        }
        glEndList();
        glNewList(w->list + WL_DECOR, GL_COMPILE);        /* decor maille, deja eclaire */
        glBegin(GL_TRIANGLES);
        for (int t = 0; t < ntri; t++) {
            if (tsec[t] != s) continue;
            for (int j = t * 3; j < t * 3 + 3; j++) {
                glColor3f(clampf(DV[j].c.x * DVL[j][0], 0, 1), clampf(DV[j].c.y * DVL[j][1], 0, 1), clampf(DV[j].c.z * DVL[j][2], 0, 1));
                glVertex3f(DV[j].p.x, DV[j].p.y, DV[j].p.z);
            }
        }
        glEnd();
        glEndList();
        k0 = k1;
    }
    free(tsec);
    build_skyline_list();
    build_sky_list();                                     /* le degrade du ciel depend de la carte et de rendu.ini */
}

/* pyramide de vision (matrices de la camera, mvMat/prMat) : une boite entierement dehors n'est pas dessinee */
static float wPlane[6][4];
static void world_frustum(void) {
    float m[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            m[c * 4 + r] = prMat[r] * mvMat[c * 4] + prMat[4 + r] * mvMat[c * 4 + 1] + prMat[8 + r] * mvMat[c * 4 + 2] + prMat[12 + r] * mvMat[c * 4 + 3];
    for (int i = 0; i < 6; i++) {
        int r = i / 2;
        float sg = (i & 1) ? -1.0f : 1.0f;
        for (int k = 0; k < 4; k++) wPlane[i][k] = m[k * 4 + 3] + sg * m[k * 4 + r];
    }
}
static int world_box_visible(V3 mn, V3 mx) {
    for (int i = 0; i < 6; i++) {
        const float *p = wPlane[i];
        float x = p[0] >= 0 ? mx.x : mn.x, y = p[1] >= 0 ? mx.y : mn.y, z = p[2] >= 0 ? mx.z : mn.z;
        if (p[0] * x + p[1] * y + p[2] * z + p[3] < 0) return 0;
    }
    return 1;
}

static void world_calls(int kind, const unsigned char *vis) {
    for (int s = 0; s < wsGX * wsGZ; s++)
        if (vis[s]) glCallList(wsec[s].list + kind);
}

/* le monde : appele par render() a la place de l'ancienne liste unique */
static void world_draw(void) {
    unsigned char vis[WSEC_N * WSEC_N];
    world_frustum();
    worldSecDrawn = 0;
    for (int s = 0; s < wsGX * wsGZ; s++) {
        vis[s] = wsec[s].used && wsec[s].list && world_box_visible(wsec[s].mn, wsec[s].mx);
        worldSecDrawn += vis[s];
    }
    const float *fgc = map_fog_col();
    GLfloat fogScene[4] = { fgc[0], fgc[1], fgc[2], 1 }, fogGray[4] = { 0.5f, 0.5f, 0.5f, 1 }, fogWhite[4] = { 1, 1, 1, 1 };
    glEnable(GL_CULL_FACE);                               /* normales sortantes : l'interieur des boites n'est pas dessine */
    glCullFace(GL_BACK);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1, 1);
    glDisable(GL_LIGHTING);                               /* l'eclairage vient des lightmaps */
    glEnable(GL_TEXTURE_2D);
    if (worldFused) {
        mtActive(GL_TEXTURE1);                            /* unite 1 : x lightmap x 2 */
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
        glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f);
        if (worldGrainUnit) {                             /* unite 2 : x grain */
            mtActive(GL_TEXTURE2);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, texGrime);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
        mtActive(GL_TEXTURE0);
        if (worldNoTex) glDisable(GL_TEXTURE_2D);         /* unite 0 coupee : la lightmap multiplie la couleur */
        world_calls(WL_BASE, vis);
        if (worldGrainUnit) { mtActive(GL_TEXTURE2); glDisable(GL_TEXTURE_2D); }
        mtActive(GL_TEXTURE1);
        glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDisable(GL_TEXTURE_2D);
        mtActive(GL_TEXTURE0);
    } else {
        if (worldNoTex) glDisable(GL_TEXTURE_2D);
        world_calls(WL_BASE, vis);                        /* passe 1 : sol et murs */
        glEnable(GL_TEXTURE_2D);
        glDepthFunc(GL_LEQUAL);                           /* passes suivantes : meme profondeur */
        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);          /* passe 2 : lightmaps, x2 (0,5 = inchange) */
        glFogfv(GL_FOG_COLOR, fogGray);                   /* au loin, la lightmap s'efface dans le brouillard */
        world_calls(WL_LM, vis);
    }
    if (worldGrainPass) {                                 /* grain fin, multiplicatif, s'il n'a pas eu son unite */
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        glFogfv(GL_FOG_COLOR, fogWhite);
        world_calls(WL_GRAIN, vis);
    }
    glFogfv(GL_FOG_COLOR, fogScene);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);                                   /* liseres */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5f);
    world_calls(WL_EDGES, vis);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    world_calls(WL_DECOR, vis);                           /* decor maille (deux faces) */
}

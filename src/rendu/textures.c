/* VELOCITY - textures (et .bmp perso), primitives de boites
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* BMP 24 ou 32 bits non compresse, carre, cote puissance de 2 (16 a 1024) : image RGB de haut en bas, ou NULL */
static unsigned char *load_bmp_square(const char *path, int *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    unsigned char h[54];
    unsigned char *img = NULL;
    if (fread(h, 1, 54, f) == 54 && h[0] == 'B' && h[1] == 'M') {
        int off = h[10] | h[11] << 8 | h[12] << 16 | h[13] << 24;
        int w = h[18] | h[19] << 8 | h[20] << 16 | h[21] << 24, hh = h[22] | h[23] << 8 | h[24] << 16 | h[25] << 24;
        int bpp = h[28] | h[29] << 8, comp = h[30] | h[31] << 8 | h[32] << 16 | h[33] << 24;
        int topDown = hh < 0;
        if (topDown) hh = -hh;
        if ((bpp == 24 || bpp == 32) && (comp == 0 || comp == 3) && w == hh && w >= 16 && w <= 1024 && !(w & (w - 1))) {
            int bytes = bpp / 8, stride = (w * bytes + 3) & ~3;
            unsigned char *row = (unsigned char *)malloc((size_t)stride);
            img = (unsigned char *)malloc((size_t)w * w * 3);
            fseek(f, off, SEEK_SET);
            for (int y = 0; y < w && img; y++) {
                if (fread(row, 1, (size_t)stride, f) != (size_t)stride) { free(img); img = NULL; break; }
                int dy = topDown ? y : w - 1 - y;
                for (int x = 0; x < w; x++) {
                    img[((size_t)dy * w + x) * 3 + 0] = row[x * bytes + 2];
                    img[((size_t)dy * w + x) * 3 + 1] = row[x * bytes + 1];
                    img[((size_t)dy * w + x) * 3 + 2] = row[x * bytes + 0];
                }
            }
            free(row);
            *size = w;
        }
    }
    fclose(f);
    return img;
}

/* envoie une image RGB a OpenGL avec des mipmaps faits main (filtre boite) ; modifie/consomme img */
static void upload_rgb_mip(GLuint *tex, unsigned char *img, int N) {
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    for (int level = 0, s = N; ; level++) {                /* mipmaps faits main */
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGB, s, s, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
        if (s == 1) break;
        int h = s / 2;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < h; x++)
                for (int c = 0; c < 3; c++) {
                    int sum = img[((2 * y) * s + 2 * x) * 3 + c] + img[((2 * y) * s + 2 * x + 1) * 3 + c] +
                              img[((2 * y + 1) * s + 2 * x) * 3 + c] + img[((2 * y + 1) * s + 2 * x + 1) * 3 + c];
                    img[(y * h + x) * 3 + c] = (unsigned char)(sum / 4);
                }
        s = h;
    }
}

/* texture remplacee par une image du dossier graphismes (sol.bmp, mur.bmp, grain.bmp) ? */
static int tex_override(GLuint *tex, const char *name) {
    char p[600];
    int n = 0;
    if (!gfxDir[0]) return 0;
    snprintf(p, sizeof p, "%s/%s", gfxDir, name);
    unsigned char *img = load_bmp_square(p, &n);
    if (!img) return 0;
    upload_rgb_mip(tex, img, n);
    free(img);
    return 1;
}

/* ---- textures generees ----------------------------------------------------------------------------------
 * Chaque generateur calcule une image RGB (de haut en bas, malloc) sur le processeur, sans OpenGL, a partir
 * des reglages de rendu.ini (sol_texture, mur_texture, grain_force) ; make_textures l'envoie
 * telle quelle (upload_rgb_mip), --export-textures l'ecrit en .bmp sous le nom que tex_override cherche.
 * Rien ne depend de la carte, de l'ambiance ni de la qualite : une seule image par texture. */
/* --- sol/plafond : dalles biseautees, boulons, taches d'usure ---------------------- */
static unsigned char *tex_gen_sol(int *size) {
    unsigned int s;
    int N = 256, panel = (int)gxFloorTex[0];
    unsigned char *img = (unsigned char *)malloc(N * N * 3);
    float stx[10], sty[10], str[10], stp[10];
    s = 0xA17Eu;
    for (int i = 0; i < 10; i++) {
        stx[i] = tnoise(&s) * N; sty[i] = tnoise(&s) * N;
        str[i] = 10 + tnoise(&s) * 22; stp[i] = 0.08f + tnoise(&s) * 0.14f;
    }
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            int px = x % panel, py = y % panel;
            float dx = (float)(px < panel - px ? px : panel - px), dy = (float)(py < panel - py ? py : panel - py);
            float v = gxFloorTex[1];
            v += sinf(x * 0.045f + 1.3f) * cosf(y * 0.037f + 0.7f) * 0.05f;       /* grain large */
            v += sinf(x * 0.21f + py * 0.3f) * cosf(y * 0.17f) * 0.015f;          /* ondulation fine */
            v += (tnoise(&s) * 2 - 1) * 0.035f;                                  /* grain fin */
            /* joint biseaute en degrade (pas de marches nettes = moins "dessine") */
            v -= 0.15f * fmaxf(expf(-dx * dx / 1.3f), expf(-dy * dy / 1.3f));            /* fond du joint */
            v += 0.08f * fmaxf(expf(-(dx - 2.4f) * (dx - 2.4f) / 1.1f),
                                expf(-(dy - 2.4f) * (dy - 2.4f) / 1.1f));                  /* arete eclairee */
            {                                                                     /* un boulon par dalle, en degrade */
                float bx = (float)px - 9, by = (float)py - 9, br2 = bx * bx + by * by;
                v -= 0.12f * expf(-br2 / 5.0f);
                v += 0.17f * expf(-br2 / 0.6f);
            }
            for (int i = 0; i < 10; i++) {                                       /* taches, repetition sans couture */
                float ddx = x - stx[i], ddy = y - sty[i];
                if (fabsf(ddx) > N * 0.5f) ddx -= (ddx > 0 ? (float)N : -(float)N);
                if (fabsf(ddy) > N * 0.5f) ddy -= (ddy > 0 ? (float)N : -(float)N);
                float d = sqrtf(ddx * ddx + ddy * ddy);
                if (d < str[i]) { float k = 1 - d / str[i]; v -= stp[i] * k * k; }
            }
            float warm = sinf(x * 0.013f + y * 0.021f + 4.0f) * 0.02f;   /* teinte minerale tres legere */
            img[(y * N + x) * 3 + 0] = (unsigned char)(clampf(v + warm, 0, 1) * 255);
            img[(y * N + x) * 3 + 1] = (unsigned char)(clampf(v, 0, 1) * 255);
            img[(y * N + x) * 3 + 2] = (unsigned char)(clampf(v - warm, 0, 1) * 255);
        }
    *size = N;
    return img;
}

/* --- murs : plaques metalliques boulonnees, brossage vertical, haut de plaque eclaire --- */
static unsigned char *tex_gen_mur(int *size) {
    unsigned int s;
    int N = 256, panelX = (int)gxWallTex[0], panelY = (int)gxWallTex[1];
    unsigned char *img = (unsigned char *)malloc(N * N * 3);
    float colTint[256];
    s = 0x5EEDu;
    for (int x = 0; x < N; x++) colTint[x] = (tnoise(&s) * 2 - 1) * 0.03f;        /* stries de brossage */
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            int panelI = x / panelX, panelJ = y / panelY;
            unsigned int ph = (unsigned int)(panelI * 97 + panelJ * 193 + 71);
            ph = ph * 1664525u + 1013904223u;
            float tint = ((float)(ph >> 8) * (1.0f / 16777216.0f) * 2 - 1) * 0.045f;  /* teinte par plaque */
            int px = x % panelX, py = y % panelY;
            float dx = (float)(px < panelX - px ? px : panelX - px), dy = (float)(py < panelY - py ? py : panelY - py);
            float v = gxWallTex[2] + tint + colTint[x];
            /* joint en degrade, comme le sol : evite l'effet "trait d'encre" */
            v -= 0.14f * fmaxf(expf(-dx * dx / 1.3f), expf(-dy * dy / 1.3f));
            v += 0.09f * fmaxf(expf(-(dx - 2.4f) * (dx - 2.4f) / 1.1f),
                                expf(-(dy - 2.4f) * (dy - 2.4f) / 1.1f));
            if (py > 2 && py < 7) v += 0.05f;                                     /* le haut accroche la lumiere */
            {                                                                      /* boulons aux 4 coins de la plaque */
                int cx = px < panelX / 2 ? 8 : panelX - 9, cy = py < panelY / 2 ? 8 : panelY - 9;
                float bdx = (float)(px - cx), bdy = (float)(py - cy), br2 = bdx * bdx + bdy * bdy;
                v -= 0.11f * expf(-br2 / 4.5f);
                v += 0.16f * expf(-br2 / 0.55f);
            }
            v += (tnoise(&s) * 2 - 1) * 0.02f;
            float warm = tint * 0.5f;                     /* plaques legerement plus froides ou rouillees entre elles */
            img[(y * N + x) * 3 + 0] = (unsigned char)(clampf(v + warm, 0, 1) * 255);
            img[(y * N + x) * 3 + 1] = (unsigned char)(clampf(v, 0, 1) * 255);
            img[(y * N + x) * 3 + 2] = (unsigned char)(clampf(v - warm * 1.4f, 0, 1) * 255);
        }
    *size = N;
    return img;
}

/* --- grain fin, 2e passe multiplicative pour casser la repetition vue de pres --------- */
static unsigned char *tex_gen_grain(int *size) {
    unsigned int s;
    int N = 64;
    unsigned char *img = (unsigned char *)malloc(N * N * 3);
    s = 0xC0FFEEu;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float v = 0.92f;
            v += sinf(x * 0.5f + 2.1f) * cosf(y * 0.41f) * 0.03f;
            v -= tnoise(&s) * gxGrain;                                           /* poussiere/micro-rayures */
            float sp = tnoise(&s);                                               /* quelques specks, bord adouci */
            if (sp < 0.04f) v -= 0.14f * (1 - sp / 0.04f);
            unsigned char c = (unsigned char)(clampf(v, 0.55f, 1) * 255);
            img[(y * N + x) * 3 + 0] = img[(y * N + x) * 3 + 1] = img[(y * N + x) * 3 + 2] = c;
        }
    *size = N;
    return img;
}

enum { TEXG_SOL, TEXG_MUR, TEXG_GRAIN, TEXG_N };
static const char *const texGenFile[TEXG_N] = { "sol.bmp", "mur.bmp", "grain.bmp" };

static unsigned char *tex_generate(int k, int *size) {
    switch (k) {
    case TEXG_SOL:    return tex_gen_sol(size);
    case TEXG_MUR:    return tex_gen_mur(size);
    case TEXG_GRAIN:  return tex_gen_grain(size);
    }
    *size = 0;
    return NULL;
}

static void bmp_put32(unsigned char *p, unsigned int v) {
    p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); p[2] = (unsigned char)(v >> 16); p[3] = (unsigned char)(v >> 24);
}

/* image RGB carree (de haut en bas) -> BMP 24 bits non compresse : lignes de bas en haut, octets B V R,
 * lignes completees a 4 octets ; exactement ce que load_bmp_square relit. C standard (Windows et macOS) */
static int save_bmp_rgb(const char *path, const unsigned char *img, int N) {
    int stride = (N * 3 + 3) & ~3, ok = 1;
    unsigned int data = (unsigned int)stride * (unsigned int)N;
    unsigned char h[54] = { 0 };
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    h[0] = 'B'; h[1] = 'M';
    bmp_put32(h + 2, 54 + data);                          /* taille du fichier */
    bmp_put32(h + 10, 54);                                /* debut des pixels */
    bmp_put32(h + 14, 40);                                /* en-tete BITMAPINFOHEADER */
    bmp_put32(h + 18, (unsigned int)N); bmp_put32(h + 22, (unsigned int)N);   /* hauteur positive : de bas en haut */
    h[26] = 1; h[28] = 24;                                /* 1 plan, 24 bits, compression 0 */
    bmp_put32(h + 34, data);
    bmp_put32(h + 38, 2835); bmp_put32(h + 42, 2835);     /* 72 dpi */
    unsigned char *row = (unsigned char *)calloc((size_t)stride, 1);
    if (!row || fwrite(h, 1, 54, f) != 54) ok = 0;
    for (int y = N - 1; y >= 0 && ok; y--) {
        for (int x = 0; x < N; x++) {
            row[x * 3 + 0] = img[((size_t)y * N + x) * 3 + 2];
            row[x * 3 + 1] = img[((size_t)y * N + x) * 3 + 1];
            row[x * 3 + 2] = img[((size_t)y * N + x) * 3 + 0];
        }
        if (fwrite(row, 1, (size_t)stride, f) != (size_t)stride) ok = 0;
    }
    free(row);
    if (fclose(f) != 0) ok = 0;
    return ok;
}

/* --export-textures : chaque texture generee ecrite dans dir (sol.bmp, mur.bmp...) ; nombre de fichiers ecrits */
static int export_textures(const char *dir) {
    int done = 0;
    for (int k = 0; k < TEXG_N; k++) {
        char p[700];
        int n = 0;
        unsigned char *img = tex_generate(k, &n);
        snprintf(p, sizeof p, "%s/%s", dir, texGenFile[k]);
        if (img && save_bmp_rgb(p, img, n)) done++;
        free(img);
    }
    return done;
}

static void make_textures(void) {
    GLuint *dst[TEXG_N] = { &texGrid, &texWall, &texGrime };
    for (int k = 0; k < TEXG_N; k++) {                   /* un .bmp du dossier graphismes, sinon la texture generee */
        int n = 0;
        if (tex_override(dst[k], texGenFile[k])) continue;
        unsigned char *img = tex_generate(k, &n);
        upload_rgb_mip(dst[k], img, n);
        free(img);
    }

    int D = 32;
    unsigned char *dot = (unsigned char *)malloc(D * D * 4);
    for (int y = 0; y < D; y++)
        for (int x = 0; x < D; x++) {
            float dx = (x + 0.5f) / D * 2 - 1, dy = (y + 0.5f) / D * 2 - 1;
            float a = clampf(1 - sqrtf(dx * dx + dy * dy), 0, 1);
            unsigned char *p = &dot[(y * D + x) * 4];
            p[0] = p[1] = p[2] = 255; p[3] = (unsigned char)(a * a * 255);
        }
    glGenTextures(1, &texDot);
    glBindTexture(GL_TEXTURE_2D, texDot);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, D, D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dot);
    free(dot);
}

static void quadf(V3 n, V3 a, V3 b, V3 c, V3 d) {
    V3 vs[4] = { a, b, c, d };
    glNormal3f(n.x, n.y, n.z);
    for (int i = 0; i < 4; i++) {
        V3 p = vs[i]; float u, v;
        if (fabsf(n.x) > 0.5f) { u = p.z; v = p.y; }
        else if (fabsf(n.y) > 0.5f) { u = p.x; v = p.z; }
        else { u = p.x; v = p.y; }
        glTexCoord2f(u * 0.5f, v * 0.5f);
        glVertex3f(p.x, p.y, p.z);
    }
}

static void box_faces(V3 a, V3 b) {
    glBegin(GL_QUADS);
    quadf(v3(1, 0, 0),  v3(b.x, a.y, a.z), v3(b.x, b.y, a.z), v3(b.x, b.y, b.z), v3(b.x, a.y, b.z));
    quadf(v3(-1, 0, 0), v3(a.x, a.y, a.z), v3(a.x, a.y, b.z), v3(a.x, b.y, b.z), v3(a.x, b.y, a.z));
    quadf(v3(0, 1, 0),  v3(a.x, b.y, a.z), v3(a.x, b.y, b.z), v3(b.x, b.y, b.z), v3(b.x, b.y, a.z));
    quadf(v3(0, -1, 0), v3(a.x, a.y, a.z), v3(b.x, a.y, a.z), v3(b.x, a.y, b.z), v3(a.x, a.y, b.z));
    quadf(v3(0, 0, 1),  v3(a.x, a.y, b.z), v3(b.x, a.y, b.z), v3(b.x, b.y, b.z), v3(a.x, b.y, b.z));
    quadf(v3(0, 0, -1), v3(a.x, a.y, a.z), v3(a.x, b.y, a.z), v3(b.x, b.y, a.z), v3(b.x, a.y, a.z));
    glEnd();
}

/* masque de faces et quad a echelle de texture libre : rendu du monde en plusieurs passes (world_faces),
 * sans toucher a box_faces, utilise partout ailleurs (armes, ennemis) a l'echelle fixe 0.5 */
enum { FACE_PX = 1, FACE_NX = 2, FACE_PY = 4, FACE_NY = 8, FACE_PZ = 16, FACE_NZ = 32,
       FACE_SIDES = FACE_PX | FACE_NX | FACE_PZ | FACE_NZ, FACE_UD = FACE_PY | FACE_NY,
       FACE_ALL = FACE_SIDES | FACE_UD };

static void quad_tex(V3 n, V3 a, V3 b, V3 c, V3 d, float scale) {
    V3 vs[4] = { a, b, c, d };
    glNormal3f(n.x, n.y, n.z);
    for (int i = 0; i < 4; i++) {
        V3 p = vs[i]; float u, v;
        if (fabsf(n.x) > 0.5f) { u = p.z; v = p.y; }
        else if (fabsf(n.y) > 0.5f) { u = p.x; v = p.z; }
        else { u = p.x; v = p.y; }
        glTexCoord2f(u * scale, v * scale);
        glVertex3f(p.x, p.y, p.z);
    }
}

static void box_edges(V3 a, V3 b) {
    float xs[2] = { a.x, b.x }, ys[2] = { a.y, b.y }, zs[2] = { a.z, b.z };
    glBegin(GL_LINES);
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++) {
            glVertex3f(a.x, ys[i], zs[j]); glVertex3f(b.x, ys[i], zs[j]);
            glVertex3f(xs[i], a.y, zs[j]); glVertex3f(xs[i], b.y, zs[j]);
            glVertex3f(xs[i], ys[j], a.z); glVertex3f(xs[i], ys[j], b.z);
        }
    glEnd();
}

/* prisme a facettes le long de z (cylindre/cone selon r0/r1), centre c, n faces, bouts fermes si
 * cap0/cap1 : sert aux canons ronds, bagues, lunettes... des armes (modeles bien plus anguleux sinon,
 * fait uniquement de boites) */
static void prism_z(V3 c, float z0, float z1, float r0, float r1, int n, int cap0, int cap1) {
    glBegin(GL_QUADS);
    for (int i = 0; i < n; i++) {
        float a0 = i * (2 * PI / n), a1 = (i + 1) * (2 * PI / n);
        float cx0 = cosf(a0), sx0 = sinf(a0), cx1 = cosf(a1), sx1 = sinf(a1);
        V3 nrm = vnorm(v3(cx0 + cx1, sx0 + sx1, 0));
        glNormal3f(nrm.x, nrm.y, nrm.z);
        glVertex3f(c.x + cx0 * r0, c.y + sx0 * r0, c.z + z0);
        glVertex3f(c.x + cx1 * r0, c.y + sx1 * r0, c.z + z0);
        glVertex3f(c.x + cx1 * r1, c.y + sx1 * r1, c.z + z1);
        glVertex3f(c.x + cx0 * r1, c.y + sx0 * r1, c.z + z1);
    }
    glEnd();
    if (cap0 && r0 > 0.0001f) {
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 0, -1);
        glVertex3f(c.x, c.y, c.z + z0);
        for (int i = n; i >= 0; i--) { float a = i * (2 * PI / n); glVertex3f(c.x + cosf(a) * r0, c.y + sinf(a) * r0, c.z + z0); }
        glEnd();
    }
    if (cap1 && r1 > 0.0001f) {
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 0, 1);
        glVertex3f(c.x, c.y, c.z + z1);
        for (int i = 0; i <= n; i++) { float a = i * (2 * PI / n); glVertex3f(c.x + cosf(a) * r1, c.y + sinf(a) * r1, c.z + z1); }
        glEnd();
    }
}

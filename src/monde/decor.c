/* VELOCITY - decor maille : colonnes, arches, coupoles, toits de tuiles, caisses biseautees, arbres, auvents...
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Le monde jouable reste en boites (collisions, rayons, lightmaps).   */
/* Le decor ajoute des triangles par-dessus : formes arrondies et      */
/* details, sans collision propre (une boite B_HIDDEN, invisible, sert */
/* de collision et porte l'ombre quand il le faut). Chaque sommet      */
/* garde sa couleur de base ; l'eclairage (soleil, ombres, occlusion,  */
/* ciel) est calcule par sommet au chargement, comme les lightmaps     */
/* (decor_bake dans rendu/eclairage.c), puis tout part dans la liste   */
/* d'affichage du monde.                                               */
/* ------------------------------------------------------------------ */
#define DEC_MAXV 750000                                   /* sommets (triangles x 3) : 250 000 triangles */
typedef struct { V3 p, n, c; } DecV;
static DecV DV[DEC_MAXV];
static float DVL[DEC_MAXV][3];                             /* couleur eclairee (decor_bake) */
static int ndv;
static unsigned decSeed = 1;

static void decor_reset(void) { ndv = 0; decSeed = 1; }
static float dc_rand(void) { decSeed = decSeed * 1664525u + 1013904223u; return (decSeed >> 8) * (1.0f / 16777216.0f); }
static V3 dc_jit(V3 c, float k) {                          /* petite variation de teinte : fait main, pas imprime */
    float j = 1 + (dc_rand() * 2 - 1) * k;
    return v3(clampf(c.x * j, 0, 1), clampf(c.y * j, 0, 1), clampf(c.z * j, 0, 1));
}
static void dc_v(V3 p, V3 n, V3 c) {
    if (ndv >= DEC_MAXV) return;
    DV[ndv].p = p; DV[ndv].n = n; DV[ndv].c = c; ndv++;
}
static void dc_tri(V3 a, V3 b, V3 c, V3 col) {            /* normale plate */
    V3 n = vnorm(vcross(vsub(b, a), vsub(c, a)));
    dc_v(a, n, col); dc_v(b, n, col); dc_v(c, n, col);
}
static void dc_quad(V3 a, V3 b, V3 c, V3 d, V3 col) { dc_tri(a, b, c, col); dc_tri(a, c, d, col); }
/* quad avec normales par sommet (surfaces courbes lisses) */
static void dc_quadn(V3 a, V3 na, V3 b, V3 nb2, V3 c, V3 nc, V3 d, V3 nd, V3 col) {
    dc_v(a, na, col); dc_v(b, nb2, col); dc_v(c, nc, col);
    dc_v(a, na, col); dc_v(c, nc, col); dc_v(d, nd, col);
}

/* tronc de cone (r1 = 0 : cone ; r0 = r1 : cylindre) d'axe vertical, bouchons optionnels */
static void dc_cone(V3 b, float r0, float r1, float h, int seg, V3 col, int capBot, int capTop) {
    float slope = (r0 - r1) / fmaxf(h, 0.01f);
    for (int i = 0; i < seg; i++) {
        float a0 = 2 * PI * i / seg, a1 = 2 * PI * (i + 1) / seg;
        V3 d0 = v3(cosf(a0), 0, sinf(a0)), d1 = v3(cosf(a1), 0, sinf(a1));
        V3 n0 = vnorm(v3(d0.x, slope, d0.z)), n1 = vnorm(v3(d1.x, slope, d1.z));
        V3 p00 = vadd(b, vmul(d0, r0)), p01 = vadd(b, vmul(d1, r0));
        V3 p10 = vadd(b, v3(d0.x * r1, h, d0.z * r1)), p11 = vadd(b, v3(d1.x * r1, h, d1.z * r1));
        dc_quadn(p00, n0, p10, n0, p11, n1, p01, n1, col);
        if (capBot) dc_tri(b, p01, p00, col);
        if (capTop && r1 > 0) dc_tri(vadd(b, v3(0, h, 0)), p10, p11, col);
    }
}
static void dc_cyl(V3 b, float r, float h, int seg, V3 col) { dc_cone(b, r, r, h, seg, col, 0, 1); }
/* sphere (ou demi-sphere si half) aplatie de sy, normales lisses */
static void dc_sphere(V3 c, float r, float sy, int seg, int rings, int half, V3 col) {
    int r0 = half ? rings / 2 : 0;
    for (int j = r0; j < rings; j++) {
        float t0 = PI * (0.5f - (float)j / rings), t1 = PI * (0.5f - (float)(j + 1) / rings);   /* du haut vers le bas */
        if (half) { t0 = PI * 0.5f * (1 - (float)(j - r0) / (rings - r0)); t1 = PI * 0.5f * (1 - (float)(j + 1 - r0) / (rings - r0)); }
        for (int i = 0; i < seg; i++) {
            float a0 = 2 * PI * i / seg, a1 = 2 * PI * (i + 1) / seg;
            V3 n00 = v3(cosf(t0) * cosf(a0), sinf(t0), cosf(t0) * sinf(a0)), n01 = v3(cosf(t0) * cosf(a1), sinf(t0), cosf(t0) * sinf(a1));
            V3 n10 = v3(cosf(t1) * cosf(a0), sinf(t1), cosf(t1) * sinf(a0)), n11 = v3(cosf(t1) * cosf(a1), sinf(t1), cosf(t1) * sinf(a1));
            V3 p00 = vadd(c, v3(n00.x * r, n00.y * r * sy, n00.z * r)), p01 = vadd(c, v3(n01.x * r, n01.y * r * sy, n01.z * r));
            V3 p10 = vadd(c, v3(n10.x * r, n10.y * r * sy, n10.z * r)), p11 = vadd(c, v3(n11.x * r, n11.y * r * sy, n11.z * r));
            dc_quadn(p00, n00, p01, n01, p11, n11, p10, n10, col);
        }
    }
}
/* boite aux aretes chanfreinees (caisses, bancs, pierres) */
static void dc_bevel(V3 a, V3 b, float e, V3 col) {
    float x[4] = { a.x, a.x + e, b.x - e, b.x }, y[4] = { a.y, a.y + e, b.y - e, b.y }, z[4] = { a.z, a.z + e, b.z - e, b.z };
    /* 6 faces internes */
    dc_quad(v3(x[3], y[1], z[1]), v3(x[3], y[2], z[1]), v3(x[3], y[2], z[2]), v3(x[3], y[1], z[2]), col);
    dc_quad(v3(x[0], y[1], z[1]), v3(x[0], y[1], z[2]), v3(x[0], y[2], z[2]), v3(x[0], y[2], z[1]), col);
    dc_quad(v3(x[1], y[3], z[1]), v3(x[1], y[3], z[2]), v3(x[2], y[3], z[2]), v3(x[2], y[3], z[1]), col);
    dc_quad(v3(x[1], y[0], z[1]), v3(x[2], y[0], z[1]), v3(x[2], y[0], z[2]), v3(x[1], y[0], z[2]), col);
    dc_quad(v3(x[1], y[1], z[3]), v3(x[2], y[1], z[3]), v3(x[2], y[2], z[3]), v3(x[1], y[2], z[3]), col);
    dc_quad(v3(x[1], y[1], z[0]), v3(x[1], y[2], z[0]), v3(x[2], y[2], z[0]), v3(x[2], y[1], z[0]), col);
    /* 12 chanfreins */
    for (int sx = 0; sx < 2; sx++)
        for (int sy = 0; sy < 2; sy++) {
            float X0 = sx ? x[2] : x[1], X1 = sx ? x[3] : x[0], Y0 = sy ? y[2] : y[1], Y1 = sy ? y[3] : y[0];
            float Z0 = sx ? z[2] : z[1], Z1 = sx ? z[3] : z[0];
            (void)Z0; (void)Z1;
            dc_quad(v3(X1, Y0, z[1]), v3(X0, Y1, z[1]), v3(X0, Y1, z[2]), v3(X1, Y0, z[2]), col);   /* aretes le long de z */
        }
    for (int sx = 0; sx < 2; sx++)
        for (int sz = 0; sz < 2; sz++) {
            float X0 = sx ? x[2] : x[1], X1 = sx ? x[3] : x[0], Z0 = sz ? z[2] : z[1], Z1 = sz ? z[3] : z[0];
            dc_quad(v3(X1, y[1], Z0), v3(X1, y[2], Z0), v3(X0, y[2], Z1), v3(X0, y[1], Z1), col);   /* aretes verticales */
        }
    for (int sy = 0; sy < 2; sy++)
        for (int sz = 0; sz < 2; sz++) {
            float Y0 = sy ? y[2] : y[1], Y1 = sy ? y[3] : y[0], Z0 = sz ? z[2] : z[1], Z1 = sz ? z[3] : z[0];
            dc_quad(v3(x[1], Y1, Z0), v3(x[2], Y1, Z0), v3(x[2], Y0, Z1), v3(x[1], Y0, Z1), col);   /* aretes le long de x */
        }
    for (int c = 0; c < 8; c++) {                         /* 8 coins */
        int sx = c & 1, sy = (c >> 1) & 1, sz = (c >> 2) & 1;
        V3 p1 = v3(sx ? x[3] : x[0], sy ? y[2] : y[1], sz ? z[2] : z[1]);
        V3 p2 = v3(sx ? x[2] : x[1], sy ? y[3] : y[0], sz ? z[2] : z[1]);
        V3 p3 = v3(sx ? x[2] : x[1], sy ? y[2] : y[1], sz ? z[3] : z[0]);
        dc_tri(p1, p2, p3, col);
    }
}
/* arche en plein cintre : anneau entre rIn et rIn + w, profondeur d (axe 0 : l'arche s'ouvre selon x, epaisseur
 * en z ; axe 2 : l'inverse). c = milieu du pied de l'arche, a la hauteur ou commence la courbe */
static void dc_arch(V3 c, int axis, float rIn, float w, float d, int seg, V3 col) {
    float rOut = rIn + w;
    for (int i = 0; i < seg; i++) {
        float a0 = PI * i / seg, a1 = PI * (i + 1) / seg;
        float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        V3 u = axis == 0 ? v3(1, 0, 0) : v3(0, 0, 1), dep = axis == 0 ? v3(0, 0, d * 0.5f) : v3(d * 0.5f, 0, 0);
        V3 i0 = vadd(c, vadd(vmul(u, c0 * rIn), v3(0, s0 * rIn, 0))), i1 = vadd(c, vadd(vmul(u, c1 * rIn), v3(0, s1 * rIn, 0)));
        V3 o0 = vadd(c, vadd(vmul(u, c0 * rOut), v3(0, s0 * rOut, 0))), o1 = vadd(c, vadd(vmul(u, c1 * rOut), v3(0, s1 * rOut, 0)));
        V3 ni0 = vmul(vadd(vmul(u, c0), v3(0, s0, 0)), -1), ni1 = vmul(vadd(vmul(u, c1), v3(0, s1, 0)), -1);
        dc_quadn(vadd(i0, dep), ni0, vsub(i0, dep), ni0, vsub(i1, dep), ni1, vadd(i1, dep), ni1, col);   /* intrados */
        dc_quadn(vsub(o0, dep), vmul(ni0, -1), vadd(o0, dep), vmul(ni0, -1), vadd(o1, dep), vmul(ni1, -1), vsub(o1, dep), vmul(ni1, -1), col);
        dc_quad(vadd(i0, dep), vadd(i1, dep), vadd(o1, dep), vadd(o0, dep), col);   /* faces avant et arriere */
        dc_quad(vsub(i0, dep), vsub(o0, dep), vsub(o1, dep), vsub(i1, dep), col);
    }
}
/* arche dont les ecoincons sont remplis jusqu'a topY (arcade, porte de ville) : le mur au-dessus de la courbe,
 * sur les deux faces, plus les jambages de chaque cote jusqu'a topY */
static void dc_arch_fill(V3 c, int axis, float rIn, float w, float d, int seg, float topY, V3 col) {
    dc_arch(c, axis, rIn, w, d, seg, col);
    float rOut = rIn + w;
    V3 u = axis == 0 ? v3(1, 0, 0) : v3(0, 0, 1), dep = axis == 0 ? v3(0, 0, d * 0.5f) : v3(d * 0.5f, 0, 0);
    for (int i = 0; i < seg; i++) {
        float a0 = PI * i / seg, a1 = PI * (i + 1) / seg;
        V3 o0 = vadd(c, vadd(vmul(u, cosf(a0) * rOut), v3(0, sinf(a0) * rOut, 0)));
        V3 o1 = vadd(c, vadd(vmul(u, cosf(a1) * rOut), v3(0, sinf(a1) * rOut, 0)));
        V3 t0 = v3(o0.x, topY, o0.z), t1 = v3(o1.x, topY, o1.z);
        if (topY <= fmaxf(o0.y, o1.y)) continue;
        dc_quad(vadd(o0, dep), vadd(t0, dep), vadd(t1, dep), vadd(o1, dep), col);
        dc_quad(vsub(o0, dep), vsub(o1, dep), vsub(t1, dep), vsub(t0, dep), col);
    }
    V3 top = v3(c.x, topY, c.z);                          /* dessus du remplissage */
    V3 e0 = vadd(top, vmul(u, -rOut)), e1 = vadd(top, vmul(u, rOut));
    dc_quad(vsub(e0, dep), vadd(e0, dep), vadd(e1, dep), vsub(e1, dep), col);
}
/* toit a deux pans en tuiles sur l'emprise (a, b) au niveau a.y : faitage selon x (ax 0) ou z (ax 2), pente h,
 * debord ov, tuiles en rangees de demi-cylindres, faitiere, pignons */
static void dc_roof(V3 a, V3 b, float h, int ax, float ov, V3 tile, V3 wall) {
    float y = a.y;
    float L0 = (ax == 0 ? a.x : a.z) - ov, L1 = (ax == 0 ? b.x : b.z) + ov;   /* le long du faitage */
    float W0 = (ax == 0 ? a.z : a.x) - ov, W1 = (ax == 0 ? b.z : b.x) + ov;   /* en travers */
    float Wm = (W0 + W1) * 0.5f;
#define RP(l, w, yy) (ax == 0 ? v3(l, yy, w) : v3(w, yy, l))
    for (int side = 0; side < 2; side++) {                /* deux pans */
        float We = side ? W1 : W0;
        V3 p0 = RP(L0, We, y - 0.15f), p1 = RP(L1, We, y - 0.15f), p2 = RP(L1, Wm, y + h), p3 = RP(L0, Wm, y + h);
        if (side) dc_quad(p0, p3, p2, p1, dc_jit(tile, 0.05f)); else dc_quad(p0, p1, p2, p3, dc_jit(tile, 0.05f));
        dc_quad(p0, p1, vadd(p1, v3(0, -0.18f, 0)), vadd(p0, v3(0, -0.18f, 0)), vmul(tile, 0.6f));   /* tranche du debord */
        /* rangees de tuiles : demi-cylindres qui descendent la pente */
        float span = fabsf(Wm - We), len = sqrtf(span * span + h * h);
        int rows = (int)((L1 - L0) / 0.55f);
        int seg = 4;
        for (int r = 0; r < rows; r++) {
            float l = L0 + (r + 0.5f) * (L1 - L0) / rows, rr = 0.2f;
            V3 top = RP(l, Wm, y + h), bot = RP(l, We, y - 0.15f);
            V3 dn = vmul(vsub(bot, top), 1 / len), across = ax == 0 ? v3(1, 0, 0) : v3(0, 0, 1);
            V3 up = vnorm(vcross(side ? across : vmul(across, -1), dn));
            if (up.y < 0) up = vmul(up, -1);
            V3 tc = dc_jit(tile, 0.09f);
            for (int s = 0; s < seg; s++) {
                float a0 = PI * s / seg, a1 = PI * (s + 1) / seg;
                V3 o0 = vadd(vmul(across, cosf(a0) * rr), vmul(up, sinf(a0) * rr)), o1 = vadd(vmul(across, cosf(a1) * rr), vmul(up, sinf(a1) * rr));
                V3 n0 = vnorm(o0), n1 = vnorm(o1);
                dc_quadn(vadd(top, o0), n0, vadd(bot, o0), n0, vadd(bot, o1), n1, vadd(top, o1), n1, tc);
            }
        }
    }
    for (int e = 0; e < 2; e++) {                         /* pignons (triangles d'enduit) */
        float L = e ? (ax == 0 ? b.x : b.z) : (ax == 0 ? a.x : a.z);
        float w0 = ax == 0 ? a.z : a.x, w1 = ax == 0 ? b.z : b.x;
        V3 q0 = RP(L, w0, y), q1 = RP(L, w1, y), q2 = RP(L, (w0 + w1) * 0.5f, y + h * (w1 - w0) / (W1 - W0));
        if (e) dc_tri(q0, q2, q1, wall); else dc_tri(q0, q1, q2, wall);
    }
    {                                                     /* faitiere : cylindre couche */
        int seg = 8;
        for (int s = 0; s < seg; s++) {
            float a0 = 2 * PI * s / seg, a1 = 2 * PI * (s + 1) / seg;
            V3 o0 = ax == 0 ? v3(0, sinf(a0) * 0.28f, cosf(a0) * 0.28f) : v3(cosf(a0) * 0.28f, sinf(a0) * 0.28f, 0);
            V3 o1 = ax == 0 ? v3(0, sinf(a1) * 0.28f, cosf(a1) * 0.28f) : v3(cosf(a1) * 0.28f, sinf(a1) * 0.28f, 0);
            V3 r0 = RP(L0, Wm, y + h + 0.05f), r1 = RP(L1, Wm, y + h + 0.05f);
            dc_quadn(vadd(r0, o0), vnorm(o0), vadd(r1, o0), vnorm(o0), vadd(r1, o1), vnorm(o1), vadd(r0, o1), vnorm(o1), vmul(tile, 0.85f));
        }
    }
#undef RP
}
/* fenetre sur une facade : centre c, normale sortante n (axe x ou z), largeur w, hauteur h ; cadre, verre sombre
 * en retrait, linteau arrondi, appui, volets ouverts */
static void dc_window(V3 c, V3 n, float w, float h, V3 frame, V3 shutter) {
    V3 u = v3(-n.z, 0, n.x), up = v3(0, 1, 0);
    V3 out = vmul(n, 0.05f), glassC = vsub(c, vmul(n, 0.12f));
    V3 g0 = vadd(vsub(glassC, vmul(u, w * 0.5f)), vmul(up, -h * 0.5f)), g1 = vadd(vadd(glassC, vmul(u, w * 0.5f)), vmul(up, -h * 0.5f));
    V3 g2 = vadd(g1, vmul(up, h)), g3 = vadd(g0, vmul(up, h));
    dc_quad(g0, g1, g2, g3, v3(0.08f, 0.1f, 0.14f));      /* verre sombre */
    for (int k = 0; k < 6; k++) {                         /* demi-lune au-dessus, en eventail */
        float a0 = PI * k / 6, a1 = PI * (k + 1) / 6, r = w * 0.5f;
        V3 m = vadd(glassC, vmul(up, h * 0.5f));
        dc_tri(m, vadd(m, vadd(vmul(u, cosf(a1) * r), vmul(up, sinf(a1) * r))), vadd(m, vadd(vmul(u, cosf(a0) * r), vmul(up, sinf(a0) * r))), v3(0.1f, 0.12f, 0.17f));
    }
    float t = 0.14f;                                      /* cadre en relief */
    V3 fc = vadd(c, out);
    for (int s = 0; s < 2; s++) {                         /* montants */
        V3 m = vadd(fc, vmul(u, (s ? 1 : -1) * (w * 0.5f + t * 0.5f)));
        V3 a = vsub(vsub(m, vmul(u, t * 0.5f)), v3(0, h * 0.5f, 0)), b = vadd(vadd(m, vmul(u, t * 0.5f)), v3(0, h * 0.5f, 0));
        dc_bevel(v3(fminf(a.x, b.x) - fabsf(n.x) * 0.1f, a.y, fminf(a.z, b.z) - fabsf(n.z) * 0.1f),
                 v3(fmaxf(a.x, b.x) + fabsf(n.x) * 0.1f, b.y, fmaxf(a.z, b.z) + fabsf(n.z) * 0.1f), 0.03f, frame);
    }
    {                                                     /* appui de fenetre */
        V3 m = vadd(fc, v3(0, -h * 0.5f - 0.08f, 0));
        V3 a = vsub(m, vmul(u, w * 0.5f + 0.3f)), b = vadd(m, vmul(u, w * 0.5f + 0.3f));
        dc_bevel(v3(fminf(a.x, b.x) - fabsf(n.x) * 0.25f, m.y - 0.1f, fminf(a.z, b.z) - fabsf(n.z) * 0.25f),
                 v3(fmaxf(a.x, b.x) + fabsf(n.x) * 0.25f, m.y + 0.08f, fmaxf(a.z, b.z) + fabsf(n.z) * 0.25f), 0.03f, frame);
    }
    for (int s = 0; s < 2; s++) {                         /* volets ouverts, a lames */
        V3 hinge = vadd(fc, vmul(u, (s ? 1 : -1) * (w * 0.5f + t)));
        V3 o = vmul(u, (s ? 1 : -1) * w * 0.5f);
        for (int k = 0; k < 6; k++) {
            float y0 = -h * 0.5f + h * k / 6, y1 = y0 + h / 6 - 0.04f;
            V3 p0 = vadd(hinge, v3(0, y0, 0)), p1 = vadd(vadd(hinge, o), v3(0, y0, 0));
            V3 p2 = vadd(vadd(hinge, o), v3(0, y1, 0)), p3 = vadd(hinge, v3(0, y1, 0));
            V3 tilt = vmul(n, 0.06f);
            dc_quad(vadd(p0, vmul(tilt, 0.2f)), vadd(p1, vmul(tilt, 0.2f)), vadd(p2, tilt), vadd(p3, tilt), dc_jit(shutter, 0.06f));
        }
    }
}
/* auvent de toile raye entre a et b (bord haut contre le mur), qui descend de drop sur depth vers n ; leger creux */
static void dc_awning(V3 a, V3 b, V3 n, float depth, float drop, V3 c1, V3 c2) {
    int nu = 8, nv = 4;
    for (int i = 0; i < nu; i++)
        for (int j = 0; j < nv; j++) {
            V3 P[4];
            for (int k = 0; k < 4; k++) {
                float u = (float)(i + (k == 1 || k == 2)) / nu, v = (float)(j + (k >= 2)) / nv;
                V3 base = vlerp(a, b, u);
                float sag = 0.25f * sinf(PI * u) * sinf(PI * v);
                P[k] = vadd(vadd(base, vmul(n, depth * v)), v3(0, -drop * v - sag, 0));
            }
            dc_quad(P[0], P[1], P[2], P[3], (i & 1) ? c1 : c2);
            dc_quad(P[0], P[3], P[2], P[1], vmul((i & 1) ? c1 : c2, 0.7f));   /* dessous */
        }
    for (int i = 0; i <= nu; i += 2) {                    /* festons du bord avant */
        V3 e = vadd(vadd(vlerp(a, b, (float)i / nu), vmul(n, depth)), v3(0, -drop, 0));
        V3 u = vnorm(vsub(b, a));
        dc_tri(vsub(e, vmul(u, 0.3f)), vadd(e, vmul(u, 0.3f)), vadd(e, v3(0, -0.35f, 0)), c2);
    }
}
/* cypres : cone fusele en couches ; palmier : tronc a anneaux et palmes */
static void dc_cypress(V3 b, float h) {
    V3 g = dc_jit(v3(0.12f, 0.28f, 0.15f), 0.1f);
    dc_cyl(b, 0.18f, h * 0.15f, 6, v3(0.35f, 0.24f, 0.15f));
    for (int k = 0; k < 5; k++) {
        float y0 = h * (0.1f + 0.17f * k), r = 1.1f * (1 - 0.16f * k);
        dc_cone(vadd(b, v3(0, y0, 0)), r, r * 0.55f, h * 0.24f, 10, vmul(g, 1 + 0.05f * k), 1, k == 4);
    }
    dc_cone(vadd(b, v3(0, h * 0.85f, 0)), 0.45f, 0, h * 0.22f, 10, g, 1, 0);
}
static void dc_palm(V3 b, float h, float lean) {
    V3 bark = v3(0.5f, 0.38f, 0.25f), leaf = dc_jit(v3(0.17f, 0.38f, 0.15f), 0.1f);
    V3 p = b;
    int n = 8;
    for (int k = 0; k < n; k++) {                         /* tronc courbe en anneaux */
        float t = (float)k / n;
        V3 q = vadd(b, v3(lean * t * t * h * 0.3f, h * (k + 1) / n, 0));
        dc_cone(p, 0.32f - 0.12f * t, 0.3f - 0.12f * t, vlen(vsub(q, p)), 7, vmul(bark, 0.9f + 0.2f * (k & 1)), 0, 0);
        p = q;
    }
    for (int f = 0; f < 9; f++) {                         /* palmes : bandes qui retombent */
        float a = 2 * PI * f / 9 + 0.3f, len = 3.2f;
        V3 d = v3(cosf(a), 0, sinf(a)), s = v3(-d.z, 0, d.x);
        V3 prev = p;
        for (int k = 1; k <= 5; k++) {
            float t = (float)k / 5, wdt = 0.55f * sinf(PI * t) + 0.08f;
            V3 q = vadd(p, vadd(vmul(d, len * t), v3(0, 0.6f * t - 1.6f * t * t, 0)));
            float w0 = 0.55f * sinf(PI * (t - 0.2f)) + 0.08f;
            dc_quad(vsub(prev, vmul(s, w0)), vadd(prev, vmul(s, w0)), vadd(q, vmul(s, wdt)), vsub(q, vmul(s, wdt)), leaf);
            dc_quad(vsub(prev, vmul(s, w0)), vsub(q, vmul(s, wdt)), vadd(q, vmul(s, wdt)), vadd(prev, vmul(s, w0)), vmul(leaf, 0.8f));
            prev = q;
        }
    }
    dc_sphere(vadd(p, v3(0, -0.2f, 0)), 0.45f, 1, 6, 4, 0, v3(0.4f, 0.3f, 0.18f));   /* noix de coco */
}
/* rambarde a balustres entre a et b (au sol a.y), hauteur h */
static void dc_railing(V3 a, V3 b, float h, V3 col) {
    V3 d = vsub(b, a);
    float L = hlen(d);
    int n = (int)(L / 0.45f);
    for (int i = 0; i <= n; i++) {
        V3 p = vlerp(a, b, (float)i / (n > 0 ? n : 1));
        dc_cone(p, 0.07f, 0.11f, h * 0.45f, 6, col, 0, 0);
        dc_cone(vadd(p, v3(0, h * 0.45f, 0)), 0.11f, 0.06f, h * 0.45f, 6, col, 0, 0);
    }
    V3 s = vmul(vnorm(v3(-d.z, 0, d.x)), 0.14f);
    dc_bevel(v3(fminf(a.x, b.x) - fabsf(s.x), a.y + h * 0.9f, fminf(a.z, b.z) - fabsf(s.z)),
             v3(fmaxf(a.x, b.x) + fabsf(s.x), a.y + h, fmaxf(a.z, b.z) + fabsf(s.z)), 0.04f, col);
}
/* lanterne murale : potence, cage, verre lumineux */
static void dc_lantern(V3 c, V3 n) {
    V3 iron = v3(0.12f, 0.12f, 0.13f);
    dc_bevel(v3(fminf(c.x, c.x + n.x * 0.6f) - 0.04f, c.y + 0.5f, fminf(c.z, c.z + n.z * 0.6f) - 0.04f),
             v3(fmaxf(c.x, c.x + n.x * 0.6f) + 0.04f, c.y + 0.58f, fmaxf(c.z, c.z + n.z * 0.6f) + 0.04f), 0.01f, iron);
    V3 l = vadd(c, vmul(n, 0.6f));
    dc_cone(vadd(l, v3(0, -0.2f, 0)), 0.18f, 0.24f, 0.45f, 6, v3(1.0f, 0.85f, 0.5f), 1, 0);
    dc_cone(vadd(l, v3(0, 0.25f, 0)), 0.3f, 0.05f, 0.25f, 6, iron, 1, 0);
}
/* colonne classique : base, fut cannele leger, chapiteau */
static void dc_column(V3 b, float h, float r, V3 col) {
    dc_bevel(v3(b.x - r * 1.45f, b.y, b.z - r * 1.45f), v3(b.x + r * 1.45f, b.y + 0.35f, b.z + r * 1.45f), 0.06f, col);
    dc_cone(vadd(b, v3(0, 0.35f, 0)), r * 1.25f, r, 0.25f, 14, col, 0, 0);
    dc_cone(vadd(b, v3(0, 0.6f, 0)), r, r * 0.9f, h - 1.4f, 14, col, 0, 0);
    dc_cone(vadd(b, v3(0, h - 0.8f, 0)), r * 0.9f, r * 1.3f, 0.4f, 14, col, 0, 0);
    dc_bevel(v3(b.x - r * 1.5f, b.y + h - 0.4f, b.z - r * 1.5f), v3(b.x + r * 1.5f, b.y + h, b.z + r * 1.5f), 0.06f, col);
}

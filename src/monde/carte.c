/* VELOCITY - carte en boites (AABB), recharges, collisions, rayons
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Monde : une liste d'AABB                                            */
/* ------------------------------------------------------------------ */
enum { B_SOLID, B_PAD };
typedef struct { V3 mn, mx, col, launch; int kind; } Box;
#define MAXB 256
static Box boxes[MAXB];
static int nb;

static void addbox(float x0, float y0, float z0, float x1, float y1, float z1, float r, float g, float b) {
    if (nb >= MAXB) return;
    Box *bx = &boxes[nb++];
    bx->mn = v3(x0, y0, z0); bx->mx = v3(x1, y1, z1); bx->col = v3(r, g, b);
    bx->kind = B_SOLID; bx->launch = v3(0, 0, 0);
}

static void addpad(float x, float z, V3 launch) {
    addbox(x - 1.5f, 0, z - 1.5f, x + 1.5f, 0.3f, z + 1.5f, 0.2f, 0.9f, 1.0f);
    boxes[nb - 1].kind = B_PAD;
    boxes[nb - 1].launch = launch;
}

static int near2(float x, float z, float cx, float cz, float r) {
    return (x - cx) * (x - cx) + (z - cz) * (z - cz) < r * r;
}

/* La carte est dessinee a l'echelle 1 puis agrandie horizontalement (les hauteurs ne changent pas) */
#define MAP_S 1.4f
static float ms(float v) { return v * MAP_S; }

/* cellules de munitions posees sur la carte ; t > 0 : deja ramassee, revient dans t secondes.
 * pts : points donnes a la jauge de chacune des deux armes du loadout (PICKUP_SMALL ou PICKUP_BIG) */
#define MAXPICK 16
typedef struct { V3 pos; float t, pending; int pts; } Pickup;
static Pickup PU[MAXPICK];
static int npu;

static void addpickup(float x, float y, float z, int pts) {
    if (npu >= MAXPICK) return;
    PU[npu].pos = v3(ms(x), y + 0.9f, ms(z));
    PU[npu].t = PU[npu].pending = 0;
    PU[npu].pts = pts;
    npu++;
}

static void build_level(void) {
    float W = ms(56);
    nb = 0; npu = 0;
    addbox(-W, -2, -W, W, 0, W, 0.52f, 0.54f, 0.60f);              /* sol */
    addbox(-W - 2, 0, -W - 2, W + 2, 24, -W, 0.40f, 0.42f, 0.52f);   /* murs exterieurs */
    addbox(-W - 2, 0, W, W + 2, 24, W + 2, 0.40f, 0.42f, 0.52f);
    addbox(-W - 2, 0, -W, -W, 24, W, 0.40f, 0.42f, 0.52f);
    addbox(W, 0, -W, W + 2, 24, W, 0.40f, 0.42f, 0.52f);

    addbox(-ms(8), 0, -ms(8), ms(8), 4, ms(8), 0.78f, 0.56f, 0.40f);   /* plateforme centrale */
    for (int i = 0; i < 10; i++) {                                  /* escaliers N et S */
        float h = 0.4f * (i + 1), z0 = ms(8 + (9 - i) * 0.6f), z1 = ms(8 + (10 - i) * 0.6f);
        float c = 0.62f + 0.012f * i;
        addbox(-ms(3), 0, z0, ms(3), h, z1, c, 0.5f, 0.38f);
        addbox(-ms(3), 0, -z1, ms(3), h, -z0, c, 0.5f, 0.38f);
    }
    addbox(-ms(2), 4, -ms(2), ms(2), 13, ms(2), 0.55f, 0.55f, 0.66f);   /* tour centrale */

    for (int sx = -1; sx <= 1; sx += 2)                             /* piliers */
        for (int sz = -1; sz <= 1; sz += 2)
            addbox(ms(sx * 24 - 2.0f), 0, ms(sz * 24 - 2.0f), ms(sx * 24 + 2.0f), 16, ms(sz * 24 + 2.0f), 0.5f, 0.5f, 0.62f);

    addbox(ms(28), 6, -ms(8), ms(44), 7, ms(8), 0.30f, 0.62f, 0.64f);   /* plateformes hautes */
    addbox(-ms(44), 6, -ms(8), -ms(28), 7, ms(8), 0.30f, 0.62f, 0.64f);
    addbox(-ms(8), 6, ms(28), ms(8), 7, ms(44), 0.30f, 0.62f, 0.64f);
    addbox(-ms(8), 6, -ms(44), ms(8), 7, -ms(28), 0.30f, 0.62f, 0.64f);

    for (int sx = -1; sx <= 1; sx += 2)                             /* perchoirs, accessibles au grappin */
        for (int sz = -1; sz <= 1; sz += 2)
            addbox(sx * ms(36) - 4, 12, sz * ms(36) - 4, sx * ms(36) + 4, 13, sz * ms(36) + 4, 0.85f, 0.7f, 0.35f);

    addbox(ms(30), 0, ms(38), ms(50), 8, ms(38) + 1, 0.75f, 0.42f, 0.62f);   /* murs de wall-run */
    addbox(-ms(50), 0, -ms(38) - 1, -ms(30), 8, -ms(38), 0.75f, 0.42f, 0.62f);
    addbox(ms(38), 0, -ms(50), ms(38) + 1, 8, -ms(30), 0.75f, 0.42f, 0.62f);
    addbox(-ms(38) - 1, 0, ms(30), -ms(38), 8, ms(50), 0.75f, 0.42f, 0.62f);

    addpad(ms(16), 0, v3(ms(13), 20, 0));                           /* jump pads */
    addpad(-ms(16), 0, v3(-ms(13), 20, 0));
    addpad(ms(9), ms(16), v3(-ms(1.8f), 20, ms(13)));
    addpad(-ms(9), -ms(16), v3(ms(1.8f), 20, -ms(13)));

    /* cellules : grandes aux endroits exposes (tour, perchoirs), petites ailleurs ; l'ordre fixe les index reseau */
    addpickup(0, 13, 0, PICKUP_BIG);                                /* sommet de la tour, */
    addpickup(36, 7, 0, PICKUP_SMALL); addpickup(-36, 7, 0, PICKUP_SMALL);   /* plateformes hautes, */
    addpickup(0, 7, 36, PICKUP_SMALL); addpickup(0, 7, -36, PICKUP_SMALL);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            addpickup(sx * 36.0f, 13, sz * 36.0f, PICKUP_BIG);      /* perchoirs */
            addpickup(sx * 46.0f, 0, sz * 46.0f, PICKUP_SMALL);     /* et coins au sol */
        }

    rng = 777;                                                      /* blocs de couverture */
    for (int k = 0, placed = 0; k < 400 && placed < 34; k++) {
        float x = frange(-ms(50), ms(50)), z = frange(-ms(50), ms(50));
        float w = frange(1.5f, 3.2f), d = frange(1.5f, 3.2f), h = frange(1.0f, 3.5f);
        if (fabsf(x) < ms(13) && fabsf(z) < ms(18)) continue;
        if (near2(x, z, ms(16), 0, 8) || near2(x, z, -ms(16), 0, 8) || near2(x, z, ms(9), ms(16), 8) || near2(x, z, -ms(9), -ms(16), 8)) continue;
        if (near2(x, z, ms(24), ms(24), 9) || near2(x, z, -ms(24), ms(24), 9) || near2(x, z, ms(24), -ms(24), 9) || near2(x, z, -ms(24), -ms(24), 9)) continue;
        if (near2(x, z, 0, ms(24), 6)) continue;
        if (fabsf(fabsf(x) - ms(38.5f)) < 6 && fabsf(z) > ms(26)) continue;
        if (fabsf(fabsf(z) - ms(38.5f)) < 6 && fabsf(x) > ms(26)) continue;
        if (fabsf(fabsf(x) - ms(46)) < 4 && fabsf(fabsf(z) - ms(46)) < 4) continue;   /* recharges des coins */
        if (fabsf(x + 20) < 6 && z > 12 && z < 70) continue;        /* couloir degage pour les tests automatiques */
        float t = frand();
        addbox(x - w, 0, z - d, x + w, h, z + d, 0.45f + 0.2f * t, 0.48f, 0.58f - 0.12f * t);
        placed++;
    }
    rng = (unsigned)GetTickCount();
}

static int box_overlap(V3 mn, V3 mx, const Box *b) {
    return mn.x < b->mx.x && mx.x > b->mn.x && mn.y < b->mx.y && mx.y > b->mn.y && mn.z < b->mx.z && mx.z > b->mn.z;
}

static int ray_box(V3 o, V3 d, V3 mn, V3 mx, float maxt, float *tout, V3 *nout) {
    float tmin = 0, tmax = maxt; int axis = -1; float sign = 0;
    for (int a = 0; a < 3; a++) {
        float oa = *vc(&o, a), da = *vc(&d, a), lo = *vc(&mn, a), hi = *vc(&mx, a);
        if (fabsf(da) < 1e-8f) {
            if (oa < lo || oa > hi) return 0;
        } else {
            float inv = 1.0f / da, t1 = (lo - oa) * inv, t2 = (hi - oa) * inv, s = -1;
            if (t1 > t2) { float tt = t1; t1 = t2; t2 = tt; s = 1; }
            if (t1 > tmin) { tmin = t1; axis = a; sign = s; }
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return 0;
        }
    }
    *tout = tmin;
    if (nout) { *nout = v3(0, 0, 0); if (axis >= 0) *vc(nout, axis) = sign; else *nout = vmul(d, -1); }
    return 1;
}

static int rayBox = -1;                                   /* boite touchee par le dernier ray_world (-1 : aucune) */
static float ray_world(V3 o, V3 d, float maxt, V3 *n) {
    float best = maxt; V3 bn = v3(0, 1, 0);
    rayBox = -1;
    for (int i = 0; i < nb; i++) {
        float t; V3 nn;
        if (ray_box(o, d, boxes[i].mn, boxes[i].mx, best, &t, &nn) && t < best) { best = t; bn = nn; rayBox = i; }
    }
    if (n) *n = bn;
    return best;
}

static int ray_sphere(V3 o, V3 d, V3 c, float r, float maxt, float *t) {
    V3 oc = vsub(o, c);
    float b = vdot(oc, d), cc = vdot(oc, oc) - r * r, h = b * b - cc;
    if (h < 0) return 0;
    h = sqrtf(h);
    float t0 = -b - h;
    if (t0 < 0) t0 = -b + h;
    if (t0 < 0 || t0 > maxt) return 0;
    *t = t0;
    return 1;
}

static int los(V3 a, V3 b) {
    V3 d = vsub(b, a); float l = vlen(d);
    if (l < 1e-4f) return 1;
    d = vmul(d, 1.0f / l);
    return ray_world(a, d, l, 0) >= l - 0.01f;
}

static void sphere_pushout(V3 *p, V3 *v, float r) {
    for (int i = 0; i < nb; i++) {
        Box *b = &boxes[i];
        if (p->x + r <= b->mn.x || p->x - r >= b->mx.x || p->y + r <= b->mn.y || p->y - r >= b->mx.y ||
            p->z + r <= b->mn.z || p->z - r >= b->mx.z) continue;
        float pen[6] = { p->x + r - b->mn.x, b->mx.x - (p->x - r), p->y + r - b->mn.y,
                         b->mx.y - (p->y - r), p->z + r - b->mn.z, b->mx.z - (p->z - r) };
        int k = 0;
        for (int j = 1; j < 6; j++) if (pen[j] < pen[k]) k = j;
        int ax = k / 2; float s = (k % 2) ? 1.0f : -1.0f;
        *vc(p, ax) += s * pen[k];
        if (*vc(v, ax) * s < 0) *vc(v, ax) = 0;
    }
}

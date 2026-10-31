/* VELOCITY - LA CITADELLE : ville fortifiee au soleil, pierre, enduit, tuiles et coupoles turquoise
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Plan (nord = -z) : spawn sud (z 50 a 68) et spawn nord (z -68 a    */
/* -50) ; trois couloirs : A a l'ouest (rue longue x -60..-46), centre */
/* (place du clocher), B a l'est (rue x 46..60). Site A : cour de la   */
/* fontaine, bordee d'une arcade ; site B : marche couvert a colonnes. */
/* Maisons en enduit avec toits de tuiles ou terrasses, remparts       */
/* crenelés (chemin de ronde a 10 m, escaliers), tours rondes aux      */
/* angles, pads de la place vers deux terrasses.                       */
/* Les boites donnent les collisions et les lightmaps ; le decor       */
/* maille (decor.c) ajoute colonnes, arches, toits, fenetres, arbres.  */
/* ------------------------------------------------------------------ */
static const V3 ctSand = { 0.88f, 0.76f, 0.6f }, ctWhite = { 0.93f, 0.91f, 0.86f }, ctOchre = { 0.88f, 0.64f, 0.42f };
static const V3 ctRose = { 0.9f, 0.7f, 0.62f }, ctStone = { 0.7f, 0.66f, 0.6f }, ctDark = { 0.5f, 0.46f, 0.42f };
static const V3 ctTile = { 0.78f, 0.36f, 0.22f }, ctTeal = { 0.16f, 0.6f, 0.62f }, ctBlue = { 0.16f, 0.34f, 0.66f };
static const V3 ctWood = { 0.47f, 0.3f, 0.18f }, ctGold = { 0.95f, 0.76f, 0.3f }, ctLeaf = { 0.27f, 0.5f, 0.24f };

static void ct_hidden(float x0, float y0, float z0, float x1, float y1, float z1) {
    addbox(x0, y0, z0, x1, y1, z1, 0.5f, 0.5f, 0.5f);
    boxes[nb - 1].kind = B_HIDDEN;
}
static void ct_box(float x0, float y0, float z0, float x1, float y1, float z1, V3 c) { addbox(x0, y0, z0, x1, y1, z1, c.x, c.y, c.z); }

/* caisse en bois : planches (bandes plus sombres), coins renforces ; collision invisible */
static void ct_crate(float x, float y, float z, float s) {
    float h = s * 0.5f;
    V3 w = dc_jit(ctWood, 0.12f);
    dc_bevel(v3(x - h, y, z - h), v3(x + h, y + s, z + h), 0.06f, w);
    for (int k = 0; k < 3; k++) {                         /* bandes de planches sur les 4 faces */
        float yy = y + s * (0.18f + 0.32f * k);
        dc_bevel(v3(x - h - 0.03f, yy, z - h - 0.03f), v3(x + h + 0.03f, yy + 0.05f, z + h + 0.03f), 0.01f, vmul(w, 0.7f));
    }
    for (int c = 0; c < 4; c++) {                         /* montants d'angle */
        float cx = x + (c & 1 ? h : -h), cz = z + (c & 2 ? h : -h);
        dc_bevel(v3(cx - 0.1f, y, cz - 0.1f), v3(cx + 0.1f, y + s, cz + 0.1f), 0.02f, vmul(w, 0.8f));
    }
    ct_hidden(x - h, y, z - h, x + h, y + s, z + h);
}
/* amphore : profil tourne */
static void ct_amphora(float x, float z, float k) {
    static const float pr[][2] = { { 0.0f, 0.0f }, { 0.18f, 0.02f }, { 0.34f, 0.25f }, { 0.4f, 0.55f }, { 0.36f, 0.8f },
                                   { 0.2f, 0.98f }, { 0.13f, 1.12f }, { 0.17f, 1.22f } };
    V3 c = dc_jit(v3(0.78f, 0.45f, 0.3f), 0.1f), b = v3(x, 0, z);
    for (int i = 0; i + 1 < (int)(sizeof pr / sizeof pr[0]); i++)
        dc_cone(vadd(b, v3(0, pr[i][1] * k, 0)), pr[i][0] * k, pr[i + 1][0] * k, (pr[i + 1][1] - pr[i][1]) * k, 12, c, 0, 0);
}
/* bac a fleurs en pierre avec un buisson rond */
static void ct_planter(float x, float z, float s) {
    dc_bevel(v3(x - s, 0, z - s), v3(x + s, 0.7f, z + s), 0.08f, ctStone);
    dc_sphere(v3(x, 0.8f, z), s * 0.85f, 0.7f, 10, 6, 0, dc_jit(ctLeaf, 0.15f));
    for (int k = 0; k < 5; k++) dc_sphere(v3(x + (dc_rand() - 0.5f) * s, 1.2f + dc_rand() * 0.3f, z + (dc_rand() - 0.5f) * s), 0.12f, 1, 6, 4, 0, dc_rand() < 0.5f ? v3(0.95f, 0.35f, 0.45f) : v3(0.98f, 0.85f, 0.3f));
    ct_hidden(x - s, 0, z - s, x + s, 1.3f, z + s);
}
/* muret de pierre (couvert bas) avec chaperon */
static void ct_lowwall(float x0, float z0, float x1, float z1, float h) {
    ct_box(x0, 0, z0, x1, h - 0.15f, z1, ctStone);
    dc_bevel(v3(x0 - 0.12f, h - 0.18f, z0 - 0.12f), v3(x1 + 0.12f, h, z1 + 0.12f), 0.05f, ctWhite);
}

/* maison : murs (boite), soubassement, corniche, fenetres a volets sur les 4 faces, porte, lanternes ;
 * roof 0 = terrasse (parapet + rambarde), 1 = tuiles faitage selon x, 2 = tuiles faitage selon z */
static void ct_house(float x0, float z0, float x1, float z1, float h, V3 wall, int roof, V3 shutter) {
    ct_box(x0, 0, z0, x1, h, z1, wall);
    dc_bevel(v3(x0 - 0.12f, 0, z0 - 0.12f), v3(x1 + 0.12f, 0.7f, z1 + 0.12f), 0.05f, ctStone);   /* soubassement */
    dc_bevel(v3(x0 - 0.3f, h - 0.35f, z0 - 0.3f), v3(x1 + 0.3f, h, z1 + 0.3f), 0.08f, ctWhite);   /* corniche */
    dc_bevel(v3(x0 - 0.18f, h - 0.6f, z0 - 0.18f), v3(x1 + 0.18f, h - 0.35f, z1 + 0.18f), 0.04f, vmul(ctWhite, 0.92f));
    int floors = h >= 7.5f ? 2 : 1;
    for (int face = 0; face < 4; face++) {                /* fenetres : +x, -x, +z, -z */
        V3 n = face == 0 ? v3(1, 0, 0) : face == 1 ? v3(-1, 0, 0) : face == 2 ? v3(0, 0, 1) : v3(0, 0, -1);
        float a = face < 2 ? z0 : x0, b = face < 2 ? z1 : x1, fixed = face == 0 ? x1 : face == 1 ? x0 : face == 2 ? z1 : z0;
        int cnt = (int)((b - a) / 3.6f);
        for (int fl = 0; fl < floors; fl++)
            for (int k = 0; k < cnt; k++) {
                float t = a + (b - a) * (k + 0.5f) / cnt, y = fl == 0 ? 2.3f : h - 2.9f;
                if (fl == 0 && face == 2 && k == cnt / 2) continue;   /* place de la porte */
                V3 c = face < 2 ? v3(fixed, y, t) : v3(t, y, fixed);
                dc_window(vadd(c, vmul(n, 0.02f)), n, 1.1f, 1.5f, ctWhite, shutter);
            }
        if (face == 2 && cnt > 0) {                       /* porte en bois sous un arc, marche de pierre */
            float t = a + (b - a) * (cnt / 2 + 0.5f) / cnt;
            dc_bevel(v3(t - 0.8f, 0, fixed - 0.05f), v3(t + 0.8f, 2.4f, fixed + 0.08f), 0.03f, ctWood);
            for (int p = 0; p < 4; p++) dc_bevel(v3(t - 0.75f + p * 0.4f, 0.1f, fixed + 0.07f), v3(t - 0.7f + p * 0.4f, 2.3f, fixed + 0.1f), 0.01f, vmul(ctWood, 0.75f));
            dc_arch(v3(t, 2.4f, fixed + 0.06f), 0, 0.8f, 0.25f, 0.2f, 10, ctStone);
            dc_bevel(v3(t - 1.1f, 0, fixed), v3(t + 1.1f, 0.18f, fixed + 0.6f), 0.04f, ctStone);
            dc_lantern(v3(t + 1.4f, 2.6f, fixed), n);
        }
    }
    if (roof == 0) {                                      /* terrasse : parapet bas et rambarde, dalles */
        ct_box(x0, h, z0, x1, h + 0.9f, z0 + 0.35f, wall); ct_box(x0, h, z1 - 0.35f, x1, h + 0.9f, z1, wall);
        ct_box(x0, h, z0 + 0.35f, x0 + 0.35f, h + 0.9f, z1 - 0.35f, wall); ct_box(x1 - 0.35f, h, z0 + 0.35f, x1, h + 0.9f, z1 - 0.35f, wall);
        dc_railing(v3(x0 + 0.4f, h + 0.9f, z0 + 0.17f), v3(x1 - 0.4f, h + 0.9f, z0 + 0.17f), 0.5f, ctWhite);
        dc_railing(v3(x0 + 0.4f, h + 0.9f, z1 - 0.17f), v3(x1 - 0.4f, h + 0.9f, z1 - 0.17f), 0.5f, ctWhite);
        dc_bevel(v3(x0 + 1.5f, h, z0 + 1.5f), v3(x0 + 3.0f, h + 0.5f, z0 + 2.4f), 0.05f, ctWood);   /* banc */
        ct_amphora(x1 - 1.2f, z1 - 1.2f, 1.1f);
        ct_amphora(x1 - 2.0f, z1 - 1.0f, 0.8f);
        dc_bevel(v3(x0 + 0.6f, h, z1 - 2.6f), v3(x0 + 1.8f, h + 1.0f, z1 - 0.6f), 0.06f, ctWhite);  /* cheminee */
    } else {                                              /* toit de tuiles : pente 40 % de la portee, collisions en gradins */
        int ax = roof == 1 ? 0 : 2;
        float span = ax == 0 ? z1 - z0 : x1 - x0, rh = span * 0.4f;
        dc_roof(v3(x0, h, z0), v3(x1, h, z1), rh, ax, 0.5f, ctTile, wall);
        for (int k = 1; k <= 3; k++) {                    /* gradins invisibles sous les pans (on peut marcher dessus) */
            float f = k / 4.0f, cut = span * 0.5f * f;
            if (ax == 0) ct_hidden(x0, h, z0 + cut, x1, h + rh * f, z1 - cut);
            else ct_hidden(x0 + cut, h, z0, x1 - cut, h + rh * f, z1);
        }
        if (ax == 0) dc_bevel(v3(x0 + 1.5f, h + rh * 0.5f, (z0 + z1) * 0.5f - 0.5f), v3(x0 + 2.5f, h + rh + 0.9f, (z0 + z1) * 0.5f + 0.5f), 0.05f, ctWhite);
        else dc_bevel(v3((x0 + x1) * 0.5f - 0.5f, h + rh * 0.5f, z0 + 1.5f), v3((x0 + x1) * 0.5f + 0.5f, h + rh + 0.9f, z0 + 2.5f), 0.05f, ctWhite);
    }
    dc_lantern(v3(x0 - 0.02f, h - 1.6f, z0 + 0.8f), v3(-1, 0, 0));
    dc_lantern(v3(x1 + 0.02f, h - 1.6f, z1 - 0.8f), v3(1, 0, 0));
}

/* arcade : colonnes le long de z en x = cx, arches entre elles, dalle au-dessus (collision), toit de tuiles */
static void ct_arcade(float cx, float z0, float z1, int bays, float colH, V3 col) {
    float step = (z1 - z0) / bays, r = 0.45f, rIn = step * 0.5f - 0.6f, w = 0.45f, top = colH + rIn + w;
    for (int i = 0; i <= bays; i++) {
        float z = z0 + step * i;
        dc_column(v3(cx, 0, z), colH, r, col);
        ct_hidden(cx - 0.6f, 0, z - 0.6f, cx + 0.6f, colH, z + 0.6f);
    }
    for (int i = 0; i < bays; i++) dc_arch_fill(v3(cx, colH, z0 + step * (i + 0.5f)), 2, rIn, w, 0.9f, 12, top + 0.6f, col);
    ct_box(cx - 3.5f, top + 0.6f, z0 - 1, cx + 0.6f, top + 1.3f, z1 + 1, ctWhite);
    dc_bevel(v3(cx - 3.6f, top + 1.3f, z0 - 1.1f), v3(cx + 0.75f, top + 1.55f, z1 + 1.1f), 0.06f, ctStone);
    dc_roof(v3(cx - 3.5f, top + 1.55f, z0 - 1), v3(cx + 0.6f, top + 1.55f, z1 + 1), 1.6f, 2, 0.4f, ctTile, ctWhite);
}

/* clocher de la place : fut carre, beffroi a 4 piliers et arches, cloche, tambour, coupole turquoise, fleche doree */
static void ct_belltower(float x, float z) {
    float H = 16, s = 4;
    ct_box(x - s, 0, z - s, x + s, H, z + s, ctSand);
    dc_bevel(v3(x - s - 0.2f, 0, z - s - 0.2f), v3(x + s + 0.2f, 1.2f, z + s + 0.2f), 0.08f, ctStone);
    for (int k = 1; k < 4; k++) dc_bevel(v3(x - s - 0.15f, k * 4.0f - 0.2f, z - s - 0.15f), v3(x + s + 0.15f, k * 4.0f + 0.1f, z + s + 0.15f), 0.05f, ctWhite);
    dc_bevel(v3(x - s - 0.4f, H - 0.4f, z - s - 0.4f), v3(x + s + 0.4f, H + 0.3f, z + s + 0.4f), 0.1f, ctWhite);
    for (int f = 0; f < 4; f++) {                         /* horloge sur deux faces, fenetres etroites sur les autres */
        V3 n = f == 0 ? v3(0, 0, 1) : f == 1 ? v3(0, 0, -1) : f == 2 ? v3(1, 0, 0) : v3(-1, 0, 0);
        V3 c = vadd(v3(x, 12.5f, z), vmul(n, s + 0.03f));
        if (f < 2) {
            V3 u = v3(-n.z, 0, n.x);
            for (int k = 0; k < 24; k++) {
                float a0 = 2 * PI * k / 24, a1 = 2 * PI * (k + 1) / 24;
                dc_tri(c, vadd(c, vadd(vmul(u, cosf(a1) * 1.6f), v3(0, sinf(a1) * 1.6f, 0))), vadd(c, vadd(vmul(u, cosf(a0) * 1.6f), v3(0, sinf(a0) * 1.6f, 0))), ctWhite);
                V3 r0 = vadd(c, vadd(vmul(u, cosf(a0) * 1.6f), v3(0, sinf(a0) * 1.6f, 0))), r1 = vadd(c, vadd(vmul(u, cosf(a1) * 1.6f), v3(0, sinf(a1) * 1.6f, 0)));
                dc_quad(r0, r1, vadd(r1, vmul(n, 0.15f)), vadd(r0, vmul(n, 0.15f)), ctGold);
            }
            for (int k = 0; k < 2; k++) {                 /* aiguilles */
                float a = k ? 1.2f : -0.5f, L = k ? 1.1f : 1.4f;
                V3 tip = vadd(vadd(c, vmul(n, 0.05f)), vadd(vmul(u, cosf(a) * L), v3(0, sinf(a) * L, 0)));
                dc_tri(vadd(vadd(c, vmul(n, 0.05f)), v3(0, 0.08f, 0)), tip, vadd(vadd(c, vmul(n, 0.05f)), v3(0, -0.08f, 0)), v3(0.1f, 0.1f, 0.12f));
            }
        } else dc_window(vadd(v3(x, 8.0f, z), vmul(n, s + 0.02f)), n, 0.9f, 2.0f, ctWhite, ctTeal);
    }
    for (int c = 0; c < 4; c++) {                         /* beffroi : 4 piliers (collision) */
        float px = x + (c & 1 ? s - 0.7f : -s + 0.7f), pz = z + (c & 2 ? s - 0.7f : -s + 0.7f);
        ct_box(px - 0.7f, H + 0.3f, pz - 0.7f, px + 0.7f, H + 5.2f, pz + 0.7f, ctSand);
    }
    for (int f = 0; f < 2; f++) {                         /* arches du beffroi sur les 4 cotes */
        dc_arch_fill(v3(x, H + 3.2f, z + (f ? s - 0.7f : -s + 0.7f)), 0, s - 1.4f, 0.35f, 1.4f, 12, H + 5.2f, ctSand);
        dc_arch_fill(v3(x + (f ? s - 0.7f : -s + 0.7f), H + 3.2f, z), 2, s - 1.4f, 0.35f, 1.4f, 12, H + 5.2f, ctSand);
    }
    ct_box(x - s, H + 5.2f, z - s, x + s, H + 6.0f, z + s, ctWhite);
    dc_cone(v3(x, H + 1.6f, z), 1.4f, 0.5f, 2.2f, 16, ctGold, 1, 1);   /* cloche */
    dc_sphere(v3(x, H + 1.5f, z), 0.25f, 1, 8, 6, 0, vmul(ctGold, 0.7f));
    dc_cyl(v3(x, H + 6.0f, z), 3.2f, 1.6f, 24, ctWhite);                 /* tambour */
    for (int k = 0; k < 12; k++) {                        /* petites fenetres du tambour */
        float a = 2 * PI * k / 12;
        V3 n = v3(cosf(a), 0, sinf(a));
        V3 p = vadd(v3(x, H + 6.8f, z), vmul(n, 3.22f)), u = v3(-n.z, 0, n.x);
        dc_quad(vadd(p, vadd(vmul(u, -0.2f), v3(0, -0.4f, 0))), vadd(p, vadd(vmul(u, 0.2f), v3(0, -0.4f, 0))),
                vadd(p, vadd(vmul(u, 0.2f), v3(0, 0.4f, 0))), vadd(p, vadd(vmul(u, -0.2f), v3(0, 0.4f, 0))), v3(0.1f, 0.12f, 0.16f));
    }
    dc_sphere(v3(x, H + 7.6f, z), 3.35f, 1.15f, 28, 16, 1, ctTeal);     /* coupole turquoise */
    for (int k = 0; k < 12; k++) {                        /* nervures dorees de la coupole */
        float a = 2 * PI * k / 12;
        for (int j = 0; j < 8; j++) {
            float t0 = PI * 0.5f * j / 8, t1 = PI * 0.5f * (j + 1) / 8, R = 3.4f;
            V3 p0 = v3(x + cosf(a) * cosf(t0) * R, H + 7.6f + sinf(t0) * R * 1.15f, z + sinf(a) * cosf(t0) * R);
            V3 p1 = v3(x + cosf(a) * cosf(t1) * R, H + 7.6f + sinf(t1) * R * 1.15f, z + sinf(a) * cosf(t1) * R);
            V3 sd = vmul(v3(-sinf(a), 0, cosf(a)), 0.08f);
            dc_quad(vsub(p0, sd), vadd(p0, sd), vadd(p1, sd), vsub(p1, sd), ctGold);
        }
    }
    ct_hidden(x - 3.2f, H + 6.0f, z - 3.2f, x + 3.2f, H + 10.5f, z + 3.2f);
    dc_cone(v3(x, H + 11.4f, z), 0.35f, 0.0f, 2.6f, 10, ctGold, 1, 0);  /* fleche */
    dc_sphere(v3(x, H + 11.6f, z), 0.4f, 1, 10, 6, 0, ctGold);
    for (int b = 0; b < 2; b++) {                         /* bannieres rayees qui ondulent */
        float bx = x + (b ? s + 0.05f : -s - 0.05f);
        for (int k = 0; k < 10; k++) {
            float y0 = H - 1 - k * 0.7f, y1 = y0 - 0.7f, w0 = 0.12f * sinf(k * 0.9f), w1 = 0.12f * sinf((k + 1) * 0.9f);
            V3 c = (k & 1) ? ctBlue : ctGold;
            float sx = b ? 1.0f : -1.0f;
            dc_quad(v3(bx + sx * (0.05f + w0), y0, z - 1.0f), v3(bx + sx * (0.05f + w0), y0, z + 1.0f),
                    v3(bx + sx * (0.05f + w1), y1, z + 1.0f), v3(bx + sx * (0.05f + w1), y1, z - 1.0f), c);
        }
    }
}

/* fontaine du site A : bassin rond, eau, vasque, jet */
static void ct_fountain(float x, float z) {
    V3 b = v3(x, 0, z);
    dc_cone(b, 4.3f, 4.1f, 0.9f, 32, ctStone, 0, 0);                     /* paroi exterieure */
    dc_cone(b, 3.6f, 3.6f, 0.9f, 32, vmul(ctStone, 0.85f), 0, 0);        /* paroi interieure */
    for (int k = 0; k < 32; k++) {                        /* margelle */
        float a0 = 2 * PI * k / 32, a1 = 2 * PI * (k + 1) / 32;
        V3 i0 = v3(x + cosf(a0) * 3.5f, 0.95f, z + sinf(a0) * 3.5f), i1 = v3(x + cosf(a1) * 3.5f, 0.95f, z + sinf(a1) * 3.5f);
        V3 o0 = v3(x + cosf(a0) * 4.35f, 0.95f, z + sinf(a0) * 4.35f), o1 = v3(x + cosf(a1) * 4.35f, 0.95f, z + sinf(a1) * 4.35f);
        dc_quad(i0, i1, o1, o0, ctWhite);
        V3 w0 = v3(x + cosf(a0) * 3.6f, 0.7f, z + sinf(a0) * 3.6f), w1 = v3(x + cosf(a1) * 3.6f, 0.7f, z + sinf(a1) * 3.6f);
        dc_tri(v3(x, 0.7f, z), w1, w0, v3(0.25f, 0.62f, 0.75f));          /* eau */
    }
    dc_cone(vadd(b, v3(0, 0.6f, 0)), 0.75f, 0.45f, 1.6f, 16, ctWhite, 0, 0);   /* pied */
    dc_cone(vadd(b, v3(0, 2.2f, 0)), 0.5f, 1.7f, 0.5f, 20, ctWhite, 1, 0);    /* vasque */
    dc_cone(vadd(b, v3(0, 2.7f, 0)), 1.5f, 1.5f, 0.02f, 20, v3(0.3f, 0.66f, 0.8f), 0, 1);
    dc_cone(vadd(b, v3(0, 2.7f, 0)), 0.25f, 0.18f, 0.9f, 10, ctTeal, 0, 0);
    dc_sphere(vadd(b, v3(0, 3.75f, 0)), 0.3f, 1.3f, 10, 8, 0, ctGold);
    for (int k = 0; k < 8; k++) {                         /* filets d'eau qui retombent */
        float a = 2 * PI * k / 8;
        V3 p0 = v3(x + cosf(a) * 1.5f, 2.72f, z + sinf(a) * 1.5f), p1 = v3(x + cosf(a) * 2.6f, 0.75f, z + sinf(a) * 2.6f);
        V3 s = v3(-sinf(a) * 0.08f, 0, cosf(a) * 0.08f), m = vadd(vlerp(p0, p1, 0.5f), v3(cosf(a) * 0.4f, 0.4f, sinf(a) * 0.4f));
        dc_quad(vsub(p0, s), vadd(p0, s), vadd(m, s), vsub(m, s), v3(0.6f, 0.85f, 0.95f));
        dc_quad(vsub(m, s), vadd(m, s), vadd(p1, s), vsub(p1, s), v3(0.6f, 0.85f, 0.95f));
    }
    ct_hidden(x - 3.9f, 0, z - 3.9f, x + 3.9f, 0.95f, z + 3.9f);
    ct_hidden(x - 0.7f, 0, z - 0.7f, x + 0.7f, 3.9f, z + 0.7f);
}

/* tour d'angle ronde : fut, couronne de merlons, toit conique de tuiles */
static void ct_corner_tower(float x, float z) {
    dc_cone(v3(x, 0, z), 6.4f, 5.8f, 13, 32, ctSand, 0, 0);
    for (int k = 1; k < 3; k++) dc_cone(v3(x, k * 4.5f, z), 6.25f - k * 0.2f, 6.25f - k * 0.2f, 0.3f, 32, ctWhite, 0, 0);
    dc_cone(v3(x, 13, z), 6.3f, 6.3f, 0.5f, 32, ctWhite, 0, 1);
    for (int k = 0; k < 16; k++) {
        float a = 2 * PI * k / 16;
        V3 p = v3(x + cosf(a) * 5.9f, 13.5f, z + sinf(a) * 5.9f);
        dc_bevel(v3(p.x - 0.5f, p.y, p.z - 0.5f), v3(p.x + 0.5f, p.y + 1.2f, p.z + 0.5f), 0.06f, ctSand);
    }
    dc_cone(v3(x, 13.5f, z), 4.8f, 0, 5.5f, 32, ctTile, 1, 0);
    dc_sphere(v3(x, 19.1f, z), 0.35f, 1, 8, 6, 0, ctGold);
    ct_hidden(x - 5.5f, 0, z - 5.5f, x + 5.5f, 14, z + 5.5f);
}

/* rempart : mur epais (chemin de ronde a 10 m), merlons sur le bord exterieur, garde-corps invisible */
static void ct_rampart(float x0, float z0, float x1, float z1, int outerSide) {
    ct_box(x0, 0, z0, x1, 10, z1, ctSand);
    dc_bevel(v3(x0 - 0.2f, 0, z0 - 0.2f), v3(x1 + 0.2f, 1.4f, z1 + 0.2f), 0.08f, ctStone);
    int alongX = x1 - x0 > z1 - z0;
    float L0 = alongX ? x0 : z0, L1 = alongX ? x1 : z1;
    for (float l = L0 + 1.2f; l < L1 - 0.6f; l += 2.4f) { /* merlons */
        float ox = alongX ? l : (outerSide > 0 ? x1 - 0.8f : x0), oz = alongX ? (outerSide > 0 ? z1 - 0.8f : z0) : l;
        if (alongX) dc_bevel(v3(ox - 0.6f, 10, oz), v3(ox + 0.6f, 11.4f, oz + 0.8f), 0.07f, dc_jit(ctSand, 0.04f));
        else dc_bevel(v3(ox, 10, oz - 0.6f), v3(ox + 0.8f, 11.4f, oz + 0.6f), 0.07f, dc_jit(ctSand, 0.04f));
    }
    if (alongX) ct_hidden(x0, 10, outerSide > 0 ? z1 - 0.8f : z0, x1, 11.4f, outerSide > 0 ? z1 : z0 + 0.8f);
    else ct_hidden(outerSide > 0 ? x1 - 0.8f : x0, 10, z0, outerSide > 0 ? x1 : x0 + 0.8f, 11.4f, z1);
    for (float l = L0 + 6; l < L1 - 6; l += 12) {         /* lanternes cote ville */
        V3 n = alongX ? v3(0, 0, outerSide > 0 ? -1.0f : 1.0f) : v3(outerSide > 0 ? -1.0f : 1.0f, 0, 0);
        V3 p = alongX ? v3(l, 6.5f, outerSide > 0 ? z0 : z1) : v3(outerSide > 0 ? x0 : x1, 6.5f, l);
        dc_lantern(p, n);
    }
}

/* escalier droit le long de x (dir = +1 ou -1) de y 0 a top, largeur z0..z1 */
static void ct_stairs(float x, float z0, float z1, int dir, float top) {
    int n = (int)(top / 0.8f + 0.5f);
    for (int k = 0; k < n; k++) {
        float xa = x + dir * k * 1.0f, xb = xa + dir * 1.0f, y = (k + 1) * top / n;
        ct_box(fminf(xa, xb), 0, z0, fmaxf(xa, xb), y, z1, ctStone);
    }
    dc_railing(v3(x, top / n, z0 + 0.15f), v3(x + dir * n, top, z0 + 0.15f), 0.9f, ctWhite);
}

/* paysage hors les murs (decor seul, sans collision) : collines qui montent au loin, mer au sud, oliviers et
 * cypres epars, montagnes a l'horizon. Evite le vide noir vu du chemin de ronde. */
static float ct_land_h(float x, float z) {
    float r = fmaxf(fabsf(x), fabsf(z)), k = clampf((r - 76) / 120, 0, 1);
    float hills = 9 * sinf(x * 0.021f + 1.3f) * cosf(z * 0.017f) + 6 * sinf(x * 0.047f + z * 0.031f) + 3 * sinf(z * 0.083f + 0.4f);
    float h = (hills + 14) * k * k - 1.5f;
    if (z > 90) h -= (z - 90) * 0.25f;                     /* la cote descend vers la mer */
    return h;
}
static void ct_landscape(void) {
    const float S = 14, R = 280;
    for (float x = -R; x < R; x += S)
        for (float z = -R; z < R; z += S) {
            if (fmaxf(fabsf(x + S * 0.5f), fabsf(z + S * 0.5f)) < 70) continue;   /* l'interieur des murs */
            V3 p[4] = { v3(x, 0, z), v3(x, 0, z + S), v3(x + S, 0, z + S), v3(x + S, 0, z) };
            for (int k = 0; k < 4; k++) p[k].y = ct_land_h(p[k].x, p[k].z);
            float hm = (p[0].y + p[1].y + p[2].y + p[3].y) * 0.25f, r = fmaxf(fabsf(x), fabsf(z));
            V3 c = hm < -2 ? v3(0.86f, 0.8f, 0.62f) : hm < 6 ? v3(0.72f, 0.66f, 0.42f) : v3(0.45f, 0.52f, 0.3f);   /* sable, herbe seche, garrigue */
            if (r > 200) c = vlerp(c, v3(0.5f, 0.56f, 0.62f), 0.4f);
            dc_quad(p[0], p[1], p[2], p[3], dc_jit(c, 0.06f));
        }
    for (float x = -R; x < R; x += 40)                    /* la mer, au sud */
        for (float z = 100; z < R; z += 40)
            dc_quad(v3(x, -3.0f, z), v3(x, -3.0f, z + 40), v3(x + 40, -3.0f, z + 40), v3(x + 40, -3.0f, z), dc_jit(v3(0.18f, 0.45f, 0.62f), 0.04f));
    for (int k = 0; k < 70; k++) {                        /* arbres hors les murs */
        float a = dc_rand() * 2 * PI, r = 84 + dc_rand() * 110, x = cosf(a) * r, z = sinf(a) * r;
        if (z > 95) continue;
        float y = ct_land_h(x, z);
        if (dc_rand() < 0.5f) dc_cypress(v3(x, y, z), 7 + dc_rand() * 5);
        else {                                            /* olivier : tronc court, feuillage gris-vert en boules */
            dc_cone(v3(x, y, z), 0.35f, 0.25f, 2.2f, 6, v3(0.4f, 0.32f, 0.24f), 0, 0);
            for (int b = 0; b < 4; b++)
                dc_sphere(v3(x + (dc_rand() - 0.5f) * 2.2f, y + 2.6f + dc_rand() * 0.8f, z + (dc_rand() - 0.5f) * 2.2f), 1.3f + dc_rand() * 0.5f, 0.75f, 8, 5, 0,
                          dc_jit(v3(0.42f, 0.5f, 0.36f), 0.08f));
        }
    }
    for (int k = 0; k < 9; k++) {                         /* montagnes a l'horizon, au nord */
        float x = -260 + k * 65 + dc_rand() * 20, z = -250 - dc_rand() * 40, h = 50 + dc_rand() * 50;
        dc_cone(v3(x, -5, z), 55 + dc_rand() * 25, 4, h, 10, dc_jit(v3(0.5f, 0.55f, 0.6f), 0.05f), 0, 1);
    }
}

static void build_map_citadelle(void) {
    ct_box(-72, -2, -72, 72, 0, 72, ctStone);             /* dallage */
    /* remparts (chemin de ronde a 10 m) et tours d'angle */
    ct_rampart(-72, -72, 72, -68, -1); ct_rampart(-72, 68, 72, 72, 1);
    ct_rampart(-72, -68, -68, 68, -1); ct_rampart(68, -68, 72, 68, 1);
    ct_corner_tower(-68, -68); ct_corner_tower(68, -68); ct_corner_tower(-68, 68); ct_corner_tower(68, 68);
    ct_stairs(-30, 64, 68, -1, 10); ct_stairs(30, -68, -64, 1, 10);   /* escaliers vers le chemin de ronde */
    /* grande porte (decor) dans le rempart sud, cote ville */
    dc_arch_fill(v3(0, 4.5f, 67.6f), 0, 4.0f, 0.9f, 1.2f, 16, 10, ctWhite);
    dc_bevel(v3(-4.0f, 0, 67.3f), v3(4.0f, 8.5f, 67.9f), 0.05f, ctWood);
    dc_column(v3(-5.4f, 0, 67.2f), 4.5f, 0.5f, ctWhite); dc_column(v3(5.4f, 0, 67.2f), 4.5f, 0.5f, ctWhite);
    /* maisons : bloc ouest, bloc est, nord, sud */
    ct_house(-44, 6, -30, 20, 9, ctSand, 1, ctTeal);
    ct_house(-28, 4, -16, 18, 11, ctWhite, 0, ctBlue);
    ct_house(-46, 26, -32, 42, 8, ctOchre, 2, ctTeal);
    ct_house(-30, 24, -16, 40, 10, ctRose, 1, ctBlue);
    ct_house(16, 4, 28, 18, 10, ctWhite, 0, ctTeal);
    ct_house(30, 6, 44, 20, 8, ctOchre, 1, ctBlue);
    ct_house(16, 24, 30, 40, 9, ctSand, 2, v3(0.3f, 0.55f, 0.3f));
    ct_house(32, 26, 46, 42, 11, ctRose, 1, ctTeal);
    ct_house(-30, -68, -12, -56, 10, ctSand, 1, ctBlue);
    ct_house(12, -68, 30, -56, 9, ctWhite, 0, ctTeal);
    ct_house(-68, -52, -59, -16, 9, ctOchre, 2, ctTeal);
    ct_house(56, -52, 68, -16, 10, ctSand, 2, ctBlue);
    ct_house(-42, 48, -24, 62, 9, ctWhite, 0, ctTeal);
    ct_house(24, 48, 42, 62, 8, ctRose, 1, ctBlue);
    ct_house(-68, 16, -60, 46, 8, ctSand, 2, v3(0.7f, 0.25f, 0.2f));
    ct_house(60, 16, 68, 46, 9, ctWhite, 2, ctTeal);
    /* place centrale : clocher, pads vers les terrasses, palmiers, bancs */
    ct_belltower(0, -8);
    addpad_to(-10, 0, 14, v3(-22, 11, 11), 16);
    addpad_to(10, 0, 14, v3(22, 10, 11), 16);
    dc_palm(v3(-9, 0, 30), 7.5f, 1); dc_palm(v3(9, 0, 30), 7, -1); ct_hidden(-9.3f, 0, 29.7f, -8.7f, 6, 30.3f); ct_hidden(8.7f, 0, 29.7f, 9.3f, 6, 30.3f);
    ct_planter(-9, 30, 1.2f); ct_planter(9, 30, 1.2f);
    ct_lowwall(-6, 22, 6, 22.8f, 1.2f);                  /* muret au milieu de la place */
    ct_crate(-4, 0, 2, 1.4f); ct_crate(-2.5f, 0, 2.6f, 1.1f); ct_crate(-4, 1.4f, 2, 1.0f);
    ct_crate(4, 0, -1, 1.4f);
    dc_bevel(v3(-12, 0, 40), v3(-9, 0.5f, 41), 0.05f, ctWood); dc_bevel(v3(9, 0, 40), v3(12, 0.5f, 41), 0.05f, ctWood);
    /* site A : cour de la fontaine, arcade a l'ouest, couverts */
    ct_fountain(-36, -34);
    ct_arcade(-55, -50, -18, 5, 4.2f, ctWhite);
    ct_lowwall(-30, -46, -22, -45.2f, 1.2f); ct_lowwall(-48, -22, -40, -21.2f, 1.2f);
    ct_crate(-26, 0, -26, 1.5f); ct_crate(-24.4f, 0, -26.2f, 1.2f); ct_crate(-26, 1.5f, -26, 1.1f);
    ct_crate(-46, 0, -42, 1.5f); ct_crate(-46, 0, -40.4f, 1.3f);
    dc_cypress(v3(-48, 0, -48), 8); dc_cypress(v3(-24, 0, -48), 7); ct_hidden(-48.3f, 0, -48.3f, -47.7f, 7, -47.7f); ct_hidden(-24.3f, 0, -48.3f, -23.7f, 6, -47.7f);
    ct_planter(-24, -48, 1.0f); ct_planter(-48, -48, 1.0f);
    for (int k = 0; k < 4; k++) ct_amphora(-50 + k * 0.9f, -20.5f, 0.9f + 0.2f * (k & 1));
    /* site B : marche couvert, 12 colonnes, grand toit, etals */
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++) {
            float cx = 24 + 9.0f * i, cz = -44 + 10.0f * j;
            dc_column(v3(cx, 0, cz), 5.5f, 0.5f, ctWhite);
            ct_hidden(cx - 0.65f, 0, cz - 0.65f, cx + 0.65f, 5.5f, cz + 0.65f);
        }
    for (int j = 0; j < 3; j++) dc_bevel(v3(22, 5.5f, -44.5f + 10 * j), v3(52, 6.1f, -43.5f + 10 * j), 0.06f, ctWood);   /* poutres */
    ct_box(20, 6.1f, -48, 56, 6.8f, -20, ctWhite);
    dc_roof(v3(20, 6.8f, -48), v3(56, 6.8f, -20), 4.0f, 0, 0.8f, ctTile, ctWhite);
    for (int k = 1; k <= 3; k++) ct_hidden(20, 6.8f, -48 + 14 * k / 4.0f, 56, 6.8f + 4 * k / 4.0f, -20 - 14 * k / 4.0f);
    for (int s = 0; s < 4; s++) {                         /* etals : table, cagettes de fruits */
        float sx = 28 + 9.0f * (s % 3), sz = s < 3 ? -39.0f : -29.0f;
        dc_bevel(v3(sx - 1.6f, 0.9f, sz - 0.7f), v3(sx + 1.6f, 1.05f, sz + 0.7f), 0.03f, ctWood);
        for (int l = 0; l < 4; l++) dc_bevel(v3(sx - 1.5f + (l & 1) * 2.8f, 0, sz - 0.6f + (l >> 1) * 1.0f), v3(sx - 1.3f + (l & 1) * 2.8f, 0.9f, sz - 0.4f + (l >> 1) * 1.0f), 0.02f, ctWood);
        for (int c = 0; c < 3; c++) {
            float bx = sx - 1.0f + c * 1.0f;
            dc_bevel(v3(bx - 0.4f, 1.05f, sz - 0.4f), v3(bx + 0.4f, 1.35f, sz + 0.4f), 0.02f, vmul(ctWood, 1.2f));
            V3 fc = c == 0 ? v3(0.95f, 0.5f, 0.12f) : c == 1 ? v3(0.85f, 0.15f, 0.15f) : v3(0.55f, 0.75f, 0.2f);
            for (int f = 0; f < 6; f++) dc_sphere(v3(bx - 0.22f + (f % 3) * 0.22f, 1.45f, sz - 0.15f + (f / 3) * 0.3f), 0.13f, 1, 7, 5, 0, dc_jit(fc, 0.1f));
        }
        ct_hidden(sx - 1.6f, 0, sz - 0.7f, sx + 1.6f, 1.5f, sz + 0.7f);
    }
    ct_crate(50, 0, -26, 1.5f); ct_crate(50, 1.5f, -26, 1.2f); ct_crate(25, 0, -24, 1.4f);
    /* rue B (est) : etals avec auvents rayes contre les maisons */
    dc_awning(v3(60, 3.2f, 22), v3(60, 3.2f, 30), v3(-1, 0, 0), 2.2f, 0.9f, ctTeal, ctWhite);
    dc_awning(v3(44, 3.0f, 8), v3(44, 3.0f, 16), v3(1, 0, 0), 2.0f, 0.8f, v3(0.85f, 0.3f, 0.25f), ctWhite);
    dc_awning(v3(-44, 3.0f, 10), v3(-44, 3.0f, 18), v3(-1, 0, 0), 2.0f, 0.8f, ctBlue, ctWhite);
    dc_awning(v3(-16, 3.0f, 8), v3(-16, 3.0f, 15), v3(1, 0, 0), 2.0f, 0.8f, v3(0.9f, 0.6f, 0.2f), ctWhite);
    ct_crate(53, 0, -8, 1.4f); ct_crate(48, 0, 12, 1.3f); ct_crate(-48, 0, 14, 1.3f); ct_crate(-55, 0, 2, 1.5f);
    /* arches au-dessus des rues (decor + piliers) */
    for (int s = -1; s <= 1; s += 2) {
        float cx = s * 53.0f;
        dc_arch_fill(v3(cx, 5.0f, -12), 0, 6.2f, 0.8f, 1.6f, 18, 13, ctSand);
        ct_box(cx - 8, 0, -12.8f, cx - 6.2f, 5.0f, -11.2f, ctSand); ct_box(cx + 6.2f, 0, -12.8f, cx + 8, 5.0f, -11.2f, ctSand);
        ct_box(cx - 8, 11.0f, -12.8f, cx + 8, 13.0f, -11.2f, ctSand);
    }
    /* spawns : nord (fontaine murale et cypres), sud (palmiers) */
    dc_cypress(v3(-8, 0, -62), 7); dc_cypress(v3(8, 0, -62), 7); ct_hidden(-8.3f, 0, -62.3f, -7.7f, 6, -61.7f); ct_hidden(7.7f, 0, -62.3f, 8.3f, 6, -61.7f);
    ct_planter(-8, -62, 1.1f); ct_planter(8, -62, 1.1f);
    dc_palm(v3(-16, 0, 52), 8, 1); dc_palm(v3(16, 0, 52), 8, -1); ct_hidden(-16.3f, 0, 51.7f, -15.7f, 7, 52.3f); ct_hidden(15.7f, 0, 51.7f, 16.3f, 7, 52.3f);
    ct_planter(-16, 52, 1.2f); ct_planter(16, 52, 1.2f);
    for (int k = 0; k < 6; k++) ct_amphora(-3 + k * 1.2f, 66.4f, 0.9f + 0.25f * (k % 3));
    ct_lowwall(-14, -44, -4, -43.2f, 1.2f); ct_lowwall(4, -44, 14, -43.2f, 1.2f);   /* couverts du milieu nord */
    /* recharges */
    addcell(0, 0, 36, PICKUP_SMALL); addcell(-36, 0, -27, PICKUP_BIG); addcell(38, 0, -34, PICKUP_BIG);
    addcell(-53, 0, 30, PICKUP_SMALL); addcell(53, 0, 30, PICKUP_SMALL); addcell(-22, 11, 11, PICKUP_SMALL); addcell(22, 10, 11, PICKUP_SMALL);
    addcell(0, 0, -30, PICKUP_SMALL); addcell(-40, 10, 70, PICKUP_SMALL); addcell(40, 10, -70, PICKUP_SMALL);
    ct_landscape();
}

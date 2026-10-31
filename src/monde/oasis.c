/* VELOCITY - L'OASIS : carte tactique en equipes, sans grappin, symetrique (BLEUS au nord, ROUGES au sud)
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Pensee pour les modes d'equipe : chaque camp a sa salle de depart   */
/* fermee (toit, 3 sorties ; l'adversaire y brule), une cour avec son  */
/* drapeau (et le bouclier des BLEUS), puis 3 couloirs vers le milieu  */
/* ou sont les 3 zones : A a l'ouest (ruines a colonnade, a decouvert),*/
/* B au centre (place surelevee d'un temple en ruine), C a l'est       */
/* (bazar sous les tentes, couloirs etroits). Deux tours de guet par   */
/* escalier donnent la hauteur. Pas de grappin : on joue les angles.   */
/* Tout ce qui est au sud est le miroir du nord (z -> -z).             */
/* ------------------------------------------------------------------ */
static const V3 oaSand = { 0.82f, 0.68f, 0.48f }, oaStone = { 0.78f, 0.66f, 0.5f }, oaPale = { 0.95f, 0.88f, 0.74f };
static const V3 oaTeal = { 0.15f, 0.55f, 0.58f }, oaRed = { 0.75f, 0.25f, 0.2f }, oaCream = { 0.94f, 0.9f, 0.8f };

/* tente du bazar : 4 poteaux, toile rayee qui fait le creux, etal dessous ; collision : poteaux et etal */
static void oa_tent(float x, float z, float w, float d, V3 c1) {
    for (int k = 0; k < 4; k++) {
        float px = x + (k & 1 ? w : -w), pz = z + (k & 2 ? d : -d);
        dc_cyl(v3(px, 0, pz), 0.12f, 3.2f, 6, ctWood);
        ct_hidden(px - 0.15f, 0, pz - 0.15f, px + 0.15f, 3.2f, pz + 0.15f);
    }
    dc_awning(v3(x - w, 3.3f, z - d), v3(x + w, 3.3f, z - d), v3(0, 0, 1), d * 2, 0.4f, c1, oaCream);
    dc_bevel(v3(x - w * 0.7f, 0, z - 0.6f), v3(x + w * 0.7f, 1.0f, z + 0.6f), 0.04f, ctWood);
    for (int k = 0; k < 5; k++) dc_sphere(v3(x - w * 0.5f + k * w * 0.25f, 1.15f, z), 0.18f, 1, 7, 5, 0, dc_jit(k & 1 ? v3(0.9f, 0.55f, 0.15f) : v3(0.7f, 0.2f, 0.25f), 0.1f));
    ct_hidden(x - w * 0.7f, 0, z - 0.6f, x + w * 0.7f, 1.0f, z + 0.6f);
}
/* tour de guet : fut en pierre, escalier exterieur, plate-forme a 6 m avec garde-corps a merlons */
static void oa_watchtower(float x, float z, int dir) {
    ct_box(x - 3, 0, z - 3, x + 3, 6, z + 3, oaStone);
    dc_bevel(v3(x - 3.3f, 5.6f, z - 3.3f), v3(x + 3.3f, 6.2f, z + 3.3f), 0.08f, oaPale);
    for (int k = 0; k < 8; k++) {                         /* escalier le long de la face est/ouest */
        float zz = z - 3 + k * 0.85f, h = (k + 1) * 0.75f;
        ct_box(x + dir * 3, 0, zz, x + dir * 4.6f, h, zz + 0.85f, oaPale);
    }
    for (int s = 0; s < 4; s++) {                         /* garde-corps en merlons */
        for (int k = 0; k < 3; k++) {
            float t = -2.2f + k * 2.2f;
            float mx = s < 2 ? x + t : x + (s == 2 ? -2.75f : 2.75f), mz = s < 2 ? z + (s == 0 ? -2.75f : 2.75f) : z + t;
            if (s == 3 - (dir > 0) && k == 0) continue;   /* passage de l'escalier */
            ct_box(mx - 0.45f, 6, mz - 0.45f, mx + 0.45f, 7.1f, mz + 0.45f, oaSand);
        }
    }
    dc_awning(v3(x - 3, 9.0f, z - 3), v3(x + 3, 9.0f, z - 3), v3(0, 0, 1), 6, 0.5f, oaTeal, oaCream);
    for (int k = 0; k < 4; k++) dc_cyl(v3(x + (k & 1 ? 2.8f : -2.8f), 6, z + (k & 2 ? 2.8f : -2.8f)), 0.1f, 3.0f, 6, ctWood);
}
/* bassin de l'oasis : margelle, eau, palmiers autour */
static void oa_pool(float x, float z, float r) {
    dc_cone(v3(x, 0, z), r + 0.5f, r + 0.4f, 0.6f, 28, oaStone, 0, 0);
    for (int k = 0; k < 28; k++) {
        float a0 = 2 * PI * k / 28, a1 = 2 * PI * (k + 1) / 28;
        dc_quad(v3(x + cosf(a0) * r, 0.6f, z + sinf(a0) * r), v3(x + cosf(a1) * r, 0.6f, z + sinf(a1) * r),
                v3(x + cosf(a1) * (r + 0.55f), 0.6f, z + sinf(a1) * (r + 0.55f)), v3(x + cosf(a0) * (r + 0.55f), 0.6f, z + sinf(a0) * (r + 0.55f)), oaPale);
        dc_tri(v3(x, 0.45f, z), v3(x + cosf(a1) * r, 0.45f, z + sinf(a1) * r), v3(x + cosf(a0) * r, 0.45f, z + sinf(a0) * r), v3(0.2f, 0.55f, 0.6f));
    }
    ct_hidden(x - r * 0.8f, 0, z - r * 0.8f, x + r * 0.8f, 0.6f, z + r * 0.8f);
}

/* la moitie nord (BLEUS) ; appelee deux fois, la seconde en miroir (s = -1) */
static void oa_half(float s) {
#define Z(z) ((z) * s)
#define ZB(a, b) (s > 0 ? (a) : -(b)), (s > 0 ? (b) : -(a))
    /* salle de depart : murs, toit, 3 sorties vers la cour */
    float zr0 = Z(-74), zr1 = Z(-60);
    ct_box(-16, 0, fminf(zr0, zr1), -14, 7, fmaxf(zr0, zr1), oaSand);
    ct_box(14, 0, fminf(zr0, zr1), 16, 7, fmaxf(zr0, zr1), oaSand);
    float fz0 = fminf(Z(-62), Z(-60)), fz1 = fmaxf(Z(-62), Z(-60));
    ct_box(-14, 0, fz0, -11, 7, fz1, oaSand); ct_box(-7, 0, fz0, -2.5f, 7, fz1, oaSand);
    ct_box(2.5f, 0, fz0, 7, 7, fz1, oaSand); ct_box(11, 0, fz0, 14, 7, fz1, oaSand);
    ct_box(-11, 3.4f, fz0, -7, 7, fz1, oaSand); ct_box(-2.5f, 3.4f, fz0, 2.5f, 7, fz1, oaSand); ct_box(7, 3.4f, fz0, 11, 7, fz1, oaSand);
    ct_box(-16, 7, fminf(Z(-74), Z(-60)), 16, 8, fmaxf(Z(-74), Z(-60)), oaPale);   /* toit */
    for (int k = -1; k <= 1; k++) dc_arch(v3(k * 9.0f, 3.4f - 1.9f, Z(-60) + 0.05f * s), 0, 1.9f, 0.3f, 2.2f, 10, oaPale);
    dc_lantern(v3(-4.6f, 4.5f, Z(-60)), v3(0, 0, s)); dc_lantern(v3(4.6f, 4.5f, Z(-60)), v3(0, 0, s));
    /* cour de la base : murets de couverture, palmiers, caisses */
    ct_lowwall(-12, fminf(Z(-48), Z(-47.2f)), -4, fmaxf(Z(-48), Z(-47.2f)), 1.2f);
    ct_lowwall(4, fminf(Z(-48), Z(-47.2f)), 12, fmaxf(Z(-48), Z(-47.2f)), 1.2f);
    ct_crate(-20, 0, Z(-52), 1.5f); ct_crate(-18.5f, 0, Z(-52.2f), 1.2f); ct_crate(20, 0, Z(-50), 1.5f);
    dc_palm(v3(-24, 0, Z(-58)), 7, 1); ct_hidden(-24.3f, 0, fminf(Z(-58.3f), Z(-57.7f)), -23.7f, 6, fmaxf(Z(-58.3f), Z(-57.7f)));
    dc_palm(v3(24, 0, Z(-58)), 7.5f, -1); ct_hidden(23.7f, 0, fminf(Z(-58.3f), Z(-57.7f)), 24.3f, 6, fmaxf(Z(-58.3f), Z(-57.7f)));
    /* blocs qui separent les couloirs : maisons a terrasse */
    float a0 = fminf(Z(-40), Z(-14)), a1 = fmaxf(Z(-40), Z(-14));
    ct_house(-30, a0, -12, a1, 7, oaSand, 0, oaTeal);
    ct_house(12, a0, 30, a1, 7, oaStone, 0, oaRed);
    float b0 = fminf(Z(-56), Z(-44)), b1 = fmaxf(Z(-56), Z(-44));
    ct_house(-46, b0, -34, b1, 8, oaPale, 2, oaTeal);
    ct_house(34, b0, 46, b1, 8, oaSand, 2, oaRed);
    /* couloir A (ouest) : colonnade en ruine, pans de mur */
    for (int k = 0; k < 4; k++) {
        float zc = Z(-36 + k * 8.0f), hcol = k == 2 ? 2.5f : 4.5f;   /* une colonne brisee */
        dc_column(v3(-46, 0, zc), hcol, 0.5f, oaPale);
        ct_hidden(-46.65f, 0, zc - 0.65f, -45.35f, hcol, zc + 0.65f);
    }
    ct_box(-52, 0, fminf(Z(-24), Z(-18)), -50.5f, 3.2f, fmaxf(Z(-24), Z(-18)), oaStone);   /* pan de mur */
    ct_box(-40, 0, fminf(Z(-12), Z(-9)), -36, 1.3f, fmaxf(Z(-12), Z(-9)), oaStone);          /* muret pres de la zone A */
    ct_crate(-50, 0, Z(-8), 1.5f);
    /* couloir C (est) : bazar */
    oa_tent(40, Z(-30), 3, 2.2f, oaRed); oa_tent(48, Z(-20), 2.5f, 2, oaTeal);
    oa_tent(38, Z(-16), 2.5f, 2, v3(0.85f, 0.6f, 0.15f));
    ct_crate(52, 0, Z(-34), 1.4f); ct_crate(52, 0, Z(-32.5f), 1.2f); ct_crate(34, 0, Z(-24), 1.3f);
    for (int k = 0; k < 3; k++) ct_amphora(44 + k * 0.9f, Z(-38), 0.9f + 0.2f * (k & 1));
    /* place centrale : marches d'acces nord/sud */
    for (int k = 0; k < 3; k++) {
        float zz = Z(-13.0f + k * 1.0f), h = 0.4f * (k + 1);
        ct_box(-4, 0, fminf(zz, zz + s), 4, h, fmaxf(zz, zz + s), oaPale);
    }
#undef Z
#undef ZB
}

static void build_map_oasis(void) {
    ct_box(-58, -2, -76, 58, 0, 76, oaSand);              /* sable */
    ct_box(-58, 0, -76, -56, 12, 76, oaStone); ct_box(56, 0, -76, 58, 12, 76, oaStone);   /* enceinte */
    ct_box(-56, 0, -76, 56, 12, -74, oaStone); ct_box(-56, 0, 74, 56, 12, 76, oaStone);
    for (float x = -54; x <= 54; x += 3.6f) {             /* merlons de l'enceinte */
        dc_bevel(v3(x - 0.7f, 12, -76), v3(x + 0.7f, 13.2f, -74.8f), 0.06f, oaSand);
        dc_bevel(v3(x - 0.7f, 12, 74.8f), v3(x + 0.7f, 13.2f, 76), 0.06f, oaSand);
    }
    for (float z = -72; z <= 72; z += 3.6f) {
        dc_bevel(v3(-58, 12, z - 0.7f), v3(-56.8f, 13.2f, z + 0.7f), 0.06f, oaSand);
        dc_bevel(v3(56.8f, 12, z - 0.7f), v3(58, 13.2f, z + 0.7f), 0.06f, oaSand);
    }
    oa_half(1); oa_half(-1);
    /* milieu : place surelevee du temple (zone B), tours de guet, bassins */
    ct_box(-10, 0, -10, 10, 1.2f, 10, oaPale);
    dc_bevel(v3(-10.2f, 1.0f, -10.2f), v3(10.2f, 1.3f, 10.2f), 0.06f, oaStone);
    for (int c = 0; c < 4; c++) {                         /* temple en ruine : 4 colonnes, linteaux sur deux cotes */
        float px = c & 1 ? 8 : -8, pz = c & 2 ? 8 : -8;
        dc_column(v3(px, 1.2f, pz), 5.5f, 0.6f, oaPale);
        ct_hidden(px - 0.8f, 1.2f, pz - 0.8f, px + 0.8f, 6.7f, pz + 0.8f);
    }
    ct_box(-9, 6.7f, -8.8f, 9, 7.6f, -7.2f, oaStone); ct_box(-9, 6.7f, 7.2f, 9, 7.6f, 8.8f, oaStone);
    ct_box(-3, 1.2f, -0.6f, 3, 2.4f, 0.6f, oaStone);      /* autel : couvert au centre */
    dc_bevel(v3(-3.2f, 2.3f, -0.8f), v3(3.2f, 2.6f, 0.8f), 0.05f, oaPale);
    oa_watchtower(-22, 0, 1); oa_watchtower(22, 0, -1);
    oa_pool(-40, 26, 3.2f); oa_pool(-40, -26, 3.2f);
    dc_palm(v3(-44, 0, 26), 7, 1); dc_palm(v3(-44, 0, -26), 7, 1);
    oa_tent(40, 0, 4, 3, v3(0.85f, 0.6f, 0.15f));         /* zone C : grand auvent */
    ct_crate(36, 0, 4, 1.4f); ct_crate(44, 0, -4, 1.4f);
    ct_box(-40, 0, -1, -36, 1.3f, 1, oaStone);            /* zone A : muret central */
    addcell(0, 1.2f, 4, PICKUP_SMALL); addcell(-50, 0, 0, PICKUP_SMALL); addcell(52, 0, 0, PICKUP_SMALL);
    addcell(-22, 6, 0, PICKUP_BIG); addcell(22, 6, 0, PICKUP_BIG);
    /* dehors : dunes et palmiers (decor seul) */
    for (float x = -260; x < 260; x += 16)
        for (float z = -260; z < 260; z += 16) {
            if (fabsf(x + 8) < 60 && fabsf(z + 8) < 78) continue;
            V3 p[4] = { v3(x, 0, z), v3(x, 0, z + 16), v3(x + 16, 0, z + 16), v3(x + 16, 0, z) };
            for (int k = 0; k < 4; k++) {
                float r = fmaxf(fabsf(p[k].x) - 56, fabsf(p[k].z) - 74), q = clampf(r / 80, 0, 1);
                p[k].y = (6 * sinf(p[k].x * 0.03f + p[k].z * 0.012f) + 4 * sinf(p[k].z * 0.05f) + 8) * q * q - 0.5f;
            }
            dc_quad(p[0], p[1], p[2], p[3], dc_jit(v3(0.9f, 0.76f, 0.54f), 0.05f));
        }
    for (int k = 0; k < 40; k++) {
        float a = dc_rand() * 2 * PI, r = 95 + dc_rand() * 90;
        V3 b = v3(cosf(a) * r, 2.0f, sinf(a) * r);
        if (dc_rand() < 0.7f) dc_palm(b, 6 + dc_rand() * 4, dc_rand() * 2 - 1);
    }
}

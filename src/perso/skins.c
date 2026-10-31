/* VELOCITY - skins des joueurs : modeles et animation
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ---- skins : modeles des joueurs ----------------------------------------------------------
 * Pieds en (0, 0, 0), regard vers -z, taille 1,8 m (mise a l'echelle pour l'accroupi). La couleur
 * principale c est celle de l'equipe (ou du joueur en chacun pour soi). Les accents fixes (bec,
 * pattes, acier) gardent leur couleur. skinFlat : tout d'une couleur (contour) ; skinFlash : touche. */
typedef struct { float run, spd, air, slide, pitch, t; } SkinAnim;
static int skinFlat;
static float skinFlatCol[3], skinFlash;
static void skc(float r, float g, float b) {
    if (skinFlat) { glColor3f(skinFlatCol[0], skinFlatCol[1], skinFlatCol[2]); return; }
    float k = clampf(skinFlash, 0, 1);
    glColor3f(r + (1 - r) * k, g + (1 - g) * k, b + (1 - b) * k);
}
static void skcv(const float *c, float m) { skc(fminf(1, c[0] * m), fminf(1, c[1] * m), fminf(1, c[2] * m)); }
static void skmix(const float *c, float w) { skc(c[0] + (1 - c[0]) * w, c[1] + (1 - c[1]) * w, c[2] + (1 - c[2]) * w); }
static void bxs(float x0, float y0, float z0, float x1, float y1, float z1) { box_faces(v3(x0, y0, z0), v3(x1, y1, z1)); }
static void emit_on(void) { if (!skinFlat) glDisable(GL_LIGHTING); }   /* parties lumineuses : sans eclairage */
static void emit_off(void) { if (!skinFlat) glEnable(GL_LIGHTING); }

/* jambe : pivot a la hanche (x, hip), angle en degres (positif = vers l'avant) */
static void skin_leg(float x, float hip, float ang, float w, float len) {
    glPushMatrix();
    glTranslatef(x, hip, 0);
    glRotatef(-ang, 1, 0, 0);
    bxs(-w, -len, -w, w, 0, w);
    glPopMatrix();
}
static float leg_angle(const SkinAnim *a, int side) {   /* course, saut, glissade */
    if (a->slide > 0.5f) return 70;
    if (a->air > 0.5f) return side ? 30.0f : -20.0f;
    return sinf(a->run + (side ? PI : 0)) * 38 * a->spd;
}
/* bras : droit vers l'avant (tient l'arme, suit la visee), gauche qui balance */
static void skin_arms(const float *armCol, float armM, float sh, float sx, float w, const SkinAnim *a) {
    glPushMatrix();
    glTranslatef(sx, sh, 0);
    glRotatef(a->pitch * 180 / PI - 8, 1, 0, 0);
    skcv(armCol, armM);
    bxs(-w, -w, -0.5f, w, w, 0.05f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-sx, sh, 0);
    glRotatef(a->air > 0.5f ? 150.0f : -sinf(a->run) * 30 * a->spd, 1, 0, 0);
    skcv(armCol, armM);
    bxs(-w, -0.6f, -w, w, 0.04f, w);
    glPopMatrix();
}

static void skin_robot(const float *c, const SkinAnim *a) {
    static const float steel[3] = { 0.28f, 0.29f, 0.34f };
    skcv(c, 0.45f);
    skin_leg(0.14f, 0.8f, leg_angle(a, 0), 0.09f, 0.8f);
    skin_leg(-0.14f, 0.8f, leg_angle(a, 1), 0.09f, 0.8f);
    skcv(steel, 1); bxs(-0.25f, 0.72f, -0.16f, 0.25f, 0.86f, 0.16f);
    skcv(c, 0.8f); bxs(-0.32f, 0.85f, -0.2f, 0.32f, 1.38f, 0.2f);
    skcv(c, 1.0f); bxs(-0.44f, 1.22f, -0.17f, -0.3f, 1.4f, 0.17f); bxs(0.3f, 1.22f, -0.17f, 0.44f, 1.4f, 0.17f);
    skcv(c, 1.0f); bxs(-0.22f, 1.4f, -0.22f, 0.22f, 1.8f, 0.22f);
    skcv(steel, 1); bxs(0.11f, 1.8f, -0.02f, 0.14f, 1.98f, 0.01f);
    emit_on();
    skc(0.75f, 0.95f, 1.0f); bxs(-0.18f, 1.55f, -0.245f, 0.18f, 1.66f, -0.2f);   /* visiere */
    skmix(c, 0.5f); bxs(-0.07f, 1.05f, -0.215f, 0.07f, 1.19f, -0.2f);           /* coeur lumineux */
    bxs(0.095f, 1.97f, -0.035f, 0.155f, 2.03f, 0.025f);                           /* bout d'antenne */
    emit_off();
    skin_arms(c, 0.6f, 1.3f, 0.38f, 0.07f, a);
}

static void skin_poussin(const float *c, const SkinAnim *a) {
    static const float orange[3] = { 1.0f, 0.58f, 0.1f };
    skcv(orange, 1);
    skin_leg(0.12f, 0.45f, leg_angle(a, 0), 0.035f, 0.42f);
    skin_leg(-0.12f, 0.45f, leg_angle(a, 1), 0.035f, 0.42f);
    bxs(0.04f, 0, -0.16f, 0.2f, 0.035f, 0.04f); bxs(-0.2f, 0, -0.16f, -0.04f, 0.035f, 0.04f);   /* pattes */
    skcv(c, 1.0f);                                          /* corps rond : trois boites croisees */
    bxs(-0.42f, 0.52f, -0.34f, 0.42f, 1.18f, 0.34f);
    bxs(-0.35f, 0.44f, -0.42f, 0.35f, 1.26f, 0.42f);
    bxs(-0.3f, 0.4f, -0.3f, 0.3f, 1.3f, 0.3f);
    skmix(c, 0.55f); bxs(-0.24f, 0.56f, -0.435f, 0.24f, 1.02f, -0.41f);   /* ventre clair */
    skcv(c, 1.08f);                                         /* tete */
    bxs(-0.28f, 1.24f, -0.32f, 0.28f, 1.76f, 0.22f);
    bxs(-0.23f, 1.2f, -0.36f, 0.23f, 1.8f, 0.26f);
    for (int k = 0; k < 3; k++) {                           /* plumes du dessus */
        glPushMatrix();
        glTranslatef(-0.06f + k * 0.06f, 1.8f, -0.02f + k * 0.03f);
        glRotatef(-25 + k * 25.0f, 0, 0, 1);
        skcv(c, 1.2f); bxs(-0.025f, 0, -0.025f, 0.025f, 0.16f, 0.025f);
        glPopMatrix();
    }
    skc(0.05f, 0.05f, 0.07f); bxs(-0.19f, 1.52f, -0.37f, -0.08f, 1.64f, -0.35f); bxs(0.08f, 1.52f, -0.37f, 0.19f, 1.64f, -0.35f);
    emit_on(); skc(1, 1, 1); bxs(-0.16f, 1.6f, -0.375f, -0.13f, 1.63f, -0.365f); bxs(0.11f, 1.6f, -0.375f, 0.14f, 1.63f, -0.365f); emit_off();
    skcv(orange, 1);                                        /* bec */
    bxs(-0.09f, 1.42f, -0.5f, 0.09f, 1.5f, -0.34f);
    bxs(-0.06f, 1.37f, -0.45f, 0.06f, 1.42f, -0.34f);
    float flap = a->air > 0.5f ? sinf(a->t * 22) * 45 : sinf(a->run) * 10 * a->spd;   /* ailes : battent en l'air */
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * 0.42f, 1.05f, 0.02f);
        glRotatef(side * (15 + flap), 0, 0, 1);
        skcv(c, 0.85f); bxs(side < 0 ? -0.08f : 0, -0.38f, -0.2f, side < 0 ? 0 : 0.08f, 0.02f, 0.22f);
        glPopMatrix();
    }
    skin_arms(c, 0.85f, 1.15f, 0.46f, 0.05f, a);
}

static void skin_ninja(const float *c, const SkinAnim *a) {
    float suit[3] = { c[0] * 0.3f + 0.05f, c[1] * 0.3f + 0.05f, c[2] * 0.3f + 0.06f };
    static const float skin[3] = { 0.93f, 0.76f, 0.62f };
    skcv(suit, 1);
    skin_leg(0.12f, 0.82f, leg_angle(a, 0), 0.075f, 0.82f);
    skin_leg(-0.12f, 0.82f, leg_angle(a, 1), 0.075f, 0.82f);
    bxs(-0.27f, 0.82f, -0.17f, 0.27f, 1.38f, 0.17f);
    skcv(c, 1.0f); bxs(-0.28f, 0.8f, -0.18f, 0.28f, 0.9f, 0.18f);         /* ceinture */
    glPushMatrix(); glTranslatef(0, 1.1f, 0.19f); glRotatef(35, 0, 0, 1);  /* fourreau dans le dos */
    skc(0.12f, 0.1f, 0.1f); bxs(-0.03f, -0.45f, -0.02f, 0.03f, 0.45f, 0.03f); glPopMatrix();
    skcv(suit, 1); bxs(-0.2f, 1.4f, -0.2f, 0.2f, 1.78f, 0.2f);
    skcv(skin, 1); bxs(-0.19f, 1.55f, -0.21f, 0.19f, 1.64f, -0.19f);      /* bande des yeux */
    skc(0.05f, 0.05f, 0.05f); bxs(-0.13f, 1.58f, -0.215f, -0.06f, 1.61f, -0.205f); bxs(0.06f, 1.58f, -0.215f, 0.13f, 1.61f, -0.205f);
    skcv(c, 1.0f); bxs(-0.21f, 1.66f, -0.21f, 0.21f, 1.72f, 0.21f);       /* bandeau */
    float lift = 75 - 60 * a->spd;                          /* rubans : pendent a l'arret, flottent a pleine vitesse */
    for (int k = 0; k < 2; k++) {
        glPushMatrix();
        glTranslatef(k ? 0.05f : -0.05f, 1.69f, 0.21f);
        glRotatef(lift + sinf(a->t * 11 + k * 1.7f) * 9, 1, 0, 0);
        glRotatef(k ? 8.0f : -8.0f, 0, 1, 0);
        skcv(c, 1.0f); bxs(-0.03f, -0.01f, 0, 0.03f, 0.01f, k ? 0.5f : 0.6f);
        glPopMatrix();
    }
    skin_arms(suit, 1, 1.3f, 0.34f, 0.06f, a);
}

static void skin_chat(const float *c, const SkinAnim *a) {
    static const float pink[3] = { 1.0f, 0.55f, 0.65f };
    skcv(c, 0.8f);
    skin_leg(0.13f, 0.8f, leg_angle(a, 0), 0.085f, 0.8f);
    skin_leg(-0.13f, 0.8f, leg_angle(a, 1), 0.085f, 0.8f);
    skmix(c, 0.75f); bxs(0.04f, 0, -0.12f, 0.22f, 0.06f, 0.08f); bxs(-0.22f, 0, -0.12f, -0.04f, 0.06f, 0.08f);   /* pattes */
    skcv(c, 1.0f); bxs(-0.3f, 0.8f, -0.2f, 0.3f, 1.34f, 0.2f);
    skmix(c, 0.7f); bxs(-0.17f, 0.86f, -0.21f, 0.17f, 1.24f, -0.2f);      /* ventre */
    skcv(c, 1.05f); bxs(-0.25f, 1.36f, -0.23f, 0.25f, 1.76f, 0.22f);       /* tete */
    for (int side = -1; side <= 1; side += 2) {             /* oreilles : losanges a moitie enfonces */
        glPushMatrix();
        glTranslatef(side * 0.15f, 1.76f, 0);
        glRotatef(45, 0, 0, 1);
        skcv(c, 1.05f); bxs(-0.08f, -0.08f, -0.04f, 0.08f, 0.08f, 0.04f);
        skcv(pink, 1); bxs(-0.045f, -0.045f, -0.045f, 0.045f, 0.045f, -0.035f);
        glPopMatrix();
    }
    skmix(c, 0.8f); bxs(-0.12f, 1.4f, -0.27f, 0.12f, 1.52f, -0.22f);       /* museau */
    skcv(pink, 1); bxs(-0.035f, 1.5f, -0.28f, 0.035f, 1.54f, -0.265f);
    emit_on(); skc(0.45f, 1.0f, 0.4f); bxs(-0.15f, 1.58f, -0.24f, -0.06f, 1.65f, -0.225f); bxs(0.06f, 1.58f, -0.24f, 0.15f, 1.65f, -0.225f);
    if (!skinFlat) {                                        /* moustaches */
        glColor3f(0.95f, 0.95f, 0.95f); glLineWidth(1.5f);
        glBegin(GL_LINES);
        for (int s2 = -1; s2 <= 1; s2 += 2)
            for (int k = 0; k < 3; k++) { glVertex3f(s2 * 0.1f, 1.46f, -0.28f); glVertex3f(s2 * 0.34f, 1.42f + k * 0.04f, -0.25f); }
        glEnd();
    }
    emit_off();
    for (int k = 0; k < 6; k++) {                           /* queue qui ondule, remonte en arriere */
        float sway = sinf(a->t * 3 + k * 0.6f) * 0.06f * k;
        skcv(c, k == 5 ? 1.2f : 0.95f);
        float y = 0.95f + k * 0.11f, z = 0.22f + k * 0.07f - (k > 3 ? (k - 3) * 0.05f : 0);
        bxs(sway - 0.05f, y, z, sway + 0.05f, y + 0.12f, z + 0.1f);
    }
    skin_arms(c, 0.85f, 1.3f, 0.37f, 0.065f, a);
}

static void skin_chevalier(const float *c, const SkinAnim *a) {
    static const float steel[3] = { 0.62f, 0.64f, 0.7f }, dark[3] = { 0.3f, 0.31f, 0.36f };
    skcv(steel, 0.85f);
    skin_leg(0.14f, 0.8f, leg_angle(a, 0), 0.095f, 0.8f);
    skin_leg(-0.14f, 0.8f, leg_angle(a, 1), 0.095f, 0.8f);
    skcv(steel, 1); bxs(-0.33f, 0.82f, -0.21f, 0.33f, 1.38f, 0.21f);
    skcv(c, 0.9f);                                          /* tabard de l'equipe, devant et derriere */
    bxs(-0.24f, 0.55f, -0.235f, 0.24f, 1.32f, -0.21f); bxs(-0.24f, 0.55f, 0.21f, 0.24f, 1.32f, 0.235f);
    skmix(c, 0.6f); bxs(-0.03f, 0.85f, -0.24f, 0.03f, 1.25f, -0.235f); bxs(-0.12f, 1.1f, -0.24f, 0.12f, 1.16f, -0.235f);   /* croix */
    skcv(c, 1.0f); bxs(-0.5f, 1.24f, -0.21f, -0.3f, 1.44f, 0.21f); bxs(0.3f, 1.24f, -0.21f, 0.5f, 1.44f, 0.21f);   /* epaulieres */
    skcv(steel, 1.05f); bxs(-0.23f, 1.38f, -0.23f, 0.23f, 1.82f, 0.23f);   /* heaume */
    skcv(dark, 1); bxs(-0.18f, 1.58f, -0.24f, 0.18f, 1.62f, -0.225f); bxs(-0.02f, 1.42f, -0.24f, 0.02f, 1.58f, -0.225f);
    skcv(c, 1.15f);                                         /* cimier en plumes, retombe derriere */
    bxs(-0.035f, 1.82f, -0.18f, 0.035f, 1.96f, 0.22f);
    bxs(-0.03f, 1.62f, 0.22f, 0.03f, 1.94f, 0.36f);
    skin_arms(steel, 0.85f, 1.3f, 0.4f, 0.075f, a);
}

static void draw_skin(int skin, float h, const float *c, const SkinAnim *a) {
    glPushMatrix();
    glScalef(1, h / H_STAND, 1);
    switch (skin) {
    case SKN_POUSSIN: skin_poussin(c, a); break;
    case SKN_NINJA: skin_ninja(c, a); break;
    case SKN_CHAT: skin_chat(c, a); break;
    case SKN_CHEVALIER: skin_chevalier(c, a); break;
    default: skin_robot(c, a); break;
    }
    glPopMatrix();
}

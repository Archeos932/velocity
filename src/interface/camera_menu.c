/* VELOCITY - camera du menu principal : plans qui s'enchainent sur la carte choisie (orbite, survol, au sol,
   travelling), fondu au noir entre deux plans, bouton VUE pour passer au suivant.
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c).

   Les plans se calculent seuls pour chaque carte : centre et etendue d'apres les points de depart, hauteur du
   decor par des rayons verticaux (la camera reste au-dessus des toits, jamais dans une salle fermee). */
enum { MCAM_ORBIT, MCAM_SURVOL, MCAM_SOL, MCAM_TRAVEL, MCAM_COUNT };
static const char *mcamNames[MCAM_COUNT] = { "ORBITE", "SURVOL", "AU SOL", "TRAVELLING" };
static int mcamShot, mcamMap = -1, mcamVar;               /* plan en cours ; variante (quel depart, quel cote) */
static float mcamT, mcamYaw0, menuFade;                   /* temps dans le plan ; voile noir des fondus (render) */
static V3 mcamCenter;
static float mcamRadius, mcamTop;

/* hauteur du dessus du decor en (x, z) : premier obstacle en descendant depuis le ciel (0 si rien) */
static float mcam_ground(float x, float z) {
    float d = ray_world(v3(x, 300, z), v3(0, -1, 0), 600, NULL);
    return d >= 599 ? 0 : 300 - d;
}
static V3 mcam_spawn(int k) {
    const MapDef *m = &maps[curMap];
    if (m->nspawns <= 0) return m->menuCam;
    k %= m->nspawns;
    return v3(m->spawns[k][0], m->spawns[k][1], m->spawns[k][2]);
}
static void mcam_setup(void) {                            /* centre, rayon et hauteur du decor de la carte */
    const MapDef *m = &maps[curMap];
    V3 c = v3(0, 0, 0);
    int n = m->nspawns > 0 ? m->nspawns : 1;
    for (int k = 0; k < n; k++) c = vadd(c, mcam_spawn(k));
    c = vmul(c, 1.0f / n);
    mcamRadius = 30;
    for (int k = 0; k < n; k++) mcamRadius = fmaxf(mcamRadius, hlen(vsub(mcam_spawn(k), c)) + 12);
    mcamTop = 0;
    for (int i = -4; i <= 4; i++)
        for (int j = -4; j <= 4; j++) mcamTop = fmaxf(mcamTop, mcam_ground(c.x + i * mcamRadius / 4, c.z + j * mcamRadius / 4));
    mcamTop = fminf(mcamTop, 80);
    c.y = mcam_ground(c.x, c.z);
    mcamCenter = c;
}
static void mcam_look(V3 eye, V3 at, float *yaw, float *pitch) {
    V3 d = vsub(at, eye);
    float l = vlen(d);
    *yaw = atan2f(d.x, -d.z);
    *pitch = l > 1e-3f ? asinf(clampf(d.y / l, -1, 1)) : 0;
}
static float mcam_smooth(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }

/* position de l'oeil et orientation du plan en cours, au temps t */
static void mcam_pose(V3 *eye, float *yaw, float *pitch) {
    const MapDef *m = &maps[curMap];
    float t = mcamT;
    switch (mcamShot) {
    case MCAM_SURVOL: {                                   /* tres haut, tourne lentement autour du centre */
        float a = mcamVar * 1.7f + t * 0.07f, r = mcamRadius * 0.85f;
        *eye = v3(mcamCenter.x + cosf(a) * r, mcamTop + 24, mcamCenter.z + sinf(a) * r);
        mcam_look(*eye, mcamCenter, yaw, pitch);
        break;
    }
    case MCAM_SOL: {                                      /* a hauteur d'homme, vers le centre, leger panoramique */
        V3 s = mcam_spawn(mcamVar), best = v3(s.x, mcam_ground(s.x, s.z), s.z);
        for (int k = 1; k <= 3; k++) {                    /* un point d'ou l'on voit le centre (pas derriere un mur) */
            V3 p = vlerp(s, mcamCenter, 0.15f * k);
            V3 e = v3(p.x, mcam_ground(p.x, p.z) + 1.7f, p.z), to = vadd(mcamCenter, v3(0, 2, 0)), d = vsub(to, e);
            float l = vlen(d);
            if (l > 1 && ray_world(e, vmul(d, 1 / l), l, NULL) > l * 0.6f) { best = v3(p.x, e.y - 1.7f, p.z); break; }
        }
        *eye = vadd(best, v3(0, 1.7f, 0));
        mcam_look(*eye, vadd(mcamCenter, v3(0, 2, 0)), yaw, pitch);
        *yaw += sinf(t * 0.35f) * 0.3f;
        *pitch += 0.04f;
        break;
    }
    case MCAM_TRAVEL: {                                   /* d'un depart vers le plus lointain, au-dessus du decor */
        V3 a = mcam_spawn(mcamVar), b = a;
        float far = 0;
        for (int k = 0; k < (m->nspawns > 0 ? m->nspawns : 1); k++)
            if (hlen(vsub(mcam_spawn(k), a)) > far) { far = hlen(vsub(mcam_spawn(k), a)); b = mcam_spawn(k); }
        if (far < 10) b = vadd(mcamCenter, vsub(mcamCenter, a));
        float h = a.y + 8;                                /* marge : les toits mailles depassent un peu des boites */
        for (int k = 0; k <= 24; k++) { V3 p = vlerp(a, b, k / 24.0f); h = fmaxf(h, mcam_ground(p.x, p.z) + 10); }
        V3 p = vlerp(a, b, 0.1f + 0.8f * mcam_smooth(t / MENU_SHOT_T));
        *eye = v3(p.x, h, p.z);
        mcam_look(*eye, v3(b.x, h - 16, b.z), yaw, pitch);
        break;
    }
    default:                                              /* ORBITE : la vue d'avant, qui tourne doucement */
        *eye = vadd(m->menuCam, v3(0, P.eyeH, 0));
        *yaw = mcamYaw0 + t * 0.05f;
        *pitch = m->menuPitch;
        break;
    }
}

static void menu_cam_next(void) {                         /* plan suivant (bouton VUE, ou fin du plan) */
    mcamShot = (mcamShot + 1) % MCAM_COUNT;
    if (mcamShot == MCAM_ORBIT) mcamVar++;                /* a chaque tour : d'autres departs, un autre cote */
    if (mcamShot == MCAM_ORBIT) mcamYaw0 += 1.2f;
    mcamT = 0;
}

/* a chaque image hors partie (boucle principale) : place le joueur-camera P */
static void menu_cam_tick(float dt) {
    if (curMap != mcamMap) { mcamMap = curMap; mcam_setup(); mcamShot = MCAM_ORBIT; mcamT = 0; mcamYaw0 = P.yaw; }
    mcamT += dt;
    if (mcamT > MENU_SHOT_T) menu_cam_next();
    menuFade = fmaxf(clampf(1 - mcamT / MENU_FADE_T, 0, 1), clampf((mcamT - (MENU_SHOT_T - MENU_FADE_T)) / MENU_FADE_T, 0, 1));
    V3 eye;
    float yaw, pitch;
    mcam_pose(&eye, &yaw, &pitch);
    if (point_in_box(eye)) eye.y = mcam_ground(eye.x, eye.z) + 4;   /* garde-fou : jamais dans un bloc (tour fine...) */
    P.pos = P.prevPos = vsub(eye, v3(0, P.eyeH, 0));
    P.vel = v3(0, 0, 0);
    P.yaw = yaw; P.pitch = clampf(pitch, -1.5f, 1.5f); P.roll = 0;
}

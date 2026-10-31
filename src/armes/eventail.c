/* VELOCITY - Eventail (fusil a pompe)
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* --- Eventail (fusil a pompe) ------------------------------------------- */
/* 10 plombs a motif deterministe (generateur local : le rng global n'est jamais touche, chaque client
 * recalcule les memes directions a partir de la graine d'EV_SHOT). Les degats sont agreges par cible :
 * un seul net_hit par joueur et par tir. Recul en l'air plafonne, dispersion divisee par 2 en slide,
 * un plomb qui touche une piece la fait eclater (souffle de 3 m, ni degats ni recul sur soi). */
static unsigned int sgCounter;                            /* compteur de graines */
static int sgBursts;                                      /* pieces eclatees au dernier tir (tests, journal) */
static int sgShots, sgPelletSum, sgStreakShots, sgJumps, sgBurstSum;   /* journal du bot */

static float sg_rand(unsigned int *s) { *s = *s * 1664525u + 1013904223u; return (*s >> 8) * (1.0f / 16777216.0f); }
static unsigned int sg_new_seed(void) {                   /* unique, deterministe, sans toucher rng */
    return (((unsigned int)myId + 1) << 24) ^ ((++sgCounter) * 2654435761u);
}
/* motif semi-fixe : 1 plomb au centre, 3 sur un anneau a 0.45, 6 sur un anneau a 0.9, tourne par la graine */
static void sg_pellet_dirs(V3 d, float half, unsigned int seed, V3 *out) {
    unsigned int s = seed;
    V3 r = vnorm(vcross(d, v3(0, 1, 0.01f))), u = vcross(r, d);   /* meme astuce que fire_rail : jamais nul */
    float rot = sg_rand(&s) * 2 * PI, th = tanf(half);
    for (int k = 0; k < SG_PELLETS; k++) {
        float rho, ang;
        if (k == 0) { rho = 0; ang = 0; }
        else if (k < 4) { rho = 0.45f; ang = rot + (k - 1) * (2 * PI / 3); }
        else { rho = 0.9f; ang = rot + PI / 6 + (k - 4) * (PI / 3); }
        rho += (sg_rand(&s) - 0.5f) * 0.2f;               /* +-0.1 */
        ang += (sg_rand(&s) - 0.5f) * 0.35f;              /* +-10 deg */
        if (rho > 1) rho = 1;
        out[k] = vnorm(vadd(d, vadd(vmul(r, cosf(ang) * rho * th), vmul(u, sinf(ang) * rho * th))));
    }
}
static float sg_falloff(float t) {                        /* plein jusqu'a 10 m, puis lineaire jusqu'a x0.3 a 30 m */
    if (t <= SG_NEAR) return 1;
    if (t >= SG_FAR) return SG_FAR_MULT;
    return 1 - (1 - SG_FAR_MULT) * (t - SG_NEAR) / (SG_FAR - SG_NEAR);
}
static V3 sg_tint(float k) { return vlerp(v3(1, 0.6f, 0.25f), v3(1, 0.9f, 0.4f), clampf(k, 0, 1)); }   /* cuivre -> or */

/* cumul d'un tir : degats, recul, plombs et dernier point touche par cible */
typedef struct {
    float e[MAXE], p[MAXP];
    V3 ke[MAXE], kp[MAXP], he[MAXE], hp[MAXP];
    int ne[MAXE], np[MAXP], burstHit;
} SgAcc;

/* eclat de piece : mine = 1 chez le tireur (impact frame, hitstop), 0 pour celui d'un autre joueur */
static void sg_burst_fx(V3 o, int mine) {
    burst(o, v3(1, 0.82f, 0.3f), 40, 10, 0.6f, 0.18f);
    shards(o, v3(1, 0.82f, 0.3f), 6, 7);
    play(S_EXPLO, 0.6f * (mine ? 1 : atten(o)), 1.7f);
    if (mine) { impact_frame(o, 0.05f); hitstop = fmaxf(hitstop, 0.04f); }
}

static void sg_coin_burst(int ci, int depth, SgAcc *A) {
    Coin *c = &CO[ci];
    if (!c->alive) return;                                /* deja eclatee par un autre plomb */
    V3 o = c->pos;
    c->alive = 0; sgBursts++;
    if (dmMode) net_fx_coin(1, c->netId, o, v3(1, 0, 0));   /* f[3] = 1 : detruite EN ECLAT */
    for (int i = 0; i < MAXE; i++) {
        if (!E[i].alive) continue;
        V3 dv = vsub(E[i].pos, o);
        float dd = vlen(dv), R = SG_BURST_RADIUS + enemy_radius(E[i].type);
        if (dd < R) {
            float f = 1 - dd / R;
            A->e[i] += SG_BURST_DMG * (0.3f + 0.7f * f);
            A->ke[i] = vadd(A->ke[i], vmul(vnorm(dv), SG_BURST_KNOCK * f)); A->burstHit = 1;
        }
    }
    for (int i = 0; i < MAXP; i++) {
        if (!np_target(i)) continue;
        V3 dv = vsub(np_center(i), o);
        float dd = vlen(dv), R = SG_BURST_RADIUS + 0.5f;
        if (dd < R) {
            float f = 1 - dd / R;
            A->p[i] += SG_BURST_DMG * (0.3f + 0.7f * f);
            A->kp[i] = vadd(A->kp[i], vmul(vnorm(dv), SG_BURST_KNOCK * f + 2)); A->burstHit = 1;
        }
    }
    /* soi : ni degats ni recul (comme explode byPlayer == 2) */
    sg_burst_fx(o, 1);
    for (int j = 0; depth < SG_BURST_CHAIN && j < MAXCOIN; j++)   /* reaction en chaine */
        if (CO[j].alive && vlen(vsub(CO[j].pos, o)) < SG_BURST_RADIUS && los(o, CO[j].pos))
            sg_coin_burst(j, depth + 1, A);
}

/* shotgun jump : 7 m/s a l'oppose de la visee ; peut freiner, ou pousser jusqu'a un seuil, jamais au-dela
 * de la vitesse qu'on avait deja (9 m/s vertical, 16 m/s horizontal) */
static void sg_recoil(V3 d) {
    V3 v0 = P.vel, v = vadd(P.vel, vmul(d, -SG_RECOIL));
    if (v.y > v0.y && v.y > fmaxf(v0.y, SG_RECOIL_VMAX)) v.y = fmaxf(v0.y, SG_RECOIL_VMAX);
    float h0 = hlen(v0), h = hlen(v), lim = fmaxf(h0, SG_RECOIL_HCAP);
    if (h > lim) { v.x *= lim / h; v.z *= lim / h; }
    if (d.y < -0.7f) sgJumps++;                           /* stat : shotgun jump */
    P.vel = v;
}

static void fire_shotgun(void) {
    if (P.ammo[WP_SHOTGUN] > 0) P.ammo[WP_SHOTGUN]--;
    V3 eye = eye_pos(), d = view_dir(), dir[SG_PELLETS], end[SG_PELLETS], nrm[SG_PELLETS];
    float half = SG_HALF_ANGLE * (P.sliding ? SG_SLIDE_SPREAD : 1), mult = streak_mult();
    unsigned int seed = sg_new_seed();
    static SgAcc A;
    int pellets = 0, burstList[SG_PELLETS], nBurst = 0, onWall[SG_PELLETS];
    int mineList[SG_PELLETS], nMineHit = 0;               /* Ventouse : mines touchees (une explosion par mine) */
    memset(&A, 0, sizeof A);
    sg_pellet_dirs(d, half, seed, dir);
    for (int k = 0; k < SG_PELLETS; k++) {                /* le plomb s'arrete sur le plus proche : piece, drone ou joueur */
        float tw = ray_world(eye, dir[k], SG_RANGE, &nrm[k]), t;
        int ci = -1, te = -1, tp = -1;
        onWall[k] = tw < SG_RANGE;
        for (int i = 0; i < MAXCOIN; i++)                 /* toute piece, la mienne ou non */
            if (CO[i].alive && ray_sphere(eye, dir[k], CO[i].pos, SG_COIN_RADIUS, tw, &t)) { tw = t; ci = i; }
        for (int i = 0; i < MAXE; i++)
            if (E[i].alive && ray_sphere(eye, dir[k], E[i].pos, enemy_radius(E[i].type), tw, &t)) { tw = t; te = i; ci = -1; }
        for (int i = 0; i < MAXP; i++) {
            V3 mn, mx;
            if (!np_target(i)) continue;
            np_aabb(i, 0.05f, &mn, &mx);
            if (ray_box(eye, dir[k], mn, mx, tw, &t, NULL) && t < tw) { tw = t; tp = i; te = ci = -1; }
        }
        int mi = -1;                                      /* --- Ventouse : le plomb s'arrete sur une mine (x1) --- */
        for (int i = 0; i < MAXPR; i++)
            if (mine_shootable(&PR[i]) && ray_sphere(eye, dir[k], PR[i].pos, MINE_HIT_R, tw, &t)) { tw = t; te = tp = ci = -1; mi = i; }
        if (mi >= 0) { mineList[nMineHit++] = mi; onWall[k] = 0; }   /* --- fin Ventouse --- */
        end[k] = vadd(eye, vmul(dir[k], tw));
        float dmg = SG_PELLET_DMG * sg_falloff(tw) * mult;
        if (te >= 0) {
            A.e[te] += dmg; A.ke[te] = vadd(A.ke[te], vmul(dir[k], SG_PELLET_KNOCK)); A.ne[te]++; A.he[te] = end[k];
            pellets++; onWall[k] = 0;
        } else if (tp >= 0) {
            A.p[tp] += dmg; A.kp[tp] = vadd(A.kp[tp], vmul(dir[k], SG_PELLET_KNOCK)); A.np[tp]++; A.hp[tp] = end[k];
            pellets++; onWall[k] = 0;
        } else if (ci >= 0) { burstList[nBurst++] = ci; onWall[k] = 0; }   /* eclate apres la boucle (une fois par piece) */
    }
    sgBursts = 0;
    for (int b = 0; b < nBurst; b++) sg_coin_burst(burstList[b], 0, &A);
    int juice = 0; V3 juiceAt = eye;
    for (int i = 0; i < MAXE; i++) {                      /* un seul chiffre de degats par cible */
        if (A.e[i] <= 0) continue;
        V3 at = A.ne[i] ? A.he[i] : E[i].pos;
        if (A.ne[i] >= 8 && vlen(vsub(E[i].pos, eye)) < 6) { juice = 1; juiceAt = at; }
        burst(at, v3(1, 0.45f, 0.2f), 12, 7, 0.3f, 0.1f);
        damage_enemy(i, A.e[i], A.ke[i]);
    }
    for (int i = 0; i < MAXP; i++) {                      /* AGREGATION : un seul net_hit par joueur et par tir */
        if (A.p[i] <= 0) continue;
        V3 at = A.np[i] ? A.hp[i] : np_center(i);
        if (A.np[i] >= 8 && vlen(vsub(np_center(i), eye)) < 6) { juice = 1; juiceAt = at; }
        burst(at, v3(1, 0.45f, 0.2f), 12, 7, 0.3f, 0.1f);
        if (!hit_player_ex(i, A.p[i] * PVP_SCALE, vadd(A.kp[i], v3(0, 2, 0)), W_SHOTGUN, A.np[i] ? d : vnorm(vsub(np_center(i), eye)),
                           A.np[i] ? SHB_DIRECT : SHB_BLAST, 0, 0, 0, 0) && A.np[i])
            pellets -= A.np[i];                           /* Egide : plombs bloques = rates (eclat seul : souffle, 50 %) */
    }
    for (int b = 0; b < nMineHit; b++)                    /* --- Ventouse : chaque mine touchee explose une fois --- */
        if (PR[mineList[b]].alive && PR[mineList[b]].kind == PR_MINE) {
            int foreign = PR[mineList[b]].owner != myId;
            if (mine_explode(&PR[mineList[b]], 0, 1)) A.burstHit = 1;
            snprintf(popText, sizeof popText, foreign ? "MINE VOLEE !" : "MINE !"); popT = 1.0f;
        }                                                 /* --- fin Ventouse --- */
    int hit = pellets >= SG_STREAK_MIN || A.burstHit;
    shot_result(hit);
    sgShots++; sgPelletSum += pellets; sgStreakShots += hit; sgBurstSum += sgBursts;
    if (botMode) netlog("tir pompe : %d plomb(s), %d eclat(s), %s", pellets, sgBursts, hit ? "touche" : "rate");
    if (!P.grounded) sg_recoil(d);

    V3 m = muzzle(), col = sg_tint(shotStreak / (float)STREAK_MAX);   /* effets locaux : traceurs, etincelles, son */
    for (int k = 0; k < SG_PELLETS; k++) {
        add_tracer(m, end[k], col);
        if (onWall[k])
            for (int j = 0; j < 3; j++)
                add_part(end[k], vadd(vmul(nrm[k], 3), vmul(rand_dir(), 2)), v3(1, 0.75f, 0.4f), 0.35f, 0.07f, 12);
    }
    if (dmMode) net_fx_shot(eye, d, half, seed);
    P.fireCd = P.sgPumpMax = P.sgPumpT = SG_CD / streak_rate();
    P.recoil = 1.6f; shake += 0.1f + 0.012f * pellets; muzzleT = 0.09f;
    P.sgLastPellets = pellets; P.sgLastT = pellets ? 0.5f : 0;
    play(S_SG_FIRE, 0.85f, frange(0.96f, 1.04f));
    if (pellets || A.burstHit) { hitMarker = 0.18f; play(S_HIT, 0.6f, 0.9f + 0.03f * pellets); }
    if (juice) { hitstop = fmaxf(hitstop, 0.05f); impact_frame(juiceAt, 0.05f); }
    if (sgBursts) {
        if (sgBursts > 1) snprintf(popText, sizeof popText, "ECLAT x%d", sgBursts);
        else snprintf(popText, sizeof popText, "ECLAT !");
        popT = 1.0f;
    }
}
/* --- fin Eventail ------------------------------------------------------- */

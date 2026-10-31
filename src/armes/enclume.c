/* VELOCITY - Enclume : coup au sol, pique, onde de choc
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Enclume (marteau-pilon) : coup au sol, pique, onde de choc          */
/* Au sol : coup lourd frontal. En l'air (assez haut) : pique a 30 m/s */
/* minimum, puis onde de choc a l'atterrissage dont rayon et degats    */
/* montent avec la vitesse de chute. Touche : rebond + dash ; rate :   */
/* immobilise 0,6 s. L'onde rampe au sol : sauter l'esquive.           */
/* ------------------------------------------------------------------ */
static int slamDives, slamWaves, slamWaveHits, slamStuns;   /* journal du bot */
static float slamVySum, slamDmgSum;

static float anvil_range(float rel) { return ANVIL_RANGE + rel * ANVIL_RANGE_PER_MS; }
static float anvil_dmg(float rel) { return ANVIL_DMG + rel * ANVIL_DMG_PER_MS; }
static float slam_radius(float vy) { return SLAM_RADIUS + SLAM_RADIUS_PER_MS * vy; }
static float slam_kh(float fallH) { return SLAM_FULL_H > 0 ? clampf(fallH / SLAM_FULL_H, 0.5f, 1) : 1; }
static float slam_dmg(float vy, float fallH) { return (SLAM_DMG + SLAM_DMG_PER_MS * vy) * slam_kh(fallH); }

/* distance au sol sous un point (les pieds), 50 si rien */
static float anvil_ground_dist(V3 feet) {
    return ray_world(vadd(feet, v3(0, 0.05f, 0)), v3(0, -1, 0), 50, 0) - 0.05f;
}

static void anvil_reset(void) { P.anvilState = ANVIL_IDLE; P.anvilStunT = 0; P.anvilLandT = 10; }

/* Prevision du pique (sans controle aerien, la trajectoire est deterministe) : point d'impact,
 * |vy| a l'impact, rayon de l'onde et temps de chute. Sert au repere d'impact, au HUD et au bot. */
static int slam_predict(V3 *at, float *vy, float *R, float *tFall) {
    float h = anvil_ground_dist(P.pos);
    if (h >= 50) return 0;
    float v0 = P.anvilState == ANVIL_DIVE ? fmaxf(-P.vel.y, 0) : fmaxf(-P.vel.y, SLAM_VY);
    float t = (-v0 + sqrtf(v0 * v0 + 2 * GRAVITY * h)) / GRAVITY;
    V3 p = vadd(P.pos, v3(P.vel.x * t, 0, P.vel.z * t));
    float drop = anvil_ground_dist(v3(p.x, P.pos.y, p.z));
    if (drop >= 50) drop = h;
    *at = v3(p.x, P.pos.y - drop, p.z);
    *vy = fminf(sqrtf(v0 * v0 + 2 * GRAVITY * drop), SLAM_VY_MAX);
    *R = slam_radius(*vy);
    if (tFall) *tFall = t;
    return 1;
}

/* coup au sol : copie de l'epee avec les reglages de l'Enclume */
static void anvil_strike(void) {
    float sp = vlen(P.vel);
    V3 eye = eye_pos(), f = view_dir();
    V3 r = v3(cosf(P.yaw), 0, sinf(P.yaw)), u = vcross(r, f);
    int hits = 0, parried = 0;

    melee_start_swing();
    parried = try_parry(MELEE_RANGE + 1.5f, 1);
    for (int i = 0; i < MAXE; i++) {
        Enemy *e = &E[i];
        if (!e->alive) continue;
        float rel = rel_speed(e->vel);
        V3 d = vsub(e->pos, eye);
        float dist = vlen(d), reach = dist - enemy_radius(e->type);
        if (reach > anvil_range(rel)) continue;
        if (reach > 0.8f && vdot(vmul(d, 1.0f / dist), f) < ANVIL_CONE) continue;
        if (e->type == EN_RUSHER) {                       /* kamikaze pare : explose dans sa face */
            V3 at = e->pos;
            damage_enemy(i, 9999, vmul(f, 10));
            parry_feedback(at);
            melee_parry_points(PTS_PARRY);
            parried++;
        } else {
            damage_enemy(i, anvil_dmg(rel), vadd(vmul(f, ANVIL_KNOCK), v3(0, ANVIL_KNOCK_UP, 0)));
            if (!e->alive) { P.hp = fminf(100, P.hp + 15); ammo_points(PTS_MELEE_KILL, AP_KILL); }
        }
        hits++;
    }
    for (int i = 0; i < MAXP; i++) {                     /* LAN : les autres joueurs */
        if (!np_target(i)) continue;
        float rel = rel_speed(np_vel(i));
        V3 c = np_center(i), d = vsub(c, eye);
        float dist = vlen(d), reach = dist - 0.5f;
        if (reach > anvil_range(rel)) continue;
        if (reach > 0.8f && vdot(vmul(d, 1.0f / dist), f) < ANVIL_CONE) continue;
        if (!hit_player(i, anvil_dmg(rel) * PVP_SCALE, vadd(vmul(f, ANVIL_KNOCK), v3(0, ANVIL_KNOCK_UP, 0)), W_SLAM, vnorm(d))) {
            parried++;                                    /* Egide : coup au sol bloque de face */
            continue;
        }
        burst(c, v3(1, 0.7f, 0.35f), 24, 9, 0.35f, 0.14f);
        hits++;
    }
    float range = anvil_range(sp);
    for (float a = -0.6f; a <= 0.6f; a += 0.1f) {        /* arc vertical : la tete tombe de haut en bas */
        V3 p = vadd(eye, vadd(vmul(f, range * 0.6f + 0.5f), vadd(vmul(u, -a * range * 0.5f), vmul(r, a * 0.15f))));
        add_part(p, vadd(vmul(f, 2), v3(0, -4, 0)), v3(1, 0.75f, 0.45f), 0.18f, 0.09f + sp * 0.002f, 0);
    }
    if (dmMode) net_fx_melee(eye, f, sp);                 /* EV_MELEE : le recepteur lit meleeKind dans pad */
    P.meleeCd = ANVIL_CD;
    P.meleeT = ANVIL_SWING_T;
    P.parryWin = ANVIL_PARRY_WINDOW;
    P.swingLanded = hits || parried;
    play(S_DASH, 0.45f, 0.8f);                            /* souffle plus grave que l'epee */
    if (hits) {
        melee_hit_points(meleePts[P.meleeKind]);
        P.dashCharge = fminf(2, P.dashCharge + 1);
        hitMarker = 0.18f;
        shake += 0.25f;
        if (!parried) hitstop = fmaxf(hitstop, 0.06f + sp * 0.002f);
        play(S_ANVIL_CLANG, 0.9f, frange(0.9f, 1.05f));
    }
}

static void slam_start(void) {
    if (P.hook != HOOK_NONE) hook_release(0);
    P.wallrun = P.sliding = 0;
    P.anvilState = ANVIL_DIVE; P.anvilStartY = P.pos.y;
    P.vel.y = fminf(P.vel.y, -SLAM_VY);
    P.fovKick += 6; shake += 0.1f;
    slamDives++;
    play(S_ANVIL_DIVE, 0.8f, 1.0f);
    if (botMode) netlog("enclume : pique depuis %.1f m", anvil_ground_dist(P.pos));
}

/* appui sur la melee avec l'Enclume : au sol (ou trop pres du sol) coup lourd, sinon pique */
static void anvil_press(void) {
    if (P.anvilState != ANVIL_IDLE) return;
    if (P.grounded || anvil_ground_dist(P.pos) < SLAM_MIN_H) anvil_strike();
    else slam_start();
}

/* onde de choc (calquee sur explode, mais au sol et sans traverser les murs).
 * directP / directE : cible d'un contact direct en l'air (-1 = onde au sol) : elle seule prend tout. */
static void slam_resolve(float vyImpact, int directP, int directE) {
    float vy = fminf(fabsf(vyImpact), SLAM_VY_MAX), H = P.anvilStartY - P.pos.y;
    float R = slam_radius(vy), D = slam_dmg(vy, H);
    V3 c = P.pos, eyeC = vadd(c, v3(0, 0.5f, 0));
    int hits = 0, nt = 0, tgt[MAXP];
    float tdmg[MAXP];
    V3 tkn[MAXP];

    melee_start_swing();                                  /* une onde = un coup (points de jauge) */
    for (int i = 0; i < MAXE; i++) {
        Enemy *e = &E[i];
        float er = enemy_radius(e->type), f = 1;
        if (!e->alive || (directE >= 0 && i != directE) || directP >= 0) continue;
        if (directE < 0) {
            if (e->pos.y - er - c.y > SLAM_REACH_UP || e->pos.y + er < c.y - SLAM_REACH_DOWN) continue;
            float d = hlen(v3(e->pos.x - c.x, 0, e->pos.z - c.z)), Rc = R + er;
            if (d >= Rc || !los(eyeC, e->pos)) continue;
            f = 1 - d / Rc;
        }
        V3 out = vnorm(v3(e->pos.x - c.x, 0.001f, e->pos.z - c.z));
        float dmg = D * (0.3f + 0.7f * f);
        damage_enemy(i, dmg, v3(out.x * SLAM_KNOCK_OUT * f, SLAM_KNOCK_UP, out.z * SLAM_KNOCK_OUT * f));
        if (!e->alive && e->type == EN_DRONE) ammo_points(PTS_MELEE_KILL, AP_KILL);
        slamDmgSum += dmg;
        hits++;
    }
    for (int i = 0; i < MAXP; i++) {                     /* LAN : les touches partent apres le rebond (voir plus bas) */
        V3 d = NP[i].disp;
        float f = 1;
        if (!np_target(i) || (directP >= 0 && i != directP) || directE >= 0) continue;
        if (directP < 0) {
            if (d.y - c.y > SLAM_REACH_UP || d.y < c.y - SLAM_REACH_DOWN) continue;
            float dh = hlen(v3(d.x - c.x, 0, d.z - c.z)), Rc = R + 0.5f;
            if (dh >= Rc || !los(eyeC, np_center(i))) continue;
            f = 1 - dh / Rc;
        }
        V3 out = vnorm(v3(d.x - c.x, 0.001f, d.z - c.z));
        tgt[nt] = i; tdmg[nt] = D * (0.3f + 0.7f * f) * PVP_SCALE;
        tkn[nt] = v3(out.x * SLAM_KNOCK_OUT * f, SLAM_KNOCK_UP, out.z * SLAM_KNOCK_OUT * f);
        slamDmgSum += tdmg[nt];
        nt++; hits++;
    }

    /* effets locaux */
    for (int k = 0; k < 36; k++) {
        float a = k * (2 * PI / 36);
        add_part(vadd(c, v3(0, 0.15f, 0)), v3(cosf(a) * R * 2.2f, 0.6f, sinf(a) * R * 2.2f), v3(1, 0.7f, 0.35f), 0.4f, 0.22f, 0);
    }
    burst(c, v3(0.75f, 0.72f, 0.7f), 40, 6, 0.6f, 0.2f);
    shards(c, v3(0.55f, 0.52f, 0.5f), 8 + (int)(vy / 4), 9);
    shake += 0.3f + vy * 0.01f; P.landDip = 0.3f;
    play(S_ANVIL_SLAM, clampf(vy / 40, 0.5f, 1), 1.1f - vy * 0.005f);
    if (dmMode) net_fx_slam(c, R, vy, hits);
    P.anvilLastVy = vy; P.anvilLandT = 0; P.meleeCd = fmaxf(P.meleeCd, SLAM_CD);
    slamWaves++; slamVySum += vy;
    if (hits) {
        slamWaveHits++;
        P.vel.y = SLAM_BOUNCE; P.grounded = 0; P.airJumps = 1;   /* vitesse horizontale conservee */
        P.dashCharge = fminf(2, P.dashCharge + 1);
        melee_hit_points(meleePts[P.meleeKind]);         /* une fois par onde */
        hitMarker = 0.18f; play(S_HIT, 0.7f, 0.8f);
        hitstop = fmaxf(hitstop, fminf(0.12f, 0.05f + vy * 0.0015f));
        if (vy > 35) impact_frame(c, 0.07f);
        P.anvilState = ANVIL_IDLE;
    } else {
        P.anvilState = ANVIL_STUN; P.anvilStunT = SLAM_STUN; P.vel.x = P.vel.z = 0;
        slamStuns++;
    }
    if (botMode) netlog("enclume : onde |vy| %.1f, rayon %.1f, degats %.0f, %d touche(s)", vy, R, D, hits);
    /* le rebond est decide chez le frappeur avant l'arbitrage : chez l'hote, une onde paree
     * (EV_MELEE_PARRY, arbitre tout de suite) ecrase donc bien le rebond et rend les points */
    for (int k = 0; k < nt; k++)                          /* Egide : l'onde passe sous le bouclier ; le contact direct est bloque */
        hit_player_ex(tgt[k], tdmg[k], tkn[k], W_SLAM, vnorm(vsub(np_center(tgt[k]), pcenter())), directP >= 0 ? SHB_DIRECT : SHB_NEVER,
                      0, 0, 0, 0);
}
static void slam_land(float vyImpact) { slam_resolve(vyImpact, -1, -1); }

/* contact direct pendant le pique avec une cible hors d'atteinte de l'onde (en l'air) : elle prend tout */
static void slam_contact(void) {
    V3 mn, mx, omn, omx;
    paabb(P.pos, pheight(), &mn, &mx);
    for (int i = 0; i < MAXP; i++) {
        if (!np_target(i)) continue;
        np_aabb(i, 0, &omn, &omx);
        if (mn.x < omx.x && mx.x > omn.x && mn.y < omx.y && mx.y > omn.y && mn.z < omx.z && mx.z > omn.z &&
            anvil_ground_dist(NP[i].disp) > SLAM_REACH_UP) { slam_resolve(-P.vel.y, i, -1); return; }
    }
    for (int j = 0; j < MAXE; j++) {
        if (!E[j].alive) continue;
        float er = enemy_radius(E[j].type);
        V3 q = v3(clampf(E[j].pos.x, mn.x, mx.x), clampf(E[j].pos.y, mn.y, mx.y), clampf(E[j].pos.z, mn.z, mx.z));
        if (vlen(vsub(q, E[j].pos)) < er && anvil_ground_dist(v3(E[j].pos.x, E[j].pos.y - er, E[j].pos.z)) > SLAM_REACH_UP) {
            slam_resolve(-P.vel.y, -1, j);
            return;
        }
    }
}

/* piques des autres joueurs : trainee, et sifflement au debut */
static void anvil_remote_tick(void) {
    for (int i = 0; i < MAXP; i++) {
        unsigned char pad = NP[i].st.pad;
        if (!np_present(i)) { NP[i].anvilPrevPad = 0; continue; }
        if (pad & ST_SLAM) {
            V3 p = NP[i].disp;
            for (int k = 0; k < 2; k++)
                add_part(vadd(p, v3(frange(-.3f, .3f), frange(0.2f, 1.6f), frange(-.3f, .3f))), v3(frange(-1, 1), 6, frange(-1, 1)),
                         v3(1, 0.6f, 0.25f), 0.35f, 0.14f, 0);
            if (!(NP[i].anvilPrevPad & ST_SLAM)) play(S_ANVIL_DIVE, 0.8f * atten(p), 1.0f);
        }
        NP[i].anvilPrevPad = pad;
    }
}

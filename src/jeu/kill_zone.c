/* VELOCITY - kill zone : chute sous le plan de mort de la carte, et prudence des bots au bord du vide
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

static void cfg_save(void);
/* kill zone : sous le plan de mort de la carte, on meurt (solo : tout de suite ; LAN : l'hote arbitre et credite le kill) */
static double voidSentT;
static int voidPending, voidFalls;                        /* demande en cours ; mes chutes (journal du bot) */
static void void_check(void) {
    float ky = maps[curMap].killY;
    if (ky < -999 || spectating) return;
    if (P.dead) {                                         /* le corps s'arrete sous le plan : la camera ne tombe pas sans fin */
        if (P.pos.y < ky - 6) { P.pos.y = ky - 6; P.prevPos = P.pos; P.vel = v3(0, 0, 0); }
        return;
    }
    if (P.pos.y >= ky) { voidPending = 0; fellOut = 0; return; }
    if (!voidPending) { voidPending = 1; voidFalls++; fellOut = 1; voidSentT = -1; if (botMode) netlog("chute dans le vide"); }
    if (!dmMode) { damage_player(999); return; }
    if (nowT - voidSentT > 0.5) {                         /* renvoyee tant que l'hote n'a pas tranche */
        voidSentT = nowT;
        net_hit(myId, 999, v3(0, 0, 0), W_VOID);
    }
}

/* --- bots et kill zone : trajectoire balistique (sans air-strafe) ; retombe-t-elle sur un sol avant killY ? --- */
static int bot_lands(V3 p, V3 v, float ky) {
    const float st = 1.0f / 30;
    int air = 0;                                          /* a-t-on quitte le sol ? */
    for (int i = 0; i < 90; i++) {                        /* 3 s */
        v.y -= GRAVITY * st;
        V3 q = vadd(p, vmul(v, st));
        if (!pos_free(v3(q.x, p.y + 0.3f, q.z), 1.2f)) { v.x = v.z = 0; q.x = p.x; q.z = p.z; }   /* mur : on glisse le long */
        float drop = p.y - q.y + 0.4f;
        int ground = v.y <= 0 && ray_world(v3(q.x, p.y + 0.2f, q.z), v3(0, -1, 0), drop, 0) < drop;
        if (ground) {
            if (air || i * st > 0.35f) return 1;          /* retombe sur un sol, ou reste au sol assez longtemps */
            q.y = p.y; v.y = 0;                           /* encore au sol : on continue de glisser (sans friction : prudent) */
        } else air = 1;
        if (q.y < ky + 1) return 0;
        p = q;
    }
    return 1;
}
/* apres bot_think : annule un saut ou un dash qui part dans le vide, freine au bord, et grappin d'urgence vers le decor
 * le plus proche (au-dessus de nous) quand la chute ne retombe sur rien */
static void bot_void_guard(float dt) {
    static float rescueT;
    float ky = maps[curMap].killY;
    if (ky < -999 || appState != APP_GAME || P.dead || spectating) { rescueT = 0; return; }
    V3 v = P.vel, f = v3(sinf(P.yaw), 0, -cosf(P.yaw)), r = v3(cosf(P.yaw), 0, sinf(P.yaw));
    V3 wish = vnorm(vadd(vmul(f, (float)(botHeld[A_FWD] - botHeld[A_BACK])), vmul(r, (float)(botHeld[A_RIGHT] - botHeld[A_LEFT]))));
    if (vlen(wish) < 0.5f) wish = f;
    if (botPress[A_DASH] && P.dashCharge >= 1) {          /* dash : meme vitesse que player_tick */
        float sp = fmaxf(hlen(v) + 3, DASH_SPEED), vy = sinf(P.pitch) * sp;
        V3 dv = v3(wish.x * sp, P.grounded ? fmaxf(v.y, vy) : (vy > 2 ? vy : vy < 0 ? vy : 2), wish.z * sp);
        if (!bot_lands(P.pos, dv, ky) && bot_lands(P.pos, v, ky)) botPress[A_DASH] = 0;
    }
    if (P.grounded) {
        rescueT = 0;
        int onPad = boxes[P.groundBox].kind == B_PAD;
        if ((botPress[A_JUMP] || botHeld[A_JUMP]) && !onPad && !bot_lands(P.pos, v3(v.x, JUMP_V, v.z), ky))
            botPress[A_JUMP] = botHeld[A_JUMP] = 0;       /* saut vers le vide : non */
        V3 ah = vadd(P.pos, v3(v.x * 0.25f + wish.x * 0.6f, 0.3f, v.z * 0.25f + wish.z * 0.6f));
        if (!onPad && ray_world(ah, v3(0, -1, 0), 2.5f, 0) >= 2.5f && !bot_lands(P.pos, v3(v.x, JUMP_V, v.z), ky)) {
            memset(botHeld, 0, sizeof botHeld);           /* bord : on freine, dos au vide */
            botPress[A_JUMP] = botPress[A_DASH] = 0;
            if (hlen(v) > 0.5f) { P.yaw = atan2f(v.x, -v.z); botHeld[A_BACK] = 1; }
        }
        return;
    }
    if (P.hook == HOOK_ATTACHED && rescueT > 0) {         /* tire par le grappin de secours : on tient tant que ca ne retombe sur rien */
        rescueT -= dt;
        botHeld[A_HOOK] = !bot_lands(P.pos, v, ky) || v.y < 0;
        return;
    }
    if (P.hook != HOOK_NONE || bot_lands(P.pos, v, ky)) { if (P.hook == HOOK_FLYING && rescueT > 0) botHeld[A_HOOK] = 1; return; }
    if (P.airJumps > 0 && bot_lands(P.pos, v3(v.x, JUMP_V, v.z), ky)) { botPress[A_JUMP] = 1; return; }   /* le double saut suffit */
    if (P.hookCd > 0) return;
    V3 eye = eye_pos(), best = v3(0, 0, 0);
    float bs = 1e9f, bd = 0;
    for (int i = 0; i < nb; i++) {                        /* point du decor le plus proche, de preference le bord d'un dessus */
        Box *b = &boxes[i];
        if (b->kind != B_SOLID || b->mx.y < P.pos.y - 2) continue;
        for (int top = 0; top < 2; top++) {
            V3 q = v3(clampf(eye.x, b->mn.x + 0.3f, b->mx.x - 0.3f),
                      top ? b->mx.y - 0.3f : fminf(b->mx.y - 0.3f, fmaxf(eye.y + 6, b->mn.y + 0.3f)),
                      clampf(eye.z, b->mn.z + 0.3f, b->mx.z - 0.3f));
            V3 d = vsub(q, eye);
            float l = vlen(d), score = l + (top ? 0 : 15);   /* une paroi retient, un dessus sauve */
            if (l < 2 || l > GRAPPLE_RANGE * 0.95f || score >= bs) continue;
            if (ray_world(eye, vmul(d, 1 / l), l + 0.5f, 0) < l - 0.6f) continue;   /* masque par autre chose */
            bs = score; bd = l; best = q;
        }
    }
    if (bs > 1e8f) return;
    V3 d = vsub(best, eye);
    P.yaw = atan2f(d.x, -d.z); P.pitch = asinf(clampf(d.y / bd, -1, 1));
    botPress[A_HOOK] = botHeld[A_HOOK] = 1;
    rescueT = 2.5f;
    netlog("grappin de secours (%.0f m, y %.1f)", bd, P.pos.y);
}

/* VELOCITY - captures --shot et tests automatiques --test
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

static void save_bmp(const char *path) {
    int rowSize = (winW * 3 + 3) & ~3;
    unsigned char *px = (unsigned char *)malloc(rowSize * winH);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, winW, winH, 0x80E0 /* GL_BGR_EXT */, GL_UNSIGNED_BYTE, px);
    FILE *f = fopen(path, "wb");
    if (f) {
        BITMAPFILEHEADER fh = { 0 }; BITMAPINFOHEADER ih = { 0 };
        fh.bfType = 0x4D42; fh.bfOffBits = sizeof fh + sizeof ih; fh.bfSize = fh.bfOffBits + rowSize * winH;
        ih.biSize = sizeof ih; ih.biWidth = winW; ih.biHeight = winH; ih.biPlanes = 1; ih.biBitCount = 24;
        fwrite(&fh, sizeof fh, 1, f); fwrite(&ih, sizeof ih, 1, f); fwrite(px, rowSize * winH, 1, f);
        fclose(f);
    }
    free(px);
}

/* tests de l'Enclume : pique depuis `from` (vitesse `vel`), ticks jusqu'a la fin du pique */
static void test_slam_drop(V3 from, V3 vel) {
    P.meleeKind = MK_ANVIL; P.anvilState = ANVIL_IDLE; P.meleeCd = 0;
    P.pos = P.prevPos = from; P.vel = vel; P.grounded = 0;
    anvil_press();
    for (int i = 0; i < 600 && P.anvilState == ANVIL_DIVE; i++) player_tick(TICK, 0, 0);
}

/* tests de la Lance : remise a zero, Lance en melee, immobile en pos, vue vers -z */
static void lance_test_reset(V3 pos) {
    reset_game();
    hitstop = 0;
    memset(act, 0, sizeof act); memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
    P.meleeKind = MK_LANCE; P.pos = P.prevPos = pos; P.yaw = 0; P.pitch = 0; P.vel = v3(0, 0, 0);
}
/* LAN simule : moi (hote, 0) et NP[1] a la position t, ni parry ni lance couchee recents */
static void lance_test_lan(V3 t) {
    dmMode = 1; matchOver = 0;
    memset(NP, 0, sizeof NP);
    memset(joustT, 0, sizeof joustT);
    NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Moi");
    NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Cible");
    NP[1].disp = t; vf(NP[1].st.pos, t);
    NP[0].disp = P.pos;
    nowT += 1;
}

/* tests de la Scie : remise a zero, scie dans le loadout, en main */
static void saw_test_reset(V3 pos, float yaw, float pitch) {
    reset_game();
    dmMode = 0; hitstop = 0;
    memset(act, 0, sizeof act); memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);   /* entrees des tests d'avant */
    P.pos = P.prevPos = pos; P.yaw = yaw; P.pitch = pitch; P.vel = v3(0, 0, 0);
    P.loadout[0] = WP_SAW; P.loadout[1] = WP_RAIL; P.weapon = WP_SAW; P.ammo[WP_SAW] = 1; P.switchT = 0;
}
/* place une scie (la mienne si owner == myId) dans l'etat st */
static Proj *saw_test_put(V3 pos, V3 vel, int st, int owner) {
    Proj *p = spawn_proj(PR_SAW, pos, vel);
    p->sawState = st; p->owner = owner; p->sawAxis = v3(0, 1, 0); p->sawN = v3(0, 1, 0);
    p->netId = ((unsigned int)(owner + 1) << 24) | ((unsigned int)PR_SAW << 20) | 77;
    if (owner == myId) { P.sawIdx = (int)(p - PR) + 1; P.ammo[WP_SAW] = 0; }
    return p;
}

/* --- Tesla : outils des tests --- */
/* remise a zero : solo, Tesla + rail en main, batterie 60, regard vers -z (ou yaw) depuis pos */
static void tesla_test_reset(V3 pos, float yaw) {
    reset_game();
    dmMode = 0; hitstop = 0;
    memset(act, 0, sizeof act); memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
    memset(DN, 0, sizeof DN);
    P.pos = P.prevPos = pos; P.yaw = yaw; P.pitch = 0; P.vel = v3(0, 0, 0);
    P.loadout[0] = WP_TESLA; P.loadout[1] = WP_RAIL; P.weapon = WP_TESLA; P.switchT = 0; P.fireCd = 0;
    P.ammo[WP_TESLA] = 60; P.ammo[WP_RAIL] = 2;
}
/* `ticks` ticks comme game_tick (pieces, recharge, Tesla), sans deplacer les drones ; held = tir tenu */
static void tesla_test_run(int ticks, int held) {
    for (int i = 0; i < ticks; i++) {
        nowT += TICK;
        coin_tick(TICK);
        P.fireCd -= TICK; P.switchT -= TICK;
        tesla_tick(TICK, held && !P.dead && P.weapon == WP_TESLA && P.switchT <= 0);
    }
}
static Enemy *tesla_test_drone(int i, V3 pos, float hp) {
    E[i].alive = 1; E[i].type = EN_DRONE; E[i].hp = hp; E[i].pos = pos; E[i].vel = v3(0, 0, 0);
    return &E[i];
}
/* --- fin Tesla : outils des tests --- */

/* --- Ventouse : outils des tests --- */
/* remise a zero : solo, Ventouse + rail, Ventouse en main avec 1 mine, sans drones */
static void mine_test_reset(V3 pos, float yaw, float pitch) {
    reset_game();
    dmMode = 0; hitstop = 0; waveDelay = 1000;
    memset(act, 0, sizeof act); memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
    memset(DN, 0, sizeof DN);
    P.pos = P.prevPos = pos; P.yaw = yaw; P.pitch = pitch; P.vel = v3(0, 0, 0);
    P.loadout[0] = WP_MINE; P.loadout[1] = WP_RAIL; P.weapon = WP_MINE; P.switchT = 0; P.fireCd = 0;
    P.ammo[WP_MINE] = 1; P.ammoGauge[WP_MINE] = 0;
}
static Proj *mine_test_last(void) {                       /* ma derniere mine lancee */
    Proj *r = NULL;
    for (int i = 0; i < MAXPR; i++) if (mine_own(&PR[i]) && (!r || PR[i].mineSeq > r->mineSeq)) r = &PR[i];
    return r;
}
static int mine_test_stick(Proj *p, int moveP) {          /* ticks jusqu'au collage (ou l'explosion) */
    int n = 0;
    while (n < 600 && p->alive && p->kind == PR_MINE && p->mineState == MS_FLY) {
        nowT += TICK; proj_tick(TICK);
        if (moveP) player_tick(TICK, 0, 0);
        n++;
    }
    return n;
}
static Proj *mine_test_put(V3 pos, int st, int owner) {   /* mine posee a la main (armee) */
    Proj *p = spawn_proj(PR_MINE, pos, v3(0, 0, 0));
    p->owner = owner; p->mineState = st; p->mineN = v3(0, 1, 0); p->mineSeq = ++mineCounter;
    p->netId = ((unsigned int)(owner + 1) << 24) | ((unsigned int)PR_MINE << 20) | (mineCounter & 0xFFFFF);
    return p;
}
/* LAN simule : moi (hote, 0) et NP[1] en t ; table d'arbitrage vide */
static void mine_test_lan(V3 t) {
    dmMode = 1; matchOver = 0;
    memset(NP, 0, sizeof NP);
    memset(hostMine, 0, sizeof hostMine);
    NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Moi");
    NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Cible");
    NP[1].disp = t; vf(NP[1].st.pos, t);
    NP[0].disp = P.pos;
    nowT += 1;
}
/* --- fin Ventouse : outils des tests --- */
/* --- Fleau : outils des tests --- */
/* remise a zero : solo, Fleau en melee, immobile en pos, vue vers -z */
static void flail_test_reset(V3 pos) {
    reset_game();
    dmMode = 0; hitstop = 0;
    memset(act, 0, sizeof act); memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
    P.meleeKind = MK_FLAIL; P.pos = P.prevPos = pos; P.yaw = 0; P.pitch = 0; P.vel = v3(0, 0, 0);
}
static Enemy *flail_test_enemy(int i, V3 pos, int type) {
    E[i].alive = 1; E[i].type = type; E[i].hp = type == EN_DRONE ? 100.0f : 50.0f; E[i].pos = pos; E[i].vel = v3(0, 0, 0);
    E[i].fireT = 10;                                      /* pas de tir d'orbe pendant le test */
    return &E[i];
}
/* dernier EV_HIT de la file d'envoi (NULL si aucun) */
static const NetEvent *flail_test_last_hit(void) {
    for (int k = 1; k <= OUTQ; k++) {
        const NetEvent *e = &outq[(outqHead - k + OUTQ) % OUTQ].ev;
        if (e->kind == EV_HIT) return e;
    }
    return NULL;
}
/* --- fin Fleau : outils des tests --- */
/* --- Poings : outils des tests --- */
/* remise a zero : solo, Poings en melee, immobile en pos, vue vers -z */
static void fist_test_reset(V3 pos) {
    reset_game();
    dmMode = 0; hitstop = 0;
    memset(act, 0, sizeof act); memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
    P.meleeKind = MK_FIST; P.pos = P.prevPos = pos; P.yaw = 0; P.pitch = 0; P.vel = v3(0, 0, 0);
    P.grounded = pos.y < 0.01f;                           /* pose au sol (sinon un coup sur un drone "suspend" en l'air) */
}
/* un tick de melee comme dans game_tick, sans le reste du monde */
static void fist_test_tick(int press) { P.meleeCd -= TICK; P.meleeT -= TICK; fist_tick(TICK, press); hitstop = 0; }
/* un coup tout de suite ; le drone e reste immobile (son recul ne compte pas dans v_rel) */
static void fist_test_punch(Enemy *e) { if (e) e->vel = v3(0, 0, 0); P.meleeCd = 0; fist_punch(); hitstop = 0; }
/* --- fin Poings : outils des tests --- */
/* --- Egide : outils des tests --- */
/* remise a zero : Egide en melee, rail en main (2 munitions), immobile en (-20, 0, 41), vue vers -z */
static void egide_test_reset(void) {
    reset_game();
    hitstop = 0;
    memset(act, 0, sizeof act); memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
    memset(DN, 0, sizeof DN);
    P.meleeKind = MK_SHIELD; P.pos = P.prevPos = v3(-20, 0, 41); P.yaw = 0; P.pitch = 0; P.vel = v3(0, 0, 0); P.grounded = 1;
    P.weapon = WP_RAIL; P.ammo[WP_RAIL] = 2; P.fireCd = 0; P.switchT = 0; shotStreak = 0;
}
/* LAN simule (je suis l'hote, 0) : NP[1] en t ; shield = son bouclier leve vers yaw (jauge pleine), sinon Egide baissee */
static void egide_test_lan(V3 t, int shield, float yaw) {
    lance_test_lan(t);
    NP[0].shieldGauge = NP[1].shieldGauge = SHIELD_GAUGE_MAX;
    NP[1].st.pad = (unsigned char)(MK_SHIELD | (shield ? ST_SHIELD : 0)); NP[1].st.yaw = yaw;
    NP[1].lastShieldT = shield ? nowT : 0;
}
/* --- fin Egide : outils des tests --- */

/* Tests du mouvement sans fenetre : velocity.exe --test resultat.txt */
static void run_tests(const char *path) {
    botAimTest = 1;
    FILE *f = fopen(path, "w");
    if (!f) return;
    build_level();

    {                                                     /* molette : une rafale (inertie) = un seul changement */
        int burst = 0, two = 0;
        for (int i = 0; i < 120; i++) burst += wheel_accept(100.0 + i * 0.016);   /* 2 s d'evenements toutes les 16 ms */
        two += wheel_accept(200.0); two += wheel_accept(200.6);                      /* deux crans espaces */
        int quick = wheel_accept(300.0) + wheel_accept(300.2 + WHEEL_QUIET);        /* second cran trop tot */
        fprintf(f, "molette : rafale de 2 s=%d, deux crans a 0,6 s=%d, deux crans a 0,35 s=%d (attendu 1, 2, 2)\n",
                burst, two, quick);
        wheelLastEv = wheelLastOk = -1e9;
    }

    reset_game();                                         /* escalier -> plateforme centrale */
    P.pos = P.prevPos = v3(0, 0, ms(14) + 0.6f); P.yaw = 0;
    act[A_FWD] = 1;
    for (int i = 0; i < 240; i++) player_tick(TICK, 0, 0);
    act[A_FWD] = 0;
    fprintf(f, "escalier : y=%.2f z=%.2f (attendu y=4)\n", P.pos.y, P.pos.z);

    reset_game();                                         /* escalier a 14 m/s : la vitesse se conserve comme sur une rampe */
    P.pos = P.prevPos = v3(0, 0, ms(14) + 0.6f); P.yaw = 0; P.vel = v3(0, 0, -14);
    P.grounded = 1;
    act[A_FWD] = 1;
    for (int i = 0; i < 30; i++) player_tick(TICK, 0, 0);
    act[A_FWD] = 0;
    fprintf(f, "escalier a 14 m/s : y=%.2f vitesse apres 0,25 s=%.1f (attendu y > 1, vitesse > 11)\n", P.pos.y, hlen(P.vel));

    reset_game();                                         /* au sol, direction tenue : friction reduite */
    P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = PI * 0.5f; P.vel = v3(15, 0, 0);
    P.grounded = 1;
    act[A_FWD] = 1;
    for (int i = 0; i < 30; i++) player_tick(TICK, 0, 0);
    act[A_FWD] = 0;
    fprintf(f, "course a 15 m/s sans sauter : vitesse apres 0,25 s=%.1f (attendu > 12)\n", hlen(P.vel));

    reset_game();                                         /* jump pad -> plateforme haute */
    P.pos = P.prevPos = v3(ms(16), 0.31f, 0);
    float maxY = 0;
    for (int i = 0; i < 360; i++) { player_tick(TICK, 0, 0); maxY = fmaxf(maxY, P.pos.y); }
    fprintf(f, "jump pad est : apogee=%.2f final x=%.2f y=%.2f grounded=%d (attendu y=7, x entre 39 et 62)\n",
            maxY, P.pos.x, P.pos.y, P.grounded);
    reset_game();
    P.pos = P.prevPos = v3(ms(9), 0.31f, ms(16));
    for (int i = 0; i < 360; i++) player_tick(TICK, 0, 0);
    fprintf(f, "jump pad nord : final x=%.2f y=%.2f z=%.2f (attendu y=7, z entre 39 et 62)\n", P.pos.x, P.pos.y, P.pos.z);

    reset_game();                                         /* jump pad aborde au sol a grande vitesse (bhop) : meme vecteur attendu */
    P.pos = P.prevPos = v3(ms(16) - 6, 0, 0); P.yaw = PI * 0.5f; P.vel = v3(45, 0, 0); P.grounded = 1;
    act[A_FWD] = 1;                                       /* tenu seulement le temps de l'approche (20 ticks ~0,17 s @ 45 m/s) */
    for (int i = 0; i < 20; i++) player_tick(TICK, 0, 0);
    act[A_FWD] = 0;
    maxY = 0;
    for (int i = 0; i < 360; i++) { player_tick(TICK, 0, 0); maxY = fmaxf(maxY, P.pos.y); }
    fprintf(f, "jump pad a grande vitesse au sol : apogee=%.2f final x=%.2f y=%.2f (attendu proches de la vitesse normale : apogee=8.55 x=46.73)\n",
            maxY, P.pos.x, P.pos.y);

    reset_game();                                         /* jump pad aborde EN L'AIR (bhop qui retombe dessus) : meme chose doit marcher */
    P.pos = P.prevPos = v3(ms(16) - 1.2f, 0.15f, 0); P.yaw = PI * 0.5f; P.vel = v3(45, -3, 0); P.grounded = 0;
    maxY = 0;
    for (int i = 0; i < 360; i++) { player_tick(TICK, 0, 0); maxY = fmaxf(maxY, P.pos.y); }
    fprintf(f, "jump pad a grande vitesse en l'air : apogee=%.2f final x=%.2f y=%.2f (attendu proches de la vitesse normale : apogee=8.55 x=46.73)\n",
            maxY, P.pos.x, P.pos.y);

    reset_game();                                         /* bhop + air-strafe */
    P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = PI * 0.5f;
    act[A_FWD] = 1;
    for (int i = 0; i < 60; i++) player_tick(TICK, 0, 0);
    act[A_FWD] = 0;
    float startSpeed = hlen(P.vel), maxSpeed = 0;
    act[A_JUMP] = 1;
    for (int i = 0; i < 300; i++) {
        int side = (i / 50) % 2 ? 1 : -1;                 /* alterne gauche/droite */
        act[A_RIGHT] = side > 0; act[A_LEFT] = side < 0;
        if (!P.grounded) P.yaw += side * 2.2f * TICK;
        player_tick(TICK, 0, 0);
        maxSpeed = fmaxf(maxSpeed, hlen(P.vel));
    }
    memset(act, 0, sizeof act);
    fprintf(f, "bhop : vitesse depart=%.1f max=%.1f (attendu max > %.1f)\n", startSpeed, maxSpeed, RUN_SPEED + 3);

    reset_game();                                         /* aimant de visee : doit corriger un petit ecart, doucement */
    P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0; P.grounded = 1;   /* couloir degage des tests */
    for (int i = 0; i < MAXE; i++) E[i].alive = 0;
    V3 eyeNow = eye_pos();
    E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(eyeNow.x + 0.7f, eyeNow.y, eyeNow.z - 20);
    aim_assist(TICK);
    float yawOneTick = P.yaw;
    for (int i = 0; i < 60; i++) aim_assist(TICK);
    float targetYaw = atan2f(0.7f, 20.0f);
    fprintf(f, "aimant de visee : apres 1 tick=%.5f (attendu petit, < 0.01), apres 0.5 s=%.4f sur %.4f vise (attendu entre 0 et %.4f, sans depasser)\n",
            yawOneTick, P.yaw, targetYaw, targetYaw);
    E[0].alive = 0;

    reset_game();                                         /* rocket jump */
    P.pos = P.prevPos = v3(-20, 0, 40); P.pitch = -1.5f;
    fire_rocket();
    player_tick(TICK, 1, 0);
    maxY = 0;
    for (int i = 0; i < 200; i++) { proj_tick(TICK); player_tick(TICK, 0, 0); maxY = fmaxf(maxY, P.pos.y); }
    fprintf(f, "rocket jump : apogee=%.2f hp=%.0f (attendu apogee > 3)\n", maxY, P.hp);

    reset_game();                                         /* grappin vers le mur nord */
    P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = PI; P.pitch = 0.35f;
    hookPressIn = 1; hookHeldIn = 1;
    int attachedTicks = 0;
    float maxSp = 0;
    V3 anchor = v3(0, 0, 0);
    for (int i = 0; i < 150; i++) {
        player_tick(TICK, 0, 0);
        hookPressIn = 0;
        if (P.hook == HOOK_ATTACHED) { attachedTicks++; anchor = P.anchor; }
        maxSp = fmaxf(maxSp, vlen(P.vel));
    }
    fprintf(f, "grappin : ancre=(%.1f, %.1f, %.1f) accroche %d ticks, vitesse max=%.1f, pos=(%.1f, %.1f, %.1f) (attendu ancre z=78, pos proche du mur)\n",
            anchor.x, anchor.y, anchor.z, attachedTicks, maxSp, P.pos.x, P.pos.y, P.pos.z);

    reset_game();                                         /* lacher en plein vol : l'elan est conserve */
    P.pos = P.prevPos = v3(-20, 0, 30); P.yaw = PI; P.pitch = 0.35f;
    hookPressIn = 1; hookHeldIn = 1;
    for (int i = 0; i < 60; i++) { player_tick(TICK, 0, 0); hookPressIn = 0; }
    float spBefore = vlen(P.vel);
    int stillAttached = P.hook == HOOK_ATTACHED;
    player_tick(TICK, 1, 0);                              /* saut = lacher + boost */
    float spAfter = vlen(P.vel), vyAfter = P.vel.y;
    hookHeldIn = 0;
    fprintf(f, "lacher : accroche=%d vitesse avant=%.1f apres=%.1f vy=%.1f hook=%d (attendu vitesse conservee, hook=0)\n",
            stillAttached, spBefore, spAfter, vyAfter, P.hook);

    /* grappin sur un drone : traction, impact, rebond */
    reset_game();
    P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0.16f;
    E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 4, 25);
    hookPressIn = 1; hookHeldIn = 1;
    int wasHooked = 0;
    float impactSp = 0;
    for (int i = 0; i < 120 && E[0].alive && E[0].hp == 100; i++) {
        player_tick(TICK, 0, 0);
        hookPressIn = 0;
        if (P.hookEnemy == 1) { wasHooked = 1; impactSp = vlen(P.vel); }
    }
    hookHeldIn = 0;
    fprintf(f, "grappin ennemi : accroche=%d vitesse a l'impact=%.1f drone vivant=%d hp=%.0f vy apres=%.1f hookCd=%.2f (attendu impact, drone mort, rebond)\n",
            wasHooked, impactSp, E[0].alive, E[0].hp, P.vel.y, P.hookCd);

    /* corps a corps : a l'arret puis lance a 20 m/s, avec recharge du dash */
    for (int pass = 0; pass < 2; pass++) {
        reset_game();
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0;
        P.vel = pass ? v3(0, 0, -20) : v3(0, 0, 0);
        P.dashCharge = 0.2f;
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.6f, 38);
        melee();
        fprintf(f, "corps a corps a %.0f m/s : hp drone=%.0f vivant=%d dash=%.1f (attendu %s, dash +1)\n",
                vlen(P.vel), E[0].hp, E[0].alive, P.dashCharge, pass ? "drone mort" : "hp 65");
    }

    /* parry d'une orbe : elle repart vers le drone vise et le detruit */
    reset_game();
    P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0.05f; P.hp = 50;
    E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 3, 20);
    spawn_proj(PR_ORB, v3(-20, 1.7f, 38), v3(0, 0, 16));
    melee();
    int kindAfter = PR[0].kind;
    hitstop = 0;
    for (int i = 0; i < 120; i++) proj_tick(TICK);
    fprintf(f, "parry : projectile renvoye=%d hp joueur=%.0f drone vivant=%d (attendu renvoye=1, hp 75 = 50 + 10 parry + 15 kill, drone mort)\n",
            kindAfter == PR_PARRIED, P.hp, E[0].alive);

    /* chaine de tir : 3 touches puis un rate */
    reset_game();
    P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0;
    float dmgSeen[3];
    for (int s = 0; s < 3; s++) {
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-20, 1.6f, 30);
        fire_rail();
        dmgSeen[s] = 1000 - E[0].hp;
    }
    int before = shotStreak;
    P.yaw = PI;                                           /* on vise le mur derriere */
    fire_rail();
    fprintf(f, "chaine : degats %.0f / %.0f / %.0f, chaine=%d puis apres un rate=%d (attendu degats croissants, 3 puis 0)\n",
            dmgSeen[0], dmgSeen[1], dmgSeen[2], before, shotStreak);

    /* loadout et munitions a jauge : spawn, conversion des points, surplus, maximum */
    reset_game();
    {
        int spawnOk = P.ammo[WP_RAIL] == 2 && P.ammo[WP_ROCKET] == 4 && P.ammoGauge[WP_RAIL] == 0 &&
                      P.weapon == WP_RAIL && P.meleeKind == MK_SWORD;
        P.weapon = WP_ROCKET; P.ammo[WP_ROCKET] = 0;
        ammo_points(50, AP_HIT);
        int conv1 = P.ammo[WP_ROCKET]; float g1 = P.ammoGauge[WP_ROCKET];
        ammo_points(70, AP_HIT);
        int conv2 = P.ammo[WP_ROCKET]; float g2 = P.ammoGauge[WP_ROCKET];
        P.weapon = WP_RAIL; P.ammo[WP_RAIL] = 2; P.ammo[WP_ROCKET] = 4; P.ammoGauge[WP_ROCKET] = 0;
        ammo_points(100, AP_HIT);                          /* rail plein : tout part aux roquettes */
        int spill = P.ammo[WP_ROCKET];
        P.weapon = WP_ROCKET; P.ammo[WP_ROCKET] = 7; P.ammoGauge[WP_ROCKET] = 30;
        float lostBefore = apStat[AP_LOST];
        ammo_points(100, AP_HIT);                          /* 30 + 100 : une roquette, puis le maximum */
        int atMax = P.ammo[WP_ROCKET]; float gMax = P.ammoGauge[WP_ROCKET], fMax = gauge_frac(WP_ROCKET);
        float lost = apStat[AP_LOST] - lostBefore;
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = PI; P.pitch = -0.5f;
        fire_rocket();                                     /* un rocket jump coute une roquette */
        fprintf(f, "jauges : spawn ok=%d, 50 pts -> %d (jauge %.0f), +70 -> %d (jauge %.0f), surplus -> roquettes %d, "
                   "maximum %d jauge %.0f affichee %.0f perdus %.0f, apres un tir %d (attendu 1, 1 0, 2 20, 6, 8 0 1 80, 7)\n",
                spawnOk, conv1, g1, conv2, g2, spill, atMax, gMax, fMax, lost, P.ammo[WP_ROCKET]);
    }

    /* l'epee remplit la jauge de l'arme en main : touche +50, kill +100 */
    reset_game();
    {
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0;
        P.weapon = WP_ROCKET; P.ammo[WP_ROCKET] = 0;
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.6f, 38);
        melee();
        int afterHit = P.ammo[WP_ROCKET];
        E[0].hp = 10;
        melee();
        fprintf(f, "epee -> munitions : touche %d roquette(s), touche + kill %d (attendu 1, 4)\n", afterHit, P.ammo[WP_ROCKET]);
    }

    /* cellules : deux tailles, points aux deux armes, refusee si tout est plein, reapparition */
    reset_game();
    {
        int cell = 6;                                      /* une petite cellule au sol, dans un coin */
        P.ammo[WP_RAIL] = 0; P.ammo[WP_ROCKET] = 2;
        P.pos = P.prevPos = v3(PU[cell].pos.x, 0, PU[cell].pos.z);
        pickup_tick(TICK);
        float railG = P.ammoGauge[WP_RAIL]; int rockets = P.ammo[WP_ROCKET], gone = PU[cell].t > 0;
        P.ammo[WP_RAIL] = 2; P.ammo[WP_ROCKET] = 8; PU[cell].t = 0;
        pickup_tick(TICK);
        int refused = PU[cell].t <= 0;
        PU[cell].t = PICKUP_RESPAWN;
        P.pos = P.prevPos = v3(0, 0, 30);
        for (int i = 0; i < (int)(PICKUP_RESPAWN / TICK) + 2; i++) pickup_tick(TICK);
        fprintf(f, "cellules : %d cellules (grandes %d %d), rail jauge 0 -> %.0f, roquettes 2 -> %d, prise=%d, pleine refusee=%d, "
                   "revenue apres 12 s=%d (attendu 13, 150 150, 100, 4, 1, 1, 1)\n",
                npu, PU[0].pts, PU[5].pts, railG, rockets, gone, refused, PU[cell].t <= 0);
    }

    /* pieces : un drone hors de la ligne de mire, on tire dans la piece, le ricochet le trouve */
    for (int pass = 0; pass < 2; pass++) {
        reset_game();
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0.1f;
        P.vel = v3(0, 0, -20);                             /* a pleine vitesse */
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-34, 3, 22);
        throw_coin();
        if (pass) { P.yaw = 0.15f; throw_coin(); P.yaw = 0; }   /* deuxieme piece : chaine */
        for (int i = 0; i < 30; i++) { coin_tick(TICK); P.pos = vadd(P.pos, vmul(P.vel, TICK)); }
        int c0 = -1;
        for (int i = 0; i < MAXCOIN; i++) if (CO[i].alive && c0 < 0) c0 = i;
        V3 d = vsub(CO[c0].pos, eye_pos());
        P.yaw = atan2f(d.x, -d.z); P.pitch = asinf(d.y / vlen(d));
        lastChain = 0;
        float coinsLeft = P.coins;
        fire_rail();
        fprintf(f, "piece%s : chaine %d, degats au drone %.0f (attendu %.0f), pieces restantes %.0f" "\n",
                pass ? "s x2" : "", lastChain, 1000 - E[0].hp, pass ? RAIL_DMG * 2.25f : RAIL_DMG * 1.5f, coinsLeft);
    }

    /* --- Elan : degats selon la vitesse depensee, freinage, chaine, piece, slide, munitions, fenetre du dash --- */
    {
        static const float vs[4] = { 0, 9.5f, 15, 25 };
        float dm[4], hl[4];
        for (int s = 0; s < 4; s++) {                      /* 1. tir droit devant a 0 / 9,5 / 15 / 25 m/s */
            reset_game();
            P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0;
            P.ammo[WP_KINETIC] = 6; shotStreak = 0; P.kinDashT = 1;
            E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-20, 1.6f, 30);
            P.vel = v3(0, 0, -vs[s]);
            fire_kinetic();
            dm[s] = 1000 - E[0].hp; hl[s] = hlen(P.vel);
        }
        fprintf(f, "elan a 0 / 9.5 / 15 / 25 m/s : degats %.1f / %.1f / %.1f / %.1f, vitesse apres %.1f / %.1f / %.1f / %.1f "
                   "(attendu 20 / 67.5 / 95 / 95, 0 / 0 / 0 / 10)\n", dm[0], dm[1], dm[2], dm[3], hl[0], hl[1], hl[2], hl[3]);

        reset_game();                                      /* 2. vitesse sur le cote et en montee */
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0;
        P.ammo[WP_KINETIC] = 6; shotStreak = 0; P.kinDashT = 1;
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-20, 1.6f, 30);
        P.vel = v3(12, 6, 5);
        fire_kinetic();
        fprintf(f, "elan sur le cote a 13 m/s : degats %.1f, vitesse apres %.1f, vy %.1f (attendu 85, 0, 6)\n",
                1000 - E[0].hp, hlen(P.vel), P.vel.y);

        reset_game();                                      /* 3. tir rate : le freinage s'applique quand meme */
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 1.2f;
        P.ammo[WP_KINETIC] = 6; shotStreak = 0; P.kinDashT = 1;
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-20, 1.6f, 30);
        P.vel = v3(0, 0, -15);
        fire_kinetic();
        fprintf(f, "elan rate a 15 m/s : vitesse apres %.1f, chaine %d (attendu 0, 0)\n", hlen(P.vel), shotStreak);

        reset_game();                                      /* 4. chaine : 3 tirs a 15 m/s */
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0;
        P.ammo[WP_KINETIC] = 6; shotStreak = 0; P.kinDashT = 1;
        for (int s = 0; s < 3; s++) {
            E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-20, 1.6f, 30);
            P.vel = v3(0, 0, -15);
            fire_kinetic();
            dm[s] = 1000 - E[0].hp;
        }
        fprintf(f, "elan chaine a 15 m/s : degats %.2f / %.2f / %.2f (attendu 95 / 109.25 / 123.5)\n", dm[0], dm[1], dm[2]);

        reset_game();                                      /* 5. piece : ricochet x1,5 sur les degats convertis */
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0.1f;
        P.ammo[WP_KINETIC] = 6; shotStreak = 0; P.kinDashT = 1;
        P.vel = v3(0, 0, -20);
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-34, 3, 22);
        throw_coin();
        for (int i = 0; i < 30; i++) { coin_tick(TICK); P.pos = vadd(P.pos, vmul(P.vel, TICK)); }
        {
            int c0 = -1;
            for (int i = 0; i < MAXCOIN; i++) if (CO[i].alive && c0 < 0) c0 = i;
            V3 d = vsub(CO[c0].pos, eye_pos());
            P.yaw = atan2f(d.x, -d.z); P.pitch = asinf(d.y / vlen(d));
        }
        fire_kinetic();
        fprintf(f, "elan piece a 20 m/s : degats au drone %.1f, vitesse apres %.1f (attendu 142.5, 5)\n", 1000 - E[0].hp, hlen(P.vel));

        reset_game();                                      /* 6. slide : degats pleins, le tir arrete le slide */
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0;
        player_tick(TICK, 0, 0);                           /* pose au sol */
        P.ammo[WP_KINETIC] = 6; shotStreak = 0; P.kinDashT = 1;
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-20, 1.6f, 30);
        act[A_CROUCH] = 1; P.vel = v3(0, 0, -12);
        player_tick(TICK, 0, 0);
        int slid = P.sliding;
        fire_kinetic();
        player_tick(TICK, 0, 0);
        memset(act, 0, sizeof act);
        fprintf(f, "elan en slide : slide avant=%d, degats %.1f, slide apres=%d (attendu 1, 95, 0)\n", slid, 1000 - E[0].hp, P.sliding);

        reset_game();                                      /* 7. chargeur vide : clic sec, bascule, aucun freinage */
        P.pos = P.prevPos = v3(-20, 6, 40); P.yaw = 0; P.pitch = 0;
        P.loadout[0] = WP_KINETIC; P.loadout[1] = WP_RAIL; P.weapon = WP_KINETIC;
        P.ammo[WP_KINETIC] = 0; P.fireCd = 0; P.switchT = 0;
        P.vel = v3(0, 0, -15);
        botHeld[A_FIRE] = 1;
        game_tick(TICK);
        botHeld[A_FIRE] = 0;
        fprintf(f, "elan a sec : munitions %d, arme %s, vitesse %.1f (attendu 0, RAILGUN, 15)\n", P.ammo[WP_KINETIC],
                ammoDef[P.weapon].name, hlen(P.vel));

        /* 10 a 14. fenetre du dash-tir (en l'air, sans friction) */
        float wv[7];
        static const int pre[7] = { 0, 12, 13, 24, 73, 30, -1 };   /* ticks apres le dash ; -1 = cas 14 */
        for (int s = 0; s < 6; s++) {
            reset_game();
            memset(act, 0, sizeof act);
            P.pos = P.prevPos = v3(-20, 8, 40); P.yaw = 0; P.pitch = 0;
            P.dashCharge = 2; P.kinDashT = 1;
            if (s == 3) P.vel = v3(0, 0, -9.5f);
            player_tick(TICK, 0, 1);                       /* dash */
            for (int i = 0; i < pre[s]; i++) player_tick(TICK, 0, 0);
            if (s == 5) {                                  /* 14. deuxieme dash pendant le malus, puis 24 ticks */
                player_tick(TICK, 0, 1);
                for (int i = 0; i < 24; i++) player_tick(TICK, 0, 0);
            }
            wv[s] = kin_vdep();
            if (s == 3) wv[6] = hlen(P.vel);
        }
        fprintf(f, "dash-tir : vdep %.1f apres 0 tick (attendu 15)\n", wv[0]);
        fprintf(f, "dash-tir : vdep %.1f apres 12 ticks, %.1f apres 13 ticks (attendu 15, 0)\n", wv[1], wv[2]);
        fprintf(f, "dash-tir : vdep %.1f apres 24 ticks depuis 9.5 m/s, vitesse %.1f (attendu 9.5, 18)\n", wv[3], wv[6]);
        fprintf(f, "dash-tir : vdep %.1f apres 73 ticks (attendu 15)\n", wv[4]);
        fprintf(f, "dash-tir : vdep %.1f apres un 2e dash pendant le malus + 24 ticks (attendu 0)\n", wv[5]);

        reset_game();                                      /* 15. la melee remplit la jauge de l'Elan (80 pts) */
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0;
        P.loadout[0] = WP_KINETIC; P.loadout[1] = WP_RAIL; P.weapon = WP_KINETIC;
        P.ammo[WP_KINETIC] = 3; P.ammoGauge[WP_KINETIC] = 0;
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-20, 1.6f, 38);
        melee();
        P.meleeCd = 0; P.parryWin = 0; hitstop = 0;
        melee();
        fprintf(f, "elan jauge : 2 coups d'epee -> munitions %d, jauge %.0f (attendu 4, 20)\n", P.ammo[WP_KINETIC],
                P.ammoGauge[WP_KINETIC]);
    }

    /* grappin plus nerveux : vitesse atteinte en 0,5 s vers un mur */
    reset_game();
    P.pos = P.prevPos = v3(-20, 0, 30); P.yaw = PI; P.pitch = 0.35f;
    hookPressIn = 1; hookHeldIn = 1;
    float sp05 = 0;
    for (int i = 0, since = 0; i < 240 && since < 60; i++) {   /* mesure 0,5 s apres l'accroche */
        player_tick(TICK, 0, 0); hookPressIn = 0; sp05 = vlen(P.vel);
        if (P.hook == HOOK_ATTACHED) since++;
    }
    hookHeldIn = 0;
    fprintf(f, "grappin : vitesse 0,5 s apres l'accroche = %.1f m/s (attendu > 25)\n", sp05);

    /* --- Enclume (solo) --- */
    {   /* 1. pique depuis 13 m, drone a 2 m du point d'impact : onde, rebond, dash, points de jauge */
        reset_game();
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.6f, 38);
        P.dashCharge = 0.5f;
        P.meleeKind = MK_ANVIL; P.pos = P.prevPos = v3(-20, 13, 40); P.grounded = 0; P.meleeCd = 0;
        anvil_press();
        float dashBefore = P.dashCharge;
        for (int i = 0; i < 600 && P.anvilState == ANVIL_DIVE; i++) { dashBefore = P.dashCharge; player_tick(TICK, 0, 0); }
        fprintf(f, "enclume pique 13 m : |vy| %.1f, PV drone %.1f, vy apres %.1f, dash %+.2f, roquettes %d jauge %.0f "
                   "(attendu ~39.0, ~36.8, 9.0, +1.00, 5 25)\n",
                P.anvilLastVy, E[0].hp, P.vel.y, P.dashCharge - dashBefore - TICK / DASH_RECHARGE,
                P.ammo[WP_ROCKET], P.ammoGauge[WP_ROCKET]);
    }
    {   /* 2. pique rate : immobilise 0,6 s (ni deplacement, ni saut, ni dash), puis libre */
        reset_game();
        test_slam_drop(v3(-20, 13, 40), v3(0, 0, -5));
        int st0 = P.anvilState; float hv = hlen(P.vel);
        V3 p0 = P.pos;
        act[A_FWD] = 1;
        for (int i = 0; i < 36; i++) player_tick(TICK, 1, 1);
        float moved = vlen(vsub(P.pos, p0)); int st1 = P.anvilState;
        for (int i = 0; i < 48; i++) player_tick(TICK, 0, 0);
        act[A_FWD] = 0;
        fprintf(f, "enclume rate : etat %d, vitesse horizontale %.1f, deplacement en 0,3 s (saut et dash appuyes) %.2f m etat %d, "
                   "etat apres 0,7 s %d (attendu 2, 0.0, 0.00 2, 0)\n", st0, hv, moved, st1, P.anvilState);
    }
    {   /* 3. trop pres du sol : c'est le coup au sol, meme en l'air */
        reset_game();
        P.meleeKind = MK_ANVIL; P.pos = P.prevPos = v3(-20, 0.8f, 40); P.grounded = 0;
        anvil_press();
        fprintf(f, "enclume a 0,8 m du sol : coup au sol=%d, etat %d, recharge %.2f (attendu 1, 0, 0.80)\n",
                P.meleeT > 0, P.anvilState, P.meleeCd);
    }
    {   /* 4. pique bas (2 m) : facteur de hauteur kh = 0,5 */
        reset_game();
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-18.6f, 1.0f, 40);
        test_slam_drop(v3(-20, 2, 40), v3(0, 0, 0));
        fprintf(f, "enclume pique bas (2 m) : |vy| %.1f, kh %.2f, degats %.1f (attendu 31.6, 0.50, 29.5)\n",
                P.anvilLastVy, slam_kh(2), 100 - E[0].hp);
    }
    {   /* 5. plafond de la vitesse de chute */
        reset_game();
        test_slam_drop(v3(-20, 40, 40), v3(0, 0, 0));
        fprintf(f, "enclume pique de 40 m : |vy| %.1f (attendu 45.0)\n", P.anvilLastVy);
    }
    for (int pass = 0; pass < 2; pass++) {   /* 6. coup au sol : a l'arret, puis a 10 m/s */
        reset_game();
        P.meleeKind = MK_ANVIL; P.pos = P.prevPos = v3(-20, 0, 40); P.grounded = 1; P.yaw = 0; P.pitch = 0;
        P.vel = pass ? v3(0, 0, -10) : v3(0, 0, 0);
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.6f, 38);
        melee();
        fprintf(f, "enclume coup au sol a %.0f m/s : PV drone %.0f, recharge %.2f, fenetre de parry %.2f (attendu %s, 0.80, 0.10)\n",
                vlen(P.vel), E[0].hp, P.meleeCd, P.parryWin, pass ? "25" : "55");
    }
    {   /* 7. le dash annule le pique ; pack_state annonce le pique (ST_SLAM) */
        reset_game();
        P.meleeKind = MK_ANVIL; P.pos = P.prevPos = v3(-20, 13, 40); P.grounded = 0; P.pitch = 0;
        anvil_press();
        for (int i = 0; i < 5; i++) player_tick(TICK, 0, 0);
        NetPState s;
        pack_state(&s);
        float dc = P.dashCharge;
        player_tick(TICK, 0, 1);
        fprintf(f, "enclume dash pendant le pique : pad %d, etat %d, vy %.1f, charges %.2f -> %.2f, recharge %.2f "
                   "(attendu %d, 0, 1.8 (2 moins un tick de gravite), 2.00 -> 1.00, 0.80)\n",
                s.pad, P.anvilState, P.vel.y, dc, P.dashCharge, P.meleeCd, MK_ANVIL | ST_SLAM);
    }
    {   /* 8. pique sur le jump pad est : onde, puis le pad relance (pas d'immobilisation) */
        reset_game();
        test_slam_drop(v3(ms(16), 5, 0), v3(0, 0, 0));
        V3 L = v3(ms(13), 20, 0);
        fprintf(f, "enclume sur un jump pad : onde=%d, vitesse (%.1f, %.1f, %.1f), etat %d (attendu 1, (%.1f, %.1f, %.1f), 0)\n",
                P.anvilLandT == 0, P.vel.x, P.vel.y, P.vel.z, P.anvilState, L.x, L.y, L.z);
    }

    /* --- Eventail (solo) : motif, cone, degats selon la distance, recul, pieces, munitions --- */
    {
        V3 dA[SG_PELLETS], dB[SG_PELLETS], dC[SG_PELLETS], d0 = vnorm(v3(0.3f, -0.2f, -0.93f));
        unsigned int rng0 = rng;                           /* 1-2. meme graine = memes plombs, rng global intact */
        sg_pellet_dirs(d0, SG_HALF_ANGLE, 1234u, dA);
        sg_pellet_dirs(d0, SG_HALF_ANGLE, 1234u, dB);
        sg_pellet_dirs(d0, SG_HALF_ANGLE, 98765u, dC);
        fprintf(f, "eventail motif : meme graine identique %d, autre graine differente %d, rng global intact %d (attendu 1, 1, 1)\n",
                !memcmp(dA, dB, sizeof dA), memcmp(dA, dC, sizeof dA) != 0, rng == rng0);
        float amax[2] = { 0, 0 };                          /* 3. angle max sur 200 graines, debout puis en slide */
        for (int pass = 0; pass < 2; pass++)
            for (unsigned int sd = 1; sd <= 200; sd++) {
                sg_pellet_dirs(d0, SG_HALF_ANGLE * (pass ? SG_SLIDE_SPREAD : 1), sd * 7919u, dA);
                for (int k = 0; k < SG_PELLETS; k++)
                    amax[pass] = fmaxf(amax[pass], atan2f(vlen(vcross(d0, dA[k])), vdot(d0, dA[k])) * 180 / PI);
            }
        fprintf(f, "eventail cone : angle max %.2f deg debout, %.2f en slide (attendu <= 4.50, <= 2.25)\n", amax[0], amax[1]);

        static const float dz[3] = { 5, 10, 20 };          /* 4, 5 b, 5. drone a 5 / 10 / 20 m */
        int pel[3]; float hpD[3];
        for (int s = 0; s < 3; s++) {
            reset_game();
            P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0; P.grounded = 1;
            P.ammo[WP_SHOTGUN] = 6; shotStreak = 0;
            E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.6f, 40 - dz[s]);
            fire_shotgun();
            pel[s] = P.sgLastPellets; hpD[s] = E[0].hp; hitstop = 0;
        }
        fprintf(f, "eventail drone a 5 / 10 / 20 m : plombs %d / %d / %d, PV %.1f / %.1f / %.1f (attendu 10 / 10 / 4, 20 / 20 / ~78.5)\n",
                pel[0], pel[1], pel[2], hpD[0], hpD[1], hpD[2]);

        reset_game();                                      /* 9-10. recul en l'air, plafonds, au sol */
        P.pos = P.prevPos = v3(-20, 8, 40); P.grounded = 0; P.yaw = 0; P.pitch = -PI / 2; P.ammo[WP_SHOTGUN] = 6;
        float rv[4];
        P.vel = v3(0, 0, 0); fire_shotgun(); rv[0] = P.vel.y;
        P.vel = v3(0, 8.4f, 0); fire_shotgun(); rv[1] = P.vel.y;
        P.vel = v3(0, -10, 0); fire_shotgun(); rv[2] = P.vel.y;
        P.vel = v3(0, 0, -20); P.yaw = PI; P.pitch = 0; fire_shotgun(); rv[3] = hlen(P.vel);   /* tir arriere apres un dash */
        P.grounded = 1; P.vel = v3(3, 0, -5); P.pitch = -PI / 2; fire_shotgun();
        fprintf(f, "eventail recul : vy %.1f depuis 0, %.1f depuis 8.4, %.1f en chute a -10, vitesse %.1f (tir arriere a 20), "
                   "au sol (%.1f %.1f %.1f) (attendu 7.0, 9.0, -3.0, 20.0, (3.0 0.0 -5.0))\n",
                rv[0], rv[1], rv[2], rv[3], P.vel.x, P.vel.y, P.vel.z);
        hitstop = 0;

        reset_game();                                      /* 11. piece eclatee : drone a 2 m d'elle, tireur a 2 m */
        P.pos = P.prevPos = v3(-20, 0, 38); P.yaw = 0; P.pitch = 0; P.grounded = 1; P.ammo[WP_SHOTGUN] = 6; shotStreak = 0;
        CO[0].alive = 1; CO[0].owner = myId; CO[0].life = 3; CO[0].pos = v3(-20, 1.6f, 36);
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-18, 1.6f, 36);
        fire_shotgun();
        fprintf(f, "eventail eclat : pieces eclatees %d, plombs %d, PV drone %.1f, PV joueur %.0f, chaine %d (attendu 1, 0, 71.2, 100, 1)\n",
                sgBursts, P.sgLastPellets, E[0].hp, P.hp, shotStreak);
        hitstop = 0;

        reset_game();                                      /* 12. deux pieces a 2 m : reaction en chaine */
        P.pos = P.prevPos = v3(-20, 0, 38); P.yaw = 0; P.pitch = 0; P.grounded = 1; P.ammo[WP_SHOTGUN] = 6;
        CO[0].alive = 1; CO[0].owner = myId; CO[0].life = 3; CO[0].pos = v3(-20, 1.6f, 36);
        CO[1].alive = 1; CO[1].owner = myId; CO[1].life = 3; CO[1].pos = v3(-18, 1.6f, 36);
        fire_shotgun();
        fprintf(f, "eventail eclat en chaine : pieces eclatees %d, message \"%s\" (attendu 2, ECLAT x2)\n", sgBursts, popText);
        hitstop = 0;

        int wl0 = wantLoadout[0], wl1 = wantLoadout[1];    /* 14. loadout Eventail + rail : 3 au spawn, 3 tirs, bascule */
        wantLoadout[0] = WP_SHOTGUN; wantLoadout[1] = WP_RAIL;
        reset_game();
        waveDelay = 1000;                                  /* aucun drone pendant le test */
        int spawnAmmo = P.ammo[WP_SHOTGUN], spawnW = P.weapon, shots = 0;
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = PI; P.pitch = 0;
        botHeld[A_FIRE] = 1;
        for (int i = 0; i < 480 && P.weapon == WP_SHOTGUN; i++) {
            int a0 = P.ammo[WP_SHOTGUN];
            game_tick(TICK); hitstop = 0;
            shots += P.ammo[WP_SHOTGUN] < a0;
        }
        botHeld[A_FIRE] = 0;
        fprintf(f, "eventail munitions : au spawn %d (%s), %d tirs puis a sec : munitions %d, arme %s (attendu 3 (EVENTAIL), 3, 0, RAILGUN)\n",
                spawnAmmo, ammoDef[spawnW].name, shots, P.ammo[WP_SHOTGUN], ammoDef[P.weapon].name);

        P.weapon = WP_SHOTGUN; P.switchT = 0;              /* 15. a vide pendant 10 s : aucune recharge passive */
        for (int i = 0; i < 1200; i++) game_tick(TICK);
        fprintf(f, "eventail a vide 10 s : munitions %d, jauge %.0f (attendu 0, 0)\n", P.ammo[WP_SHOTGUN], P.ammoGauge[WP_SHOTGUN]);

        reset_game();                                      /* 16. a vide, Eventail en main : 2 coups d'epee = 1 cartouche */
        P.pos = P.prevPos = v3(-20, 0, 40); P.yaw = 0; P.pitch = 0;
        P.weapon = WP_SHOTGUN; P.ammo[WP_SHOTGUN] = 0; P.ammoGauge[WP_SHOTGUN] = 0;
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 1000; E[0].pos = v3(-20, 1.6f, 38);
        melee();
        P.meleeCd = 0; P.parryWin = 0; hitstop = 0;
        melee();
        hitstop = 0;
        int afterSword = P.ammo[WP_SHOTGUN]; float gSword = P.ammoGauge[WP_SHOTGUN];
        P.ammo[WP_SHOTGUN] = 5; P.ammoGauge[WP_SHOTGUN] = 0;   /* 17. a 5 : grande cellule (150) -> 6, jauge pleine */
        pickup_taken(0, myId);
        fprintf(f, "eventail jauge : 2 coups d'epee -> %d cartouche(s), jauge %.0f ; a 5 + grande cellule (%d) -> %d, jauge affichee %.0f "
                   "(attendu 1, 0 ; 150, 6, 100)\n", afterSword, gSword, PU[0].pts, P.ammo[WP_SHOTGUN], 100 * gauge_frac(WP_SHOTGUN));
        PU[0].t = 0;
        wantLoadout[0] = wl0; wantLoadout[1] = wl1;
    }

    /* LAN simule (hote sans socket) : un bot contre une cible immobile */
    netMode = NET_HOST; connected = 1; myId = 0; botMode = 2; appState = APP_GAME;
    botLog = f;
    for (int pass = 0; pass < 2; pass++) {
        reset_game();
        dmMode = 1;
        memset(NP, 0, sizeof NP);
        NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Bot");
        NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Cible");
        if (pass == 0) {                                   /* cross-up : cible a 10 m devant */
            NP[1].disp = v3(-20, 0, 31);
            P.pos = P.prevPos = v3(-20, 0, 41);
        } else {                                           /* epee : la cible fonce sur le bot */
            NP[1].disp = v3(-20, 0, 37.5f);
            vf(NP[1].st.vel, v3(0, 0, 10));
            P.pos = P.prevPos = v3(-20, 0, 41);
            P.vel = v3(0, 0, -10);
        }
        vf(NP[1].st.pos, NP[1].disp);
        P.yaw = 0; P.dashCharge = 2; P.hp = 100; P.dead = 0;
        float maxY = 0;
        int behind = 0, hitBehind = 0, swordHits = 0;
        float hpBefore = NP[1].hp;
        P.dashCharge = pass ? 0.2f : 2;
        if (pass) P.fireCd = 10;                           /* test de l'epee seule : pas de rail */
        for (int i = 0; i < (pass ? 30 : 360); i++) {
            nowT += TICK;
            bot_think(TICK);
            game_tick(TICK);
            maxY = fmaxf(maxY, P.pos.y);
            if (P.pos.z < NP[1].disp.z - 1) behind = 1;
            if (NP[1].hp < hpBefore) {
                if (behind) hitBehind = 1;
                if (P.meleeT > 0.1f) swordHits++;
                hpBefore = NP[1].hp;
            }
            if (!NP[1].alive) break;
        }
        if (pass == 0)
            fprintf(f, "cross-up : hauteur max %.1f m, passe derriere=%d, touche dans le dos=%d, PV cible=%.0f (attendu 1, 1)\n",
                    maxY, behind, hitBehind, NP[1].hp);
        else
            fprintf(f, "epee a %.0f m/s relatifs : %d coup(s), PV cible=%.0f (attendu ~%.0f), dash=%.1f (attendu +1)\n",
                    20.0f, swordHits, NP[1].hp, 100 - fminf(melee_dmg(20) * PVP_SCALE, PVP_HIT_CAP), P.dashCharge);
    }
    /* parry : coup dans le vide = recharge longue */
    reset_game();
    dmMode = 0;
    melee();
    for (int i = 0; i < 24; i++) game_tick(TICK);
    fprintf(f, "coup dans le vide : recharge restante %.2f s apres 0,2 s (attendu ~0,7 au lieu de 0,25)" "\n", P.meleeCd);

    /* ping-pong : une roquette adverse renvoyee deux fois, puis le plafond de degats par touche */
    reset_game();
    dmMode = 1;
    memset(NP, 0, sizeof NP);
    NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Moi");
    NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Tireur");
    NP[1].disp = v3(-20, 0, 24); vf(NP[1].st.pos, NP[1].disp);
    P.pos = P.prevPos = v3(-20, 0, 41); P.yaw = 0; P.pitch = 0;
    {
        Proj *r = spawn_proj(PR_ROCKET, v3(-20, 1.6f, 38.5f), v3(0, 0, 38));
        r->owner = 1; r->netId = 0x02000009;
        melee();
        float m1 = r->mult, s1 = vlen(r->vel);
        int ok1 = r->kind == PR_PARRIED && r->owner == myId;
        r->owner = 1; r->pos = v3(-20, 1.6f, 38.5f); r->vel = v3(0, 0, s1);   /* l'adversaire la re-pare */
        P.meleeCd = 0; P.parryWin = 0;
        melee();
        fprintf(f, "ping-pong : renvoi 1 ok=%d x%.2f a %.0f m/s, renvoi 2 x%.2f a %.0f m/s (attendu x1.25 a 36, x1.44 a 41)" "\n",
                ok1, m1, s1, r->mult, vlen(r->vel));
        r->alive = 0;
    }
    NP[1].hp = 100;
    net_hit(1, 200, v3(0, 0, 0), W_MELEE);
    fprintf(f, "plafond PvP : une touche de 200 laisse %.0f PV (attendu 40)" "\n", NP[1].hp);

    /* Elan en LAN simule : x0,7 et plafond 60 par l'hote, puis aucune recharge passive */
    {
        static const float vs[3] = { 0, 9.5f, 15 };
        float hpLeft[3];
        NP[1].disp = v3(-20, 0, 31); vf(NP[1].st.pos, NP[1].disp);
        for (int s = 0; s < 3; s++) {
            P.pos = P.prevPos = v3(-20, 0, 41); P.yaw = 0; P.pitch = 0;
            P.ammo[WP_KINETIC] = 6; shotStreak = 0; P.kinDashT = 1; hitstop = 0;
            NP[1].hp = 100; NP[1].alive = 1;
            P.vel = v3(0, 0, -vs[s]);
            fire_kinetic();
            hpLeft[s] = NP[1].hp;
        }
        fprintf(f, "elan PvP a 0 / 9.5 / 15 m/s : PV cible %.2f / %.2f / %.2f (attendu 86 / 52.75 / 40)\n",
                hpLeft[0], hpLeft[1], hpLeft[2]);
        reset_game();
        dmMode = 1;
        NP[1].hp = 100;
        P.pos = P.prevPos = v3(-20, 0, 41); P.yaw = PI; P.pitch = 0;   /* vers le mur : aucune touche */
        P.loadout[0] = WP_KINETIC; P.loadout[1] = WP_RAIL; P.weapon = WP_KINETIC;
        P.ammo[WP_KINETIC] = 6; P.ammoGauge[WP_KINETIC] = 0; P.fireCd = 0; P.switchT = 0;
        botHeld[A_FIRE] = 1;
        for (int i = 0; i < 119; i++) { nowT += TICK; game_tick(TICK); }
        botHeld[A_FIRE] = 0;
        fprintf(f, "elan tenu 0.99 s : munitions %d, jauge %.0f (attendu 4, 0)\n", P.ammo[WP_KINETIC], P.ammoGauge[WP_KINETIC]);
    }

    /* parry d'epee : je suis en fenetre de parry quand l'epee adverse arrive, puis plus */
    {
        reset_game();
        dmMode = 1;
        memset(NP, 0, sizeof NP);
        NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Moi");
        NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Bretteur");
        NP[1].disp = v3(-20, 0, 38.5f); vf(NP[1].st.pos, NP[1].disp);
        P.pos = P.prevPos = v3(-20, 0, 41); P.dashCharge = 0.5f;
        P.parryWin = 0.1f;
        host_tick(TICK);
        NetEvent e = ev_new(EV_HIT);
        e.from = 1; e.target = 0; e.sub = W_MELEE; e.f[0] = 60;
        vf(&e.f[1], v3(0, 0, 10));                        /* recul porte par le coup */
        P.vel = v3(0, 0, 0);
        host_event_in(&e);                                /* comme un paquet recu du client 1 */
        float hpParried = NP[0].hp, dash = P.dashCharge;
        V3 velParried = P.vel;
        P.parryWin = 0; nowT += 1;
        host_tick(TICK);
        e.eid++;
        P.vel = v3(0, 0, 0);
        host_event_in(&e);
        fprintf(f, "parry d'epee : PV apres coup pare=%.0f, dash=%.1f ; sans parry PV=%.0f (attendu 100, 1.5, 40)" "\n",
                hpParried, dash, NP[0].hp);
        fprintf(f, "recul d'un coup pare : %.1f m/s ; sans parry %.1f m/s (attendu 0 et 10 : le coup pare n'est pas rejoue)" "\n",
                velParried.z, P.vel.z);
    }

    /* Enclume en LAN : l'onde sur un joueur au sol, pas sur un joueur en l'air ; puis l'onde paree par la victime */
    {
        reset_game();
        dmMode = 1;
        memset(NP, 0, sizeof NP);
        NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Moi");
        NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Cible");
        NP[1].disp = v3(-20, 0, 38); vf(NP[1].st.pos, NP[1].disp);
        nowT += 1;
        test_slam_drop(v3(-20, 13, 40), v3(0, 0, 0));      /* 9. cible au sol a 2 m : f = 0,626, x 0,7 */
        float hpGround = NP[1].hp;
        NP[1].hp = 100; NP[1].disp.y = 1.6f; vf(NP[1].st.pos, NP[1].disp);
        test_slam_drop(v3(-20, 13, 40), v3(0, 0, 0));      /* pieds a 1,6 m : au-dessus de l'onde */
        fprintf(f, "enclume LAN : PV apres l'onde %.1f, cible en l'air (pieds a 1,6 m) %.0f, etat %d (attendu ~56.8, 100, 2)\n",
                hpGround, NP[1].hp, P.anvilState);

        reset_game();                                      /* 10. la victime pare l'onde (0,18 s plus tot : tolerance 0,20 s) */
        dmMode = 1;
        NP[1].hp = 100; NP[1].disp.y = 0; vf(NP[1].st.pos, NP[1].disp);
        nowT += 1;
        NP[1].lastParryT = nowT - 0.18;
        int parriesBefore = meleeParries;
        test_slam_drop(v3(-20, 13, 40), v3(0, 0, 0));
        float hpParried = NP[1].hp, vyAtt = P.vel.y, cdAtt = P.meleeCd;
        int rockets = P.ammo[WP_ROCKET];
        net_hit(1, 10, v3(0, 0, 0), W_MELEE);              /* une epee a la meme distance n'est plus paree (0,15 s) */
        fprintf(f, "enclume onde paree : PV cible %.0f, vy attaquant %.1f, recharge %.2f, parries +%d, roquettes %d (points rendus) ; "
                   "epee a 0,18 s : PV %.0f (attendu 100, 12.0, 1.00, +1, 4 ; 90)\n",
                hpParried, vyAtt, cdAtt, meleeParries - parriesBefore, rockets, NP[1].hp);

        reset_game();                                      /* 11. bot a l'Enclume, 4 m au-dessus d'une cible au sol : il pique */
        dmMode = 1;
        NP[1].hp = 100; NP[1].lastParryT = 0; NP[1].disp = v3(-20, 0, 38); vf(NP[1].st.pos, NP[1].disp);
        P.meleeKind = MK_ANVIL; P.pos = P.prevPos = v3(-20, 4, 40); P.grounded = 0;
        int wavesBefore = slamWaveHits, dived = 0;
        for (int i = 0; i < 120 && slamWaveHits == wavesBefore; i++) {
            nowT += TICK;
            bot_think(TICK);
            game_tick(TICK);
            dived |= P.anvilState == ANVIL_DIVE;
        }
        fprintf(f, "bot enclume : pique=%d, onde qui touche=%d, PV cible %.0f (attendu 1, 1, < 100)\n",
                dived, slamWaveHits - wavesBefore, NP[1].hp);
    }

    /* Eventail en LAN simule : un seul EV_HIT par tir (agregation), x0,7, plafond 60, chaine a 3 plombs, traceurs recus */
    {
        static const float dist[5] = { 3, 10, 3, 30, 10 };
        static const int streak0[5] = { 0, 0, 8, 2, 2 };
        float hpLeft[5]; int evHits[5], streakAfter[5], pel[5];
        for (int s = 0; s < 5; s++) {
            reset_game();
            dmMode = 1;
            memset(NP, 0, sizeof NP);
            NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Moi");
            NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Cible");
            P.pos = P.prevPos = v3(-20, 0, 60); P.grounded = 1;
            NP[1].disp = v3(-20, 0, 60 - dist[s]); vf(NP[1].st.pos, NP[1].disp);
            V3 d = vsub(np_center(1), eye_pos());
            P.yaw = atan2f(d.x, -d.z); P.pitch = asinf(d.y / vlen(d));
            P.ammo[WP_SHOTGUN] = 6; shotStreak = streak0[s]; hitstop = 0;
            int h0 = hitCount[W_SHOTGUN];
            fire_shotgun();
            hpLeft[s] = NP[1].hp; evHits[s] = hitCount[W_SHOTGUN] - h0; streakAfter[s] = shotStreak; pel[s] = P.sgLastPellets;
        }
        hitstop = 0;
        fprintf(f, "eventail PvP a 3 m : %d EV_HIT, %d plombs, PV %.1f (attendu 1, 10, 44)\n", evHits[0], pel[0], hpLeft[0]);
        fprintf(f, "eventail PvP a 10 m : %d EV_HIT, %d plombs, PV %.1f (attendu 1, 6 a 8 selon la rotation, %.1f = 100 - plombs x 5.6)\n",
                evHits[1], pel[1], hpLeft[1], 100 - pel[1] * SG_PELLET_DMG * PVP_SCALE);
        fprintf(f, "eventail PvP a 3 m, chaine 8 : PV %.1f (attendu 40 : plafond de 60)\n", hpLeft[2]);
        fprintf(f, "eventail PvP a 30 m, chaine 2 : %d plomb(s), chaine apres %d (attendu 1, 0 : moins de 3 plombs)\n", pel[3], streakAfter[3]);
        fprintf(f, "eventail PvP a 10 m, chaine 2 : %d plombs, chaine apres %d, PV %.1f (attendu 6 a 8, 3, %.1f = 100 - plombs x 7.28)\n",
                pel[4], streakAfter[4], hpLeft[4], 100 - pel[4] * SG_PELLET_DMG * 1.3f * PVP_SCALE);

        memset(TR, 0, sizeof TR);                          /* 13. EV_SHOT recu d'un autre joueur : 10 traceurs recalcules */
        NetEvent e = ev_new(EV_SHOT);
        e.from = 1; e.sub = SG_PELLETS; e.netId = 4242u; e.f[6] = SG_HALF_ANGLE;
        vf(e.f, v3(-20, 1.6f, 30)); vf(e.f + 3, v3(0, 0, 1));
        apply_event(&e);
        int live = 0;
        for (int i = 0; i < MAXTR; i++) live += TR[i].t < TR[i].life;
        fprintf(f, "eventail EV_SHOT recu : %d traceurs, recul visuel du tireur %.2f (attendu 10, 0.12)\n", live, NP[1].sgFireT);

        reset_game();                                      /* bot au loadout Eventail + rail, cible immobile a 8 m */
        dmMode = 1;
        NP[1].active = NP[1].alive = 1; NP[1].hp = 100; NP[1].disp = v3(-20, 0, 52); vf(NP[1].st.pos, NP[1].disp);
        P.pos = P.prevPos = v3(-20, 0, 60); P.yaw = 0; P.dashCharge = 0;   /* pas de cross-up : on veut le tir direct */
        P.loadout[0] = WP_RAIL; P.loadout[1] = WP_SHOTGUN; P.weapon = WP_RAIL;
        int hs0 = hitCount[W_SHOTGUN], hr0 = hitCount[W_RAIL];
        for (int i = 0; i < 90 && NP[1].alive; i++) { nowT += TICK; bot_think(TICK); game_tick(TICK); hitstop = 0; }
        fprintf(f, "bot eventail a 8 m : arme %s, touches eventail %d, rail %d (attendu EVENTAIL, >= 1, 0)\n",
                ammoDef[P.weapon].name, hitCount[W_SHOTGUN] - hs0, hitCount[W_RAIL] - hr0);
    }

    /* --- Scie --- */
    {
        int savedBot = botMode;
        botMode = 0;                                       /* pas de journal du bot dans les resultats */
        float pv[6];
        static const float rels[4] = { 0, 32, 41.5f, 51 };
        for (int k = 0; k < 4; k++) pv[k] = saw_dmg(rels[k]);
        fprintf(f, "scie formule : %.2f/%.2f %.2f/%.2f %.2f/%.2f %.2f/%.2f (attendu 30.00/21.00 60.00/42.00 74.25/51.97 88.50/60.00)\n",
                pv[0], fminf(pv[0] * PVP_SCALE, PVP_HIT_CAP), pv[1], fminf(pv[1] * PVP_SCALE, PVP_HIT_CAP),
                pv[2], fminf(pv[2] * PVP_SCALE, PVP_HIT_CAP), pv[3], fminf(pv[3] * PVP_SCALE, PVP_HIT_CAP));
        pv[4] = saw_dmg(48) * SAW_RETURN_PARRY_DMG_MULT; pv[5] = saw_dmg(57.5f) * SAW_RETURN_PARRY_DMG_MULT;
        fprintf(f, "scie renvoi : %.2f/%.2f a 48 m/s, %.2f/%.2f a 57.5 m/s (attendu 75.60/52.92 88.42/60.00)\n",
                pv[4], fminf(pv[4] * PVP_SCALE, PVP_HIT_CAP), pv[5], fminf(pv[5] * PVP_SCALE, PVP_HIT_CAP));

        /* 2. lancer et plantage dans le mur nord (18,4 m) */
        saw_test_reset(v3(-20, 0, 60), PI, 0);
        fire_saw();
        Proj *sp = saw_mine();
        V3 from = eye_pos();
        for (int i = 0; i < 600 && sp->sawState == SAW_FLY; i++) proj_tick(TICK);
        float dz = sp->pos.z - SAW_EMBED - from.z, drop = from.y - sp->pos.y;
        fprintf(f, "scie plantee : etat %d, normale (%.0f %.0f %.0f), distance %.1f m, chute %.2f m (calcul %.2f), munition %d, cd %.2f "
                   "(attendu %d, (0 0 -1), 18.4, chute ~ calcul, 0, 0.40)\n",
                sp->sawState, sp->sawN.x, sp->sawN.y, sp->sawN.z, dz, drop, 0.5f * SAW_GRAVITY * (dz / SAW_SPEED) * (dz / SAW_SPEED),
                P.ammo[WP_SAW], P.sawCd, SAW_PLANTED);

        /* 6. rappel au grappin : le joueur ne bouge pas */
        for (int i = 0; i < 30; i++) player_tick(TICK, 0, 0);
        {
            V3 p0 = P.pos, d = vsub(sp->pos, eye_pos());
            float dist0 = vlen(vsub(sp->pos, pcenter()));
            P.yaw = atan2f(d.x, -d.z); P.pitch = asinf(d.y / vlen(d));
            hookPressIn = 1; hookHeldIn = 1;
            int recallTick = -1, caughtTick = -1, hookAfter = -1;
            float vAt = -1, moved = -1, cdAt = 0;
            for (int i = 0; i < 240 && caughtTick < 0; i++) {
                player_tick(TICK, 0, 0); hookPressIn = 0;
                if (recallTick < 0 && saw_mine() && saw_mine()->sawState == SAW_RECALL) {
                    recallTick = i; hookAfter = P.hook; vAt = vlen(P.vel); moved = vlen(vsub(P.pos, p0)); cdAt = P.hookCd;
                }
                proj_tick(TICK);
                if (recallTick >= 0 && !saw_mine()) caughtTick = i;
            }
            hookHeldIn = 0;
            fprintf(f, "scie rappel : crochet %d, vitesse %.2f, deplacement %.3f m, recharge %.2f, rattrapee apres %.2f s (calcul %.2f), "
                       "munition %d (attendu 0, 0.00, 0.000, 0.50, ~calcul, 1)\n",
                    hookAfter, vAt, moved, cdAt, (caughtTick - recallTick) * TICK, (dist0 - SAW_CATCH_R) / SAW_RECALL_SPEED,
                    P.ammo[WP_SAW]);
        }
        /* 7. la scie plantee d'un autre est ignoree : le crochet s'accroche au mur derriere */
        saw_test_reset(v3(-20, 0, 60), PI, 0);
        {
            Proj *o = saw_test_put(v3(-20, 1.0f, 78.4f + SAW_EMBED), v3(0, 0, 0), SAW_PLANTED, 1);   /* dans la face -z du mur */
            o->sawN = v3(0, 0, -1);
            V3 d = vsub(o->pos, eye_pos());
            P.yaw = atan2f(d.x, -d.z); P.pitch = asinf(d.y / vlen(d));
            hookPressIn = 1; hookHeldIn = 1;
            for (int i = 0; i < 30 && P.hook != HOOK_ATTACHED; i++) { player_tick(TICK, 0, 0); hookPressIn = 0; }
            int hk = P.hook;
            hookHeldIn = 0;
            player_tick(TICK, 0, 0);
            fprintf(f, "scie d'un autre au grappin : crochet %d, scie toujours plantee %d (attendu %d, 1)\n", hk,
                    o->alive && o->sawState == SAW_PLANTED, HOOK_ATTACHED);
        }

        /* 3. touche sur un drone immobile a 10 m, retour, rattrapage (+0,5 dash) ; 4. lancer en courant a 10 m/s */
        for (int pass = 0; pass < 2; pass++) {
            saw_test_reset(v3(-20, 0, 40), 0, 0);
            E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.7f, 30);
            P.dashCharge = 1;
            if (pass) P.vel = v3(0, 0, -10);
            fire_saw();
            float speed0 = vlen(saw_mine()->vel), hpHit = -1;
            int retTick = -1, catchTick = -1;
            for (int i = 0; i < 240 && catchTick < 0; i++) {
                proj_tick(TICK);
                Proj *q = saw_mine();
                if (retTick < 0 && q && q->sawState == SAW_RETURN) { retTick = i; hpHit = E[0].hp; }
                if (!q) catchTick = i;
            }
            if (!pass)
                fprintf(f, "scie touche un drone a 10 m : PV %.1f, retour=%d, rattrapee=%d, munition %d, sawIdx %d, dash %.1f "
                           "(attendu ~39.9, 1, 1, 1, 0, 1.5)\n",
                        hpHit, retTick >= 0, catchTick >= 0, P.ammo[WP_SAW], P.sawIdx, P.dashCharge);
            else
                fprintf(f, "scie lancee a 10 m/s : vitesse %.1f, degats %.1f (attendu 42.0, ~75)\n", speed0, 100 - hpHit);
        }

        /* 5. ramassage a pied : 1,4 m -> rattrapee, 1,6 m -> toujours plantee */
        {
            int got[2];
            for (int k = 0; k < 2; k++) {
                float r = k ? 1.6f : 1.4f, dy = 0.9f + SAW_EMBED;
                saw_test_reset(v3(-20, 0, 35 + sqrtf(r * r - dy * dy)), 0, 0);
                saw_test_put(v3(-20, -SAW_EMBED, 35), v3(0, 0, 0), SAW_PLANTED, myId);
                proj_tick(TICK);
                got[k] = P.ammo[WP_SAW] == 1 && !saw_mine();
            }
            fprintf(f, "scie ramassee a pied : a 1.4 m %d, a 1.6 m %d (attendu 1, 0)\n", got[0], got[1]);
        }

        /* 9. renvoi avec aide a la visee : drone a 18 m, ma scie revient a 3 m */
        {
            float hp0 = 0, dash0 = 0, hpAfter = -1;
            int stAfter = -1, bnc = 0, stEnd = -1;
            float spd = 0;
            saw_test_reset(v3(-20, 0, 40), 0, 0);
            E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.7f, 22);
            { V3 d = vsub(E[0].pos, eye_pos()); P.pitch = asinf(d.y / vlen(d)); }
            P.hp = 50; P.dashCharge = 1; hp0 = P.hp; dash0 = P.dashCharge;
            Proj *q = saw_test_put(v3(-20, 1.6f, 37), v3(0, 0, 36), SAW_RETURN, myId);
            melee();
            stAfter = q->sawState; spd = vlen(q->vel); bnc = q->sawBounces;
            for (int i = 0; i < 60 && q->alive && q->sawState == SAW_FLY; i++) proj_tick(TICK);
            hpAfter = E[0].hp; stEnd = q->sawState;
            fprintf(f, "scie renvoi : etat %d, vitesse %.1f, renvois %d, PV joueur %+.0f, dash %+.1f, PV drone %.1f, etat %d "
                       "(attendu %d, 48.0, 1, +0, +0.0, ~24.4, %d)\n",
                    stAfter, spd, bnc, P.hp - hp0, P.dashCharge - dash0, hpAfter, stEnd, SAW_FLY, SAW_RETURN);
        }
        /* 10. cone de l'aide : drone a 5 degres (aide), puis a 10 degres (tout droit dans le viseur) */
        {
            float res[2];
            for (int k = 0; k < 2; k++) {
                float ang = (k ? 10.0f : 5.0f) * PI / 180;
                saw_test_reset(v3(-20, 0, 40), 0, 0);
                E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20 + 18 * tanf(ang), 1.6f, 22);
                Proj *q = saw_test_put(v3(-20, 1.6f, 37), v3(0, 0, 36), SAW_RETURN, myId);
                melee();
                V3 v = vnorm(q->vel), td = vsub(E[0].pos, q->pos);
                if (!k) res[k] = fabsf(atan2f(v.x, -v.z) - atan2f(td.x, -td.z)) * 180 / PI;   /* ecart horizontal avec le drone */
                else res[k] = acosf(clampf(vdot(v, view_dir()), -1, 1)) * 180 / PI;            /* ecart avec le viseur */
            }
            fprintf(f, "scie aide au renvoi : drone a 5 deg -> ecart %.2f deg avec le drone ; a 10 deg -> ecart %.2f deg avec le viseur "
                       "(attendu < 0.3, 0.00)\n", res[0], res[1]);
        }
        /* 11. garde-fous : 3 renvois deja faits, scie plantee, scie rappelee */
        {
            saw_test_reset(v3(-20, 0, 40), 0, 0);
            Proj *q = saw_test_put(v3(-20, 1.6f, 37), v3(0, 0, 36), SAW_RETURN, myId);
            q->sawBounces = SAW_BOUNCE_CHAIN;
            melee();
            int st3 = q->sawState;
            for (int i = 0; i < 60 && saw_mine(); i++) proj_tick(TICK);
            int caught = !saw_mine() && P.ammo[WP_SAW] == 1;
            saw_test_reset(v3(-20, 0, 40), 0, 0);
            q = saw_test_put(v3(-20, 1.6f, 38), v3(0, 0, 0), SAW_PLANTED, myId);
            melee();
            int stP = q->sawState;
            saw_test_reset(v3(-20, 0, 40), 0, 0);
            q = saw_test_put(v3(-20, 1.6f, 37), v3(0, 0, 45), SAW_RECALL, myId);
            melee();
            fprintf(f, "scie garde-fous : apres 3 renvois etat %d puis rattrapee %d ; plantee %d ; rappelee -> etat %d "
                       "(attendu %d 1, %d, %d)\n", st3, caught, stP, q->sawState, SAW_RETURN, SAW_PLANTED, SAW_FLY);
        }
        /* 12. piece frolee : relancee vers le haut, +2 s de vie, la scie ne devie pas */
        {
            saw_test_reset(v3(-20, 0, 40), 0, 0);
            CO[0].alive = 1; CO[0].owner = myId; CO[0].life = 3; CO[0].pos = v3(-19.5f, 1.55f, 35); CO[0].vel = v3(0, 0, 0);
            fire_saw();
            Proj *q = saw_mine();
            V3 v0 = q->vel;
            for (int i = 0; i < 30 && !(q->sawCoins & 1u); i++) proj_tick(TICK);
            fprintf(f, "scie et piece : vy piece %.1f, vie %.1f, scie etat %d, vx %.2f (attendu 11.0, 5.0, %d, %.2f)\n",
                    CO[0].vel.y, CO[0].life, q->sawState, q->vel.x, SAW_FLY, v0.x);
        }
        /* 13. perdue sous le plan de mort, rendue apres 3 s */
        {
            saw_test_reset(v3(-20, 0, 40), 0, 0);
            Proj *q = saw_test_put(v3(-20, -25, 30), v3(0, 0, -10), SAW_FLY, myId);
            proj_tick(TICK);
            int stL = q->sawState;
            int t2 = -1;
            for (int i = 0; i < 400 && t2 < 0; i++) { proj_tick(TICK); if (!saw_mine()) t2 = i + 1; }
            fprintf(f, "scie perdue : etat %d, rendue apres %.2f s, munition %d (attendu %d, 3.00, 1)\n", stL, t2 * TICK,
                    P.ammo[WP_SAW], SAW_LOST);
        }
        /* clic avec la scie plantee : clic sec, message, bascule sur l'autre arme */
        {
            saw_test_reset(v3(-20, 0, 40), 0, 0);
            saw_test_put(v3(-20, -SAW_EMBED, 22), v3(0, 0, 0), SAW_PLANTED, myId);
            P.ammo[WP_RAIL] = 2; P.fireCd = 0;
            botPress[A_FIRE] = 1;
            game_tick(TICK);
            fprintf(f, "scie plantee, clic : arme %s, message \"%s\" (attendu RAILGUN, SCIE PLANTEE (18 m))\n", ammoDef[P.weapon].name, popText);
        }

        /* LAN simule (hote) */
        reset_game();
        dmMode = 1;
        memset(NP, 0, sizeof NP);
        NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Moi");
        NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Lanceur");
        NP[1].disp = v3(-20, 0, 24); vf(NP[1].st.pos, NP[1].disp);
        matchOver = 0;
        {   /* 1 bis. combo lancer + renvoi, arbitre par l'hote */
            net_hit_saw(1, saw_dmg(32) * PVP_SCALE, v3(0, 0, 0), W_SAW, 0);
            net_hit_saw(1, saw_dmg(48) * SAW_RETURN_PARRY_DMG_MULT * PVP_SCALE, v3(0, 0, 0), W_SAW, 1);
            float hpStatic = NP[1].hp; int aliveStatic = NP[1].alive;
            NP[1].hp = 100;
            net_hit_saw(1, saw_dmg(32) * PVP_SCALE, v3(0, 0, 0), W_SAW, 0);
            net_hit_saw(1, saw_dmg(57.5f) * SAW_RETURN_PARRY_DMG_MULT * PVP_SCALE, v3(0, 0, 0), W_SAW, 1);
            fprintf(f, "scie combo PvP : lancer + renvoi a l'arret -> PV %.1f vivant %d ; renvoi en course -> vivant %d (attendu 5.1 1, 0)\n",
                    hpStatic, aliveStatic, NP[1].alive);
            NP[1].alive = 1; NP[1].hp = 100;
        }
        {   /* 8. parry d'une scie adverse : elle repart vers son lanceur a 40 m/s ; celui qui a pare juge la touche */
            P.pos = P.prevPos = v3(-20, 0, 41); P.yaw = 0; P.pitch = 0; hitstop = 0;
            Proj *q = saw_test_put(v3(-20, 1.6f, 39), v3(0, 0, SAW_SPEED), SAW_FLY, 1);
            melee();
            V3 to = vnorm(vsub(np_center(1), q->pos));
            int st8 = q->sawState, by = q->sawBy;
            float sp8 = vlen(q->vel), al = vdot(vnorm(q->vel), to);
            for (int i = 0; i < 120 && q->alive; i++) proj_tick(TICK);
            fprintf(f, "scie paree par moi : etat %d, par %d, vitesse %.1f, alignement vers le lanceur %.3f, PV du lanceur %.1f "
                       "(attendu %d, 0, 40.0, 1.000, ~49.5)\n", st8, by, sp8, al, NP[1].hp, SAW_PARRIED);
            NP[1].hp = 100;
        }
        {   /* reception : scie plantee d'un autre, puis rattrapee */
            NetEvent e = ev_new(EV_SAW);
            e.from = 1; e.sub = SAWE_PLANT; e.netId = (2u << 24) | ((unsigned int)PR_SAW << 20) | 9;
            vf(e.f, v3(-10, 1, 78.3f)); vf(e.f + 3, v3(1, 0, 0)); e.f[6] = 4;   /* face -z */
            apply_event(&e);
            Proj *q = find_proj(e.netId);
            int okP = q && q->kind == PR_SAW && q->owner == 1 && q->sawState == SAW_PLANTED && q->sawN.z < -0.5f;
            e.sub = SAWE_CATCH; e.eid++;
            apply_event(&e);
            fprintf(f, "scie reseau : copie plantee %d, retiree au rattrapage %d (attendu 1, 1)\n", okP, find_proj(e.netId) == NULL);
        }
        {   /* 15. netId distinct de la roquette, type PR_SAW dans les bits 20-23 */
            P.loadout[0] = WP_SAW; P.loadout[1] = WP_ROCKET; P.ammo[WP_SAW] = 1; P.ammo[WP_ROCKET] = 4; P.sawCd = 0; P.sawIdx = 0;
            P.pos = P.prevPos = v3(-20, 0, 41); P.yaw = PI; P.pitch = 0;
            fire_rocket();
            unsigned int rid = 0;
            for (int i = 0; i < MAXPR; i++) if (PR[i].alive && PR[i].kind == PR_ROCKET) rid = PR[i].netId;
            fire_saw();
            unsigned int sid = saw_mine() ? saw_mine()->netId : 0;
            /* 14. mort du lanceur (EV_KILL sur moi) : la scie disparait */
            NetEvent e = ev_new(EV_KILL);
            e.actor = 1; e.target = 0; e.sub = W_SAW;
            apply_event(&e);
            int sawsLeft = 0;
            for (int i = 0; i < MAXPR; i++) sawsLeft += PR[i].alive && PR[i].kind == PR_SAW && PR[i].owner == myId;
            fprintf(f, "scie netId : roquette %08X, scie %08X, type %u ; apres ma mort : scies %d, sawIdx %d (attendu differents, %d ; 0, 0)\n",
                    rid, sid, (sid >> 20) & 0xF, sawsLeft, P.sawIdx, PR_SAW);
        }
        memset(NP, 0, sizeof NP);
        dmMode = 0;
        botMode = savedBot;
    }

    /* --- Tesla --- (couloir x = -20, regard vers -z ; drones a hauteur des yeux, immobiles) */
    {
        int savedBot = botMode;
        botMode = 0;
        const float EY = EYE_STAND;
        /* 1. portee selon la vitesse */
        fprintf(f, "tesla portee a 0 / 20 / 60 m/s : %.1f / %.1f / %.1f (attendu 14.0 / 20.0 / 28.0)\n",
                tesla_range(v3(0, 0, 0)), tesla_range(v3(0, 0, 20)), tesla_range(v3(60, 0, 0)));

        /* 2. batterie 60, tir tenu 1 s dans le vide */
        tesla_test_reset(v3(-20, 0, 40), PI);
        int p0 = teslaPulses;
        tesla_test_run(120, 1);
        fprintf(f, "tesla tir tenu 1 s dans le vide : batterie %d, %d impulsions (attendu 40, 10)\n", P.ammo[WP_TESLA], teslaPulses - p0);

        /* 3. tir tenu 4 s avec 60, puis 5 s sans tirer : aucune recharge passive */
        tesla_test_reset(v3(-20, 0, 40), PI);
        tesla_test_run(480, 1);
        int b4 = P.ammo[WP_TESLA];
        const char *w4 = ammoDef[P.weapon].name;
        tesla_test_run(600, 0);
        fprintf(f, "tesla tenu 4 s : batterie %d, arme %s ; 5 s plus tard : batterie %d, jauge %.0f (attendu 0, RAILGUN ; 0, 0)\n",
                b4, w4, P.ammo[WP_TESLA], P.ammoGauge[WP_TESLA]);

        /* 4. drone de 100 PV a 10 m, 1 s : 10 impulsions x 7, au plus 5 chiffres de degats */
        tesla_test_reset(v3(-20, 0, 40), 0);
        Enemy *d0 = tesla_test_drone(0, v3(-20, EY, 30), 100);
        tesla_test_run(120, 1);
        tesla_test_run(1, 0);                              /* relache : les cumuls partent */
        int dn = 0;
        for (int i = 0; i < MAXDN; i++) dn += DN[i].t > 0;
        fprintf(f, "tesla drone a 10 m, 1 s : PV %.1f, %d chiffres de degats (attendu 30.0, <= 5)\n", d0->hp, dn);

        /* 5. meme drone, tir jusqu'au kill : envoi anticipe, sans attendre la fenetre */
        tesla_test_reset(v3(-20, 0, 40), 0);
        d0 = tesla_test_drone(0, v3(-20, EY, 30), 100);
        p0 = teslaPulses;
        int killTick = -1;
        for (int i = 0; i < 300 && killTick < 0; i++) { tesla_test_run(1, 1); if (!d0->alive) killTick = i + 1; }
        fprintf(f, "tesla jusqu'au kill : tue a l'impulsion %d, apres %.3f s (attendu 15, 1.400 +- 1 tick)\n", teslaPulses - p0,
                killTick * TICK);

        /* 6. drone a 16 m : hors de portee a l'arret, touche a 10 m/s (portee 17) */
        float hp6[2];
        for (int k = 0; k < 2; k++) {
            tesla_test_reset(v3(-20, 0, 40), 0);
            d0 = tesla_test_drone(0, v3(-20, EY, 24), 100);
            P.vel = k ? v3(0, 0, -10) : v3(0, 0, 0);
            tesla_test_run(120, 1); tesla_test_run(1, 0);
            hp6[k] = 100 - d0->hp;
        }
        fprintf(f, "tesla drone a 16 m : degats %.1f a l'arret, %.1f a 10 m/s (attendu 0.0, 70.0)\n", hp6[0], hp6[1]);

        /* 7. chaine : drones a 10, 15, 20 et 27 m dans l'axe ; 8. une boite entre le 2e et le 3e */
        float ch[2][4];
        for (int k = 0; k < 2; k++) {
            static const float dz[4] = { 10, 15, 20, 27 };
            tesla_test_reset(v3(-20, 0, 40), 0);
            for (int j = 0; j < 4; j++) tesla_test_drone(j, v3(-20, EY, 40 - dz[j]), 1000);
            if (k) {
                boxes[nb].mn = v3(-21, 0, 22); boxes[nb].mx = v3(-19, 3, 23); boxes[nb].kind = B_SOLID; nb++;
            }
            tesla_test_run(120, 1); tesla_test_run(1, 0);
            if (k) nb--;
            for (int j = 0; j < 4; j++) ch[k][j] = 1000 - E[j].hp;
        }
        fprintf(f, "tesla chaine : %.1f / %.1f / %.1f / %.1f (attendu 70.0 / 42.0 / 42.0 / 0.0)\n", ch[0][0], ch[0][1], ch[0][2], ch[0][3]);
        fprintf(f, "tesla chaine coupee par un mur : %.1f / %.1f / %.1f (attendu 70.0 / 42.0 / 0.0)\n", ch[1][0], ch[1][1], ch[1][2]);

        /* 9. relais : piece a 8 m dans l'axe, drone a 4 m de la piece hors de l'axe ; 1 s puis 2,1 s */
        tesla_test_reset(v3(-20, 0, 40), 0);
        d0 = tesla_test_drone(0, v3(-16, EY, 32), 100);
        CO[0].alive = 1; CO[0].owner = myId; CO[0].life = 4; CO[0].pos = v3(-20, EY, 32); CO[0].vel = v3(0, 0, 0);
        tesla_test_run(1, 1);
        float y9 = CO[0].pos.y;
        tesla_test_run(119, 1);
        tesla_flush();
        float t9 = CO[0].teslaT, dy9 = CO[0].pos.y - y9, hp9 = d0->hp;
        tesla_test_run(132, 0);
        fprintf(f, "tesla relais : actif %d (%.2f s restantes), ecart de hauteur %.3f, PV drone %.1f ; a 2,1 s piece vivante %d "
                   "(attendu 1, ~1.0, 0.000, 50.0 ; 0)\n", t9 > 0, t9, dy9, hp9, CO[0].alive);

        /* 10. pas de cumul : drone dans l'arc ET a 3 m d'un relais, 1 s */
        tesla_test_reset(v3(-20, 0, 40), 0);
        d0 = tesla_test_drone(0, v3(-20, EY, 31), 100);
        CO[0].alive = 1; CO[0].owner = myId; CO[0].life = 4; CO[0].pos = v3(-20, EY, 34); CO[0].vel = v3(0, 0, 0);
        tesla_test_run(120, 1); tesla_test_run(1, 0);
        fprintf(f, "tesla arc + relais sur la meme cible, 1 s : degats %.1f, relais actif %d (attendu 70.0 et pas 120, 1)\n",
                100 - d0->hp, CO[0].teslaT > 0);

        /* 11. plafond : 4 pieces traversees -> 3 relais */
        tesla_test_reset(v3(-20, 0, 40), 0);
        for (int j = 0; j < 4; j++) { CO[j].alive = 1; CO[j].owner = myId; CO[j].life = 4; CO[j].pos = v3(-20, EY, 36 - 2.0f * j); }
        tesla_test_run(1, 1);
        fprintf(f, "tesla 4 pieces traversees : %d relais actifs (attendu 3)\n", tesla_relays_of(myId));

        /* 12. la chaine de touches n'est ni augmentee ni remise a zero */
        tesla_test_reset(v3(-20, 0, 40), 0);
        tesla_test_drone(0, v3(-20, EY, 30), 1000);
        shotStreak = 3;
        tesla_test_run(120, 1); tesla_test_run(1, 0);
        int st12 = shotStreak;
        tesla_test_reset(v3(-20, 0, 40), PI);
        shotStreak = 3;
        tesla_test_run(120, 1); tesla_test_run(1, 0);
        fprintf(f, "tesla chaine de tir : %d apres 1 s sur un drone, %d apres 1 s dans le vide (attendu 3, 3)\n", st12, shotStreak);

        /* 17. rendu : arc_rand ne touche pas le rng global */
        unsigned int r0 = rng;
        for (int k = 0; k < 50; k++) arc_rand();
        fprintf(f, "tesla rendu : rng global intact %d (attendu 1)\n", rng == r0);

        /* 13. LAN simule : cible a 10 m, 1 s -> 5 net_hit de 9,8 */
        tesla_test_reset(v3(-20, 0, 40), 0);
        dmMode = 1; matchOver = 0;
        memset(NP, 0, sizeof NP);
        NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Moi");
        NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Cible");
        NP[1].disp = v3(-20, 0, 30); vf(NP[1].st.pos, NP[1].disp);
        int h13 = hitCount[W_TESLA];
        tesla_test_run(60, 1);
        NetPState ps;
        pack_state(&ps);
        tesla_test_run(60, 1); tesla_test_run(1, 0);
        fprintf(f, "tesla LAN a 10 m, 1 s : %d EV_HIT, PV %.1f, ST_ZAP envoye %d (attendu 5, 51.0, 1)\n", hitCount[W_TESLA] - h13,
                NP[1].hp, (ps.pad & ST_ZAP) != 0);

        /* 14. parry : la victime pare, l'hote renvoie la decharge ; le tireur (moi) est etourdi */
        NP[1].hp = 100; NP[0].hp = 100; NP[1].lastParryT = nowT; NP[0].teslaReflT = 0;
        int refl0 = teslaReflects;
        net_hit(1, 9.8f, v3(0, 0, 0), W_TESLA);
        float stun14 = P.teslaStunT, hpV = NP[1].hp, hpS = NP[0].hp;
        net_hit(1, 9.8f, v3(0, 0, 0), W_TESLA);            /* deja en vol : jete */
        hitstop = 0;
        p0 = teslaPulses;
        tesla_test_run(48, 1);
        int during = teslaPulses - p0;
        tesla_test_run(1, 1);
        fprintf(f, "tesla parry : PV victime %.0f, PV tireur %.0f, etourdi %.2f s, renvois %d, 2e touche : PV %.0f / %.0f, "
                   "impulsions pendant 48 ticks %d, au 49e %d (attendu 100, 85, 0.40, 1, 100 / 85, 0, 1)\n",
                hpV, hpS, stun14, teslaReflects - refl0, NP[1].hp, NP[0].hp, during, teslaPulses - p0 - during);

        /* EV_TESLA sous-type 0 recu : la piece d'un autre devient son relais */
        {
            CO[5].alive = 1; CO[5].owner = 1; CO[5].netId = 0x02000042; CO[5].pos = v3(0, 5, 0); CO[5].vel = v3(0, -3, 0); CO[5].teslaT = 0;
            NetEvent e = ev_new(EV_TESLA);
            e.from = 1; e.sub = 0; e.netId = 0x02000042; vf(e.f, v3(1, 6, 1));
            apply_event(&e);
            fprintf(f, "tesla relais recu : actif %d, par %d, vitesse %.1f (attendu 1, 1, 0.0)\n", CO[5].teslaT > 0, CO[5].teslaBy,
                    vlen(CO[5].vel));
            CO[5].alive = 0;
        }

        /* 16. recalcul chez les autres : NP[1] rejoue mon tir (etat de pack_state) -> memes cibles */
        tesla_test_reset(v3(-20, 0, 40), 0);
        dmMode = 1;
        memset(NP, 0, sizeof NP);
        NP[0].active = NP[0].alive = 1; NP[0].hp = 100;
        for (int j = 2; j < 4; j++) { NP[j].active = NP[j].alive = 1; NP[j].hp = 100; }
        NP[2].disp = v3(-20.3f, 0, 29); NP[3].disp = v3(-16, 0, 27);
        P.pitch = -0.03f; P.vel = v3(0, 0, -5);
        TeslaTgt ta[3], tb[3];
        int na = tesla_trace(eye_pos(), view_dir(), tesla_range(P.vel), myId, ta, NULL, NULL);
        pack_state(&NP[1].st);
        NP[1].disp = P.pos; NP[1].active = NP[1].alive = 1;
        P.dead = 1;                                        /* moi hors du jeu : je ne masque plus la copie */
        int nb2 = tesla_trace(np_eye(1), yp_dir(NP[1].st.yaw, NP[1].st.pitch), tesla_range(np_vel(1)), 1, tb, NULL, NULL);
        int same = na == nb2;
        for (int k = 0; same && k < na; k++) same = ta[k].kind == tb[k].kind && ta[k].idx == tb[k].idx;
        fprintf(f, "tesla recalcul distant : %d cible(s) chez moi, %d chez l'autre, identiques %d, premiere %d (attendu 2, 2, 1, 2)\n",
                na, nb2, same, na ? ta[0].idx : -1);

        /* 15. points avec la Tesla en main (1 point = 0,4 de batterie) */
        tesla_test_reset(v3(-20, 0, 40), 0);
        P.ammo[WP_TESLA] = 0; P.ammo[WP_RAIL] = 0;
        int b15[6];
        ammo_points(50, AP_HIT); b15[0] = P.ammo[WP_TESLA];
        ammo_points(PTS_MELEE_KILL, AP_KILL); b15[1] = P.ammo[WP_TESLA];
        ammo_points(PTS_PARRY, AP_PARRY); b15[2] = P.ammo[WP_TESLA];
        {
            int pts0 = PU[0].pts;
            PU[0].pts = PICKUP_BIG; pickup_taken(0, myId); PU[0].t = 0; PU[0].pts = pts0;
        }
        b15[3] = P.ammo[WP_TESLA];
        P.ammo[WP_TESLA] = 90; P.ammoGauge[WP_RAIL] = 0;
        ammo_points(100, AP_HIT); b15[4] = P.ammo[WP_TESLA];
        fprintf(f, "tesla points : epee %d, + kill %d, + parry %d, + grande cellule %d ; a 90 + 100 points %d, jauge du rail %.0f "
                   "(attendu 20, 60, 80, 100 ; 100, 75)\n", b15[0], b15[1], b15[2], b15[3], b15[4], P.ammoGauge[WP_RAIL]);

        /* bot au loadout Tesla + rail, cible immobile a 12 m : il tient l'arc */
        botMode = 2;
        tesla_test_reset(v3(-20, 0, 40), 0);
        dmMode = 1;
        memset(NP, 0, sizeof NP);
        NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Bot");
        NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Cible");
        NP[1].disp = v3(-20, 0, 28); vf(NP[1].st.pos, NP[1].disp);
        P.dashCharge = 0; P.coins = 0;
        P.loadout[0] = WP_RAIL; P.loadout[1] = WP_TESLA; P.weapon = WP_RAIL;
        FILE *savedLog = botLog;
        botLog = NULL;                                     /* journal du bot hors des resultats */
        int ht0 = hitCount[W_TESLA], hr0 = hitCount[W_RAIL];
        for (int i = 0; i < 120 && NP[1].alive; i++) { nowT += TICK; bot_think(TICK); game_tick(TICK); hitstop = 0; }
        botLog = savedLog;
        fprintf(f, "bot tesla a 12 m : arme %s, touches tesla %d, rail %d (attendu TESLA, >= 2, 0)\n", ammoDef[P.weapon].name,
                hitCount[W_TESLA] - ht0, hitCount[W_RAIL] - hr0);

        memset(NP, 0, sizeof NP);
        memset(CO, 0, sizeof CO);
        dmMode = 0;
        botMode = savedBot;
    }
    /* --- fin Tesla --- */
    /* --- Lance (solo) : couloir degage x = -20, lance_press / lance_tick appeles directement --- */
    dmMode = 0;
    {   /* 1. joute contre un drone immobile a 20 m/s */
        lance_test_reset(v3(-20, 0, 40));
        P.vel = v3(0, 0, -20); P.dashCharge = 0.2f;
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.6f, 30); E[0].vel = v3(0, 0, 0);
        float reachHit = -1;
        lance_press();
        for (int i = 0; i < 300 && P.lanceState != LS_IDLE; i++) {
            float reach = vlen(vsub(E[0].pos, eye_pos())) - enemy_radius(EN_DRONE);
            lance_tick(TICK, 1);
            if (!E[0].alive && reachHit < 0) reachHit = reach;
            P.pos = P.prevPos = vadd(P.pos, vmul(P.vel, TICK));
        }
        hitstop = 0;
        fprintf(f, "lance joute drone a 20 m/s : portee a la touche %.2f, drone vivant %d, vitesse apres %.1f, recharge %.2f, dash %.1f "
                   "(attendu <= 3.50, 0, 10.0, 1.00, 1.2)\n", reachHit, E[0].alive, vlen(P.vel), P.meleeCd, P.dashCharge);
    }
    {   /* 2. v_c insuffisante (8 m/s) : aucune touche en 2 s, puis relevee avec 0,5 s de recharge */
        lance_test_reset(v3(-20, 0, 40));
        P.vel = v3(0, 0, -8);
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.6f, 30); E[0].vel = v3(0, 0, 0);
        lance_press();
        int ticks = 0;
        for (; ticks < 300 && P.lanceState != LS_IDLE; ticks++) {
            lance_tick(TICK, 1);
            P.pos = P.prevPos = vadd(P.pos, vmul(P.vel, TICK));
        }
        fprintf(f, "lance a 8 m/s : PV drone %.0f, relevee apres %.2f s, etat %d, recharge %.2f (attendu 100, 2.00, 0, 0.50)\n",
                E[0].hp, ticks * TICK, P.lanceState, P.meleeCd);
    }
    {   /* 3. cone : drone decale de 2 m a 3 m devant (33,7 degres), v_c et portee suffisantes */
        lance_test_reset(v3(-20, 0, 40));
        P.vel = v3(0, 0, -20); P.lanceState = LS_COUCH; P.lanceT = 0.5f;
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-18, 1.6f, 37); E[0].vel = v3(0, 0, 0);
        lance_tick(TICK, 1);
        fprintf(f, "lance hors du cone (33.7 deg) : PV drone %.0f, etat %d (attendu 100, 2)\n", E[0].hp, P.lanceState);
    }
    {   /* 4. tap = estoc : drone a 2,5 m, touche relachee au tick suivant */
        lance_test_reset(v3(-20, 0, 40));
        E[0].alive = 1; E[0].type = EN_DRONE; E[0].hp = 100; E[0].pos = v3(-20, 1.6f, 37.5f); E[0].vel = v3(0, 0, 0);
        lance_press();
        float pw = P.parryWin;
        lance_tick(TICK, 0);
        hitstop = 0;
        fprintf(f, "lance estoc (tap) : PV drone %.0f, fenetre de parry %.2f, etat %d, recharge %.2f (attendu 70, 0.12, 0, 0.50)\n",
                E[0].hp, pw, P.lanceState, P.meleeCd);
    }
    {   /* 5. kamikaze a 10,5 m/s contre une lance couchee a l'arret */
        lance_test_reset(v3(-20, 0, 40));
        E[0].alive = 1; E[0].type = EN_RUSHER; E[0].hp = 50; E[0].pos = v3(-20, 1.6f, 32); E[0].vel = v3(0, 0, 10.5f);
        lance_press();
        int jo = lanceJousts;
        for (int i = 0; i < 240 && E[0].alive; i++) {
            lance_tick(TICK, 1);
            if (E[0].alive) E[0].pos = vadd(E[0].pos, vmul(E[0].vel, TICK));
        }
        hitstop = 0;
        fprintf(f, "lance contre kamikaze : kamikaze vivant %d, joute %d, PV joueur %.0f (attendu 0, 1, 100)\n",
                E[0].alive, lanceJousts - jo, P.hp);
    }
    memset(E, 0, sizeof E);

    /* --- Lance en LAN simule (je suis l'hote) --- */
    {   /* 6. joute a v_c 15 puis 20 : x 0,7, plafond 60 */
        float hpL[2];
        for (int s = 0; s < 2; s++) {
            lance_test_reset(v3(-20, 0, 40));
            lance_test_lan(v3(-20, 0, 37));
            V3 dir = vnorm(vsub(np_center(1), eye_pos()));
            P.vel = vmul(dir, s ? 20.0f : 15.0f); P.lanceState = LS_COUCH; P.lanceT = 0.5f;
            lance_tick(TICK, 1);
            hpL[s] = NP[1].hp; hitstop = 0;
        }
        fprintf(f, "lance PvP a v_c 15 / 20 : PV cible %.1f / %.1f (attendu 40.5 / 40.0)\n", hpL[0], hpL[1]);
    }
    {   /* 7. la victime pare la lance couchee ; puis je pare une lance couchee : pas de recul */
        lance_test_reset(v3(-20, 0, 40));
        lance_test_lan(v3(-20, 0, 37));
        NP[1].lastParryT = nowT;
        int mp = meleeParries;
        V3 dir = vnorm(vsub(np_center(1), eye_pos()));
        P.vel = vmul(dir, 15); P.lanceState = LS_COUCH; P.lanceT = 0.5f;
        lance_tick(TICK, 1);
        hitstop = 0;
        float hpP = NP[1].hp, cdP = P.meleeCd;
        int stP = P.lanceState;
        mp = meleeParries - mp;
        lance_test_reset(v3(-20, 0, 40));
        lance_test_lan(v3(-20, 0, 37));
        P.parryWin = 0.1f;
        host_tick(TICK);
        NetEvent e = ev_new(EV_HIT);
        e.eid = 0x0277AA01u; e.from = 1; e.target = 0; e.sub = W_LANCE; e.f[0] = 59.5f; vf(&e.f[1], v3(0, 3, 20));
        e.f[4] = 15; e.f[5] = 15; e.f[6] = 1;
        P.vel = v3(0, 0, 0);
        host_event_in(&e);
        hitstop = 0;
        fprintf(f, "lance paree : PV cible %.0f, parries +%d, recharge attaquant %.2f, etat %d ; je pare : PV %.0f, recul %.1f m/s "
                   "(attendu 100, +1, 1.50, 0 ; 100, 0.0)\n", hpP, mp, cdP, stP, NP[0].hp, vlen(P.vel));
    }
    {   /* 8. choc de joute : NP[1] lance couchee vers moi, vitesse propre 15 / 19 / 25 contre mes 20 m/s */
        static const float sb[3] = { 15, 19, 25 };
        float hpOther[3], hpMe[3];
        int cl[3];
        lance_test_reset(v3(-20, 0, 41));
        lance_test_lan(v3(-20, 0, 38));
        for (int s = 0; s < 3; s++) {
            nowT += 1;
            NP[0].hp = 100; NP[1].hp = 100; NP[1].alive = 1; NP[0].alive = 1;
            NP[0].disp = P.pos; vf(NP[0].st.vel, v3(0, 0, -20));
            NP[1].st.yaw = PI; NP[1].st.pitch = 0; vf(NP[1].st.vel, v3(0, 0, sb[s]));
            NP[1].lastLanceT = nowT;
            int c0 = clashes;
            NetEvent e = ev_new(EV_HIT);
            e.target = 1; e.sub = W_LANCE; e.f[0] = 60; vf(&e.f[1], v3(0, 3, -12));
            e.f[4] = 20; e.f[5] = s == 2 ? 15.0f : 20.0f; e.f[6] = 1;
            net_emit(&e);
            hitstop = 0;
            hpOther[s] = NP[1].hp; hpMe[s] = NP[0].hp; cl[s] = clashes - c0;
        }
        NetEvent r = ev_new(EV_HIT);                      /* touche reciproque de NP[1] juste apres sa victoire */
        r.eid = 0x0277AA02u; r.from = 1; r.target = 0; r.sub = W_LANCE; r.f[0] = 60; r.f[4] = 25; r.f[5] = 15; r.f[6] = 1;
        host_event_in(&r);
        hitstop = 0;
        fprintf(f, "choc de joute 20 contre 15 : PV %.0f / %.0f ; 20 contre 19 : PV %.0f / %.0f, chocs +%d ; 20 contre 25 : PV %.0f / %.1f ; "
                   "reciproque ignoree : mes PV %.1f (attendu 40 / 100 ; 100 / 100, +1 ; 100 / 40.5 ; 40.5)\n",
                hpOther[0], hpMe[0], hpOther[1], hpMe[1], cl[1], hpOther[2], hpMe[2], NP[0].hp);
        (void)cl[0];
    }
    {   /* 9. grappin + lance : la joute part avant l'impact et consomme le crochet ; sans viser : impact du grappin */
        int dL[2], dH[2], hk[2];
        float cd[2];
        for (int s = 0; s < 2; s++) {
            lance_test_reset(v3(-20, 0, 40));
            lance_test_lan(v3(-20, 0, 25));
            V3 d = vsub(np_center(1), eye_pos());
            P.pitch = asinf(d.y / vlen(d));
            P.lanceState = LS_COUCH; P.lanceT = 0.2f;
            int l0 = hitCount[W_LANCE], h0 = hitCount[W_HOOK], attached = 0;
            hookPressIn = 1; hookHeldIn = 1;
            for (int i = 0; i < 240; i++) {
                nowT += TICK;
                player_tick(TICK, 0, 0);
                hookPressIn = 0;
                if (P.hook == HOOK_ATTACHED) { attached = 1; if (s) P.yaw = 1.2f; }   /* 2e passe : on regarde ailleurs */
                lance_tick(TICK, 1);
                if (attached && P.hook == HOOK_NONE) break;
            }
            hookHeldIn = 0; hitstop = 0;
            dL[s] = hitCount[W_LANCE] - l0; dH[s] = hitCount[W_HOOK] - h0; hk[s] = P.hook; cd[s] = P.hookCd;
        }
        fprintf(f, "grappin + lance : touches lance %d, grappin %d, crochet %d, recharge %.2f ; sans viser : lance %d, grappin %d "
                   "(attendu 1, 0, 0, 0.00 ; 0, 1)\n", dL[0], dH[0], hk[0], cd[0], dL[1], dH[1]);
    }
    {   /* 10. bot a la Lance : la cible fonce a 10 m/s, le bot a 10 m/s depuis 10 m */
        lance_test_reset(v3(-20, 0, 41));
        lance_test_lan(v3(-20, 0, 31));
        vf(NP[1].st.vel, v3(0, 0, 10));
        P.vel = v3(0, 0, -10); P.dashCharge = 0.2f; P.fireCd = 10;
        int l0 = hitCount[W_LANCE];
        for (int i = 0; i < 180 && hitCount[W_LANCE] == l0; i++) {
            nowT += TICK;
            bot_think(TICK);
            game_tick(TICK);
        }
        hitstop = 0;
        fprintf(f, "bot lance : touches lance %d, PV cible %.0f (attendu >= 1, 40)\n", hitCount[W_LANCE] - l0, NP[1].hp);
        memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
        reset_game();
        dmMode = 0;
        memset(NP, 0, sizeof NP);
    }
    /* --- fin Lance --- */

    /* --- Ventouse --- */
    {   /* 1. vol et collage au sol, puis contre un mur (8 m) */
        mine_test_reset(v3(-20, 0, 40), PI, -0.3f);
        fire_mine();
        Proj *m = mine_test_last();
        int ammo = P.ammo[WP_MINE];
        for (int i = 0; i < 120; i++) proj_tick(TICK);
        fprintf(f, "ventouse sol : etat %d, normale (%.0f %.0f %.0f), y %.2f, z %.1f, munitions %d, jauge %.0f "
                   "(attendu 2, (0 1 0), 0.06, entre 43 et 46, 0, 0)\n",
                m->mineState, m->mineN.x, m->mineN.y, m->mineN.z, m->pos.y, m->pos.z, ammo, P.ammoGauge[WP_MINE]);
        mine_test_reset(v3(-20, 0, 70), PI, 0.1f);
        fire_mine();
        m = mine_test_last();
        mine_test_stick(m, 0);
        fprintf(f, "ventouse mur : etat %d, normale (%.0f %.0f %.0f), z %.2f (attendu 2, (0 0 -1), face du mur moins 0.06)\n",
                m->mineState, m->mineN.x, m->mineN.y, m->mineN.z, m->pos.z);
    }
    {   /* 2. meche : 3 s depuis le collage */
        mine_test_reset(v3(-20, 0, 40), PI, -0.3f);
        fire_mine();
        Proj *m = mine_test_last();
        mine_test_stick(m, 0);
        int n = 0;
        while (n < 1000 && m->alive) { proj_tick(TICK); n++; }
        fprintf(f, "ventouse meche : explosion %d ticks apres le collage (attendu 360 +- 1)\n", n);
    }
    {   /* 3. mine-jump (sans sauter), compare au rocket jump sans saut */
        mine_test_reset(v3(-20, 0, 40), 0, -1.5f);
        fire_mine();
        Proj *m = mine_test_last();
        mine_test_stick(m, 1);
        int st = m->mineState;
        mine_detonate(m, MX_REMOTE);
        float top = 0;
        for (int i = 0; i < 200; i++) { proj_tick(TICK); player_tick(TICK, 0, 0); top = fmaxf(top, P.pos.y); }
        float hpM = P.hp;
        reset_game();
        P.pos = P.prevPos = v3(-20, 0, 40); P.pitch = -1.5f;
        fire_rocket();
        float topR = 0;
        for (int i = 0; i < 200; i++) { proj_tick(TICK); player_tick(TICK, 0, 0); topR = fmaxf(topR, P.pos.y); }
        fprintf(f, "ventouse mine-jump : collee %d, apogee %.2f, PV %.0f ; rocket jump sans saut %.2f (attendu 2, > 4.3 (~4.6), 96 ; ~4.0)\n",
                st, top, hpM, topR);
    }
    {   /* 4. drone colle : 90 garantis ; un 2e drone a 3 m de la mine */
        mine_test_reset(v3(-20, 0, 40), 0, 0.13f);
        tesla_test_drone(0, v3(-20, 1.6f, 30), 1000);
        fire_mine();
        Proj *m = mine_test_last();
        mine_test_stick(m, 0);
        int st = m->mineState;
        tesla_test_drone(1, vadd(m->pos, v3(3, 0, 0)), 1000);
        mine_detonate(m, MX_REMOTE);
        fprintf(f, "ventouse drone colle : etat %d, degats %.2f, 2e drone a 3 m %.2f (attendu 3, 90.00, 51.43)\n",
                st, 1000 - E[0].hp, 1000 - E[1].hp);
    }
    {   /* 5. rail dans une mine posee a 2 m d'un drone (le drone derriere la mine n'est pas touche par le rail) */
        mine_test_reset(v3(-20, 0, 40), 0, 0);
        Proj *m = mine_test_put(v3(-20, 1.6f, 30), MS_WORLD, myId);
        tesla_test_drone(0, v3(-20, 1.6f, 28), 1000);
        P.weapon = WP_RAIL; P.ammo[WP_RAIL] = 2;
        fire_rail();
        fprintf(f, "ventouse rail : mine %d, degats au drone %.2f, message \"%s\", chaine %d (attendu 0, 102.97, MINE x1,5 !, 1)\n",
                m->alive, 1000 - E[0].hp, popText, shotStreak);
        hitstop = 0;
    }
    {   /* 5b. un plomb de l'Eventail, puis l'arc de la Tesla, dans une mine a 2 m d'un drone (x1) */
        float dm[2];
        for (int k = 0; k < 2; k++) {
            mine_test_reset(v3(-20, 0, 40), 0, 0);
            mine_test_put(v3(-20, 1.6f, 34), MS_WORLD, myId);
            tesla_test_drone(0, v3(-18, 1.6f, 34), 1000);
            if (k == 0) { P.weapon = WP_SHOTGUN; P.ammo[WP_SHOTGUN] = 3; fire_shotgun(); }
            else { P.weapon = WP_TESLA; P.ammo[WP_TESLA] = 60; tesla_pulse(); tesla_flush(); }
            dm[k] = 1000 - E[0].hp;
            hitstop = 0;
        }
        fprintf(f, "ventouse plomb / arc : degats au drone %.2f / %.2f (attendu 64.29 / 64.29)\n", dm[0], dm[1]);
    }
    {   /* 6. limite de 2 : la 3e fait sauter la plus ancienne */
        mine_test_reset(v3(-20, 0, 40), PI, -0.3f);
        P.ammo[WP_MINE] = 3;
        unsigned int first = 0;
        for (int k = 0; k < 3; k++) {
            P.fireCd = 0;
            fire_mine();
            if (!k) first = mine_test_last()->mineSeq;
            for (int i = 0; i < 30; i++) proj_tick(TICK);
        }
        int n = 0, firstAlive = 0;
        for (int i = 0; i < MAXPR; i++)
            if (mine_own(&PR[i])) { n++; firstAlive |= PR[i].mineSeq == first; }
        fprintf(f, "ventouse limite : %d mines actives, la 1re vivante %d, munitions %d (attendu 2, 0, 0)\n", n, firstAlive, P.ammo[WP_MINE]);
    }
    {   /* 7. deux commandes : DETONER UNE (la plus ancienne), DETONER TOUT, puis sans mine (clic sec) */
        mine_test_reset(v3(-20, 0, 40), PI, -0.3f);
        P.ammo[WP_MINE] = 2;
        Proj *ma, *mb;
        fire_mine(); ma = mine_test_last();
        for (int i = 0; i < 30; i++) proj_tick(TICK);
        P.fireCd = 0;
        fire_mine(); mb = mine_test_last();
        for (int i = 0; i < 30; i++) proj_tick(TICK);
        botPress[A_DETONATE_ONE] = 1; game_tick(TICK); hitstop = 0;
        int aOne = ma->alive, bOne = mb->alive;
        botPress[A_DETONATE_ALL] = 1; game_tick(TICK); hitstop = 0;
        int bAll = mb->alive;
        float hp0 = P.hp;
        botPress[A_DETONATE_ONE] = 1; game_tick(TICK); hitstop = 0;
        fprintf(f, "ventouse commandes : UNE -> A %d B %d ; TOUT -> B %d ; sans mine : mines %d, PV %+.0f (attendu 0 1 ; 0 ; 0, +0)\n",
                aOne, bOne, bAll, mine_count(), P.hp - hp0);
    }
    {   /* 8. munitions par jauge : 3 coups d'epee = 1 mine, grande cellule = +1, plein a 3, aucune recharge passive */
        mine_test_reset(v3(-20, 0, 40), 0, 0);
        P.ammo[WP_MINE] = 0;
        tesla_test_drone(0, v3(-20, 1.6f, 38), 1000);
        for (int k = 0; k < 3; k++) { P.meleeCd = 0; P.parryWin = 0; hitstop = 0; melee(); }
        hitstop = 0; P.parryWin = 0;
        int sword = P.ammo[WP_MINE]; float gSword = P.ammoGauge[WP_MINE];
        int big = 0;
        for (int i = 0; i < npu; i++) if (PU[i].pts == PICKUP_BIG) { big = i; break; }
        pickup_taken(big, myId); PU[big].t = 0;
        int cell = P.ammo[WP_MINE];
        P.ammo[WP_MINE] = 3; P.ammoGauge[WP_MINE] = 0;
        pickup_taken(big, myId); PU[big].t = 0;
        int full = P.ammo[WP_MINE]; float gFull = gauge_frac(WP_MINE);
        E[0].alive = 0;
        P.ammo[WP_MINE] = 0; P.ammoGauge[WP_MINE] = 0;
        for (int i = 0; i < 3600; i++) { game_tick(TICK); hitstop = 0; }
        fprintf(f, "ventouse jauge : 3 coups d'epee -> %d mine(s), jauge %.0f ; grande cellule (%d) -> %d ; a 3 + cellule -> %d, "
                   "jauge affichee %.0f%% ; 30 s a vide -> %d, jauge %.0f (attendu 1, 0 ; 150, 2 ; 3, 100%% ; 0, 0)\n",
                sword, gSword, PU[big].pts, cell, full, 100 * gFull, P.ammo[WP_MINE], P.ammoGauge[WP_MINE]);
    }
    {   /* 9. LAN : mine collee sur un joueur, detonation demandee a 0,3 s, explosion a l'armement (0,6 s), 60 garantis */
        mine_test_reset(v3(-20, 0, 40), 0, 0);
        mine_test_lan(v3(-20, 0, 34));
        fire_mine();
        Proj *m = mine_test_last();
        mine_test_stick(m, 0);
        int st = m->mineState, at = m->mineAttach;
        int t = 0;
        for (; t < 36; t++) { nowT += TICK; proj_tick(TICK); }
        mine_detonate(m, MX_REMOTE);
        float hpMid = NP[1].hp;
        while (t < 600 && m->alive) { nowT += TICK; proj_tick(TICK); t++; }
        hitstop = 0;
        fprintf(f, "ventouse LAN : etat %d sur %d, PV a 0,3 s %.0f, explosion a %.2f s, PV %.0f (attendu 4 sur 2, 100, 0.60, 40)\n",
                st, at, hpMid, t * TICK, NP[1].hp);
    }
    {   /* 7b. LAN : A collee sur un joueur (pas armee), B au sol : deux DETONER UNE dans le meme 0,1 s */
        mine_test_reset(v3(-20, 0, 40), 0, 0);
        mine_test_lan(v3(-20, 0, 30));
        Proj *ma = mine_test_put(v3(-20, 1, 30.4f), MS_PLAYER, myId), *mb = mine_test_put(v3(-14, 0.06f, 30), MS_WORLD, myId);
        ma->mineAttach = 2; ma->mineOfs = v3(0, 1, 0.4f); ma->mineArmT = MINE_ARM_PLAYER;
        mine_remote(0);
        mine_remote(0);
        hitstop = 0;
        fprintf(f, "ventouse UNE x2 : A vivante %d (detonation en attente %d), B vivante %d (attendu 1 (1), 0)\n",
                ma->alive, ma->mineDet != 0, mb->alive);
        ma->alive = 0;
    }
    {   /* 10. arrachage : mine adverse collee sur moi, parry -> renvoyee dans ma visee a 36 m/s */
        mine_test_reset(v3(-20, 0, 40), 0.3f, 0.1f);
        mine_test_lan(v3(-20, 0, 30));
        Proj *m = mine_test_put(vadd(P.pos, v3(0, 1, 0.3f)), MS_PLAYER, 1);
        m->mineAttach = myId + 1; m->mineOfs = v3(0, 1, 0.3f); m->mineArmT = 0.3f;
        P.parryWin = 0.1f;
        int n = try_parry(MELEE_RANGE + 1.5f, 1);
        fprintf(f, "ventouse arrachage : pares %d, kind %d, owner %d, vitesse %.1f, alignement %.4f, mult %.2f, message \"%s\" "
                   "(attendu 1, %d, 0, 36.0, > 0.999, 1.25, ARRACHEE !)\n",
                n, m->kind, m->owner, vlen(m->vel), vdot(vnorm(m->vel), view_dir()), m->mult, popText, PR_PARRIED);
        m->alive = 0; P.parryWin = 0; hitstop = 0;
    }
    {   /* 11. arbitrage de l'hote : arrachage puis touche du poseur ; puis explosion, touche, arrachage tardif */
        mine_test_reset(v3(-20, 0, 40), 0, 0);
        mine_test_lan(v3(-20, 0, 30));
        unsigned int id1 = (1u << 24) | ((unsigned int)PR_MINE << 20) | 0x9001, id2 = id1 + 1;
        NetEvent e = ev_new(EV_MINE);
        e.sub = MINE_EV_TORN; e.from = 1; e.target = 1; e.netId = id1;
        int r1 = host_handle(&e);
        NetEvent h = ev_new(EV_HIT);
        h.from = 0; h.target = 1; h.sub = W_MINE; h.f[0] = 63; h.netId = id1;
        int r2 = host_handle(&h);
        float hpA = NP[1].hp;
        NP[1].hp = 100;
        int c0 = mineCancels;
        e.sub = MINE_EV_BOOM; e.from = 0; e.netId = id2;
        int r3 = host_handle(&e);
        h.netId = id2;
        int r4 = host_handle(&h);
        float hpB = NP[1].hp;
        e.sub = MINE_EV_TORN; e.from = 1;
        int r5 = host_handle(&e);
        fprintf(f, "ventouse arbitrage : arrachage %d puis touche du poseur annulee %d, PV %.0f ; explosion %d, touche %d, PV %.0f, "
                   "arrachage tardif annule %d, CANCEL %d (attendu 0 1, 100 ; 0 0, 40, 1, 1)\n",
                r1, r2, hpA, r3, r4, hpB, r5, mineCancels - c0);
    }
    {   /* 12. mine adverse volee au rail : son poseur pres de la mine prend le souffle (plafonne a 60) */
        mine_test_reset(v3(-20, 0, 41), 0, 0);
        mine_test_lan(v3(-18.5f, 0, 33));
        Proj *m = mine_test_put(v3(-20, 1.6f, 33), MS_WORLD, 1);
        P.weapon = WP_RAIL; P.ammo[WP_RAIL] = 2;
        fire_rail();
        hitstop = 0;
        fprintf(f, "ventouse volee : mine %d, PV du poseur %.0f, message \"%s\" (attendu 0, 40, MINE VOLEE !)\n", m->alive, NP[1].hp, popText);
    }
    {   /* 13. reception : copie d'une mine adverse, collee sur moi (alerte), puis explosee */
        mine_test_reset(v3(-20, 0, 40), 0, 0);
        mine_test_lan(v3(-20, 0, 30));
        unsigned int id = (2u << 24) | ((unsigned int)PR_MINE << 20) | 0x9100;
        NetEvent e = ev_new(EV_MINE);
        e.sub = MINE_EV_THROW; e.from = 1; e.netId = id; vf(e.f, v3(-20, 1.6f, 31)); vf(e.f + 3, v3(0, 0, 24));
        apply_event(&e);
        Proj *p = find_proj(id);
        int copy = p && p->kind == PR_MINE && p->owner == 1 && p->mineState == MS_FLY;
        e.sub = MINE_EV_PLAYER; e.target = 0; vf(e.f, v3(0, 1.2f, -0.4f));
        apply_event(&e);
        int onMe = p && p->mineState == MS_PLAYER && p->mineAttach == 1;
        char alert[32];
        snprintf(alert, sizeof alert, "%s", popText);
        e.sub = MINE_EV_BOOM; vf(e.f, v3(-20, 1.2f, 39.6f)); vf(e.f + 3, v3(0, 0, 0));
        apply_event(&e);
        fprintf(f, "ventouse reseau : copie %d, collee sur moi %d, alerte \"%s\", retiree %d (attendu 1, 1, MINE SUR TOI ! PARE !, 1)\n",
                copy, onMe, alert, find_proj(id) == NULL);
    }
    {   /* 14. bot a la Ventouse : cible immobile a 12 m, il lance, la mine colle, il la fait sauter une fois armee */
        mine_test_reset(v3(-20, 0, 41), 0, 0);
        mine_test_lan(v3(-20, 0, 29));
        P.ammo[WP_MINE] = 2; P.dashCharge = 0.2f; P.hookCd = 100;
        int t0 = mineThrows, h0 = hitCount[W_MINE], i = 0;
        for (; i < 480 && hitCount[W_MINE] == h0; i++) {
            nowT += TICK;
            bot_think(TICK);
            game_tick(TICK);
            P.hookCd = 100;
        }
        hitstop = 0;
        fprintf(f, "bot ventouse : lancers %d, touches ventouse %d apres %.2f s, PV cible %.0f (attendu >= 1, >= 1, < 100)\n",
                mineThrows - t0, hitCount[W_MINE] - h0, i * TICK, NP[1].hp);
        memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
        reset_game();
        dmMode = 0;
        memset(NP, 0, sizeof NP);
        memset(hostMine, 0, sizeof hostMine);
    }
    /* --- fin Ventouse --- */

    /* --- Fleau (solo) : couloir degage x = -20, joueur en (-20, 0, 40), vue vers -z --- */
    {   /* 1. degats et traction d'un drone a 5,5 m : il s'arrete vers 2,5 m (minimum : son orbite le ramene ensuite) */
        flail_test_reset(v3(-20, 0, 40));
        Enemy *e = flail_test_enemy(0, v3(-20, 1.6f, 34.5f), EN_DRONE);
        flail_swing();
        float hp = e->hp, vz = e->vel.z, dmin = 1e9f;
        for (int i = 0; i < 60; i++) { enemy_tick(TICK); dmin = fminf(dmin, vlen(vsub(e->pos, eye_pos()))); }
        hitstop = 0;
        fprintf(f, "fleau drone a 5.5 m : PV %.0f, traction vel.z %.1f, distance min %.2f (attendu 82, 6.0, < 3.5)\n", hp, vz, dmin);
    }
    {   /* 2. a 20 m/s : portee 7 m (drone a 7,6 m, rayon 0,9), 18 + 1,5 x 20 = 48, dash +1 */
        flail_test_reset(v3(-20, 0, 40));
        P.vel = v3(0, 0, -20); P.dashCharge = 0.5f;
        Enemy *e = flail_test_enemy(0, v3(-20, 1.6f, 32.4f), EN_DRONE);
        flail_swing();
        hitstop = 0;
        fprintf(f, "fleau a 20 m/s, drone a 7.6 m : PV %.0f, dash %.1f, cibles %d (attendu 52, 1.5, 1)\n", e->hp, P.dashCharge, P.flailHitN);
    }
    {   /* 3. arc de 140 degres : drones a 0, +60, -60 et 80 degres a 5 m ; un seul gain de points pour le coup */
        static const float ang[4] = { 0, 1.0472f, -1.0472f, 1.3963f };
        flail_test_reset(v3(-20, 0, 40));
        P.loadout[0] = WP_ROCKET; P.loadout[1] = WP_RAIL; P.weapon = WP_ROCKET; P.ammo[WP_ROCKET] = 4; P.ammoGauge[WP_ROCKET] = 0;
        for (int k = 0; k < 4; k++) flail_test_enemy(k, v3(-20 + 5 * sinf(ang[k]), 1.6f, 40 - 5 * cosf(ang[k])), EN_DRONE);
        flail_swing();
        hitstop = 0;
        int n = 0;
        for (int k = 0; k < 4; k++) n += E[k].hp < 100;
        fprintf(f, "fleau arc : %d drones touches, celui a 80 deg PV %.0f, roquettes %d jauge %.0f (attendu 3, 100, 4, 35)\n",
                n, E[3].hp, P.ammo[WP_ROCKET], P.ammoGauge[WP_ROCKET]);
    }
    {   /* 4. ligne de vue : un muret entre le joueur et un drone a 5 m ; coup dans le vide : 0,9 s depuis l'appui */
        flail_test_reset(v3(-20, 0, 40));
        Enemy *e = flail_test_enemy(0, v3(-20, 1.6f, 35), EN_DRONE);
        int nb0 = nb;
        addbox(-21, 0, 36.5f, -19, 3.5f, 37.5f, 0.5f, 0.5f, 0.5f);
        P.meleeCd = 0;
        botPress[A_MELEE] = 1;                            /* comme un appui : game_tick -> melee -> flail_swing */
        int ticks = 0;
        do { game_tick(TICK); ticks++; } while (P.parryWin > 0 && ticks < 60);
        float hp = e->hp;
        int landed = P.swingLanded;
        nb = nb0;
        memset(E, 0, sizeof E);
        fprintf(f, "fleau derriere un muret : PV drone %.0f, touche %d, recharge %.2f apres %.2f s (attendu 100, 0, 0.80, ~0.1)\n",
                hp, landed, P.meleeCd, ticks * TICK);
    }
    {   /* 5. kamikaze a 5 m : il explose au bout de la chaine, sans soin de parry (seulement le soin du kill, 8) */
        flail_test_reset(v3(-20, 0, 40));
        P.hp = 80;
        Enemy *e = flail_test_enemy(0, v3(-20, 1.6f, 35), EN_RUSHER);
        flail_swing();
        hitstop = 0;
        fprintf(f, "fleau kamikaze : vivant %d, PV joueur %.0f (attendu 0, 88)\n", e->alive, P.hp);
    }
    {   /* 6. piece frappee : ma piece devant, drone a 22 m plus loin -> 45 m/s, 40 degats */
        flail_test_reset(v3(-20, 0, 40));
        Enemy *e = flail_test_enemy(0, v3(-20, 2, 15), EN_DRONE);
        Coin *c = &CO[0];
        c->alive = 1; c->owner = myId; c->life = 3; c->pos = v3(-20, 2.5f, 37); c->vel = v3(0, 0, 0); c->netId = 0x01000077;
        P.dashCharge = 0.5f;                              /* une piece frappee ne rend pas de dash */
        flail_swing();
        int struck = c->flailStruck;
        float sp = vlen(c->vel);
        for (int i = 0; i < 90; i++) coin_tick(TICK);
        hitstop = 0;
        fprintf(f, "fleau piece frappee : frappee %d a %.0f m/s, PV drone %.0f, piece vivante %d, dash %.1f (attendu 1, 45, 60, 0, 0.5)\n",
                struck, sp, e->hp, c->alive, P.dashCharge);
    }
    {   /* 7. piece frappee + rail : le rail dans la piece en vol ricoche (x1,5), ses 40 degats sont remplaces */
        flail_test_reset(v3(-20, 0, 40));
        Enemy *e = flail_test_enemy(0, v3(-20, 2, 15), EN_DRONE);
        Coin *c = &CO[0];
        c->alive = 1; c->owner = myId; c->life = 3; c->pos = v3(-20, 2.5f, 37); c->vel = v3(0, 0, 0); c->netId = 0x01000078;
        flail_swing();
        for (int i = 0; i < 3; i++) coin_tick(TICK);
        V3 d = vsub(c->pos, eye_pos());
        P.yaw = atan2f(d.x, -d.z); P.pitch = asinf(d.y / vlen(d));
        lastChain = 0;
        fire_rail();
        hitstop = 0;
        fprintf(f, "fleau piece frappee + rail : chaine %d, drone vivant %d (attendu 1, 0)\n", lastChain, e->alive);
    }
    {   /* 8. parry a 6 m : orbe a 5,5 m renvoyee a 36 x 0,8 ; l'epee, elle, ne l'atteint pas */
        flail_test_reset(v3(-20, 0, 40));
        Proj *o = spawn_proj(PR_ORB, v3(-20, 1.6f, 34.5f), v3(0, 0, 16));
        flail_swing();
        int okF = o->kind == PR_PARRIED;
        float spF = vlen(o->vel);
        flail_test_reset(v3(-20, 0, 40));
        P.meleeKind = MK_SWORD;
        o = spawn_proj(PR_ORB, v3(-20, 1.6f, 34.5f), v3(0, 0, 16));
        melee();
        hitstop = 0;
        fprintf(f, "fleau parry a 5.5 m : renvoyee %d a %.1f m/s ; epee : renvoyee %d (attendu 1 a 28.8 ; 0)\n", okF, spF, o->kind == PR_PARRIED);
        memset(PR, 0, sizeof PR);
    }
    /* --- Fleau en LAN simule (je suis l'hote) --- */
    {   /* 9. touche : 18 x 0,7 = 12,6 ; recul negatif (traction) de 12 m/s + 3 vers le haut */
        flail_test_reset(v3(-20, 0, 40));
        lance_test_lan(v3(-20, 0, 35));
        flail_swing();
        hitstop = 0;
        const NetEvent *h = flail_test_last_hit();
        V3 kn = h ? fv(h->f + 1) : v3(0, 0, 0);
        fprintf(f, "fleau PvP : PV cible %.1f, arme %d, recul (%.1f, %.1f, %.1f) (attendu 87.4, %d, (0.0, 3.0, 12.0))\n",
                NP[1].hp, h ? h->sub : -1, kn.x, kn.y, kn.z, W_FLAIL);
    }
    {   /* 10. la cible pare : pas de degats, le lanceur est tire vers elle, fleau bloque 1 s ; deux pareurs : une seule traction */
        flail_test_reset(v3(-20, 0, 40));
        lance_test_lan(v3(-20, 0, 35));
        NP[1].lastParryT = nowT;
        int fp = flailHostParried;
        flail_swing();
        hitstop = 0;
        float hp1 = NP[1].hp, vz = P.vel.z, vy = P.vel.y, cd = P.meleeCd;
        fp = flailHostParried - fp;
        flail_test_reset(v3(-20, 0, 40));
        lance_test_lan(v3(-20, 0, 35));
        NP[2].active = NP[2].alive = 1; NP[2].hp = 100; snprintf(NP[2].name, 16, "Cible2");
        NP[2].disp = v3(-21, 0, 35); vf(NP[2].st.pos, NP[2].disp);
        NP[1].lastParryT = NP[2].lastParryT = nowT;
        flail_swing();
        hitstop = 0;
        fprintf(f, "fleau pare : PV cible %.0f, fleaux pares +%d, vel attaquant (y %.1f, z %.1f), recharge %.2f ; deux pareurs : |v| %.1f "
                   "(attendu 100, +1, (4.0, -16.0), 1.00 ; 16.5)\n", hp1, fp, vy, vz, cd, vlen(P.vel));
    }
    {   /* 11. bot au Fleau : cible a 5 m, le coup puis le rail dans les 0,3 s (combo fleau -> tir) */
        flail_test_reset(v3(-20, 0, 40));
        lance_test_lan(v3(-20, 0, 35));
        P.dashCharge = 0.2f;                              /* ni cross-up ni dash */
        P.fireCd = 0.05f;                                 /* le tir part apres le coup de fleau, pas dans le meme tick */
        int h0 = hitCount[W_FLAIL], c0 = flailCombos, i = 0;
        for (; i < 120 && flailCombos == c0; i++) {
            nowT += TICK;
            bot_think(TICK);
            game_tick(TICK);
        }
        hitstop = 0;
        fprintf(f, "bot fleau : touches fleau %d, combos fleau -> rail %d en %.2f s, PV cible %.1f (attendu 1, 1, < 0.35, < 30)\n",
                hitCount[W_FLAIL] - h0, flailCombos - c0, i * TICK, NP[1].hp);
        memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
        reset_game();
        dmMode = 0;
        memset(NP, 0, sizeof NP);
    }
    /* --- fin Fleau --- */

    /* --- Poings (solo) : couloir degage x = -20, joueur en (-20, 0, 40), vue vers -z --- */
    {
        FILE *savedLog = botLog;
        botLog = NULL;                                    /* pas de lignes du journal du bot au milieu des resultats */
        {   /* 1. cadence : appui a chaque tick, drone immobile a 1,5 m. Temps de jeu (hitstop exclu), recharges arrondies au tick */
            fist_test_reset(v3(-20, 0, 40));
            Enemy *e = flail_test_enemy(0, v3(-20, 1.6f, 38.5f), EN_DRONE);
            float tp[4] = { -1, -1, -1, -1 }, hp3 = -1;
            int n = 0, prev = fistPunches;
            gameTime = 0;
            for (int i = 0; i < 400 && gameTime < 1.5f; i++) {
                e->pos = v3(-20, 1.6f, 38.5f); e->vel = v3(0, 0, 0); e->fireT = 10;   /* enemy_tick court-circuite */
                botPress[A_MELEE] = 1;
                float t0 = gameTime;
                game_tick(TICK);
                if (fistPunches != prev) { prev = fistPunches; if (n < 4) tp[n] = t0; if (++n == 3) hp3 = e->hp; }
            }
            hitstop = 0;
            memset(botPress, 0, sizeof botPress);
            fprintf(f, "poings cadence : coups a %.3f %.3f %.3f %.3f s, PV du drone apres 3 coups %.0f "
                       "(attendu 0.000 0.225 0.450 1.158 : 0,22 / 0,44 / 1,14 arrondis au tick ; 52)\n", tp[0], tp[1], tp[2], tp[3], hp3);
            memset(E, 0, sizeof E);
        }
        {   /* 2. pause de 0,5 s apres le coup 1 : le combo revient au coup 1 (14 degats, pas 20) */
            fist_test_reset(v3(-20, 0, 40));
            Enemy *e = flail_test_enemy(0, v3(-20, 1.6f, 38.5f), EN_DRONE);
            fist_test_tick(1);
            for (int i = 0; i < 60; i++) fist_test_tick(0);
            float hp1 = e->hp;
            e->vel = v3(0, 0, 0);                         /* drone immobile : le recul du coup 1 ne compte pas dans v_rel */
            fist_test_tick(1);
            fprintf(f, "poings retour au coup 1 : etape %d, degats du 2e appui %.0f, recharge du combo %.2f (attendu 1, 14, 0.22)\n",
                    P.fistStep, hp1 - e->hp, P.meleeCd);
            memset(E, 0, sizeof E);
        }
        {   /* 3. 3 coups dans le vide : 0,70 + 0,60 - 0,06 = ~1,24 s de recharge a la fin de la fenetre du 3e coup */
            fist_test_reset(v3(-20, 0, 40));
            int i = 0;
            for (; i < 120 && P.fistStep < 3; i++) fist_test_tick(1);
            for (; i < 240 && P.fistStep == 3; i++) fist_test_tick(0);
            fprintf(f, "poings dans le vide : recharge %.2f a la fin du 3e coup, combo %d, punch-jumps %d (attendu 1.24, 0, 0)\n",
                    P.meleeCd, P.fistStep, P.fistPJ ? 0 : 1);
        }
        {   /* 4. mini-saut : pieds a 0,25 m, chute a -12, coup vers le sol -> vy 11 ; 2e coup du meme sejour : rien ; au sol : rendu */
            fist_test_reset(v3(-20, 0.25f, 40));
            P.grounded = 0; P.coyote = 0.5f; P.fistAirT = 0.5f; P.vel = v3(0, -12, 0); P.pitch = -PI / 2;
            fist_punch();
            float vy1 = P.vel.y;
            P.vel.y = -12; P.wallJumpCd = 0; P.meleeCd = 0;
            fist_punch();
            float vy2 = P.vel.y;
            int pj2 = P.fistPJ, land = 0;
            for (int i = 0; i < 360 && !land; i++) { player_tick(TICK, 0, 0); land = P.grounded; }
            fprintf(f, "poings mini-saut : vy %.1f ; 2e coup vy %.1f, charge %d ; apres l'atterrissage (%d) charge %d (attendu 11.0 ; -12.0, 0 ; 1, 1)\n",
                    vy1, vy2, pj2, land, P.fistPJ);
        }
        {   /* 5. super-saut : saut depuis le sol puis coup vers le sol au tick suivant -> refuse (vy 8,4 - 2 ticks de gravite) ;
             *    au sol, coup vers le sol -> rien */
            fist_test_reset(v3(-20, 0, 40));
            P.pitch = -PI / 2;
            player_tick(TICK, 0, 0);
            player_tick(TICK, 1, 0);
            player_tick(TICK, 0, 0);
            fist_punch();
            float vyJ = P.vel.y;
            int chJ = P.fistPJ;
            fist_test_reset(v3(-20, 0, 40));
            P.pitch = -PI / 2;
            player_tick(TICK, 0, 0);
            int g = P.grounded;
            fist_punch();
            fprintf(f, "poings super-saut : apres un saut vy %.1f, charge %d ; au sol (%d) vy %.1f, charge %d (attendu 8.0, 1 ; 1, 0.0, 1)\n",
                    vyJ, chJ, g, P.vel.y, P.fistPJ);
        }
        {   /* 6. mur a 1,5 m de l'oeil, vel (0, 0, -5) vers le mur : composante normale 11 ; wall-jump refuse 0,25 s */
            fist_test_reset(v3(-20, 3, 40));
            int nb0 = nb;
            addbox(-21, 0, 38, -19, 6, 38.5f, 0.5f, 0.5f, 0.5f);
            P.grounded = 0; P.coyote = 1; P.fistAirT = 0.5f; P.vel = v3(0, 0, -5);
            fist_punch();
            float vz = P.vel.z, cd = P.wallJumpCd;
            P.wallTouch = 1; P.wallN = v3(0, 0, 1); P.airJumps = 0;
            player_tick(TICK, 1, 0);                      /* appui de saut contre le mur, tout de suite apres */
            int wj = P.vel.y > 5;
            nb = nb0;
            fprintf(f, "poings mur : vel.z %.1f, recharge partagee %.2f, wall-jump juste apres %d (attendu 11.0, 0.25, 0)\n", vz, cd, wj);
        }
        {   /* 7. plafond horizontal : 25 m/s le long du mur, coup au mur -> |v_h| reste 25, direction tournee ;
             *    20 impulsions sur des normales qui tournent de 70 degres, depuis 5 m/s -> jamais plus de 20 */
            fist_test_reset(v3(-20, 3, 40));
            int nb0 = nb;
            addbox(-21, 0, 38, -19, 6, 38.5f, 0.5f, 0.5f, 0.5f);
            P.grounded = 0; P.fistAirT = 0.5f; P.vel = v3(25, 0, 0);
            fist_punch();
            float h1 = hlen(P.vel), ang = atan2f(P.vel.z, P.vel.x) * 180 / PI;
            nb = nb0;
            P.vel = v3(5, 0, 0);
            float hmax = 0, hfree = 5;
            for (int k = 0; k < 20; k++) {
                float a = k * 1.2217f;
                V3 n = v3(cosf(a), 0, sinf(a));
                P.fistPJ = 1; P.wallJumpCd = 0;
                hfree += FIST_PJ_PUSH;                    /* sans garde-fou : +11 a chaque mur */
                fist_pj_push(n);
                hmax = fmaxf(hmax, hlen(P.vel));
            }
            fprintf(f, "poings plafond horizontal : |v_h| %.2f tournee de %.0f deg ; 20 murs : max %.2f (sans garde-fou jusqu'a %.0f) "
                       "(attendu 25.00, ~24 ; <= 20.00)\n", h1, ang, hmax, hfree);
        }
        {   /* 8. LAN simule (je suis l'hote) : coup 1 a v_rel 0 = 14 x 0,7 = 9,8 ; uppercut : recul vertical 11 ;
             *    combo a v_rel 20 : 30,8 + 30,8 + 49 = 110,6 -> mort */
            fist_test_reset(v3(-20, 0, 40));
            lance_test_lan(v3(-20, 0, 38.5f));
            fist_punch();
            float hp1 = NP[1].hp;
            const NetEvent *h = flail_test_last_hit();
            int w = h ? h->sub : -1;
            P.meleeCd = 0; fist_punch();
            P.meleeCd = 0; fist_punch();
            h = flail_test_last_hit();
            float ky = h ? h->f[2] : 0;
            fist_test_reset(v3(-20, 0, 40));
            lance_test_lan(v3(-20, 0, 38.5f));
            P.vel = v3(0, 0, -20);
            for (int k = 0; k < 3; k++) { P.meleeCd = 0; fist_punch(); }
            hitstop = 0;
            fprintf(f, "poings PvP : PV apres le coup 1 %.1f, arme %d, recul vertical de l'uppercut %.1f ; combo a v_rel 20 : "
                       "cible vivante %d (attendu 90.2, %d, 11.0 ; 0)\n", hp1, w, ky, NP[1].alive, W_FIST);
        }
        {   /* 9. parry de l'hote : victime aux Poings, tolerance 0,05 s (coup d'epee 0,04 s apres sa fenetre : pare ; 0,08 s :
             *    touche ; victime a l'epee a 0,08 s : paree). Attaquant aux Poings pare : combo casse, 1 s, points rendus */
            fist_test_reset(v3(-20, 0, 40));
            lance_test_lan(v3(-20, 0, 38.5f));
            NP[1].st.pad = MK_FIST;
            NP[1].lastParryT = nowT - 0.04;
            net_hit(1, 20, v3(0, 0, -5), W_MELEE);
            float hpA = NP[1].hp;
            NP[1].lastParryT = nowT - 0.08;
            net_hit(1, 20, v3(0, 0, -5), W_MELEE);
            float hpB = NP[1].hp;
            NP[1].hp = 100; NP[1].st.pad = MK_SWORD;
            net_hit(1, 20, v3(0, 0, -5), W_MELEE);
            float hpC = NP[1].hp;
            fist_test_reset(v3(-20, 0, 40));
            lance_test_lan(v3(-20, 0, 38.5f));
            NP[1].lastParryT = nowT;
            int fp = fistHostParried;
            fist_punch();
            hitstop = 0;
            fprintf(f, "poings parry hote : victime aux Poings a +0,04 s PV %.0f, a +0,08 s PV %.0f ; a l'epee a +0,08 s PV %.0f ; "
                       "mon poing pare : combo %d, recharge %.2f, pares +%d, points rendus : jauges %.0f %.0f (attendu 100, 80, 100 ; 0, 1.00, +1, 0 0)\n",
                    hpA, hpB, hpC, P.fistStep, P.meleeCd, fistHostParried - fp, P.ammoGauge[WP_RAIL], P.ammoGauge[WP_ROCKET]);
        }
        {   /* 10. recul recu pendant le combo : 4 m/s (rail) ne casse rien, 10 m/s (roquette) casse le combo (0,3 s) */
            fist_test_reset(v3(-20, 0, 40));
            lance_test_lan(v3(-20, 0, 30));
            P.fistStep = 1; P.meleeCd = 0.1f;
            NetEvent e = ev_new(EV_HIT);
            e.from = 1; e.target = (unsigned char)myId; e.sub = W_RAIL; vf(e.f + 1, v3(4, 0, 0));
            apply_event(&e);
            int s1 = P.fistStep;
            e.sub = W_ROCKET; vf(e.f + 1, v3(10, 0, 0));
            apply_event(&e);
            fprintf(f, "poings recul recu : 4 m/s -> combo %d ; 10 m/s -> combo %d, recharge %.2f (attendu 1 ; 0, 0.30)\n",
                    s1, P.fistStep, P.meleeCd);
        }
        {   /* 11. jauge (roquettes en main, 0 munition, jauge de 50) : combo de 3 touches = 25 + 25 + 0 -> 1 roquette ;
             *     seul l'uppercut touche -> +25 ; combo qui tue au 3e coup = 50 + 100 -> 3 roquettes ; parry d'une orbe pendant
             *     le 3e coup -> +50 hors plafond (2 roquettes) ; 10 s sans rien faire -> rien */
            fist_test_reset(v3(-20, 0, 40));
            dmMode = 0;
            memset(NP, 0, sizeof NP);
            P.loadout[0] = WP_ROCKET; P.loadout[1] = WP_RAIL; P.weapon = WP_ROCKET;
            P.ammo[WP_ROCKET] = 0; P.ammoGauge[WP_ROCKET] = 0; P.ammo[WP_RAIL] = 2;
            Enemy *e = flail_test_enemy(0, v3(-20, 1.6f, 38.5f), EN_DRONE);
            e->hp = 1000;
            for (int k = 0; k < 3; k++) fist_test_punch(e);
            int a1 = P.ammo[WP_ROCKET]; float g1 = P.ammoGauge[WP_ROCKET];
            for (int i = 0; i < 30 && P.fistStep; i++) fist_test_tick(0);
            e->pos = v3(-20, 1.6f, 20);                   /* coups 1 et 2 dans le vide */
            for (int k = 0; k < 2; k++) fist_test_punch(e);
            e->pos = v3(-20, 1.6f, 38.5f);
            fist_test_punch(e);
            int a2 = P.ammo[WP_ROCKET]; float g2 = P.ammoGauge[WP_ROCKET];
            for (int i = 0; i < 30 && P.fistStep; i++) fist_test_tick(0);
            P.ammo[WP_ROCKET] = 0; P.ammoGauge[WP_ROCKET] = 0; e->hp = 48;
            for (int k = 0; k < 3; k++) fist_test_punch(e);
            int a3 = P.ammo[WP_ROCKET], dead = !e->alive;
            for (int i = 0; i < 30 && P.fistStep; i++) fist_test_tick(0);
            P.ammo[WP_ROCKET] = 0; P.ammoGauge[WP_ROCKET] = 0;
            e = flail_test_enemy(0, v3(-20, 1.6f, 38.5f), EN_DRONE);
            e->hp = 1000;
            for (int k = 0; k < 2; k++) fist_test_punch(e);
            Proj *o = spawn_proj(PR_ORB, v3(-20, 1.6f, 37.5f), v3(0, 0, 16));
            fist_test_punch(e);
            int a4 = P.ammo[WP_ROCKET], par = o && o->kind == PR_PARRIED;
            memset(PR, 0, sizeof PR); memset(E, 0, sizeof E);
            float before = P.ammo[WP_ROCKET] * 1000.0f + P.ammoGauge[WP_ROCKET] + P.ammo[WP_RAIL] * 1000000.0f + P.ammoGauge[WP_RAIL];
            for (int i = 0; i < 1200; i++) fist_test_tick(0);
            float after = P.ammo[WP_ROCKET] * 1000.0f + P.ammoGauge[WP_ROCKET] + P.ammo[WP_RAIL] * 1000000.0f + P.ammoGauge[WP_RAIL];
            hitstop = 0;
            fprintf(f, "poings jauge : 3 touches -> %d roquette(s) jauge %.0f ; uppercut seul -> %d jauge %.0f ; kill au 3e coup (%d) -> %d ; "
                       "parry (%d) -> %d ; 10 s sans rien : inchange %d (attendu 1 0 ; 1 25 ; 1, 3 ; 1, 2 ; 1)\n",
                    a1, g1, a2, g2, dead, a3, par, a4, before == after);
        }
        {   /* 12. NetPState.melee : coup 2 a mi-animation -> etape 1, avancement ~32 ; swing_start au debut du coup seulement */
            fist_test_reset(v3(-20, 0, 40));
            NetPState s;
            P.fistAnim = 2; P.meleeT = FIST_ANIM * 0.5f;
            pack_state(&s);
            int stp = s.melee >> 6, adv = s.melee & 63;
            NP[1].st = s;
            int mid = swing_start(1);
            P.meleeT = FIST_ANIM - TICK;
            pack_state(&NP[1].st);
            int start = swing_start(1);
            P.meleeT = 0;
            pack_state(&NP[1].st);
            fprintf(f, "poings NetPState.melee : etape %d avancement %d, debut de coup vu %d, mi-coup vu %d, repos %d (attendu 1 32, 1, 0, 0)\n",
                    stp, adv, start, mid, NP[1].st.melee);
            memset(NP, 0, sizeof NP);
        }
        {   /* 13. bot aux Poings : cible a 1,5 m, il appuie a chaque tick : le combo complet part tout seul (3 touches) */
            fist_test_reset(v3(-20, 0, 40));
            lance_test_lan(v3(-20, 0, 38.5f));
            P.dashCharge = 0.2f;                          /* ni cross-up ni dash */
            int h0 = hitCount[W_FIST], i = 0;
            for (; i < 120 && hitCount[W_FIST] - h0 < 3; i++) {
                nowT += TICK;
                bot_think(TICK);
                game_tick(TICK);
            }
            hitstop = 0;
            fprintf(f, "bot poings : %d touches au poing en %.2f s (attendu 3, ~0.5)\n", hitCount[W_FIST] - h0, i * TICK);
            memset(botHeld, 0, sizeof botHeld); memset(botPress, 0, sizeof botPress);
            reset_game();
            dmMode = 0;
            memset(NP, 0, sizeof NP);
        }
        botLog = savedLog;
    }
    /* --- fin Poings --- */
    /* --- Egide (bouclier) --- */
    {
        double savedNow = nowT;
        {   /* 1. penalites : au sol a 12 m/s, bouclier leve, 1 s en avancant -> 7 m/s ; ni dash ni slide ; au grappin : aucune */
            egide_test_reset();
            P.shieldState = SHS_UP; P.vel = v3(0, 0, -12); act[A_FWD] = 1;
            for (int i = 0; i < 120; i++) player_tick(TICK, 0, 0);
            float h = hlen(P.vel);
            P.dashCharge = 2;
            player_tick(TICK, 0, 1);
            float dc = P.dashCharge, hd = hlen(P.vel);
            act[A_CROUCH] = 1;
            player_tick(TICK, 0, 0);
            int sl = P.sliding;
            act[A_CROUCH] = 0;
            egide_test_reset();
            P.shieldState = SHS_UP; P.vel = v3(0, 0, -12); act[A_FWD] = 1;
            P.hook = HOOK_ATTACHED; P.anchor = P.hookPos = v3(-20, 1.5f, 15); P.ropeLen = 100;
            hookPressIn = 0; hookHeldIn = 1;
            for (int i = 0; i < 30; i++) player_tick(TICK, 0, 0);
            float hh = hlen(P.vel);
            hookHeldIn = 0; act[A_FWD] = 0;
            fprintf(f, "egide penalites : vitesse %.2f, dash %.1f (vitesse %.1f), slide %d ; au grappin %.1f (attendu 7.00, 2.0 (7.0), 0 ; > 7)\n",
                    h, dc, hd, sl, hh);
        }
        {   /* 2. air-strafe 0,4 s : vue qui tourne a 2,2 rad/s (comme le test du bhop), puis strafe optimal a chaque tick */
            float gain[2][2];
            for (int m = 0; m < 2; m++)
                for (int s = 0; s < 2; s++) {
                    egide_test_reset();
                    P.shieldState = s ? SHS_UP : SHS_DOWN;
                    P.pos = P.prevPos = v3(-20, 4, 41); P.grounded = 0; P.vel = v3(0, 0, -10); act[A_RIGHT] = 1;
                    for (int i = 0; i < 48; i++) {
                        if (!m) P.yaw += 2.2f * TICK;
                        else {                            /* souhait a l'angle qui maximise le gain : vdot(vel, wish) = cap - a */
                            float v = hlen(P.vel), a = AIR_ACCEL * (s ? SHIELD_AIR_MULT : 1) * RUN_SPEED * TICK;
                            float c = clampf(fmaxf(0, AIR_CAP - a) / v, -1, 1), sn = sqrtf(1 - c * c);
                            V3 vh = v3(P.vel.x / v, 0, P.vel.z / v), w = v3(vh.x * c - vh.z * sn, 0, vh.z * c + vh.x * sn);
                            P.yaw = atan2f(w.z, w.x);             /* la droite (A_RIGHT) = (cos yaw, 0, sin yaw) */
                        }
                        player_tick(TICK, 0, 0);
                    }
                    gain[m][s] = hlen(P.vel) - 10;
                    act[A_RIGHT] = 0;
                }
            fprintf(f, "egide air-strafe : vue a 2,2 rad/s gain %.2f / %.2f (rapport %.2f) ; optimal %.2f / %.2f (rapport %.2f) "
                       "(attendu rapports < 1 ; le plafond AIR_CAP limite l'effet, voir la fiche, question 4)\n",
                    gain[0][0], gain[0][1], gain[0][1] / fmaxf(gain[0][0], 1e-3f),
                    gain[1][0], gain[1][1], gain[1][1] / fmaxf(gain[1][0], 1e-3f));
        }
        {   /* 3. tireur, cible bouclier leve de face : rail 1 -> jauge 60, rail 2 -> brise, rail 3 -> 60 de degats */
            egide_test_reset();
            egide_test_lan(v3(-20, 0, 31), 1, PI);
            fire_rail(); hitstop = 0;
            float hp1 = NP[1].hp, g1 = NP[1].shieldGauge;
            P.ammo[WP_RAIL] = 2; P.fireCd = 0; shotStreak = 0;
            fire_rail(); hitstop = 0;
            float g2 = NP[1].shieldGauge, b2 = NP[1].shieldBrokenT;
            P.ammo[WP_RAIL] = 2; P.fireCd = 0; shotStreak = 0;
            fire_rail(); hitstop = 0;
            fprintf(f, "egide de face : PV %.0f jauge %.0f ; jauge %.0f brise %.1f s ; PV %.0f, chaine %d (attendu 100 60 ; 0 3.0 ; 40, 1)\n",
                    hp1, g1, g2, b2, NP[1].hp, shotStreak);
        }
        {   /* 4. de flanc (bouclier tourne de 90 degres) : touche */
            egide_test_reset();
            egide_test_lan(v3(-20, 0, 31), 1, PI * 0.5f);
            fire_rail(); hitstop = 0;
            fprintf(f, "egide de flanc : PV %.0f, jauge %.0f (attendu 40, 120)\n", NP[1].hp, NP[1].shieldGauge);
        }
        {   /* 5. le rail s'arrete sur le bouclier : la cible derriere n'est pas touchee */
            egide_test_reset();
            egide_test_lan(v3(-20, 0, 31), 1, PI);
            NP[2].active = NP[2].alive = 1; NP[2].hp = 100; snprintf(NP[2].name, 16, "Derriere");
            NP[2].disp = v3(-20, 0, 24); vf(NP[2].st.pos, NP[2].disp);
            fire_rail(); hitstop = 0;
            fprintf(f, "egide rail arrete : PV porteur %.0f, PV derriere %.0f (attendu 100, 100)\n", NP[1].hp, NP[2].hp);
        }
        {   /* 6. parry parfait (je suis le porteur, levee a l'instant) : le rail de 1 lui revient a 60, jauge intacte */
            egide_test_reset();
            egide_test_lan(v3(-20, 0, 31), 0, 0);
            NP[0].hp = 95;
            shield_tick(TICK, 1, 1);                       /* levee : fenetre parfaite (baisse depuis 1 s) */
            host_tick(TICK);
            NetEvent e = ev_new(EV_HIT);
            e.from = 1; e.target = 0; e.sub = W_RAIL; e.f[0] = 70; e.eid = 0x02000501;
            vf(&e.f[1], v3(0, 0, 4));
            host_event_in(&e);
            hitstop = 0;
            fprintf(f, "egide parfait : PV tireur %.0f, PV porteur %.0f, jauge %.0f, parfaits %d (attendu 40, 100, 120, 1)\n",
                    NP[1].hp, NP[0].hp, NP[0].shieldGauge, shieldPerfects);
        }
        {   /* 7. revendication refusee : bouclier baisse depuis 1 s chez l'hote -> degats complets */
            egide_test_reset();
            egide_test_lan(v3(-20, 0, 31), 0, 0);
            P.shieldState = SHS_UP;
            host_tick(TICK);
            P.shieldState = SHS_DOWN; nowT += 1;
            host_tick(TICK);
            NetEvent e = ev_new(EV_SHIELD);
            e.from = 1; e.target = 0; e.sub = SHE_CLAIM; e.actor = W_RAIL; e.f[0] = 70; e.eid = 0x02000502;
            vf(&e.f[1], v3(0, 0, 1)); vf(&e.f[4], v3(0, 0, 4));
            int den = shieldDenied;
            host_event_in(&e);
            fprintf(f, "egide refus : PV %.0f, refus +%d (attendu 40, +1)\n", NP[0].hp, shieldDenied - den);
        }
        {   /* 8. conversion : EV_HIT du rail, mon bouclier leve de face hors fenetre parfaite -> bloque par l'hote */
            egide_test_reset();
            egide_test_lan(v3(-20, 0, 31), 0, 0);
            P.shieldState = SHS_UP; P.shieldDownT = 0;
            host_tick(TICK);
            NetEvent e = ev_new(EV_HIT);
            e.from = 1; e.target = 0; e.sub = W_RAIL; e.f[0] = 70; e.eid = 0x02000503;
            vf(&e.f[1], v3(0, 0, 4));
            int cv = shieldConverted;
            host_event_in(&e);
            hitstop = 0;
            fprintf(f, "egide conversion : PV %.0f, jauge hote %.0f / locale %.0f, conversions +%d (attendu 100, 60 / 60, +1)\n",
                    NP[0].hp, NP[0].shieldGauge, P.shieldGauge, shieldConverted - cv);
        }
        {   /* 9. souffle a 1 m : de face 50 % bloques ; de dos et sous les pieds rien */
            float loss[3];
            static const float cz[3] = { 32, 30, 31 }, cy[3] = { 0.9f, 0.9f, -0.2f };
            for (int s = 0; s < 3; s++) {
                egide_test_reset();
                egide_test_lan(v3(-20, 0, 31), 1, PI);
                explode(v3(-20, cy[s], cz[s]), ROCKET_RADIUS, ROCKET_DMG, 1);
                hitstop = 0;
                loss[s] = 100 - NP[1].hp;
            }
            fprintf(f, "egide souffle : perte de face %.1f, de dos %.1f, sous les pieds %.1f (attendu ~33, 60, 60)\n", loss[0], loss[1], loss[2]);
        }
        {   /* 10. coup de bouclier (tap) : cible a 1,5 m, v_rel 10 -> (25 + 30) x 0,7 = 38,5 ; dash +1 */
            egide_test_reset();
            egide_test_lan(v3(-20, 0, 39), 0, 0);
            NP[1].st.pad = 0;
            P.vel = v3(0, 0, -10); P.dashCharge = 0.5f;
            shield_tick(TICK, 1, 0);                       /* appui et relache dans le meme tick */
            hitstop = 0;
            fprintf(f, "egide coup de bouclier : PV %.1f, dash %.1f, recharge %.2f (attendu 61.5, 1.5, 0.50)\n",
                    NP[1].hp, P.dashCharge, P.meleeCd);
        }
        lance_test_lan(v3(-20, 0, 31));                    /* fin du LAN simule */
        memset(NP, 0, sizeof NP);
        {   /* 11. regeneration (solo) : jauge vide -> brise 3 s, puis 0 -> 120 en 4 s */
            egide_test_reset();
            dmMode = 0;
            P.shieldState = SHS_UP; P.shieldGauge = 0;
            shield_tick(TICK, 0, 1);
            int st0 = P.shieldState;
            for (int i = 0; i < 360; i++) shield_tick(TICK, 0, 0);
            int st3 = P.shieldState; float g3 = P.shieldGauge;
            for (int i = 0; i < 480; i++) shield_tick(TICK, 0, 0);
            fprintf(f, "egide regeneration : etat %d ; a 3 s etat %d jauge %.1f ; a 7 s jauge %.1f (attendu 2 ; 0, < 0.5 ; 120.0)\n",
                    st0, st3, g3, P.shieldGauge);
        }
        {   /* 12. orbe (solo) : de face sur le bouclier (12 sur la jauge), de dos dans le corps */
            float hp[2], g[2];
            for (int s = 0; s < 2; s++) {
                egide_test_reset();
                dmMode = 0;
                P.shieldState = SHS_UP; P.yaw = s ? PI : 0;
                spawn_proj(PR_ORB, v3(-20, 1.7f, 37), v3(0, 0, 16));
                for (int i = 0; i < 60; i++) proj_tick(TICK);
                hp[s] = P.hp; g[s] = P.shieldGauge;
            }
            fprintf(f, "egide orbe : de face PV %.0f jauge %.0f ; de dos PV %.0f (attendu 100 108 ; 88)\n", hp[0], g[0], hp[1]);
        }
        {   /* 13. arrache par le grappin adverse : levee impossible pendant 1 s */
            egide_test_reset();
            egide_test_lan(v3(-20, 0, 31), 0, 0);
            NetEvent e = ev_new(EV_SHIELD);
            e.from = 1; e.target = 0; e.sub = SHE_RIP; e.eid = 0x02000504;
            host_event_in(&e);
            shield_tick(TICK, 1, 1);
            int up0 = P.shieldState == SHS_UP;
            for (int i = 0; i < 120; i++) shield_tick(TICK, 0, 0);
            shield_tick(TICK, 1, 1);
            fprintf(f, "egide arrache : levee pendant %d, apres 1 s %d, arrache chez l'hote %.2f s (attendu 0, 1, 1.00)\n",
                    up0, P.shieldState == SHS_UP, NP[0].shieldRipT);
        }
        {   /* 14. Tesla de face : une tranche de 0,2 s (2 impulsions, 9,8) bloquee, pas de chaine vers la cible voisine */
            egide_test_reset();
            P.meleeKind = MK_SWORD;
            egide_test_lan(v3(-20, 0, 31), 1, PI);
            NP[2].active = NP[2].alive = 1; NP[2].hp = 100; snprintf(NP[2].name, 16, "Voisin");
            NP[2].disp = v3(-17, 0, 31); vf(NP[2].st.pos, NP[2].disp);
            P.loadout[0] = WP_TESLA; P.loadout[1] = WP_RAIL; P.weapon = WP_TESLA; P.ammo[WP_TESLA] = 60;
            for (int i = 0; i < 25; i++) { NP[1].lastShieldT = nowT; tesla_test_run(1, i < 24); }   /* ST_SHIELD recu a chaque tick */
            fprintf(f, "egide tesla : PV porteur %.0f, jauge %.1f, PV voisin %.0f (attendu 100, 110.2, 100)\n",
                    NP[1].hp, NP[1].shieldGauge, NP[2].hp);
        }
        {   /* 15. belier : bouclier leve au grappin sur un joueur -> coup de bouclier gratuit (W_SHIELD), rebond */
            egide_test_reset();
            egide_test_lan(v3(-20, 0, 33), 0, 0);
            NP[1].st.pad = 0;
            P.shieldState = SHS_UP; P.shieldDownT = 0;
            P.hook = HOOK_ATTACHED; P.hookPlayer = 2; P.anchor = P.hookPos = np_center(1); P.ropeLen = 10;
            hookHeldIn = 1;
            int h0 = hitCount[W_SHIELD], i = 0;
            for (; i < 120 && P.hook == HOOK_ATTACHED; i++) {
                nowT += TICK;
                player_tick(TICK, 0, 0);
                shield_tick(TICK, 0, 1);
            }
            hookHeldIn = 0; hitstop = 0;
            fprintf(f, "egide belier : touches bouclier +%d, PV cible %.0f, grappin %d, vy %.1f (attendu +1, < 70, 0, > 5)\n",
                    hitCount[W_SHIELD] - h0, NP[1].hp, P.hook, P.vel.y);
        }
        lance_test_lan(v3(-20, 0, 31));
        memset(NP, 0, sizeof NP);
        memset(act, 0, sizeof act);
        reset_game();
        dmMode = 0; hitstop = 0;
        nowT = savedNow;
    }
    /* --- fin Egide --- */

    /* reglages : sauvegarde puis rechargement de velocity.ini */
    {
        int savedBot = botMode;
        botMode = 0;                                      /* les bots ne sauvegardent jamais : on simule un joueur */
        snprintf(cfgPath, sizeof cfgPath, "cfg_test.ini");
        binds[A_COIN][1] = K_X2; sens = 0.003f; fovBase = 115; invertY = 1;
        cfg_save();
        memcpy(binds, bindDefault, sizeof binds); sens = 0.0022f; fovBase = 105; invertY = 0;
        cfg_load();
        fprintf(f, "reglages : piece -> %d (attendu %d), sens %.4f, fov %.0f, axe Y inverse %d (attendu 0.0030, 115, 1)%s",
                binds[A_COIN][1], K_X2, sens, fovBase, invertY, "\n");
        remove(cfgPath);
        cfgPath[0] = 0;
        botMode = savedBot;
        memcpy(binds, bindDefault, sizeof binds); sens = 0.0022f; fovBase = 105; invertY = 0;
    }

    /* contre-jeu : je tire dans la piece de l'adversaire, le ricochet le touche lui */
    reset_game();
    dmMode = 1;
    memset(NP, 0, sizeof NP);
    NP[0].active = NP[0].alive = 1; NP[0].hp = 100; snprintf(NP[0].name, 16, "Moi");
    NP[1].active = NP[1].alive = 1; NP[1].hp = 100; snprintf(NP[1].name, 16, "Lanceur");
    NP[1].disp = v3(-20, 0, 24); vf(NP[1].st.pos, NP[1].disp);
    CO[0].alive = 1; CO[0].owner = 1; CO[0].life = 3; CO[0].netId = 0x02000001;
    CO[0].pos = v3(-18, 4, 28);
    P.pos = P.prevPos = v3(-20, 0, 41);
    {
        V3 d = vsub(CO[0].pos, eye_pos());
        P.yaw = atan2f(d.x, -d.z); P.pitch = asinf(d.y / vlen(d));
    }
    fire_rail();
    fprintf(f, "contre-jeu : piece adverse detruite=%d, PV du lanceur=%.0f (attendu 1, %.0f), message \"%s\"\n",
            !CO[0].alive, NP[1].hp, 100 - fminf(RAIL_DMG * PVP_SCALE, PVP_HIT_CAP), popText);

    /* en ligne : analyse STUN, lignes du lobby, adresses */
    {
        static const unsigned char stunOk[32] = { 0x01, 0x01, 0, 12, 0x21, 0x12, 0xA4, 0x42, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                                  0x00, 0x20, 0, 8, 0, 1, 0xF5, 0x23, 0x73, 0x13, 0xA6, 0x41 };
        struct sockaddr_in m, a;
        char t[40] = "?";
        int ok = stun_parse(stunOk, 32, &m);
        if (ok) ep_text(&m, t, sizeof t);
        PkHead ph;
        memset(&ph, 0, sizeof ph);
        ph.magic = NET_MAGIC; ph.type = PK_SNAP;
        fprintf(f, "STUN : reponse -> %s, paquet de jeu pris pour du STUN=%d (attendu 82.1.2.3:54321 et 0)\n", t,
                stun_parse((const unsigned char *)&ph, 20, &m));
        fprintf(f, "adresses : valide=%d, sans port=%d, octet 300=%d (attendu 1, 0, 0)\n", parse_ep("10.0.0.7:27960", &a),
                parse_ep("10.0.0.7", &a), parse_ep("10.0.0.300:5", &a));

        nFound = 0;
        lb_line("room abc123 3 8 1 5.6.7.8:27960 Bob");
        int okRoom = nFound == 1 && found[0].online && found[0].players == 3 && found[0].maxp == 8 && found[0].spec == 1 &&
                     !strcmp(found[0].id, "abc123") && !strcmp(found[0].name, "Bob") && ntohs(found[0].addr.sin_port) == 27960;
        lb_line("room abc123 4 8 1 5.6.7.8:27960 Bob");
        int updated = nFound == 1 && found[0].players == 4;
        lb_line("room zzz999 1 8 0 9.9.9.9:27960 Eve");
        lb_line("gone abc123");
        fprintf(f, "lobby : salon lu=%d, mise a jour sans doublon=%d, apres gone: %d salon(s), reste %s (attendu 1, 1, 1, Eve)\n",
                okRoom, updated, nFound, nFound ? found[0].name : "-");
        nFound = 0;

        netMode = NET_HOST; myId = 0;                     /* spectateurs : des places a part, sans toucher aux joueurs */
        memset(NP, 0, sizeof NP); memset(SP, 0, sizeof SP);
        NP[0].active = 1;
        int accepted = 0;
        for (int k = 0; k < MAXSPEC + 2; k++) {
            PkJoin pj;
            struct sockaddr_in from;
            memset(&pj, 0, sizeof pj);
            memset(&from, 0, sizeof from);
            from.sin_family = AF_INET; from.sin_port = htons((unsigned short)(40000 + k)); from.sin_addr.s_addr = htonl(0x0A000001);
            snprintf(pj.name, sizeof pj.name, "spec%d", k);
            pj.spec = 1;
            host_on_join(&pj, &from);
            accepted = spec_count();
            if (k == 0) host_on_join(&pj, &from);         /* le meme spectateur qui reessaie ne prend pas une 2e place */
        }
        fprintf(f, "spectateurs : %d acceptes sur %d tentatives (max %d), joueurs actifs=%d (attendu %d, %d, %d, 1)\n", accepted,
                MAXSPEC + 2, MAXSPEC, NP[1].active + NP[0].active, MAXSPEC, MAXSPEC + 2, MAXSPEC);
        memset(SP, 0, sizeof SP);
        memset(NP, 0, sizeof NP);
    }

    {                                                     /* visee des bots : tirs cadres selon le niveau, sinon ils frolent */
        int lv0 = botLevel;
        for (int lv = 0; lv < 3; lv++) {
            int hits = 0, nearMiss = 0;
            botLevel = lv;
            for (int k = 0; k < 2000; k++) {
                float ox, oy;
                if (bot_roll_aim(20, &ox, &oy)) hits++;
                else if (fabsf(ox) > 0.45f && fabsf(ox) < 1.5f) nearMiss++;
            }
            fprintf(f, "visee bot niveau %d a 20 m : cadres %.0f%%, rates qui frolent (0,45 a 1,5 m) %.0f%% des rates (attendu ~%.0f%%, 100%%)\n",
                    lv, hits / 20.0f, 100.0f * nearMiss / fmaxf(1, 2000 - hits), botHitP[lv] * 90);
        }
        botLevel = lv0;
    }
    {                                                     /* chute hors de la carte : mort (solo) */
        reset_game();
        dmMode = 0; netMode = NET_OFF;
        P.pos = P.prevPos = v3(ms(70), 30, 0);              /* au-dessus du vide, derriere le mur est */
        int i = 0;
        for (; i < 600 && !P.dead; i++) game_tick(TICK);
        fprintf(f, "chute hors de la carte : mort=%d apres %.1f s, y=%.0f, PV=%.0f (attendu 1, ~2.4, < -25, 0)\n", P.dead, i * TICK, P.pos.y, P.hp);
        reset_game();
        fprintf(f, "chute : apres respawn, plus marque=%d (attendu 0)\n", P.pos.y > FALL_DEATH_Y + 5 ? 0 : 1);
    }
    {                                                     /* skin : voyage dans l'etat du joueur (NetPState), borne a l'affichage */
        int ws = wantSkin;
        NetPState st;
        memset(&st, 0, sizeof st);
        wantSkin = SKN_CHAT;
        pack_state(&st);
        fprintf(f, "skin : envoye=%d (%s), taille NetPState=%d, 5 skins=%d (attendu %d, CHAT, 52, 5)\n", st.skin,
                skinNames[st.skin < SKN_COUNT ? st.skin : 0], (int)sizeof st, SKN_COUNT, SKN_CHAT);
        wantSkin = ws;
    }
    {                                                     /* graphismes/rendu.ini : lecture d'une ligne, cle inconnue, bornes */
        float d0 = gxSunDir[1], f0 = gxFog[1], b0 = gxBloom[1], a0 = gxAoRays;
        char l1[] = "  soleil_direction = 0.1 0.9 -0.2   # commentaire", l2[] = "cle_inconnue = 4", l3[] = "bloom = 0.5 9";
        char l4[] = "occlusion_rayons = 3";
        int k1 = gfx_parse_line(l1), k2 = gfx_parse_line(l2), k3 = gfx_parse_line(l3);
        gfx_parse_line(l4);
        gfx_clamp();
        fprintf(f, "graphismes : soleil y=%.1f, cle inconnue=%d, bloom seuil borne=%.0f, rayons=%d, lue=%d%d (attendu 0.9, 0, 4, 3, 11)\n",
                gxSunDir[1], k2, gxBloom[1], AO_RAYS, k1, k3);
        gxSunDir[0] = 0.55f; gxSunDir[1] = d0; gxSunDir[2] = -0.82f; gxFog[1] = f0; gxBloom[0] = 0.7f; gxBloom[1] = b0; gxAoRays = a0;
    }
    {                                                     /* eclairage precalcule : ombre portee de la tour, soleil */
        float lit[3], sh[3];
        lm_grid_build();
        lm_light(v3(ms(6), 4, -ms(6)), v3(0, 1, 0), 1, lit);    /* plateforme centrale, cote soleil */
        lm_light(v3(-4, 4, 6), v3(0, 1, 0), 1, sh);             /* meme plateforme, derriere la tour */
        fprintf(f, "eclairage : sol au soleil %.2f, a l'ombre de la tour %.2f, soleil visible %.0f / %.0f (attendu > 1.5x, 1 / 0)\n",
                lit[1], sh[1], sun_vis(v3(ms(6), 4.1f, -ms(6))), sun_vis(v3(-4, 4.1f, 6)));
    }
    {                                                     /* equipes 2v2 : arrivee equilibree, pas de tir allie, victoire d'equipe */
        int nm0 = netMode, dm0 = dmMode, id0 = myId;
        netMode = NET_HOST; dmMode = 1; myId = 0; matchOver = 0; teamSize = 2; fragLimit = team_frag_limit(2);
        memset(NP, 0, sizeof NP);
        NP[0].active = NP[0].alive = 1; NP[0].hp = 100; NP[0].team = 0; snprintf(NP[0].name, 16, "Hote");
        for (int k = 0; k < 4; k++) {                     /* 3 places libres, la 4e arrivee est refusee */
            PkJoin pj;
            struct sockaddr_in from;
            memset(&pj, 0, sizeof pj); memset(&from, 0, sizeof from);
            from.sin_family = AF_INET; from.sin_port = htons((unsigned short)(41000 + k)); from.sin_addr.s_addr = htonl(0x0A000002);
            snprintf(pj.name, sizeof pj.name, "J%d", k + 1);
            host_on_join(&pj, &from);
        }
        int act = 0;
        for (int i = 0; i < MAXP; i++) act += NP[i].active;
        fprintf(f, "equipes arrivee : actifs=%d equipes %d%d%d%d, bleus=%d rouges=%d (attendu 4, 0101, 2, 2)\n", act,
                NP[0].team, NP[1].team, NP[2].team, NP[3].team, team_count(0), team_count(1));
        for (int i = 0; i < 4; i++) { NP[i].alive = 1; NP[i].hp = 100; NP[i].disp = v3(-20, 0, 20.0f + 8 * i); }
        NetEvent e;
        memset(&e, 0, sizeof e);
        e.kind = EV_HIT; e.from = 2; e.target = 0; e.sub = W_RAIL; e.f[0] = 50;
        int cancel = host_handle(&e);
        float hpAlly = NP[0].hp;
        e.from = 1;
        host_handle(&e);
        float hpEnemy = NP[0].hp;
        e.from = 0; e.sub = W_SELF; e.f[0] = 10;
        host_handle(&e);
        fprintf(f, "equipes tir allie : annule=%d PV=%.0f ; tir ennemi PV=%.0f ; degats sur soi PV=%.0f (attendu 1, 100, 50, 40)\n",
                cancel, hpAlly, hpEnemy, NP[0].hp);
        fprintf(f, "equipes cibles : allie cible=%d dessine=%d, ennemi cible=%d (attendu 0, 1, 1)\n", np_target(2), np_present(2), np_target(1));
        V3 sb = pick_spawn(2), sr = pick_spawn(3);
        fprintf(f, "equipes spawns : bleu z=%.0f, rouge z=%.0f (attendu < 0, > 0)\n", sb.z, sr.z);
        NP[0].kills = 10; NP[2].kills = 9; NP[1].kills = 12;   /* aucun joueur n'a 20, mais l'equipe bleue y arrive */
        host_kill(2, 1, W_RAIL);
        fprintf(f, "equipes victoire : limite=%d score bleu=%d rouge=%d fin=%d gagnant=%d (attendu 20, 20, 12, 1, 0)\n",
                fragLimit, team_score(0), team_score(1), matchOver, matchWinner);
        PkSnap sn;                                         /* le mode et les equipes voyagent dans le snapshot */
        memset(&sn, 0, sizeof sn);
        sn.teamSize = 3; sn.fragLimit = 25;
        for (int i = 0; i < 4; i++) { sn.p[i].active = 1; sn.p[i].team = (unsigned char)(i & 1 ? 0 : 1); }
        myId = 7;
        apply_snap(&sn);
        fprintf(f, "equipes snapshot : mode=%d equipes %d%d%d%d (attendu 3, 1010)\n", teamSize, NP[0].team, NP[1].team, NP[2].team, NP[3].team);
        memset(NP, 0, sizeof NP);                         /* un depart laisse 3 contre 1 : reequilibre au match suivant */
        teamSize = 2; myId = 0;
        for (int i = 0; i < 4; i++) { NP[i].active = 1; NP[i].team = (unsigned char)(i == 3); }
        host_balance_teams();
        fprintf(f, "equipes reequilibrage : bleus=%d rouges=%d (attendu 2, 2)\n", team_count(0), team_count(1));
        memset(NP, 0, sizeof NP);
        teamSize = 0; matchOver = 0; fragLimit = FRAG_LIMIT;
        netMode = nm0; dmMode = dm0; myId = id0;
    }
    for (int lv = 0; lv <= 4; lv += 4) {                  /* musique : 4 s de kicks (niveau 0 = 172 BPM, niveau 4 = 188 BPM), signal borne */
        int kicks = 0, bad = 0; float peak = 0, prevK = 0;
        music_set(0, lv);
        for (int i = 0; i < 4 * SR - 100; i++) {
            float s = music_sample();
            if (!(fabsf(s) < 8)) bad = 1;
            peak = fmaxf(peak, fabsf(s));
            if (mus.kEnv >= 0.9996f && prevK < 0.9996f) kicks++;
            prevK = mus.kEnv;
        }
        music_set(-1, 0);
        fprintf(f, "musique niveau %d : kicks en 4 s=%d pic=%.2f valeurs invalides=%d (attendu %d, pic entre 0.5 et 3, 0)\n",
                lv, kicks, peak, bad, lv ? 13 : 12);
    }
    {                                                     /* le niveau demande ne s'applique qu'a la mesure suivante (1,4 s a 172 BPM) */
        music_set(0, 0);
        for (int i = 0; i < SR / 2; i++) music_sample();
        music_level(8);
        int before = mus.lvl;
        for (int i = 0; i < 2 * SR; i++) music_sample();
        fprintf(f, "musique changement de niveau : avant la mesure=%d apres=%d tempo=%.0f BPM (attendu 0, 8, 204)\n",
                before, mus.lvl, SR * 60.0f / 4 / mus.sps);
        music_set(-1, 0);
    }
    {                                                     /* la limite de frags voyage dans PkSnap (partie rapide = 7) */
        PkSnap sn;
        memset(&sn, 0, sizeof sn);
        sn.fragLimit = FRAG_LIMIT_QUICK; apply_snap(&sn); int recu = fragLimit;
        sn.fragLimit = 0; apply_snap(&sn);
        fprintf(f, "limite de frags : recue=%d, snapshot sans limite=%d, taille PkSnap=%d (attendu %d, %d)\n",
                recu, fragLimit, (int)sizeof sn, FRAG_LIMIT_QUICK, FRAG_LIMIT);
        memset(NP, 0, sizeof NP);
    }
    fprintf(f, "musique PvP (limite 15) : 0-0=%d 4-2=%d 5-1=%d 9-9=%d 10-3=%d 14-13=%d 14-14=%d (attendu 0 0 3 3 6 6 8)\n",
            music_pvp_level(0, 0, 15), music_pvp_level(4, 2, 15), music_pvp_level(5, 1, 15), music_pvp_level(9, 9, 15),
            music_pvp_level(10, 3, 15), music_pvp_level(14, 13, 15), music_pvp_level(14, 14, 15));
    fprintf(f, "musique PvP (limite 7) : 2-0=%d 3-0=%d 5-2=%d 6-6=%d (attendu 0 3 6 8)\n",
            music_pvp_level(2, 0, 7), music_pvp_level(3, 0, 7), music_pvp_level(5, 2, 7), music_pvp_level(6, 6, 7));

    botLog = NULL;
    netMode = NET_OFF; dmMode = 0;
    fclose(f);
}

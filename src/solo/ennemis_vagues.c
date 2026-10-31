/* VELOCITY - mode solo : ennemis et vagues
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Ennemis et vagues                                                   */
/* ------------------------------------------------------------------ */
static void spawn_enemy(void) {
    for (int i = 0; i < MAXE; i++) {
        if (E[i].alive) continue;
        Enemy *e = &E[i];
        memset(e, 0, sizeof *e);
        e->alive = 1;
        float rusherChance = fminf(0.55f, 0.25f + wave * 0.04f);
        e->type = (wave > 1 && frand() < rusherChance) ? EN_RUSHER : EN_DRONE;
        e->hp = e->type == EN_DRONE ? 100.0f : 50.0f;
        e->fireT = frange(1.5f, 3.0f);
        e->t = frange(0, 10);
        for (int tries = 0; tries < 20; tries++) {
            const MapDef *md = &maps[curMap];
            float ang = frange(0, 2 * PI), rad = frange(md->enemyRMin, md->enemyRMax);
            e->pos = v3(cosf(ang) * rad, frange(md->enemyMinY, md->enemyMaxY), sinf(ang) * rad);
            if (vlen(vsub(e->pos, P.pos)) > 22) break;
        }
        sphere_pushout(&e->pos, &e->vel, enemy_radius(e->type));
        burst(e->pos, enemy_color(e->type), 40, 8, 0.6f, 0.15f);
        return;
    }
}

static void enemy_tick(float dt) {
    V3 pc = pcenter();
    for (int i = 0; i < MAXE; i++) {
        Enemy *e = &E[i];
        if (!e->alive) continue;
        e->t += dt; e->flash -= dt;
        V3 to = vsub(pc, e->pos);
        float dist = vlen(to);
        V3 dir = vmul(to, 1.0f / fmaxf(dist, 0.01f));
        if (e->type == EN_DRONE) {
            float ang = e->t * 0.35f + i * 1.7f;
            V3 target = vadd(pc, v3(cosf(ang) * 13, 4.5f + sinf(e->t * 0.7f + i) * 2, sinf(ang) * 13));
            V3 tv = vsub(target, e->pos);
            V3 want = vmul(vnorm(tv), 8.0f * clampf(vlen(tv) / 4, 0, 1));
            e->vel = vlerp(e->vel, want, fminf(1, dt * 2));
            e->fireT -= dt;
            if (e->fireT <= 0 && !P.dead) {
                if (dist < 70 && los(e->pos, pc)) {
                    spawn_proj(PR_ORB, vadd(e->pos, vmul(dir, 1.0f)), vmul(dir, 16));
                    play(S_ESHOT, clampf(1.2f - dist / 60, 0.15f, 0.8f), frange(0.9f, 1.1f));
                }
                e->fireT = frange(1.6f, 2.6f) - fminf(wave * 0.08f, 0.7f);
            }
        } else {
            float sp = 10.5f + fminf(wave * 0.3f, 3);
            e->vel = vlerp(e->vel, vmul(dir, sp), fminf(1, dt * 1.6f));
            if (!P.dead && dist < 1.4f && P.hookEnemy != i + 1) {
                int blk = shield_local_faces(dir);       /* Egide : contact sur le bouclier, recul x0,5 */
                if (blk) shield_absorb_local(18, dir);
                else damage_player(18);
                P.vel = vadd(P.vel, vmul(dir, blk ? 9 * SHIELD_KNOCK_MULT : 9));
                kill_enemy(i, 0);
                continue;
            }
            if (frand() < 0.4f) add_part(e->pos, vmul(rand_dir(), 0.5f), v3(1, 0.5f, 0.1f), 0.35f, 0.12f, 0);
        }
        for (int j = 0; j < MAXE; j++) {                   /* separation */
            if (j == i || !E[j].alive) continue;
            V3 dv = vsub(e->pos, E[j].pos);
            float d = vlen(dv);
            if (d < 1.8f && d > 1e-4f) e->vel = vadd(e->vel, vmul(dv, (1.8f - d) * 4 * dt / d));
        }
        e->pos = vadd(e->pos, vmul(e->vel, dt));
        sphere_pushout(&e->pos, &e->vel, enemy_radius(e->type));
        float floorY = maps[curMap].enemyFloor;          /* plancher des drones (au-dessus du vide) */
        if (e->pos.y < floorY) { e->pos.y = floorY; if (e->vel.y < 0) e->vel.y = 0; }
    }
}

static int enemies_alive(void) {
    int n = 0;
    for (int i = 0; i < MAXE; i++) n += E[i].alive;
    return n;
}

static void wave_tick(float dt) {
    if (toSpawn > 0) {
        spawnT -= dt;
        if (spawnT <= 0) { spawn_enemy(); toSpawn--; spawnT = 0.35f; }
    } else if (enemies_alive() == 0) {
        waveDelay -= dt;
        if (waveDelay <= 0) {
            wave++;
            toSpawn = 2 + wave * 2;
            if (toSpawn > 40) toSpawn = 40;
            waveDelay = 3; spawnT = 0;
            snprintf(msg, sizeof msg, "VAGUE %d", wave);
            msgT = 2.5f;
            play(S_WAVE, 0.7f, 1.0f);
        }
    }
}

static void reset_game(void) {
    memset(&P, 0, sizeof P);
    P.pos = P.prevPos = maps[curMap].start;
    P.hp = 100; P.dashCharge = 2; P.airJumps = 1; P.eyeH = EYE_STAND; P.fovCur = fovBase;
    loadout_apply();
    anvil_reset();
    P.skillCh = (float)skillDef[P.skill].max;
    memset(CO, 0, sizeof CO);
    memset(markP, 0, sizeof markP); memset(markE, 0, sizeof markE); markedMeT = 0; lastHpSeen = 100;
    for (int i = 0; i < npu; i++) PU[i].t = PU[i].pending = 0;
    memset(E, 0, sizeof E); memset(PR, 0, sizeof PR); memset(PA, 0, sizeof PA); memset(BM, 0, sizeof BM);
    memset(TR, 0, sizeof TR);
    wave = 0; toSpawn = 0; waveDelay = 1.5f; spawnT = 0;
    score = kills = combo = 0; gameTime = 0; hitMarker = comboT = msgT = popT = shake = 0;
    shotStreak = 0; hitstop = parryFlash = streakLostT = 0;
}

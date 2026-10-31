/* VELOCITY - navigation des bots : graphe de la carte (sols, pads, ancres), plus court chemin, suivi
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Le graphe est calcule tout seul a partir des boites de la carte :   */
/*  - noeud SOL : dessus libre d'une boite (une grille de points tous  */
/*    les NAV_GRID m sur les grands sols) ; noeud PAD : dessus d'un    */
/*    jump pad ; noeud ANCRE : petit cube flottant (grappin).          */
/*  - aretes : MARCHE (sol continu), SAUT (trou ou marche franchissable*/
/*    en double saut), PAD (point de chute calcule), GRAPPIN (vers un  */
/*    dessus plus haut ou une ancre, en ligne de vue), LACHER (d'une   */
/*    ancre vers un sol plus bas).                                     */
/* Le bot suit le chemin quand sa cible est cachee ou de l'autre cote  */
/* d'un vide ; en vue directe sur le meme sol, il se bat comme avant.  */
/* ------------------------------------------------------------------ */
#define NAV_MAX      900
#define NAV_EMAX     16        /* aretes sortantes par noeud */
#define NAV_GRID     8.0f      /* pas de la grille sur les grands sols */
#define NAV_WALK     11.0f     /* longueur max d'une arete MARCHE */
#define NAV_JUMP     8.5f      /* trou franchissable en sautant (bhop + double saut) */
#define NAV_JUMP_UP  2.4f      /* marche franchissable en double saut */
#define NAV_HOOK     46.0f     /* portee utile du grappin (un peu sous GRAPPLE_RANGE) */
enum { NN_SURF, NN_PAD, NN_ANCHOR };
enum { NE_WALK, NE_JUMP, NE_PAD, NE_HOOK, NE_DROP };
typedef struct { V3 p, aim; int kind, box; int ne; short to[NAV_EMAX]; unsigned char type[NAV_EMAX]; float cost[NAV_EMAX]; } NavNode;
static NavNode NV[NAV_MAX];
static int nnav, navMap = -1, navNb = -1, navHookOk = -1;
static int navStat[5], navPaths, navArrive;               /* journal : aretes suivies par type, chemins calcules, arrivees */

static int nav_ground_below(V3 p, float maxDrop) { return ray_world(v3(p.x, p.y + 0.3f, p.z), v3(0, -1, 0), maxDrop + 0.3f, 0) < maxDrop + 0.3f; }
static int nav_fat_los(V3 a, V3 b) {                     /* passage assez large pour le corps : 4 rayons decales */
    V3 d = v3(b.x - a.x, 0, b.z - a.z), s = vnorm(v3(-d.z, 0, d.x));
    for (int k = 0; k < 4; k++) {
        V3 o = vadd(vmul(s, k & 1 ? 0.5f : -0.5f), v3(0, k & 2 ? 1.5f : 0.45f, 0));
        if (!los(vadd(a, o), vadd(b, o))) return 0;
    }
    return 1;
}
static int nav_add(V3 p, int kind, int box) {
    if (nnav >= NAV_MAX) return -1;
    NavNode *n = &NV[nnav];
    memset(n, 0, sizeof *n);
    n->p = p; n->aim = p; n->kind = kind; n->box = box;
    return nnav++;
}
static void nav_edge(int a, int b, int type, float cost) {
    NavNode *n = &NV[a];
    if (a == b) return;
    for (int k = 0; k < n->ne; k++) if (n->to[k] == b) { if (cost < n->cost[k]) { n->cost[k] = cost; n->type[k] = (unsigned char)type; } return; }
    int k = n->ne;
    if (k >= NAV_EMAX) {                                  /* plein : on remplace l'arete la plus chere (les grappins lointains) */
        k = 0;
        for (int j = 1; j < NAV_EMAX; j++) if (n->cost[j] > n->cost[k]) k = j;
        if (n->cost[k] <= cost) return;
    } else n->ne++;
    n->to[k] = (short)b; n->type[k] = (unsigned char)type; n->cost[k] = cost;
}
/* point visable du grappin sur la boite b, vu de l'oeil e : bord superieur cote e (on finit par monter dessus) */
static V3 nav_hook_point(V3 e, int b) {
    Box *x = &boxes[b];
    if (e.y > x->mx.y + 2.5f)                             /* bien au-dessus : le dessus, cote e (sinon le crochet rase) */
        return v3(clampf(e.x, x->mn.x + 0.6f, x->mx.x - 0.6f), x->mx.y, clampf(e.z, x->mn.z + 0.6f, x->mx.z - 0.6f));
    /* d'en bas : la face laterale la plus proche, juste sous le bord (le crochet tire sur la paroi, pas dessous) */
    float ox = e.x < x->mn.x ? x->mn.x - e.x : e.x > x->mx.x ? e.x - x->mx.x : 0;
    float oz = e.z < x->mn.z ? x->mn.z - e.z : e.z > x->mx.z ? e.z - x->mx.z : 0;
    V3 q = v3(clampf(e.x, x->mn.x + 0.6f, x->mx.x - 0.6f), x->mx.y - 0.35f, clampf(e.z, x->mn.z + 0.6f, x->mx.z - 0.6f));
    if (ox >= oz && ox > 0) q.x = e.x < x->mn.x ? x->mn.x + 0.02f : x->mx.x - 0.02f;
    else if (oz > 0) q.z = e.z < x->mn.z ? x->mn.z + 0.02f : x->mx.z - 0.02f;
    else q.y = -1000;                                     /* juste dessous : pas de face visible */
    return q;
}
static int nav_ray_ok(V3 a, V3 q) {                       /* le crochet tire de a touche bien q (pas un obstacle avant) */
    if (q.y < -999) return 0;
    V3 d = vsub(q, a);
    float l = vlen(d);
    if (l < 2 || l > NAV_HOOK) return 0;
    return ray_world(a, vmul(d, 1 / l), l + 0.6f, 0) > l - 0.7f;
}

static void nav_build(void) {
    nnav = 0; navMap = curMap; navNb = nb; navHookOk = hook_allowed();
    float ky = maps[curMap].killY > -999 ? maps[curMap].killY : -1000;
    for (int i = 0; i < nb; i++) {
        Box *b = &boxes[i];
        V3 s = vsub(b->mx, b->mn);
        if (b->kind == B_PAD) { nav_add(v3((b->mn.x + b->mx.x) * 0.5f, b->mx.y, (b->mn.z + b->mx.z) * 0.5f), NN_PAD, i); continue; }
        if (s.x < 2 * SWING_HALF + 0.2f && s.y < 2 * SWING_HALF + 0.2f && s.z < 2 * SWING_HALF + 0.2f && b->mn.y > ky + 3) {   /* petit cube : ancre si rien dessous de proche */
            V3 c = vmul(vadd(b->mn, b->mx), 0.5f);
            if (!nav_ground_below(v3(c.x, b->mn.y - 0.1f, c.z), 2.0f)) { int k = nav_add(c, NN_ANCHOR, i); if (k >= 0) NV[k].aim = c; continue; }
        }
        if (s.x < 1.4f || s.z < 1.4f) continue;           /* trop etroit pour s'y poser */
        int gx = s.x > NAV_GRID * 1.5f ? (int)(s.x / NAV_GRID) : 1, gz = s.z > NAV_GRID * 1.5f ? (int)(s.z / NAV_GRID) : 1;
        for (int u = 0; u < gx; u++)
            for (int w = 0; w < gz; w++) {
                V3 p = v3(b->mn.x + s.x * (u + 0.5f) / gx, b->mx.y, b->mn.z + s.z * (w + 0.5f) / gz);
                if (!pos_free(v3(p.x, p.y + 0.02f, p.z), H_STAND)) {   /* couvert (un pad, un muret) : un coin libre de la case */
                    int found = 0;
                    for (int c = 0; c < 4 && !found; c++) {
                        V3 q = v3(p.x + (c & 1 ? 1 : -1) * fminf(s.x / gx * 0.5f - 0.5f, 3), p.y, p.z + (c & 2 ? 1 : -1) * fminf(s.z / gz * 0.5f - 0.5f, 3));
                        if (pos_free(v3(q.x, q.y + 0.02f, q.z), H_STAND)) { p = q; found = 1; }
                    }
                    if (!found) continue;
                }
                if (p.y > 60) continue;
                nav_add(p, NN_SURF, i);
            }
    }
    for (int a = 0; a < nnav; a++) {
        NavNode *A = &NV[a];
        V3 ea = v3(A->p.x, A->p.y + EYE_STAND, A->p.z);
        if (A->kind == NN_ANCHOR) ea = v3(A->p.x, A->p.y - 1.5f, A->p.z);   /* pendu sous l'ancre */
        if (A->kind == NN_PAD) {                          /* pad : point de chute balistique (pas d'air-strafe) */
            V3 v = boxes[A->box].launch, q = A->p;
            for (int t = 0; t < 300; t++) {
                v.y -= GRAVITY / 60; V3 nq = vadd(q, vmul(v, 1.0f / 60));
                if (v.y < 0 && ray_world(v3(nq.x, q.y, nq.z), v3(0, -1, 0), q.y - nq.y + 0.2f, 0) < q.y - nq.y + 0.2f) { q = nq; break; }
                if (!pos_free(nq, 1.0f)) break;
                q = nq;
                if (q.y < ky) break;
            }
            int best = -1; float bd = 6;
            for (int b = 0; b < nnav; b++) {
                if (NV[b].kind != NN_SURF) continue;
                float d = vlen(vsub(NV[b].p, q));
                if (d < bd) { bd = d; best = b; }
            }
            if (best >= 0) nav_edge(a, best, NE_PAD, hlen(vsub(NV[best].p, A->p)) * 0.6f);
        }
        for (int b = 0; b < nnav; b++) {
            if (a == b) continue;
            NavNode *B = &NV[b];
            V3 d = vsub(B->p, A->p);
            float h = hlen(d), dy = d.y, l = vlen(d);
            if (A->kind != NN_ANCHOR && B->kind != NN_ANCHOR) {   /* sol -> sol (et vers un pad) */
                V3 a1 = vadd(A->p, v3(0, 1.0f, 0)), b1 = vadd(B->p, v3(0, 1.0f, 0));
                if (h <= NAV_WALK && fabsf(dy) <= 0.6f && nav_fat_los(A->p, B->p)) {
                    int cont = 1;                         /* sol continu tout le long ? */
                    for (float t = 1.2f; t < h && cont; t += 1.2f) cont = nav_ground_below(vadd(A->p, vmul(d, t / h)), 1.2f);
                    if (cont) { nav_edge(a, b, NE_WALK, h); continue; }
                }
                if (h <= NAV_JUMP + (dy < -1 ? -dy * 0.8f : 0) && dy <= NAV_JUMP_UP && h > 0.5f && los(a1, b1) &&
                    los(vadd(A->p, v3(0, 2.6f, 0)), vadd(B->p, v3(0, 2.6f, 0)))) { nav_edge(a, b, NE_JUMP, h * 1.15f + 1); continue; }
                if (!navHookOk) continue;                     /* pas de grappin : seulement marcher, sauter, pads */
                if (dy > 1.5f && l <= NAV_HOOK && B->kind == NN_SURF) {   /* grappin vers un dessus plus haut */
                    V3 q = nav_hook_point(ea, B->box);
                    if (nav_ray_ok(ea, q)) { nav_edge(a, b, NE_HOOK, l * 1.25f + 3); continue; }
                }
                if (h > NAV_JUMP && h <= 34 && dy > -6 && B->kind == NN_SURF && ky > -999) {   /* grand vide : grappin a plat */
                    V3 q = nav_hook_point(ea, B->box);
                    if (nav_ray_ok(ea, q)) nav_edge(a, b, NE_HOOK, l * 1.3f + 4);
                }
            } else if (B->kind == NN_ANCHOR) {            /* vers une ancre : grappin */
                if (navHookOk && l <= NAV_HOOK && nav_ray_ok(ea, B->p)) nav_edge(a, b, NE_HOOK, l * 1.2f + 2);
            } else {                                      /* ancre -> sol : on lache (et on plane) */
                if (dy < 2 && h < 26 && los(ea, vadd(B->p, v3(0, 1, 0)))) nav_edge(a, b, NE_DROP, h + 2);
            }
        }
    }
}

/* noeud ou se trouve un point (sol sous les pieds) : le plus proche a portee de vue, peu de difference de hauteur */
static int nav_nearest(V3 p, int anchors) {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < nnav; i++) {
        if (!anchors && NV[i].kind == NN_ANCHOR) continue;
        V3 d = vsub(NV[i].p, p);
        float c = hlen(d) + fabsf(d.y) * 3;
        if (c >= bd || !los(vadd(p, v3(0, 1, 0)), vadd(NV[i].p, v3(0, 1, 0)))) continue;
        bd = c; best = i;
    }
    return best;
}

static short navPrev[NAV_MAX];
static unsigned char navPrevT[NAV_MAX];
static int nav_path(int from, int to, int *path, int maxLen) {   /* Dijkstra (n petit) ; renvoie le nombre d'etapes */
    static float dist[NAV_MAX];
    static unsigned char done[NAV_MAX];
    if (from < 0 || to < 0) return 0;
    for (int i = 0; i < nnav; i++) { dist[i] = 1e9f; done[i] = 0; navPrev[i] = -1; }
    dist[from] = 0;
    for (;;) {
        int u = -1;
        for (int i = 0; i < nnav; i++) if (!done[i] && dist[i] < 1e8f && (u < 0 || dist[i] < dist[u])) u = i;
        if (u < 0 || u == to) break;
        done[u] = 1;
        for (int k = 0; k < NV[u].ne; k++) {
            int v = NV[u].to[k];
            float nd = dist[u] + NV[u].cost[k];
            if (nd < dist[v]) { dist[v] = nd; navPrev[v] = (short)u; navPrevT[v] = NV[u].type[k]; }
        }
    }
    if (dist[to] > 1e8f) return 0;
    int n = 0, tmp[NAV_MAX];
    for (int v = to; v != from && n < NAV_MAX; v = navPrev[v]) tmp[n++] = v;
    int m = n < maxLen ? n : maxLen;
    for (int i = 0; i < m; i++) path[i] = tmp[n - 1 - i];
    return m;
}

/* --- suivi du chemin par le bot ---------------------------------------------------------------------------- */
static int navPath[24], navLen, navStep, navFrom, navGoal = -1, navBusy, navLanding;   /* navLanding : grappin lache, on se pose */
static float navReplanT, navEdgeT;
static int nav_edge_type(int a, int b) {
    for (int k = 0; a >= 0 && k < NV[a].ne; k++) if (NV[a].to[k] == b) return NV[a].type[k];
    return NE_WALK;
}
/* calcule (au plus tous les 0,4 s) un chemin vers `goal` ; renvoie 1 si le bot doit le suivre maintenant */
static int nav_plan(V3 goal, int force, float dt) {
    if (navMap != curMap || navNb != nb || navHookOk != hook_allowed()) { nav_build(); navGoal = -1; navLen = 0; }
    if (nnav == 0) return 0;
    navReplanT -= dt;
    int flying = !P.grounded && navLen > 0 && navStep < navLen && navEdgeT < 6;   /* en vol (pad, saut, grappin) : on suit */
    if ((navReplanT <= 0 && !flying) || force) {
        navReplanT = 0.4f;
        int from = P.hook == HOOK_ATTACHED && navLen > 0 && navStep < navLen && NV[navPath[navStep]].kind == NN_ANCHOR ? -1 : nav_nearest(P.pos, 0);
        int to = nav_nearest(goal, 0);
        if (from >= 0 && to >= 0 && (to != navGoal || from != navFrom || navStep >= navLen)) {
            navFrom = from; navGoal = to;
            navLen = nav_path(from, to, navPath, 24); navStep = 0; navEdgeT = 0; navLanding = 0;
            navPaths++;
        }
    }
    return navLen > 0 && navStep < navLen;
}
static void nav_aim(V3 q) {
    V3 d = vsub(q, eye_pos());
    P.yaw = atan2f(d.x, -d.z);
    P.pitch = asinf(clampf(d.y / fmaxf(vlen(d), 0.01f), -1, 1));
}
/* une etape du chemin : ecrit botHeld / botPress et la visee ; renvoie le type d'arete en cours */
static int nav_follow(float dt) {
    navBusy = 0;
    if (navStep >= navLen) return -1;
    int cur = navStep > 0 ? navPath[navStep - 1] : navFrom, nxt = navPath[navStep];
    int type = nav_edge_type(cur, nxt);
    NavNode *N = &NV[nxt];
    V3 to = vsub(N->p, P.pos);
    float h = hlen(to);
    navEdgeT += dt;
    static float slowT;
    slowT = (hlen(P.vel) < 1.5f && P.hook == HOOK_NONE && type != NE_PAD) ? slowT + dt : 0;
    if (navEdgeT > 6 || slowT > 0.8f) {                   /* bloque : cette arete ne marche pas, on l'oublie et on recalcule */
        NavNode *C = cur >= 0 ? &NV[cur] : NULL;
        for (int k = 0; C && k < C->ne; k++)
            if (C->to[k] == nxt) { C->to[k] = C->to[C->ne - 1]; C->type[k] = C->type[C->ne - 1]; C->cost[k] = C->cost[C->ne - 1]; C->ne--; break; }
        slowT = 0; navLen = 0; navReplanT = 0; navGoal = -1;
        return -1;
    }
    switch (type) {
    case NE_WALK: case NE_JUMP: case NE_PAD: case NE_DROP:
        if (P.hook == HOOK_ATTACHED && type != NE_DROP) break;
        if (type == NE_DROP && P.hook == HOOK_ATTACHED) { botHeld[A_HOOK] = 0; botPress[A_JUMP] = 1; }
        if (type == NE_PAD && P.grounded && boxes[P.groundBox].kind != B_PAD) {   /* pas encore lance : on revient sur le pad */
            to = vsub(NV[cur].p, P.pos); h = hlen(to);
            if (h < 0.3f) to = vsub(N->p, P.pos);
            navBusy = 1;
            P.yaw = atan2f(to.x, -to.z); P.pitch = 0;
            botHeld[A_FWD] = 1;
            break;
        }
        if (N->kind == NN_PAD) navBusy = 1;               /* on va vers un pad pres d'un bord : pas de freinage */
        P.yaw = atan2f(to.x, -to.z); P.pitch = 0;
        botHeld[A_FWD] = 1;
        /* bhop seulement une fois lance (sinon, en l'air sans air-strafe, on ne prend aucune vitesse) */
        botHeld[A_JUMP] = (type != NE_WALK && h < 6) || (hlen(P.vel) > RUN_SPEED * 0.9f && h > 4);
        if (!P.grounded && P.hook == HOOK_NONE && h > 4 && type == NE_WALK) {   /* air-strafe, comme le bot de combat */
            float ph = gameTime * 2.6f;
            P.yaw += 0.4f * sinf(ph);
            botHeld[cosf(ph) > 0 ? A_RIGHT : A_LEFT] = 1;
        }
        if (type == NE_JUMP && !P.grounded && P.vel.y < 1 && P.airJumps > 0 && P.pos.y < N->p.y + 0.5f) botPress[A_JUMP] = 1;
        if (type == NE_JUMP && P.grounded && h > 5 && P.dashCharge >= 1 && hlen(P.vel) < 10) botPress[A_DASH] = 1;
        if (type == NE_PAD) navBusy = 1;
        if (h < 1.6f && fabsf(to.y) < 1.5f) { navStep++; navEdgeT = 0; navStat[type]++; navArrive += navStep >= navLen; }
        break;
    case NE_HOOK: {
        navBusy = 1;
        if (navLanding) {                                 /* lache au bord : double saut et air-control vers le dessus */
            P.yaw = atan2f(to.x, -to.z); P.pitch = 0;
            botHeld[A_FWD] = 1;
            if (!P.grounded && P.vel.y < 0.5f && P.airJumps > 0 && P.pos.y < N->p.y + 0.3f) botPress[A_JUMP] = 1;
            if ((P.grounded && fabsf(to.y) < 1.0f) || h < 1.2f) { navLanding = 0; navStep++; navEdgeT = 0; navStat[type]++; }
            else if (P.pos.y < N->p.y - 4 || navEdgeT > 4) { navLanding = 0; navLen = 0; navReplanT = 0; }
            break;
        }
        static V3 hookQ;
        if (P.hook == HOOK_NONE) {                        /* point vise : fixe au lancer (sinon il glisse pendant la montee) */
            hookQ = N->kind == NN_ANCHOR ? N->p : nav_hook_point(eye_pos(), N->box);
            if (hookQ.y < -999) { hookQ = N->p; }
            nav_aim(hookQ);
            botHeld[A_FWD] = P.grounded;
            if (P.hookCd <= 0) { botPress[A_HOOK] = 1; botHeld[A_HOOK] = 1; if (P.grounded) botPress[A_JUMP] = 1; }
        } else {
            V3 q = P.hook == HOOK_ATTACHED ? P.anchor : hookQ;
            botHeld[A_HOOK] = 1;
            nav_aim(q);
            V3 dq = vsub(q, pcenter());
            if (P.hook == HOOK_ATTACHED && vlen(vsub(P.anchor, hookQ)) > 4) { botHeld[A_HOOK] = 0; break; }   /* accroche ailleurs : on lache, puis on retire */
            if (N->kind == NN_ANCHOR) {
                botHeld[A_FWD] = 1;
                if (vlen(dq) < 5 || navEdgeT > 3) { navStep++; navEdgeT = 0; navStat[type]++; }   /* a l'ancre : arete suivante */
            } else if (P.hook == HOOK_ATTACHED &&
                       (hookQ.y >= boxes[N->box].mx.y - 0.05f ? hlen(dq) < 3.0f            /* accroche sur le dessus : tout pres */
                                                              : ((P.pos.y + 0.6f > q.y && hlen(dq) < 4.5f) || vlen(dq) < 2.4f))) {   /* paroi : au bord */
                botHeld[A_HOOK] = 0; botPress[A_JUMP] = 1;
                navLanding = 1; navEdgeT = 0;
            }
        }
        break;
    }
    }
    return type;
}

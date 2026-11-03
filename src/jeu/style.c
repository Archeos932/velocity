/* VELOCITY - jauge de style (D, C, B, A, S) : le mouvement et les beaux kills la remplissent, les coups recus la vident
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Le style monte avec : la vitesse (au-dela de STYLE_SPEED_MIN), le   */
/* wall-run, le grappin a pleine vitesse, et surtout les kills, avec   */
/* des bonus (en l'air, a grande vitesse, pendu au grappin, juste      */
/* apres un wall-run, au contact, par ricochet, enchaines). Il baisse  */
/* tout seul (plus vite en haut de l'echelle) et a chaque coup recu.   */
/* Seulement en CHALLENGE (mode COURSE, style_on) : ni en PvP ni en  */
/* solo par vagues pour l'instant. Effets : musique plus intense en A  */
/* et S, et le rang affiche a droite du viseur.                        */
/* ------------------------------------------------------------------ */
enum { ST_D, ST_C, ST_B, ST_A, ST_S, ST_COUNT };
static const float styleAt[ST_COUNT] = { 0, STYLE_C, STYLE_B, STYLE_A, STYLE_S };   /* seuils de chaque rang */
static const char *styleName[ST_COUNT] = { "D", "C", "B", "A", "S" };
static const char *styleWord[ST_COUNT] = { "DOUCEMENT", "CORRECT", "BIEN", "AUDACIEUX", "SUPERSONIQUE" };
static const float styleCol[ST_COUNT][3] = {
    { 0.55f, 0.6f, 0.7f }, { 0.4f, 0.85f, 1.0f }, { 0.5f, 1.0f, 0.45f }, { 1.0f, 0.75f, 0.25f }, { 1.0f, 0.3f, 0.55f },
};
static const float styleMultTab[ST_COUNT] = { 1.0f, 1.1f, 1.25f, 1.5f, 2.0f };
#define STYLE_FEED 5
static float style, styleRankT, styleFlash, styleWallT, styleLastKillT = -10, styleBest;
static int styleRank, styleChain, styleTopRank;
static float styleHpPrev = 100;
static struct { char txt[24]; float t, pts; } styleFeed[STYLE_FEED];   /* derniers bonus, du plus recent au plus ancien */

static int styleShow;                                     /* capture --shot style : jauge forcee hors course */
static int style_on(void) { return raceOn || styleShow; }   /* jauge active : CHALLENGE seulement */
static int style_rank(void) { int r = 0; while (r + 1 < ST_COUNT && style >= styleAt[r + 1]) r++; return r; }
static float style_mult(void) { return style_on() ? styleMultTab[styleRank] : 1.0f; }
static void style_reset(void) {
    style = 0; styleRank = 0; styleChain = 0; styleFlash = 0; styleHpPrev = P.hp;
    memset(styleFeed, 0, sizeof styleFeed);
}
static void style_add(float pts, const char *why) {
    if (!style_on()) return;
    style = clampf(style + pts, 0, STYLE_MAX);
    if (why && pts > 0) {
        memmove(&styleFeed[1], &styleFeed[0], sizeof styleFeed[0] * (STYLE_FEED - 1));
        snprintf(styleFeed[0].txt, sizeof styleFeed[0].txt, "%s", why);
        styleFeed[0].t = 1.6f; styleFeed[0].pts = pts;
    }
    int r = style_rank();
    if (r > styleRank) {                                  /* on monte d'un rang : eclat et petit son */
        styleFlash = 1; styleRankT = 0;
        play(S_WAVE, 0.35f, 1.2f + 0.15f * r);
        if (r > styleTopRank) styleTopRank = r;
    }
    styleRank = r;
}
/* un kill a moi : w = arme (W_*), en LAN comme en solo */
static void style_kill(int w) {
    if (!style_on()) return;                              /* hors CHALLENGE : pas meme le chrono des enchainements */
    float base = STYLE_KILL;
    style_add(base, "KILL");
    if (!P.grounded && P.hook == HOOK_NONE) style_add(STYLE_BONUS, "AERIEN");
    if (hlen(P.vel) >= STYLE_FAST) style_add(STYLE_BONUS, "PLEINE VITESSE");
    if (P.hook == HOOK_ATTACHED) style_add(STYLE_BONUS, "TARZAN");
    if (styleWallT > 0) style_add(STYLE_BONUS * 0.7f, "SUR LE MUR");
    if (w == W_COIN) style_add(STYLE_BONUS, "RICOCHET");
    if (w == W_MELEE || (w >= W_SLAM && w <= W_SHIELD) || w == W_HOOK) style_add(STYLE_BONUS * 0.7f, "AU CONTACT");
    if (gameTime - styleLastKillT < STYLE_CHAIN_T) {
        styleChain++;
        style_add(STYLE_BONUS * styleChain, styleChain == 1 ? "DOUBLE" : styleChain == 2 ? "TRIPLE" : "CARNAGE");
    } else styleChain = 0;
    styleLastKillT = gameTime;
}
static void style_parry(void) { style_add(STYLE_PARRY, "PARRY"); }

static void style_tick(float dt) {
    if (P.dead || !style_on()) { if (style > 0 || styleFeed[0].t > 0) style_reset(); return; }
    float sp = hlen(P.vel);
    if (P.wallrun) styleWallT = 1.0f; else styleWallT -= dt;
    float gain = 0;
    if (sp > STYLE_SPEED_MIN) gain += (sp - STYLE_SPEED_MIN) * STYLE_SPEED_K;   /* la vitesse */
    if (P.wallrun) gain += STYLE_WALL_K;
    if (P.hook == HOOK_ATTACHED && sp > 15) gain += STYLE_WALL_K;
    style_add(gain * dt, NULL);
    if (P.hp < styleHpPrev - 0.5f) {                      /* coup recu : le style chute */
        style_add(-(styleHpPrev - P.hp) * STYLE_HURT_K, NULL);
        styleFlash = -1;
    }
    styleHpPrev = P.hp;
    float decay = STYLE_DECAY + STYLE_DECAY_RANK * styleRank;
    if (sp < STYLE_SPEED_MIN * 0.6f) decay *= 1.6f;      /* a l'arret, ca retombe plus vite */
    style = fmaxf(0, style - decay * dt);
    styleRank = style_rank();
    styleBest = fmaxf(styleBest, style);
    styleRankT += dt;
    if (styleFlash > 0) styleFlash = fmaxf(0, styleFlash - dt * 2.5f);
    if (styleFlash < 0) styleFlash = fminf(0, styleFlash + dt * 3);
    for (int i = 0; i < STYLE_FEED; i++) if (styleFeed[i].t > 0) styleFeed[i].t -= dt;
}

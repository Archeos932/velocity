/* VELOCITY - horloge de match commune : heure du serveur, calage des joueurs
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* Le chrono du match suit le temps reel, plus les ticks de simulation : sous 10 images/s (dt plafonne a 0,1 s)
   ou pendant le hitstop, la simulation ralentit mais pas le chrono. L'hote en fait foi (matchT, temps reel ecoule) ;
   chaque instantane porte matchT et l'heure de l'hote a l'envoi (clk), le client y ajoute le temps du trajet.
   Heure de reference : celle du serveur du lobby (worker, "time" -> "time <ms>"), sur laquelle chacun se cale
   (l'aller-retour le plus court donne la mesure la plus juste). Sans lobby (LAN, ancien worker), le client se cale
   sur l'horloge de l'hote (le trajet le plus court). */
#define CLK_SAMPLES 6          /* mesures rapprochees a la connexion au lobby (0,5 s d'ecart) ... */
#define CLK_EVERY   24.0       /* ... puis une toutes les 24 s : remplace le "ping" de maintien (25 s), meme cout */
#define CLK_LOAD    1.0        /* au-dela (s) entre deux ticks : chargement de carte, pas du jeu */

static double clkSrvOff, clkSrvRtt;                       /* heure serveur - nowT (s), aller-retour de la mesure gardee */
static int clkSrvOk, clkSamples;
static double clkAskT = -1, clkLastAskT = -100;           /* "time" en attente de reponse (envoi), dernier envoi */
static double clkHostOff;                                 /* client : heure de l'hote - nowT, au plus juste */
static int clkHostOk, clkHostSrv;                         /* client : calage fait, l'hote est cale sur le serveur */
static double matchClkT;                                  /* hote : nowT du dernier tick du chrono */

static double clk_now(void) { return nowT + (clkSrvOk ? clkSrvOff : 0); }   /* heure que l'hote publie (clk) */

/* reponse du serveur : demande envoyee a sentT, recue a recvT (nowT), heure du serveur srvMs (ms) */
static void clk_srv_sample(double sentT, double recvT, double srvMs) {
    double rtt = recvT - sentT;
    clkSamples++;
    if (rtt < 0 || rtt > 5 || srvMs <= 0) return;
    if (clkSrvOk) clkSrvRtt += 0.002;                     /* la mesure gardee vieillit : une autre finit par la remplacer */
    if (!clkSrvOk || rtt <= clkSrvRtt) { clkSrvOff = srvMs / 1000.0 - (sentT + recvT) / 2; clkSrvRtt = rtt; clkSrvOk = 1; }
}

/* lobby connecte : faut-il demander l'heure maintenant ? */
static int clk_want_ask(void) {
    if (clkAskT >= 0 && nowT - clkAskT < 3) return 0;     /* reponse attendue */
    return nowT - clkLastAskT >= (clkSamples < CLK_SAMPLES ? 0.5 : CLK_EVERY);
}

/* hote : nouveau match, le chrono repart de 0 */
static void match_clock_start(void) { matchT = 0; matchClkT = nowT; }

/* hote : le chrono avance du temps reel ecoule depuis le dernier tick (un seul tick par image le recoit) */
static void match_clock_tick(void) {
    double el = nowT - matchClkT;
    matchClkT = nowT;
    matchT += (float)(el < 0 ? 0 : el > CLK_LOAD ? CLK_LOAD : el);
}

/* client : instantane de l'hote (matchT, heure clk a l'envoi, hote cale sur le serveur ou non) -> chrono local */
static float clk_match_from_snap(float hostMatchT, double hostClk, int hostSrv, int over) {
    double sample = hostClk - nowT;                       /* = decalage des horloges - trajet : le plus grand est le plus juste */
    if (!clkHostOk || hostSrv != clkHostSrv || fabs(sample - clkHostOff) > 2) { clkHostOff = sample; clkHostSrv = hostSrv; clkHostOk = 1; }
    else if (sample > clkHostOff) clkHostOff = sample;
    if (over) return hostMatchT;                          /* match fini : chrono fige */
    double mine = clkSrvOk && hostSrv ? nowT + clkSrvOff : nowT + clkHostOff;   /* les deux cales sur le serveur : son heure */
    double trip = mine - hostClk;
    return hostMatchT + (float)(trip < 0 ? 0 : trip > 1 ? 1 : trip);
}

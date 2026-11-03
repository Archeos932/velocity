/* VELOCITY - parties en ligne : STUN, lobby WebSocket
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Parties en ligne : lobby (WebSocket WinHTTP, dans un fil) + STUN     */
/*                                                                     */
/* Le lobby (worker/) tient la liste des salons et met hote et client  */
/* en relation ; le jeu, lui, reste en UDP direct entre joueurs. Les   */
/* adresses publiques viennent d'une requete STUN faite depuis le      */
/* socket du jeu (meme port => meme mapping NAT). Voir docs/PLAN_EN_LIGNE.md */
/* ------------------------------------------------------------------ */
#define LOBBY_DEFAULT "velocity-lobby.velocity-void.workers.dev"   /* lobby deploye ; lobby= dans velocity.ini le remplace */
#define STUN_HOST     "stun.l.google.com"
#define STUN_PORT     19302
#define LB_Q          48
#define LB_LINE       256

enum { LB_OFF, LB_CONNECTING, LB_ON, LB_FAILED };
static char lobbyAddr[96] = LOBBY_DEFAULT;                /* "hote" (wss, 443) ou "hote:port" (ws si le port n'est pas 443) */
static char lobbyIni[96];                                 /* lobby= de velocity.ini ("" = defaut) : --lobby ne change que lobbyAddr */
static int onlineOn = 1;                                  /* chercher et publier les parties en ligne */
static int lanEp;                                         /* --lanep : adresses locales a la place de STUN (tests sur une machine) */
static int lbWantHost;                                    /* le lobby n'a trouve aucune partie : do_action() en heberge une */

static volatile LONG lbState, lbStop;
static CRITICAL_SECTION lbCs;
static int lbCsReady;
static char lbIn[LB_Q][LB_LINE], lbOut[LB_Q][LB_LINE];
static int lbInN, lbOutN;
static HANDLE lbEvt, lbReaderTh, lbWriterTh;
#ifdef _WIN32
static HINTERNET lbSes, lbCon, lbWs;
#define LB_WS_SEND(s, n) (WinHttpWebSocketSend(lbWs, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (PVOID)(s), (DWORD)(n)) != NO_ERROR)
#else
static MacWs *lbWs;
#define LB_WS_SEND(s, n) (mac_ws_send(lbWs, (s), (int)(n)) != 0)
#endif
static struct sockaddr_in stunAddr, myPub;
static volatile LONG stunReady;
static int myPubOk, lbGreeted, lbLastPlayers = -1, lbLastSpec = -1;
static double lbFailT, stunT, lbHostT, reqT, reqSentT;
static char reqKind, reqId[8];                            /* demande en attente de STUN : 'j' = rejoindre reqId, 'q' = partie rapide */
static int reqSpec;
typedef struct { struct sockaddr_in addr; float left, acc; } Punch;
static Punch punchQ[4];                                   /* clients a qui l'hote ouvre son NAT */

static int parse_ep(const char *s, struct sockaddr_in *a) {
    int p[5];
    if (sscanf(s, "%d.%d.%d.%d:%d", &p[0], &p[1], &p[2], &p[3], &p[4]) != 5) return 0;
    for (int i = 0; i < 4; i++) if (p[i] < 0 || p[i] > 255) return 0;
    if (p[4] <= 0 || p[4] > 65535) return 0;
    memset(a, 0, sizeof *a);
    a->sin_family = AF_INET; a->sin_port = htons((unsigned short)p[4]);
    a->sin_addr.s_addr = htonl(((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) | ((unsigned)p[2] << 8) | (unsigned)p[3]);
    return 1;
}

static void ep_text(const struct sockaddr_in *a, char *out, int n) {
    snprintf(out, n, "%s:%d", inet_ntoa(a->sin_addr), ntohs(a->sin_port));
}

/* --- STUN (RFC 5389) : "quelle est mon adresse vue d'Internet ?" ------- */
static void stun_send(void) {
    if (sock == INVALID_SOCKET || !stunReady) return;
    unsigned char b[20] = { 0, 1, 0, 0, 0x21, 0x12, 0xA4, 0x42 };
    for (int i = 8; i < 20; i++) b[i] = (unsigned char)(rand() & 255);
    sendto(sock, (const char *)b, 20, 0, (const struct sockaddr *)&stunAddr, sizeof stunAddr);
}

/* reponse "Binding Success" -> adresse publique (XOR-MAPPED-ADDRESS, ou MAPPED-ADDRESS) */
static int stun_parse(const unsigned char *b, int n, struct sockaddr_in *out) {
    if (n < 20 || b[0] != 0x01 || b[1] != 0x01 || b[4] != 0x21 || b[5] != 0x12 || b[6] != 0xA4 || b[7] != 0x42) return 0;
    int len = (b[2] << 8) | b[3];
    if (20 + len > n) len = n - 20;
    for (int p = 20; p + 4 <= 20 + len;) {
        int t = (b[p] << 8) | b[p + 1], l = (b[p + 2] << 8) | b[p + 3];
        if (p + 4 + l > 20 + len) break;
        const unsigned char *v = b + p + 4;
        if ((t == 0x0020 || t == 0x0001) && l >= 8 && v[1] == 1) {
            unsigned short port = (unsigned short)((v[2] << 8) | v[3]);
            unsigned char ip[4] = { v[4], v[5], v[6], v[7] };
            if (t == 0x0020) { port ^= 0x2112; ip[0] ^= 0x21; ip[1] ^= 0x12; ip[2] ^= 0xA4; ip[3] ^= 0x42; }
            memset(out, 0, sizeof *out);
            out->sin_family = AF_INET; out->sin_port = htons(port); memcpy(&out->sin_addr, ip, 4);
            return 1;
        }
        p += 4 + ((l + 3) & ~3);
    }
    return 0;
}

static void stun_got(const struct sockaddr_in *a) {
    char t[40];
    if (myPubOk && same_addr(&myPub, a)) return;
    myPub = *a; myPubOk = 1;
    lbLastPlayers = -1;                                   /* l'hote republie son salon avec la nouvelle adresse */
    ep_text(a, t, sizeof t);
    netlog("adresse publique (STUN) : %s", t);
}

/* adresse LAN de cette machine (route par defaut) : pour deux joueurs derriere la meme box */
static void lan_ep_text(char *out, int n, int port) {
    snprintf(out, n, "127.0.0.1:%d", port);
    if (lanEp || !wsa_init()) return;
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return;
    struct sockaddr_in to, me;
    sock_len_t ml = sizeof me;
    memset(&to, 0, sizeof to);
    to.sin_family = AF_INET; to.sin_port = htons(53); to.sin_addr.s_addr = inet_addr("8.8.8.8");
    if (connect(s, (struct sockaddr *)&to, sizeof to) == 0 && getsockname(s, (struct sockaddr *)&me, &ml) == 0 &&
        me.sin_addr.s_addr != 0)                          /* connect() sur UDP n'envoie rien : il choisit juste la route */
        snprintf(out, n, "%s:%d", inet_ntoa(me.sin_addr), port);
    closesocket(s);
}

/* --- fil WinHTTP : une connexion WebSocket texte vers le lobby ---------- */
static DWORD WINAPI lb_writer(LPVOID arg) {
    (void)arg;
    while (!lbStop && lbState == LB_ON) {
        DWORD w = WaitForSingleObject(lbEvt, 25000);
        if (lbStop || lbState != LB_ON) break;
        for (;;) {
            char line[LB_LINE];
            EnterCriticalSection(&lbCs);
            int have = lbOutN > 0;
            if (have) {
                memcpy(line, lbOut[0], LB_LINE);
                memmove(lbOut[0], lbOut[1], (size_t)(lbOutN - 1) * LB_LINE);
                lbOutN--;
            }
            LeaveCriticalSection(&lbCs);
            if (!have) break;
            if (LB_WS_SEND(line, strlen(line))) {
                lbState = LB_FAILED;
                return 0;
            }
        }
        if (w == WAIT_TIMEOUT && LB_WS_SEND("ping", 4)) {
            lbState = LB_FAILED;
            return 0;
        }
    }
    return 0;
}

static DWORD WINAPI lb_reader(LPVOID arg) {
    (void)arg;
    char addr[96];
    snprintf(addr, sizeof addr, "%s", lobbyAddr);
    int port = 443;
    char *colon = strchr(addr, ':');
    if (colon) { *colon = 0; port = atoi(colon + 1); }
#ifdef _WIN32
    wchar_t whost[96];
    MultiByteToWideChar(CP_UTF8, 0, addr, -1, whost, 96);
#endif

    if (!lanEp && !stunReady) {                           /* resolu ici pour ne jamais bloquer le jeu sur le DNS */
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
        if (getaddrinfo(STUN_HOST, NULL, &hints, &res) == 0) {
            stunAddr = *(struct sockaddr_in *)res->ai_addr;
            stunAddr.sin_port = htons(STUN_PORT);
            stunReady = 1;
            freeaddrinfo(res);
        }
    }

#ifndef _WIN32
    MacWs *ws = mac_ws_open(addr, port, port == 443);
    EnterCriticalSection(&lbCs);
    int stopped = lbStop;
    if (!stopped && ws) lbWs = ws;
    LeaveCriticalSection(&lbCs);
    if (stopped || !ws) {
        if (ws) { mac_ws_close(ws); mac_ws_free(ws); }
        if (!stopped) lbState = LB_FAILED;
        return 0;
    }
    lbState = LB_ON;
    lbWriterTh = CreateThread(NULL, 0, lb_writer, NULL, 0, NULL);
    for (;;) {
        char buf[LB_LINE];
        int got = mac_ws_recv(ws, buf, sizeof buf);
        if (got < 0) break;
        if (got == 0) continue;                           /* ligne trop longue : jetee */
        EnterCriticalSection(&lbCs);
        if (lbInN < LB_Q) memcpy(lbIn[lbInN++], buf, (size_t)got + 1);
        LeaveCriticalSection(&lbCs);
    }
    if (!lbStop) lbState = LB_FAILED;
    SetEvent(lbEvt);                                      /* reveille le fil d'ecriture pour qu'il se termine */
    return 0;
#else
    HINTERNET ses = WinHttpOpen(L"VELOCITY", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET con = NULL, req = NULL, ws = NULL;
    if (ses) {
        WinHttpSetTimeouts(ses, 5000, 5000, 5000, 60000);   /* le serveur repond a nos "ping" : 60 s sans rien = connexion morte */
        con = WinHttpConnect(ses, whost, (INTERNET_PORT)port, 0);
    }
    if (con) req = WinHttpOpenRequest(con, L"GET", L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                      port == 443 ? WINHTTP_FLAG_SECURE : 0);
    if (req && WinHttpSetOption(req, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0) &&
        WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(req, NULL))
        ws = WinHttpWebSocketCompleteUpgrade(req, 0);
    if (req) WinHttpCloseHandle(req);

    EnterCriticalSection(&lbCs);
    int stopped = lbStop;
    if (!stopped) { lbSes = ses; lbCon = con; lbWs = ws; }
    LeaveCriticalSection(&lbCs);
    if (stopped || !ws) {
        if (ws) WinHttpCloseHandle(ws);
        if (con) WinHttpCloseHandle(con);
        if (ses) WinHttpCloseHandle(ses);
        if (!stopped) lbState = LB_FAILED;
        return 0;
    }

    lbState = LB_ON;
    lbWriterTh = CreateThread(NULL, 0, lb_writer, NULL, 0, NULL);
    char buf[LB_LINE];
    int len = 0;
    for (;;) {
        DWORD got = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bt;
        if (WinHttpWebSocketReceive(ws, buf + len, (DWORD)(sizeof buf - 1 - len), &got, &bt) != NO_ERROR) break;
        if (bt == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) break;
        len += (int)got;
        if (bt == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
            buf[len] = 0;
            EnterCriticalSection(&lbCs);
            if (lbInN < LB_Q) memcpy(lbIn[lbInN++], buf, (size_t)len + 1);
            LeaveCriticalSection(&lbCs);
            len = 0;
        } else if (len >= (int)sizeof buf - 8) len = 0;   /* ligne beaucoup trop longue : on jette */
    }
    if (!lbStop) lbState = LB_FAILED;
    SetEvent(lbEvt);                                      /* reveille le fil d'ecriture pour qu'il se termine */
    return 0;
#endif
}

static void lb_stop(void) {
    if (!lbCsReady) return;
    lbStop = 1;
    SetEvent(lbEvt);
#ifdef _WIN32
    EnterCriticalSection(&lbCs);
    HINTERNET ws = lbWs, con = lbCon, ses = lbSes;
    lbWs = lbCon = lbSes = NULL;
    LeaveCriticalSection(&lbCs);
    if (ws) WinHttpCloseHandle(ws);                       /* fait echouer le WinHttpWebSocketReceive en cours */
    if (con) WinHttpCloseHandle(con);
    if (ses) WinHttpCloseHandle(ses);
#else
    EnterCriticalSection(&lbCs);
    MacWs *ws = lbWs;
    LeaveCriticalSection(&lbCs);
    if (ws) mac_ws_close(ws);                             /* fait echouer le mac_ws_recv en cours */
#endif
    if (lbReaderTh) { WaitForSingleObject(lbReaderTh, 1500); CloseHandle(lbReaderTh); lbReaderTh = NULL; }
    if (lbWriterTh) { WaitForSingleObject(lbWriterTh, 1500); CloseHandle(lbWriterTh); lbWriterTh = NULL; }
#ifndef _WIN32
    if (ws) { mac_ws_free(ws); lbWs = NULL; }             /* les fils n'y touchent plus */
#endif
    lbState = LB_OFF; lbStop = 0; lbInN = lbOutN = 0; lbGreeted = 0;
}

static void lb_start(void) {
    if (lbState == LB_CONNECTING || lbState == LB_ON) return;
    if (!lbCsReady) {
        InitializeCriticalSection(&lbCs);
        lbEvt = CreateEventA(NULL, FALSE, FALSE, NULL);
        lbCsReady = 1;
    }
    if (lbReaderTh || lbWriterTh) lb_stop();
    wsa_init();
    lbStop = 0; lbInN = lbOutN = 0;
    lbState = LB_CONNECTING;
    lbReaderTh = CreateThread(NULL, 0, lb_reader, NULL, 0, NULL);
    if (!lbReaderTh) lbState = LB_FAILED;
    netlog("lobby : connexion a %s", lobbyAddr);
}

static void lb_send(const char *fmt, ...) {
    char b[LB_LINE];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    if (!lbCsReady || lbState != LB_ON) return;
    EnterCriticalSection(&lbCs);
    if (lbOutN < LB_Q) memcpy(lbOut[lbOutN++], b, strlen(b) + 1);
    LeaveCriticalSection(&lbCs);
    SetEvent(lbEvt);
    netlog("lobby >> %s", b);
}

static void online_reset(void) {
    lb_stop();
    myPubOk = 0; reqKind = 0; reqSentT = -100;
    lbLastPlayers = lbLastSpec = -1; stunT = 0; lbHostT = 0;
    memset(punchQ, 0, sizeof punchQ);
}

static const char *lb_status_text(void) {
    if (!onlineOn) return "parties en ligne desactivees (PARAMETRES)";
    switch (lbState) {
    case LB_CONNECTING: return "connexion au lobby en ligne ...";
    case LB_ON: return myPubOk ? "en ligne" : "en ligne - recherche de ton adresse publique (STUN) ...";
    case LB_FAILED: return "lobby injoignable - nouvel essai ...";
    }
    return "";
}

static void lb_safe_name(char *out, int n) {                 /* le lobby n'accepte que [A-Za-z0-9_.-] */
    int k = 0;
    for (const char *s = myName; *s && k < n - 1; s++)
        out[k++] = (isalnum((unsigned char)*s) || *s == '.' || *s == '-') ? *s : '_';
    out[k] = 0;
    if (!k) snprintf(out, n, "joueur");
}

static void lb_room(const char *id, int players, int maxp, int spec, const char *ep, const char *name, int rk, int mmr) {
    struct sockaddr_in a;
    if (!parse_ep(ep, &a)) return;
    int k = -1;
    for (int i = 0; i < nFound; i++) if (found[i].online && !strcmp(found[i].id, id)) k = i;
    if (k < 0) {
        if (nFound >= MAXFOUND) return;
        k = nFound++;
        memset(&found[k], 0, sizeof found[k]);
    }
    snprintf(found[k].id, sizeof found[k].id, "%s", id);
    snprintf(found[k].name, sizeof found[k].name, "%s", name);
    found[k].addr = a; found[k].players = players; found[k].maxp = maxp; found[k].spec = spec;
    found[k].ranked = rk; found[k].mmr = mmr;             /* mode classe */
    found[k].online = 1; found[k].seen = nowT; found[k].map = -1;
}

static void lb_line(const char *s) {
    char a[64], b[64], c[64];
    int n1, n2, n3, n4 = 0, n5 = 0;
    double ms;
    if (sscanf(s, "time %lf", &ms) == 1) {               /* heure du serveur : calage de l'horloge de match */
        int was = clkSrvOk;
        if (clkAskT >= 0) clk_srv_sample(clkAskT, nowT, ms);
        if (clkSrvOk && !was) netlog("horloge : calee sur le serveur (aller-retour %.0f ms)", clkSrvRtt * 1000);
        clkAskT = -1;
        return;
    }
    if (!strcmp(s, "err cmd") && clkAskT >= 0) { clkAskT = -1; clkSamples++; return; }   /* ancien worker : sans "time" */
    netlog("lobby << %s", s);
    if (sscanf(s, "room %7s %d %d %d %63s %63s %d %d", a, &n1, &n2, &n3, b, c, &n4, &n5) >= 6) lb_room(a, n1, n2, n3, b, c, n4 & 1, n5);
    else if (sscanf(s, "gone %7s", a) == 1) {
        for (int i = 0; i < nFound; i++) if (found[i].online && !strcmp(found[i].id, a)) { found[i] = found[--nFound]; break; }
    } else if (sscanf(s, "go %63s %d", a, &n1) == 2) {
        struct sockaddr_in t;
        if (parse_ep(a, &t)) net_connect_ex(t, n1, 1);
    } else if (sscanf(s, "punch %63s", a) == 1) {
        struct sockaddr_in t;
        if (netMode != NET_HOST || !parse_ep(a, &t)) return;
        int k = 0;                                        /* un emplacement libre, sinon le plus ancien */
        for (int i = 0; i < 4; i++) if (punchQ[i].left <= 0) { k = i; break; }
        punchQ[k].addr = t; punchQ[k].left = 4; punchQ[k].acc = 1;
    } else if (!strcmp(s, "none")) {
        set_status(rankedWant ? "aucune partie classee : tu en crees une" : "aucune partie en ligne : tu en crees une");
        lbWantHost = 1;
    } else if (!strcmp(s, "err full")) set_status("partie pleine");
    else if (!strcmp(s, "err unknown")) set_status("cette partie n'existe plus");
}

static void lb_update(float dt) {
    int hosting = netMode == NET_HOST && !vsBot, browsing = netMode == NET_CLIENT && !connected;
    if (!onlineOn || (!hosting && !browsing)) { if (lbState != LB_OFF) lb_stop(); return; }

    if (lbState == LB_FAILED) {
        if (lbFailT == 0) lbFailT = nowT;
        if (nowT - lbFailT > 4) { lbFailT = 0; lb_stop(); lb_start(); }
    } else if (lbState == LB_OFF) lb_start();

    if (lbState == LB_ON && !lbGreeted) {                 /* (re)connecte : on remet le lobby au courant */
        lbGreeted = 1;
        for (int i = 0; i < nFound; i++) if (found[i].online) { found[i] = found[--nFound]; i--; }
        lbLastPlayers = -1;
        clkSamples = 0; clkAskT = -1; clkLastAskT = -100;  /* horloge : nouvelles mesures sur cette connexion */
        if (browsing) lb_send("list %d", NET_VER);
    }
    if (lbState == LB_ON && lbGreeted && clk_want_ask()) { lb_send("time"); clkAskT = clkLastAskT = nowT; }
    if (lbState == LB_FAILED) lbGreeted = 0;

    if (lbCsReady) {                                      /* lignes recues */
        static char tmp[LB_Q][LB_LINE];
        int n;
        EnterCriticalSection(&lbCs);
        n = lbInN;
        memcpy(tmp, lbIn, (size_t)n * LB_LINE);
        lbInN = 0;
        LeaveCriticalSection(&lbCs);
        for (int i = 0; i < n; i++) lb_line(tmp[i]);
    }

    if (!myPubOk) {                                       /* notre adresse vue d'Internet */
        if (lanEp) {
            struct sockaddr_in me;
            sock_len_t ml = sizeof me;
            if (getsockname(sock, (struct sockaddr *)&me, &ml) == 0) {
                me.sin_addr.s_addr = inet_addr("127.0.0.1");
                stun_got(&me);
            }
        } else if (stunReady && nowT - stunT > 0.7) { stunT = nowT; stun_send(); }
    } else if (hosting && !lanEp && nowT - stunT > 30) { stunT = nowT; stun_send(); }

    if (lbState == LB_ON && hosting && myPubOk) {
        int pl = 0, sp = spec_count();
        for (int i = 0; i < MAXP; i++) pl += NP[i].active;
        if (pl != lbLastPlayers || sp != lbLastSpec || nowT - lbHostT > 20) {
            char pub[40], lan[40], nm[16];
            int sum = 0;                                  /* mode classe : cote moyenne du salon */
            for (int i = 0; i < MAXP; i++) if (NP[i].active) sum += rkMmr[i] ? rkMmr[i] : RANK_START;
            lbLastPlayers = pl; lbLastSpec = sp; lbHostT = nowT;
            ep_text(&myPub, pub, sizeof pub);
            lan_ep_text(lan, sizeof lan, NET_PORT);
            lb_safe_name(nm, sizeof nm);
            if (teamSize > 0) snprintf(nm + (strlen(nm) > 11 ? 11 : strlen(nm)), 5, "_%dV%d", teamSize, teamSize);   /* le mode se lit dans le nom (15 car. max) */
            lb_send("host %d %d %d %d %s %s %s %d %d", NET_VER, pl, host_max_players(), sp, pub, lan, nm, ranked, pl ? sum / pl : 0);
        }
    }

    if (reqKind) {
        if (lbState == LB_ON && myPubOk) {
            char me[40];
            ep_text(&myPub, me, sizeof me);
            if (reqKind == 'j') lb_send("join %s %s %d", reqId, me, reqSpec);
            else lb_send("quick %d %s %d %d", NET_VER, me, rankedWant, myMmr);   /* mode classe : salon classe de cote proche */
            reqKind = 0; reqSentT = nowT;
        } else if (nowT - reqT > 10) {
            set_status(lbState == LB_ON ? "adresse publique introuvable (STUN bloque ?)" : "lobby injoignable");
            reqKind = 0;
        }
    }

    for (int i = 0; i < 4; i++) {                         /* l'hote ouvre son NAT vers les clients annonces */
        if (punchQ[i].left <= 0) continue;
        punchQ[i].left -= dt; punchQ[i].acc += dt;
        if (punchQ[i].acc >= 0.1f) {
            PkHead p;
            punchQ[i].acc = 0;
            head(&p, PK_PUNCH);
            net_send(&p, sizeof p, &punchQ[i].addr);
        }
    }
}

/* rejoindre / regarder la partie found[i] : directement (LAN) ou via le lobby (en ligne) */
static void join_found(int i, int spec) {
    if (i < 0 || i >= nFound) return;
    if (found[i].ranked && !spec && !rkOnline) {          /* mode classe : sans serveur de classement, pas de resultat */
        set_status("partie classee : serveur de classement injoignable");
        return;
    }
    if (found[i].online) {
        reqKind = 'j'; reqSpec = spec; reqT = nowT;
        snprintf(reqId, sizeof reqId, "%s", found[i].id);
        set_status(spec ? "demande pour regarder la partie ..." : "demande de connexion ...");
    } else net_connect_ex(found[i].addr, spec, 0);
}

/* PARTIE CLASSEE : il faut le serveur de classement (hello reussi) ; la partie rapide du lobby cherche un salon
 * classe de cote proche, sinon on heberge une partie classee */
static void ranked_play(void) {
    if (!onlineOn) { set_status("PARTIE CLASSEE : active les parties en ligne (PARAMETRES)"); return; }
    if (!rkOnline) {
        rkAfterHello = 1;
        if (!rkHelloBusy) rkHelloAt = nowT;               /* relance le hello tout de suite */
        set_status("connexion au serveur de classement ...");
        return;
    }
    if (!net_browse_start()) return;
    appState = APP_JOIN;
    rankedWant = 1;
    reqKind = 'q'; reqT = nowT;
    set_status("recherche d'une partie classee ...");
}

static void quick_play(void) {
    if (!net_browse_start()) return;
    appState = APP_JOIN;
    reqKind = 'q'; reqT = nowT;
    set_status("recherche d'une partie ...");
}

static int join_busy(void) { return joining || reqKind || nowT - reqSentT < 4; }

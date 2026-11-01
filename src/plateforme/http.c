/* VELOCITY - requete HTTP(S) simple et bloquante (WinHTTP sous Windows, curl du systeme sous macOS)
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c).
   Sert au mode classe (src/reseau/classe_serveur.c), qui l'appelle depuis un fil. Sous macOS il faut upd_spawn
   (src/interface/mise_a_jour.c, inclus plus haut). */

/* hostport : "hote" (https, 443) ou "hote:port" (http si le port n'est pas 443). POST si body != NULL (JSON).
 * Renvoie 1 si le serveur a repondu (quel que soit le statut HTTP : le JSON dit ok ou non), reponse dans out */
static int http_request(const char *hostport, const char *path, const char *body, char *out, int cap) {
    char host[96];
    int port = 443, n = 0, ok = 0, post = body != NULL;
    snprintf(host, sizeof host, "%s", hostport);
    char *colon = strchr(host, ':');
    if (colon) { *colon = 0; port = atoi(colon + 1); }
    out[0] = 0;
#ifdef _WIN32
    wchar_t whost[96], wpath[160];
    DWORD len = post ? (DWORD)strlen(body) : 0;
    MultiByteToWideChar(CP_UTF8, 0, host, -1, whost, 96);
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 160);
    HINTERNET ses = WinHttpOpen(L"VELOCITY", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET con = NULL, req = NULL;
    if (ses) {
        WinHttpSetTimeouts(ses, 5000, 5000, 10000, 10000);
        con = WinHttpConnect(ses, whost, (INTERNET_PORT)port, 0);
    }
    if (con) req = WinHttpOpenRequest(con, post ? L"POST" : L"GET", wpath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                      port == 443 ? WINHTTP_FLAG_SECURE : 0);
    if (req && WinHttpSendRequest(req, post ? L"Content-Type: application/json\r\n" : WINHTTP_NO_ADDITIONAL_HEADERS,
                                  post ? (DWORD)-1 : 0, post ? (LPVOID)body : WINHTTP_NO_REQUEST_DATA, len, len, 0) &&
        WinHttpReceiveResponse(req, NULL)) {
        ok = 1;
        while (n < cap - 1) {
            DWORD got = 0;
            if (!WinHttpReadData(req, out + n, (DWORD)(cap - 1 - n), &got)) { ok = 0; break; }
            if (!got) break;
            n += (int)got;
        }
    }
    if (req) WinHttpCloseHandle(req);
    if (con) WinHttpCloseHandle(con);
    if (ses) WinHttpCloseHandle(ses);
#else                                                     /* macOS : curl du systeme, sans -f pour lire le JSON d'erreur */
    char url[300], tmp[MAX_PATH];
    static int seq;
    pid_t pid;
    int st = 0;
    snprintf(url, sizeof url, "%s://%s%s", port == 443 ? "https" : "http", hostport, path);
    snprintf(tmp, sizeof tmp, "/tmp/velocity-http-%d-%d.txt", (int)getpid(), ++seq);
    char *get[] = { "/usr/bin/curl", "-sS", "--max-time", "10", "-o", tmp, url, NULL };
    char *postv[] = { "/usr/bin/curl", "-sS", "--max-time", "10", "-o", tmp, "-X", "POST", "-H", "Content-Type: application/json",
                      "--data-binary", (char *)(post ? body : ""), url, NULL };
    if (upd_spawn(post ? postv : get, &pid) && waitpid(pid, &st, 0) >= 0) ok = WIFEXITED(st) && WEXITSTATUS(st) == 0;
    FILE *f = fopen(tmp, "rb");
    if (f) { n = (int)fread(out, 1, (size_t)(cap - 1), f); fclose(f); }
    remove(tmp);
#endif
    out[n] = 0;
    return ok && n > 0;
}

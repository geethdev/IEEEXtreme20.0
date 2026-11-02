#include <bits/stdc++.h>
using namespace std;

static int N[3], TURNS, turnsUsed = 0;
static bool mine[3][64], revd[3][64], noneDen[3][64], elimC[3][64];
static vector<int> U[3], M[3];
static int K[3], upos[3][64], fixedFiller[3];
static int solvedJ[3] = {-1, -1, -1};
static const char CAT[3] = {'P', 'W', 'L'};
static mt19937_64 rng(12345);

enum { T_STRICT, T_MIXED, T_FILLER, T_RANDOM, T_NAIVE, T_ONE, T_TWO, T };
static const double PRIOR[T] = {0.15, 0.12, 0.1, 0.13, 0.1, 0.2, 0.2};
static const double PAR[5][2][3] = {
    {{0.92, 0.06, 0.02}, {0.30, 0.65, 0.05}},
    {{0.60, 0.30, 0.10}, {0.25, 0.50, 0.25}},
    {{0.35, 0.60, 0.05}, {0.10, 0.85, 0.05}},
    {{-1, 0, 0}, {-1, 0, 0}},
    {{0.93, 0.07, -1}, {0.30, 0.65, 0.05}},
};
static const double EPS = 0.04;
static const double TEMPER = 0.6;

static int NW;
static vector<double> W[T];
static double logS[T];
static vector<double> P;
static double pi_[T], qm[3][64];

static inline int wj(int w, int c) {
    if (c == 0) return w % K[0];
    if (c == 1) return w / K[0] % K[1];
    return w / (K[0] * K[1]);
}

static string cardName(int c, int i) { return string(1, CAT[c]) + to_string(i + 1); }
static bool parseCard(const string& s, int& c, int& i) {
    if (s.size() < 2) return false;
    const char* p = strchr("PWL", s[0]);
    if (!p) return false;
    c = p - "PWL";
    i = atoi(s.c_str() + 1) - 1;
    return i >= 0 && i < N[c];
}
static void say(const string& s) {
    if (getenv("CLUE_DEBUG")) fprintf(stderr, "> %s\n", s.c_str());
    cout << s << endl;
    cout.flush();
}

static int unrevealedMine(int c) {
    int n = 0;
    for (int x : M[c]) n += !revd[c][x];
    return n;
}
static int revealedMine(int c) { return (int)M[c].size() - unrevealedMine(c); }
static bool publicCat[3];
static double pubP[3];
static bool oppSolved(int c, int j) {
    return (publicCat[c] && pubP[c] > 0.5) || noneDen[c][U[c][j]] || unrevealedMine(c) == 0;
}

static void renorm() {
    for (int t = 0; t < T; t++) {
        double s = 0;
        for (double x : W[t]) s += x;
        if (s <= 0) {
            logS[t] = -1e300;
            continue;
        }
        logS[t] += log(s);
        for (double& x : W[t]) x /= s;
    }
}
static void zeroWorlds(int c, int j) {
    for (int w = 0; w < NW; w++)
        if (wj(w, c) == j)
            for (int t = 0; t < T; t++) W[t][w] = 0;
}
static void elimJ(int c, int j) {
    if (solvedJ[c] >= 0 || elimC[c][j]) return;
    int left = 0, last = -1;
    for (int k = 0; k < K[c]; k++)
        if (!elimC[c][k] && k != j) left++, last = k;
    if (left == 0) return;
    elimC[c][j] = true;
    zeroWorlds(c, j);
    if (left == 1) solvedJ[c] = last;
    renorm();
}
static void solveJ(int c, int j) {
    if (elimC[c][j] || solvedJ[c] >= 0) return;
    solvedJ[c] = j;
    for (int k = 0; k < K[c]; k++)
        if (k != j && !elimC[c][k]) elimC[c][k] = true, zeroWorlds(c, k);
    renorm();
}

static double likIndep(int t, int c, int s, int g, bool sol, int mu, int r) {
    double uni = 1.0 / N[c];
    if (PAR[t][0][0] < 0) return uni;
    int k = K[c], msz = M[c].size();
    bool naive = PAR[t][0][2] < 0;
    int sz[3];
    double pr[3];
    if (sol) {
        sz[0] = 1, sz[1] = k - 1, sz[2] = msz;
        for (int x = 0; x < 3; x++) pr[x] = PAR[t][1][x];
    } else if (naive) {
        sz[0] = msz + 1, sz[1] = k - 1, sz[2] = 0;
        pr[0] = PAR[t][0][0], pr[1] = PAR[t][0][1], pr[2] = 0;
    } else {
        sz[0] = mu + 1, sz[1] = k - 1, sz[2] = r;
        for (int x = 0; x < 3; x++) pr[x] = PAR[t][0][x];
    }
    int w;
    if (g == s) w = 0;
    else if (!mine[c][g]) w = 1;
    else if (sol) w = 2;
    else if (naive) w = 0;
    else w = revd[c][g] ? 2 : 0;
    double Z = 0;
    for (int x = 0; x < 3; x++)
        if (sz[x] > 0) Z += pr[x];
    if (sz[w] <= 0 || Z <= 0) return uni;
    return EPS * uni + (1 - EPS) * pr[w] / Z / sz[w];
}

static void observeOppGuess(const int g[3]) {
    static double li[5][3][64], fC[3][64], fO[3][64];
    static bool sl[3][64];
    double uni3 = 1.0 / ((double)N[0] * N[1] * N[2]);
    for (int c = 0; c < 3; c++) {
        int mu = unrevealedMine(c), r = revealedMine(c);
        for (int j = 0; j < K[c]; j++) {
            int s = U[c][j];
            bool sol = oppSolved(c, j);
            sl[c][j] = sol;
            for (int t = 0; t < 5; t++) li[t][c][j] = pow(likIndep(t, c, s, g[c], sol, mu, r), TEMPER);
            bool inC = g[c] == s || (!sol && mine[c][g[c]] && !revd[c][g[c]]);
            fC[c][j] = inC ? 1.0 / (sol ? 1 : mu + 1) : 0;
            fO[c][j] = (!mine[c][g[c]] && g[c] != s) ? 1.0 / (K[c] - 1) : 0;
        }
    }
    for (int w = 0; w < NW; w++) {
        if (W[T_ONE][w] == 0 && W[T_STRICT][w] == 0 && W[T_RANDOM][w] == 0) continue;
        int j[3] = {wj(w, 0), wj(w, 1), wj(w, 2)};
        for (int t = 0; t < 5; t++) W[t][w] *= li[t][0][j[0]] * li[t][1][j[1]] * li[t][2][j[2]];
        double C[3], O[3];
        int un[3], nu = 0;
        for (int c = 0; c < 3; c++) {
            C[c] = fC[c][j[c]], O[c] = fO[c][j[c]];
            if (!sl[c][j[c]]) un[nu++] = c;
        }
        double one = 0, two = 0;
        if (nu == 0) {
            one = two = uni3;
        } else {
            for (int a = 0; a < nu; a++) {
                int p = un[a];
                double v = C[p];
                for (int c = 0; c < 3; c++)
                    if (c != p) v *= O[c];
                one += v / nu;
            }
            if (nu == 1) two = one;
            else if (nu == 2) {
                int f = 3 - un[0] - un[1];
                two = O[f] * C[un[0]] * C[un[1]];
            } else {
                for (int f = 0; f < 3; f++) two += O[f] * C[(f + 1) % 3] * C[(f + 2) % 3] / 3;
            }
        }
        W[T_ONE][w] *= pow(EPS * uni3 + (1 - EPS) * one, TEMPER);
        W[T_TWO][w] *= pow(EPS * uni3 + (1 - EPS) * two, TEMPER);
    }
    renorm();
}

static void computePosterior() {
    double mx = -1e300;
    for (int t = 0; t < T; t++) mx = max(mx, log(PRIOR[t]) + logS[t]);
    double s = 0;
    for (int t = 0; t < T; t++) s += pi_[t] = logS[t] < -1e299 ? 0 : exp(log(PRIOR[t]) + logS[t] - mx);
    for (int t = 0; t < T; t++) pi_[t] /= s;
    P.assign(NW, 0);
    for (int t = 0; t < T; t++)
        if (pi_[t] > 0)
            for (int w = 0; w < NW; w++) P[w] += pi_[t] * W[t][w];
    double tot = 0;
    for (double x : P) tot += x;
    if (!(tot > 0)) {
        tot = 0;
        for (int w = 0; w < NW; w++) {
            P[w] = !elimC[0][wj(w, 0)] && !elimC[1][wj(w, 1)] && !elimC[2][wj(w, 2)];
            tot += P[w];
        }
        if (!(tot > 0)) P.assign(NW, 1), tot = NW;
    }
    for (double& x : P) x /= tot;
    for (int c = 0; c < 3; c++)
        for (int j = 0; j < K[c]; j++) qm[c][j] = 0;
    for (int w = 0; w < NW; w++)
        for (int c = 0; c < 3; c++) qm[c][wj(w, c)] += P[w];
}

static vector<double> testDist(vector<double> p) {
    sort(p.rbegin(), p.rend());
    int k = p.size();
    if (k <= 1) return {1.0};
    vector<double> d(k, 0);
    for (int i = 0; i < k; i++) d[min(i + 1, k - 1)] += p[i];
    return d;
}
static vector<double> conv(const vector<double>& a, const vector<double>& b) {
    vector<double> r(a.size() + b.size() - 1, 0);
    for (size_t i = 0; i < a.size(); i++)
        for (size_t j = 0; j < b.size(); j++) r[i + j] += a[i] * b[j];
    return r;
}
static double mean(const vector<double>& d) {
    double m = 0;
    for (size_t i = 0; i < d.size(); i++) m += i * d[i];
    return m;
}

static bool shouldAccuse(double pstar, double risk) {
    int mu[3];
    double psm[3] = {0, 0, 0};
    for (int c = 0; c < 3; c++) {
        mu[c] = unrevealedMine(c);
        for (int j = 0; j < K[c]; j++)
            if (oppSolved(c, j)) psm[c] += qm[c][j];
    }
    vector<double> me = {1.0}, op = {1.0};
    for (int c = 0; c < 3; c++) {
        vector<double> p;
        for (int j = 0; j < K[c]; j++)
            if (!elimC[c][j]) p.push_back(qm[c][j]);
        me = conv(me, testDist(p));
        vector<double> u(1, 1.0);
        if (mu[c] > 0) {
            int m = mu[c] + 1;
            u = testDist(vector<double>(m, 1.0 / m));
            for (auto& x : u) x *= 1 - psm[c];
            u[0] += psm[c];
        }
        op = conv(op, u);
    }
    double opRest = 1 - op[0];
    double pWin = 0;
    if (opRest > 1e-9) {
        vector<double> cum(me.size() + 1, 0);
        for (size_t i = 0; i < me.size(); i++) cum[i + 1] = cum[i] + me[i];
        for (size_t j = 1; j < op.size(); j++) pWin += op[j] / opRest * cum[min(j + 1, me.size())];
        op[0] = 0;
        for (auto& x : op) x /= opRest;
    }
    double Em = mean(me), Eo = mean(op);
    double frac = Em > 0 ? min(1.0, Eo / Em) : 1.0;
    double pThen = pow(max(pstar, 1e-12), 1 - frac);
    double vCont = pWin * 10 + (1 - pWin) * (3 + 7 * pThen);
    double vWait = 0;
    for (int w = 0; w < NW; w++) {
        if (P[w] <= 0) continue;
        double x = 1;
        for (int c = 0; c < 3; c++)
            if (!oppSolved(c, wj(w, c))) x /= mu[c] + 1;
        double g = x > 0.999 ? 1.0 : 0.3 * x;
        vWait += P[w] * (g * 7 * (1 - x) + (1 - g) * vCont);
    }
    vWait *= 1 - risk;
    double vAcc = 3 + 7 * pstar;
    if (getenv("CLUE_DEBUG"))
        fprintf(stderr, "turn %d p*=%.3f pWin=%.3f vAcc=%.2f vWait=%.2f Em=%.1f Eo=%.1f\n", turnsUsed, pstar,
                pWin, vAcc, vWait, Em, Eo);
    return vAcc > vWait;
}

static double entropy(int c, int skip = -1) {
    double s = 0, h = 0;
    for (int j = 0; j < K[c]; j++)
        if (!elimC[c][j] && j != skip) s += qm[c][j];
    if (s <= 0) return 0;
    for (int j = 0; j < K[c]; j++)
        if (!elimC[c][j] && j != skip && qm[c][j] > 0) {
            double p = qm[c][j] / s;
            h -= p * log(p);
        }
    return h;
}

static int lastGuess[3];

static double stealRisk[3][64];
static void computeStealRisk() {
    for (int c = 0; c < 3; c++)
        for (int j = 0; j < K[c]; j++) stealRisk[c][j] = 0;
    for (int w = 0; w < NW; w++) {
        if (P[w] <= 0) continue;
        bool kn[3];
        for (int c = 0; c < 3; c++) kn[c] = oppSolved(c, wj(w, c));
        for (int c = 0; c < 3; c++)
            if (kn[(c + 1) % 3] && kn[(c + 2) % 3]) stealRisk[c][wj(w, c)] += P[w];
    }
}

static double planGuess(int out[3], bool fillersOnly = false) {
    const double LAMBDA = 4;
    computeStealRisk();
    int bc = -1, bjj = -1;
    double best = -1e18;
    if (!fillersOnly)
        for (int c = 0; c < 3; c++) {
            if (solvedJ[c] >= 0) continue;
            double H = entropy(c);
            for (int j = 0; j < K[c]; j++) {
                if (elimC[c][j]) continue;
                double qq = qm[c][j];
                double gain = qq * H + (1 - qq) * (H - entropy(c, j));
                double score = gain - LAMBDA * stealRisk[c][j] + 1e-9 * (rng() % 1000);
                if (score > best) best = score, bc = c, bjj = j;
            }
        }
    for (int c = 0; c < 3; c++) {
        if (c == bc) {
            out[c] = U[c][bjj];
            continue;
        }
        if (revd[c][fixedFiller[c]]) {
            vector<int> f;
            for (int x : M[c])
                if (!revd[c][x]) f.push_back(x);
            if (!f.empty()) fixedFiller[c] = f[rng() % f.size()];
        }
        out[c] = fixedFiller[c];
    }
    return bc >= 0 ? stealRisk[bc][bjj] : 0;
}

static void emitGuess(const int g[3]) {
    for (int c = 0; c < 3; c++) lastGuess[c] = g[c];
    say("GUESS " + cardName(0, g[0]) + " " + cardName(1, g[1]) + " " + cardName(2, g[2]));
}

static void myTurn() {
    computePosterior();
    int bw = 0;
    for (int w = 1; w < NW; w++)
        if (P[w] > P[bw]) bw = w;
    double pstar = P[bw];
    bool certain = solvedJ[0] >= 0 && solvedJ[1] >= 0 && solvedJ[2] >= 0;
    for (int c = 0; c < 3; c++)
        if (solvedJ[c] < 0) pstar = min(pstar, pstar * 0.85 / max(0.85, qm[c][wj(bw, c)]));
    bool last = turnsUsed + 3 > TURNS;
    int g[3];
    if (last && !certain && turnsUsed + 1 < TURNS) {
        const double A_END = 0.7;
        int mu[3];
        for (int c = 0; c < 3; c++) mu[c] = unrevealedMine(c);
        double vWait = 0;
        for (int w = 0; w < NW; w++) {
            double x = 1;
            for (int c = 0; c < 3; c++)
                if (!oppSolved(c, wj(w, c))) x /= mu[c] + 1;
            vWait += P[w] * A_END * 7 * (1 - x);
        }
        if (3 + 7 * pstar < vWait) {
            planGuess(g, true);
            emitGuess(g);
            return;
        }
    }
    double risk = certain ? 0 : planGuess(g);
    if (certain || last || shouldAccuse(pstar, risk)) {
        say("ACCUSE " + cardName(0, U[0][wj(bw, 0)]) + " " + cardName(1, U[1][wj(bw, 1)]) + " " +
            cardName(2, U[2][wj(bw, 2)]));
        return;
    }
    emitGuess(g);
}

static void oppGuess(const int g[3]) {
    observeOppGuess(g);
    for (int c = 0; c < 3; c++)
        if (publicCat[c]) {
            bool probesMine = mine[c][g[c]] && !revd[c][g[c]];
            double lk = probesMine ? 0.05 : 0.95, ln = probesMine ? 0.35 : 0.65;
            pubP[c] = pubP[c] * lk / (pubP[c] * lk + (1 - pubP[c]) * ln);
        }
    vector<pair<int, int>> held;
    for (int c = 0; c < 3; c++)
        if (mine[c][g[c]]) held.push_back({c, g[c]});
    if (held.empty()) {
        for (int c = 0; c < 3; c++) noneDen[c][g[c]] = true;
        say("NONE");
        return;
    }
    vector<pair<int, int>> shown;
    for (auto& h : held)
        if (revd[h.first][h.second]) shown.push_back(h);
    pair<int, int> pick;
    if (!shown.empty()) pick = shown[rng() % shown.size()];
    else {
        shuffle(held.begin(), held.end(), rng);
        pick = held[0];
        for (auto& h : held)
            if (unrevealedMine(h.first) > unrevealedMine(pick.first)) pick = h;
    }
    revd[pick.first][pick.second] = true;
    say("SHOW " + cardName(pick.first, pick.second));
}

int main() {
    rng.seed(chrono::steady_clock::now().time_since_epoch().count());
    string line;
    while (getline(cin, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (getenv("CLUE_DEBUG")) fprintf(stderr, "< %s\n", line.c_str());
        istringstream is(line);
        string cmd;
        is >> cmd;
        if (cmd == "GAME") {
            is >> N[0] >> N[1] >> N[2] >> TURNS;
        } else if (cmd == "HAND") {
            string s;
            while (is >> s) {
                int c, i;
                if (parseCard(s, c, i)) mine[c][i] = true;
            }
            for (int c = 0; c < 3; c++) {
                U[c].clear(), M[c].clear();
                for (int i = 0; i < N[c]; i++) {
                    upos[c][i] = -1;
                    if (mine[c][i]) M[c].push_back(i);
                    else upos[c][i] = U[c].size(), U[c].push_back(i);
                }
                K[c] = U[c].size();
                if (K[c] == 1) solvedJ[c] = 0;
                fixedFiller[c] = M[c].empty() ? 0 : M[c][rng() % M[c].size()];
            }
            NW = K[0] * K[1] * K[2];
            for (int t = 0; t < T; t++) W[t].assign(NW, 1.0 / NW), logS[t] = 0;
        } else if (cmd == "TURN") {
            string who;
            is >> who;
            if (who == "YOU") {
                myTurn();
            } else {
                string tag, a[3];
                is >> tag >> a[0] >> a[1] >> a[2];
                int g[3] = {0, 0, 0};
                for (int k = 0; k < 3; k++) {
                    int c, i;
                    if (parseCard(a[k], c, i)) g[c] = i;
                }
                oppGuess(g);
            }
            turnsUsed++;
        } else if (cmd == "RESULT") {
            string kind;
            is >> kind;
            if (kind == "SHOWN") {
                string s;
                is >> s;
                int c, i;
                if (parseCard(s, c, i) && !mine[c][i]) elimJ(c, upos[c][i]);
            } else {
                for (int c = 0; c < 3; c++)
                    if (!mine[c][lastGuess[c]]) solveJ(c, upos[c][lastGuess[c]]), publicCat[c] = true, pubP[c] = 0.75;
            }
        } else if (cmd == "GAME_END") {
            break;
        }
    }
    return 0;
}

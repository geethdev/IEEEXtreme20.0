#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
typedef long long ll;
typedef unsigned long long u64;

static double wallNow() {
    static const auto t0 = chrono::steady_clock::now();
    return chrono::duration<double>(chrono::steady_clock::now() - t0).count();
}

static double envd(const char* k, double d) {
    const char* s = getenv(k);
    return s ? atof(s) : d;
}

struct Duel {
    double TIME_TOTAL = envd("AD_TIME", 1.0);
    vector<double> THETAS = envd("AD_WIDE", 1) > 0 ? vector<double>{0.35, 0.5, 0.7, 0.85, 1.0, 1.2, 1.5, 2.0}
                                                   : vector<double>{0.5, 0.7, 0.85, 1.0, 1.2, 1.5};
    int NQ = envd("AD_WIDE", 1) > 0 ? 20 : 12;
    double TEMPER = envd("AD_TEMPER", 0.5), FPSHADE = envd("AD_FPSHADE", 0.5), PUSH = envd("AD_PUSH", 2.0);
    double BONUSW = envd("AD_BONUSW", 1.0);
    bool CAP = envd("AD_CAP", 1) > 0;

    int K, M, B, NR;
    bool FP;
    double bf;
    vector<int> V, L;
    double meanL, lv[8];
    ll myC, opC;
    int rnd, curG;
    int rem[8], mw[8], ow[8];

    int NP;
    vector<uint8_t> perm;
    vector<double> W, cdf;
    vector<array<float, 6>> CF;
    double sig;
    double Ev[8];

    double G[8][6][6];
    int nobs;
    double b0[6], lam[6];
    double logR[64];
    ll opBefore, myBefore = 0;

    mt19937_64 rng{0x5DEECE66DULL};
    double tUsed = 0, stepCost = 40e-9;
    double theta = 1.0;

    void start(int K_, int M_, ll cash, int B_, bool fp, const vector<int>& vals) {
        K = K_, M = M_, B = B_, FP = fp;
        bf = B / 100.0;
        NR = K * M;
        V = vals;
        L = vals;
        sort(L.begin(), L.end());
        meanL = 0;
        for (int x : L) meanL += x;
        meanL /= K;
        for (int j = 0; j < K; j++) lv[j] = log(L[j] / meanL);
        myC = opC = cash;
        for (int g = 0; g < K; g++) rem[g] = M, mw[g] = ow[g] = 0;
        vector<int> a(K);
        iota(a.begin(), a.end(), 0);
        perm.clear();
        do
            for (int g = 0; g < K; g++) perm.push_back(a[g]);
        while (next_permutation(a.begin(), a.end()));
        NP = perm.size() / K;
        memset(G, 0, sizeof G);
        nobs = 0;
        for (int r = 1; r < 64; r++) logR[r] = log((double)r);
        double P0 = (double)cash / NR;
        b0[1] = envd("AD_SLOPE", 0.45), b0[2] = 0.5, b0[3] = -0.5;
        b0[0] = log(envd("AD_PRIORMUL", 1.4) * min(meanL, P0) + 1) - b0[2] * log(cash + 1.0) - b0[3] * logR[NR];
        lam[0] = 1, lam[1] = 2, lam[2] = 2, lam[3] = 2;
        b0[4] = 0, b0[5] = 0, lam[4] = envd("AD_SETLAM", 3), lam[5] = envd("AD_SETLAM", 3);
        sig = 0.5;
        W.assign(NP, 1.0 / NP);
        CF.assign(NP, {(float)b0[0], (float)b0[1], (float)b0[2], (float)b0[3], (float)b0[4], (float)b0[5]});
        buildCdf();
        u64 seed = getenv("AD_SEED") ? (u64)atoll(getenv("AD_SEED"))
                                      : (u64)chrono::steady_clock::now().time_since_epoch().count();
        rng.seed(0x5DEECE66DULL ^ (u64)cash * 1000003ULL ^ seed);
    }

    void buildCdf() {
        cdf.resize(NP);
        double s = 0;
        for (int p = 0; p < NP; p++) cdf[p] = (s += W[p]);
        for (int g = 0; g < K; g++) Ev[g] = 0;
        for (int p = 0; p < NP; p++)
            for (int g = 0; g < K; g++) Ev[g] += W[p] * L[perm[p * K + g]];
        for (int g = 0; g < K; g++) Ev[g] /= s;
    }

    void fit() {
        const int F = 6;
        const int gi[F] = {0, -1, 1, 2, 3, 4};
        double Gt[6][6] = {};
        for (int g = 0; g < K; g++)
            for (int i = 0; i < 6; i++)
                for (int j = 0; j < 6; j++) Gt[i][j] += G[g][i][j];
        double pen = 0;
        for (int i = 0; i < F; i++) pen += lam[i] * b0[i] * b0[i];
        vector<double> rss(NP);
        double mn = 1e300;
        for (int p = 0; p < NP; p++) {
            const uint8_t* pr = &perm[p * K];
            double lm[6] = {}, lln = 0;
            for (int g = 0; g < K; g++)
                if (G[g][0][0] > 0) {
                    double l = lv[pr[g]];
                    for (int k = 0; k < 6; k++) lm[k] += l * G[g][0][k];
                    lln += l * l * G[g][0][0];
                }
            double A[6][6], r[6];
            for (int i = 0; i < F; i++) {
                for (int j = 0; j < F; j++) {
                    if (i == 1 && j == 1) A[i][j] = lln;
                    else if (i == 1) A[i][j] = lm[gi[j]];
                    else if (j == 1) A[i][j] = lm[gi[i]];
                    else A[i][j] = Gt[gi[i]][gi[j]];
                }
                r[i] = (i == 1) ? lm[5] : Gt[gi[i]][5];
            }
            for (int i = 0; i < F; i++) A[i][i] += lam[i], r[i] += lam[i] * b0[i];
            double Lc[6][6] = {}, x[6], yv[6];
            for (int i = 0; i < F; i++)
                for (int j = 0; j <= i; j++) {
                    double v = A[i][j];
                    for (int k = 0; k < j; k++) v -= Lc[i][k] * Lc[j][k];
                    Lc[i][j] = i == j ? sqrt(max(v, 1e-12)) : v / Lc[j][j];
                }
            for (int i = 0; i < F; i++) {
                double v = r[i];
                for (int k = 0; k < i; k++) v -= Lc[i][k] * yv[k];
                yv[i] = v / Lc[i][i];
            }
            for (int i = F - 1; i >= 0; i--) {
                double v = yv[i];
                for (int k = i + 1; k < F; k++) v -= Lc[k][i] * x[k];
                x[i] = v / Lc[i][i];
            }
            double xr = 0;
            for (int i = 0; i < F; i++) xr += x[i] * r[i];
            rss[p] = Gt[5][5] + pen - xr;
            for (int i = 0; i < F; i++) CF[p][i] = (float)x[i];
            mn = min(mn, rss[p]);
        }
        double s2 = max(0.08 * 0.08, (max(0.0, mn) + 0.35 * 0.35 * 3) / (nobs + 3));
        sig = sqrt(s2);
        for (int p = 0; p < NP; p++) W[p] = exp(-(rss[p] - mn) / (2 * s2) * TEMPER);
        buildCdf();
    }

    inline int samplePerm() {
        double u = uniform_real_distribution<double>(0, cdf[NP - 1])(rng);
        return min(NP - 1, (int)(lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin()));
    }

    inline double oppMu(int p, int g, ll oc, int remIncl, int myW, int opW) const {
        const array<float, 6>& c = CF[p];
        double d = (myW > 0 && opW == 0) ? 1.0 : 0.0, o = (opW > 0 && myW == 0) ? 1.0 : 0.0;
        return c[0] + c[1] * lv[perm[p * K + g]] + c[2] * log(oc + 1.0) + c[3] * logR[remIncl] + c[4] * d + c[5] * o;
    }
    inline ll oppBidFrom(double mu, double noise, ll oc, ll mcap) const {
        double q = exp(mu + noise) - 1;
        if (q <= 0) return 0;
        ll r = (ll)(q + 0.5);
        if (CAP && r > mcap + 1) r = mcap + 1;
        return r > oc ? oc : r;
    }

    inline ll policyBid(int g, double vo, double qhat, ll mc, ll oc, const int* r, const int* a, const int* b, double th) const {
        double eMe = V[g], eOp = vo;
        if (b[g] == 0) eMe += BONUSW * bf * M * V[g] / r[g];
        if (a[g] == 0) eOp += BONUSW * bf * M * vo / r[g];
        double t = th * 0.5 * (eMe + eOp);
        if (FP) t = min(t, qhat * exp(FPSHADE * sig) + 1);
        else if (PUSH > 0) t = max(t, qhat * exp(-PUSH * sig));
        ll x = (ll)t;
        if (CAP && x > oc + 1) x = oc + 1;
        return x > mc ? mc : (x < 0 ? 0 : x);
    }

    struct Sample {
        int p;
        float z[64];
        int ord[64];
        u64 coin;
        ll q;
    };

    double roll(const Sample& s, int T, ll mc, ll oc, const int* mw0, const int* ow0, const int* rem0, double th) const {
        int a[8], b[8], r[8];
        memcpy(a, mw0, K * sizeof(int)), memcpy(b, ow0, K * sizeof(int)), memcpy(r, rem0, K * sizeof(int));
        const uint8_t* pr = &perm[s.p * K];
        for (int t = 0; t < T; t++) {
            int g = s.ord[t];
            int remIncl = T - t;
            double mu = oppMu(s.p, g, oc, remIncl, a[g], b[g]);
            double vo = L[pr[g]];
            ll my = policyBid(g, vo, exp(mu) - 1, mc, oc, r, a, b, th);
            ll q = oppBidFrom(mu, sig * s.z[t + 1], oc, mc);
            bool win = my > q || (my == q && ((s.coin >> t) & 1));
            if (win) mc -= FP ? my : q, a[g]++;
            else oc -= FP ? q : my, b[g]++;
            r[g]--;
        }
        double d = (double)(mc - oc);
        for (int g = 0; g < K; g++) {
            d += a[g] * V[g] * (a[g] == M ? 1 + bf : 1.0);
            d -= b[g] * L[pr[g]] * (b[g] == M ? 1 + bf : 1.0);
        }
        return d;
    }

    double evalBid(const Sample& s, int T, ll x, double th) const {
        int a[8], b[8], r[8];
        memcpy(a, mw, K * sizeof(int)), memcpy(b, ow, K * sizeof(int)), memcpy(r, rem, K * sizeof(int));
        ll mc = myC, oc = opC;
        bool win = x > s.q || (x == s.q && (s.coin >> 63));
        if (win) mc -= FP ? x : s.q, a[curG]++;
        else oc -= FP ? s.q : x, b[curG]++;
        r[curG]--;
        return roll(s, T, mc, oc, a, b, r, th);
    }

    ll bid(int n, int g, ll my, ll op) {
        double t0 = wallNow();
        rnd = n, curG = g, myC = my, opC = op, opBefore = op, myBefore = my;
        int remIncl = NR - n + 1;
        int T = remIncl - 1;
        ll ans = decide(T, remIncl, t0);
        ans = max(0LL, min(ans, myC));
        tUsed += wallNow() - t0;
        return ans;
    }

    ll decide(int T, int remIncl, double t0) {
        if (myC == 0) return 0;
        double budget = max(0.0, TIME_TOTAL - tUsed) / remIncl;
        const int NC_EST = (int)THETAS.size() + 30;
        int N = (int)(budget / (stepCost * NC_EST * (T + 1) + 1e-12));
        N = max(24, min(N, 3000));

        vector<int> base;
        for (int g = 0; g < K; g++)
            for (int k = 0; k < rem[g] - (g == curG); k++) base.push_back(g);
        vector<Sample> S(N);
        normal_distribution<float> nd(0, 1);
        for (auto& s : S) {
            s.p = samplePerm();
            for (int t = 0; t <= T; t++) s.z[t] = nd(rng);
            for (int t = 0; t < T; t++) s.ord[t] = base[t];
            shuffle(s.ord, s.ord + T, rng);
            s.coin = rng();
            s.q = oppBidFrom(oppMu(s.p, curG, opC, remIncl, mw[curG], ow[curG]), sig * s.z[0], opC, myC);
        }
        long long steps = 0;

        double bestV = -1e300;
        int bt = 0;
        vector<double> qh(N);
        for (int i = 0; i < N; i++) qh[i] = exp(oppMu(S[i].p, curG, opC, remIncl, mw[curG], ow[curG])) - 1;
        for (int k = 0; k < (int)THETAS.size(); k++) {
            double th = THETAS[k], tot = 0;
            for (int i = 0; i < N; i++) {
                ll x = policyBid(curG, Ev[curG], qh[i], myC, opC, rem, mw, ow, th);
                tot += evalBid(S[i], T, x, th);
            }
            steps += (ll)N * (T + 1);
            if (tot > bestV) bestV = tot, bt = k;
        }
        theta = THETAS[bt];

        vector<ll> qs(N);
        for (int i = 0; i < N; i++) qs[i] = S[i].q;
        sort(qs.begin(), qs.end());
        vector<ll> cand = {0, myC};
        for (int k = 0; k < NQ; k++) {
            ll q = qs[min(N - 1, (int)((k + 0.5) / NQ * N))];
            cand.push_back(q + 1);
            cand.push_back(q - 1);
            cand.push_back(q);
        }
        if (CAP) cand.push_back(opC + 1);
        for (auto& c : cand) c = max(0LL, min(c, CAP ? min(myC, opC + 1) : myC));
        sort(cand.begin(), cand.end());
        cand.erase(unique(cand.begin(), cand.end()), cand.end());
        ll best = 0;
        bestV = -1e300;
        for (ll c : cand) {
            double tot = 0;
            for (int i = 0; i < N; i++) tot += evalBid(S[i], T, c, theta);
            steps += (ll)N * (T + 1);
            if (tot > bestV) bestV = tot, best = c;
        }
        double el = wallNow() - t0;
        if (steps > 20000) stepCost = 0.7 * stepCost + 0.3 * (el / steps);
        return best;
    }

    void result(bool won, ll oppBid, ll price) {
        double fd = (mw[curG] > 0 && ow[curG] == 0) ? 1.0 : 0.0, fo = (ow[curG] > 0 && mw[curG] == 0) ? 1.0 : 0.0;
        if (won) mw[curG]++, myC -= price;
        else ow[curG]++, opC -= price;
        rem[curG]--;
        bool capped = CAP && oppBid >= myBefore + 1 && oppBid < opBefore;
        if (opBefore > 0 && !capped) {
            double z[6] = {1, log(opBefore + 1.0), logR[NR - rnd + 1], fd, fo, log(oppBid + 1.0)};
            for (int i = 0; i < 6; i++)
                for (int j = 0; j < 6; j++) G[curG][i][j] += z[i] * z[j];
            nobs++;
            double t0 = wallNow();
            fit();
            tUsed += wallNow() - t0;
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    Duel bot;
    string line;
    while (getline(cin, line)) {
        istringstream is(line);
        string tag;
        is >> tag;
        if (tag == "GAME") {
            int K, M, B;
            ll cash;
            string rule;
            is >> K >> M >> cash >> B >> rule;
            string l2;
            while (getline(cin, l2) && l2.find("VALUES") == string::npos) {}
            istringstream vs(l2);
            vs >> tag;
            vector<int> v(K);
            for (auto& x : v) vs >> x;
            bot.start(K, M, cash, B, rule == "first", v);
        } else if (tag == "ROUND") {
            int n, g;
            ll my, op;
            is >> n >> g >> my >> op;
            ll b = bot.bid(n, g - 1, my, op);
            printf("BID %lld\n", b);
            fflush(stdout);
        } else if (tag == "RESULT") {
            int w;
            ll ob, pr;
            is >> w >> ob >> pr;
            bot.result(w == 1, ob, pr);
        } else if (tag == "GAME_END") {
            break;
        }
    }
    return 0;
}
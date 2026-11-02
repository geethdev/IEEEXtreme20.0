#include <bits/stdc++.h>
#define F(i, a, b) for (int i = (a); i <= (b); ++i)
#define E(G, u) for (int e = G.h[u]; e; e = G.nx[e])
using namespace std;
typedef long long ll;
const int N = 100005, V = 200005, K = 400010, H = 3000005;
template <int NV, int NE> struct Grafu {
        int h[NV], to[NE], nx[NE], e; ll w[NE];
            void unga(int u, int v, ll x) { to[++e] = v, w[e] = x, nx[e] = h[u], h[u] = e; }
                void mbili(int u, int v, ll x) { unga(u, v, x), unga(v, u, x); }
};
struct Hali { int n1, n2; ll u1, u2, k; };
Grafu<N, V> A, C; Grafu<V, K> B;
bool kata[K];
int n, q, jd[17][N], zr[N], hz, jv, mg, kl, sz[V], hl[V], rU[V], rP[V], mp[V], uk[V], wV[V];
int idM[N], nfM[N][32], ila[N], hN[H], hS[K], toS[3500005], nxS[3500005], eS;
ll um[N], umN[N], rD[V], wW[V], jb[N], uzM[N][32];
Hali vp[K], hH[H];

ll tof(int a, ll wa, int b, ll wb) {
        if (a > b) swap(a, b);
            int k = __lg(b - a);
                return wa + wb - 2 * umN[min(jd[k][a + 1], jd[k][b - (1 << k) + 1])];
}
void zunguka() {
        F(i, 1, n) sz[i] = C.h[i];
            int j = 1; rU[0] = 1, rP[0] = 0, zr[1] = ++hz, jd[0][1] = 1;
                while (j) {
                            int u = rU[j - 1], e = sz[u];
                                    while (e && C.to[e] == rP[j - 1]) e = C.nx[e];
                                            sz[u] = e ? C.nx[e] : 0;
                                                    if (!e) { --j; continue; }
                                                            int v = C.to[e];
                                                                    um[v] = um[u] + C.w[e], zr[v] = ++hz, umN[hz] = um[v], jd[0][hz] = zr[u], rU[j] = v, rP[j++] = u;
                }
                    for (int j = 1; (1 << j) <= hz; ++j) F(i, 1, hz - (1 << j) + 1) jd[j][i] = min(jd[j - 1][i], jd[j - 1][i + (1 << (j - 1))]);
}
void pacha() {
        jv = n; int f = 0, b = 1; rU[0] = 1, rP[1] = 0;
            while (f < b) {
                        int u = rU[f++], wa = 0, wb = 0;
                                E(A, u) if (int v = A.to[e]; v != rP[u]) rP[v] = u, rU[b++] = v, wV[wb] = v, wW[wb++] = A.w[e];
                                        for (; wb - wa > 2; wa += 2) B.mbili(++jv, wV[wa], wW[wa]), B.mbili(jv, wV[wa + 1], wW[wa + 1]), wV[wb] = jv, wW[wb++] = 0;
                                                for (; wa < wb; ++wa) B.mbili(u, wV[wa], wW[wa]);
            }
}
void kusanya(int s, ll d0, int nf) {
        int j = 1; rU[0] = s, rP[0] = 0, rD[0] = d0;
            while (j) {
                        int u = rU[--j], p = rP[j]; ll d = rD[j];
                                if (u <= n) nfM[u][idM[u]] = nf, uzM[u][idM[u]++] = d + um[u];
                                        E(B, u) if (!kata[e] && B.to[e] != p) rU[j] = B.to[e], rP[j] = u, rD[j++] = d + B.w[e];
            }
}
void gawanya(int r) {
        int j = 1, c = 0, t = 0; rU[0] = r, rP[0] = 0, uk[r] = 0;
            while (j) {
                        int u = rU[--j], p = rP[j];
                                mp[c++] = u, sz[u] = 1, t += hl[u] = u <= n;
                                        E(B, u) if (!kata[e] && B.to[e] != p) uk[B.to[e]] = e, rU[j] = B.to[e], rP[j++] = u;
            }
                if (t <= 1) return;
                    int bo = -1, th = c + 1;
                        for (int i = c - 1; i >= 1; --i) {
                                    int u = mp[i], p = B.to[uk[u] ^ 1], m = max(sz[u], c - sz[u]);
                                            sz[p] += sz[u], hl[p] += hl[u];
                                                    if (m < th) th = m, bo = uk[u];
                        }
                            kata[bo] = kata[bo ^ 1] = 1;
                                int u0 = B.to[bo], u1 = B.to[bo ^ 1], h0 = hl[u0], h1 = t - h0;
                                    if (h0 > 0 && h1 > 0) kusanya(u0, 0, mg << 1), kusanya(u1, B.w[bo], mg++ << 1 | 1);
                                        if (h0 > 1) gawanya(u0);
                                            if (h1 > 1) gawanya(u1);
}
void weka(int x, int l, int r, int ql, int qr, int c) {
        if (ql <= l && r <= qr) { toS[++eS] = c, nxS[eS] = hS[x], hS[x] = eS; return; }
            int m = (l + r) >> 1;
                if (ql <= m) weka(x << 1, l, m, ql, qr, c);
                    if (qr > m) weka(x << 1 | 1, m + 1, r, ql, qr, c);
}
void pita(int x, int l, int r, ll bo) {
        int al = kl;
            for (int e = hS[x]; e; e = nxS[e]) {
                        int u = toS[e], tu = zr[u];
                                for (int i = 0; i < idM[u]; ++i) {
                                                int s = nfM[u][i]; ll wu = uzM[u][i];
                                                            Hali o = vp[s ^ 1], h = vp[s];
                                                                        if (o.n1) {
                                                                                            bo = max(bo, tof(tu, wu, o.n1, o.u1));
                                                                                                            if (o.n2 != o.n1) bo = max(bo, tof(tu, wu, o.n2, o.u2));
                                                                        }
                                                                                    bool bd = !h.n1;
                                                                                                if (bd) h = {tu, tu, wu, wu, 0};
                                                                                                            else {
                                                                                                                                ll d1 = tof(tu, wu, h.n1, h.u1), d2 = h.n2 == h.n1 ? -1 : tof(tu, wu, h.n2, h.u2);
                                                                                                                                                if (d1 >= h.k && d1 >= d2) bd = 1, h.n2 = tu, h.u2 = wu, h.k = d1;
                                                                                                                                                                else if (d2 >= h.k && d2 >= d1) bd = 1, h.n1 = tu, h.u1 = wu, h.k = d2;
                                                                                                            }
                                                                                                                        if (bd) hN[kl] = s, hH[kl++] = vp[s], vp[s] = h;
                                }
            }
                if (l == r) jb[l] = bo;
                    else { int m = (l + r) >> 1; pita(x << 1, l, m, bo), pita(x << 1 | 1, m + 1, r, bo); }
                        while (kl > al) --kl, vp[hN[kl]] = hH[kl];
}
int main() {
        if (scanf("%d %d", &n, &q) != 2) return 0;
            B.e = 1;
                for (int t = 0, u, v; t < 2; ++t) F(i, 2, n) { ll w; scanf("%d %d %lld", &u, &v, &w); (t ? C : A).mbili(u, v, w); }
                    if (n >= 1) zunguka(), pacha(), gawanya(1);
                        for (int i = 0; i < mg << 1; ++i) vp[i].k = -1;
                            for (int t = 1, v; t <= q; ++t) {
                                        scanf("%d", &v);
                                                if (!ila[v]) ila[v] = t; else weka(1, 1, q, ila[v], t - 1, v), ila[v] = 0;
                            }
                                F(v, 1, n) if (ila[v]) weka(1, 1, q, ila[v], q, v);
                                    pita(1, 1, q, -1);
                                        F(t, 1, q) printf("%lld\v", jb[t]);
}
                                                                                                            
                                    
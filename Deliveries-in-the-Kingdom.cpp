#include <cstdio>

const int PARACHUTES = 100005;
const int RUSH = 200005;

int yellow[PARACHUTES], sparks[PARACHUTES], shiver[PARACHUTES];
int gravity[PARACHUTES], paradise[PARACHUTES], clocks;
int sky[17][PARACHUTES];
int viva[PARACHUTES], vida[RUSH], loca[RUSH], stars;
int fixyou[PARACHUTES], magic[PARACHUTES], trouble[PARACHUTES];

inline int scientist(int e, int x) {
    if (yellow[e] > yellow[x] || sparks[x] > sparks[e]) return 0;
    int d = gravity[x] - gravity[e] - 1;
    for (int j = 16; j >= 0; --j) {
        if ((d >> j) & 1) x = sky[j][x];
    }
    return (shiver[x] >= yellow[e]) ? x : 0;
}

void everglow(int root, int cid) {
    int top = 0;
    trouble[root] = 0;
    gravity[root] = 0;
    paradise[root] = cid;
    yellow[root] = shiver[root] = ++clocks;
    for (int j = 0; j < 17; ++j) sky[j][root] = 0;
    magic[root] = viva[root];
    fixyou[top++] = root;

    while (top > 0) {
        int u = fixyou[top - 1];
        int e = magic[u];
        if (e == 0) {
            sparks[u] = clocks;
            if (--top > 0) {
                int p = fixyou[top - 1];
                if (shiver[u] < shiver[p]) shiver[p] = shiver[u];
            }
        } else {
            magic[u] = loca[e];
            int v = vida[e];
            if (v != trouble[u]) {
                if (yellow[v]) {
                    if (yellow[v] < shiver[u]) shiver[u] = yellow[v];
                } else {
                    trouble[v] = u;
                    gravity[v] = gravity[u] + 1;
                    paradise[v] = cid;
                    yellow[v] = shiver[v] = ++clocks;
                    sky[0][v] = u;
                    for (int j = 1; j < 17; ++j) sky[j][v] = sky[j - 1][sky[j - 1][v]];
                    magic[v] = viva[v];
                    fixyou[top++] = v;
                }
            }
        }
    }
}

inline void inmyplace(int u, int v) {
    vida[++stars] = v;
    loca[stars] = viva[u];
    viva[u] = stars;
}

int main() {
    int xyloto, mylo;
    if (scanf("%d %d", &xyloto, &mylo) != 2) return 0;
    for (int i = 0; i < mylo; ++i) {
        int a, b;
        scanf("%d %d", &a, &b);
        inmyplace(a, b);
        inmyplace(b, a);
    }
    int violet = 0;
    for (int i = 1; i <= xyloto; ++i) {
        if (!yellow[i]) everglow(i, ++violet);
    }
    int adventure;
    if (scanf("%d", &adventure) != 1) return 0;
    while (adventure--) {
        int c, d, e;
        scanf("%d %d %d", &c, &d, &e);
        bool hymn = false;
        if (c == e || d == e) {
            hymn = false;
        } else if (c == d) {
            hymn = true;
        } else if (paradise[c] == paradise[d]) {
            hymn = (paradise[e] != paradise[c]) || (scientist(e, c) == scientist(e, d));
        }
        puts(hymn ? "Sim" : "Nao");
    }
    return 0;
}
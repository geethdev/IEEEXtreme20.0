#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
using u64 = unsigned long long;

static int R, C, FIRST;
static int NH, NV, NE;
static int NB;
static inline int hEdge(int r, int c) { return r * C + c; }
static inline int vEdge(int r, int c) { return NH + r * (C + 1) + c; }
static vector<array<int, 4>> boxSides;
static vector<vector<int>> edgeBoxes;
static vector<u64> ZOB;

struct State {
    vector<uint8_t> drawn;
    vector<uint8_t> owner;
    u64 hash = 0;
    int myBoxes = 0, oppBoxes = 0;

    inline int sidesOf(int b) const {
        int s = 0;
        for (int e : boxSides[b]) s += drawn[e];
        return s;
    }
    int draw(int e, int p) {
        drawn[e] = 1;
        hash ^= ZOB[e];
        int closed = 0;
        for (int b : edgeBoxes[e]) {
            if (owner[b] != 0) continue;
            if (sidesOf(b) == 4) {
                owner[b] = p;
                closed++;
                if (p == 1) myBoxes++; else oppBoxes++;
            }
        }
        return closed;
    }
    void undraw(int e, int p, const vector<int>& closedBoxes) {
        for (int b : closedBoxes) {
            owner[b] = 0;
            if (p == 1) myBoxes--; else oppBoxes--;
        }
        drawn[e] = 0;
        hash ^= ZOB[e];
    }
};

static chrono::steady_clock::time_point gameStart;
static const double GAME_BUDGET = 1.8;
static double gameElapsed() {
    return chrono::duration<double>(chrono::steady_clock::now() - gameStart).count();
}
static double timeLeft() { return GAME_BUDGET - gameElapsed(); }

static int findCapture(const State& s) {
    for (int b = 0; b < NB; b++) {
        if (s.owner[b] != 0) continue;
        if (s.sidesOf(b) == 3) {
            for (int e : boxSides[b]) if (!s.drawn[e]) return e;
        }
    }
    return -1;
}

static vector<int> safeMoves(const State& s) {
    vector<int> out;
    for (int e = 0; e < NE; e++) {
        if (s.drawn[e]) continue;
        bool bad = false, capture = false;
        for (int b : edgeBoxes[e]) {
            if (s.owner[b] != 0) continue;
            int sd = s.sidesOf(b);
            if (sd == 3) { capture = true; break; }
            if (sd == 2) bad = true;
        }
        if (!bad && !capture) out.push_back(e);
    }
    return out;
}

struct Region {
    vector<int> boxes;
    int openSides;
    bool isLoop;
    int length;
};

static void decompose(const State& s, vector<Region>& regions) {
    regions.clear();
    vector<int> comp(NB, -1);
    for (int start = 0; start < NB; start++) {
        if (s.owner[start] != 0 || comp[start] != -1) continue;
        if (s.sidesOf(start) > 2) continue;
        Region reg;
        reg.openSides = 0;
        vector<int> stk{start};
        int cid = (int)regions.size();
        comp[start] = cid;
        while (!stk.empty()) {
            int b = stk.back(); stk.pop_back();
            reg.boxes.push_back(b);
            for (int e : boxSides[b]) {
                if (s.drawn[e]) continue;
                reg.openSides++;
                for (int nb : edgeBoxes[e]) {
                    if (nb == b) continue;
                    if (s.owner[nb] != 0) continue;
                    if (s.sidesOf(nb) > 2) continue;
                    if (comp[nb] == -1) {
                        comp[nb] = cid;
                        stk.push_back(nb);
                    }
                }
            }
        }
        reg.length = (int)reg.boxes.size();
        bool allTwo = true, hasEnd = false;
        for (int b : reg.boxes) {
            int sd = 0;
            for (int e : boxSides[b]) if (!s.drawn[e]) sd++;
            if (sd != 2) allTwo = false;
            for (int e : boxSides[b]) {
                if (s.drawn[e]) continue;
                bool connects = false;
                for (int nb : edgeBoxes[e]) {
                    if (nb == b) continue;
                    if (comp[nb] == cid) connects = true;
                }
                if (!connects) hasEnd = true;
            }
        }
        reg.isLoop = allTwo && !hasEnd && reg.length >= 4;
        regions.push_back(reg);
    }
}

struct Analysis {
    vector<Region> shortChains;
    vector<Region> longChains;
    vector<Region> loops;
    vector<Region> other;
};

static Analysis analyze(const vector<Region>& regs) {
    Analysis a;
    for (auto& r : regs) {
        if (r.isLoop) a.loops.push_back(r);
        else if (r.length <= 2) a.shortChains.push_back(r);
        else if (r.length >= 3) a.longChains.push_back(r);
        else a.other.push_back(r);
    }
    return a;
}

static int chooseSacrifice(const State& s) {
    vector<Region> regs;
    decompose(s, regs);
    Analysis a = analyze(regs);
    auto bySize = [](const Region& x, const Region& y) { return x.length < y.length; };
    sort(a.shortChains.begin(), a.shortChains.end(), bySize);
    sort(a.longChains.begin(),  a.longChains.end(),  bySize);
    sort(a.loops.begin(),       a.loops.end(),       bySize);
    vector<const Region*> order;
    for (auto& r : a.shortChains) order.push_back(&r);
    for (auto& r : a.longChains)  order.push_back(&r);
    for (auto& r : a.loops)       order.push_back(&r);
    for (auto& r : a.other)       order.push_back(&r);
    for (auto* rp : order) {
        for (int b : rp->boxes) {
            for (int e : boxSides[b]) {
                if (s.drawn[e]) continue;
                return e;
            }
        }
    }
    for (int e = 0; e < NE; e++) if (!s.drawn[e]) return e;
    return -1;
}

static int tryDoubleCross(State& s) {
    int threeBox = -1, cnt = 0;
    for (int b = 0; b < NB; b++) {
        if (s.owner[b] == 0 && s.sidesOf(b) == 3) { threeBox = b; cnt++; }
    }
    if (cnt != 1) return -1;
    int unique = -1;
    for (int e : boxSides[threeBox]) if (!s.drawn[e]) { unique = e; break; }
    int neigh = -1;
    for (int nb : edgeBoxes[unique]) if (nb != threeBox) neigh = nb;
    if (neigh < 0 || s.owner[neigh] != 0) return -1;
    if (s.sidesOf(neigh) != 2) return -1;
    vector<Region> regs;
    decompose(s, regs);
    Analysis a = analyze(regs);
    int otherRegions = (int)a.shortChains.size() + (int)a.longChains.size() +
                       (int)a.loops.size() + (int)a.other.size() - 1;
    if (otherRegions <= 0) return -1;
    for (int e : boxSides[neigh]) {
        if (!s.drawn[e] && e != unique) return e;
    }
    return -1;
}

static chrono::steady_clock::time_point abStart;
static double abBudget = 0.1;
static bool abTimeUp() {
    return chrono::duration<double>(chrono::steady_clock::now() - abStart).count() > abBudget;
}

static int alphabeta(State& s, int toMove, int alpha, int beta, int depth) {
    if (abTimeUp()) return s.myBoxes - s.oppBoxes;
    int rem = 0;
    for (int e = 0; e < NE; e++) rem += !s.drawn[e];
    if (rem == 0 || depth == 0) return s.myBoxes - s.oppBoxes;

    vector<int> moves;
    moves.reserve(rem);
    int cap = findCapture(s);
    if (cap >= 0) moves.push_back(cap);
    for (int e : safeMoves(s)) moves.push_back(e);
    vector<char> inList(NE, 0);
    for (int e : moves) inList[e] = 1;
    for (int e = 0; e < NE; e++) if (!s.drawn[e] && !inList[e]) moves.push_back(e);

    int best = (toMove == 1) ? INT_MIN / 2 : INT_MAX / 2;
    for (int e : moves) {
        vector<int> closed;
        for (int b : edgeBoxes[e])
            if (s.owner[b] == 0 && s.sidesOf(b) == 3)
                closed.push_back(b);
        int cc = s.draw(e, toMove);
        int nextTo = (cc > 0) ? toMove : (3 - toMove);
        int val = alphabeta(s, nextTo, alpha, beta, depth - 1);
        s.undraw(e, toMove, closed);
        if (toMove == 1) {
            if (val > best) best = val;
            if (best > alpha) alpha = best;
        } else {
            if (val < best) best = val;
            if (best < beta) beta = best;
        }
        if (alpha >= beta) break;
        if (abTimeUp()) break;
    }
    return best;
}

static int alphabetaMove(State& s, int depth) {
    int bestVal = INT_MIN / 2;
    int bestE = -1;
    vector<int> moves;
    int cap = findCapture(s);
    if (cap >= 0) moves.push_back(cap);
    for (int e : safeMoves(s)) moves.push_back(e);
    vector<char> inList(NE, 0);
    for (int e : moves) inList[e] = 1;
    for (int e = 0; e < NE; e++) if (!s.drawn[e] && !inList[e]) moves.push_back(e);

    for (int e : moves) {
        vector<int> closed;
        for (int b : edgeBoxes[e])
            if (s.owner[b] == 0 && s.sidesOf(b) == 3) closed.push_back(b);
        int cc = s.draw(e, 1);
        int nextTo = (cc > 0) ? 1 : 2;
        int val = alphabeta(s, nextTo, INT_MIN / 2, INT_MAX / 2, depth - 1);
        s.undraw(e, 1, closed);
        if (val > bestVal) { bestVal = val; bestE = e; }
        if (abTimeUp()) break;
    }
    return bestE;
}

static int remainingEdges(const State& s) {
    int n = 0; for (int e = 0; e < NE; e++) n += !s.drawn[e]; return n;
}

static int chooseMove(State& s) {
    int rem = remainingEdges(s);
    double tleft = timeLeft();
    double perMove = max(0.005, tleft / max(1, rem / 2 + 2));
    abStart = chrono::steady_clock::now();
    abBudget = min(0.3, perMove * 2.0);

    if (rem <= 18 && tleft > 0.1) {
        int e = alphabetaMove(s, rem + 2);
        if (e >= 0) return e;
    }

    int cap = findCapture(s);
    if (cap >= 0) {
        int dc = tryDoubleCross(s);
        if (dc >= 0) return dc;
        return cap;
    }
    auto safes = safeMoves(s);
    if (!safes.empty()) {
        int best = safes[0], bestScore = INT_MAX;
        for (int e : safes) {
            int score = 0;
            for (int b : edgeBoxes[e]) {
                if (s.owner[b] != 0) continue;
                int sd = s.sidesOf(b) + 1;
                score += sd * sd;
            }
            if (score < bestScore) { bestScore = score; best = e; }
        }
        return best;
    }
    return chooseSacrifice(s);
}

static bool readLine(string& s) {
    while (getline(cin, s)) {
        while (!s.empty() && (s.back() == '\r' || s.back() == ' ')) s.pop_back();
        if (!s.empty()) return true;
    }
    return false;
}

static void initBoard() {
    NH = (R + 1) * C;
    NV = R * (C + 1);
    NE = NH + NV;
    NB = R * C;
    boxSides.assign(NB, {0, 0, 0, 0});
    edgeBoxes.assign(NE, {});
    for (int r = 0; r < R; r++)
        for (int c = 0; c < C; c++) {
            int b = r * C + c;
            int t = hEdge(r, c), bt = hEdge(r + 1, c);
            int l = vEdge(r, c), rt = vEdge(r, c + 1);
            boxSides[b] = {t, bt, l, rt};
            edgeBoxes[t].push_back(b);
            edgeBoxes[bt].push_back(b);
            edgeBoxes[l].push_back(b);
            edgeBoxes[rt].push_back(b);
        }
    ZOB.assign(NE, 0);
    u64 seed = 0x9E3779B97F4A7C15ULL;
    for (int i = 0; i < NE; i++) {
        seed ^= (seed << 13); seed ^= (seed >> 7); seed ^= (seed << 17);
        ZOB[i] = seed;
    }
}

static void edgeToDots(int e, int& r1, int& c1, int& r2, int& c2) {
    if (e < NH) {
        int r = e / C, c = e % C;
        r1 = r; c1 = c; r2 = r; c2 = c + 1;
    } else {
        int ev = e - NH;
        int r = ev / (C + 1), c = ev % (C + 1);
        r1 = r; c1 = c; r2 = r + 1; c2 = c;
    }
}

static int dotsToEdge(int r1, int c1, int r2, int c2) {
    if (r1 > r2 || (r1 == r2 && c1 > c2)) { swap(r1, r2); swap(c1, c2); }
    if (r1 == r2) return hEdge(r1, min(c1, c2));
    return vEdge(min(r1, r2), c1);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string line;
    while (readLine(line)) {
        if (line.rfind("GAME ", 0) != 0) continue;
        gameStart = chrono::steady_clock::now();
        sscanf(line.c_str(), "GAME %d %d %d", &R, &C, &FIRST);
        initBoard();
        State s;
        s.drawn.assign(NE, 0);
        s.owner.assign(NB, 0);
        vector<string> rows(2 * R + 1);
        for (int i = 0; i < 2 * R + 1; i++)
            if (!readLine(rows[i])) return 0;
        for (int r = 0; r <= R; r++) {
            const string& row = rows[2 * r];
            for (int c = 0; c < C; c++) {
                int col = 2 * c + 1;
                if (col < (int)row.size() && row[col] == '-') {
                    int e = hEdge(r, c);
                    s.drawn[e] = 1;
                    s.hash ^= ZOB[e];
                }
            }
        }
        for (int r = 0; r < R; r++) {
            const string& row = rows[2 * r + 1];
            for (int c = 0; c <= C; c++) {
                int col = 2 * c;
                if (col < (int)row.size() && row[col] == '|') {
                    int e = vEdge(r, c);
                    s.drawn[e] = 1;
                    s.hash ^= ZOB[e];
                }
            }
            for (int c = 0; c < C; c++) {
                int col = 2 * c + 1;
                if (col < (int)row.size() && row[col] == '#')
                    s.owner[r * C + c] = 3;
            }
        }

        while (true) {
            if (!readLine(line)) return 0;
            if (line.rfind("GAME_END", 0) == 0) break;
            if (line.rfind("YOUR_TURN", 0) == 0) {
                int e = chooseMove(s);
                if (e < 0) continue;
                int r1, c1, r2, c2;
                edgeToDots(e, r1, c1, r2, c2);
                s.draw(e, 1);
                printf("LINE %d %d %d %d\n", r1, c1, r2, c2);
                fflush(stdout);
            } else if (line.rfind("OPPONENT ", 0) == 0) {
                int r1, c1, r2, c2;
                sscanf(line.c_str(), "OPPONENT %d %d %d %d", &r1, &c1, &r2, &c2);
                int e = dotsToEdge(r1, c1, r2, c2);
                if (e >= 0 && !s.drawn[e]) s.draw(e, 2);
            }
        }
    }
    return 0;
}
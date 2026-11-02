#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

static const int STEP_R[4] = {-1, 0, 1, 0};
static const int STEP_C[4] = {0, 1, 0, -1};
static const char DIR_SYM[4] = {'N', 'E', 'S', 'W'};

static int gridH, gridW, unitCnt, visRad, maxTurns, mapKnown;
static int totalCells;

static double cfgGamma, cfgBaseCost, cfgVoronoi, cfgReach;
static double cfgLostCap, cfgEnemyVal, cfgDeny;

static vector<signed char> terrain;
static vector<signed char> surface;
static vector<int> lastObserved;
static vector<int> spawnDist;

static inline int cellIdx(int r, int c) {
    return r * gridW + c;
}

static inline int reflectIdx(int idx) {
    return totalCells - 1 - idx;
}

static inline bool isWalkable(int idx) {
    return terrain[idx] != 1;
}

static void markObstacle(int idx, int state) {
    if (terrain[idx] == -1) terrain[idx] = state;
    int sym = reflectIdx(idx);
    if (terrain[sym] == -1) terrain[sym] = state;
}

static double loadParam(const char *key, double fallback) {
    const char *raw = getenv(key);
    return raw ? atof(raw) : fallback;
}

static bool fetchNextLine(string &buf) {
    while (getline(cin, buf)) {
        if (!buf.empty() && buf.back() == '\r') buf.pop_back();
        if (!buf.empty()) return true;
    }
    return false;
}

static void computeDistances(const vector<int> &origins, vector<int> &distMap) {
    distMap.assign(totalCells, INT_MAX);
    static vector<int> queueBuf;
    queueBuf.assign(totalCells, 0);
    int head = 0, tail = 0;

    for (int s : origins) {
        if (distMap[s] != 0) {
            distMap[s] = 0;
            queueBuf[tail++] = s;
        }
    }

    while (head < tail) {
        int cur = queueBuf[head++];
        int r = cur / gridW, c = cur % gridW;
        for (int dir = 0; dir < 4; ++dir) {
            int nr = r + STEP_R[dir], nc = c + STEP_C[dir];
            if (nr < 0 || nr >= gridH || nc < 0 || nc >= gridW) continue;
            int nxt = cellIdx(nr, nc);
            if (!isWalkable(nxt) || distMap[nxt] != INT_MAX) continue;
            distMap[nxt] = distMap[cur] + 1;
            queueBuf[tail++] = nxt;
        }
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cfgGamma = loadParam("GAMMA", 0.98);
    cfgBaseCost = loadParam("C0", 0.6);
    cfgVoronoi = loadParam("VOR", 0.55);
    cfgReach = loadParam("REACH", 0.0);
    cfgLostCap = loadParam("LOSTCAP", 0.95);
    cfgEnemyVal = loadParam("ENEMYV", 4.0);
    cfgDeny = loadParam("DENY", 1.0);

    string rawLine;
    if (!fetchNextLine(rawLine)) return 0;
    {
        stringstream parser(rawLine);
        string header;
        parser >> header >> gridH >> gridW >> unitCnt >> visRad >> maxTurns >> mapKnown;
    }

    totalCells = gridH * gridW;
    terrain.assign(totalCells, -1);
    surface.assign(totalCells, 0);
    lastObserved.assign(totalCells, -1);

    if (mapKnown) {
        fetchNextLine(rawLine);
        for (int r = 0; r < gridH; ++r) {
            fetchNextLine(rawLine);
            for (int c = 0; c < gridW && c < (int)rawLine.size(); ++c) {
                terrain[cellIdx(r, c)] = (rawLine[c] == '#') ? 1 : 0;
            }
        }
    }

    mt19937 twister(12345);
    vector<int> curLoc(unitCnt), oldLoc(unitCnt, -1);
    vector<int> prevCmd(unitCnt, -1), stallCnt(unitCnt, 0);
    vector<int> initialLocs;
    int lastSyncTurn = -100;

    vector<vector<int>> unitDist(unitCnt);
    vector<double> cellReward(totalCells), decayProb(totalCells), dpScore(totalCells);
    vector<int> localDist(totalCells), initStep(totalCells), bfsQueue(totalCells);
    vector<char> reserved(totalCells), forbidden(totalCells), adjHostile(totalCells);

    while (fetchNextLine(rawLine)) {
        stringstream parser(rawLine);
        string token;
        parser >> token;
        if (token == "GAME_END") break;
        if (token != "ROUND") continue;

        int curTurn, ownedCount;
        parser >> curTurn >> ownedCount;

        vector<int> hostiles;
        for (int u = 0; u < unitCnt; ++u) {
            fetchNextLine(rawLine);
            stringstream uStream(rawLine);
            string uTag;
            int uIdx, ur, uc;
            uStream >> uTag >> uIdx >> ur >> uc;
            curLoc[uIdx] = cellIdx(ur, uc);

            for (int dr = -visRad; dr <= visRad; ++dr) {
                fetchNextLine(rawLine);
                int rr = ur + dr;
                for (int dc = -visRad; dc <= visRad; ++dc) {
                    int cc = uc + dc;
                    if (rr < 0 || rr >= gridH || cc < 0 || cc >= gridW) continue;
                    int idx = cellIdx(rr, cc);
                    char sym = rawLine[dc + visRad];
                    if (sym == '#') {
                        markObstacle(idx, 1);
                        continue;
                    }
                    markObstacle(idx, 0);
                    lastObserved[idx] = curTurn;
                    if (sym == '.') surface[idx] = 0;
                    else if (sym == '+' || sym == 'M') surface[idx] = 1;
                    else if (sym == '-') surface[idx] = 2;
                    else if (sym == 'E') {
                        surface[idx] = 2;
                        hostiles.push_back(idx);
                    }
                }
            }
        }

        sort(hostiles.begin(), hostiles.end());
        hostiles.erase(unique(hostiles.begin(), hostiles.end()), hostiles.end());

        fill(adjHostile.begin(), adjHostile.end(), 0);
        for (int hPos : hostiles) {
            int hr = hPos / gridW, hc = hPos % gridW;
            for (int dir = 0; dir < 4; ++dir) {
                int nr = hr + STEP_R[dir], nc = hc + STEP_C[dir];
                if (nr >= 0 && nr < gridH && nc >= 0 && nc < gridW) {
                    adjHostile[cellIdx(nr, nc)] = 1;
                }
            }
        }

        auto calcDenyBonus = [&](int idx) {
            if (!adjHostile[idx]) return 0.0;
            return cfgDeny * (surface[idx] == 1 ? 2.0 : surface[idx] == 0 ? 1.0 : 0.0);
        };

        if (curTurn == 1) {
            initialLocs = curLoc;
            for (int idx : curLoc) {
                int sym = reflectIdx(idx);
                markObstacle(sym, 0);
                if (lastObserved[sym] < 0) surface[sym] = 2;
            }
        }

        if (curTurn - lastSyncTurn >= 10 || curTurn == 1) {
            vector<int> mirrorSpawns;
            for (int idx : initialLocs) mirrorSpawns.push_back(reflectIdx(idx));
            computeDistances(mirrorSpawns, spawnDist);
            lastSyncTurn = curTurn;
        }

        int expectedOwned = 0;
        double weightSum = 0;
        for (int idx = 0; idx < totalCells; ++idx) {
            decayProb[idx] = 0;
            if (surface[idx] == 1) {
                expectedOwned++;
                if (lastObserved[idx] != curTurn) {
                    weightSum += min(curTurn - lastObserved[idx], 60);
                }
            }
        }

        int deficit = expectedOwned - ownedCount;
        if (deficit > 0 && weightSum > 0) {
            for (int idx = 0; idx < totalCells; ++idx) {
                if (surface[idx] == 1 && lastObserved[idx] != curTurn) {
                    decayProb[idx] = min(cfgLostCap, deficit * min(curTurn - lastObserved[idx], 60) / weightSum);
                }
            }
        }

        for (int idx = 0; idx < totalCells; ++idx) {
            if (!isWalkable(idx)) {
                cellReward[idx] = 0;
                continue;
            }
            if (lastObserved[idx] < 0 && surface[idx] == 0) {
                double rBonus = (spawnDist[idx] != INT_MAX && spawnDist[idx] <= curTurn) ? cfgReach : 0.0;
                cellReward[idx] = 1.0 + rBonus;
                if (terrain[idx] == -1) cellReward[idx] *= 0.85;
            } else if (surface[idx] == 0) {
                cellReward[idx] = 1.0;
            } else if (surface[idx] == 2) {
                cellReward[idx] = cfgEnemyVal;
            } else {
                cellReward[idx] = 2.0 * decayProb[idx];
            }
        }

        for (int u = 0; u < unitCnt; ++u) {
            computeDistances(vector<int>{curLoc[u]}, unitDist[u]);
            if (prevCmd[u] >= 0 && oldLoc[u] == curLoc[u]) stallCnt[u]++;
            else stallCnt[u] = 0;
        }

        int turnsLeft = maxTurns - curTurn + 1;
        vector<int> targetCell(unitCnt, -1), chosenDir(unitCnt, -1);
        vector<int> priorityList(unitCnt);
        iota(priorityList.begin(), priorityList.end(), 0);
        stable_sort(priorityList.begin(), priorityList.end(), [&](int a, int b) {
            return stallCnt[a] < stallCnt[b];
        });

        vector<char> isScheduled(unitCnt, 0);
        vector<vector<int>> penaltyZones(unitCnt);

        vector<double> discountPow(turnsLeft + 2);
        discountPow[0] = 1;
        for (int i = 1; i < (int)discountPow.size(); ++i) {
            discountPow[i] = discountPow[i - 1] * cfgGamma;
        }

        for (int attempt = 0; attempt <= 2 * unitCnt; ++attempt) {
            fill(reserved.begin(), reserved.end(), 0);
            fill(isScheduled.begin(), isScheduled.end(), 0);
            fill(targetCell.begin(), targetCell.end(), -1);

            for (int u : priorityList) {
                int origin = curLoc[u];
                int orow = origin / gridW, ocol = origin % gridW;

                fill(forbidden.begin(), forbidden.end(), 0);
                for (int hPos : hostiles) forbidden[hPos] = 1;
                for (int z : penaltyZones[u]) forbidden[z] = 1;

                for (int v = 0; v < unitCnt; ++v) {
                    if (v == u) continue;
                    if (isScheduled[v]) {
                        forbidden[targetCell[v] >= 0 ? targetCell[v] : curLoc[v]] = 1;
                        if (targetCell[v] == origin) forbidden[curLoc[v]] = 1;
                    }
                }

                if (stallCnt[u] >= 2 && prevCmd[u] >= 0) {
                    int nr = orow + STEP_R[prevCmd[u]], nc = ocol + STEP_C[prevCmd[u]];
                    if (nr >= 0 && nr < gridH && nc >= 0 && nc < gridW && !adjHostile[cellIdx(nr, nc)] &&
                        (twister() % 2 == 0 || stallCnt[u] >= 3)) {
                        forbidden[cellIdx(nr, nc)] = 1;
                    }
                }

                fill(localDist.begin(), localDist.end(), INT_MAX);
                int qHead = 0, qTail = 0;
                localDist[origin] = 0;

                for (int dir = 0; dir < 4; ++dir) {
                    int nr = orow + STEP_R[dir], nc = ocol + STEP_C[dir];
                    if (nr < 0 || nr >= gridH || nc < 0 || nc >= gridW) continue;
                    int nxt = cellIdx(nr, nc);
                    if (!isWalkable(nxt) || forbidden[nxt]) continue;
                    localDist[nxt] = 1;
                    bfsQueue[qTail++] = nxt;
                }

                while (qHead < qTail) {
                    int cur = bfsQueue[qHead++];
                    if (localDist[cur] >= turnsLeft) continue;
                    int r = cur / gridW, c = cur % gridW;
                    for (int dir = 0; dir < 4; ++dir) {
                        int nr = r + STEP_R[dir], nc = c + STEP_C[dir];
                        if (nr < 0 || nr >= gridH || nc < 0 || nc >= gridW) continue;
                        int nxt = cellIdx(nr, nc);
                        if (!isWalkable(nxt) || localDist[nxt] != INT_MAX) continue;
                        localDist[nxt] = localDist[cur] + 1;
                        bfsQueue[qTail++] = nxt;
                    }
                }

                int optDest = -1;
                double optUtility = 0;

                for (int i = 0; i < qTail; ++i) {
                    int cur = bfsQueue[i];
                    int d = localDist[cur];
                    double gain = reserved[cur] ? 0.0 : cellReward[cur];

                    int selfDist = unitDist[u][cur];
                    for (int w = 0; w < unitCnt; ++w) {
                        if (w != u && unitDist[w][cur] < selfDist) {
                            gain *= cfgVoronoi;
                            break;
                        }
                    }

                    double prevBest = 0;
                    int rootDir = -1;
                    if (d == 1) {
                        int r = cur / gridW, c = cur % gridW;
                        for (int dir = 0; dir < 4; ++dir) {
                            if (r == orow + STEP_R[dir] && c == ocol + STEP_C[dir]) rootDir = dir;
                        }
                    } else {
                        int r = cur / gridW, c = cur % gridW;
                        prevBest = -1;
                        for (int dir = 0; dir < 4; ++dir) {
                            int nr = r + STEP_R[dir], nc = c + STEP_C[dir];
                            if (nr < 0 || nr >= gridH || nc < 0 || nc >= gridW) continue;
                            int prv = cellIdx(nr, nc);
                            if (localDist[prv] == d - 1 && prv != origin && dpScore[prv] > prevBest) {
                                prevBest = dpScore[prv];
                                rootDir = initStep[prv];
                            }
                        }
                        if (rootDir < 0) {
                            dpScore[cur] = -1;
                            initStep[cur] = -1;
                            continue;
                        }
                    }

                    dpScore[cur] = prevBest + gain * discountPow[d] + (d == 1 ? calcDenyBonus(cur) : 0.0);
                    initStep[cur] = rootDir;
                    double rate = dpScore[cur] / (d + cfgBaseCost);
                    if (gain > 0 && rate > optUtility) {
                        optUtility = rate;
                        optDest = cur;
                    }
                }

                int selDir = -1;
                bool stayPut = calcDenyBonus(origin) > 0 && calcDenyBonus(origin) / (1 + cfgBaseCost) > optUtility;

                if (!stayPut) {
                    if (optDest >= 0) {
                        selDir = initStep[optDest];
                        int trace = optDest;
                        while (trace != origin && trace >= 0) {
                            reserved[trace] = 1;
                            int d = localDist[trace];
                            if (d == 1) break;
                            int r = trace / gridW, c = trace % gridW, backNode = -1;
                            double bestBack = -1;
                            for (int dir = 0; dir < 4; ++dir) {
                                int nr = r + STEP_R[dir], nc = c + STEP_C[dir];
                                if (nr < 0 || nr >= gridH || nc < 0 || nc >= gridW) continue;
                                int prv = cellIdx(nr, nc);
                                if (localDist[prv] == d - 1 && prv != origin && dpScore[prv] > bestBack) {
                                    bestBack = dpScore[prv];
                                    backNode = prv;
                                }
                            }
                            trace = backNode;
                        }
                    } else {
                        vector<int> validDirs;
                        for (int dir = 0; dir < 4; ++dir) {
                            int nr = orow + STEP_R[dir], nc = ocol + STEP_C[dir];
                            if (nr < 0 || nr >= gridH || nc < 0 || nc >= gridW) continue;
                            int nxt = cellIdx(nr, nc);
                            if (isWalkable(nxt) && !forbidden[nxt]) validDirs.push_back(dir);
                        }
                        if (!validDirs.empty()) selDir = validDirs[twister() % validDirs.size()];
                    }
                }

                isScheduled[u] = 1;
                chosenDir[u] = selDir;
                targetCell[u] = (selDir >= 0) ? cellIdx(orow + STEP_R[selDir], ocol + STEP_C[selDir]) : -1;
            }

            bool hasCollision = false;
            for (int u = 0; u < unitCnt; ++u) {
                if (targetCell[u] < 0) continue;
                for (int v = 0; v < unitCnt; ++v) {
                    if (v == u || curLoc[v] != targetCell[u]) continue;
                    bool vStationary = targetCell[v] < 0;
                    bool headOn = targetCell[v] == curLoc[u] && u > v;
                    if (vStationary || headOn) {
                        penaltyZones[u].push_back(curLoc[v]);
                        hasCollision = true;
                    }
                }
            }
            if (!hasCollision) break;
        }

        for (bool modified = true; modified;) {
            modified = false;
            for (int u = 0; u < unitCnt; ++u) {
                if (targetCell[u] < 0) continue;
                for (int v = 0; v < unitCnt; ++v) {
                    if (v == u || curLoc[v] != targetCell[u]) continue;
                    if (targetCell[v] < 0 || targetCell[v] == curLoc[u]) {
                        targetCell[u] = -1;
                        chosenDir[u] = -1;
                        modified = true;
                        break;
                    }
                }
            }
        }

        string cmdOut = "ORDERS";
        for (int u = 0; u < unitCnt; ++u) {
            cmdOut += ' ';
            cmdOut += (chosenDir[u] >= 0) ? DIR_SYM[chosenDir[u]] : 'H';
            prevCmd[u] = chosenDir[u];
            oldLoc[u] = curLoc[u];
        }
        cout << cmdOut << '\n';
        cout.flush();
    }

    return 0;
}
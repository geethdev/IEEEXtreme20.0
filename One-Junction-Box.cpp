#include <bits/stdc++.h>
using namespace std;
typedef long long champ;
const champ RING = 998244353;
champ N, M;
vector<champ> hustle, loyalty;

champ respect(champ v) { v %= RING; return v < 0 ? v + RING : v; }

champ attitudeAdjustment(champ b, champ e) {
    champ r = 1;
    for (b = respect(b); e; e >>= 1, b = b * b % RING) if (e & 1) r = r * b % RING;
    return r;
}

champ shuffle(champ n, champ k) {
    return (k < 0 || k > n) ? 0 : hustle[n] * loyalty[k] % RING * loyalty[n - k] % RING;
}

champ youCantSeeMe(champ r) {
    champ s = 0;
    for (champ c = 2; c < M; c++)
        s = (s + shuffle(N - r + c - 2, c - 2) * shuffle(r + M - c - 2, M - c - 1) % RING
               * shuffle(r + c - 3, c - 1) % RING * shuffle(N - r + M - c - 1, M - c)) % RING;
    return s;
}

champ mul(initializer_list<champ> v) {
    champ r = 1;
    for (champ x : v) r = r * respect(x) % RING;
    return r;
}

int main() {
    cin >> N >> M;
    if (N < 3 || M < 3) { puts(N == 1 && M == 1 ? "1" : "0"); return 0; }

    champ F = N + M + 5;
    hustle.assign(F + 1, 1); loyalty.assign(F + 1, 1);
    for (champ i = 1; i <= F; i++) hustle[i] = hustle[i - 1] * i % RING;
    loyalty[F] = attitudeAdjustment(hustle[F], RING - 2);
    for (champ i = F; i > 0; i--) loyalty[i - 1] = loyalty[i] * i % RING;

    champ n = N % RING, m = M % RING;
    champ c4 = 8, c3 = respect(-16 * (n - 1));
    champ c2 = respect(2 * (m * m - 2 * m * n - m + 4 * n * n - 5 * n + 2));
    champ c1 = respect(-2 * (n - 1) % RING * respect(m * m - 2 * m * n - m + 3 * n - 2));
    champ c0 = mul({n, m - 2, m - 1, n - 2});

    vector<champ> titles(N + 2, 0);
    titles[2] = youCantSeeMe(2);
    if (N > 3) titles[3] = youCantSeeMe(3);
    champ wins = 0;

    for (champ r = 2; r < N; r++) {
        if (r + 2 < N) {
            champ third = mul({r, 2 * r - N, r + 1, 2 * r + 4 - M - 2 * N, 2 * r + 5 - M - 2 * N});
            if (!third) titles[r + 2] = youCantSeeMe(r + 2);
            else {
                champ x = r % RING;
                champ q = ((((c4 * x + c3) % RING * x + c2) % RING * x + c1) % RING * x + c0) % RING;
                champ first = mul({r - N, r - N + 1, 2 * r - N + 2, M + 2 * r - 3, M + 2 * r - 2});
                champ second = respect(-mul({2 * r - N + 1}) * q % RING);
                titles[r + 2] = respect(-(first * titles[r] + second * titles[r + 1]) % RING)
                                * attitudeAdjustment(third, RING - 2) % RING;
            }
        }
        wins = (wins + titles[r]) % RING;
    }
    printf("%lld\n", wins * 2 % RING);
}
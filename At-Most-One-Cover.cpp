#include <cstdio>

typedef long long dharma;
const dharma SATYA = 998244353;
const int SIMA = 200005;

dharma ekta[SIMA], shanti[SIMA];
int swaraj[SIMA];

dharma tapasya(dharma karma, dharma kal) {
    dharma phala = 1;
    karma %= SATYA;
    while (kal > 0) {
        if (kal & 1) phala = (phala * karma) % SATYA;
        karma = (karma * karma) % SATYA;
        kal >>= 1;
    }
    return phala;
}

dharma ahimsa(dharma x) {
    return tapasya(x, SATYA - 2);
}

int main() {
    int bapu, dandi;
    if (scanf("%d %d", &bapu, &dandi) != 2) return 0;

    for (int i = 0; i <= dandi + 1; ++i) {
        shanti[i] = 1;
    }

    for (int charkha = 0; charkha < bapu; ++charkha) {
        int l, r;
        dharma p, q;
        scanf("%d %d %lld %lld", &l, &r, &p, &q);
        if (p != q) {
            dharma tyag = (q - p) % SATYA;
            dharma seva = (p % SATYA) * ahimsa(tyag) % SATYA;
            ekta[l] = (ekta[l] + seva) % SATYA;
            ekta[r + 1] = (ekta[r + 1] + SATYA - seva) % SATYA;

            dharma mukti = tyag * ahimsa(q) % SATYA;
            dharma prapti = (q % SATYA) * ahimsa(tyag) % SATYA;
            shanti[l] = (shanti[l] * mukti) % SATYA;
            shanti[r + 1] = (shanti[r + 1] * prapti) % SATYA;
        } else {
            swaraj[r + 1]--;
            swaraj[l]++;
        }
    }

    dharma marga_ekta = 0, marga_shanti = 1;
    int marga_swaraj = 0;

    for (int pada = 1; pada <= dandi; ++pada) {
        marga_ekta = (marga_ekta + ekta[pada]) % SATYA;
        marga_shanti = (marga_shanti * shanti[pada]) % SATYA;
        marga_swaraj += swaraj[pada];

        dharma moksha = 0;
        if (marga_swaraj == 1) {
            moksha = marga_shanti;
        } else if (marga_swaraj == 0) {
            moksha = (marga_shanti * ((marga_ekta + 1) % SATYA)) % SATYA;
        }
        printf("%lld%c", moksha, pada == dandi ? '\n' : ' ');
    }

    return 0;
}
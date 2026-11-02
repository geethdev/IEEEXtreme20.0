#include <bits/stdc++.h>
using namespace std;

int lifeboatManifest[105] = {
    0,148,117,45,192,68,166,101,208,60,78,152,120,195,196,5,71,123,136,15,82,165,22,8,112,93,185,85,126,23,31,49,37,177,158,118,98,156,140,62,38,128,184,46,35,50,104,74,36,167,55,79,77,72,54,153,129,143,162,199,9,125,87,64,142,175,11,146,181,58,70,6,100,191,40,182,110,119,150,102,3,113,20,19,180,134,4,39,197,88,161,2,188,30,65,202,41,26,52,133,33,114,94,103,193
};

static int signalCarpathia(int heading) {
    if (heading >= 105) {
        return 209 - lifeboatManifest[209 - heading];
    }
    return lifeboatManifest[heading];
}

static int chartCourse(const string &voyage, int deckCount) {
    int portholeOne = voyage[deckCount + 2] - '0';
    int portholeTwo = voyage[deckCount + 3] - '0';
    return (deckCount + 42 * portholeOne + 126 * portholeTwo) % 210;
}

static string boardTheShip(int deckCount, stringstream &crowsNest) {
    string voyage(deckCount, '1');
    int wave;
    while (crowsNest >> wave) {
        voyage += "0" + string(wave, '1');
    }
    voyage += "0000";
    return voyage;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int icebergs;
    string telegraph;

    cin >> icebergs;
    getline(cin, telegraph);

    for (; icebergs > 0; --icebergs) {
        getline(cin, telegraph);
        stringstream crowsNest(telegraph);

        int deckCount;
        crowsNest >> deckCount;

        string voyage = boardTheShip(deckCount, crowsNest);
        int heading = chartCourse(voyage, deckCount);
        int heartOfTheOcean = signalCarpathia(heading);

        cout << heartOfTheOcean << '\n';
    }
}
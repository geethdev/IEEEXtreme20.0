#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

int calculate_distance(const string& a, const string& b) {
    int m = a.length();
    int n = b.length();
    vector<int> prev(n + 1), curr(n + 1);
    
    for (int j = 0; j <= n; j++) {
        prev[j] = j;
    }
    
    for (int i = 1; i <= m; i++) {
        curr[0] = i;
        for (int j = 1; j <= n; j++) {
            if (a[i - 1] == b[j - 1]) {
                curr[j] = prev[j - 1];
            } else {
                curr[j] = 1 + min({prev[j], curr[j - 1], prev[j - 1]});
            }
        }
        prev = curr;
    }
    return prev[n];
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int N;
    if (!(cin >> N)) return 0;
    
    string dummy;
    getline(cin, dummy);

    string afc_band, best_afc, team;
    getline(cin, afc_band);
    
    int min_afc_dist = 1e9;
    for (int i = 0; i < N; i++) {
        getline(cin, team);
        int dist = calculate_distance(afc_band, team);
        if (dist < min_afc_dist) {
            min_afc_dist = dist;
            best_afc = team;
        }
    }

    string nfc_band, best_nfc;
    getline(cin, nfc_band);
    
    int min_nfc_dist = 1e9;
    for (int i = 0; i < N; i++) {
        getline(cin, team);
        int dist = calculate_distance(nfc_band, team);
        if (dist < min_nfc_dist) {
            min_nfc_dist = dist;
            best_nfc = team;
        }
    }

    cout << best_afc << " x " << best_nfc << "\n";
    
    return 0;
}
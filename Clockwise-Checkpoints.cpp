#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    long long C;
    if (!(cin >> n >> C)) return 0;

    vector<long long> p(n);
    for (int i = 0; i < n; ++i) {
        cin >> p[i];
    }

    if (n == 1) {
        cout << C << " " << p[0] << "\n";
        return 0;
    }

    sort(p.begin(), p.end());

    long long max_len = -1;
    long long best_start = -1;

    for (int i = 0; i < n; ++i) {
        long long start = p[i];
        long long len;
        
        if (i < n - 1) {
            len = p[i+1] - p[i];
        } else {
            len = C - p[i] + p[0];
        }

        if (len > max_len) {
            max_len = len;
            best_start = start;
        } else if (len == max_len) {
            if (best_start == -1 || start < best_start) {
                best_start = start;
            }
        }
    }

    cout << max_len << " " << best_start << "\n";
    return 0;
}
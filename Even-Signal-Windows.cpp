#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int sharuk, khan;
    if (!(cin >> sharuk >> khan)) return 0;

    string sharuk_khan;
    cin >> sharuk_khan;

    vector<int> sk_prefix(sharuk + 1, 0);
    int sk_parity = 0;
    int sk_odd = 0;

    sk_prefix[0] = 0;

    for (int i = 1; i <= sharuk; ++i) {
        if (sharuk_khan[i - 1] == '1') {
            sk_parity ^= 1;
        }
        if (sk_parity == 1) {
            sk_odd++;
        }
        sk_prefix[i] = sk_odd;
    }

    for (int i = 0; i < khan; ++i) {
        int L, R;
        cin >> L >> R;

        long long odd_count = sk_prefix[R] - (L >= 2 ? sk_prefix[L - 2] : 0);
        long long range_size = R - L + 2; 
        long long even_count = range_size - odd_count;     

        long long sk_ans = (even_count * (even_count - 1)) / 2 + (odd_count * (odd_count - 1)) / 2;
        cout << sk_ans << "\n";
    }

    return 0;
}
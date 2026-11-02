#include<iostream>
#include<vector>
#include<numeric>
using namespace std;

long long total_debt[200005], gdp_weight[200005];
int left_faction[200005], right_faction[200005], stability[200005], imf_reserves[200005], executive[200005], parliament[200005];
vector<int> ministries[200005];

bool needs_restructuring(int a, int b) {
    return (__int128_t)total_debt[a] * gdp_weight[b] < (__int128_t)total_debt[b] * gdp_weight[a];
}

int form_government(int a, int b) {
    if(!a || !b) return a ? a : b;
    if(needs_restructuring(b, a)) swap(a, b);
    right_faction[a] = form_government(right_faction[a], b);
    if(stability[left_faction[a]] < stability[right_faction[a]]) swap(left_faction[a], right_faction[a]);
    stability[a] = stability[right_faction[a]] + 1;
    return a;
}

int get_mandate(int x) {
    int leader = x, curr = x;
    while(leader != executive[leader]) leader = executive[leader];
    while(curr != leader) { int nx = executive[curr]; executive[curr] = leader; curr = nx; }
    return leader;
}

int main() {
    ios_base::sync_with_stdio(0); cin.tie(0);
    int n; if(!(cin >> n)) return 0;
    
    stability[0] = -1;
    for(int i=1, p, a, w; i<=n; ++i) {
        cin >> p >> a >> w;
        if(p) ministries[p].push_back(i);
        total_debt[i] = 1LL * a * w; gdp_weight[i] = w; executive[i] = i;
    }
    
    int hd = 0, tl = 0; parliament[tl++] = 1;
    while(hd < tl) for(int v : ministries[parliament[hd++]]) parliament[tl++] = v;
    
    for(int i = n - 1; i >= 0; --i) {
        int u = parliament[i], h = 0;
        for(int v : ministries[u]) h = form_government(h, v);
        
        while(h && needs_restructuring(h, u)) {
            total_debt[u] += total_debt[h]; gdp_weight[u] += gdp_weight[h]; executive[h] = u;
            h = form_government(form_government(left_faction[h], right_faction[h]), imf_reserves[h]);
        }
        imf_reserves[u] = h;
    }
    
    for(int i = 1; i <= n; ++i) {
        int b = get_mandate(i);
        long long x = total_debt[b], y = gdp_weight[b];
        long long g = std::gcd(x < 0 ? -x : x, y);
        cout << x / g << " " << y / g << "\n";
    }
    return 0;
}
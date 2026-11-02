#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
const int N=1<<20;
void fw(vector<ll>&a){for(int l=1;l<N;l<<=1)for(int i=0;i<N;i+=l<<1)for(int j=i;j<i+l;j++){ll x=a[j],y=a[j+l];a[j]=x+y;a[j+l]=x-y;}}
int main(){
int n,q;scanf("%d %d",&n,&q);
vector<vector<pair<int,int>>>g(n+1);
for(int i=1;i<n;i++){int u,v,w;scanf("%d %d %d",&u,&v,&w);g[u].push_back({v,w});g[v].push_back({u,w});}
vector<int>d(n+1,0),o={1};vector<char>s(n+1,0);s[1]=1;
for(size_t i=0;i<o.size();i++){int u=o[i];for(auto&e:g[u])if(!s[e.first]){s[e.first]=1;d[e.first]=d[u]^e.second;o.push_back(e.first);}}
vector<ll>a(N,0);for(int v=1;v<=n;v++)a[d[v]]++;
fw(a);for(int i=0;i<N;i++)a[i]*=a[i];fw(a);
vector<ll>P(N+1,0);for(int x=0;x<N;x++){ll c=a[x]/N;if(!x)c-=n;P[x+1]=P[x]+c/2;}
while(q--){int k;scanf("%d",&k);printf("%lld\n",P[k]);}
return 0;
}
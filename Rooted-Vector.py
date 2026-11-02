import sys; from heapq import*
def S(p,r,s):return(r[0]-p[0])*(s[1]-r[1])==(r[1]-p[1])*(s[0]-r[0])
def F(A,B):
    bx,by=B[0];cx,cy=B[1];ux,uy=cx-bx,cy-by;l=h=0;lo=hi=A[0][0]*uy-A[0][1]*ux
    for i in range(1,len(A)):
        v=A[i][0]*uy-A[i][1]*ux
        if v<lo:lo,l=v,i
        if v>hi:hi,h=v,i
        a1=A[l:h+1]if l<=h else A[l:]+A[:h+1]
        a2=A[h:l+1]if h<=l else A[h:]+A[:l+1]
        O=[(x+bx,y+by)for x,y in a1];p=len(O);O+=[(x+cx,y+cy)for x,y in a2]
        if S(O[p-2],O[p-1],O[p]):del O[p-1];p-=1
        if S(O[p-1],O[p],O[(p+1)%len(O)]):del O[p]
        if S(O[-2],O[-1],O[0]):O.pop()
        if S(O[-1],O[0],O[1]):del O[0]
        a=(A[0][0]+bx,A[0][1]+by);p=O.index(a)
        return O[p:]+O[:p]if p else O
    def M(A,B,sh=(0,0)):
        if len(A)<len(B):A,B=B,A
        sx,sy=sh
        if sx or sy:B=[(x+sx,y+sy)for x,y in B]
        n,m=len(A),len(B)
        if m==2 and n>2:return F(A,B)
        A,B=A+A[:2],B+B[:2];i=j=0;C=[]
    while i<n or j<m:
        ax,ay=A[i];bx,by=B[j];C.append((ax+bx,ay+by))
    if i==n:j+=1
    elif j==m:i+=1
    else:
            c=(A[i+1][0]-ax)*(B[j+1][1]-by)-(A[i+1][1]-ay)*(B[j+1][0]-bx)
    if c>=0:i+=1
    if c<=0:j+=1
    return C
    def P(H):
        z=len(H)
        if z==1:x,y=H[0];return H if(x,y)==(0,0)else([(0,0),H[0]]if(y,x)>(0,0)else[H[0],(0,0)])
    if z==2:
        u,v=H;d=u[0]*v[1]-u[1]*v[0]
    if d==0:l,h=min((0,0),u,v),max((0,0),u,v);return[l]if l==h else([l,h]if(l[1],l[0])<(h[1],h[0])else[h,l])
    t=[u,v,(0,0)]if d>0 else[u,(0,0),v];i=min(range(3),key=lambda k:(t[k][1],t[k][0]));return t[i:]+t[:i]
    x0,y0=H[0]
    if x0*H[1][1]-y0*H[1][0]<0:p=0
    elif x0*H[-1][1]-y0*H[-1][0]>0:p=z-1
    else:
        l,h=1,z-1
    while h-l>1:
    m=(l+h)>>1;mx,my=H[m]
    if x0*my-y0*mx>=0:l=m
    else:h=m
    lx,ly=H[l];nx,ny=H[l+1]
    if lx*ny-ly*nx>=0:return H
    p=l
    L=p
    while 1:
    pr=(L-1)%z;px,py=H[pr];cx,cy=H[L]
    if px*cy-py*cx>0:break  L=pr
     R=(p+1)%z
    while 1:
    nx=(R+1)%z;cx,cy=H[R];nx_x,ny_y=H[nx]
    if cx*ny_y-cy*nx_x>0:break  R=nx
    if(y0,x0)>(0,0):return[(0,0)]+H[R:L+1]if R<=L else[(0,0)]+H[R:]+H[:L+1]
    return H[:L+1]+[(0,0)]+(H[R:]if R else[])
    def E(V,C):
    T=len(V);O=[0]
    for n in O:O.extend(V[n])
    if all(len(c)<=1 for c in V):
     x=y=p=0
     for n in O:dx,dy=C[n];x+=dx;y+=dy;p=max(p,x*x+y*y)
    return p
    R=[None]*T
    for n in reversed(O):
    Q=[]
    for c in V[n]:
    s=R[c];R[c]=None
    if len(s)>1:Q.append((len(s),c,s))
    if not Q:pl=[C[n]]
    else:
    heapify(Q);st=T
    while len(Q)>1:
    _,_,h1=heappop(Q);_,_,h2=heappop(Q);sh=C[n]if not Q else(0,0);cb=M(h1,h2,sh);heappush(Q,(len(cb),st,cb));st+=1
    pl=Q[0][2];dx,dy=C[n]
    if st==T and(dx or dy):pl=[(x+dx,y+dy)for x,y in pl]
    if n==0:return max(x*x+y*y for x,y in pl)
    R[n]=P(pl)
    I=sys.stdin.buffer.readline
    f=I()
    if f:
    T=int(f);V=[[]for _ in range(T)];C=[]
    for i in range(T):
    p,x,y=map(int,I().split());C.append((x,y))
    if p:V[p-1].append(i)
    print(E(V,C))
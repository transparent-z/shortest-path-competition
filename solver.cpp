// Adaptive exact shortest-path solver.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

using std::vector;
using u64 = uint64_t;
static constexpr u64 INF = std::numeric_limits<u64>::max() / 4;

class Input {
    static constexpr size_t N = 1 << 20;
    FILE* f; char buf[N]; size_t p = 0, n = 0;
public:
    explicit Input(const char* path) : f(std::fopen(path, "rb")) {
        if (!f) { std::fprintf(stderr, "cannot open %s\n", path); std::exit(1); }
    }
    ~Input() { std::fclose(f); }
    int64_t integer() {
        char c;
        do { if (p == n) { n = std::fread(buf, 1, N, f); p = 0; if (!n) return 0; } c = buf[p++]; } while (c <= ' ');
        bool neg = c == '-'; if (neg) { if (p == n) { n = std::fread(buf, 1, N, f); p = 0; } c = buf[p++]; }
        int64_t x = 0;
        while (c > ' ') { x = x * 10 + c - '0'; if (p == n) { n = std::fread(buf, 1, N, f); p = 0; if (!n) break; } c = buf[p++]; }
        return neg ? -x : x;
    }
};

struct Arc { int to, w; };
struct CSR {
    vector<int> off;
    vector<Arc> edge;
};

static int V, E, flags;
static CSR fw, rv;
static vector<int> gx, gy;
static int lattice_side = 0;
static bool graph_directed = false;
static bool balance_by_arc_work = false;
static int minimum_weight = std::numeric_limits<int>::max();

static CSR make_csr(const vector<std::array<int, 3>>& es, bool reverse) {
    CSR g; g.off.assign(V + 1, 0); g.edge.resize(es.size());
    for (auto e : es) ++g.off[(reverse ? e[1] : e[0]) + 1];
    for (int i = 1; i <= V; ++i) g.off[i] += g.off[i - 1];
    vector<int> pos = g.off;
    for (auto e : es) {
        int a = reverse ? e[1] : e[0], b = reverse ? e[0] : e[1];
        g.edge[pos[a]++] = {b, e[2]};
    }
    return g;
}

static void read_graph(const char* path) {
    Input in(path); V = (int)in.integer(); E = (int)in.integer(); flags = (int)in.integer();
    bool directed = flags & 2; graph_directed = directed;
    vector<std::array<int, 3>> es; es.reserve(directed ? E : 2LL * E);
    for (int i = 0; i < E; ++i) {
        int a = (int)in.integer(), b = (int)in.integer(), w = (int)in.integer();
        minimum_weight = std::min(minimum_weight, w);
        es.push_back({a,b,w}); if (!directed) es.push_back({b,a,w});
    }
    // Coordinates are deliberately skipped: the globally admissible Euclidean
    // potential is weak on the variable-speed road family.
    if (flags & 1) { gx.resize(V);gy.resize(V);for(int i=0;i<V;++i){gx[i]=(int)in.integer();gy[i]=(int)in.integer();} }
    // Recognize (rather than assume) the 2-D torus topology.  Its Manhattan
    // distance times the lightest edge is an exact admissible potential.  It is
    // useful for wide-weight lattices, but too weak to pay for itself when the
    // minimum weight is tiny.
    if (!directed && minimum_weight >= 1000000) {
        int n = (int)std::sqrt((double)V);
        if (n * n == V) {
            bool lattice = true;
            for (auto e : es) {
                int ax=e[0]%n, ay=e[0]/n, bx=e[1]%n, by=e[1]/n;
                int dx=std::abs(ax-bx), dy=std::abs(ay-by);
                dx=std::min(dx,n-dx); dy=std::min(dy,n-dy);
                if (dx+dy != 1) { lattice=false; break; }
            }
            if (lattice) lattice_side=n;
        }
    }
    fw = make_csr(es, false); rv = make_csr(es, true);
    if (directed) {
        int maximum_reverse_degree = 0;
        for (int v = 0; v < V; ++v)
            maximum_reverse_degree = std::max(maximum_reverse_degree,
                                               rv.off[v + 1] - rv.off[v]);
        // Power-law directed graphs expose very large reverse frontiers;
        // geometric road graphs have uniformly small local degrees.
        balance_by_arc_work = maximum_reverse_degree >= 64;
    }
}

static inline int msb(u64 x) { return x ? 64 - __builtin_clzll(x) : 0; }
class RadixHeap {
    using Item = std::pair<u64,int>;
    std::array<vector<Item>, 65> b;
    u64 last = 0; size_t count = 0;
    void pull() {
        if (!b[0].empty()) return;
        int i = 1; while (b[i].empty()) ++i;
        u64 nl = INF; for (auto x : b[i]) nl = std::min(nl, x.first);
        last = nl;
        for (auto x : b[i]) b[msb(x.first ^ last)].push_back(x);
        b[i].clear();
    }
public:
    void clear() { for (auto& x : b) x.clear(); last = 0; count = 0; }
    bool empty() const { return count == 0; }
    void push(u64 key, int value) { b[msb(key ^ last)].push_back({key,value}); ++count; }
    u64 min_key() { pull(); return b[0].back().first; }
    Item pop() { pull(); Item x = b[0].back(); b[0].pop_back(); --count; return x; }
};

static vector<u64> df, db;
static vector<uint32_t> sf, sb;
static uint32_t epoch = 0;
static RadixHeap hf, hb;
static inline u64 getf(int x) { return sf[x] == epoch ? df[x] : INF; }
static inline u64 getb(int x) { return sb[x] == epoch ? db[x] : INF; }
static inline void setf(int x, u64 d) { sf[x] = epoch; df[x] = d; }
static inline void setb(int x, u64 d) { sb[x] = epoch; db[x] = d; }

static int64_t plain_bidijkstra(int s, int t) {
    if (s == t) return 0;
    ++epoch; hf.clear(); hb.clear(); setf(s,0); setb(t,0); hf.push(0,s); hb.push(0,t);
    u64 best = INF, forward_work = 0, backward_work = 0;
    while (!hf.empty() && !hb.empty()) {
        u64 af = hf.min_key(), ab = hb.min_key();
        if (af + ab >= best) break;
        // Either frontier may be advanced without changing the standard
        // min-key termination proof.  Balancing relaxed arcs rather than key
        // radii avoids repeatedly expanding a high-in-degree reverse hub in
        // directed power-law graphs.
        if (balance_by_arc_work ? forward_work <= backward_work : af <= ab) {
            auto [d,u] = hf.pop(); if (d != getf(u)) continue;
            forward_work += fw.off[u + 1] - fw.off[u];
            u64 od = getb(u); if (od != INF && d + od < best) best = d + od;
            for (int j=fw.off[u]; j<fw.off[u+1]; ++j) { Arc e=fw.edge[j]; u64 nd=d+(uint32_t)e.w;
                if (nd < getf(e.to)) { setf(e.to,nd); hf.push(nd,e.to); u64 z=getb(e.to); if(z!=INF && nd+z<best) best=nd+z; }
            }
        } else {
            auto [d,u] = hb.pop(); if (d != getb(u)) continue;
            backward_work += rv.off[u + 1] - rv.off[u];
            u64 od = getf(u); if (od != INF && d + od < best) best = d + od;
            for (int j=rv.off[u]; j<rv.off[u+1]; ++j) { Arc e=rv.edge[j]; u64 nd=d+(uint32_t)e.w;
                if (nd < getb(e.to)) { setb(e.to,nd); hb.push(nd,e.to); u64 z=getf(e.to); if(z!=INF && nd+z<best) best=nd+z; }
            }
        }
    }
    return best == INF ? -1 : (int64_t)best;
}

static inline int64_t lattice_h(int a,int b){int n=lattice_side,ax=a%n,ay=a/n,bx=b%n,by=b/n;int dx=std::abs(ax-bx),dy=std::abs(ay-by);dx=std::min(dx,n-dx);dy=std::min(dy,n-dy);return int64_t(dx+dy)*minimum_weight;}

// Balanced bidirectional A*: P(v)=(h(v,t)-h(v,s))/2.  Keys are doubled to
// retain integral monotone radix-heap keys and exact stopping arithmetic.
static int64_t lattice_bidijkstra(int s,int t) {
    if(s==t)return 0;
    ++epoch; hf.clear(); hb.clear(); setf(s,0); setb(t,0); hf.push(0,s); hb.push(0,t);
    u64 best=INF; int64_t ps=lattice_h(s,t), pt=-lattice_h(t,s), constant=pt-ps;
    while(!hf.empty()&&!hb.empty()) {
        u64 af=hf.min_key(),ab=hb.min_key();
        if((__int128)af+ab>=(__int128)2*best+constant)break;
        if(af<=ab) {
            auto [key,u]=hf.pop(); int64_t p=lattice_h(u,t)-lattice_h(u,s); u64 d=getf(u);
            if(key!=u64(2*d+p-ps))continue;
            u64 z=getb(u); if(z!=INF)best=std::min(best,d+z);
            for(int j=fw.off[u];j<fw.off[u+1];++j){Arc e=fw.edge[j];u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);int64_t np=lattice_h(e.to,t)-lattice_h(e.to,s);hf.push(u64(2*nd+np-ps),e.to);z=getb(e.to);if(z!=INF)best=std::min(best,nd+z);}}
        } else {
            auto [key,u]=hb.pop(); int64_t p=lattice_h(u,t)-lattice_h(u,s); u64 d=getb(u);
            if(key!=u64(2*d+pt-p))continue;
            u64 z=getf(u); if(z!=INF)best=std::min(best,d+z);
            for(int j=rv.off[u];j<rv.off[u+1];++j){Arc e=rv.edge[j];u64 nd=d+(uint32_t)e.w;if(nd<getb(e.to)){setb(e.to,nd);int64_t np=lattice_h(e.to,t)-lattice_h(e.to,s);hb.push(u64(2*nd+pt-np),e.to);z=getf(e.to);if(z!=INF)best=std::min(best,nd+z);}}
        }
    }
    return best==INF?-1:(int64_t)best;
}

static int64_t bidijkstra(int s,int t) {
    return lattice_side ? lattice_bidijkstra(s,t) : plain_bidijkstra(s,t);
}

struct Query { int s,t; };
static vector<Query> qs;
static vector<int64_t> ans;
static vector<int> head, nextq;
static vector<u64> from_landmark, to_landmark;
static int landmark_count=0;
static vector<u64> full_sssp(int root,const CSR& g);

static void build_landmarks(int count){
    landmark_count=count;from_landmark.reserve((size_t)count*V);if(graph_directed)to_landmark.reserve((size_t)count*V);
    int root=0;for(int v=1;v<V;++v)if(fw.off[v+1]-fw.off[v]+rv.off[v+1]-rv.off[v]>fw.off[root+1]-fw.off[root]+rv.off[root+1]-rv.off[root])root=v;
    vector<u64> nearest(V,INF);
    for(int k=0;k<count;++k){auto a=full_sssp(root,fw);auto b=graph_directed?full_sssp(root,rv):vector<u64>();from_landmark.insert(from_landmark.end(),a.begin(),a.end());if(graph_directed)to_landmark.insert(to_landmark.end(),b.begin(),b.end());u64 far=0;int next=root;for(int v=0;v<V;++v){u64 sep=a[v];if(graph_directed&&b[v]!=INF)sep=sep==INF?b[v]:std::min(sep,b[v]);nearest[v]=std::min(nearest[v],sep);if(nearest[v]!=INF&&nearest[v]>far){far=nearest[v];next=v;}}root=next;}
}

static inline u64 alt_h(int v,int t){u64 h=0;for(int k=0;k<landmark_count;++k){size_t z=(size_t)k*V;u64 lv=from_landmark[z+v],lt=from_landmark[z+t];if(lv!=INF&&lt!=INF&&lt>lv)h=std::max(h,lt-lv);if(graph_directed){u64 vl=to_landmark[z+v],tl=to_landmark[z+t];if(vl!=INF&&tl!=INF&&vl>tl)h=std::max(h,vl-tl);}else if(lv!=INF&&lt!=INF&&lv>lt)h=std::max(h,lv-lt);}return h;}

static int64_t alt_astar(int s,int t){if(s==t)return 0;++epoch;hf.clear();setf(s,0);hf.push(alt_h(s,t),s);while(!hf.empty()){auto[key,u]=hf.pop();u64 d=getf(u);if(key!=d+alt_h(u,t))continue;if(u==t)return (int64_t)d;for(int j=fw.off[u];j<fw.off[u+1];++j){Arc e=fw.edge[j];u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd+alt_h(e.to,t),e.to);}}}return -1;}
static vector<int8_t> separator_side;
static vector<int8_t> separator_region;
static vector<int8_t> separator_leaf;
static vector<u64> separator_from, separator_to;
static int separator_count=0;
struct SubSeparator { int8_t side; int count=0; vector<u64> from,to; };
static SubSeparator subsep[2];
static SubSeparator leafsep[4];

static vector<u64> full_sssp(int root,const CSR& g){vector<u64>d(V,INF);RadixHeap h;d[root]=0;h.push(0,root);while(!h.empty()){auto[du,u]=h.pop();if(du!=d[u])continue;for(int j=g.off[u];j<g.off[u+1];++j){Arc e=g.edge[j];u64 nd=du+(uint32_t)e.w;if(nd<d[e.to]){d[e.to]=nd;h.push(nd,e.to);}}}return d;}
static vector<u64> region_sssp(int root,const CSR& g,int8_t allowed){vector<u64>d(V,INF);RadixHeap h;d[root]=0;h.push(0,root);while(!h.empty()){auto[du,u]=h.pop();if(du!=d[u])continue;for(int j=g.off[u];j<g.off[u+1];++j){Arc e=g.edge[j];if(separator_side[e.to]!=allowed)continue;u64 nd=du+(uint32_t)e.w;if(nd<d[e.to]){d[e.to]=nd;h.push(nd,e.to);}}}return d;}
static vector<u64> subregion_sssp(int root,const CSR& g,int8_t allowed){vector<u64>d(V,INF);RadixHeap h;d[root]=0;h.push(0,root);while(!h.empty()){auto[du,u]=h.pop();if(du!=d[u])continue;for(int j=g.off[u];j<g.off[u+1];++j){Arc e=g.edge[j];if(separator_region[e.to]!=allowed)continue;u64 nd=du+(uint32_t)e.w;if(nd<d[e.to]){d[e.to]=nd;h.push(nd,e.to);}}}return d;}

static void build_leafseparator(int8_t parent,int index){
    SubSeparator& ss=leafsep[index];ss.side=parent;vector<int> nodes;for(int v=0;v<V;++v)if(separator_region[v]==parent)nodes.push_back(v);
    int xmin=gx[nodes[0]],xmax=xmin,ymin=gy[nodes[0]],ymax=ymin;for(int v:nodes){xmin=std::min(xmin,gx[v]);xmax=std::max(xmax,gx[v]);ymin=std::min(ymin,gy[v]);ymax=std::max(ymax,gy[v]);}
    bool xa=(int64_t)xmax-xmin>=(int64_t)ymax-ymin;vector<int> vals;for(int v:nodes)vals.push_back(xa?gx[v]:gy[v]);std::nth_element(vals.begin(),vals.begin()+vals.size()/2,vals.end());int cut=vals[vals.size()/2];int8_t lo=2*index+1,hi=lo+1,bar=20+index;
    for(int v:nodes)separator_leaf[v]=((xa?gx[v]:gy[v])<cut)?lo:hi;
    vector<uint8_t> mark(V);for(int u:nodes)for(int j=fw.off[u];j<fw.off[u+1];++j){int v=fw.edge[j].to;if(separator_region[v]==parent&&separator_leaf[u]!=separator_leaf[v])mark[separator_leaf[u]==hi?u:v]=1;}
    vector<int> hubs;for(int v:nodes)if(mark[v]){separator_leaf[v]=bar;hubs.push_back(v);}ss.count=hubs.size();ss.from.resize((size_t)V*ss.count);ss.to.resize((size_t)V*ss.count);for(int k=0;k<ss.count;++k){auto a=subregion_sssp(hubs[k],fw,parent),b=subregion_sssp(hubs[k],rv,parent);for(int v:nodes){ss.from[(size_t)v*ss.count+k]=a[v];ss.to[(size_t)v*ss.count+k]=b[v];}}
}

static void build_subseparator(int8_t top_side,SubSeparator& ss){
    ss.side=top_side;vector<int> nodes;for(int v=0;v<V;++v)if(separator_side[v]==top_side)nodes.push_back(v);
    int xmin=gx[nodes[0]],xmax=xmin,ymin=gy[nodes[0]],ymax=ymin;for(int v:nodes){xmin=std::min(xmin,gx[v]);xmax=std::max(xmax,gx[v]);ymin=std::min(ymin,gy[v]);ymax=std::max(ymax,gy[v]);}
    bool xa=(int64_t)xmax-xmin>=(int64_t)ymax-ymin;vector<int> vals;vals.reserve(nodes.size());for(int v:nodes)vals.push_back(xa?gx[v]:gy[v]);std::nth_element(vals.begin(),vals.begin()+vals.size()/2,vals.end());int cut=vals[vals.size()/2];
    for(int v:nodes)separator_region[v]=((xa?gx[v]:gy[v])<cut)?top_side:2*top_side;
    vector<uint8_t> mark(V);for(int u:nodes)for(int j=fw.off[u];j<fw.off[u+1];++j){int v=fw.edge[j].to;if(separator_side[v]==top_side&&separator_region[u]!=separator_region[v])mark[separator_region[u]==2*top_side?u:v]=1;}
    vector<int> hubs;for(int v:nodes)if(mark[v]){separator_region[v]=3*top_side;hubs.push_back(v);}ss.count=hubs.size();ss.from.resize((size_t)V*ss.count);ss.to.resize((size_t)V*ss.count);
    for(int k=0;k<ss.count;++k){auto a=region_sssp(hubs[k],fw,top_side),b=region_sssp(hubs[k],rv,top_side);for(int v:nodes){ss.from[(size_t)v*ss.count+k]=a[v];ss.to[(size_t)v*ss.count+k]=b[v];}}
}

static void build_separator(){
    separator_side.assign(V,0);int xmin=gx[0],xmax=xmin,ymin=gy[0],ymax=ymin;for(int v=1;v<V;++v){xmin=std::min(xmin,gx[v]);xmax=std::max(xmax,gx[v]);ymin=std::min(ymin,gy[v]);ymax=std::max(ymax,gy[v]);}
    bool xa=(int64_t)xmax-xmin>=(int64_t)ymax-ymin;vector<int> vals(V);for(int v=0;v<V;++v)vals[v]=xa?gx[v]:gy[v];std::nth_element(vals.begin(),vals.begin()+V/2,vals.end());int cut=vals[V/2];for(int v=0;v<V;++v)separator_side[v]=((xa?gx[v]:gy[v])<cut)?-1:1;
    // One endpoint per crossing edge is sufficient: choosing its positive-side
    // endpoint is a vertex cover of all median-plane arcs and halves the table.
    vector<uint8_t> sep(V);for(int u=0;u<V;++u)for(int j=fw.off[u];j<fw.off[u+1];++j){int v=fw.edge[j].to;if(separator_side[u]!=separator_side[v])sep[separator_side[u]>0?u:v]=1;}
    vector<int> hubs;for(int v=0;v<V;++v)if(sep[v]){separator_side[v]=0;hubs.push_back(v);}separator_count=hubs.size();
    separator_from.resize((size_t)V*separator_count);separator_to.resize((size_t)V*separator_count);
    for(int k=0;k<separator_count;++k){auto a=full_sssp(hubs[k],fw),b=full_sssp(hubs[k],rv);for(int v=0;v<V;++v){separator_from[(size_t)v*separator_count+k]=a[v];separator_to[(size_t)v*separator_count+k]=b[v];}}
    separator_region.assign(V,0);build_subseparator(-1,subsep[0]);build_subseparator(1,subsep[1]);separator_leaf=separator_region;int8_t parents[4]={-2,-1,1,2};for(int i=0;i<4;++i)build_leafseparator(parents[i],i);
}

static u64 separator_answer(int s,int t){u64 z=INF;size_t a=(size_t)s*separator_count,b=(size_t)t*separator_count;for(int k=0;k<separator_count;++k){u64 x=separator_to[a+k],y=separator_from[b+k];if(x!=INF&&y!=INF)z=std::min(z,x+y);}return z;}
static u64 subseparator_answer(int s,int t,int8_t side){SubSeparator& ss=subsep[side>0];u64 z=INF;size_t a=(size_t)s*ss.count,b=(size_t)t*ss.count;for(int k=0;k<ss.count;++k){u64 x=ss.to[a+k],y=ss.from[b+k];if(x!=INF&&y!=INF)z=std::min(z,x+y);}return z;}
static u64 leafseparator_answer(int s,int t,int8_t parent){int index=parent==-2?0:parent==-1?1:parent==1?2:3;SubSeparator& ss=leafsep[index];u64 z=INF;size_t a=(size_t)s*ss.count,b=(size_t)t*ss.count;for(int k=0;k<ss.count;++k){u64 x=ss.to[a+k],y=ss.from[b+k];if(x!=INF&&y!=INF)z=std::min(z,x+y);}return z;}

static void grouped_side(const vector<int>& ids){
    ++epoch;hf.clear();int root=qs[ids[0]].t;int8_t side=separator_leaf[root];vector<int> touched;int remaining=0;
    for(int id:ids){int x=qs[id].s;if(head[x]==-1){touched.push_back(x);++remaining;}nextq[id]=head[x];head[x]=id;}
    setf(root,0);hf.push(0,root);
    while(!hf.empty()&&remaining){auto[d,u]=hf.pop();if(d!=getf(u))continue;if(head[u]!=-1){for(int id=head[u];id!=-1;id=nextq[id])ans[id]=ans[id]<0?(int64_t)d:std::min(ans[id],(int64_t)d);head[u]=-1;--remaining;}for(int j=rv.off[u];j<rv.off[u+1];++j){Arc e=rv.edge[j];if(separator_leaf[e.to]!=side)continue;u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd,e.to);}}}
    for(int x:touched)head[x]=-1;
}

// One Dijkstra answers all queries sharing an endpoint.  Search direction is
// selected by the repeated endpoint, and it stops as soon as the last requested
// opposite endpoint is settled.
static void grouped(const vector<int>& ids, bool same_source) {
    ++epoch; hf.clear();
    int root = same_source ? qs[ids[0]].s : qs[ids[0]].t;
    vector<int> touched; touched.reserve(ids.size());
    int remaining = 0;
    for (int id : ids) {
        int x = same_source ? qs[id].t : qs[id].s;
        if (head[x] == -1) { touched.push_back(x); ++remaining; }
        nextq[id] = head[x]; head[x] = id;
    }
    setf(root,0); hf.push(0,root); const CSR& g = same_source ? fw : rv;
    while (!hf.empty() && remaining) {
        auto [d,u]=hf.pop(); if (d != getf(u)) continue;
        if (head[u] != -1) {
            for (int id=head[u]; id!=-1; id=nextq[id]) ans[id]=(int64_t)d;
            head[u]=-1; --remaining;
        }
        for (int j=g.off[u]; j<g.off[u+1]; ++j) { Arc e=g.edge[j]; u64 nd=d+(uint32_t)e.w;
            if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd,e.to);}
        }
    }
    // Any endpoints not reached retain -1.
    for (int x : touched) head[x] = -1;
}

static void run_queries(const char* qpath, const char* opath) {
    Input in(qpath); int Q=(int)in.integer(); qs.resize(Q); ans.assign(Q,-1); nextq.resize(Q); head.assign(V,-1);
    std::unordered_map<int,vector<int>> bys, byt; bys.reserve(Q/2); byt.reserve(Q/2);
    for(int i=0;i<Q;++i){ int s=(int)in.integer(),t=(int)in.integer(); qs[i]={s,t}; if(s==t) ans[i]=0; else {bys[s].push_back(i);byt[t].push_back(i);} }
    if(!gx.empty()&&double(Q)/V>=5.0){
        build_separator();vector<uint8_t> handled(Q);for(int i=0;i<Q;++i)if(ans[i]<0){int s=qs[i].s,t=qs[i].t;u64 z=separator_answer(s,t);if(separator_side[s]==separator_side[t]&&separator_side[s]!=0)z=std::min(z,subseparator_answer(s,t,separator_side[s]));if(separator_region[s]==separator_region[t]&&separator_region[s]!=0&&std::abs((int)separator_region[s])<=2)z=std::min(z,leafseparator_answer(s,t,separator_region[s]));ans[i]=z==INF?-1:(int64_t)z;if(separator_side[s]!=separator_side[t]||separator_side[s]==0||separator_region[s]!=separator_region[t]||std::abs((int)separator_region[s])==3||separator_leaf[s]!=separator_leaf[t]||separator_leaf[s]>=20)handled[i]=1;}
        for(auto& kv:byt){std::array<vector<int>,8> part;for(int id:kv.second)if(!handled[id])part[separator_leaf[qs[id].s]-1].push_back(id);for(auto& ids:part)if(!ids.empty())grouped_side(ids);for(int id:kv.second)handled[id]=1;}
        FILE* out=std::fopen(opath,"wb");if(!out){std::fprintf(stderr,"cannot open output\n");std::exit(1);}char buf[64];for(auto x:ans){int n=std::snprintf(buf,sizeof(buf),"%lld\n",(long long)x);std::fwrite(buf,1,n,out);}std::fclose(out);return;
    }
    double qpv=double(Q)/V,avgdeg=double(fw.edge.size())/V;
    if(qpv>=0.5&&qpv<5.0&&avgdeg<5.0)build_landmarks(12);
    vector<uint8_t> done(Q,0);
    // Repeated targets are especially valuable on the scale-free hub workload.
    for(auto& kv:byt) if(kv.second.size()>=4){ grouped(kv.second,false); for(int id:kv.second)done[id]=1; }
    // Balanced-source huge-Q workloads amortize one search over many targets.
    for(auto& kv:bys){ vector<int> ids; if(kv.second.size()>=4){ for(int id:kv.second)if(!done[id])ids.push_back(id); if(ids.size()>=4){grouped(ids,true);for(int id:ids)done[id]=1;} } }
    for(int i=0;i<Q;++i) if(ans[i]<0 && !done[i]) ans[i]=landmark_count?alt_astar(qs[i].s,qs[i].t):bidijkstra(qs[i].s,qs[i].t);
    FILE* out=std::fopen(opath,"wb"); if(!out){std::fprintf(stderr,"cannot open output\n");std::exit(1);} char buf[64];
    for(auto x:ans){int n=std::snprintf(buf,sizeof(buf),"%lld\n",(long long)x);std::fwrite(buf,1,n,out);} std::fclose(out);
}

int main(int argc,char**argv){if(argc!=4){std::fprintf(stderr,"usage: %s <graph_file> <query_file> <output_file>\n",argv[0]);return 1;} read_graph(argv[1]);df.resize(V);db.resize(V);sf.assign(V,0);sb.assign(V,0);run_queries(argv[2],argv[3]);return 0;}

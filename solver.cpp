// Adaptive exact shortest-path solver.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <limits>
#include <unordered_map>
#include <queue>
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
static vector<uint8_t> road_corridor_level;
static int lattice_side = 0;
static int torus2_side = 0;
static int torus3_side = 0;
static bool graph_directed = false;
// Power-law directed graphs have highly skewed reverse degrees. Expanding
// whichever side has relaxed fewer arcs avoids repeatedly paying for a hub.
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
    if (!directed) {
        int n = (int)std::sqrt((double)V);
        if (n * n == V) {
            bool lattice = true;
            for (auto e : es) {
                int ax=e[0]%n, ay=e[0]/n, bx=e[1]%n, by=e[1]/n;
                int dx=std::abs(ax-bx), dy=std::abs(ay-by);
                dx=std::min(dx,n-dx); dy=std::min(dy,n-dy);
                if (dx+dy != 1) { lattice=false; break; }
            }
            if (lattice) {torus2_side=n;if(minimum_weight>=1000000)lattice_side=n;}
        }
    }
    // Verify a cubic 3-D torus from its observable edge topology.  Lattice3d
    // uses a denser global ALT set than the 2-D workloads; all other graphs
    // keep the standard 24-landmark path.
    if(!directed&&!torus2_side){int n=(int)std::round(std::cbrt((double)V));if((int64_t)n*n*n==V){bool lattice=true;for(auto e:es){int a=e[0],b=e[1],dx=std::abs(a%n-b%n),dy=std::abs((a/n)%n-(b/n)%n),dz=std::abs(a/(n*n)-b/(n*n));dx=std::min(dx,n-dx);dy=std::min(dy,n-dy);dz=std::min(dz,n-dz);if(dx+dy+dz!=1){lattice=false;break;}}if(lattice)torus3_side=n;}}
    fw = make_csr(es, false); rv = make_csr(es, true);
    if (directed) {
        int maximum_reverse_degree = 0;
        for (int v = 0; v < V; ++v)
            maximum_reverse_degree = std::max(maximum_reverse_degree,
                                               rv.off[v + 1] - rv.off[v]);
        balance_by_arc_work = maximum_reverse_degree >= 64;
    }
}

// Every removed edge has an original-graph positive-weight detour no more
// expensive than itself. Each detour edge is strictly lighter. Recursively
// substituting removed detour edges therefore terminates, proving that marking
// all decisions before rebuilding still preserves every pairwise distance.
static void sparsify_undirected(){
    if(graph_directed||minimum_weight<=0)return;
    auto start=std::chrono::steady_clock::now();
    const int degree_cap=64,work_cap=512;
    vector<uint8_t> remove(fw.edge.size());
    vector<uint32_t> stamp(V);vector<u64> last_weight(V);uint32_t tick=0;
    size_t removed=0,examined=0;
    for(int u=0;u<V;++u){
        if(fw.off[u+1]-fw.off[u]>degree_cap)continue;
        for(int j=fw.off[u];j<fw.off[u+1];++j){
            Arc edge=fw.edge[j];int v=edge.to;u64 w=(uint32_t)edge.w;
            if(u>=v||remove[j]||fw.off[v+1]-fw.off[v]>degree_cap)continue;
            ++examined;
            if(++tick==0){std::fill(stamp.begin(),stamp.end(),0);++tick;}
            // Original undirected neighbors of v provide O(1) final-edge
            // lookup for both two-edge and three-edge candidate detours.
            for(int k=fw.off[v];k<fw.off[v+1];++k){
                Arc e=fw.edge[k];u64 z=(uint32_t)e.w;
                if(stamp[e.to]!=tick){stamp[e.to]=tick;last_weight[e.to]=z;}
                else last_weight[e.to]=std::min(last_weight[e.to],z);
            }
            bool dominated=false;int work=0;
            for(int k=fw.off[u];k<fw.off[u+1]&&!dominated&&work<work_cap;++k){
                Arc a=fw.edge[k];u64 wa=(uint32_t)a.w;
                if(a.to==u||a.to==v||wa>=w)continue;
                ++work;
                if(stamp[a.to]==tick&&wa+last_weight[a.to]<=w){dominated=true;break;}
                if(fw.off[a.to+1]-fw.off[a.to]>degree_cap)continue;
                for(int l=fw.off[a.to];l<fw.off[a.to+1]&&work<work_cap;++l){
                    ++work;Arc b=fw.edge[l];
                    if(b.to==u||b.to==v||b.to==a.to)continue;
                    u64 prefix=wa+(uint32_t)b.w;
                    if(prefix<w&&stamp[b.to]==tick&&prefix+last_weight[b.to]<=w){dominated=true;break;}
                }
            }
            if(dominated){
                remove[j]=1;++removed;
                // Symmetric removal is required by undirected ALT. Parallel
                // arcs of the same weight share this identical witness.
                for(int k=fw.off[v];k<fw.off[v+1];++k)
                    if(fw.edge[k].to==u&&fw.edge[k].w==edge.w&&!remove[k]){remove[k]=1;++removed;}
            }
        }
    }
    size_t original=fw.edge.size();CSR reduced;reduced.off.resize(V+1);reduced.edge.reserve(original-removed);
    for(int u=0;u<V;++u){reduced.off[u]=reduced.edge.size();for(int j=fw.off[u];j<fw.off[u+1];++j)if(!remove[j])reduced.edge.push_back(fw.edge[j]);}
    reduced.off[V]=reduced.edge.size();fw=std::move(reduced);rv=fw;
    std::fprintf(stderr,"SPARSIFY removed %zu / %zu arcs (%.3f%%), examined %zu undirected edges, preprocessing %.6f s\n",removed,original,original?100.0*removed/original:0.0,examined,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
}

// Directed road-only exact dominated-arc certificate. Every witness is a
// shorter-hop path whose individual arcs are all strictly lighter than the
// removed arc, so simultaneous removal is safe by weight induction.
static void sparsify_directed_road(){
    if(!graph_directed)return;auto started=std::chrono::steady_clock::now();const int degree_cap=64,work_cap=512;
    vector<uint8_t> remove(fw.edge.size());vector<uint32_t> stamp(V);vector<u64> last(V);uint32_t tick=0;size_t removed=0,examined=0;
    for(int u=0;u<V;++u){if(fw.off[u+1]-fw.off[u]>degree_cap)continue;
        for(int j=fw.off[u];j<fw.off[u+1];++j){Arc edge=fw.edge[j];int v=edge.to;u64 w=(uint32_t)edge.w;if(u==v||remove[j]||rv.off[v+1]-rv.off[v]>degree_cap)continue;++examined;
            if(++tick==0){std::fill(stamp.begin(),stamp.end(),0);++tick;}
            for(int k=rv.off[v];k<rv.off[v+1];++k){Arc e=rv.edge[k];u64 z=(uint32_t)e.w;if(stamp[e.to]!=tick){stamp[e.to]=tick;last[e.to]=z;}else last[e.to]=std::min(last[e.to],z);}
            bool dominated=false;int work=0;
            for(int k=fw.off[u];k<fw.off[u+1]&&!dominated&&work<work_cap;++k){Arc a=fw.edge[k];u64 wa=(uint32_t)a.w;if(a.to==u||a.to==v||wa>=w)continue;++work;if(stamp[a.to]==tick&&wa+last[a.to]<=w){dominated=true;break;}
                if(fw.off[a.to+1]-fw.off[a.to]>degree_cap)continue;
                for(int l=fw.off[a.to];l<fw.off[a.to+1]&&work<work_cap;++l){++work;Arc b=fw.edge[l];if(b.to==u||b.to==v||b.to==a.to)continue;u64 prefix=wa+(uint32_t)b.w;if(prefix<w&&stamp[b.to]==tick&&prefix+last[b.to]<=w){dominated=true;break;}}
            }
            if(dominated){remove[j]=1;++removed;}
        }
    }
    vector<std::array<int,3>> es;es.reserve(fw.edge.size()-removed);for(int u=0;u<V;++u)for(int j=fw.off[u];j<fw.off[u+1];++j)if(!remove[j])es.push_back({u,fw.edge[j].to,fw.edge[j].w});size_t original=fw.edge.size();fw=make_csr(es,false);rv=make_csr(es,true);
    std::fprintf(stderr,"ROAD_SPARSE removed %zu / %zu arcs (%.3f%%), examined %zu, time %.6f s\n",removed,original,original?100.0*removed/original:0.0,examined,std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count());
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
static vector<u64> current_fkey;
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
        // Balancing relaxed arcs is the exact min-key bidirectional stopping
        // algorithm used by the scale-free fast path. The choice of side does
        // not affect the proof; it only avoids expanding a high-in-degree hub.
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
static vector<uint32_t> local_grid_landmark;
static constexpr int local_grid_side = 15;
// Per-query heuristic memoization retained from the directed ALT path.
// Landmark tables are immutable after preprocessing; the epoch stamp keeps
// this cache exact when the same vertex is examined repeatedly in one query.
static vector<u64> landmark_heuristic_cache;
static vector<uint32_t> landmark_heuristic_stamp;
static int landmark_count=0;
static vector<u64> full_sssp(int root,const CSR& g);

static void build_landmarks(int count){
    landmark_count=count;from_landmark.reserve((size_t)count*V);if(graph_directed)to_landmark.reserve((size_t)count*V);
    landmark_heuristic_cache.resize(V); landmark_heuristic_stamp.assign(V,0);
    int root=0;for(int v=1;v<V;++v)if(fw.off[v+1]-fw.off[v]+rv.off[v+1]-rv.off[v]>fw.off[root+1]-fw.off[root]+rv.off[root+1]-rv.off[root])root=v;
    vector<u64> nearest(V,INF);
    for(int k=0;k<count;++k){auto a=full_sssp(root,fw);auto b=graph_directed?full_sssp(root,rv):vector<u64>();from_landmark.insert(from_landmark.end(),a.begin(),a.end());if(graph_directed)to_landmark.insert(to_landmark.end(),b.begin(),b.end());u64 far=0;int next=root;for(int v=0;v<V;++v){u64 sep=a[v];if(graph_directed&&b[v]!=INF)sep=sep==INF?b[v]:std::min(sep,b[v]);nearest[v]=std::min(nearest[v],sep);if(nearest[v]!=INF&&nearest[v]>far){far=nearest[v];next=v;}}root=next;}
}

// On a verified low-weight 2-D torus, a regular grid of exact landmark SSSPs
// amortizes across the large batch of mostly local queries.  The four anchors
// nearest the target give a much stronger short-range bound than global
// farthest-point landmarks while retaining exact A* fallback behavior.
static void build_local_grid_landmarks(){
    if(!torus2_side||lattice_side)return;
    // A simple path has at most V-1 edges, so this observed maximum-weight
    // bound proves every finite landmark distance fits in uint32_t.
    uint32_t max_w=0;for(const Arc& e:fw.edge)max_w=std::max(max_w,(uint32_t)e.w);
    if((__int128)(V-1)*max_w>=UINT32_MAX)return;
    auto started=std::chrono::steady_clock::now();local_grid_landmark.reserve((size_t)local_grid_side*local_grid_side*V);
    for(int ay=0;ay<local_grid_side;++ay)for(int ax=0;ax<local_grid_side;++ax){int x=(int64_t)ax*torus2_side/local_grid_side,y=(int64_t)ay*torus2_side/local_grid_side;auto d=full_sssp(y*torus2_side+x,fw);for(u64 z:d){if(z==INF||z>UINT32_MAX){local_grid_landmark.clear();return;}local_grid_landmark.push_back((uint32_t)z);}}
    std::fprintf(stderr,"LOCAL_GRID %d preprocessing %.6f s\n",local_grid_side*local_grid_side,std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count());
}

static int64_t local_grid_astar(int s,int t){
    if(s==t)return 0;int n=torus2_side,sx=s%n,sy=s/n,tx=t%n,ty=t/n,dx=std::abs(sx-tx),dy=std::abs(sy-ty);dx=std::min(dx,n-dx);dy=std::min(dy,n-dy);
    std::array<const uint32_t*,4> table{};std::array<u64,4> target{};int c=0;
    if(dx+dy>96){std::array<std::pair<u64,int>,4>active;for(auto&x:active)x={0,-1};for(int k=0;k<local_grid_side*local_grid_side;++k){const uint32_t*p=local_grid_landmark.data()+(size_t)k*V;u64 a=p[s],b=p[t];std::pair<u64,int>x={a>b?a-b:b-a,k};for(int j=0;j<4;++j)if(x>active[j])std::swap(x,active[j]);}for(auto x:active){table[c]=local_grid_landmark.data()+(size_t)x.second*V;target[c]=table[c][t];++c;}}
    else{std::array<std::pair<int,int>,local_grid_side>xd,yd;for(int a=0;a<local_grid_side;++a){int z=(int64_t)a*n/local_grid_side,ax=std::abs(z-tx),ay=std::abs(z-ty);xd[a]={std::min(ax,n-ax),a};yd[a]={std::min(ay,n-ay),a};}std::nth_element(xd.begin(),xd.begin()+2,xd.end());std::nth_element(yd.begin(),yd.begin()+2,yd.end());for(int iy=0;iy<2;++iy)for(int ix=0;ix<2;++ix){int k=yd[iy].second*local_grid_side+xd[ix].second;table[c]=local_grid_landmark.data()+(size_t)k*V;target[c]=table[c][t];++c;}}
    auto h=[&](int v){u64 z=0;for(int k=0;k<4;++k){u64 d=table[k][v];z=std::max(z,d>target[k]?d-target[k]:target[k]-d);}return z;};
    ++epoch;hf.clear();setf(s,0);current_fkey[s]=h(s);hf.push(current_fkey[s],s);while(!hf.empty()){auto[key,u]=hf.pop();u64 d=getf(u);if(key!=current_fkey[u])continue;if(u==t)return (int64_t)d;for(int j=fw.off[u];j<fw.off[u+1];++j){Arc e=fw.edge[j];u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);current_fkey[e.to]=nd+h(e.to);hf.push(current_fkey[e.to],e.to);}}}return -1;
}

static inline u64 alt_h(int v,int t){u64 h=0;for(int k=0;k<landmark_count;++k){size_t z=(size_t)k*V;u64 lv=from_landmark[z+v],lt=from_landmark[z+t];if(lv!=INF&&lt!=INF&&lt>lv)h=std::max(h,lt-lv);if(graph_directed){u64 vl=to_landmark[z+v],tl=to_landmark[z+t];if(vl!=INF&&tl!=INF&&vl>tl)h=std::max(h,vl-tl);}else if(lv!=INF&&lt!=INF&&lv>lt)h=std::max(h,lv-lt);}return h;}
static inline u64 cached_alt_h(int v,int t){
    if(landmark_heuristic_stamp[v]!=epoch){
        landmark_heuristic_stamp[v]=epoch;
        landmark_heuristic_cache[v]=alt_h(v,t);
    }
    return landmark_heuristic_cache[v];
}

static int64_t alt_astar(int s,int t){if(s==t)return 0;++epoch;hf.clear();setf(s,0);hf.push(cached_alt_h(s,t),s);while(!hf.empty()){auto[key,u]=hf.pop();u64 d=getf(u);if(key!=d+cached_alt_h(u,t))continue;if(u==t)return (int64_t)d;for(int j=fw.off[u];j<fw.off[u+1];++j){Arc e=fw.edge[j];u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd+cached_alt_h(e.to,t),e.to);}}}return -1;}

// Fixed per-query subsets preserve consistency of the landmark lower bound.
static int64_t balanced_alt(int s, int t) {
    if (s == t) return 0;
    std::array<std::pair<u64,int>,4> active;
    for (auto& a : active) a = {0,-1};
    for (int k=0;k<landmark_count;++k) {
        size_t z=(size_t)k*V;
        u64 a=from_landmark[z+s],b=from_landmark[z+t];
        if(a==INF || b==INF) continue;
        std::pair<u64,int> score={a>b?a-b:b-a,k};
        for(int j=0;j<4;++j) if(score>active[j]) {std::swap(score,active[j]);}
    }
    std::array<const u64*,4> tables{};
    std::array<u64,4> ls{},lt{};
    int count=0;
    for(auto chosen:active) if(chosen.second>=0) {
        const u64* table=from_landmark.data()+(size_t)chosen.second*V;
        tables[count]=table;ls[count]=table[s];lt[count]=table[t];++count;
    }
    auto p = [&](int v) -> int64_t {
        u64 hs=0,ht=0;
        for(int k=0;k<count;++k) {
            u64 d=tables[k][v];
            if(d==INF)continue;
            hs=std::max(hs,d>ls[k]?d-ls[k]:ls[k]-d);
            ht=std::max(ht,d>lt[k]?d-lt[k]:lt[k]-d);
        }
        return (int64_t)ht-(int64_t)hs;
    };
    int64_t ps=p(s),pt=p(t);
    ++epoch;hf.clear();hb.clear();setf(s,0);setb(t,0);hf.push(0,s);hb.push(0,t);
    u64 best=INF,forward_work=0,backward_work=0;
    while(!hf.empty()&&!hb.empty()) {
        u64 af=hf.min_key(),ab=hb.min_key();
        if((__int128)af+ab >= (__int128)2*best+pt-ps) break;
        bool forward=torus3_side?(forward_work<=backward_work):(af<=ab);
        auto [key,u]=forward?hf.pop():hb.pop();
        u64 du=forward?getf(u):getb(u);
        int64_t pu=p(u);
        u64 expected=forward?u64(2*du+pu-ps):u64(2*du+pt-pu);
        if(key!=expected)continue;
        u64 od=forward?getb(u):getf(u);
        if(od!=INF)best=std::min(best,du+od);
        const CSR& g=forward?fw:rv;
        if(torus3_side) {
            if(forward)forward_work+=(u64)(g.off[u+1]-g.off[u]);
            else backward_work+=(u64)(g.off[u+1]-g.off[u]);
        }
        for(int j=g.off[u];j<g.off[u+1];++j) {
            Arc e=g.edge[j];u64 nd=du+(uint32_t)e.w;
            u64 old=forward?getf(e.to):getb(e.to);
            if(nd>=old)continue;
            int64_t np=p(e.to);
            if(forward){setf(e.to,nd);hf.push(u64(2*nd+np-ps),e.to);}
            else{setb(e.to,nd);hb.push(u64(2*nd+pt-np),e.to);}
            u64 other=forward?getb(e.to):getf(e.to);
            if(other!=INF)best=std::min(best,nd+other);
        }
    }
    return best==INF?-1:(int64_t)best;
}

// Rank-stratified wide torus queries are generated in the forward direction.
// Reusing the strongest four of the existing 24 landmark tables in a
// one-sided A* avoids an unconstrained reverse frontier while preserving the
// same exact consistent ALT lower bound.
static int64_t forward_alt8(int s,int t){
    if(s==t)return 0;
    // Eight tables cost more per potential than the former four, but their
    // tighter maximum removes enough high-weight-torus search work to win
    // end-to-end while preserving the same consistent ALT lower bound.
    std::array<std::pair<u64,int>,8> active;for(auto& a:active)a={0,-1};
    for(int k=0;k<landmark_count;++k){size_t z=(size_t)k*V;u64 a=from_landmark[z+s],b=from_landmark[z+t];if(a==INF||b==INF)continue;std::pair<u64,int> score={a>b?a-b:b-a,k};for(int j=0;j<8;++j)if(score>active[j])std::swap(score,active[j]);}
    std::array<const u64*,8> table{};std::array<u64,8> target{};int count=0;for(auto x:active)if(x.second>=0){table[count]=from_landmark.data()+(size_t)x.second*V;target[count]=table[count][t];++count;}
    auto h=[&](int v){u64 z=0;for(int k=0;k<count;++k){u64 d=table[k][v];if(d!=INF&&target[k]!=INF)z=std::max(z,d>target[k]?d-target[k]:target[k]-d);}return z;};
    ++epoch;hf.clear();setf(s,0);hf.push(h(s),s);
    while(!hf.empty()){auto[key,u]=hf.pop();u64 d=getf(u);if(key!=d+h(u))continue;if(u==t)return (int64_t)d;for(int j=fw.off[u];j<fw.off[u+1];++j){Arc e=fw.edge[j];u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd+h(e.to),e.to);}}}
    return -1;
}

static vector<int8_t> separator_side;
static vector<int8_t> separator_region;
static vector<int8_t> separator_leaf;
static vector<u64> separator_from, separator_to;
static int separator_count=0;
struct SubSeparator { int8_t side; int count=0; vector<u64> from,to; };
static SubSeparator subsep[2];
static SubSeparator leafsep[4];
struct MicroSeparator { int count=0; vector<u64> from,to; };
static vector<int8_t> separator_micro;
static MicroSeparator microsep[8];

static vector<u64> full_sssp(int root,const CSR& g){vector<u64>d(V,INF);RadixHeap h;d[root]=0;h.push(0,root);while(!h.empty()){auto[du,u]=h.pop();if(du!=d[u])continue;for(int j=g.off[u];j<g.off[u+1];++j){Arc e=g.edge[j];u64 nd=du+(uint32_t)e.w;if(nd<d[e.to]){d[e.to]=nd;h.push(nd,e.to);}}}return d;}
static vector<u64> region_sssp(int root,const CSR& g,int8_t allowed){vector<u64>d(V,INF);RadixHeap h;d[root]=0;h.push(0,root);while(!h.empty()){auto[du,u]=h.pop();if(du!=d[u])continue;for(int j=g.off[u];j<g.off[u+1];++j){Arc e=g.edge[j];if(separator_side[e.to]!=allowed)continue;u64 nd=du+(uint32_t)e.w;if(nd<d[e.to]){d[e.to]=nd;h.push(nd,e.to);}}}return d;}
static vector<u64> subregion_sssp(int root,const CSR& g,int8_t allowed){vector<u64>d(V,INF);RadixHeap h;d[root]=0;h.push(0,root);while(!h.empty()){auto[du,u]=h.pop();if(du!=d[u])continue;for(int j=g.off[u];j<g.off[u+1];++j){Arc e=g.edge[j];if(separator_region[e.to]!=allowed)continue;u64 nd=du+(uint32_t)e.w;if(nd<d[e.to]){d[e.to]=nd;h.push(nd,e.to);}}}return d;}
static vector<u64> microregion_sssp(int root,const CSR& g,int8_t allowed){vector<u64>d(V,INF);RadixHeap h;d[root]=0;h.push(0,root);while(!h.empty()){auto[du,u]=h.pop();if(du!=d[u])continue;for(int j=g.off[u];j<g.off[u+1];++j){Arc e=g.edge[j];if(e.to!=root&&separator_leaf[e.to]!=allowed)continue;u64 nd=du+(uint32_t)e.w;if(nd<d[e.to]){d[e.to]=nd;h.push(nd,e.to);}}}return d;}
// Exact partial pruned labels for the verified separator vertex cover.
// Prior labels prune only when they already supply a real route no longer
// than the current one. Every shortest path touching any selected separator
// root retains an exact two-hop cover. Interior paths use grouped fallback.
struct SparseSepLabel{int hub;u64 d;};
static vector<vector<SparseSepLabel>> sparse_sep_in,sparse_sep_out;
static bool sparse_sep_active=false,sparse_sep_failed=false;
static int sparse_sep_roots=0;
static u64 sparse_sep_count=0,sparse_sep_pops=0,sparse_sep_scans=0,sparse_sep_checks=0;
static std::chrono::steady_clock::time_point sparse_sep_start;
static u64 sparse_sep_answer(int s,int t){
 const auto&a=sparse_sep_out[s];const auto&b=sparse_sep_in[t];size_t i=0,j=0;u64 best=INF;
 while(i<a.size()&&j<b.size()){if(a[i].hub<b[j].hub)++i;else if(a[i].hub>b[j].hub)++j;else{best=std::min(best,a[i].d+b[j].d);++i;++j;}}
 return best;
}
static vector<int> sparse_sep_pending;
static bool sparse_sep_collecting=true;
static bool build_road_corridor_levels();
static void sep_label_root(int root){
 if(sparse_sep_collecting){sparse_sep_pending.push_back(root);return;}
 if(sparse_sep_failed)return;
 int id=sparse_sep_roots++;vector<u64> cache(sparse_sep_roots,INF);
 for(int direction=0;direction<2;++direction){
  const bool forward=direction==0;const CSR& g=forward?fw:rv;
  auto& rootlist=forward?sparse_sep_out[root]:sparse_sep_in[root];
  std::fill(cache.begin(),cache.end(),INF);for(auto x:rootlist)cache[x.hub]=x.d;
  ++epoch;hf.clear();setf(root,0);hf.push(0,root);
  while(!hf.empty()){
   auto[d,u]=hf.pop();if(d!=getf(u))continue;++sparse_sep_pops;
   auto& prior=forward?sparse_sep_in[u]:sparse_sep_out[u];
   bool covered=false;if(u!=root)for(auto x:prior){++sparse_sep_checks;if(x.hub<id&&cache[x.hub]!=INF&&cache[x.hub]+x.d<=d){covered=true;break;}}
   if(covered)continue;
   prior.push_back({id,d});++sparse_sep_count;
   for(int j=g.off[u];j<g.off[u+1];++j){++sparse_sep_scans;Arc e=g.edge[j];u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd,e.to);}}
  }
 }
 if(id%100==0){
  double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-sparse_sep_start).count();
  std::fprintf(stderr,"SPARSE_SEP roots %d labels %llu time %.3f\n",sparse_sep_roots,(unsigned long long)sparse_sep_count,seconds);
  if(seconds>20||sparse_sep_count>30000000)sparse_sep_failed=true;
 }
}
static void build_microseparator(int8_t parent,int index){
    MicroSeparator& ss=microsep[index]; vector<int> nodes; for(int v=0;v<V;++v) if(separator_leaf[v]==parent) nodes.push_back(v);
    if(nodes.empty()) return; int xmin=gx[nodes[0]],xmax=xmin,ymin=gy[nodes[0]],ymax=ymin; for(int v:nodes){xmin=std::min(xmin,gx[v]);xmax=std::max(xmax,gx[v]);ymin=std::min(ymin,gy[v]);ymax=std::max(ymax,gy[v]);}
    bool xa=(int64_t)xmax-xmin>=(int64_t)ymax-ymin; vector<int> vals; vals.reserve(nodes.size()); for(int v:nodes) vals.push_back(xa?gx[v]:gy[v]); std::nth_element(vals.begin(),vals.begin()+vals.size()/2,vals.end()); int cut=vals[vals.size()/2]; int8_t lo=41+2*index,hi=lo+1,bar=61+index;
    for(int v:nodes) separator_micro[v]=((xa?gx[v]:gy[v])<cut)?lo:hi; vector<uint8_t> mark(V); for(int u:nodes) for(int j=fw.off[u];j<fw.off[u+1];++j){int v=fw.edge[j].to; if(separator_leaf[v]==parent&&separator_micro[u]!=separator_micro[v]) mark[separator_micro[u]==hi?u:v]=1;}
    vector<int> hubs; for(int v:nodes) if(mark[v]){separator_micro[v]=bar; hubs.push_back(v);} ss.count=hubs.size();if(sparse_sep_collecting){for(int root:hubs)sep_label_root(root);return;} ss.from.resize((size_t)V*ss.count); ss.to.resize((size_t)V*ss.count); for(int k=0;k<ss.count;++k){auto a=microregion_sssp(hubs[k],fw,parent),b=microregion_sssp(hubs[k],rv,parent); for(int v:nodes){ss.from[(size_t)v*ss.count+k]=a[v]; ss.to[(size_t)v*ss.count+k]=b[v];}}
}

static void build_leafseparator(int8_t parent,int index){
    SubSeparator& ss=leafsep[index];ss.side=parent;vector<int> nodes;for(int v=0;v<V;++v)if(separator_region[v]==parent)nodes.push_back(v);
    int xmin=gx[nodes[0]],xmax=xmin,ymin=gy[nodes[0]],ymax=ymin;for(int v:nodes){xmin=std::min(xmin,gx[v]);xmax=std::max(xmax,gx[v]);ymin=std::min(ymin,gy[v]);ymax=std::max(ymax,gy[v]);}
    bool xa=(int64_t)xmax-xmin>=(int64_t)ymax-ymin;vector<int> vals;for(int v:nodes)vals.push_back(xa?gx[v]:gy[v]);std::nth_element(vals.begin(),vals.begin()+vals.size()/2,vals.end());int cut=vals[vals.size()/2];int8_t lo=2*index+1,hi=lo+1,bar=20+index;
    for(int v:nodes)separator_leaf[v]=((xa?gx[v]:gy[v])<cut)?lo:hi;
    vector<uint8_t> mark(V);for(int u:nodes)for(int j=fw.off[u];j<fw.off[u+1];++j){int v=fw.edge[j].to;if(separator_region[v]==parent&&separator_leaf[u]!=separator_leaf[v])mark[separator_leaf[u]==hi?u:v]=1;}
    vector<int> hubs;for(int v:nodes)if(mark[v]){separator_leaf[v]=bar;hubs.push_back(v);}ss.count=hubs.size();if(sparse_sep_collecting){for(int root:hubs)sep_label_root(root);return;}ss.from.resize((size_t)V*ss.count);ss.to.resize((size_t)V*ss.count);for(int k=0;k<ss.count;++k){auto a=subregion_sssp(hubs[k],fw,parent),b=subregion_sssp(hubs[k],rv,parent);for(int v:nodes){ss.from[(size_t)v*ss.count+k]=a[v];ss.to[(size_t)v*ss.count+k]=b[v];}}
}

static void build_subseparator(int8_t top_side,SubSeparator& ss){
    ss.side=top_side;vector<int> nodes;for(int v=0;v<V;++v)if(separator_side[v]==top_side)nodes.push_back(v);
    int xmin=gx[nodes[0]],xmax=xmin,ymin=gy[nodes[0]],ymax=ymin;for(int v:nodes){xmin=std::min(xmin,gx[v]);xmax=std::max(xmax,gx[v]);ymin=std::min(ymin,gy[v]);ymax=std::max(ymax,gy[v]);}
    bool xa=(int64_t)xmax-xmin>=(int64_t)ymax-ymin;vector<int> vals;vals.reserve(nodes.size());for(int v:nodes)vals.push_back(xa?gx[v]:gy[v]);std::nth_element(vals.begin(),vals.begin()+vals.size()/2,vals.end());int cut=vals[vals.size()/2];
    for(int v:nodes)separator_region[v]=((xa?gx[v]:gy[v])<cut)?top_side:2*top_side;
    vector<uint8_t> mark(V);for(int u:nodes)for(int j=fw.off[u];j<fw.off[u+1];++j){int v=fw.edge[j].to;if(separator_side[v]==top_side&&separator_region[u]!=separator_region[v])mark[separator_region[u]==2*top_side?u:v]=1;}
    vector<int> hubs;for(int v:nodes)if(mark[v]){separator_region[v]=3*top_side;hubs.push_back(v);}ss.count=hubs.size();if(sparse_sep_collecting){for(int root:hubs)sep_label_root(root);return;}ss.from.resize((size_t)V*ss.count);ss.to.resize((size_t)V*ss.count);
    for(int k=0;k<ss.count;++k){auto a=region_sssp(hubs[k],fw,top_side),b=region_sssp(hubs[k],rv,top_side);for(int v:nodes){ss.from[(size_t)v*ss.count+k]=a[v];ss.to[(size_t)v*ss.count+k]=b[v];}}
}

static void baseline_build_separator(){
    separator_side.assign(V,0);int xmin=gx[0],xmax=xmin,ymin=gy[0],ymax=ymin;for(int v=1;v<V;++v){xmin=std::min(xmin,gx[v]);xmax=std::max(xmax,gx[v]);ymin=std::min(ymin,gy[v]);ymax=std::max(ymax,gy[v]);}
    bool xa=(int64_t)xmax-xmin>=(int64_t)ymax-ymin;vector<int> vals(V);for(int v=0;v<V;++v)vals[v]=xa?gx[v]:gy[v];std::nth_element(vals.begin(),vals.begin()+V/2,vals.end());int cut=vals[V/2];for(int v=0;v<V;++v)separator_side[v]=((xa?gx[v]:gy[v])<cut)?-1:1;
    // One endpoint per crossing edge is sufficient: choosing its positive-side
    // endpoint is a vertex cover of all median-plane arcs and halves the table.
    vector<uint8_t> sep(V);for(int u=0;u<V;++u)for(int j=fw.off[u];j<fw.off[u+1];++j){int v=fw.edge[j].to;if(separator_side[u]!=separator_side[v])sep[separator_side[u]>0?u:v]=1;}
    vector<int> hubs;for(int v=0;v<V;++v)if(sep[v]){separator_side[v]=0;hubs.push_back(v);}separator_count=hubs.size();
    if(sparse_sep_collecting){for(int root:hubs)sep_label_root(root);}else{
    separator_from.resize((size_t)V*separator_count);separator_to.resize((size_t)V*separator_count);
    for(int k=0;k<separator_count;++k){auto a=full_sssp(hubs[k],fw),b=full_sssp(hubs[k],rv);for(int v=0;v<V;++v){separator_from[(size_t)v*separator_count+k]=a[v];separator_to[(size_t)v*separator_count+k]=b[v];}}
    }
    separator_region.assign(V,0);build_subseparator(-1,subsep[0]);build_subseparator(1,subsep[1]);separator_leaf=separator_region;int8_t parents[4]={-2,-1,1,2};for(int i=0;i<4;++i)build_leafseparator(parents[i],i); separator_micro.assign(V,0); for(int i=0;i<8;++i) build_microseparator(i+1,i);
}

static void build_separator(){
 sparse_sep_in.assign(V,{});sparse_sep_out.assign(V,{});sparse_sep_start=std::chrono::steady_clock::now();
 baseline_build_separator();
 build_road_corridor_levels();
 std::sort(sparse_sep_pending.begin(),sparse_sep_pending.end(),[](int a,int b){int la=road_corridor_level.empty()?0:road_corridor_level[a],lb=road_corridor_level.empty()?0:road_corridor_level[b];if(la!=lb)return la>lb;int da=fw.off[a+1]-fw.off[a]+rv.off[a+1]-rv.off[a],db=fw.off[b+1]-fw.off[b]+rv.off[b+1]-rv.off[b];return da!=db?da>db:a<b;});
 sparse_sep_collecting=false;for(int root:sparse_sep_pending)sep_label_root(root);
 double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-sparse_sep_start).count();
 std::fprintf(stderr,"SPARSE_SEP done roots %d labels %llu pops %llu scans %llu checks %llu time %.6f failed %d\n",sparse_sep_roots,(unsigned long long)sparse_sep_count,(unsigned long long)sparse_sep_pops,(unsigned long long)sparse_sep_scans,(unsigned long long)sparse_sep_checks,elapsed,sparse_sep_failed);
 if(sparse_sep_failed){sparse_sep_in.clear();sparse_sep_out.clear();baseline_build_separator();}
 else sparse_sep_active=true;
}static u64 separator_answer(int s,int t){if(sparse_sep_active)return sparse_sep_answer(s,t);u64 z=INF;size_t a=(size_t)s*separator_count,b=(size_t)t*separator_count;for(int k=0;k<separator_count;++k){u64 x=separator_to[a+k],y=separator_from[b+k];if(x!=INF&&y!=INF)z=std::min(z,x+y);}return z;}
static u64 subseparator_answer(int s,int t,int8_t side){if(sparse_sep_active)return INF;SubSeparator& ss=subsep[side>0];u64 z=INF;size_t a=(size_t)s*ss.count,b=(size_t)t*ss.count;for(int k=0;k<ss.count;++k){u64 x=ss.to[a+k],y=ss.from[b+k];if(x!=INF&&y!=INF)z=std::min(z,x+y);}return z;}
static u64 leafseparator_answer(int s,int t,int8_t parent){if(sparse_sep_active)return INF;int index=parent==-2?0:parent==-1?1:parent==1?2:3;SubSeparator& ss=leafsep[index];u64 z=INF;size_t a=(size_t)s*ss.count,b=(size_t)t*ss.count;for(int k=0;k<ss.count;++k){u64 x=ss.to[a+k],y=ss.from[b+k];if(x!=INF&&y!=INF)z=std::min(z,x+y);}return z;}
static u64 microseparator_answer(int s,int t,int8_t parent){if(sparse_sep_active)return INF;int index=parent-1;MicroSeparator& ss=microsep[index];u64 z=INF;size_t a=(size_t)s*ss.count,b=(size_t)t*ss.count;for(int k=0;k<ss.count;++k){u64 x=ss.to[a+k],y=ss.from[b+k];if(x!=INF&&y!=INF)z=std::min(z,x+y);}return z;}

static void grouped_side(const vector<int>& ids){
    ++epoch;hf.clear();int root=qs[ids[0]].t;int8_t side=separator_leaf[root];vector<int> touched;int remaining=0;
    for(int id:ids){int x=qs[id].s;if(head[x]==-1){touched.push_back(x);++remaining;}nextq[id]=head[x];head[x]=id;}
    setf(root,0);hf.push(0,root);
    while(!hf.empty()&&remaining){auto[d,u]=hf.pop();if(d!=getf(u))continue;if(head[u]!=-1){for(int id=head[u];id!=-1;id=nextq[id])ans[id]=ans[id]<0?(int64_t)d:std::min(ans[id],(int64_t)d);head[u]=-1;--remaining;}for(int j=rv.off[u];j<rv.off[u+1];++j){Arc e=rv.edge[j];if(separator_leaf[e.to]!=side)continue;u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd,e.to);}}}
    for(int x:touched)head[x]=-1;
}

// Multi-goal ALT bounds restricted searches by the nearest endpoint in a
// same-target, same-leaf query group. For each directed landmark, the minimum
// source-to-landmark and maximum landmark-to-source values give consistent
// lower bounds to every goal in the group; their maximum remains consistent.
static vector<u64> grouped_hcache;
static vector<uint32_t> grouped_hstamp;
static uint32_t grouped_epoch=0;
static void grouped_side_alt(const vector<int>& ids){
    ++epoch;hf.clear();if(grouped_hcache.size()!=V){grouped_hcache.resize(V);grouped_hstamp.assign(V,0);}if(++grouped_epoch==0){std::fill(grouped_hstamp.begin(),grouped_hstamp.end(),0);++grouped_epoch;}
    vector<u64> min_to(landmark_count,INF),max_from(landmark_count,0);
    int root=qs[ids[0]].t;int8_t side=separator_leaf[root];vector<int> touched;int remaining=0;
    for(int id:ids){int x=qs[id].s;if(head[x]==-1){touched.push_back(x);++remaining;}nextq[id]=head[x];head[x]=id;}
    for(int id:ids){int s=qs[id].s;for(int k=0;k<landmark_count;++k){size_t z=(size_t)k*V;u64 a=to_landmark[z+s],b=from_landmark[z+s];min_to[k]=std::min(min_to[k],a);max_from[k]=std::max(max_from[k],b);}}
    auto heuristic=[&](int v)->u64{if(grouped_hstamp[v]==grouped_epoch)return grouped_hcache[v];u64 h=0;for(int k=0;k<landmark_count;++k){size_t z=(size_t)k*V;u64 a=to_landmark[z+v],b=from_landmark[z+v];if(a!=INF&&min_to[k]!=INF&&min_to[k]>a)h=std::max(h,min_to[k]-a);if(b!=INF&&max_from[k]!=INF&&b>max_from[k])h=std::max(h,b-max_from[k]);}grouped_hstamp[v]=grouped_epoch;grouped_hcache[v]=h;return h;};
    setf(root,0);hf.push(heuristic(root),root);
    while(!hf.empty()&&remaining){auto[key,u]=hf.pop();u64 d=getf(u);if(d==INF||key!=d+heuristic(u))continue;if(head[u]!=-1){for(int id=head[u];id!=-1;id=nextq[id])ans[id]=ans[id]<0?(int64_t)d:std::min(ans[id],(int64_t)d);head[u]=-1;--remaining;}for(int j=rv.off[u];j<rv.off[u+1];++j){Arc e=rv.edge[j];if(separator_leaf[e.to]!=side)continue;u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd+heuristic(e.to),e.to);}}}
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

#ifndef CH_ALL_UNDIRECTED
#define CH_ALL_UNDIRECTED 0
#endif

struct CHEdge { int to; u64 w; };
static vector<vector<CHEdge>> chf,chb;
static vector<int> chr;
static bool build_road_corridor_levels(){
    if(!graph_directed||gx.empty())return false;
    int n=(int)std::sqrt((double)V);if(n*n!=V)return false;
    vector<vector<double>> horizontal(n),vertical(n);
    for(int u=0;u<V;++u){int ur=u/n,uc=u%n;for(int j=fw.off[u];j<fw.off[u+1];++j){Arc e=fw.edge[j];int vr=e.to/n,vc=e.to%n,dr=std::abs(ur-vr),dc=std::abs(uc-vc);if(dr>1||dc>1||(dr==0&&dc==0))return false;if(dr==0&&dc==1){double dx=(double)gx[u]-gx[e.to],dy=(double)gy[u]-gy[e.to];horizontal[ur].push_back((uint32_t)e.w/std::hypot(dx,dy));}else if(dr==1&&dc==0){double dx=(double)gx[u]-gx[e.to],dy=(double)gy[u]-gy[e.to];vertical[uc].push_back((uint32_t)e.w/std::hypot(dx,dy));}}}
    vector<double> hm(n),vm(n),all;all.reserve(2*n);
    for(int i=0;i<n;++i){if(horizontal[i].empty()||vertical[i].empty())return false;auto&a=horizontal[i];std::nth_element(a.begin(),a.begin()+a.size()/2,a.end());hm[i]=a[a.size()/2];auto&b=vertical[i];std::nth_element(b.begin(),b.begin()+b.size()/2,b.end());vm[i]=b[b.size()/2];all.push_back(hm[i]);all.push_back(vm[i]);}
    std::nth_element(all.begin(),all.begin()+all.size()/2,all.end());double base=all[all.size()/2];vector<uint8_t> hc(n),vc(n);int fast=0,highway=0;
    for(int i=0;i<n;++i){double z=std::min(hm[i],vm[i]);(void)z;hc[i]=hm[i]<base*0.58?1:0;vc[i]=vm[i]<base*0.58?1:0;if(hm[i]<base*0.22)hc[i]=2;if(vm[i]<base*0.22)vc[i]=2;fast+=(hc[i]>0)+(vc[i]>0);highway+=(hc[i]==2)+(vc[i]==2);}
    if(fast<2||fast>n/2||highway==0)return false;
    road_corridor_level.resize(V);int classes[3]={0,0,0};
    for(int u=0;u<V;++u){uint8_t z=std::max(hc[u/n],vc[u%n]);road_corridor_level[u]=z;++classes[z];}
    std::fprintf(stderr,"ROAD_CORRIDOR n %d baseline %.6g fast_lines %d highway_lines %d levels %d/%d/%d\n",n,base,fast,highway,classes[0],classes[1],classes[2]);return true;
}

static bool build_ch(){
    vector<vector<CHEdge>> out(V),in(V);
    for(int u=0;u<V;++u)for(int j=fw.off[u];j<fw.off[u+1];++j){auto e=fw.edge[j];if(u!=e.to){out[u].push_back({e.to,(u64)e.w});in[e.to].push_back({u,(u64)e.w});}}
    chf.resize(V);chb.resize(V);chr.assign(V,-1);vector<int> level(V);
    using Entry=std::pair<u64,int>;std::priority_queue<Entry,vector<Entry>,std::greater<Entry>> pq;
    auto priority=[&](int u){u64 base=(u64)out[u].size()*in[u].size()+8*level[u]+out[u].size()+in[u].size();return base+(road_corridor_level.empty()?0:(u64)road_corridor_level[u]*10000000000000000ULL);};
    for(int u=0;u<V;++u)pq.push({priority(u),u});
    vector<u64> dist(V);vector<uint32_t> seen(V),target_stamp(V);uint32_t tick=0;RadixHeap heap;
    size_t shortcuts=0;int rank=0;
    while(!pq.empty()){
        auto [old,u]=pq.top();pq.pop();if(chr[u]>=0)continue;u64 p=priority(u);if(!pq.empty()&&p>pq.top().first){pq.push({p,u});continue;}
        chf[u]=out[u];chb[u]=in[u];
        vector<std::pair<int,CHEdge>> add;
        for(auto a:in[u]){
            if(++tick==0){std::fill(seen.begin(),seen.end(),0);std::fill(target_stamp.begin(),target_stamp.end(),0);++tick;}
            heap.clear();seen[a.to]=tick;dist[a.to]=0;heap.push(0,a.to);u64 maxd=0;
            int remaining=0;
            for(auto b:out[u])if(a.to!=b.to){
                maxd=std::max(maxd,a.w+b.w);
                if(target_stamp[b.to]!=tick){target_stamp[b.to]=tick;++remaining;}
            }
            int settled=0;
            while(!heap.empty()&&remaining){
                auto[d,v]=heap.pop();if(d>maxd)break;if(seen[v]!=tick||d!=dist[v])continue;if(++settled>200)break;
                // A settled relevant endpoint is final even if its distance
                // exceeds its own shortcut threshold. Once all are final,
                // further expansion cannot change any shortcut decision.
                if(target_stamp[v]==tick){target_stamp[v]=0;if(--remaining==0)break;}
                for(auto e:out[v]){if(e.to==u)continue;u64 nd=d+e.w;if(nd<=maxd&&(seen[e.to]!=tick||nd<dist[e.to])){seen[e.to]=tick;dist[e.to]=nd;heap.push(nd,e.to);}}
            }
            for(auto b:out[u])if(a.to!=b.to){u64 via=a.w+b.w;if(seen[b.to]!=tick||dist[b.to]>via)add.push_back({a.to,{b.to,via}});}
        }
        chr[u]=rank++;
        for(auto a:in[u]){auto& v=out[a.to];v.erase(std::remove_if(v.begin(),v.end(),[&](CHEdge e){return e.to==u;}),v.end());level[a.to]=std::max(level[a.to],level[u]+1);}
        for(auto b:out[u]){auto& v=in[b.to];v.erase(std::remove_if(v.begin(),v.end(),[&](CHEdge e){return e.to==u;}),v.end());level[b.to]=std::max(level[b.to],level[u]+1);}
        for(auto a:add){int s=a.first,t=a.second.to;u64 w=a.second.w;bool found=false;for(auto& e:out[s])if(e.to==t){e.w=std::min(e.w,w);found=true;break;}if(found){for(auto& e:in[t])if(e.to==s){e.w=std::min(e.w,w);break;}}else{out[s].push_back({t,w});in[t].push_back({s,w});++shortcuts;}}
        vector<CHEdge>().swap(out[u]);vector<CHEdge>().swap(in[u]);
        if(rank%10000==0)std::fprintf(stderr,"CH rank %d shortcuts %zu priority %llu\n",rank,shortcuts,(unsigned long long)p);
        if(shortcuts>15000000){std::fprintf(stderr,"CH abort fill\n");return false;}
    }
    std::fprintf(stderr,"CH built shortcuts %zu\n",shortcuts);return true;
}
static int64_t ch_query(int s,int t){
    if(s==t)return 0;++epoch;hf.clear();hb.clear();setf(s,0);setb(t,0);hf.push(0,s);hb.push(0,t);u64 best=INF;
    while(!hf.empty()||!hb.empty()){
        u64 af=hf.empty()?INF:hf.min_key(),ab=hb.empty()?INF:hb.min_key();if(af>=best&&ab>=best)break;
        bool forward=af<=ab;auto [d,u]=forward?hf.pop():hb.pop();if(d!=(forward?getf(u):getb(u)))continue;
        u64 other=forward?getb(u):getf(u);if(other!=INF)best=std::min(best,d+other);
        bool stalled=false;for(auto e:(forward?chb[u]:chf[u])){u64 z=forward?getf(e.to):getb(e.to);if(z!=INF&&z+e.w<d){stalled=true;break;}}if(stalled)continue;
        for(auto e:(forward?chf[u]:chb[u])){u64 nd=d+e.w;if(nd<best&&nd<(forward?getf(e.to):getb(e.to))){if(forward){setf(e.to,nd);hf.push(nd,e.to);}else{setb(e.to,nd);hb.push(nd,e.to);}}}
    }
    return best==INF?-1:(int64_t)best;
}


struct HubLabel { int hub; u64 d; };
static vector<vector<HubLabel>> hub_in,hub_out;
static bool build_hub_labels(){
    hub_in.resize(V);hub_out.resize(V);vector<int> order(V);for(int v=0;v<V;++v)order[v]=v;
    std::sort(order.begin(),order.end(),[](int a,int b){int da=fw.off[a+1]-fw.off[a]+rv.off[a+1]-rv.off[a],dbb=fw.off[b+1]-fw.off[b]+rv.off[b+1]-rv.off[b];return da!=dbb?da>dbb:a<b;});
    vector<u64> cache(V,INF);size_t labels=0;auto start=std::chrono::steady_clock::now();
    for(int k=0;k<V;++k){int root=order[k];
        for(int direction=0;direction<2;++direction){bool forward=direction==0;const CSR& g=forward?fw:rv;
            auto& rootlabels=forward?hub_out[root]:hub_in[root];for(auto e:rootlabels)cache[e.hub]=e.d;
            ++epoch;hf.clear();setf(root,0);hf.push(0,root);
            while(!hf.empty()){
                auto[d,u]=hf.pop();if(d!=getf(u))continue;
                auto& list=forward?hub_in[u]:hub_out[u];bool covered=false;
                for(auto e:list)if(cache[e.hub]!=INF&&cache[e.hub]+e.d<=d){covered=true;break;}
                if(covered)continue;list.push_back({k,d});++labels;
                for(int j=g.off[u];j<g.off[u+1];++j){auto e=g.edge[j];u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd,e.to);}}
            }
            for(auto e:rootlabels)cache[e.hub]=INF;
        }
        if(k%1000==0)std::fprintf(stderr,"PLL hubs %d labels %zu seconds %.3f\n",k,labels,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
        if(labels>100000000||std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()>180){std::fprintf(stderr,"PLL abort resource budget\n");hub_in.clear();hub_out.clear();return false;}
    }
    std::fprintf(stderr,"PLL built labels %zu seconds %.6f\n",labels,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());return true;
}
static int64_t hub_query(int s,int t){if(s==t)return 0;u64 best=INF;auto&a=hub_out[s];auto&b=hub_in[t];size_t i=0,j=0;while(i<a.size()&&j<b.size()){if(a[i].hub<b[j].hub)++i;else if(a[i].hub>b[j].hub)++j;else{best=std::min(best,a[i].d+b[j].d);++i;++j;}}return best==INF?-1:(int64_t)best;}


static void run_queries(const char* qpath, const char* opath) {
    Input in(qpath); int Q=(int)in.integer(); qs.resize(Q); ans.assign(Q,-1); nextq.resize(Q); head.assign(V,-1);
    std::unordered_map<int,vector<int>> bys, byt; bys.reserve(Q/2); byt.reserve(Q/2);
    for(int i=0;i<Q;++i){ int s=(int)in.integer(),t=(int)in.integer(); qs[i]={s,t}; if(s==t) ans[i]=0; else {bys[s].push_back(i);byt[t].push_back(i);} }
    // Scale-free graphs use the work-balanced bidirectional path below.  A
    // previous integrated checkpoint routed them through hub labels; that
    // was exact but substantially slower on the large workload.
    double structural_qpv=double(Q)/V;
    double structural_degree=double(fw.edge.size())/V;
    if(graph_directed&&!gx.empty()&&structural_degree>4.5&&structural_qpv<5.0)build_road_corridor_levels();
    if(graph_directed&&!gx.empty()&&structural_degree>4.5&&structural_qpv<5.0)sparsify_directed_road();
    // Road-family dispatch uses only observable directedness, coordinates,
    // average degree, and query density. Scale-free directed graphs fall back
    // to the label/grouping path below.
    if(graph_directed && !gx.empty() && structural_degree>4.5 && double(Q)/V<5.0 && build_ch()){
        FILE* out=std::fopen(opath,"wb");
        if(!out){std::fprintf(stderr,"cannot open output\n");std::exit(1);}
        for(int i=0;i<Q;++i) std::fprintf(out,"%lld\n",(long long)ch_query(qs[i].s,qs[i].t));
        std::fclose(out); return;
    }
    if(!gx.empty()&&double(Q)/V>=5.0){
        if(graph_directed)build_landmarks(12);build_separator();vector<uint8_t> handled(Q);for(int i=0;i<Q;++i)if(ans[i]<0){int s=qs[i].s,t=qs[i].t;u64 z=separator_answer(s,t);if(separator_side[s]==separator_side[t]&&separator_side[s]!=0)z=std::min(z,subseparator_answer(s,t,separator_side[s]));if(separator_region[s]==separator_region[t]&&separator_region[s]!=0&&std::abs((int)separator_region[s])<=2)z=std::min(z,leafseparator_answer(s,t,separator_region[s])); if(separator_leaf[s]==separator_leaf[t]&&separator_micro[s]>=41&&separator_micro[s]<61&&separator_micro[t]>=41&&separator_micro[t]<61&&separator_micro[s]!=separator_micro[t])z=std::min(z,microseparator_answer(s,t,separator_leaf[s]));ans[i]=z==INF?-1:(int64_t)z;if(separator_side[s]!=separator_side[t]||separator_side[s]==0||separator_region[s]!=separator_region[t]||std::abs((int)separator_region[s])==3||separator_leaf[s]!=separator_leaf[t]||separator_leaf[s]>=20||(separator_micro[s]>=41&&separator_micro[s]<61&&separator_micro[t]>=41&&separator_micro[t]<61&&separator_micro[s]!=separator_micro[t]))handled[i]=1;}
        for(auto& kv:byt){std::array<vector<int>,8> part;for(int id:kv.second)if(!handled[id])part[separator_leaf[qs[id].s]-1].push_back(id);for(auto& ids:part)if(!ids.empty())grouped_side_alt(ids);for(int id:kv.second)handled[id]=1;}
        FILE* out=std::fopen(opath,"wb");if(!out){std::fprintf(stderr,"cannot open output\n");std::exit(1);}char buf[64];for(auto x:ans){int n=std::snprintf(buf,sizeof(buf),"%lld\n",(long long)x);std::fwrite(buf,1,n,out);}std::fclose(out);return;
    }
    double qpv=double(Q)/V,avgdeg=double(fw.edge.size())/V;
    if(!graph_directed){if(torus2_side&&!lattice_side)build_local_grid_landmarks();else build_landmarks(torus3_side?128:24);}
    else if(qpv>=0.5&&qpv<5.0&&avgdeg<5.0)build_landmarks(12);
    vector<uint8_t> done(Q,0);
    // A reverse grouped search is useful when target batches are compact. On
    // strongly skewed reverse-degree graphs, however, one batch can expand a
    // huge predecessor region; retain per-query work-balanced search there.
    for(auto& kv:byt) if(graph_directed&&!balance_by_arc_work&&kv.second.size()>=4){ grouped(kv.second,false); for(int id:kv.second)done[id]=1; }
    // Balanced-source huge-Q workloads amortize one search over many targets.
    for(auto& kv:bys){ vector<int> ids; if(kv.second.size()>=4){ for(int id:kv.second)if(!done[id])ids.push_back(id); if(ids.size()>=4){grouped(ids,true);for(int id:ids)done[id]=1;} } }
    for(int i=0;i<Q;++i) if(ans[i]<0 && !done[i]) ans[i]=(!local_grid_landmark.empty()?local_grid_astar(qs[i].s,qs[i].t):landmark_count?(!graph_directed?(lattice_side?forward_alt8(qs[i].s,qs[i].t):balanced_alt(qs[i].s,qs[i].t)):alt_astar(qs[i].s,qs[i].t)):bidijkstra(qs[i].s,qs[i].t));
    FILE* out=std::fopen(opath,"wb"); if(!out){std::fprintf(stderr,"cannot open output\n");std::exit(1);} char buf[64];
    for(auto x:ans){int n=std::snprintf(buf,sizeof(buf),"%lld\n",(long long)x);std::fwrite(buf,1,n,out);} std::fclose(out);
}

int main(int argc,char**argv){if(argc!=4){std::fprintf(stderr,"usage: %s <graph_file> <query_file> <output_file>\n",argv[0]);return 1;} read_graph(argv[1]);sparsify_undirected();df.resize(V);db.resize(V);current_fkey.resize(V);sf.assign(V,0);sb.assign(V,0);run_queries(argv[2],argv[3]);return 0;}

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
    bool directed = flags & 2;
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
    u64 best = INF;
    while (!hf.empty() && !hb.empty()) {
        u64 af = hf.min_key(), ab = hb.min_key();
        if (af + ab >= best) break;
        if (af <= ab) {
            auto [d,u] = hf.pop(); if (d != getf(u)) continue;
            u64 od = getb(u); if (od != INF && d + od < best) best = d + od;
            for (int j=fw.off[u]; j<fw.off[u+1]; ++j) { Arc e=fw.edge[j]; u64 nd=d+(uint32_t)e.w;
                if (nd < getf(e.to)) { setf(e.to,nd); hf.push(nd,e.to); u64 z=getb(e.to); if(z!=INF && nd+z<best) best=nd+z; }
            }
        } else {
            auto [d,u] = hb.pop(); if (d != getb(u)) continue;
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
static vector<int8_t> separator_side;
static vector<u64> separator_from, separator_to;
static int separator_count=0;

static vector<u64> full_sssp(int root,const CSR& g){vector<u64>d(V,INF);RadixHeap h;d[root]=0;h.push(0,root);while(!h.empty()){auto[du,u]=h.pop();if(du!=d[u])continue;for(int j=g.off[u];j<g.off[u+1];++j){Arc e=g.edge[j];u64 nd=du+(uint32_t)e.w;if(nd<d[e.to]){d[e.to]=nd;h.push(nd,e.to);}}}return d;}

static void build_separator(){
    separator_side.assign(V,0);int xmin=gx[0],xmax=xmin,ymin=gy[0],ymax=ymin;for(int v=1;v<V;++v){xmin=std::min(xmin,gx[v]);xmax=std::max(xmax,gx[v]);ymin=std::min(ymin,gy[v]);ymax=std::max(ymax,gy[v]);}
    bool xa=(int64_t)xmax-xmin>=(int64_t)ymax-ymin;vector<int> vals(V);for(int v=0;v<V;++v)vals[v]=xa?gx[v]:gy[v];std::nth_element(vals.begin(),vals.begin()+V/2,vals.end());int cut=vals[V/2];for(int v=0;v<V;++v)separator_side[v]=((xa?gx[v]:gy[v])<cut)?-1:1;
    // One endpoint per crossing edge is sufficient: choosing its positive-side
    // endpoint is a vertex cover of all median-plane arcs and halves the table.
    vector<uint8_t> sep(V);for(int u=0;u<V;++u)for(int j=fw.off[u];j<fw.off[u+1];++j){int v=fw.edge[j].to;if(separator_side[u]!=separator_side[v])sep[separator_side[u]>0?u:v]=1;}
    vector<int> hubs;for(int v=0;v<V;++v)if(sep[v]){separator_side[v]=0;hubs.push_back(v);}separator_count=hubs.size();
    separator_from.resize((size_t)V*separator_count);separator_to.resize((size_t)V*separator_count);
    for(int k=0;k<separator_count;++k){auto a=full_sssp(hubs[k],fw),b=full_sssp(hubs[k],rv);for(int v=0;v<V;++v){separator_from[(size_t)v*separator_count+k]=a[v];separator_to[(size_t)v*separator_count+k]=b[v];}}
}

static u64 separator_answer(int s,int t){u64 z=INF;size_t a=(size_t)s*separator_count,b=(size_t)t*separator_count;for(int k=0;k<separator_count;++k){u64 x=separator_to[a+k],y=separator_from[b+k];if(x!=INF&&y!=INF)z=std::min(z,x+y);}return z;}

static void grouped_side(const vector<int>& ids){
    ++epoch;hf.clear();int root=qs[ids[0]].t;int8_t side=separator_side[root];vector<int> touched;int remaining=0;
    for(int id:ids){int x=qs[id].s;if(head[x]==-1){touched.push_back(x);++remaining;}nextq[id]=head[x];head[x]=id;}
    setf(root,0);hf.push(0,root);
    while(!hf.empty()&&remaining){auto[d,u]=hf.pop();if(d!=getf(u))continue;if(head[u]!=-1){for(int id=head[u];id!=-1;id=nextq[id])ans[id]=ans[id]<0?(int64_t)d:std::min(ans[id],(int64_t)d);head[u]=-1;--remaining;}for(int j=rv.off[u];j<rv.off[u+1];++j){Arc e=rv.edge[j];if(separator_side[e.to]!=side)continue;u64 nd=d+(uint32_t)e.w;if(nd<getf(e.to)){setf(e.to,nd);hf.push(nd,e.to);}}}
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
        build_separator();vector<uint8_t> handled(Q);for(int i=0;i<Q;++i)if(ans[i]<0){u64 z=separator_answer(qs[i].s,qs[i].t);ans[i]=z==INF?-1:(int64_t)z;if(separator_side[qs[i].s]!=separator_side[qs[i].t]||separator_side[qs[i].s]==0)handled[i]=1;}
        for(auto& kv:byt){vector<int> neg,pos;for(int id:kv.second)if(!handled[id])((separator_side[qs[id].s]<0)?neg:pos).push_back(id);if(!neg.empty())grouped_side(neg);if(!pos.empty())grouped_side(pos);for(int id:kv.second)handled[id]=1;}
        FILE* out=std::fopen(opath,"wb");if(!out){std::fprintf(stderr,"cannot open output\n");std::exit(1);}char buf[64];for(auto x:ans){int n=std::snprintf(buf,sizeof(buf),"%lld\n",(long long)x);std::fwrite(buf,1,n,out);}std::fclose(out);return;
    }
    vector<uint8_t> done(Q,0);
    // Repeated targets are especially valuable on the scale-free hub workload.
    for(auto& kv:byt) if(kv.second.size()>=4){ grouped(kv.second,false); for(int id:kv.second)done[id]=1; }
    // Balanced-source huge-Q workloads amortize one search over many targets.
    for(auto& kv:bys){ vector<int> ids; if(kv.second.size()>=4){ for(int id:kv.second)if(!done[id])ids.push_back(id); if(ids.size()>=4){grouped(ids,true);for(int id:ids)done[id]=1;} } }
    for(int i=0;i<Q;++i) if(ans[i]<0 && !done[i]) ans[i]=bidijkstra(qs[i].s,qs[i].t);
    FILE* out=std::fopen(opath,"wb"); if(!out){std::fprintf(stderr,"cannot open output\n");std::exit(1);} char buf[64];
    for(auto x:ans){int n=std::snprintf(buf,sizeof(buf),"%lld\n",(long long)x);std::fwrite(buf,1,n,out);} std::fclose(out);
}

int main(int argc,char**argv){if(argc!=4){std::fprintf(stderr,"usage: %s <graph_file> <query_file> <output_file>\n",argv[0]);return 1;} read_graph(argv[1]);df.resize(V);db.resize(V);sf.assign(V,0);sb.assign(V,0);run_queries(argv[2],argv[3]);return 0;}

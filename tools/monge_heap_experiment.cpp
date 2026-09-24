// Standalone Stage-2 dynamic matrix-minimum experiment.  Not linked into solver.
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <queue>
#include <random>
#include <sys/resource.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using u64 = uint64_t;
static constexpr u64 INF = UINT64_MAX / 4;

struct Answer {
    u64 value = INF;
    int column = -1, row = -1;
    friend bool operator==(const Answer& a, const Answer& b) {
        return std::tie(a.value, a.column, a.row) ==
               std::tie(b.value, b.column, b.row);
    }
};

// One heap item per active row and column block, never per matrix entry.
// Each row/block has a static ordering by (matrix value, column).  Deletions
// are shared and lazy: a stale block winner advances only when it reaches the
// global heap top.  Activation costs O(columns/block_size) heap insertions.
class BlockMongeHeap {
    struct Item {
        u64 value; int column, row, block, position;
        bool operator>(const Item& x) const {
            return std::tie(value, column, row, block, position) >
                   std::tie(x.value, x.column, x.row, x.block, x.position);
        }
    };
    const std::vector<std::vector<u64>>& matrix;
    int cols, block_size, blocks;
    std::vector<std::vector<std::vector<int>>> order;
    std::vector<u64> base;
    std::vector<char> active, finalized;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> heap;
    uint64_t inspected_ = 0, pushes_ = 0, pops_ = 0;

    void push(int row, int block, int position) {
        auto& sequence = order[row][block];
        while (position < (int)sequence.size() && finalized[sequence[position]])
            ++position;
        if (position == (int)sequence.size()) return;
        int column = sequence[position];
        u64 value = matrix[row][column] >= INF - base[row]
                        ? INF : base[row] + matrix[row][column];
        // An unreachable suffix cannot become useful.  Skipping it also gives
        // the same normalized (INF,-1,-1) answer as the reference structure.
        if (value == INF) return;
        heap.push({value, column, row, block, position});
        ++pushes_;
    }

public:
    BlockMongeHeap(const std::vector<std::vector<u64>>& m, int width)
        : matrix(m), cols(m.empty() ? 0 : (int)m[0].size()),
          block_size(width), blocks((cols + width - 1) / width),
          order(m.size(), std::vector<std::vector<int>>(blocks)),
          base(m.size()), active(m.size()), finalized(cols) {
        for (int r = 0; r < (int)m.size(); ++r)
            for (int b = 0; b < blocks; ++b) {
                int lo = b * block_size, hi = std::min(cols, lo + block_size);
                auto& sequence = order[r][b];
                sequence.resize(hi - lo);
                std::iota(sequence.begin(), sequence.end(), lo);
                std::sort(sequence.begin(), sequence.end(), [&](int x, int y) {
                    ++inspected_;
                    return std::tie(matrix[r][x], x) < std::tie(matrix[r][y], y);
                });
            }
    }
    void activate_row(int row, u64 distance) {
        if (active[row]) std::abort();
        active[row] = 1; base[row] = distance;
        for (int b = 0; b < blocks; ++b) push(row, b, 0);
    }
    void finalize_column(int column) { finalized[column] = 1; }
    Answer get_min() {
        while (!heap.empty()) {
            Item item = heap.top();
            if (!finalized[item.column])
                return {item.value, item.column, item.row};
            heap.pop(); ++pops_; ++inspected_;
            push(item.row, item.block, item.position + 1);
        }
        return {};
    }
    uint64_t inspected() const { return inspected_; }
    uint64_t pushes() const { return pushes_; }
    uint64_t pops() const { return pops_; }
};

class BruteForce {
    const std::vector<std::vector<u64>>& matrix;
    std::vector<u64> base;
    std::vector<char> active, finalized;
    uint64_t inspected_ = 0;
public:
    explicit BruteForce(const std::vector<std::vector<u64>>& m)
        : matrix(m), base(m.size()), active(m.size()),
          finalized(m.empty() ? 0 : m[0].size()) {}
    void activate_row(int row, u64 d) { active[row] = 1; base[row] = d; }
    void finalize_column(int column) { finalized[column] = 1; }
    Answer get_min() {
        Answer answer;
        for (int r = 0; r < (int)matrix.size(); ++r) if (active[r])
            for (int c = 0; c < (int)finalized.size(); ++c) if (!finalized[c]) {
                ++inspected_;
                u64 value = matrix[r][c] >= INF - base[r]
                                ? INF : base[r] + matrix[r][c];
                if (std::tie(value, c, r) <
                    std::tie(answer.value, answer.column, answer.row))
                    answer = {value, c, r};
            }
        return answer;
    }
    uint64_t inspected() const { return inspected_; }
};

struct Graph { int vertices; std::vector<std::vector<std::pair<int,int>>> adj; };
static Graph read_graph(const char* path) {
    FILE* f = std::fopen(path, "r"); if (!f) std::abort();
    int v, e, flags; if (std::fscanf(f, "%d%d%d", &v, &e, &flags) != 3) std::abort();
    if (flags & 2) { std::fprintf(stderr, "directed graph is unsupported\n"); std::exit(2); }
    Graph g{v, std::vector<std::vector<std::pair<int,int>>>(v)};
    for (int i=0,a,b,w;i<e;++i) {
        if (std::fscanf(f,"%d%d%d",&a,&b,&w) != 3) std::abort();
        g.adj[a].push_back({b,w});g.adj[b].push_back({a,w});
    }
    std::fclose(f); return g;
}

static std::vector<u64> region_sssp(const Graph& g, int source,
                                    int x0,int y0,int side,int n) {
    std::vector<u64> d(g.vertices, INF);
    using P=std::pair<u64,int>;std::priority_queue<P,std::vector<P>,std::greater<P>>q;
    d[source]=0;q.push({0,source});
    while(!q.empty()){auto [du,u]=q.top();q.pop();if(du!=d[u])continue;
        for(auto [v,w]:g.adj[u]){int x=v%n,y=v/n;if(x<x0||x>=x0+side||y<y0||y>=y0+side)continue;
            u64 nd=du+(uint32_t)w;if(nd<d[v]){d[v]=nd;q.push({nd,v});}}
    } return d;
}

static std::vector<std::vector<std::vector<u64>>>
region_matrices(const Graph& g,int x0,int y0,int side,int n) {
    std::vector<std::vector<int>> a(4);
    for(int x=x0;x<x0+side;++x)a[0].push_back(y0*n+x);
    for(int y=y0+1;y<y0+side;++y)a[1].push_back(y*n+x0+side-1);
    for(int x=x0+side-2;x>=x0;--x)a[2].push_back((y0+side-1)*n+x);
    for(int y=y0+side-2;y>y0;--y)a[3].push_back(y*n+x0);
    std::vector<std::vector<u64>> dist;
    for(auto& arc:a)for(int v:arc)dist.push_back(region_sssp(g,v,x0,y0,side,n));
    std::vector<std::vector<std::vector<u64>>> matrices;
    for(int i=0,offset=0;i<4;offset+=a[i++].size())for(int j=i+1;j<4;++j){
        std::vector<int> cols=a[j];std::reverse(cols.begin(),cols.end());
        std::vector<std::vector<u64>> m(a[i].size(),std::vector<u64>(cols.size()));
        for(int r=0;r<(int)a[i].size();++r)for(int c=0;c<(int)cols.size();++c)m[r][c]=dist[offset+r][cols[c]];
        matrices.push_back(std::move(m));
    } return matrices;
}

struct Totals { uint64_t sequences=0, activations=0, extracts=0, operations=0, fast=0, brute=0, pushes=0, pops=0; };
static void run_sequence(const std::vector<std::vector<u64>>& m,double fraction,
                         bool random_finalization,std::mt19937_64& rng,Totals& totals) {
    if(m.empty()||m[0].empty())return;
    int rows=m.size(),cols=m[0].size();
    int width=std::max(2,(int)std::sqrt(cols));BlockMongeHeap fast(m,width);BruteForce brute(m);
    std::vector<int> order(rows);std::iota(order.begin(),order.end(),0);std::shuffle(order.begin(),order.end(),rng);
    int count=std::max(1,(int)(rows*fraction));
    auto check=[&](){Answer a=fast.get_min(),b=brute.get_min();++totals.operations;++totals.extracts;if(!(a==b)){
        std::fprintf(stderr,"mismatch fast=(%llu,%d,%d) brute=(%llu,%d,%d)\n",
          (unsigned long long)a.value,a.column,a.row,(unsigned long long)b.value,b.column,b.row);std::exit(3);}};
    for(int k=0;k<count;++k){u64 base=rng()%100000;fast.activate_row(order[k],base);brute.activate_row(order[k],base);++totals.activations;check();}
    std::vector<int> columns(cols);std::iota(columns.begin(),columns.end(),0);
    if(random_finalization)std::shuffle(columns.begin(),columns.end(),rng);
    for(int k=0;k<cols;++k){Answer a=fast.get_min();++totals.extracts;if(a.column<0)break;
        int column=random_finalization?columns[k]:a.column;
        fast.finalize_column(column);brute.finalize_column(column);check();}
    totals.sequences++;totals.fast+=fast.inspected();totals.brute+=brute.inspected();totals.pushes+=fast.pushes();totals.pops+=fast.pops();
}

int main(int argc,char**argv){if(argc<2){std::fprintf(stderr,"usage: %s GRAPH [SIDE=16] [REGIONS=8] [REPEATS=20]\n",argv[0]);return 2;}
    int side=argc>2?std::atoi(argv[2]):16,limit=argc>3?std::atoi(argv[3]):8,repeats=argc>4?std::atoi(argv[4]):20;
    Graph g=read_graph(argv[1]);int n=(int)std::sqrt(g.vertices);if(n*n!=g.vertices)return 2;std::mt19937_64 rng(0x4d6f6e6765486561ULL);
    // Explicitly exercise unreachable entries and saturating base addition.
    std::vector<std::vector<u64>> disconnected{{0,INF,7},{INF,3,INF}};
    Totals inf_test;run_sequence(disconnected,1.0,true,rng,inf_test);
    Totals totals[3];auto started=std::chrono::steady_clock::now();int regions=0;
    for(int y=0;y+side<=n&&regions<limit;y+=side)for(int x=0;x+side<=n&&regions<limit;x+=side,++regions){auto matrices=region_matrices(g,x,y,side,n);for(auto&m:matrices)for(int z=0;z<repeats;++z){int fi=0;for(double f:{.25,.5,1.0})run_sequence(m,f,z&1,rng,totals[fi++]);}}
    double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
    rusage usage{};getrusage(RUSAGE_SELF,&usage);
    for(int i=0;i<3;++i){auto&t=totals[i];
        std::printf("active=%d%% sequences=%llu activations=%llu extracts=%llu fast_inspected=%llu brute_inspected=%llu reduction=%.2fx heap_pushes=%llu heap_pops=%llu\n",i==0?25:i==1?50:100,(unsigned long long)t.sequences,(unsigned long long)t.activations,(unsigned long long)t.extracts,(unsigned long long)t.fast,(unsigned long long)t.brute,t.fast?double(t.brute)/t.fast:0.0,(unsigned long long)t.pushes,(unsigned long long)t.pops);}
    uint64_t acts=0,extracts=0;for(auto&t:totals){acts+=t.activations;extracts+=t.extracts;}
    std::printf("regions=%d matrices=%d elapsed=%.3fs activations_per_sec=%.0f extracts_per_sec=%.0f peak_rss=%ldKiB inf_test=pass mismatches=0\n",regions,regions*6,elapsed,acts/elapsed,extracts/elapsed,usage.ru_maxrss);
}

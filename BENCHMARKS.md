# Second-generation solver research

The rollback checkpoint is Git tag `baseline-official-7x` (commit `d347d0e`).
Dispatch in the new solver uses only graph flags, vertex/edge/query counts, and
measured endpoint multiplicities; it never uses paths, filenames, or seeds.

## Observed large workload statistics

| workload | Q | unique sources | unique targets | max source freq | max target freq | targets freq >=4 |
|---|---:|---:|---:|---:|---:|---:|
| road2d | 200,000 | 200,000 | 135,113 | 1 | 126 | 3,190 |
| lattice3d | 30,000 | 30,000 | 28,478 | 1 | 4 | 1 |
| local2d | 300,000 | 300,000 | 188,635 | 1 | 7 | 6,073 |
| scalefree | 50,000 | 50,000 | 33,240 | 1 | 70 | 303 |
| hugeq | 600,000 | 50,176 | 50,176 | 12 | 29 | 50,067 |
| wide64 | 30,000 | 30,000 | 29,775 | 1 | 3 | 0 |

The graph scan confirmed directed road2d/hugeq/scalefree graphs and undirected
lattices; average out-degrees are approximately 5.4, 5.4, 2.9, 2, 3, and 2.
Lattice weights range from small integers on lattice3d/local2d to 1e8--1e9 on
wide64. Road graphs carry coordinates but variable-speed weights. Queries show
no source repetition except hugeq. Target repetition is strongest in hugeq and
in the scale-free hub mixture. The generator and sampled endpoint rank mixes
confirm local2d's predominantly short-range workload; the other query sets are
predominantly global.

## Candidates

| candidate | targeted result | decision |
|---|---|---|
| Directed/undirected ALT, 8 landmarks | weak on scale-free large (>125 s before completion) | reject there; grouped hot targets win |
| Directed ALT, 16 landmarks | road large varied from about 136 s to over 200 s, exact | reject; no reliable end-to-end improvement |
| Undirected ALT, 12 landmarks | local2d large about 100 s, exact | retain; major improvement over the prior roughly 278 s checkpoint |
| ALT, 24 landmarks, per-query A* | hugeq large exceeded 180 s | reject; endpoint grouping is better |
| ALT on sparse-query degree-6 / huge lattices | preprocessing cannot amortize | reject structurally; preserve lattice3d and wide64 paths |
| Existing grouped forward/reverse SSSP | hugeq and hot scale-free targets | retain |

ALT landmarks use farthest-point selection from an observable high-degree
starting vertex. Directed lower bounds combine `d(L,t)-d(L,v)` and
`d(v,L)-d(t,L)`; unreachable terms are ignored. Complete preprocessing time is
included in every timing above.

## Hierarchy research phase 1

| experiment | workload | preprocessing / total | correctness | memory | decision |
|---|---|---|---|---|---|
| Geometric nested-dissection CH, conservative shortcut closure | hugeq-size dev graph, 260k-query trigger | preprocessing about 15 s; 10k nontrivial queries plus 250k self-pairs about 33 s | exact without stall-on-demand (10k/10k); experimental stall variant was incorrect and discarded | comfortably below 4 GB | reject: extrapolated 600k query time is far slower than endpoint grouping despite acceptable preprocessing |
| Greedy minimum-fill CH without witness suppression | hugeq-size graph | exceeded 80 s during preprocessing in exploratory run | query phase not reached | about 275 MB when stopped | reject: uncontrolled fill |

Both CH variants used only observable structure.  The geometric variant proves
that nested-dissection controls preprocessing fill, but its upward search space
is still too large; adding a correct stall implementation would not plausibly
close the several-fold query-time gap observed here.

## Hierarchy research phase 2 — new targeted BEST

A one-level exact geometric separator overlay was implemented for coordinate
graphs with at least five queries per vertex.  The median-plane crossing
endpoints form the separator.  Full forward and reverse distance tables from
all separator vertices answer every cross-separator query exactly.  Same-side
queries take the minimum of that separator route and a reverse grouped Dijkstra
restricted to the corresponding side, which is exact because every path that
leaves the side must touch the separator.

| candidate | workload | preprocessing included | total time | correctness | memory | decision |
|---|---|---:|---:|---|---|---|
| Safe endpoint grouping (`d347d0e`) | hugeq_large | yes | about 290 s | exact | low | baseline on this runner |
| One-level separator overlay + side-restricted grouping | hugeq_large | yes (about 6 s on the same-size dev graph) | about 150 s | exact, all 600,000 official answers | about 0.4 GB tables on same-size graph | **keep: approximately 1.9x targeted improvement** |

The same candidate also matched all 10,000 `hugeq_dev` reference answers when
forced through the overlay using appended self-pairs.  Dispatch uses only
coordinates and Q/V, not filenames, seeds, or exact instance sizes.

### Separator-width and alternative-preprocessing follow-up

Selecting only the positive-side endpoint of every crossing arc is still an
exact vertex-cover separator and halves table memory.  It reduced the forced
10k-query dev run from about 16.1 s to 13.5 s and remained exact.  The official
large run was about 158 s and exact, statistically tied with the two-endpoint
separator run; the half-width version is retained for its lower memory.

Frequency-only full SSSP caching was rejected analytically after measuring the
endpoint distribution: virtually all 50,176 targets already trigger one
reverse grouped search, and a complete cached SSSP costs at least as much as
that early-stopping search.  A source/target mixed cover cannot use fewer than
about 50k roots because the 12-regular-like query bipartite graph has a matching
covering essentially every source.  Source-first grouping was also slower in a
targeted run.  The separator table is therefore the useful hub/partial-table
hybrid: its roughly 224 structural hubs answer cross-region queries while
avoiding tens of thousands of full distance tables.

Fresh-seed validation passed on seed 333333: a newly generated 50,176-vertex
`hugeq_dev` graph was forced through separator dispatch with 250,000 appended
self-pairs, and the first 10,000 nontrivial answers matched `refsolve` exactly.
All six ordinary dev workloads also matched their answer files.

## Combined certification candidate

The proven 12-landmark undirected ALT path and the half-width one-level
separator overlay are now combined.  Dispatch is structural: ALT requires
0.5 <= Q/V < 5 and average degree below 5; the separator requires coordinates
and Q/V >= 5.  All other workloads retain the `d347d0e` algorithms.  All six
dev answer files match before starting the full official certification run.

## Recursive separator phase

A second exact separator level is built independently inside each side of the
top-level cut.  Its forward/reverse tables are restricted to that parent
region.  Cross-quarter queries are answered by the minimum of the top and
second-level tables; same-quarter target groups run only inside one quarter.
This remains exact because any route leaving a quarter crosses either its
parent separator or the top separator.

| candidate | workload | total time incl. preprocessing | correctness | decision |
|---|---|---:|---|---|
| One-level separator | hugeq_large | about 158 s (earlier run) | exact | previous SAFE |
| Two-level recursive separator | hugeq_large | **89.264 s** | exact, all 600,000 answers | **keep; 43.5% faster** |

A forced 10,000-query hugeq_dev run also matched every reference answer.

## Third-level separator phase

Each of the four second-level interiors is split once more, with exact distance
tables restricted to its parent quarter and grouped searches restricted to one
of eight leaves.  An initial run exposed and fixed an edge case where two
first-level separator vertices incorrectly consulted an uninitialized deeper
table; the corrected run matched all 600,000 official answers.

| candidate | workload | total time incl. preprocessing | correctness | decision |
|---|---|---:|---|---|
| Two-level overlay | hugeq_large | 89.264 s | exact | previous BEST |
| Three-level overlay | hugeq_large | **40.340 s** | exact | **keep; 54.8% faster than two-level, ~7.2x faster than safe grouping on this runner** |

The three-level overlay also passed fresh-seed validation on seed 444444: all
10,000 generated nontrivial `hugeq_dev` answers matched `refsolve` when forced
through hierarchy dispatch by appended self-pairs.

Further direct application to `road2d_large` and `wide64_large` was rejected on
memory/preprocessing feasibility rather than filename: their observed V and
median separator widths make even the first pair of uint64 forward/reverse
tables approximately 2.6 GB and 45 GB respectively, before deeper levels.
Scale-free graphs lack a small geometric vertex separator, so the existing
hot-target grouping remains the stronger applicable method.

## Generator-structure analysis for leaderboard-scale research

* **road2d** is an `S x S` row-major lattice with both directions on every
  horizontal/vertical edge, plus only adjacent-cell diagonals.  Coordinates
  jitter inside cells but do not alter this topology.  Entire row/column road
  lines are accelerated periodically: arterial and highway strides are 16/128
  on large and 8/64 on dev.  Thus exact cell boundaries, separators, and a
  contraction order retaining fast lines are seed-independent structural
  features.  Rank queries span 2^8..2^17, so a useful hierarchy must accelerate
  both neighborhood and regional routes; Euclidean A* alone cannot model the
  periodic speeds.
* **2-D lattices** are chord-free, undirected row-major tori of degree four.
  `local2d` has uniform 1..1000 weights and 90% rank-at-most-4096 queries;
  `wide64` has log-uniform 1e8..1e9 weights and ranks 2^12..2^20.  Exact nested
  dissection has O(sqrt(V)) top separators, but dense per-vertex separator
  tables violate the 4 GB limit on wide64; compact shortcut/overlay graphs are
  required.
* **lattice3d** is a degree-six torus.  Its O(V^(2/3)) slab separator makes the
  same dense-table approach substantially less attractive than in 2-D.
* **scalefree** gives every nonsink exactly three outgoing arcs while a Pareto
  in-degree distribution creates the hubs; 30% of queries explicitly target
  the top 0.1% by in-degree.  Exact hub decomposition needs a fallback because
  not every shortest path intersects a selected hub.  Reverse grouping already
  exploits repeated hub targets, so useful labels must eliminate search rather
  than merely provide another heuristic.

### Road2d exact arterial-cell overlay experiment

A CRP-style exact overlay was prototyped after inferring the arterial period
from observable per-row edge-weight averages.  Each arterial-sized grid cell
was replaced by the complete directed distance graph on its boundary; queries
ran restricted searches in the endpoint cells and Dijkstra on the boundary
overlay.  It was exact on all 10,000 road2d_dev answers, but total time was
9.787 s versus 4.029 s for the paired BEST (2.43x slower).  The dense boundary
cliques and large upward search space dominate, so this one-level overlay is
rejected; a competitive road hierarchy needs contraction/witness suppression
or multiple sparse customization levels rather than boundary cliques.

### Scale-free exact hub-route upper-bound experiment

The 32 highest observed in-degree vertices were given complete forward and
reverse distance tables.  Their exact two-hop routes supplied an initial upper
bound to bidirectional Dijkstra, retaining fallback correctness when a shortest
path misses all selected hubs.  On scalefree_dev it was exact but took 7.812 s
versus 7.646 s paired BEST.  Preprocessing plus table probes outweighed pruning,
showing that the generator's high in-degree hubs do not form a sufficiently
complete shortest-path cover.  This exact limited hub-label family is rejected
rather than tuning k for percent-level changes.

### 2-D torus contraction experiment

An exact undirected contraction hierarchy using lazy minimum-degree elimination
and conservative clique shortcuts was tested on the chord-free torus.  The
50,176-vertex dev graph required about 12.826 s of preprocessing alone when
forced through the hierarchy.  On local2d_large, preprocessing had not finished
after 125 s and was stopped, already exceeding the complete retained ALT
runtime.  Although query answers on the completed dev hierarchy were exact,
generic minimum-degree elimination does not scale to the 300k/2M grids.  A
leaderboard-scale grid CH needs an implicit nested-dissection representation or
witness-suppressed shortcuts, not per-vertex unordered-map clique closure.

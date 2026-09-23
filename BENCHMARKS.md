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

## CCH design: topology-first nested dissection

The next candidate is a true customizable contraction hierarchy for observable
chord-free 2-D torus topology.

* **Ordering:** open the torus with two separator rows and two separator
  columns, recursively nested-disect the four resulting rectangles, and order
  each separator line by 1-D nested dissection.  The order depends only on
  topology/row-major coordinates, never weights or a seed.
* **Compact elimination topology:** store higher-ranked neighbors as flat
  integer lists.  At elimination vertex `x`, sort/unique its higher neighbors,
  choose the lowest-ranked neighbor as elimination parent, and propagate the
  remaining neighbors to that parent.  This constructs the chordal completion
  without a hash map per vertex or eager all-pairs shortcut objects.
* **Customization:** initialize each hierarchy edge with separate low-to-high
  and high-to-low uint64 weights, then relax all lower triangles in rank order.
  Separate directions make the same representation reusable for directed road
  metrics later.
* **Query:** exact bidirectional upward search uses low-to-high customized
  weights forward and high-to-low weights backward, meeting at a common rank.
* **Memory:** O(V + E_chordal), with two uint64 weights plus one 32-bit head per
  hierarchy arc after CSR packing.  Arc count and estimated bytes are measured
  on dev before any 2M-vertex wide64 attempt.

This differs fundamentally from the rejected minimum-degree CH: ordering is an
explicit separator hierarchy, topology construction uses elimination-parent
propagation in compact vectors, and metric weights are customized only after
the topology is complete.  It avoids per-vertex unordered maps and online
weighted clique closure.

### Compact nested-dissection CCH prototype results

The designed CCH was implemented and was exact on all 10,000 local2d_dev
queries.  Elimination-parent propagation avoided per-vertex hash maps and the
packed hierarchy used separate uint64 forward/backward weights.  Profiling
showed 2,248,478 upward arcs for 50,176 vertices (about 54 MB packed, roughly
45 arcs/vertex).  Opening the torus with one boundary row/column versus a
balanced double-cross separator produced essentially the same fill (2,260,786
arcs), identifying recursive separator fronts—not torus opening—as the source.

End-to-end dev time was 8.597 s, including about 4.821 s preprocessing, versus
roughly 2--3 s for the retained search.  More importantly, topology construction
plus lower-triangle customization on local2d_large took 127.116 s even with
self queries, already exceeding the complete ~74 s ALT checkpoint before any
real query.  Extrapolated fill is tens of millions of arcs and customization's
sum-of-squared upward degrees is the bottleneck, not packed storage itself.
The full CCH is therefore rejected for these query counts.  A viable successor
must be a **partial** hierarchy that stops before large separator fronts and
runs an exact compact core search, or use witness-suppressed metric shortcuts;
merely changing the top torus cut does not address the measured fill.

## Partial hierarchy + exact core design

The partial candidate retains the same observable 2-D torus nested-dissection
order, but contracts only an initial low-rank prefix.  Contraction stops before
the high-degree separator fronts.  During experimental construction, active
undirected edges carry exact uint64 shortcut weights; only contracted vertices
are clique-closed, after which both upward shortcuts and the remaining core are
packed into flat adjacency arrays.

An exact query consists of two upward searches from the endpoints.  Any path
whose rank peak is contracted is covered by their meeting distance.  Distances
that reach the uncontracted core seed an exact core Dijkstra, which is combined
with all reverse upward labels.  Consequently the core is a correctness
fallback rather than a heuristic.  Core fractions of roughly 50%, 20%, 10%,
and 5% form materially different points on the preprocessing/fill versus core
search tradeoff; only candidates surviving dev correctness and timing proceed
to the 300k-query workload.

### Partial hierarchy/core tradeoff

An exact partial hierarchy was implemented with nested-dissection prefix
contraction, uint64 clique shortcuts only for contracted interiors, packed
upward arcs, and an unrestricted exact Dijkstra on the retained core.  All four
structurally distinct core fractions matched all 10,000 local2d_dev answers.

| core fraction | core vertices | upward arcs | directed core arcs | preprocessing/self-query time | peak RSS | full dev time | decision |
|---:|---:|---:|---:|---:|---:|---:|---|
| 50% | 25,088 | 1,024,572 | 413,084 | 4.050 s | 79 MB | 14.743 s | reject |
| 20% | 10,035 | 1,589,442 | 537,120 | 6.463 s | 104 MB | 18.811 s | reject |
| 10% | 5,017 | 1,753,978 | 589,886 | 6.217 s | 111 MB | 18.826 s | reject |
| 5% | 2,508 | 1,836,104 | 617,724 | 6.297 s | 115 MB | 19.354 s | reject |

The paired retained dev solver is roughly 2--3 s.  The shallowest 50% core has
the best total: deeper contraction reduces core size but adds enough upward and
core shortcuts to make both preprocessing and queries worse.  Even the 50%
point is about six times slower on dev, so none survives screening for a costly
300k-query large run.  A short-query/ALT hybrid cannot repair this: preprocessing
alone exceeds the retained dev total, while the hierarchy would serve only the
10% long-range tail.  This properly explores the requested tradeoff and shows
that exact clique shortcuts at separator fronts remain the architectural
barrier even when half the graph is left uncontracted.  Future hierarchy work
must use metric witness suppression or a boundary representation that avoids
these shortcuts; changing only the core fraction cannot yield a 2x gain.

## Metric partial CH with batched witness searches — design

The next partial hierarchy keeps the exact-core query architecture but replaces
unconditional clique closure with metric witness suppression.  Vertices follow
a topology-only nested-dissection order and contraction stops with a moderate
core.  For each contracted vertex `v`, one bounded Dijkstra is run per active
neighbor `u`, excluding `v`; the search simultaneously resolves every
`u-v-w` candidate and stops beyond the largest candidate weight.  Unresolved
pairs after a settlement budget conservatively receive shortcuts, preserving
exactness.

Experimental construction uses append-only adjacency vectors with inactive
vertices filtered during scans and periodic per-vertex sort/min deduplication;
it does not allocate an unordered map per vertex.  The retained candidate will
pack upward and core arcs before queries.  Measurements include shortcut
candidates, witness-suppressed candidates, inserted shortcuts, preprocessing,
core size/arcs, query time, total time, and RSS.

### Witness-suppressed partial metric CH results

A batched exact witness implementation contracted the nested-dissection prefix
while retaining a 50% exact core.  Each active predecessor ran one bounded
Dijkstra excluding the contracted vertex and answered all successor candidates;
search-budget exhaustion conservatively inserted shortcuts.

| witness settlement cap | core | candidates | suppressed | suppression | inserted | upward arcs | core arcs | preprocessing | peak RSS | full dev | correct |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 256 | 25,088 | 24,350,931 | 23,011,981 | 94.50% | 1,338,950 | 449,098 | 216,014 | 66.833 s | 92 MB | 72.394 s | yes |
| 32 | 25,088 | 40,528,951 | 37,985,963 | 93.72% | 2,542,988 | 580,033 | 267,344 | 45.407 s | 150 MB | not pursued | construction exact |

Witnesses do suppress most individual candidates and roughly halve the upward
arc count versus conservative closure, validating the hypothesis at the graph
level.  Nevertheless, tens of millions of bounded searches make preprocessing
22--29x slower than the retained complete dev solver.  A smaller witness budget
creates more shortcuts, which creates still more later candidate pairs and
higher memory.  Preprocessing alone decisively violates the rejection rule, so
no large run or core-fraction sweep is justified.  Metric witness CH is rejected
for this workload and the next mandated family is exact multilevel Arc Flags.

### Exact Arc Flags feasibility probe

Classical exact region Arc Flags require preserving a shortest path to every
possible entry boundary vertex (or equivalently substantially more expensive
per-target preprocessing); a single multi-source nearest-boundary Dijkstra is
not exact for an arbitrary target inside the region.  On the 224x224 dev torus
with 16x16 cells there are 11,760 boundary vertices.  Running only 128 reverse
SSSP boundary probes took 0.825 s total including load, implying roughly 75 s
for one exact flag level on the tiny dev graph before flag propagation or any
queries.  The corresponding 548x548 large grid has tens of thousands of
boundaries and each SSSP is six times larger.  Larger cells reduce preprocessing
only by making target-region flags too coarse to offer the required 2x query
reduction; multilevel flags add rather than remove boundary preprocessing.

Thus conventional exact Arc Flags cannot amortize here, while the cheap
multi-source approximation would violate exactness.  The family is rejected
before a large run under the explicit preprocessing rejection rule.

## Road-backbone partial metric CH design

The road-specific candidate identifies fast horizontal and vertical lines from
observable per-line mean lattice-edge weights (rather than assuming a manifest
stride).  Their union remains as an exact directed core; only local-street
interiors bounded by consecutive fast lines are contracted.  For each directed
predecessor/successor pair through a contracted vertex, a batched bounded
witness search excludes that vertex.  Budget exhaustion inserts the shortcut,
so correctness never depends on witness success.

Separate upward-forward and upward-reverse arcs preserve asymmetric weights.
Queries search upward from both endpoints and then run an unrestricted exact
directed Dijkstra in the retained arterial/highway core.  This differs from the
failed torus hierarchy because contraction stops at the generator's observed
fast-road backbone and local witness searches operate on small street cells.
Measurements will include detected core fraction, candidates/suppression,
shortcuts, packed arc counts, preprocessing, query time, RSS, and exactness.

### Directed road-backbone partial CH results

The road-specific prototype detected arterial/highway rows and columns from
per-line mean edge weights, retained their union as the core, and contracted
only local-street interiors.  Directed predecessor/successor witnesses used a
64-settlement budget; upward forward/reverse shortcuts and an unrestricted
exact directed core search preserved asymmetry and correctness.

On road2d_dev it retained 11,760 core vertices (23.44%), contracted 38,416,
considered 3,633,810 candidates, suppressed 2,994,558 (82.41%), and inserted
639,252 shortcuts.  The result had 757,849 upward arcs and 84,695 core arcs.
It matched all 10,000 answers, used 77 MB peak RSS, but preprocessing/self-query
time was 6.917 s and total time was 10.546 s versus 4.029 s paired SAFE.
Preprocessing alone exceeds the current complete solver, and large uses a
narrower arterial spacing fraction with six times as many vertices, so it
cannot plausibly reach the required <50 s target.  The candidate is rejected
before a large run under the stated rule.  This directly tests—and rejects—the
hypothesis that the structured fast-road backbone makes witness contraction
cheap enough, rather than extrapolating from the torus result.

## Scale-free low-degree contraction/core design

The scale-free candidate retains the highest observed in-degree vertices as an
exact directed core and contracts the remaining vertices in increasing static
`in-degree * out-degree` importance.  Directed batched witnesses suppress
predecessor/successor shortcuts; separate forward/reverse upward arcs feed an
unrestricted directed core search.  This is an exact partial contraction, not
the previously rejected hub-route upper bound.

### Scale-free low-degree/hub-core contraction results

With a 50% high-in-degree core, the exact directed prototype contracted 25,000
vertices, considered 101,503 shortcut candidates, suppressed only 5,423
(5.34%), inserted 96,080 shortcuts, and produced 117,142 upward plus 124,459
core arcs.  Preprocessing/self-query time was only 0.580 s at 30 MB RSS, but
exact core queries raised total scalefree_dev time to 13.596 s versus 7.646 s
SAFE.  All 10,000 answers matched.

A more aggressive 20% core did not finish preprocessing within 125 s and was
stopped: contracting vertices closer to the heavy in-degree core causes the
predecessor-successor product and shortcut graph to explode.  Thus the shallow
variant has cheap preprocessing but an expensive 25k core, while the deeper
variant crosses the directed fill cliff.  Neither has credible 2x potential,
and this exact hub-core contraction family is rejected without a large run.

## 2-D grid r-division / DDG feasibility estimate

For square regions of `r` vertices, each region has `b = Theta(sqrt(r))`
boundary vertices.  Full boundary-distance matrices total
`(V/r)*b^2 = Theta(V)` uint64 entries per level, while preprocessing by boundary
SSSP costs `Theta(V*sqrt(r))` settles per level.  At r=256 this is roughly 4--5
million distance entries (~35--40 MB) and ~20 million regional settles for
local2d_large; wide64 is roughly 32 million entries (~256 MB) per level.

Those storage/preprocessing figures are feasible, but naïvely exposing each
matrix entry as an overlay arc recreates the already rejected dense CRP graph.
A useful query requires an FR-Dijkstra-style data structure supporting activated
rows and Monge column minima without scanning the boundary clique.  Boundary
matrices for a weighted planar region have the needed cyclic Monge structure
only after splitting boundary order into the appropriate face intervals; a
plain binary heap or CSR does not exploit it.  With such a structure, projected
long-query work is about `sqrt(V) polylog(V)` and is potentially serious for the
10% long local2d tail and 30k wide64 queries.  Without it, 30k--300k queries
scan billions of dense entries.  Therefore no forbidden explicit-clique
prototype was built; the next implementation-worthy grid architecture is a
correct Monge-heap/FR DDG, not another ordinary overlay.

## Coarse pattern-database A* design

For observable row-major 2-D torus topology, vertices are mapped to regular
blocks.  Each directed coarse edge stores the minimum original crossing-edge
weight.  Sparse all-pairs coarse distances are computed by one radix-heap
Dijkstra per block.  The lookup `D(block(v), block(t))` is admissible and
consistent: every original path induces a coarse path whose crossing minima do
not exceed its real crossing costs, while movement inside a block is optimistically
free.  The maximum of several shifted/scaled consistent abstractions remains
consistent.

The first experiment compares exactly four material configurations: one 16x16
partition, one 8x8 partition, four half-block-shifted 16x16 partitions, and a
max of unshifted 8x8 plus 16x16.  Tables use uint64 and sparse preprocessing;
settled work and total time determine whether the family merits road/wide64
adaptation.  No filename, seed, or released-instance identity enters mapping.

### Coarse PDB experiment results (rejected)

The design above was implemented as an isolated candidate and compared on
`local2d_dev` before any large run.  All four candidates returned exactly the
10,000 reference distances.  The measurements below include graph loading,
abstract-graph construction, all-pairs abstract Dijkstra, queries, and output.
The `settled/query` counter counts original vertices removed from the A* queue.

| abstraction | total (s) | preprocessing-only proxy (s) | peak RSS | settled/query | exact |
|---|---:|---:|---:|---:|---|
| one 16x16 partition | 6.130 | 0.091 | 8.4 MiB | 3275.55 | yes |
| one 8x8 partition | 6.031 | 0.195 | 12.7 MiB | 3175.41 | yes |
| four shifted 16x16 partitions | 12.768 | 0.115 | 9.0 MiB | 3259.80 | yes |
| max of unshifted 8x8 and 16x16 | 7.656 | 0.194 | 13.0 MiB | 3175.41 | yes |

The retained solver took 2.245 s and 2.372 s in paired runs on the same input.
Thus even the best abstraction was about 2.6x slower, rather than the required
2x improvement.  Finer blocks only reduced settling by 3.1%; the 16x16 table
was completely dominated by the 8x8 table in the two-scale maximum, and shifts
added lookup work without useful pruning.  The underlying reason is structural:
collapsing a block makes all travel inside it free, while choosing the minimum
crossing edge independently at every boundary creates unrealistically cheap
abstract walks on a random-weight torus.  The resulting lower bound is much
weaker than the retained landmark bounds despite its inexpensive construction.

**Decision:** reject coarse block PDBs, including additional block-size tuning.
The primary family missed the dev threshold decisively, so no costly large run
and no road2d/wide64 port were performed.  The experimental implementation was
removed; the exact three-level hugeq separator, 12-landmark local2d path, and
safe fallbacks remain unchanged.

### Next exact-grid architecture after the PDB result

The remaining plausible grid direction is an implicit dense-distance graph for
an r-division.  The earlier storage estimate rules out explicit boundary
cliques.  A viable implementation therefore requires ordered boundary distance
matrices together with a genuine Monge/SMAWK or FR-style heap representation;
merely replacing the clique by a flat array does not change the asymptotics.
On the weighted torus, regions must first be opened along wrap edges, with those
edges retained at the parent level, and every boundary matrix must be verified
for the exact cyclic boundary ordering before applying Monge operations.  This
is a substantially different data structure rather than another separator or
CH tuning exercise; it is the next research target, but no unvalidated FR code
has been placed in the production solver.

## Stage 0: bounded-integer search engine (rejected)

A circular Dial queue was prototyped for undirected ALT.  Consistency makes
A* keys monotone and bounds a relaxation's key increase by twice the maximum
edge weight, so a `2*max_weight+1` circular window is sufficient.  With the
same forced 12-landmark preprocessing and all 10,000 `local2d_dev` queries,
the radix version took 2.605 s and Dial took 2.559 s (both 9.5 MiB and exact).
The 1.8% difference is far below the 20% continuation threshold.

A second exact prototype seeded ALT with the cheapest of eight explicit torus
Manhattan routes (both axis orders and both wrap directions), then pruned states
whose admissible lower bound reached that incumbent.  It was exact but took
2.820 s versus 2.675 s for paired ordinary radix ALT: route construction and
edge lookup cost more than the modest pruning saved.  Consequently bucket
Dijkstra, bucket ALT, and route-upper-bound tuning are rejected; none was added
to the production solver.  Wide64 remains on its uint64 radix path.

## Stage 1: planar-region Monge validation

`tools/verify_grid_monge.py` is a standalone verifier for the mathematical
precondition of an implicit DDG.  It recognizes an undirected square row-major
grid, cuts complete square regions without wrap edges, enumerates each region's
boundary clockwise as four non-overlapping intervals, computes exact internal
boundary distances, reverses the second interval to obtain the non-crossing
cyclic order, and exhaustively checks every adjacent 2x2 Monge minor.  This
keeps the non-planar torus wrap edges outside the child regions as required.

Results for 16x16 regions:

| graph | regions | boundary entries | interval matrices | inequalities | violations | time | peak RSS |
|---|---:|---:|---:|---:|---:|---:|---:|
| public `local2d_dev` sample (20 regions) | 20 | 1,200 | 120 | 23,500 | 0 | 0.869 s | 39,752 KiB |
| fresh seed 99173 | 196 | 11,760 | 1,176 | 230,300 | 0 | 5.993 s | 39,760 KiB |
| fresh seed 77191 | 196 | 11,760 | 1,176 | 230,300 | 0 | 5.960 s | 39,764 KiB |

A separate 8x8 run over 50 public regions checked 10,750 inequalities with
zero violations in 0.473 s.  Stage 1 therefore passes for the exact interval
representation intended by the DDG.  The verifier intentionally remains
outside `solver.cpp`; Stage 2 must prove a dynamic Monge minimum primitive
against brute force before any production integration.

## Stage 2: dynamic interval-matrix minimum primitive

The standalone `tools/monge_heap_experiment.cpp` implements and tests the
required dynamic API without touching `solver.cpp`.  It uses a practical
blocked lazy structure: columns are split into sqrt-width ranges; each
row/range stores columns ordered by `(M[r][c], c)`; activating a row inserts one
winner per range; finalized winners advance lazily only when they reach the
global heap top.  Thus activation creates O(sqrt(C)) heap entries rather than
scanning C columns, and no row-column pair becomes a heap item.  Static sorted
indices cost O(R*C) integers, a tradeoff to measure during one-level DDG work.
All keys, bases, and matrix values are uint64.  Saturating addition and an
explicit disconnected-matrix test normalize unreachable results to
`(INF,-1,-1)`.

The harness builds the same six correctly oriented interval-pair matrices per
real planar region as Stage 1.  A dense reference is checked after every row
activation and every column finalization, including deterministic
`(value,column,row)` tie breaking.  Row order and base labels are randomized;
alternating trials use randomized column removal or Dijkstra-like removal of
the current minimum.

Across public and two fresh-seed graphs, sides 8 and 16, the harness ran 11,520
randomized sequences with zero mismatches.  Representative public results
(eight regions, 20 repetitions) were:

| side | active rows | sequences | optimized inspections | dense inspections | reduction |
|---:|---:|---:|---:|---:|---:|
| 8 | 25% | 960 | 35,925 | 39,040 | 1.09x |
| 8 | 50% | 960 | 42,633 | 111,200 | 2.61x |
| 8 | 100% | 960 | 56,133 | 330,400 | 5.89x |
| 16 | 25% | 960 | 234,713 | 442,080 | 1.88x |
| 16 | 50% | 960 | 253,972 | 1,152,800 | 4.54x |
| 16 | 100% | 960 | 292,506 | 3,242,400 | 11.08x |

The public side-16 run sustained 49,353 activations/s and 211,380 extract
calls/s in 0.515 s at 28,800 KiB peak RSS.  Fresh-seed side-16 reductions were
1.81--1.84x at 25%, 4.38--4.43x at 50%, and 10.75--10.86x at 100%, again with
zero mismatches.  The low-occupancy case shows that static ordering cost is not
free, but activation is sublinear and the relevant half/full activation cases
clear the requested 3x work-reduction signal with improving scaling.

**Decision:** Stage 2 passes correctness and practical-scaling gates.  The
primitive is suitable for a Stage 3 one-level DDG experiment, where the main
remaining risk is whether enough rows become active to amortize sorted-index
preprocessing.  It remains standalone; the certified production solver is
unchanged.

## Stage 3: one-level implicit DDG (rejected)

`tools/ddg_experiment.cpp` is a standalone one-level engine; `solver.cpp` is
unchanged.  It partitions the observed square row-major grid into open planar
tiles, retains torus and tile-crossing edges explicitly, computes exact
within-tile boundary distances, and provides both a dense-row Dijkstra and an
implicit blocked-matrix Dijkstra.  Per-query matrix heaps are created only when
touched and use epoch-based global vertex state.

The six side-pair matrices from Stage 1 do not cover pairs on the same side.
Stage 3 therefore recursively bisects the complete cyclic boundary.  At every
split it stores both oriented left/right interval matrices, then recurses on
both halves.  A mechanical coverage check proves that every ordered,
off-diagonal boundary pair occurs exactly once and that its recovered value is
the dense exact value.  Construction aborts on a missing, duplicate, or
unreachable covered pair; all tested regions passed.

### Public `local2d_dev`, all 10,000 queries

| side | boundary vertices | matrices | preprocessing | dense query | implicit query | implicit/dense result |
|---:|---:|---:|---:|---:|---:|---:|
| 16 | 11,760 | 23,128 | 0.679 s | 8.561 s | 40.455 s | 0.21x (4.73x slower) |
| 8 | 21,952 | 42,336 | 0.421 s | 9.891 s | 54.667 s | 0.18x (5.53x slower) |

Both engines matched every official development answer.  For side 16,
preprocessing comprised 0.459 s boundary SSSPs, 0.020 s sorted indices, and
0.200 s other construction/coverage work.  Dense endpoint searches consumed
0.999 s and its global DDG 7.562 s; implicit endpoint searches consumed 1.086 s
and its global engine 39.369 s.  Peak RSS was 39,252 KiB.  Paired forced-ALT
runs took 2.486 s and 2.371 s including its landmark preprocessing, so even the
explicit DDG was about 3.5x slower than retained ALT and the implicit engine
about 16.7x slower.

The side-16 implicit instrumentation recorded 7,778,653 settled boundary
vertices, 15,714,275 touched matrices, 46,153,415 row activations, 110,045,162
matrix heap pushes, 88,774,574 stale pops, 297,187,233 matrix-entry inspections,
and 8,297,156 explicit crossing-edge relaxations.  Occupancy was unexpectedly
high: 15,006,585 touched matrices (95.5%) were in the 75--100% bucket, accounting
for 467,873,236 of 496,006,969 recorded matrix-work operations (about 94%).
Thus the Stage-2 structure *does* amortize at real occupancy, but decomposing a
boundary into 118 small oriented matrices creates overwhelming heap/event
constant factors.  The experiment answers the key occupancy question without
confounding it with low row utilization.

Two fresh-seed side-16 full validations also matched all 10,000 independently
computed reference answers.  Seed 99173 measured 8.215 s dense and 38.993 s
implicit; seed 77191 measured 7.956 s dense and 40.335 s implicit.  Their
occupancy distributions and roughly 111 million pushes were consistent with
the public graph.

A torus-Manhattan split (`distance <= 32`) found 7,527 short and 2,473 long
queries.  On 1,000-query samples, implicit versus dense was 0.36x for short
queries (0.627 versus 0.228 s) and 0.21x for long queries (14.484 versus 3.059
s).  There is therefore no observable locality threshold at which this
implicit implementation becomes a useful hybrid.

**Decision:** Stage 3 is rejected.  It is exact and real matrices have the high
occupancy Stage 2 wanted, but it fails the required engine-speed criterion by a
large margin and has no credible path to beat the ~76 s large ALT solver.  No
large run, production integration, multi-level extension, or wide64 port is
warranted.  The standalone engine and measurements are retained to prevent
repeating this architecture.

## Stage 3B: cross-side Monge plus explicit same-side rows (rejected)

The standalone Stage-3 engine was changed exactly as proposed, without touching
`solver.cpp`.  Each corner has one canonical clockwise side.  Every region now
has 12 oriented cross-side views (the six unordered side pairs in both
directions), while a settled boundary vertex scans only its exact dense row to
other vertices on the same side.  The construction verifier counts every
ordered boundary pair and aborts unless it is covered exactly once by either a
cross-side view or a same-side block, with the exact dense distance.  All public
and fresh-seed regions passed.

On all 10,000 public `local2d_dev` queries, side 16 produced:

| metric | recursive Stage 3 | cross-side Stage 3B | change |
|---|---:|---:|---:|
| matrices total | 23,128 (~118/region) | 2,352 (12/region) | 9.83x fewer |
| preprocessing | 0.679 s | 0.559 s | 18% lower |
| implicit total query time | 40.455 s | 30.168 s | 25% lower |
| implicit global time | 39.369 s | 29.108 s | 26% lower |
| row activations | 46,153,415 | 23,335,989 | 49% lower |
| heap pushes | 110,045,162 | 82,031,574 | 25% lower |
| stale pops | 88,774,574 | 61,361,135 | 31% lower |
| matrix inspections | 297,187,233 | 225,503,203 | 24% lower |
| boundary vertices settled | 7,778,653 | 7,778,663 | unchanged |

Stage 3B additionally performed 109,160,461 tiny explicit same-side
relaxations and 8,297,166 crossing-edge relaxations.  Its preprocessing split
was 0.426 s boundary SSSPs, 0.014 s sorted indices, and 0.118 s other work; peak
RSS fell to 28,416 KiB.  All 10,000 answers matched the official reference.

The simplification attacks the diagnosed event explosion and is measurably
better, but the pushes fell by only 25%, not an order of magnitude.  Twelve
oriented cross-side views still create roughly 10.5 heap pushes per settled
boundary vertex, and explicit same-side scanning adds another 109 million
relaxations.  The dense reference took 8.250 s total / 7.245 s global, so the
new implicit engine remains 3.66x slower overall and 4.02x slower in its global
phase.  It is also about 12.5x slower than the paired ~2.4 s retained ALT run.

Fresh-seed 1,000-query checks remained exact.  Seed 99173 measured 0.920 s
dense versus 3.093 s implicit; seed 77191 measured 0.760 s versus 2.997 s.
Thus the regression is structural rather than a public-weight accident.

**Decision:** reject Stage 3B and stop DDG/Monge/FR research as directed.  It
fails the mandatory condition that implicit global DDG beat dense DDG, so no
wide64_dev port, block-width tuning, multilevel DDG, theoretical FR heap, or
production integration is justified.  The production checkpoint remains
unchanged.

## Autonomous profiling phase: frontier-work-balanced bidirectional search

Profiling focused on directed scale-free search after rejecting DDG.  Two cheap
offline alternatives were screened first.  Lowering reverse target grouping
from frequency 4 to 3 or 2 was exact but regressed `scalefree_dev` from 7.988 s
to 8.470 s and 13.463 s respectively.  Replacing bidirectional search with
forward-only Dijkstra was also exact but regressed from 7.502 s to 16.692 s.
Both are rejected.

The decisive observation was that the existing bidirectional search selected a
frontier solely by its minimum distance key.  On a directed power-law graph,
the reverse frontier can then repeatedly settle enormous-in-degree hubs and
relax far more arcs than the forward frontier.  Bidirectional Dijkstra's exact
termination condition depends on the two minimum keys, but correctness does
not require expanding the smaller-key side.  The retained change selects the
side with fewer cumulative outgoing arcs relaxed, while preserving the same
`min_forward + min_backward >= best` termination test.

This reduced paired `scalefree_dev` time from 8.047 s to 0.925 s (8.70x) while
remaining exact.  On a fresh generator seed (662211), it reduced 7.876 s to
0.857 s (9.19x), again matching all 10,000 reference answers.  It also reduced
`road2d_dev` from 4.396 s to 3.816 s; symmetric regular graphs naturally keep
both work counters close, so other dev families retain their behavior.

The official `scalefree_large` targeted comparison, including graph loading
and all preprocessing, was exact for all 50,000 queries:

| candidate | solver time | peak RSS | correctness |
|---|---:|---:|---|
| certified key-balanced SAFE | 339.340 s | 61.8 MiB | exact |
| arc-work-balanced bidirectional | **24.726 s** | **48.6 MiB** | exact |

This is a **13.72x workload improvement** from a small, generally valid change,
and is the new production candidate.  Target grouping remains at the proven
frequency-four threshold.  All six development answer sets match exactly.

### Structural dispatch and regression diagnosis

A first full-suite attempt showed that unconditional arc-work balancing is
wrong for the road family: the long road2d queries progressed poorly and the
run was interrupted rather than wasting a full certification cycle.  This did
not affect the power-law result, but required observable structural dispatch.
The distinction is strong and seed-independent: public/fresh road graphs have
maximum reverse degree 8 (average about 5.4), while scale-free dev/large have
maximum reverse degrees 2,173/14,427 (average about 2.91).

The production candidate therefore enables work balancing only on directed
graphs whose observed maximum reverse degree is at least 64.  Low-degree road
and all undirected families retain the certified minimum-key frontier policy.
After this correction `road2d_dev` was exact at 4.122 s and
`scalefree_dev` remained exact at 0.850 s.  A targeted road2d_large run was
exact at 356.303 s on this slower runner; because its maximum reverse degree is
8, its search code is byte-for-byte the old key-selection path.  The timing is
not comparable to the previously reported faster-machine certification and is
recorded only as an exact regression check.

## Post-scale-free profiling: wide64 frontier policy hypothesis

The next target is wide64.  Its regular degree-four topology removes the degree
skew that motivated scale-free work balancing, but its broad log-uniform edge
weights can make minimum reduced-key frontier selection uneven.  The smallest
first experiment compares the certified balanced-potential bidirectional A*
against strict equal-relaxation alternation.  This changes no heuristic or
preprocessing and directly tests whether frontier policy is another hidden
pathology before considering a new data structure.

## Wide64 frontier-policy screen (rejected)

The certified balanced-potential bidirectional search was compared with strict
forward/backward alternation and heap-cardinality selection on `wide64_dev`.
All were exact.  Paired timings moved between 3.75 and 4.06 s while SAFE moved
between 3.84 and 4.04 s across repeats; the apparent best result was below 8%
and reversed sign on repetition.  This is benchmark noise / a sub-threshold
micro-effect, so no wide64 frontier-policy change is retained.

## Landmark heuristic memoization (retained)

Profiling the local2d ALT loop found that the 12-landmark lower bound was
recomputed when a vertex was pushed, when it was popped, and again after later
successful decreases.  Within one query the target and landmark tables are
fixed, so the value is exactly reusable.  A timestamped per-vertex uint64 cache
now computes each touched vertex's heuristic at most once per query, without
changing the queue order or admissibility.

A forced-ALT `local2d_dev` comparison improved from 0.866 s to 0.682 s (21.2%)
and remained exact.  Fresh seed 884422 matched all 10,000 reference answers in
0.697 s.  On the official `local2d_large` workload, paired on this runner and
including graph load plus landmark preprocessing, SAFE took 157.546 s and the
cache took **141.294 s** (10.3% faster); all 300,000 answers matched.  Peak RSS
rose modestly from 111.9 to 115.4 MiB for one uint64 value and one timestamp per
vertex.  This small, low-risk improvement composes with the scale-free frontier
change and is retained.

## Directed road ALT dispatch (retained)

Profiling showed `road2d` has average directed out-degree about 5.42, narrowly
above the old landmark gate of 5.0.  Consequently the large road workload was
still using independent bidirectional searches despite having 200,000 queries
to amortize landmark preprocessing.  This is materially different from prior
coordinate A*: directed ALT uses exact forward and reverse landmark distances
and the already-proven directed lower-bound formula.

Raising the observable average-degree gate from 5.0 to 6.0 activates the
existing 12-landmark implementation for `0.5 <= Q/V < 5`.  Forced dispatch on
`road2d_dev` improved from 4.422 s to 1.999 s (2.21x), exact.  Fresh generator
seed 553311 improved from 4.302 s to 2.072 s (2.08x), matching all 10,000
reference answers.

On official `road2d_large`, the retained SAFE path took 356.303 s on this
runner.  Directed ALT, including graph loading, 24 directed landmark SSSPs,
and all 200,000 queries, took **174.518 s** at 139.8 MiB peak RSS and matched
every official answer.  This is a **2.04x workload improvement**.  The dispatch
uses only Q/V and observed average degree; hugeq remains on its earlier
high-Q/V separator path, scale-free does not meet Q/V, and undirected behavior
is unchanged except for the already-targeted local2d lattice.

## Dispatch audit: sparse-query wide64 ALT hypothesis

The next audit revisits only the *dispatch gate*, not landmark-count tuning.
`wide64` has Q/V below the existing 0.5 cutoff, but its individual high-rank
queries are expensive and the large graph still has 30,000 queries.  The
hypothesis is that the already-retained exact 12-landmark engine may amortize
at a lower Q/V when edge weights are wide and query ranks are long.  Forced
dev timing, including preprocessing, is the first rejection gate.

### Sparse-query wide64 ALT result (retained)

Forced 12-landmark ALT on `wide64_dev`, including preprocessing, improved from
4.311 s to 1.146 s (3.76x) and was exact.  Fresh generator seed 117733 improved
from 4.078 s to 1.226 s (3.33x), matching all 10,000 reference answers.

On official `wide64_large`, the existing uint64 balanced-potential search took
473.802 s on this runner.  Landmark ALT took **105.672 s**, including graph
loading and all 12 full SSSPs, used 436.9 MiB peak RSS, and matched all 30,000
64-bit reference answers.  This is a **4.48x workload improvement**, decisively
reversing the earlier assumption that preprocessing could not amortize.

The retained dispatch does not lower the general Q/V threshold.  It permits
`Q/V >= 0.01` only when the graph has already passed the observable exact 2-D
torus topology check (`lattice_side`), while keeping the low-degree and Q/V<5
requirements.  This generalizes across seeds, excludes scale-free and 3-D
lattice graphs structurally, and preserves uint64 distance arithmetic.

# Active research state

Goal: formally certify Overall geometric mean >=150x; not yet achieved.

## Authoritative formal checkpoint and current candidate (2026-09-27)

Formal baseline: all six large exact, Overall **88.35080960833166x**, Global
126.6129459x, Local 43.0204920x. Source SHA256
`4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`;
binary `2582aed37dab141b5d52d618ca4a14879c2d8747e7579c7b30a63bce9ad5401d`.
Preserved matching source/binary/result:
`research/checkpoints/certified-integrated-4610587.{cpp,bin,json}`.
Root official artifact remains `result-integrated-4610587-full.json`, SHA256
`fe329dbec22b822e62dc24d2843bcb549c3c843000638782cf6739375948b972`.

Current solver.cpp is the new sparse-separator candidate, source SHA256
`817fe03416a0c8d529e074b876d0a0429bc6e1bdd47b8cf73e1f9fb18414d599`.
It passed six dev, fresh seed and hugeq_large, with approximately 1.95x targeted
gain and much lower memory. It is NOT yet formally full-graded. See the final
sparse-separator entry and research/logs/production-sparse-sep-817fe034-validation.json.
The old 88.3508x JSON belongs to the preserved baseline, not the current candidate.
No new Overall is certified; the 150x goal remains unmet.

## Historical checkpoint notes (superseded by 4610587e)

The authoritative formal checkpoint is
`research/checkpoints/certified-integrated-e0527341.cpp`, SHA256
`e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`. Its
matching formal artifact is `research/checkpoints/certified-integrated-e0527341.json`,
SHA256 `0c69474470bd5022d213a1cfef0866ed869de6d637b47f5b28564789da16c768`.
The latest official command was:

`python3 -u grade.py --solver ./solver --instances instances.txt --json result-integrated-e052-full.json`

All six large instances were exact (`note: ok`), with three solver repeats,
five baseline repeats, and the 4 GiB cap enforced. It formally measured Global
geomean 108.555x, Local geomean 42.027x, and Overall **79.11815862212096x**.
The matching root artifact `result-integrated-e052-full.json` is the exact
grader output. `solver.cpp` matches the certified source SHA. The earlier
67.88595005047883x result remains preserved as a historical checkpoint; the
12.46x result remains rollback history. No benchmark process is live.

Current research aim is to reach formally certified Overall >=150x. Preserve
this exact source and matching JSON as the rollback checkpoint. The official
Overall is the geometric mean of all six per-instance speedups.

The current working `solver.cpp` additionally includes the exact road
corridor-level CH ordering candidate. Its source SHA is
`af27699a5416968ba9a4d840e87822c01eaa27b170822568f2ac82a5af4943d3`; it
matches the prototype byte-for-byte, passed the candidate's all-six dev grade
and fresh road seed, and passed a new road2d_large answer comparison at
18.36s / 202,888 KiB. This is a targeted result, not a new formal Overall.
The e052 source and JSON above remain the recoverable formal checkpoint.

## Latest measured candidates (all exact official answers, not certified Overall)

- Road targeted-witness product CH+stall:27.87s total,12.78s preprocessing,
  232.8MB RSS; 200k/200k exact. `witness_ch_targeted.cpp`.
- Lattice3d biALT24/4:45.36s total,30k/30k exact.
- Local2d biALT24/4 without undirected groups:35.64s total,300k/300k exact.
- Scale-free pruned labels:48.66s total,48.39s preprocessing,62.17m labels,
  1.59GB RSS;50k/50k exact. `hub_labels.cpp`.
- Hugeq and wide64 still use certified paths until new validation.

## Latest validation

The recovered balanced bidirectional ALT prototype was run on the complete
`wide64_large` workload: **41.42 s**, all 30,000 answers exact, 613 MB RSS.
The corresponding recorded foundation estimate is about 1120.9 s, giving a
targeted speedup near 27x. Together with the measured road, lattice, local,
and scale-free prototypes, a rough independent upper-bound composition would
be about 36x Overall (using the existing hugeq result); this is evidence for
the next integration step, not a certification because the sources are still
separate.

Current best scale-free architecture is core contraction plus hub labels:
45.68 s full, all 50,000 exact, 453 MB, versus 48.66 s and 1.59 GB for full
labels. The core-hub implementation was also exact on 80 fresh random directed
graphs. The 2D cell-ordered CH was exact on wide64 dev but has not beaten the
41.42 s ALT result. Dominated-edge sparsification plus CH was exact on lattice3d
dev (4.80 s, 31 MB) and is queued for full-tier measurement.

Full lattice sparsification completed: 44.941% of arcs removed in 0.079 s;
sparsified biALT24/4 took 42.68 s and matched all 30,000 official answers,
with 111 MB RSS. The unsparsified run took 45.36 s / 124 MB. This is retained
as the best isolated lattice candidate.

Next independent architectures: coreCH+labels (contract lowdegree before
labeling); exact detour sparsification followed by biALT/CH; recursively
cell-ordered witnessed CH on structurally verified2D tori. Prototype sources
in research/prototypes. Compile flags for root benchmarks are-O2 -std=c++17.
Completed representative logs copied to research/logs. Fresh standard-generator
seed validation and combined formal certification remain outstanding.

## Integrated candidate completed

The targeted road CH, sparsified balanced ALT for lattice workloads, and the
24-landmark wide/local ALT path were integrated into one solver using
structural dispatch. Exact pruned hub labels were then added for sparse
directed coordinate-free graphs. No filename, path, hash, or seed identity is
used by dispatch.

The required full grade completed with every instance `ok`:

| instance | T_base | T_solver | speedup | RSS |
|---|---:|---:|---:|---:|
| road2d_large | 953.257s | 28.217s | 33.783x | 227MB |
| lattice3d_large | 1679.830s | 35.671s | 47.092x | 108MB |
| local2d_large | 647.955s | 37.494s | 17.281x | 141MB |
| scalefree_large | 1943.555s | 52.841s | 36.781x | 1552MB |
| hugeq_large | 2151.363s | 17.844s | 120.565x | 745MB |
| wide64_large | 1176.110s | 38.665s | 30.418x | 594MB |

Global geomean is 51.537x, Local geomean 22.927x, and Overall geomean is
**39.343x**. This improves the certified 12.46x checkpoint but does not reach
the 100x/150x goal. Current solver SHA256 is
`5a05741a3dcc0ebd7088a00cf4f6939cabc1e21082d032ee762b069f029652fe`.

Fresh-seed end-to-end validation used generator seed 913771 and salt
`integrated150`, with independent `refsolve` answers. `road2d_dev`,
`lattice3d_dev`, `local2d_dev`, `scalefree_dev`, `wide64_dev`, and
`hugeq_dev` all matched exactly. The integrated candidate and matching grade
are preserved as `research/checkpoints/certified-integrated-39.34.{cpp,json}`.

The next local2d architecture was tested in isolation: verified 2-D torus
recognition plus exact canonical-route upper bounds and Manhattan lower-bound
pruning. It was exact and very fast on dev, but did not finish the large
300,304-vertex / 300,000-query workload within the current ALT runtime budget;
it is rejected and remains only as `research/prototypes/local_region.cpp`.


### Scalefree regression repair (2026-09-25)

Investigation recovered the previous fast path from historical commit `908a82d`. The regression had two causes: the integrated solver removed `balance_by_arc_work`, reverting to key-radius side selection, and a new full hub-label builder captured coordinate-free sparse directed graphs before the bidirectional path. The hub-label dispatch was removed for this structural class; reverse-degree skew recognition restores relaxed-arc work balancing in `plain_bidijkstra`, with exact min-key termination unchanged.

Validation: scalefree_large 15.11 s, RSS 52 MB, exact; fresh seed 24681357 / salt `scale-repair`, 10,000 queries exact; all six dev instances exact. Saved checkpoint: `research/checkpoints/certified-scalefree-repaired.cpp`; SHA256 recorded in `/tmp/scale_sha` during this run.


Direct post-repair large rerun retained exactness for all six workloads. Timings were road2d 35.84s, lattice3d 47.65s, local2d 45.15s, scalefree 22.42s, hugeq 22.12s, wide64 46.95s; RSS respectively 233, 111, 144, 52, 763, and 608 MB. Existing local2d ALT prototypes (`bialt_nogroups`, `sparsify_bialt`) were rechecked and measured 47.06s and 46.35s, so they do not improve the current integrated path; local2d remains the next architecture target.


### Retained-winner integration audit

The current combined solver was audited against all recorded prototypes and history. Production dispatch selects: witness CH+stall for directed coordinate road graphs (degree >4.5, Q/V <5); sparsification plus balanced ALT24/4 for undirected workloads; recursive three-level separator plus restricted grouped search for coordinate graphs with Q/V >=5; and grouped-target plus relaxed-arc work-balanced bidirectional search for sparse coordinate-free directed graphs. The hub-label/core implementations remain source prototypes only and cannot shadow scalefree because no production branch calls them.

A second accidental omission was found: historical directed ALT heuristic memoization (`landmark_heuristic_cache` and epoch stamps) had been removed. It is restored in `solver.cpp` and initialized with landmark preprocessing. Six dev outputs and all six large outputs are exact after the repair. Audit checkpoint: `research/checkpoints/audit-complete.cpp`.

### Fourth-level hugeq separator (integrated)

The three-level separator left approximately 199.8 million leaf-search pops. An exact fourth level now splits each of the eight leaf interiors, builds restricted tables for their crossing hubs, and answers cross-microregion queries by table lookup while retaining restricted grouped fallback for same-microregion and separator cases. Hub counts are 63,62,127,55,64,64,63,62 (520 total). hugeq_large measured 11.89 s / 1.20 GB RSS after integration, all 600,000 answers exact; the isolated prototype was 11.68 s. Fresh seed 777777 / salt `huge4` was exact. Prototype source: `research/prototypes/hugeq_fourlevel.cpp`; checkpoint: `research/checkpoints/certified-hugeq4-candidate.cpp`.

Fifth-level hugeq separator attempt was rejected: although it reduced the isolated runtime to 9.5s and ~2.0GB RSS, its nano-region shortcut tables produced incorrect answers (first mismatch on hugeq_large query 508). A reconstruction with explicit parent-region membership and separate globally-safe boundary candidates answered all 600,000 official hugeq queries exactly, but took 9.98s / 1.89GB in a targeted run versus the new formal 11.428s (1.15x). The improvement is below the 20% research threshold and nearly doubles peak memory; it remains isolated. Production stays at exact level four.

Torus-potential composition experiment was rejected. Extending topology recognition to low-weight local2d and taking max(24-landmark ALT, minimum-weight torus Manhattan) remained exact but slowed dev: local2d 0.38s versus ~0.27s current, wide64 0.59s versus ~0.45s. The pure torus bidirectional path also failed to finish wide64_large within the experiment budget.

A fourth-level leaf batching refinement was tested: same-microregion queries used restricted grouped search plus the micro-separator table, with separator-hub edge cases handled. It was exact at 9.97s / 1.20GB on hugeq_large versus 11.89s production fourth-level, only 1.19x; below the multi-fold threshold, so it remains isolated and production stays the simpler exact fourth-level path.

A local2d separator-overlay attempt (deriving torus coordinates from square node IDs and forcing one-level separator at Q/V>=0.1) was rejected. It was exact on local2d_dev but cost 6.32s / 1.29GB versus ~0.26s ALT, so its preprocessing/table cost overwhelms the query batch even before large scaling.

### Formal hugeq4 combined certification (2026-09-26)

The exact fourth-level hugeq separator, repaired scale-free work balancing,
road CH, sparsified balanced ALT, and retained fallback paths were graded
together with:

`python3 -u grade.py --solver ./solver --instances instances.txt --json result-hugeq4-full.json`

All six instances returned `ok`. Results:

| workload | T_base | T_solver | speedup | peak RSS |
|---|---:|---:|---:|---:|
| road2d | 975.222s | 28.858s | 33.794x | 227MB |
| lattice3d | 1731.543s | 36.928s | 46.890x | 112MB |
| local2d | 636.906s | 37.633s | 16.924x | 144MB |
| scalefree | 1921.349s | 15.259s | 125.913x | 51MB |
| hugeq | 2236.073s | 11.755s | 190.225x | 1174MB |
| wide64 | 1163.932s | 38.047s | 30.592x | 616MB |

Global geomean is 78.490x, Local geomean is 22.754x, and formally certified
Overall is **51.947x**. Source SHA256 is
`a9e6cc62298142a24d2b1dde1b93a29a717ebff994f98aa3198512207f638495`.
Matching artifacts are `research/checkpoints/certified-hugeq4-candidate.cpp`
and `.json`. The remaining gap to 150x requires about 2.89x on the overall
geomean, so research remains focused on multi-fold local2d/wide64/road/lattice
architectures rather than another unchanged full grade.

Local2d short-rank plain-search hypothesis: disabling landmarks on the
observable low-weight degree-4 undirected structure remained exact but took
1.21s on local2d_dev versus 0.27s for current balanced ALT. The heuristic work
is not the dominant cost; plain bidirectional Dijkstra expands substantially
more state. Rejected before large testing.

The retained cell-ordered witnessed CH prototype was evaluated on local2d_dev.
It was exact, but built 1,638,144 shortcuts in 27.37s and needed about 1.56s
for 10,000 queries (28.93s total), versus 0.27s total for balanced ALT. Even
excluding preprocessing its per-query work is much higher, so this CH ordering
cannot provide a large-tier gain and was rejected without threshold tuning.

Exact local tile certification was tested. A restricted tile Dijkstra tracks
the minimum first-exit cost plus the global ALT lower bound; if the internal
answer is no greater, it is provably global. On local2d_dev, 32x32 tiles tried
5,151 queries and certified 4,990 (49.9% of all queries), but total time was
0.34s. 16x16 tiles certified 3,177 of 3,319 attempts and took 0.28s, tied with
the 0.27s current solver. The coverage is high but restricted-search work
replaces already-cheap ALT work rather than eliminating it, so the approach
has no credible multi-fold large gain and was rejected.

Undirected PLL was evaluated twice. Degree ordering is pathological on the
regular grid: after only 2,000/50,176 hubs it had 69.6m labels, used 1.75GB,
and hit the 90s timeout. A topology-verified nested-dissection hub order
completed exactly, but still built 34.1m logical labels in 32.66s and used
1.64GB on dev. Its planar scaling projects to roughly 500m labels and several
minutes on large, at or beyond the 4GB cap even with compressed undirected
storage; rejected.

Sorting offline local2d queries by source ID improved dev only from 0.28--0.29s
to 0.26--0.27s. This cache-locality ceiling is far below the required
multi-fold gain, so no large run or interleaved-query implementation followed.

Wide64 full-table ALT potential: using all 24 already-built landmark tables in
each balanced-potential evaluation remained exact but took 0.91s on wide64_dev
versus 0.46s for the retained per-query top-4 subset. Extra pruning does not
repay six times the table traffic, so the existing 24/4 design remains best.

### Follow-up fail-fast architecture probes (2026-09-26)

The formal 67.886x source remains untouched at SHA256
`062942028d547557130c899270facfeaf2b0191f7164e51fed6c369022308234`.
Temporary probes live only in `/tmp` and are not production candidates.

For wide64, a bounded exact Dijkstra witness search through strictly lighter
arcs, after the retained 2/3-edge certificate, found 179 additional detours
among 90,744 tested candidate edges on dev. This increased removed arcs only
from 9.574% to 9.753%; runtime regressed from 0.38s to 0.40s (exact output).
The broader witness search is rejected as low-yield.

A pure topology-derived wide64 A* using only `minimum_weight * torus-Manhattan`
avoided all 24 landmark SSSPs but expanded 28.52M vertices on wide64_dev and
took 2.43s versus 0.38s for the retained forward-ALT path. It was exact but
rejected because its admissible geometric potential is too weak.

Two local2d hierarchy probes also failed fast. Existing dynamic witnessed CH
built 367,998 shortcuts and answered local2d_dev exactly, but took 1.44s /
32.6MB versus 0.97s / 113MB for the 256-table production path; the shortcut
count also projects close to its 15M abort cap on the large graph. A custom
bidirectional ALT using the four local-grid anchors near the target was exact
on local2d_dev but settled 73.9M total heap pops and took 7.20s, versus 0.97s
for the retained one-sided A*. Both are rejected; no production source or
checkpoint changed.

On lattice3d, evaluating all 128 existing ALT tables for every popped vertex
was exact but took 4.53s on dev, versus 1.69s for the retained top-four
bidirectional path. It popped 2.39M vertices; the additional table traffic did
not buy enough pruning. Rejected without large testing.

A new exact relaxed-metric potential was tested on lattice3d and wide64. For
each vertex, incident edge weights were relaxed to at most that vertex's
minimum incident weight (and each edge to the minimum of its original weight
and both endpoint minima); exact landmark SSSPs in this dominated graph give
admissible, consistent lower bounds for the original graph. The 24-landmark
hybrid remained exact but lattice3d_dev took 2.44s and expanded 15.19M nodes;
wide64_dev took 0.51s / 3.34M pops. Both regressed against production (1.69s
and 0.38s respectively), so this relaxation is rejected; all source edits
remain temporary.

Two further structural checks ruled out inexpensive alternatives. The exact
minimum edge weight across each coordinate cut is 1 on lattice3d and has
median 2 on local2d, so summing cut minima cannot materially strengthen their
global torus bound; on wide64 its median is only 0.11% above the graph-wide
minimum. For hugeq, swapping restricted same-leaf grouping from repeated target
to repeated source remained exact on the forced-separator dev run (first 10k
answers) and measured 6.36s versus 6.32s for target grouping at equal 1.18GB
RSS. Large query counts favor target groups more strongly (3,190 road targets
with frequency >=4, versus only 464 at >=8 and 55 at >=32); this direction
change has no credible multi-fold upside and was rejected without a large run.

Long-tail profiling of current local2d_large found 27,157 queries at torus
Manhattan displacement <=96 caused 16.48M A* pops, while 28,121 longer queries
caused 85.05M pops. The properly balanced local-grid biALT was exact on dev,
fresh seed 97531099 / salt `local-balanced`, and all 300,000 large answers.
Across all large queries it reduced total pops from 101.5M to 78.9M but slowed
runtime from 15.38s (instrumented current) to 17.30s because doubled-frontier
and potential work outweighed reduced expansions. Restricting biALT to the
long-query tail still took 16.92s / 689MB. This long-tail hybrid is exact but
rejected; production stays at one-sided local-grid A*.

An offline target-batch attempt used the existing exact grouped Dijkstra for
local2d targets with multiplicity >=4. It answered all 300,000 local2d_large
queries exactly, but only 25,654 queries fell in 6,056 groups and those shared
searches settled 354.2M vertices; total runtime was 40.40s / 689MB versus the
14.2--15.4s individual-A* path. Dev had only three qualifying groups covering
12 queries. This does not amortize across the target distribution and is
rejected.

Exact core reduction by suppressing degree-2 vertices was screened on
lattice3d_dev after retained edge sparsification. Only 9,170/50,653 vertices
(18.1%) had degree 2 and 1,057 had degree 1; degree-3+ vertices dominate, so
chain suppression cannot approach a multi-fold reduction and would complicate
landmark endpoint access. Rejected at the structural-count stage.

### Road geometry potential on CH (2026-09-26)

The road generator's observable coordinates allow an exact geometric feasible
potential without metadata: compute a conservative integer `lambda` no larger
than every directed edge's `weight / EuclideanLength`, then use
`floor(lambda * EuclideanDistance(v,t))` (rounded down with a one-unit safety
margin). A reduced-cost bidirectional CH query was implemented in `/tmp`.
Using the ordinary bidirectional `min_f+min_b` cutoff was wrong on road2d_dev;
the CH-safe conservative cutoff requiring both reduced frontier minima to
reach `best - h(s)` restored exactness on all 10,000 answers. With stalling it
still took 1.84s / 38MB versus the current road2d_dev 1.53s path. A one-sided
geometric A* on the original directed graph was exact but expanded 39.98M
vertices and took 4.48s. Both versions are rejected before large testing.
The experiment confirms the coordinate ratio alone does not capture the
variable-speed detours well enough to beat CH.

### Local2d exact tile boundary skeleton (2026-09-26)

Prototyped a safe grid decomposition: compute exact distances from every tile
node to each tile-boundary portal, replace each tile by its exact portal
distance clique, retain crossing edges, and answer with portal-set A* plus a
restricted exact within-tile path. On local2d_dev with 16x16 tiles, the
10818-node / 598224-arc skeleton and 24 landmark tables were built in 0.23s;
all 10,000 answers were exact and total time was 0.73s / 54MB. However the
query phase expanded 1.99M portal vertices and scanned 110.25M arcs, versus
1.06M original-graph A* pops and 3.87M arc scans in 0.94s / 113MB for current
dev. On the large graph the skeleton has roughly six times as many portals,
while the dense per-tile cliques preserve high query degree; scaling the query
work to 300k pairs erases the preprocessing savings. The exact version using
CH on this skeleton took 54.63s on dev due to 921k shortcuts. Both variants
are rejected before fresh-seed/large testing; production is unchanged. The
temporary prototypes are `/tmp/local_tile_oracle.cpp` (portal CH) and the
same source's later A* variant; the road potential prototype is
`/tmp/road_geo_ch.cpp`.

Wide64 forward ALT composition: because rank queries are defined by forward
settlement from the source, a one-sided A* using the same per-query top four of
24 landmark tables avoids the unconstrained reverse frontier. It measured
30.31s / 631MB on wide64_large versus 38.047s in the formal checkpoint (1.26x)
and matched all 30,000 answers. Fresh seed 888888 / salt `fwdalt4` and all six
official dev sets are exact after integration. Dispatch is restricted to the
structurally verified high-weight square torus (`lattice_side`); local2d and
lattice3d retain balanced bidirectional ALT. Candidate checkpoint:
`research/checkpoints/candidate-wide-forward-alt4.cpp`, SHA256
`c8400d73be7dbcf65310872da8a7ea8b0cabf4835e651994b20528841fde94b2`.
This gain alone does not justify another full grade.

Wide64 large zero-query timing isolates 6.68s for graph parsing,
sparsification, and all 24 landmark SSSPs (RSS 626MB). Roughly 23.6s of the
30.31s forward-ALT candidate remains in its 30,000 queries, so a faster
landmark-construction engine alone cannot deliver the required 2x; future work
must reduce query search.

The same one-sided ALT was tested on uniform-query lattice3d_dev and was
slower (2.31s versus 2.02s balanced bidirectional ALT), confirming that the
wide64 gain depends on forward-rank query structure. Lattice3d dispatch is
unchanged.

### Wide64 downward-scaled compact landmark experiment (2026-09-26)

Hypothesis: construct ALT landmarks in a graph whose edge weights are rounded
down by a fixed scale `C`, then multiply the landmark potential by `C`. Since
each scaled edge cost is no greater than its original weight, the scaled
shortest-path metric is a consistent admissible lower bound. A verified 2D
torus also gives a path of at most `n` edges between any pair, which bounds
scaled distances and permits exact `uint32_t` table storage. The isolated
prototype `research/prototypes/wide_scaled_alt.cpp` uses only this verified
topology and weight maximum; it contains no instance identity checks.

It matched all wide64_dev and wide64_large official answers. On large, using
`C=465,521` (the generic `(V-1)*max_weight` bound) took 30.93s / 493MB;
using the tighter torus `n*max_weight` bound (`C=330`) took 31.80s / 493MB;
and using `C=100,000` took 31.49s / 492MB. The certified one-sided ALT path
took 29.692s / 632MB. The compact tables save memory but the scaled landmarks
and their SSSPs do not improve the scored time; this family is rejected before
fresh-seed or full-grade validation. `solver.cpp` remains the e052 source.

### Wide64 repeated-vertex ALT heuristic memoization screen (2026-09-26)

The forward four-landmark A* recomputes the same `h(v)` if a vertex receives
multiple successful relaxations within one query. An isolated copy cached each
query's exact four-table heuristic by vertex/epoch. It remained exact on
wide64_dev and took 0.37s / 23MB versus about 0.38s / 19MB retained. The total
dev difference is within noise and the extra 4 MB did not show a clear gain;
the cache is rejected before large testing. Prototype: `/tmp/wide64_hcache.cpp`.

### Road fast-corridor-core CH ordering (2026-09-26)

Hypothesis: the road family has observable multi-speed corridors; ranking nodes
with several lowest-quantile weight/Euclidean-length incident arcs later in
contraction could retain a compact high-speed core and reduce query search.
The exact prototype `research/prototypes/road_fast_corridor_ch.cpp` classifies
the bottom 1/32 of normalized arc costs and adds a degree-weighted priority
bonus to vertices incident to at least two such arcs. This uses only supplied
coordinates and edge weights; CH exactness is independent of vertex order.

It compiled with `g++ -O2 -std=c++17`, matched all road2d_dev and road2d_large
answers, and built 296,475 dev shortcuts / 2,095,344 large shortcuts, versus
322,816 / about 2.49M in production. Dev took 1.13s / 35MB versus 1.04s / 34MB
production. The targeted large run took 20.84s / 211MB versus the formal
22.046s / 208MB (about 5.5% faster) despite ~16% fewer shortcuts. If all other
formal rows are held fixed, this projects to only ~79.86x Overall, so the
candidate is rejected before fresh-seed/full-grade validation. Production
remains the e052 checkpoint.

Commands: `g++ -O2 -std=c++17 research/prototypes/road_fast_corridor_ch.cpp -o /tmp/road_fast_corridor_ch`; timed runs on road2d_dev and
`/usr/bin/time -v /tmp/road_fast_corridor_ch instances/road2d_large.graph instances/road2d_large.queries /tmp/road_fast_large.out`; both outputs were byte-compared with official answers.

### Scalefree SCC reachability screen (2026-09-26)

Hypothesis: unreachable scale-free queries may force expensive bidirectional
searches, so exact SCC-condensation reachability checks could reject them
cheaply. The standalone probe `research/prototypes/scalefree_scc_probe.cpp`
uses iterative Kosaraju and a topological order of the condensation DAG. On
scalefree_large (V=300,000, E=873,003), it found 9,005 SCCs, a 290,996-node
giant SCC and 9,004 singleton SCCs. Of 50,000 queries, 1,559 are unreachable;
all 1,559 have a source in a sink SCC (the retained search empties its forward
frontier immediately), while the topological-order filter rejects 1,529.
Dev gave the same pattern: 289 unreachable queries, all from sink components,
and 286 topological rejects. Therefore unreachable cases are not a major
search-cost bottleneck; SCC preprocessing and query checks add work for little
benefit. The probe ran in 0.10s / 44.6MB on large and 0.01s / 14.1MB on dev.
It is rejected before solver integration; production remains unchanged.

Reproduction: `g++ -O2 -std=c++17 research/prototypes/scalefree_scc_probe.cpp -o /tmp/scalefree_scc_probe`, then `/usr/bin/time -f 'elapsed %e rss_kb %M' /tmp/scalefree_scc_probe instances/scalefree_large.graph instances/scalefree_large.queries instances/scalefree_large.answers` (repeat with `scalefree_dev` files).

A bounded-increment Dial queue was substituted for radix heaps on local2d.
Balanced ALT reduced-key increments are at most four times the observed
maximum edge weight, so a 4,001-bucket circular queue is exact. It nevertheless
slowed local2d_dev from 0.29s to 0.33s; per-query bucket management outweighed
radix redistribution savings. Rejected.

Apples-to-apples scalefree comparison on the same runner: historical 908a82d solver 17.79s / 52MB, current repaired integrated solver 16.24s / 52MB; both exact. The earlier 14.39s versus 22s spread was runner noise, not a missing algorithm. Scalefree is frozen.

### Local2d regular-grid exact landmarks (2026-09-26)

A verified low-weight 2-D torus now dispatches to 256 exact SSSP tables rooted
on a regular 16x16 spatial grid.  Each query selects the four grid anchors
nearest its target and runs one-sided A* with their maximum exact landmark
lower bound.  This targets the 300,000-query amortization directly; the
high-weight verified torus (wide64) and non-square lattice3d keep their prior
paths.  Storing the current heap key per vertex also avoids recomputing the
four-table potential merely to reject stale heap entries.

The integrated path answered all 300,000 local2d_large queries exactly in
18.37s / 689MB, versus 37.633s in the formal checkpoint (2.05x).  Preprocessing
for the 256 tables was 5.34s.  Fresh seed 97531024 / salt `local-grid256`
matched refsolve on all 10,000 dev queries, and all six official dev workloads
remain exact after integration.  A 64-anchor version was exact but slower on
large (21.75s / 300MB); a single-nearest-anchor query path was slower on dev.
The wide64 regional bounded-ball experiment was also rejected: 64 balls added
cost but left dev at 0.52s versus about 0.38s current.  Checkpoint:
`research/checkpoints/candidate-local-grid256.cpp`, SHA256
`4eebb8cbae9588454878d2175b0e55ba1df95a3c804f87d23a45bc1c91c658cd`.
Selecting the strongest four endpoint-difference landmarks from all 256 tables
for every query improved the long tail but added needless scanning to the local
majority.  The retained hybrid uses the four coordinate-nearest anchors when
observable torus Manhattan displacement is at most 96, and scans all tables
only above that bound.  It is exact regardless of the branch.  The integrated
hybrid improved local2d_large further to **14.20s / 689MB**, a 2.65x gain over
the formal 37.633s result; fresh seed and dev remain exact.  Updated checkpoint:
`research/checkpoints/candidate-local-grid256-hybrid.cpp`, SHA256
`e4a789fdd11ffc57cc32986d8c058f7fb14334da19fd915df54216d292285e3a`.
The first local256 checkpoint alone projected to 60.8x with the wide64 result;
the tail hybrid later reached 63.5x before adding the lattice3d result below.

### Lattice3d global landmark expansion (2026-09-26)

Increasing the farthest-point global landmark set from 24 to 128 gives a
meaningful win on the uniformly distributed lattice3d rank queries. The exact
candidate took 25.21s / 361MB on lattice3d_large and matched all 30,000 answers
(1.46x versus the formal 36.928s). The 128-table version also passed
lattice3d_dev and a fresh seed 97531025 / `lattice128` (10,000/10,000 exact).
Topology dispatch now verifies a cubic vertex count and every undirected edge's
3-D torus Manhattan adjacency before selecting 128 landmarks; all other
undirected workloads keep their prior count/path. All six official dev sets
and the fresh graph pass after integration. Candidate before formal grade:
`research/checkpoints/candidate-local256-lattice128-widefwd.cpp`, source SHA256
`062942028d547557130c899270facfeaf2b0191f7164e51fed6c369022308234`.
Combined targeted score projection against the previous formal checkpoint was
67.64x (+30.2%). The complete `grade.py` run has now formally certified
**67.886x Overall**. Six instance results were: road2d 28.861s / 33.718x /
230MB; lattice3d 23.531s / 71.510x / 353MB; local2d 14.375s / 43.626x /
673MB; scalefree 14.511s / 127.022x / 53MB; hugeq 11.428s / 182.119x /
1174MB; wide64 28.511s / 40.223x / 632MB. All six notes are `ok`; formal
Global geomean 86.420x, Local geomean 41.890x, Overall 67.886x. The result
JSON records 3 solver repeats, 5 baseline repeats, and the enforced 4GiB cap.
Certified source SHA256 `062942028d547557130c899270facfeaf2b0191f7164e51fed6c369022308234`;
result JSON SHA256 `d5f5d58e87e0d5a870b5ff6155cd2a4f128ee5294aa90398e729a2dc7bd314c7`.
Matching source/result are preserved at
`research/checkpoints/certified-local256-lattice128-widefwd.{cpp,json}`.

Combining 24 global and 36 spatial ALT landmarks on wide64 cut dev heap pops
from 4.02M to 3.17M (21%) but took 0.46s versus 0.39s serially; the extra 36
full SSSPs and endpoint selection erase the query reduction. Rejected before
large testing.

Additional fail-fast experiments: truncating a denser 32x32 local2d landmark
grid to exact distance labels within radius 137 preserved admissibility and
passed all 300,000 official local2d answers, but took 44.27s / 1.29GB versus
14.375s / 673MB for the full 16x16 grid. The 38.7M stored nearby labels still
left too much search outside coverage and added table lookup cost. Directed
ALT inside bidirectional CH returned wrong answers because the directed
potentials do not support the required reverse reduced costs; switching to
symmetrically relaxed landmark distances did not repair the CH stopping bound.
Expanding all CH arcs and running single-sided directed ALT was exact but took
3.05s on road2d_dev versus 1.53s current. A fixed nested-dissection witness-CH
ordering produced 1,362,728 dev shortcuts versus 363,811 for dynamic CH and
took 12.65s; it was rejected. No-witness customization on the same order
produced 4,465,545 shortcuts and 29.50s dev. These road ordering paths were
rejected before large testing.

Other architecture tests rejected this turn: a 4x4x4 grid of 64 exact
lattice3d regional landmarks took 2.48s on dev (2.02s current); composing those
64 tables with the 24 global tables took 1.59s, below a useful large-tier
gain. Expanding the global set to 128 helped dev at 1.54s and large at 25.21s;
256 tables rose to 1.87s on dev. A bounded longer-detour edge certificate only
raised lattice3d sparsification from 44.96% to 50.02% and slowed dev from 1.99s
to 2.18s. Wide64's 6x6 regular landmark grid measured 0.43s dev versus 0.37s
production. Road64 directed spatial ALT took 1.35s dev versus 1.53s CH and was
rejected. Reusing a full reverse CH target label for repeated road targets
measured 42.81s on road2d_large versus 28.86s CH; the reverse-label work does
not amortize enough across mostly singleton targets.

### Single-thread certification constraint and follow-up (2026-09-26)

An 8-worker offline query-batch prototype was exact on all six official large
answer files and all six fresh-seed dev sets (seed 20260926). Its targeted
large times were road2d 18.73s, lattice3d 12.01s, local2d 8.13s, scalefree
13.48s, hugeq 7.62s, and wide64 14.46s. It also passed a fresh-graph hugeq
separator-dispatch test (300,000 queries). However, the required formal grade
classified every row as `THREADS`: CPU time exceeded 1.4x wall time. Parallel
execution is therefore ineligible and is not integrated. The grader result is
preserved as `result-parallel-batch-rejected-threads.json`; prototype source is
`research/prototypes/local_grid_parallel.cpp`, SHA256
`b91f39a00fd914bf55b0973e82f4269be8041a5c8047d4bbe3f6f2197e01a006`.
Production was restored byte-for-byte to the certified source SHA
`062942028d547557130c899270facfeaf2b0191f7164e51fed6c369022308234`.

Target-aware road CH ordering added a 64x target-frequency priority bonus.
It remained exact on road2d_dev, but increased dev shortcuts from 363,811 to
410,477 and runtime from 1.59s to 2.05s; rejected before a large run. A
four-edge strict-lower-weight detour extension for wide64 sparsification found
no additional removals on wide64_dev (still 9.574%), with preprocessing rising
from about 0.005s to 0.007s; rejected. Using coordinate-nearest local-grid
anchors for every query was exact on local2d_dev but measured 0.95s versus
0.96s retained, too small to justify weakening long-query landmarks; rejected
before large testing.

### Exact work-balanced lattice ALT (candidate, not yet graded)

On the sparsified 3D torus, selecting the next ALT frontier by cumulative
relaxed-arc work rather than minimum key preserved the exact min-key stopping
bound and was exact on lattice3d_dev and a fresh seed (20260926). On the
official dev graph, settled pops fell from 9.46M to 5.29M and arc scans from
32.41M to 18.32M; runtime fell from 1.69s to 1.27s. Fresh-seed counts were
5.38M pops / 18.61M arcs. On lattice3d_large, the candidate was exact at
17.46s / 362MB versus 24.96s / 362MB for an instrumented key-balanced build;
large work fell from 120.46M pops / 410.61M arcs to 68.15M / 234.28M. The
production dispatch applies this policy only to verified cubic 3D tori.
Candidate SHA256 is `06c286d118ac3b21b8d0710debce348ca67b6b51c3e056d97137cefce8c5d28a`
(`research/checkpoints/candidate-lattice-workbalanced.cpp`). All six dev
answer comparisons pass. This improves one workload by 1.43x, so the formal
67.886x checkpoint remains authoritative and the full grade stays deferred.

Road CH relaxed-arc balancing was also fail-fast tested. With the same CH
hierarchy and exact conservative `af >= best && ab >= best` termination,
frontier choice by cumulative stall/relaxation arc scans remained exact on
road2d_dev but increased work from 1.06M to 1.13M settled CH nodes and from
13.70M to 14.45M arc scans; runtime was 1.62s versus 1.59s. It is rejected
before large testing. Prototype: `research/prototypes/road_work_ch.cpp`.

Classic edge-difference road CH priority remained exact on dev but produced
366,946 shortcuts instead of 363,811 and took 1.60s instead of 1.59s. It is
rejected before large testing; prototype:
`research/prototypes/road_edge_difference_ch.cpp`.

### Directed road sparsification (candidate, not yet graded)

Before CH preprocessing, a directed road-only exact certificate removes an
arc when a two- or three-edge directed detour has total weight no greater than
the arc and every detour arc is strictly lighter. Weight induction proves
simultaneous removals preserve all pairwise distances. On road2d_dev this
removed 19.8% of arcs, lowered CH shortcuts from 363,811 to 322,816, and took
1.09s versus 1.59s. Fresh seed 20260926 was exact and removed 19.9%. On
road2d_large it removed 17.5%, lowered shortcuts from 2.765M to 2.489M, and
was exact at 23.49s / 213MB versus the certified 28.86s / 227MB. Combined with
the lattice work-balanced path, the candidate is now `solver.cpp` and
`research/checkpoints/candidate-road-sparse-lattice-work.cpp` at SHA256
`132c5ed50923ce6fb49543fc2a8f02d50f492c73f33939c9a614f680d3998fd7`; all six
dev answer comparisons pass. The 67.886x formal checkpoint remains the only
certified Overall.

Local2d A* tie-breaking toward larger g within equal f shells preserved exact
answers but settled 1.065M nodes and scanned 3.844M arcs on dev, essentially
the retained path's 1.06M / 3.87M; total dev time increased from 0.96s to
1.01s with a binary heap. Rejected before large testing. Prototype:
`research/prototypes/local_grid_deep_tie.cpp`.

Extending the undirected lattice sparsifier to 4-edge strict-lower-weight
witnesses removed no additional arcs on lattice3d_dev (44.956% unchanged),
with no runtime change after combining with work-balanced ALT; rejected.
Prototype: `research/prototypes/lattice_work_sparse4.cpp`.

An exact scale-free upper-bound index built full forward/reverse SSSP tables
from the 64 highest-total-degree vertices and seeded the retained work-balanced
search with the best hub path. It was exact on scalefree_dev, but preprocessing
took 0.369s and total dev time was 0.84s versus 0.45s retained. The upper bound
did not repay index cost; rejected before large testing. Prototype:
`research/prototypes/scalefree_hub_bounds.cpp`.

On top of directed road sparsification, 16 full forward/reverse SSSP pairs
from the highest-degree intersections seeded each CH query with an exact
source-hub-target route upper bound. It remained exact on road2d_dev but added
0.132s of preprocessing and raised total time from 1.09s to 1.23s; the query
phase is too small at dev scale to repay the tables, so this is rejected before
large testing. Prototype: `research/prototypes/road_hub_upper_ch.cpp`.

The four-edge directed road sparsifier improved removal from 19.812% to
19.979% on road2d_dev, lowered shortcuts by only 554, and raised total time
from 1.09s to 1.11s. Rejected before large testing; prototype:
`research/prototypes/road_directed_sparsify4.cpp`.

Composing road CH work balancing with directed sparsification remained exact
on road2d_dev but increased total time from 1.09s to 1.11s, with 937k settled
CH nodes and 10.02M arc scans. It is rejected before large testing; prototype:
`research/prototypes/road_sparse_work_ch.cpp`.

The strict-lower-weight directed 2/3-edge certificate was also applied to the
coordinate-free scale-free path before grouped queries. It remained exact on
scalefree_dev, but removed only 31 of 145,587 arcs (0.021%), took 0.46s versus
0.45s retained, and therefore cannot repay preprocessing; rejected before large
testing. Prototype: `research/prototypes/scalefree_directed_sparsify.cpp`.

Repeated road targets were tested with a full reverse-upward CH search cached
once per target, followed by one forward CH search per source and exact
peak-intersection. On road2d_dev it answered all groups exactly, but groups
covered only 233/10,000 queries (37 targets); shared work was 46.5k settled
nodes / 341k arcs against 882.9k nodes / 9.53M arcs for the ordinary whole
batch. Total time remained 1.09s, so the batch share is too small to merit a
large run. Prototype: `research/prototypes/road_ch_target_batch.cpp`.

## Fail-fast architecture tests (2026-09-26)

Exact 2/3-edge directed sparsification was composed with the hugeq recursive
separator path on a fresh generated graph (seed 20260926), using a forced
300,000-query separator batch. It removed 53,976 / 271,129 arcs (19.908%) and
answered all queries exactly, but runtime improved only from 6.54s / 1.20GB to
5.29s / 1.11GB (1.24x). It remains isolated at
`research/prototypes/hugeq_sparse_separator.cpp`.

A nested-dissection CH for verified local2d torus structure returned exact
answers on dev but was rejected before large testing. Full contraction created
1.83M shortcuts on 50,176 vertices and took 27.82s / 59MB; partial 25%
contraction reduced fill to 480,072 shortcuts but still took 9.54s / 45MB.
Prototype: `research/prototypes/local_nested_ch.cpp`.

Two wide64 query-engine alternatives were exact on wide64_dev but failed the
dev timing gate: work-balanced bidirectional ALT took 0.44s versus 0.38s for
the retained forward four-landmark ALT (1.53M settled pops / 5.56M arc scans
per direction across the batch), and full 24-landmark one-sided ALT took
0.51s. Neither was tested on large. Prototypes:
`research/prototypes/wide64_workbalanced.cpp` and
`research/prototypes/wide64_full24_alt.cpp`.

## Hugeq separator query reuse (2026-09-26)

The production separator fallback groups same-leaf queries by target and runs
restricted reverse Dijkstra. I compared it with grouping the same unhandled
queries by source and running restricted forward Dijkstra, using the first
251,000 official hugeq_large pairs (Q/V=5.002, so this exercises separator
dispatch). Both candidates matched every corresponding official answer. The
target grouping took 8.94s / 1.19GB and 29.34M settled grouped-search vertices
/ 158.92M scanned arcs; source grouping took 9.12s / 1.19GB and 29.71M / 160.93M.
Source reuse is slightly worse, so it is rejected. Prototype:
`research/prototypes/hugeq_source_group_separator.cpp`.

Separator distance tables were also compacted from 64 to 32 bits after a full
range check; overflow leaves the 64-bit path intact. On a fresh forced 300k
separator batch, answers were exact but total time was 6.94s versus 6.54s for
the 64-bit version; peak RSS was 1.24GB during table conversion. The conversion
cost erased any scan benefit, so this prototype is rejected:
`research/prototypes/hugeq_u32_tables.cpp`.

### Multi-goal ALT inside separator fallback

For each same-target, same-leaf fallback group, the search uses a consistent
ALT lower bound to the nearest source endpoint in the group. For each
landmark, the minimum source-to-landmark and maximum landmark-to-source
distances give group-wide lower-bound components; taking their nonnegative
maximum preserves consistency. Each requested source is still settled at its
exact distance, and the search runs until every requested source is answered.
Twelve directed landmark tables are built only on the structural directed,
coordinate, Q/V>=5 separator branch.

On the first 251,000 official hugeq_large pairs, this cut grouped fallback work
from 29.34M pops / 158.92M arcs to 7.00M / 37.97M and runtime from 8.94s to
7.65s; all answers matched. On all 600,000 official hugeq_large pairs, the
prototype returned all answers exactly in 10.10s / 1.21GB, versus 12.58s /
1.20GB for the paired baseline run. After integration, another targeted run
was exact in 10.92s / 1.21GB. The less favorable integrated recheck is about
1.15x versus the paired baseline and 1.05x versus the prior formal hugeq
runtime; this is useful but not a multi-fold result. Fresh seed 20260926 with
a forced 300,000-query separator batch was exact, as were all six official dev
answer files after integration. Prototype:
`research/prototypes/hugeq_multigoal_alt.cpp`. Current source SHA is recorded
above; the formal Overall remains 67.886x because no full grade was run. A
rough one-workload projection is about 68.4x, not a certified score.

## Scale-free multi-goal ALT trial (2026-09-26)

The scale-free dev mix has 2,997 hub-target queries, so I tried reusing the
same multi-goal ALT bound for repeated target groups. The first prototype had
the reverse-direction landmark terms swapped and failed at dev query 50; the
terms were corrected before evaluating the candidate. The corrected version
was exact on all scalefree_dev answers, but took 29.01s / 25MB versus 0.46s /
13MB for the retained path, despite settling 2.43M grouped vertices / scanning
7.13M arcs. Twelve full forward/reverse landmark SSSPs plus per-pop group ALT
cost overwhelmed the work saved. Rejected before fresh-seed or large testing;
prototype `research/prototypes/scalefree_multigoal_alt.cpp`.

## Additional architecture trials (2026-09-26)

The exact coarse-metric lower bound on local2d was built by contracting each
4x4 block of the verified torus and assigning each coarse interface its
minimum original crossing-edge weight. The relaxed-graph distance is
consistent, but on local2d_dev it did not reduce the existing ALT search
materially (1.061M settled / 3.86M scanned arcs) and increased total time from
1.01s to 1.64s due to 0.62s coarse-table preprocessing. Rejected before large
testing: `research/prototypes/local_coarse_metric4.cpp`.

Dynamic-priority witness CH was then tested on the same topology (distinct
from the earlier fixed nested-dissection order). It was exact on local2d_dev,
but built 367,998 shortcuts on 50,176 vertices, settled 959k query nodes and
scanned 28.71M arcs; 10k queries took 1.46s / 33MB versus 1.01s retained. It
was rejected before large testing: `research/prototypes/local_dynamic_ch.cpp`.

Same-target local2d queries were retested with a shared reverse A* search,
using the exact four nearest regular-grid landmark tables to bound distance to
the nearest source endpoint. The interval landmark bound is consistent, so
each requested source is exact when popped. The prototype matched all
local2d_dev answers, but took 1.33s versus 0.97s for retained one-sided A*;
it settled 4.05M grouped vertices and scanned 14.88M arcs on the grouped
portion. On local2d_large, preprocessing alone took 5.41s and the grouped
search exceeded 90s without producing a complete answer file, so it was
interrupted. This repeats the earlier batch finding: many target duplicates
do not imply enough shared exploration. Rejected. Prototype:
`research/prototypes/local_target_group_alt.cpp`. Production `solver.cpp`
remains unchanged at SHA256 `e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`.

The wide64 forward ALT query was also tested as bidirectional ALT with
frontier expansion balanced by scanned arcs, while preserving the exact
reduced-cost minimum-key termination condition. It matched every
wide64_dev answer but took 0.53s versus the production 0.37s; it also took
0.53s versus the previously measured ordinary biALT 0.46s. Although its
3.07M pops / 11.12M scanned arcs are below the recorded ordinary biALT
3.38M / 12.23M, the extra two-frontier work outweighs the reduction. Rejected
before large testing. Prototype:
`research/prototypes/wide64_workbalanced_alt.cpp`.

A wide64-only fifth-edge replacement certificate was tested next. It searched
simple paths of four or five edges with total weight no greater than the
removed edge, and only for edges whose weight could cover five minimum-weight
steps. The candidate remained exact on wide64_dev, but sparsification rose
only from 9.574% to 9.753% of arcs (358 additional directed arcs) and end-to-
end time was 0.37s versus 0.36s. The mechanism has no credible multi-fold
upside; rejected before large testing. Prototype:
`research/prototypes/wide64_sparsify5.cpp`.

Local2d's 256 exact landmark tables were then stored as 32-bit distances
under the structural proof `(V-1)*max_edge_weight < UINT32_MAX`, with a
connectedness check on every built table. This matched local2d_dev exactly
and cut peak RSS from 113MB to 63MB, but time improved only from 0.92s to
0.88s. Since the measured speed gain is under 5%, it was rejected without a
large run. Prototype: `research/prototypes/local_grid_u32.cpp`.

Road target batching was broadened to every repeated-target group (multiplicity
at least two). On road2d_dev it remained exact but took 1.11s versus 1.06s for
the combined road-sparsified CH path. Since the large query set has repeated
targets in 53.5% of queries versus 23.4% on dev, I ran the targeted large
instance. It was exact, but took 36.54s / 213MB and scanned 82.10M CH nodes /
1.024B arcs, versus the retained sparse-CH path's 23.49s. The broader target
cache loses to repeated bidirectional CH work even at large multiplicity;
rejected. Prototype `research/prototypes/road_ch_target_batch2.cpp`.

Checkpoint accounting: `grade.py` computes Overall as the geometric mean of
the six per-instance speedups (`exp(mean(log(speedup)))`). Substituting the
exact targeted road2d 23.49s, lattice3d 17.46s, and hugeq 10.92s candidate
times into the six formally certified rows, while holding local2d, scalefree,
and wide64 at their certified times, gives only a **74.40x projected Overall**.
This is not a grade result; the same `solver.cpp` SHA remains ungraded. The
three measured gains multiply the six-workload product by 1.733, or Overall
by about 9.6%, so another full grade remains low-value until more gains
accumulate. Official command remains:
`python3 -u grade.py --solver ./solver --instances instances.txt --json <result.json>`.

Road CH witness searches were raised from 200 to 2,000 settled nodes per
shortcut query while leaving the safe early cutoff and query algorithm intact.
This remained exact on road2d_dev, but changed the directed-sparsified CH
shortcut count only from 322,816 to 322,815 and runtime from 1.05s to 1.06s.
The 200-settled cap is not driving hierarchy fill; rejected before large
testing. Prototype `research/prototypes/road_witness2000.cpp`.

For lattice3d, an exact 3-D block relaxation was composed with 128-landmark
work-balanced biALT. Every original edge crossing blocks is represented by
the minimum weight on that interface, and intra-block motion costs zero, so
coarse shortest paths are consistent lower bounds. On lattice3d_dev it
matched all answers, but the search work was unchanged at 5.286M pops /
18.314M arcs (the retained work-balanced path is 5.29M / 18.32M); total time
rose from 1.18s to 1.60s and RSS from 66MB to 104MB. The 2,197-cell exact
coarse table supplied no additional pruning; rejected before large testing.
Prototype `research/prototypes/lattice_coarse_exact_alt.cpp`.

For local2d, shortest-direction grid routes were evaluated as cheap feasible
upper bounds; equality with the existing four-anchor ALT lower bound would
certify a query without search. The candidate matched every local2d_dev answer
but certified zero queries and took 0.90s versus 0.92s baseline (within noise,
with no eliminated search); it adds path-sum work without useful certificates.
Rejected before large testing. Prototype
`research/prototypes/local_canonical_cert.cpp`.

An additive metric decomposition was tested on lattice3d: block-interface
minimum edge costs form the baseline graph, and each crossing edge's excess
weight forms a residual graph. The sum of lower bounds in those two graphs is
consistent by construction. With 2x2x2 blocks, 24 coarse landmarks, and 128
residual landmarks, the candidate matched lattice3d_dev exactly but expanded
22.82M nodes / scanned 78.14M arcs versus 5.29M / 18.32M for retained
work-balanced ALT. Runtime rose from 1.21s to 4.39s (preprocessing 0.39s).
The residual graph's zero-cost interface edges make its ALT bound too weak;
rejected before large testing. Prototype
`research/prototypes/lattice_additive_metric.cpp`.

The same additive interface-baseline/residual decomposition was evaluated on
wide64, where high log-uniform edge weights suggested a larger baseline
fraction. With 2x2 blocks and 24 ALT landmarks in each component, it matched
wide64_dev but expanded 11.69M nodes / scanned 42.22M arcs, versus production's
3.38M / 12.23M; 0.124s of extra preprocessing raised total time from 0.36s to
1.33s. The residual zero-cost interface network weakens its ALT bounds here as
well. Rejected before large testing. Prototype
`research/prototypes/wide64_additive_metric.cpp`.

For wide64, a 2x2-cell relaxed torus with 24 coarse ALT landmarks was exact on
wide64_dev. It changed work from 3.379M pops / 12.228M arcs to 3.358M /
12.155M (about 0.6% less), while total time rose from 0.37s to 0.50s. Rejected
before large testing: `research/prototypes/wide64_coarse_alt.cpp`.

Finally, separator path upper bounds were compared with the already-built
12-landmark directed ALT lower bound. Equality would certify a query without
fallback search; it matched only 50 of the first 251,000 official hugeq_large
queries, and time was 7.62s versus 7.65s without the check. Rejected:
`research/prototypes/hugeq_alt_certificate.cpp`.

### Resumable shared target frontier for road CH

Hypothesis: repeated road2d targets might reuse a lazy backward CH Dijkstra
frontier while each source query uses its exact bidirectional min-key cutoff,
avoiding the full reverse-target cache previously rejected. The isolated
prototype `research/prototypes/road_shared_target_ch.cpp` retains the target
heap and tentative distance labels across a target group, combines them with
per-source forward labels as valid upper bounds, and uses no CH stalling on
this shared path.

The candidate compiled with `g++ -O2 -std=c++17`. Official road2d_dev output
matched all answers (1.12s, 34MB versus 1.12s / 34MB for the certified solver).
Fresh seed 603426 / salt `shared-target-test` also matched all 10,000 answers
from `tools/ref/refsolve` exactly. A targeted road2d_large run matched all
200,000 official answers but took 26.41s / 213MB, versus 22.046s / 208MB for
the formal checkpoint. The persistent-frontier bookkeeping does not repay
sharing, so this architecture is rejected. Production `solver.cpp` remains
unchanged at SHA256 `e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`.

### Large local-tile certificate coverage diagnostic

To bound the upside of exact intra-tile paths, ran
`/tmp/local_tile_probe instances/local2d_large.graph instances/local2d_large.queries instances/local2d_large.answers 32`.
Of 300,000 queries, 152,255 (50.75%) had endpoints in one 32x32 tile;
149,642 (49.88% of all queries) had an intra-tile restricted shortest path
equal to the official global answer. The restricted searches settled 26.96M
nodes and took 3.37s with a binary heap. This is an upper-bound diagnostic,
not a solver result: a candidate must still prove equality using an admissible
global lower bound, and only the remaining half of queries could be helped.
It caps plausible upside below a multi-fold local2d gain, so I am not building
an all-pairs tile table around this mechanism.

### Hugeq repeated-source ALT grouping

Hypothesis: the 600k-query hugeq workload has about 12 queries per source, so
for unhandled same-leaf pairs a forward multi-goal ALT search per source might
share more than the current reverse searches grouped by target. I added the
dual directed-ALT bounds (minimum landmark-to-goal distance and maximum
goal-to-landmark distance) to an isolated copy of the four-level solver in
`research/prototypes/hugeq_source_alt.cpp`. Its compile-time test switch forces
the separator branch on smaller query batches; production dispatch is not
changed. A matching target-group comparator with identical forced dispatch is
`research/prototypes/hugeq_target_alt.cpp`.

Fresh seed 603427 / salt `hugeq-source-alt` used the hugeq_dev graph structure
with 100,000 uniform queries and balanced source reuse. `tools/ref/refsolve`
produced the exact answers. With `g++ -O2 -std=c++17 -DFORCE_SEPARATOR_FOR_TEST=1`,
target grouping was exact at 5.04s / 1.056GB and 2.111M pops / 9.043M arcs;
source grouping was exact at 4.94s / 1.056GB and 2.059M / 8.814M. The source
candidate also matched all 10,000 official hugeq_dev answers with the
separator branch forced (4.84s / 1.10GB). The roughly 2% fresh-batch gain is
far below a useful path toward 150x, so no targeted large or full grade was
run. Only hugeq's research prototype branch differs; production remains the
79.118x checkpoint at SHA256 `e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`.

### Directed ALT on the road contraction hierarchy

Hypothesis: directed ALT potentials remain consistent on CH shortcut arcs,
and a reduced-cost bidirectional search could prune road CH queries. The
prototype `research/prototypes/road_ch_bialt.cpp` built 12 directed ALT
landmarks after exact road sparsification. A sum-of-frontier reduced-key
cutoff was incorrect on road2d_dev; checking both frontier minima against the
best reduced route restored exactness. With all 12 terms, six dev sets and a
fresh seed were exact, but road2d_large took 28.09s / 273MB, slower than the
formal 22.046s. The full 12-term potential's random table traffic outweighed
its search pruning.

A fixed strongest-four subset from those 12 directed landmarks was tested in
`research/prototypes/road_ch_bialt4.cpp`. It matched all six dev answer files,
fresh road seed 603428 / salt `road-ch-bialt`, and all 200,000 road2d_large
answers. On road2d_dev it reduced query work from 882,865 pops / 9.53M arcs
to 454,373 / 5.65M, while runtime rose 1.03s to 1.14s. On road2d_large it
measured 21.15s / 273MB versus 22.046s / 208MB for the formal source, a 4.2%
single-workload gain. Holding the other five official rows fixed projects
only about 79.65x Overall; no full grade was run and production remains the
79.118x e052 checkpoint. This exact composition is retained as a possible
small cumulative gain, not a route to 150x by itself.

Reproduction commands were `g++ -O2 -std=c++17 research/prototypes/road_ch_bialt4.cpp -o /tmp/road_ch_bialt4`, followed by `/usr/bin/time -v /tmp/road_ch_bialt4 instances/road2d_large.graph instances/road2d_large.queries /tmp/road_ch_bialt4_large.out`; output was compared byte-for-byte with `instances/road2d_large.answers`. Fresh answers came from `python3 -m tools.gen --manifest tools/manifest/public.json --instance road2d_dev --seed 603428 --salt road-ch-bialt --out /tmp/road-ch-bialt-fresh` and `tools/ref/refsolve plan ...`.

A balanced directed ALT potential was then tested in
`research/prototypes/road_ch_balanced_alt.cpp`. It combines source-to-vertex
and vertex-to-target ALT lower bounds into a feasible doubled reduced cost on
CH arcs. Official six-dev, fresh seed 603429 / salt `road-balanced-alt`, and
road2d_large outputs were all exact. Large measured 21.42s / 273MB with
14.94M pops / 264.19M scanned arcs; strongest-four unbalanced ALT was faster
at 21.15s / 273MB despite 17.99M / 318.93M. More pruning did not repay the
two-ended potential work. Reproduction: `g++ -O2 -std=c++17 research/prototypes/road_ch_balanced_alt.cpp -o /tmp/road_ch_balanced_alt` and `/usr/bin/time -v /tmp/road_ch_balanced_alt instances/road2d_large.graph instances/road2d_large.queries /tmp/road_ch_balanced_alt_large.out`. Keep isolated.

### Local2d vertex-major landmark storage probe (2026-09-26)

Hypothesis: the 256 exact landmark tables are currently landmark-major, so each
per-vertex heuristic reads four widely separated cache lines; transposing them
to vertex-major should reduce heuristic table traffic. A standalone copy was
built at `research/prototypes/local_grid_vertex_major.cpp`; production
`solver.cpp` remains unchanged at SHA256
`e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`.

Commands: `g++ -O2 -std=c++17 research/prototypes/local_grid_vertex_major.cpp
-o /tmp/local_grid_vertex_major`, then timed runs on `local2d_dev` and
`local2d_large`, compared byte-for-byte to their official answers. It was exact
on both. The dev run was 1.14s / 113MB versus current 0.94s / 113MB. On large,
the prototype took 24.16s / 673MB versus 14.18s / 673MB for the same-source
production binary (formal grade row 15.275s). Landmark preprocessing rose from
5.04s to 6.03s, and total wall time regressed by 70%; this data layout is
rejected. No fresh seed or production edit was needed after the clear large
regression.

### Local2d circular-bucket A* queue probe (2026-09-26)

For the verified undirected torus path, an ALT-consistent key changes by at
most twice the maximum edge weight per arc. With observed max weight 1,000, a
2,001-slot circular Dial queue is exact and could avoid radix-heap work. The
isolated `research/prototypes/local_grid_dial.cpp` compiled with
`g++ -O2 -std=c++17` and matched all local2d_dev answers. It settled 1.159M
nodes / scanned 3.831M arcs and took 0.89s versus 0.94s for production, only
about a 1.06x total gain; landmark preprocessing remained about 0.77s. Since
the queue is a small part of the already-short dev run and the observed total
gain is far below the multi-fold threshold, this candidate was rejected before
large/fresh-seed testing. Production source and the certified checkpoint
remain unchanged.

### Wide64 endpoint-distribution check (2026-09-26)

Before testing a query-aware landmark-root scheme, the official large query
set was measured directly. wide64 has 30,000 queries, 30,000 unique sources,
29,775 unique targets, and a maximum source-cell share of 0.15% over a 32x32
torus partition. Torus-Manhattan displacement has p50/p90/p99 of 188/630/799
on side length 1,414. This is effectively a broad, long-range query mix rather
than a concentrated endpoint workload; target/source-root sampling has no
observable concentration to exploit and is not a credible multi-fold route.
The current farthest-point global landmark selection remains the supported
choice. No source or formal artifact changed.

### Wide64 min-plus-residual metric decomposition (2026-09-26)

Hypothesis: on a verified weighted torus, decompose each edge weight as the
global minimum `m` plus nonnegative residual `w-m`. Then `m * torus-Manhattan`
plus a residual-graph ALT bound is admissible and consistent for the original
metric, potentially strengthening the expensive wide64 searches. The isolated
prototype `research/prototypes/wide64_residual_alt.cpp` built 24 farthest-point
landmarks in the residual graph before the production-safe original-edge
sparsifier, then used a balanced bidirectional ALT query with the summed
geometric/residual potential.

`g++ -O2 -std=c++17 research/prototypes/wide64_residual_alt.cpp -o
/tmp/wide64_residual_alt` ran on `wide64_dev`; output matched all official
answers exactly. It took 0.59s / 21MB and settled 4.684M nodes / scanned
16.943M arcs, versus about 0.38s and 3.38M / 12.23M for the retained path.
The residual metric did not preserve useful landmark pruning after adding the
geometric term; the exact candidate was 1.55x slower on dev. It was rejected
before fresh-seed or large testing, and production remains unchanged.

### Scalefree one-landmark balanced directed ALT (2026-09-26)

Hypothesis: the retained scale-free path balances relaxed arcs but has no
heuristic; a single high-degree landmark's directed ALT potential might reduce
search while retaining cumulative-arc work balancing, at much lower table cost
than the rejected 12-landmark multi-goal ALT. The exact prototype
`research/prototypes/scalefree_balanced_alt1.cpp` uses both forward and reverse
landmark distances and the feasible doubled potential
`p(v)=h(v,t)-h(s,v)`; the stop condition checks the two reduced frontier minima.

It compiled with `g++ -O2 -std=c++17` and matched every scalefree_dev answer.
On dev it took 0.50s / 15MB and, for non-grouped queries, settled 2.680M nodes
and scanned 12.844M arcs. The retained production run is 0.46s / 13MB; the
candidate adds landmark preprocessing and per-node potential reads without
enough pruning to show a gain. It was rejected before fresh-seed or large
testing. Production SHA remains `e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`.

### Road hot-target cache coverage screen (2026-09-26)

Hypothesis: a hybrid of per-query CH and exact reverse target searches for only
the hottest targets could recover some reuse without the cost of the rejected
full target cache. Counting target multiplicities in the official query files
shows the possible coverage is too small: road2d_large has 200,000 queries;
only 2,424 (1.21%) belong to 55 targets with multiplicity at least 32, and
only 425 queries (0.21%) belong to the four targets with multiplicity at least
64. The dev batch has no target with multiplicity >=32. Even a free exact
answer for every >=32 group could not halve road2d time, so this mixed-cache
architecture has no credible multi-fold upside and was rejected before code.
The production road CH and formal checkpoint are unchanged.

### Local2d 16-anchor heuristic probe (2026-09-26)

The 16x16 landmark grid's nearest 4x4 anchors were tested for short local2d
queries (the long-displacement path retained the strongest-four selection).
This preserves exact ALT consistency. `research/prototypes/local_grid_16anchors.cpp`
compiled with `g++ -O2 -std=c++17` and matched local2d_dev exactly. It reduced
dev search work from the retained 1.06M pops / 3.87M arcs to 916k / 3.09M
(about 21%), but elapsed time changed only from 0.94s to 0.88s because each
heuristic evaluates four times as many table entries. This fails the required
multi-fold potential, so no large or fresh-seed run was justified. Production
remains the certified source.

### Scalefree high-hub ALT certificate coverage (2026-09-26)

The prior 64-highest-degree hub-bound prototype had a high raw bound-hit rate,
so I measured whether its directed ALT lower bounds could certify those routes
without point-to-point search. On scalefree_dev, the 64-hub route upper bound
equaled the exact answer for 8,155/10,000 queries; however, production target
grouping had already handled 3,010 queries. Among the remaining 6,990, the
ALT lower bound met the hub upper bound for only 60 (0.86%) with 64 hubs, 12
with 16 hubs, and 2 with four hubs. The large raw hit rate is therefore almost
entirely redundant with the existing repeated-target grouped search; additional
hub tables cannot eliminate meaningful remaining query work. The diagnostic
used no answer-file data inside the solver: it compared the hub bound with the
exact result already returned by the existing search. Output matched all
scalefree_dev answers. No fresh/large run was justified; production remains
unchanged.

Reproduction: `g++ -O2 -std=c++17 research/prototypes/scalefree_hub_hit_probe.cpp -o /tmp/scalefree_hub_hit_probe`, then `/usr/bin/time -f 'elapsed %e rss_kb %M' /tmp/scalefree_hub_hit_probe instances/scalefree_dev.graph instances/scalefree_dev.queries /tmp/scale_hub_ungrouped.out`; output was byte-compared with `instances/scalefree_dev.answers`. The 64-table diagnostic took 0.80s / 65MB including 0.33s landmark preprocessing, versus 0.45s / 13MB retained.

### Undirected local2d recursive separator-label probe (2026-09-26)

Hypothesis: local2d's short-rank queries could avoid repeated graph search by
using the existing exact recursive separator labels, with same-leaf queries
falling back to a search restricted to that leaf. A temporary copy of
`solver.cpp` derived `(x,y)` from node IDs only after verifying the 2D torus
edge topology, then forced the separator branch on local2d_dev. Dispatch used
observable structure and did not inspect answers or instance identity.

The command `g++ -O2 -std=c++17 /tmp/local_separator_test.cpp -o
/tmp/local_separator_test` followed by `/usr/bin/time -v /tmp/local_separator_test
instances/local2d_dev.graph instances/local2d_dev.queries
/tmp/local_separator_dev.out` returned byte-identical dev answers in 6.40s
with 1,255 MiB peak RSS, versus about 0.94s for the retained path. The table
representation allocates full-graph rows for each recursive region's
separator, so both memory and preprocessing grow superlinearly with grid side
length; scaling to the 548x548 large graph would exceed the 4 GiB cap by a wide
margin. Rejected before fresh-seed or large testing. Production and certified
artifacts are unchanged.

### Local2d grouped-landmark distance sketch (2026-09-26)

Hypothesis: a distance-to-set function is 1-Lipschitz, so its absolute
difference at two vertices is an exact consistent ALT lower bound. Grouping
the 256 local grid anchors into 64 nearby 2x2 sets could preserve useful local
gradients with one multi-source SSSP per set rather than 256 full SSSPs. The
64 grouped tables were exact on local2d_dev in 0.34s / 38MB versus about
0.94s / 113MB, but the large run took 19.86s / 234MB versus the formal
15.275s; the weaker potential increased query work enough to erase the
preprocessing savings.

A second variant kept one individual anchor per 2x2 block and selected the
strongest four among all 64 tables per query. It was also exact (0.34s / 38MB
dev) but took 18.33s / 234MB on local2d_large. Both variants are rejected;
the 256 individually rooted local grid landmarks remain the best supported
path. Temporary sources were `/tmp/local_grouped_anchors.cpp` and
`/tmp/local_64single.cpp`.

An exact Dial circular queue specialized to the verified local2d weight cap
(maximum edge weight 1,000) accelerated only landmark construction: dev
preprocessing was 0.686s versus roughly 0.77s with the radix heap, while total
dev time was 0.78s versus 0.94s. Since this affects only preprocessing and
large queries dominate the local workload, its projected overall effect is
too small for the 150x effort; it remains a rejected temporary probe at
`/tmp/local_dial_pre.cpp`.

Selecting the best four among the 16 anchors nearest each short query's target
was exact on local2d_dev, but took 0.91s versus 0.94s retained, while the
earlier all-16 heuristic had already shown only 21% fewer pops and no total
time gain. This landmark-selection refinement is rejected before large
testing (`/tmp/local_select16.cpp`).

### Wide64 large frontier-work check (2026-09-26)

The retained one-sided forward ALT path was instrumented on wide64_large. It
matched all 30,000 official answers in 28.83s / 647MB after exact sparsification,
with 213.08M settled pops and 770.84M scanned arcs. Since large queries reach
rank 2^17 while dev tops at 2^15, I rechecked the existing relaxed-arc-balanced
bidirectional ALT prototype on the full large batch. It was exact but slower:
36.06s / 647MB with 190.75M pops and 690.14M arcs. The ~10% work reduction
does not repay maintaining two frontiers and the bidirectional key math; it is
rejected at large, and production remains one-sided ALT.

### Road CH tighter-cutoff validation (2026-09-26)

Hypothesis: the sum of the two CH frontier minima might give a tighter exact
cutoff than requiring both minima independently to reach the best route. An
isolated source changed only that cutoff and compiled with
`g++ -O2 -std=c++17`. It returned an incorrect third road2d_dev answer
(expected 20,469,040; got 22,413,649). Removing stalling did not repair it,
so the classical bidirectional-Dijkstra sum bound does not apply to these
restricted CH frontiers as implemented. The candidate was rejected before
large testing.

I also tested updating the incumbent route immediately when a relaxation
reaches a vertex labeled by the opposite search. This is a valid route upper
bound and kept road2d_dev exact, but total dev time rose to 1.08s versus about
1.04s retained. It is rejected before large testing. Production CH dispatch
and its conservative cutoff are unchanged.

### Road high-hub upper-bound large recheck (2026-09-26)

The existing exact 16-high-degree-intersection upper bound had only been
measured on dev, where its preprocessing cost obscured potential query savings.
I recompiled `research/prototypes/road_hub_upper_ch.cpp` with
`g++ -O2 -std=c++17` and ran it on the full road2d_large batch. It matched all
200,000 official answers, but the 16 forward/reverse hub tables cost 0.892s
and total time was 22.71s / 288MB versus 22.046s / 208MB for the certified
source. The CH query reported 36.32M pops / 541.21M arcs. The hub upper bounds
do not compensate for their preprocessing and lookup cost at large scale; the
candidate is rejected without fresh-seed testing, and production remains
unchanged.


### Road spatial nested-dissection CH ordering (2026-09-26)

Hypothesis: a topology-only recursive coordinate separator order might retain a
small road core and cut CH query work by contracting region interiors before
separator vertices. `research/prototypes/road_spatial_ch.cpp` recursively splits
the supplied coordinates, marks endpoints of cross-partition arcs as separators,
and assigns all separator vertices after both children. It changes only CH
ordering; witness construction and exact CH queries are unchanged.

The prototype compiled with `g++ -O2 -std=c++17` and matched every
`road2d_dev` answer byte-for-byte. On dev it built 1,503,365 shortcuts and took
18.52 s / 52 MiB RSS, versus the current dynamically prioritized CH at roughly
1.04 s on the same dev tier. This produces substantially more fill and does not
justify a large run. Reject the fixed spatial order; production `solver.cpp`
remains at the certified SHA256 `e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`.

Reproduction: `g++ -O2 -std=c++17 research/prototypes/road_spatial_ch.cpp -o
/tmp/road_spatial_ch`, then `/usr/bin/time -v /tmp/road_spatial_ch
instances/road2d_dev.graph instances/road2d_dev.queries
/tmp/road_spatial_dev.out`; compare with `instances/road2d_dev.answers`.


### Local2d landmark-route incumbent screen (2026-09-26)

Hypothesis: the existing exact spatial landmark tables give a feasible route
upper bound `min_l(d(l,s)+d(l,t))`; using it as an initial incumbent can prune
A* states whose admissible `g+h` cannot beat that route. The isolated
`research/prototypes/local_grid_landmark_ub.cpp` uses the same four per-query
landmarks already selected by production, so no additional preprocessing or
landmark tables are required. It stops when the minimum consistent A* key
reaches the incumbent and returns that feasible path length.

The C++17 candidate matched all 10,000 `local2d_dev` answers byte-for-byte, but
took 1.03 s / 113 MiB including landmark preprocessing, versus about 0.94 s
for the retained path. The upper bound did not reduce total work enough to pay
for computing and checking it; reject before fresh/large testing. `solver.cpp`
remains unchanged at the formally certified 79.11815862212096x source.

Reproduction: `g++ -O2 -std=c++17 research/prototypes/local_grid_landmark_ub.cpp
-o /tmp/local_grid_landmark_ub`, followed by `/usr/bin/time -v
/tmp/local_grid_landmark_ub instances/local2d_dev.graph
instances/local2d_dev.queries /tmp/local_grid_ub_dev.out`; compare with
`instances/local2d_dev.answers`.


### Wide64 contraction-hierarchy screen (2026-09-26)

Hypothesis: the verified 2-D torus may admit a reusable hierarchy that answers
rank-based point-to-point queries after a one-time contraction cost. The
existing exact witness CH construction was enabled only for high-degree
directed road graphs; this temporary branch tests its unmodified witness,
stall, and query logic on the undirected high-weight torus after exact edge
sparsification.

`/tmp/wide64_ch.cpp` compiled under `g++ -O2 -std=c++17` and matched all
wide64_dev answers. It built 385,979 shortcuts on 50,176 vertices, taking
2.20s / 32MiB end-to-end versus ~0.37s / 23MiB for the retained 24-landmark
forward ALT. The hierarchy cost already overwhelms the whole dev candidate;
scaling preprocessing to the 2,000,000-vertex large graph cannot be amortized
by only 30,000 queries. Reject before fresh/large testing. No production
change. Formal Overall remains 79.11815862212096x.

Reproduction: `/usr/bin/time -v /tmp/wide64_ch instances/wide64_dev.graph
instances/wide64_dev.queries /tmp/wide64_ch_dev.out`; byte-compare with
`instances/wide64_dev.answers`.


### Local2d separator-ordered pruned hub labels (2026-09-26)

Hypothesis: exact pruned hub labeling might replace 300,000 individual ALT
searches with small label intersections. `research/prototypes/local_grid_pll.cpp`
orders verified torus vertices by a recursive coordinate separator hierarchy,
then performs undirected pruned landmark labeling. For each root, bounded labels
from earlier roots prune settled vertices when their existing label intersection
already gives a route no longer than the settled distance. Labels use 32-bit
hub IDs and distances under the verified `(V-1)*max_weight < 2^32` bound.

The prototype was compiled with `g++ -O2 -std=c++17` and run on
`local2d_dev`. Construction had accumulated 32.2 million labels after about
17,000 of 50,176 roots, used 389 MiB RSS, and took 33.05 s before the
30-million-label guard abandoned the index and used the retained exact fallback.
The resulting fallback answers matched all 10,000 dev answers, but the label
index itself did not complete. Extrapolating this already-large partial label
set to the 300,304-node tier is incompatible with the 4 GiB cap and costs far
more than the 15.275 s full-large local path, so the labeling architecture is
rejected without fresh-seed or large testing. Production `solver.cpp` remains
unchanged at the e052 checkpoint.

Reproduction: `g++ -O2 -std=c++17 research/prototypes/local_grid_pll.cpp -o
/tmp/local_grid_pll`; `/usr/bin/time -v /tmp/local_grid_pll
instances/local2d_dev.graph instances/local2d_dev.queries
/tmp/local_grid_pll_dev.out`; byte-compare with
`instances/local2d_dev.answers`.


### Local2d region-restricted separator distance oracle (2026-09-26)

Motivated by exact planar distance-oracle tradeoffs, this prototype stores
32-bit distances from each recursive grid separator hub only to vertices in
that hub's region, rather than allocating a full-graph table for every hub. It
uses a verified row-major torus topology, picks a one-side vertex cover for
each region's crossing edges, and stores region memberships by hierarchy level.
For a query, it takes the minimum route through separator hubs shared by the
endpoints' ancestor regions. Same-leaf and unreachable cases use exact
bidirectional Dijkstra fallback. This construction is distinct from the prior
full-graph recursive separator labels and from the explicit/implicit DDG
experiments.

The initial dev comparison exposed a correctness bug: same-leaf queries had
returned the best separator route, which is only an upper bound when the path
can stay inside the leaf. The prototype was fixed to use exact fallback for
same-leaf pairs. The corrected source
`research/prototypes/local_grid_region_oracle.cpp` compiled with
`g++ -O2 -std=c++17` and matched all 10,000 local2d_dev answers. It built
43,743,861 uint32 entries (166.9 MiB) in 3.15 s and took 3.47 s / 190.2 MiB
total; 3,799 queries used fallback and 6,201 were answered by the separator
index. The retained local2d_dev path is about 0.94 s total.

The exact large topology has 718,303,958 region-hub distance entries at the
same 256-vertex leaf cutoff (2.676 GiB as uint32 values, before graph and query
state). Scaling the measured build cost by the 16.4x entry-count ratio and the
query phase by the 30x batch-size ratio projects around 60 s total, versus the
formal 15.275 s local2d_large path. This is not a measured large runtime; it is
a conservative rejection estimate supported by the dev regression and table
count. Do not promote or large-test this candidate.

Reproduction: compile as above; run `/usr/bin/time -v
/tmp/local_grid_region_oracle instances/local2d_dev.graph
instances/local2d_dev.queries /tmp/local_grid_oracle_dev.out`; compare with
`instances/local2d_dev.answers`. Production `solver.cpp` remains unchanged at
the certified e052 source.


### Road line-class relaxed-metric ALT on CH (2026-09-26)

Hypothesis: road2d's observable jittered lattice has fast horizontal arterial
rows and fast vertical columns. For each directed row/direction or
column/direction, the prototype replaces every arc weight by the minimum
weight observed on that same line; diagonal arcs use a per-orientation minimum.
Each replacement weight is no greater than the original arc, so the relaxed
graph's exact distances and directed ALT landmarks are admissible and
consistent on original arcs and CH shortcuts. The prototype uses coordinate
rank bins and verifies every edge is a neighboring orthogonal/diagonal arc
before selecting this potential.

`research/prototypes/road_ch_line_relaxed_alt.cpp` compiled with
`g++ -O2 -std=c++17` and matched all road2d_dev answers. It selected 12
landmarks, then ran balanced directed ALT on the unchanged exact CH. Compared
with the instrumented retained query, work fell only from 882,865 pops /
9,526,670 arc scans to 847,943 / 9,238,163 (4.0% / 3.0%); end-to-end dev
time rose from 1.15 s to 1.38 s because relaxed SSSP preprocessing and
potential evaluation outweighed the small pruning. This fails the dev work and
time screens, so it was rejected before fresh-seed/large testing.

Reproduction: `g++ -O2 -std=c++17
research/prototypes/road_ch_line_relaxed_alt.cpp -o
/tmp/road_ch_line_relaxed_alt`; then `/usr/bin/time -v
/tmp/road_ch_line_relaxed_alt instances/road2d_dev.graph
instances/road2d_dev.queries /tmp/road_line_alt_dev.out`; compare with
`instances/road2d_dev.answers`. Baseline counters came from
`research/prototypes/road_ch_base_counter.cpp` on the same dev inputs.
Production `solver.cpp` remains at the formally certified e052 SHA.


### Road target-region conservative CH arc flags (2026-09-26)

Hypothesis: 32x32 coordinate target cells could share a multi-source reverse
Dijkstra profile and safely prune CH arcs that cannot lie on a shortest path to
any target in the cell. The prototype
`research/prototypes/road_ch_region_flags.cpp` computed exact distances to each
target set plus an in-cell directed diameter upper bound, then tested a
conservative inequality during both CH search directions. It compiled under
C++17 and matched all road2d_dev answers.

This certificate cannot prune: for every directed arc `u->v`, distance to the
target set satisfies `d(u,R) <= w(u,v)+d(v,R)` by the triangle inequality.
Adding the nonnegative region-diameter allowance only weakens that inequality,
so the tested exclusion condition is impossible. The dev run confirmed 0 of
4,324,730 scanned CH arcs pruned; profiles took 0.574 s, and total time was
1.72 s / 53.8 MiB versus 1.18 s retained. Reject this region-distance flag
construction before fresh/large testing; do not tune its cell size.

Reproduction: `g++ -O2 -std=c++17 research/prototypes/road_ch_region_flags.cpp
-o /tmp/road_ch_region_flags`; then `/usr/bin/time -v
/tmp/road_ch_region_flags instances/road2d_dev.graph
instances/road2d_dev.queries /tmp/road_region_flags_dev.out`; compare with
`instances/road2d_dev.answers`. The certified `solver.cpp` remains unchanged.


### Wide64 bounded lighter-subgraph edge witnesses (2026-09-26)

Hypothesis: local 2/3/5-hop detours may miss longer replacements for very heavy
wide64 edges. The isolated `research/prototypes/wide64_bounded_witness.cpp`
adds a 128-settlement Dijkstra from each remaining edge endpoint, traversing
only arcs strictly lighter than that edge and stopping beyond its weight. Any
found path is therefore a safe simultaneous-removal witness by induction on
edge weight. The search is limited to structurally verified high-minimum-weight
2-D grids.

It compiled with `g++ -O2 -std=c++17` and matched all wide64_dev answers.
Sparsification still removed exactly 19,574/200,704 arcs (9.753%), the same
count as the existing exact five-edge probe; the longer bounded search found no
additional removable edge. Preprocessing was 0.024 s and total dev time 0.42 s,
with 23.1 MiB RSS, versus the retained 0.42 s run. Since it adds work without
reducing the graph or query work, reject before fresh-seed/large testing.

Reproduction: `g++ -O2 -std=c++17 research/prototypes/wide64_bounded_witness.cpp
-o /tmp/wide64_bounded_witness`; then `/usr/bin/time -v
/tmp/wide64_bounded_witness instances/wide64_dev.graph
instances/wide64_dev.queries /tmp/wide64_bounded_dev.out`; compare with
`instances/wide64_dev.answers`. Production solver remains unchanged.


### Road CH target-region potential screen (2026-09-26)

This run recovered the prior status-127 failure from `/tmp/road_region_alt_dev.time`: `/usr/bin/time` reported that `/tmp/road_ch_region_alt` did not exist, then returned 127. The prototype source still referenced the already-removed `road_region_arc_may_be_shortest` helper from its legacy `ch_query`, so the preceding compile had failed and the shell continued to the timed execution. This was a missing-binary consequence, not a Python interpreter failure. The repaired command now builds a temporary source under `/tmp` and runs only after `g++` succeeds; Python invocations use the available `python3` executable. The repository `solver.cpp` and `research/checkpoints/certified-integrated-e0527341.cpp` still have identical SHA256 `e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`.

The prototype tests a shared exact distance-to-target-region potential on top of road CH. The first reduced-key termination `minF+minB >= best-h(s)` returned 2,373 wrong answers on road2d_dev even with stall disabled; turning off that early stop restored exactness. The profile had `h(t)=0` and satisfied the tested CH-edge consistency inequalities, so this sum-stop version is rejected and was never used for a fresh or large run. A more conservative check requiring both reduced minima individually to reach `best-h(s)` matched all road2d_dev answers and a fresh road2d_dev generated with seed 20260927 / salt `road-region-alt-repair`.

That exact conservative variant did not pay end-to-end. It took 1.20s / 52.6MiB on road2d_dev versus 1.08s / 33.6MiB for the certified solver; on road2d_large it built 324 target-region profiles (742.3MiB tables) in 8.783s, then completed in 26.99s / 951.5MiB and matched all 200,000 official answers. The official solver took 22.046s / 208MiB. CH query scans fell to 162.49M, but region preprocessing and reduced-key work erased the search savings. Reject and keep production at the e052 checkpoint.

Reproduction used the temporary repaired copy of `research/prototypes/road_ch_region_alt.cpp` at `/tmp/road_ch_region_alt_fixed.cpp`; its only source repair removes the stale arc-flag predicate from baseline `ch_query`. The exact large run was `/usr/bin/time -v /tmp/road_ch_region_alt_conservative instances/road2d_large.graph instances/road2d_large.queries /tmp/road_region_alt_conservative_large.out`, then byte-compared with `instances/road2d_large.answers`.


### Lattice3d full separator-table feasibility screen (2026-09-26)

A recursive exact 3-D separator oracle was screened before implementation. The authoritative lattice3d_large header is 300,763 vertices (side 67), not the 32k scale used in an initial rough estimate. A torus median cut crosses two 67x67 planes, yielding about 8,978 boundary endpoints; one full-graph uint32 distance row per endpoint would require roughly 10.0 GiB (the exact table estimate is 8,978 * 300,763 * 4 bytes, about 10.0 GiB / 10.7 GB), before graph state. This already exceeds the enforced 4 GiB cap at the root level, and root-level per-hub SSSP preprocessing would also be prohibitive. Reject this full separator-table architecture before coding; no solver or candidate source changed.


### Local2d target-local-anchor balanced bidirectional ALT (2026-09-26)

The current local2d path uses four target-near members of the retained 256 exact grid-landmark tables in one-sided A*. This prototype selected the same four tables, formed the exact doubled feasible potential `p(v)=h_t(v)-h_s(v)`, and searched both directions with the exact reduced-key termination `minF+minB >= 2*best+p(t)-p(s)`. It compiled from the certified `solver.cpp` as `/tmp/local_grid_bialt_anchors.cpp`, matched all 10,000 local2d_dev answers, and reduced query work to 877,130 settled nodes / 3.19M scans. The certified one-sided path took 0.89s end-to-end; this variant took 0.91s with the same 0.80s landmark preprocessing and 113MiB RSS. The 18% query-work reduction is erased by two-frontier overhead, so reject before fresh/large testing. Production remains at e052 SHA.


### Local2d target-strip relaxed-metric ALT screen (2026-09-26)

A new exact lower bound was tested for local-rank queries. For each active target x/y strip of width 8, the prototype builds a multi-source SSSP in a relaxed torus metric: retain actual weights along one axis and replace orthogonal edges by the graph-wide minimum weight. Distances to the target strip are admissible and consistent; the maximum of x-strip, y-strip, and the existing four-anchor ALT bounds is also admissible and consistent. On local2d_dev, 28 x-strips and 28 y-strips added 21.4MiB and 0.139s preprocessing. The candidate compiled from certified `solver.cpp`, matched all dev answers, but search work was unchanged at 1,061,535 pops / 3.828M scans; total time rose to 1.02s versus 0.89s certified. The regional relaxation is dominated by the existing anchor ALT, so reject without fresh/large testing. Production remains unchanged at e052.


### Local2d exact target-strip distance oracle screen (2026-09-26)

A second strip variant replaced the 256 single-vertex landmark SSSPs with exact multi-source distances to active width-8 target columns and rows in the original graph. The query heuristic is `max(dist(v,target-column), dist(v,target-row))`, which is a consistent lower bound for every target in that column/row; it avoids per-query landmark selection and cuts the table build from 256 SSSPs to 56 on dev / 138 on large. This prototype matched all 10,000 local2d_dev answers and reduced total dev time from 0.89s to 0.55s, with 21.4MiB tables and 0.167s preprocessing. However, query search grew from the retained 1.06M pops / 3.83M arcs to 5.53M / 20.24M; the measured dev query phase itself rose from roughly 0.09s to 0.38s. The apparent total win comes from the smaller dev preprocessing workload and does not scale to 300,000 queries; reject before large testing. Production remains e052.


### Local2d tile-feasible-upper-bound screen (2026-09-26)

Hypothesis: for same-32x32-tile query pairs, an exact path restricted to that tile is a feasible global upper bound; if it equals the existing global ALT lower bound, the query can be returned immediately, otherwise the incumbent can prune the global A* search. `/tmp/local_grid_tile_bound.cpp` was built from the e052 source and compiled with `g++ -O2 -std=c++17`. It matched all 10,000 official local2d_dev answers (`cmp` against `instances/local2d_dev.answers`). It attempted 5,151 same-tile queries and certified 1,965 (19.65% of all queries), while the restricted searches settled 921,108 nodes and scanned 3,362,068 arcs. End-to-end dev time was 1.06s / 108MiB versus 0.94s / 108MiB for the freshly compiled e052 source; the 256-table preprocessing was effectively unchanged (0.845s vs 0.831s). The restricted work costs more than the returned-query savings, so reject before fresh/large testing. No production source changed; the formal source remains SHA256 `e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`.


### Local2d repeated-target grouped Dijkstra screen (2026-09-26)

Hypothesis: the large local2d batch has 190,709 queries whose target occurs at least twice (79,344 target groups), so one reverse Dijkstra per repeated target might share work across nearby local sources. `/tmp/local_target_group.cpp` was built from the e052 source, changing only the local grid class to group target multiplicity >=2 and count grouped search work; it compiled with `g++ -O2 -std=c++17` and matched all 10,000 local2d_dev answers byte-for-byte. Dev has only 1,909 queries in repeated-target groups, and those grouped searches alone settled 4,994,984 nodes / scanned 18,366,690 arcs. End-to-end was 1.26s / 108MiB versus 0.94s / 108MiB for e052. This is already several times the total retained dev search work for a subset of queries, so the large endpoint repetition does not make the mechanism credible; reject before large/fresh testing. No production source changed; formal Overall remains 79.11815862212096x.


### Local2d block-cut relaxed ALT screen (2026-09-26)

Hypothesis: collapse each verified 2-D torus block to a coarse vertex, give each coarse transition the minimum retained crossing-edge weight, and precompute exact coarse distances to each target block. The resulting potential is consistent and admissible on the sparsified graph; it could provide a stronger spatial bound for longer local queries without additional full-graph SSSPs. `/tmp/local_coarse_tile_alt.cpp` compiled with `g++ -O2 -std=c++17` and matched all local2d_dev answers. With 8x8 blocks, 784 coarse vertices and 23,010 directed crossing arcs were built in 0.066s; RSS was 115MiB. Across 9,990 non-self queries, the coarse bound exceeded existing four-anchor ALT at the source zero times, and total A* work remained 1,062,244 pops / 3,830,190 scans versus about 1.062M / 3.828M retained. End-to-end was 1.01s versus 0.94s. The relaxed metric is dominated by ALT and does not prune; reject without large/fresh testing. No production source changed; formal Overall remains 79.11815862212096x.


### Wide64 median-clipped ALT screen (2026-09-26)

Hypothesis: replacing each original edge weight by `min(w, median_edge_weight)` defines a lower-weight graph; its exact ALT distances are consistent lower bounds on original edges and might complement the retained 24-landmark heuristic. The temporary `/tmp/wide64_clamped_alt.cpp` preserved the original graph for the relaxed SSSPs, selected the median from observed edge weights, and reused the same 24 root vertices as the retained exact landmarks. It compiled under C++17 and matched all wide64_dev answers byte-for-byte. The observed cap was 313,668,693; 24 additional tables took 0.089s on dev. The clipped bound raised the source heuristic for only 135/9,990 non-self queries. Work fell from the retained 3.379M pops / 12.228M scans to 3.264M / 11.776M, but end-to-end rose from 0.37s / 23MiB to 0.53s / 36MiB. The search reduction is too small to pay for extra preprocessing, table memory, and bound evaluation; reject before fresh/large testing. No production change; formal Overall remains 79.11815862212096x.


### Lattice3d one-sided ALT4 screen (2026-09-26)

Hypothesis: the retained 128-landmark work-balanced biALT may pay unnecessary two-frontier/radix-heap overhead on a six-regular graph; one-sided A* with the same strongest-four landmark potential could trade fewer heap operations for more pruning work. `/tmp/lattice_forward4.cpp` changed only the undirected lattice query engine and added counters; it compiled with `g++ -O2 -std=c++17` and matched all lattice3d_dev answers byte-for-byte. It settled 15.22M states and scanned 51.66M arcs, taking 1.83s / 65MiB; a freshly compiled e052 source took 1.30s / 65MiB. The retained work-balanced bidirectional path is substantially better, so reject before fresh/large testing. No production change; formal Overall remains 79.11815862212096x.


### Road corridor-level CH ordering (2026-09-26; integrated targeted candidate)

The candidate `/tmp/road_corridor_hierarchy.cpp` changes only the road CH
contraction order. Structural recognition checks the directed coordinate grid
and its neighboring-edge topology, estimates normalized row/column medians,
and delays contraction of verified faster arterial/highway lines. CH witness
search, stall, query termination, and exact fallback are unchanged. It compiled
with `g++ -O2 -std=c++17`.

On road2d_dev it recognized 56 faster lines (8 highway), generated 224,335 CH
shortcuts versus 322,816 for the checkpoint, and retained the same 19.812%
exact sparsification. It matched every official dev answer. A fresh road2d_dev
seed (20260928, salt `road-hierarchy`) also matched `refsolve` byte-for-byte;
its shortcut count was 224,779. A candidate all-six dev grade completed with
all six workloads `ok`. The candidate's road2d_dev run was 1.021s; the grade
reported all-six dev timings of 1.350s lattice3d, 0.963s local2d, 0.448s
scalefree, 1.019s hugeq, and 0.379s wide64.

Two prototype road2d_large runs matched all 200,000 official answers at
17.83s / 202,892 KiB and 17.86s / 202,956 KiB. The feature was integrated into
`solver.cpp`, compiled with `g++ -O2 -std=c++17`, and confirmed byte-identical
to the prototype source. Its integrated road2d_large rerun also matched all
200,000 answers at 18.36s / 202,888 KiB, with 1,940,888 CH shortcuts. Against
the formal e052 checkpoint's 22.046s / 218MiB, this is a stable ~1.20x road
gain and slightly lower memory. Reusing the other five formal rows projects
to about 81.6x Overall; this is not a grade result, and no full six-large grade
was run for this single-row change. The rollback source remains
`research/checkpoints/certified-integrated-e0527341.cpp`, SHA256
`e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`, with
matching formal result `result-integrated-e052-full.json` (Overall
79.11815862212096x). The working candidate SHA is
`af27699a5416968ba9a4d840e87822c01eaa27b170822568f2ac82a5af4943d3`.

Reproduction: compile the temporary source above; compare its road2d_dev and
road2d_large outputs with the corresponding `instances/*.answers`; fresh seed
generation used `python3 -m tools.gen --manifest tools/manifest/public.json
--instance road2d_dev --seed 20260928 --salt road-hierarchy --out
/tmp/road-hierarchy-fresh`, followed by `tools/ref/refsolve plan` and bytewise
comparison. The all-six dev grade used
`python3 -u grade.py --solver /tmp/road_corridor_hierarchy --instances
instances_dev.txt --source /tmp/road_corridor_hierarchy.cpp --json
/tmp/road-corridor-dev.json`.

The all-six dev JSON is exact and its source hash matches the integrated
candidate. A redundant second all-six dev-grade attempt on the integrated
source was manually interrupted after 130 seconds without producing JSON;
the completed identical-source dev grade remains the validation record.


### Wide64 ALT landmark-route incumbent screen (2026-09-26)

Hypothesis: the exact distances to each of the 24 existing landmarks give a
feasible source-to-landmark-to-target route upper bound. If that bound is
already below the minimum open A* lower-bound key, a query can return it
without settling the target. This could help if many rank queries have a
near-optimal route through an existing landmark, without adding preprocessing
or tables.

`/tmp/wide64_landmark_upper.cpp` was generated from the current integrated
source and compiled with `g++ -O2 -std=c++17`. The added termination is exact:
the fixed four-landmark ALT potential remains consistent, and every landmark
route is a feasible path. It matched all wide64_dev answers byte-for-byte, but
the route bound certified only 78/9,990 non-self queries (0.78%). It popped
4.02M heap entries and took 0.42s / 22.4MiB, versus the retained ~0.37s /
23MiB. The bound is too weak to reduce search and adds overhead; reject before
fresh/large testing. The formal Overall remains 79.11815862212096x; substituting
the road candidate projects to about 81.6x only, not a grade result.

Reproduction: `g++ -O2 -std=c++17 /tmp/wide64_landmark_upper.cpp -o
/tmp/wide64_landmark_upper`; `/usr/bin/time -v /tmp/wide64_landmark_upper
instances/wide64_dev.graph instances/wide64_dev.queries
/tmp/wide64_landmark_upper_dev.out`; byte-compare with
`instances/wide64_dev.answers`.


### Local2d reverse-oriented grid ALT screen (2026-09-26)

Hypothesis: the local workload's 90% rank queries are oriented from a uniform
source toward a rank-selected target. Reversing one-sided A* and choosing the
four regular-grid landmarks nearest the reversed destination (the original
source) might better focus the short-query potential. This remains exact on
the undirected graph because the four absolute landmark differences are a
consistent lower bound.

`/tmp/local_grid_reverse.cpp` changes only the local-grid query call to swap
source and target, compiled with `g++ -O2 -std=c++17`, and byte-matched all
local2d_dev answers. End-to-end was 0.99s / 113MiB including 0.883s landmark
preprocessing, versus roughly 0.94s / 108MiB for the retained orientation.
There is no dev evidence of a gain; reject before fresh/large testing. No
production change. Formal Overall remains 79.11815862212096x.

Reproduction: `/usr/bin/time -v /tmp/local_grid_reverse
instances/local2d_dev.graph instances/local2d_dev.queries
/tmp/local_grid_reverse_dev.out`; byte-compare with
`instances/local2d_dev.answers`.


### Wide64 reverse-oriented ALT4 screen (2026-09-26)

Hypothesis: although wide64 queries are generated by forward Dijkstra rank,
searching from target to source on this undirected graph may expand fewer
states due to endpoint and tie-breaking asymmetry. The fixed four-landmark
absolute-difference potential is symmetric and remains consistent after
reversing endpoints, so this is exact without additional memory or tables.

`/tmp/wide64_reverse_alt4.cpp` swaps the A* root and goal and selects the same
strongest-four landmark subset. It compiled with `g++ -O2 -std=c++17` and
byte-matched all wide64_dev answers. Runtime was 0.40s / 22.4MiB versus about
0.37s / 23MiB for the retained forward direction. The dev result gives no
meaningful evidence of an orientation advantage; reject before fresh or large
testing. No production change. Formal Overall remains 79.11815862212096x.

Reproduction: `g++ -O2 -std=c++17 /tmp/wide64_reverse_alt4.cpp -o
/tmp/wide64_reverse_alt4`; `/usr/bin/time -v /tmp/wide64_reverse_alt4
instances/wide64_dev.graph instances/wide64_dev.queries
/tmp/wide64_reverse_alt4_dev.out`; byte-compare with
`instances/wide64_dev.answers`.


### Wide64 coordinate-monotone route upper-bound screen (2026-09-26)

Hypothesis: for each torus query, the shortest of the eight coordinate-monotone
routes (two axis orders and four wrap-direction combinations) is a cheap
feasible incumbent. Prefix sums are built from original edge weights before
exact sparsification; even if a route uses an edge later removed, the
sparsifier preserves or improves the global distance. Combining this with
the 24 landmark-route incumbents might allow exact ALT to stop before settling
the target.

`/tmp/wide64_torus_route_ub.cpp` compiled with `g++ -O2 -std=c++17` and
matched all wide64_dev answers byte-for-byte. It certified 118/9,990 non-self
queries (1.18%) and popped 4.016M heap entries. End-to-end was 0.42s /
23.6MiB versus ~0.37s / 23MiB retained. The feasible coordinate routes are
rarely tight enough to certify; reject before fresh/large testing. No
production change. Formal Overall remains 79.11815862212096x.

Reproduction: `/usr/bin/time -v /tmp/wide64_torus_route_ub
instances/wide64_dev.graph instances/wide64_dev.queries
/tmp/wide64_torus_route_ub_dev.out`; byte-compare with
`instances/wide64_dev.answers`.

### Additional regional ALT screens (2026-09-26)

Exact partial regional landmark balls did not show a large-tier win. On
wide64, 64 full regional SSSPs took 16.995s on the 2M-vertex graph and raised
total from the 29.692s formal row to 34.70s / 1.65GB despite cutting query
work from 213.08M to 104.19M pops. A 64-table radius-capped variant settled
only 2.10M vertices in preprocessing and remained exact on dev, a fresh seed,
and wide64_large, but total was 33.66s / 1.65GB. The preprocessing is no
longer prohibitive, yet the regional potential costs too much per search and
does not beat global ALT.

The same exact capped-ball construction on lattice3d used 4x4x4 regions and
32,768 settled vertices per region. It was exact on lattice3d_dev and reduced
search to 4.67M pops / 16.24M scans, but total dev time was 1.35s versus 1.27s
retained, with RSS rising from 65MiB to 90MiB. Both are rejected without
integration; `solver.cpp` remains the road-corridor candidate SHA256
`af27699a5416968ba9a4d840e87822c01eaa27b170822568f2ac82a5af4943d3`.

### 2026-09-26 follow-up candidate screens

The local2d route-incumbent probe used the best of the eight coordinate-monotone
paths on the verified torus, with original edge weights retained before exact
sparsification. `/tmp/local_route_ub.cpp` matched local2d_dev exactly and took
0.89s / 111MiB, indistinguishable from the retained ~0.89s. Its first compile
failed on duplicate local coordinate declarations; the shell then attempted to
run the missing binary and returned 127. The source was corrected and all
subsequent compile/run commands were chained with compile success required.
The route bound was rejected.

Per-query memoization for the local2d four-anchor heuristic had only 1.20x
reuse: 1,984,930 calls but 1,651,295 cache misses on local2d_dev. Exact dev
time was 0.92s / 111MiB versus 0.89s retained, so the cache did not repay its
lookup and stamp cost. A short-A* / long-biALT hybrid (biALT only when torus
Manhattan displacement exceeds 96) was exact on local2d_dev and local2d_large,
but large took 15.70s / 674MiB versus 14.34s / 672MiB for a fresh working
solver run; reject the hybrid.

A 32-bit local landmark table is now integrated in `solver.cpp`. The runtime
checks `(V-1)*max_edge < UINT32_MAX` and also rejects any non-finite or
out-of-range SSSP value, so the table is used only when every stored distance
is proven representable; otherwise the solver takes its exact fallback. It
matched all six official dev sets, a fresh local2d_dev seed (20260930,
`local-u32`), and all local2d_large answers. The current integrated targeted
large run was 13.44s / 379MiB; the 64-bit table version measured 14.34s /
672MiB on the same runner. This is a stable 6% speedup with 44% lower RSS, not
an Overall certification. Working source SHA256 is recorded after the most
recent integration.

A corresponding lattice3d 32-bit landmark variant used an exact root eccentricity certificate: if all vertices are reachable from the first
landmark and twice its maximum distance is below UINT32_MAX, every pairwise
shortest distance fits. It was exact, but lattice3d_large took 20.69s /
203MiB versus 16.18s / 353MiB from the current 64-bit path. Reject the time
regression despite halved table memory. A cheaper sum of per-axis minimum-edge
cycle bounds was much weaker than existing ALT on lattice3d_dev: it exceeded
the global landmark bound on 0/10,000 non-self queries (mean 27.59 versus
759.15), so reject without integration.

The formal checkpoint remains e0527341 / 79.11815862212096x. No new six-large
grade has been run. The current working candidate combines the road corridor
ordering and local2d uint32 tables; its row-wise projection is approximately
83.4x, not a measured grade.


### hugeq singleton restricted-search screen (2026-09-26)

The current separator path leaves 17,332 singleton target/leaf groups and
8,580 multi-source groups. I tested replacing just singleton reverse grouped
ALT with exact bidirectional directed ALT using the consistent target
potential `cached_alt_h` and doubled reduced-key min-key cutoff. It matched all
600,000 hugeq_large answers, but took 11.24s / 1.21GB versus 9.23s / 1.21GB
for the current path. The extra frontier and potential work outweighed pruning;
reject without fresh-seed testing. Prototype: `/tmp/hugeq_singleton_bialt.cpp`.


### Scale-free grouped-target regression removal (2026-09-26)

Profiling the retained exact path on scalefree_large found that 303 reverse
target batches covered 15,027 queries but consumed 126.35M heap pops and
254.71M scanned arcs. The 34,923 work-balanced bidirectional point searches
consumed 59.29M pops and 161.03M scans. Testing the same exact solver with
repeated-target batching disabled cut full large time from 15.62s to 9.20s; all
50,000 answers matched. The integrated source uses the observable structural
condition `graph_directed && !balance_by_arc_work` to keep target grouping on
ordinary directed graphs while routing reverse-degree-skew graphs through
the work-balanced point path. It passed all six official dev sets, a fresh
scalefree_dev seed (20260930, `scale-no-target-batch`), and an integrated
scalefree_large run at 9.43s / 40MiB with all answers exact. Current source
SHA256 is `4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`.
Against the formal 15.999s scale row this is about 1.70x targeted.

A 24-landmark regular 6x4 spatial placement was exact on wide64_dev but
settled 5.23M vertices / scanned 15.22M arcs and took 0.46s, versus roughly
3.38M / 12.23M and 0.38s for farthest-point landmarks. Reject before fresh or
large testing; no production change.

Holding the other official rows fixed, combining the road corridor 18.36s,
local2d uint32 13.44s, and scalefree no-batch 9.43s targeted runs projects
about 91.0x Overall from the 79.118x formal checkpoint. This is a row-wise
projection only, not a grade result; no six-large grade has been run.

### Further fail-fast architecture screens (2026-09-26)

The previously diagnosed exit-127 was reproduced from
`/tmp/road_region_alt_dev.time`: compilation had failed because a temporary
road prototype still called the removed `road_region_arc_may_be_shortest`,
then `/usr/bin/time` tried to start the absent binary. Python itself was not
the failing executable. All candidate runs below were gated on successful
compilation and answer comparisons.

I tested contracting reverse out-degree-one runs in the exact work-balanced
scale-free search. The relaxation eagerly propagated exact labels through
each deterministic run, checked all internal vertices for a forward meeting,
and queued only the first branching node; omitted internal minima are safe for
the stopping bound because every outgoing arc is propagated immediately and
any improving external entry remains behind an unprocessed frontier key. The
candidate was exact on scalefree_dev and scalefree_large, but large took
13.35s / 40MiB versus 8.36s on a same-session baseline repeat. The graph has
147,929 reverse out-degree-one vertices but 129,013 runs, with median run
length one and only 12,537 runs of length at least two. Heap savings do not
offset chain-label handling; reject. Temporary source:
`/tmp/scalefree_chain_contracted.cpp`.

A second scale-free candidate precomputed one forward/reverse landmark pair at
the maximum-in-degree hub, used its feasible route as an incumbent, and
pruned reverse relaxations whose directed ALT lower bound could not beat that
incumbent. The proof is per arc: `LB(s,v)+w(v,u)+d_b(u) >= best` certifies that
this route cannot improve the already feasible answer. It matched both
official scale-free answer files and pruned 15.99M of 25.10M high-degree
reverse arcs on large, but took 9.10s versus 8.36s for the same-session
baseline; dev was 0.38s versus 0.34s. The scans still read each candidate arc
and add landmark lookups, so the avoided heap work did not repay its cost.
Reject; no production source change. Prototype:
`/tmp/scalefree_hub_arc_prune.cpp`.

I tested capped local-grid landmark SSSPs for local2d: distances below radius
100,000 are exact and all larger values are clamped to the cap. Clamping a
metric distance preserves its edge Lipschitz bound, so absolute landmark
differences remain consistent and admissible. Long-displacement queries used
24 full global landmarks. The candidate was exact on local2d_dev and
local2d_large, but only settled 68.68M vertices across the 256 local tables
instead of the 76.88M table entries in the full-table baseline (11% less
preprocessing work); preprocessing fell only from 4.92s to 4.45s while total
large time regressed from 13.44s to 28.77s and RSS rose from 388MiB to 451MiB.
Reject. Prototype: `/tmp/local_grid_capped.cpp`.

Finally, I composed the existing four-anchor local-grid potential with
consistent bidirectional A* and the doubled reduced-key min-key termination.
It matched the official local2d_dev, a fresh local2d_dev generated with seed
20260930 / salt `local-bialt`, and all 300,000 local2d_large answers. Large
time was 13.66s / 389MiB versus 13.44s / 388MiB. Profiling explains the lack
of gain: settled heap pops changed from 116.59M to 115.32M and scans from
370.23M to 369.39M, about a 1% reduction, while every expanded vertex now
evaluates both endpoint potentials. Reject. Prototype:
`/tmp/local_grid_bialt.cpp`.

These candidates did not alter `solver.cpp`; the current working source SHA256
remains `4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`.
The rollback checkpoint and its same-run result remain intact at e0527341 / 
79.11815862212096x. The current working combination has only a row-wise
projection near 91x; it has not received a formal six-large run.

### Formal combined certification (2026-09-26)

The combined road-corridor CH ordering, local2d uint32 landmark tables, and
scale-free no-target-batching dispatch were rebuilt from `solver.cpp` and
validated on all six official dev answer files. The official command
`python3 -u grade.py --solver ./solver --instances instances.txt --json
result-integrated-4610587-full.json` completed with every large row `ok` and
exact. This is the first full six-large grade for source SHA256
`4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`.

| Workload | T_base | T_solver | Speedup | Peak RSS | Exact |
|---|---:|---:|---:|---:|:---:|
| road2d_large | 945.350s | 16.719s | 56.544x | 198 MiB | yes |
| lattice3d_large | 1637.251s | 15.964s | 102.559x | 353 MiB | yes |
| local2d_large | 620.957s | 13.124s | 47.315x | 379 MiB | yes |
| scalefree_large | 1755.288s | 8.835s | 198.665x | 41 MiB | yes |
| hugeq_large | 2029.355s | 9.098s | 223.065x | 1184 MiB | yes |
| wide64_large | 1122.656s | 28.701s | 39.116x | 632 MiB | yes |

Global geomean is 126.613x, Local geomean 43.020x, and official Overall
88.35080960833166x. The result JSON records solver binary SHA256
`2582aed37dab141b5d52d618ca4a14879c2d8747e7579c7b30a63bce9ad5401d` and
source SHA256 above; both were independently rechecked after the run. The
result file SHA256 is
`fe329dbec22b822e62dc24d2843bcb549c3c843000638782cf6739375948b972`.
Preserved source copy: `research/checkpoints/certified-integrated-4610587.cpp`.
The older e0527341 checkpoint remains available as a rollback ancestor.

The 150x target remains unmet. The formally weakest rows are wide64 (39.116x),
local2d (47.315x), and road2d (56.544x); further research should prioritize a
multi-fold query-work reduction in one of these paths rather than certify an
unchanged candidate again.

### Wide64 landmark-pair ALT screen (2026-09-27)

Hypothesis: with four selected landmarks `L_i`, triangle inequalities between
landmark pairs give the additional admissible potential
`max_i(max_j(d(L_i,L_j)-d(L_j,t))-d(L_i,v), 0)`. Each term is a constant minus
one exact landmark distance, hence 1-Lipschitz; its maximum with ordinary ALT
remains consistent. The query setup calculates the 4x4 landmark metric, and
the per-node work reads the same four active tables as the retained ALT4 path.

The temporary `/tmp/wide64_landmark_pair.cpp` compiled and matched all
wide64_dev answers. It settled 4.024M heap entries and scanned 12.192M arcs,
versus 3.379M / 12.228M for the retained path; dev time was 0.36s versus about
0.37s, within noise, while heap pops rose 19%. There is no evidence of search
reduction, so reject before fresh-seed or large testing. `solver.cpp` remains
at certified SHA256 `4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`,
with official Overall 88.35080960833166x; no production branch changed.

### Retained-path follow-up screens (2026-09-27)

The temporary `/tmp/local_grid_top256.cpp` selected the strongest four
landmarks from all 256 local-grid tables for every local2d query, replacing
target-near grid anchors on short displacements. It compiled and matched all
10,000 local2d_dev answers. A same-session recheck measured 0.93s / 61.3MiB
and 1,086,983 pops / 3,726,356 scans versus 0.87s / 61.5MiB for the
certified target-near path. Reject; no large run or production change.

Two exact wide64 lower-bound replacements were screened on `wide64_dev`.
`/tmp/wide64_axis_relax.cpp` used the sum of weighted-cycle distances formed
from the minimum retained edge crossing each x/y coordinate cut, eliminating
full landmark preprocessing. Its consistency follows because an original
edge crosses one cut and costs at least that cut's minimum. It matched all
dev answers, but expanded 28.20M nodes / 101.95M arcs and took 2.25s versus
about 3.38M / 12.23M and 0.37s for the retained 24-landmark ALT4 path.
`/tmp/wide64_coarse_relax.cpp` collapsed 2x2 torus cells and used 24 exact
landmarks in the relaxed graph whose inter-cell edge weight is the minimum
retained crossing-edge weight. Projection of every retained path proves the
coarse distances are admissible and consistent. It also matched all dev
answers, but took 3.41s / 15.3MiB, with 40.73M pops / 147.27M scans (0.024s
coarse preprocessing), so reject. The larger search mechanism is not offset
by smaller tables. These are scratch-only candidates; `solver.cpp` remains
the certified SHA above.

The additional global-24 table for local2d long-displacement queries was also
rejected. `/tmp/local_grid_global_long.cpp` matched all local2d_dev answers,
but the 625 queries routed to the long branch took 1.368M pops / 5.010M scans
versus 0.471M / 1.713M with the retained top-4 among 256 local anchors. Its
extra global SSSPs cost 0.076s; total dev was 1.03s versus 0.88s. Do not run
large. The query-target-cell multi-source profile in
`/tmp/local_targetset_profiles.cpp` was exact too, but the single target-cell
potential expanded 3.881M pops / 14.160M scans and total dev was 1.03s versus
0.88s retained. It is too weak despite one table lookup per node. No candidate
changed production; the official 88.35080960833166x checkpoint remains the
rollback target.

Two endpoint-local anchor variants were screened after those rejects.
Choosing the four strongest anchors among the 4x4 nearest grid candidates
matched local2d_dev and reduced total search work to 980,929 pops / 3,533,064
scans from roughly 1.062M / 3.828M, but end-to-end remained 0.88s versus
0.87s. Separate source-near and target-near potentials in exact bidirectional
ALT were also exact; short-query work was 552,877 pops / 2,006,224 scans and
the full dev run took 0.90s / 63.2MiB. Long queries retained the existing
fallback, so the total gain did not clear the time gate. Reject both without
large testing. Scratch sources: `/tmp/local_grid_short_top16.cpp` and
`/tmp/local_grid_endpoint_bialt.cpp`; production remains unchanged.

The endpoint-specific bidirectional ALT was then scheduled by cumulative
relaxed-arc work instead of min key, preserving the same exact min-key cutoff.
It matched local2d_dev and reduced short-query work from 552,877 / 2,006,224
to 505,509 / 1,836,416 pops/scans (918,208 arcs per direction), but full dev
time was 0.91s versus 0.87s retained. Reject before fresh/large: extra
potential evaluation erases the work reduction. Scratch source:
`/tmp/local_grid_endpoint_workbalanced.cpp`.

Long-displacement local2d queries were also rerouted through balanced
bidirectional ALT over 24 global landmarks, leaving short queries on the
retained local-anchor A*. The prototype matched dev answers, but the 625 long
queries expanded 1.375M pops / 5.033M scans, almost the same as the slower
global one-sided branch; total dev was 1.11s / 71.4MiB after 0.082s global
landmark preprocessing. Reject before large; the endpoint-specific local
path remains untouched.

An exact bounded lighter-subgraph witness was tested on low-weight local2d.
For every arc left by the existing 2/3-edge certificate, the prototype ran a
128-settlement Dijkstra restricted to strictly lighter arcs and paths no
heavier than the candidate edge; weight induction preserves distances under
simultaneous removals. It matched all local2d_dev answers, but found only 352
extra undirected witnesses (0.35 percentage points of directed arcs), after
438,638 settled nodes / 1,753,144 scans. Sparsification rose from 8.009% to
8.360%, preprocessing from 0.005s to 0.027s, and end-to-end from 0.87s to
0.91s. Reject before large. Scratch source: `/tmp/local_grid_long_witness.cpp`.

The same bounded strict-lighter witness was screened on directed road2d.
`/tmp/road_long_witness.cpp` matched every dev answer and found 553 extra
undirected replacements (0.203 percentage points of arcs), after 1.053M
settlements / 5.746M scans. CH shortcuts changed only from 224,335 to 224,162.
Sparsifier time rose from 0.027s to 0.096s and total dev time from 0.93s to
0.98s. Reject before large; the longer witness does not cut enough hierarchy
fill or query work.

An exact two-sided CH-label index was prototyped for road2d. Labels are
computed on the hierarchy DAG in descending rank by merging each higher
neighbor's exact upward-distance labels; query answers are the minimum sum on
the forward/backward hub intersection. On road2d_dev it was exact with
11.77M logical labels, 179.6MiB compact label storage, 1.224s label build,
and 2.02s total versus 0.93s CH. Targeted road2d_large hit the 120M-entry
guard after 18.39s of label construction (about 2.07GiB RSS), then used the
exact CH fallback; all 200,000 answers matched, but total was 35.91s versus
16.72s retained. Full labels exceed the practical preprocessing/memory
budget, so reject. Scratch source: `/tmp/road_ch_labels_compact.cpp`.

### Overall baseline reconciliation and rank-Dijkstra rejection (2026-09-27)

Reconciled the two comparable formal result artifacts with `grade.py`'s
six-speedup geometric mean. The retained current source
`solver.cpp` and `research/checkpoints/certified-integrated-4610587.cpp` have
the same SHA256
`4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`; the
matching official binary SHA is
`2582aed37dab141b5d52d618ca4a14879c2d8747e7579c7b30a63bce9ad5401d`. Its
same-run result `result-integrated-4610587-full.json` records Overall
88.35080960833166x and all six large rows `ok`. The older complete
`result-hugeq4-full.json` records 51.946922866870345x, also with all six
large rows `ok`. These use the same six-instance scoring formula; the source
and binary hashes in each JSON identify different candidates. Therefore the
comparable formal baseline is 88.351x, and the 150x goal remains open.

The requested Python-command diagnosis is now confirmed directly:
`python --version` exits 127 because this environment has no `python` command;
`python3` resolves to `/usr/bin/python3` (Python 3.14.4) and succeeds. The
repair is to invoke repository scripts with `python3`, which all subsequent
commands do. There was also a separate historical status-127 incident: a
temporary road prototype failed compilation on the removed
`road_region_arc_may_be_shortest`, and `/usr/bin/time` then tried to launch
the absent binary. That second failure was distinct from the missing `python`
alias; neither changed repository source.

For a fail-fast wide64 experiment, I compiled
`/tmp/wide64_rank_dijkstra.cpp`, which bypasses 24-landmark preprocessing on
the structurally verified high-weight 2-D torus and runs ordinary exact
one-sided Dijkstra per query. It matched all six official dev answer files,
including all 10,000 wide64_dev answers, but wide64_dev took 6.016s on a
repeat (6.10s on the first run), versus the retained ALT dev measurement near
0.36s in a same-session run; RSS was 12MiB. The rank workload settles targets at powers-of-two
positions, not at single-digit ranks, so the expected shallow-search benefit
does not occur. Reject before fresh-seed or large testing. This scratch-only
candidate did not change `solver.cpp`; the 88.351x same-run formal checkpoint
and its result remain intact.

A second wide64 architecture added exact two-edge straight-line shortcuts to
the verified weighted torus after landmark preprocessing. Each shortcut is
the sum of two retained original edges, so adding it preserves every shortest
distance; the existing landmark potential remains consistent on the shortcut
because it is consistent along every original path. The prototype compiled
and matched all six official dev outputs. On wide64_dev it added 164,570 arcs
(181,488 to 346,058 total), expanded 4.26M heap entries and scanned 22.15M
arcs, versus the retained 0.36s path's recorded 3.38M / 12.23M work; elapsed
time was 0.44s versus 0.36s and RSS 32MiB versus 23MiB. It increased rather
than reduced search work, so reject before fresh-seed or large testing.
Scratch source: `/tmp/wide64_two_step_shortcuts.cpp`. `solver.cpp` remains
byte-identical to the 88.351x certified checkpoint.

As an offline-batching screen, I counted repeated targets in the six current
large query files. Queries whose target occurs at least four times: road2d
20,844/200,000 (10.4%), lattice3d 4/30,000, local2d 25,729/300,000 (8.6%),
scalefree 15,027/50,000 (30.1%), hugeq 599,703/600,000, and wide64 0/30,000.
The road threshold-2 CH batch already regressed from 23.49s to 36.54s, while
local target grouping and scalefree full target batching were also measured
slower than their current paths. This distribution does not support another
generic endpoint-grouping attempt; hugeq already uses the retained separator
and target-group path.

For a different local2d architecture, `/tmp/local_tile_portal_oracle.cpp`
builds exact within-tile APSP tables for verified low-weight 2-D torus tiles
(B=8), then searches a graph of boundary portals with the retained 256-grid
ALT landmarks. The overlay is exact: tile-internal overlay edges are exact
restricted paths, boundary edges are original crossing edges, and every
original path decomposes into those pieces; the original ALT remains consistent
on every overlay edge. The candidate compiled and matched all six official dev
answer files. Tile tables and portal overlay took 0.207s, creating 784 tiles,
20,361 portals, and 532,248 directed arcs; landmark construction took 0.798s.
The local2d query used 464,712 portal pops and 10.562M arc scans. Its total
local2d_dev time/RSS was 1.11s/85MiB versus the retained path's same-session
0.87s/62MiB. Although heap pops fell, dense portal arc scanning increased
work and end-to-end time, so reject before fresh-seed or large testing. A
temporary compile issue (`INT_MAX` was undeclared) was fixed with
`std::numeric_limits<int>::max()`; no repository source changed.

### CH scheduling and early-termination screens (2026-09-27)

The road CH query was changed in `/tmp/road_ch_workbalanced_profile.cpp` to
choose the direction with fewer cumulative stall/relax arc inspections. It
compiled and matched `road2d_dev`; elapsed time was 0.89s, tied with the
0.89s baseline. Actual stall scans increased from 3.40M to 3.69M and relaxation
scans from 2.90M to 3.08M, so reject before fresh/large testing.

A stronger early-stop condition was screened next. Replacing the retained CH
condition (`minF >= best && minB >= best`) with the generic bidirectional
Dijkstra sum bound (`minF + minB >= best`) produced 4,159 wrong answers on
`road2d_dev` when stall-on-demand was enabled. Disabling stall did not repair
the condition; the first answer remained wrong. The ordinary bidirectional
sum proof does not directly transfer to these upward-only CH frontiers. Both
variants were rejected immediately; no fresh or large run and no production
source change.

### Wide64 64-landmark coverage screen (2026-09-27)

Increasing the global landmark cover from 24 to 64 for structurally verified
high-weight 2-D tori, while retaining per-query selection of the strongest
four, is exact and substantially reduces search work. The scratch candidate
`/tmp/wide64_alt64_uninstrumented.cpp` matched all six official dev answers,
fresh `wide64_dev` seed 987654321 / salt `lm64-screen`, and all
`wide64_large` answers. On large, an instrumented run measured 110.42M settled
pops / 399.30M arc scans, versus 213.08M / 770.73M for an instrumented
24-landmark baseline. End-to-end candidate runs were 26.12s and 26.68s, versus
28.65s for the same-session retained binary; peak RSS rose from about 647MiB
to 1,272MiB. On dev it regressed from 0.36s to 0.44-0.46s. Despite nearly
halving search work, extra landmark preprocessing and doubled table memory
leave only a 7-9% large-tier gain, far below the multi-fold research gate.
Reject for the 150x campaign; `solver.cpp` and the certified checkpoint remain
unchanged.

A deeper exact lattice3d sparsification pass ran a strictly-lighter Dijkstra
for each surviving candidate edge, stopping after 128 settled vertices. This
found 7,701 additional undirected witness pairs and raised removed arcs from
44.956% to 50.024%. It reduced lattice3d_dev query scans from 18.315M to
16.458M (10.1%) while pops were effectively unchanged, 5.286M to 5.279M. The
extra witness searches cost 0.43s preprocessing and increased end-to-end dev
time from 1.22s to 1.87s. Reject before fresh/large testing; query savings
cannot pay the extra work at this scale, and `solver.cpp` remains unchanged.

On hugeq_large, replacing some same-target restricted ALT groups with exact
same-source, same-leaf forward multi-goal ALT groups retained the separator
table answers as feasible incumbents and passed all official outputs. It took
9.63s / 1,213MiB versus the same-session retained path's 9.63s / 1,213MiB; the
candidate's grouped phase processed 23,568 groups, 20.92M pops, and 113.41M
arc inspections. The opposite orientation did not improve the endpoint-sharing
mechanism, so reject before fresh-seed testing. Prototype:
`/tmp/hugeq_source_group_alt.cpp`.

The first profiling build of that temporary hugeq candidate failed because a
counter insertion landed in the unrelated generic `grouped_side` before its
declaration. The temporary source was corrected and recompiled successfully;
this was not the Python status-127 issue. No repository source was affected.

For lattice3d, an exact coordinate projection relaxation was added to the
existing 128-landmark balanced ALT. Each of the three quotient graphs drops
one torus coordinate at zero cost and assigns every retained crossing arc the
minimum weight observed for that quotient transition; projecting any retained
path proves the quotient distance is a consistent lower bound. The candidate
compiled and matched lattice3d_dev exactly, but combined projections exceeded
the original ALT bound on only 1,265 heuristic evaluations. Search work stayed
at 5.286M pops / 18.314M scans (within 0.01% of retained), while runtime rose
from 1.22s to 1.83s. Reject before fresh/large; the bound is dominated.

A follow-up to the CH sum-bound screen also updated the feasible incumbent
whenever a relaxed endpoint already had a finite opposite-side label. The
`minF + minB` condition with stall still returned a wrong road2d_dev answer
(first observed mismatch at query 2). Thus the sum-bound direction remains
rejected for this upward-only CH query; production keeps its conservative
termination test.

### Wide64 clipped-landmark screens (2026-09-27)

Sixty-four farthest-point roots with a 32,768-settlement truncation store
`min(d(root,v),R_root)` rather than full SSSP rows. This remains an exact ALT
potential because scalar truncation is 1-Lipschitz. The candidate matched all
six dev outputs, fresh wide64_dev seed 987654321 / `lm64-screen`, and every
wide64_large answer. Despite substantially reducing dev work, large work barely
changed: 211.02M / 763.26M pops/scans versus 213.08M / 770.73M. Large took
35.03s / 1.65GB versus 28.65s / 647MiB retained; dev was 0.49s versus 0.36s.
The sparse global cover does not scale to the large graph, so reject.

A 16x16 regularly spaced set of clipped roots with sparse constant-time maps
also failed on wide64_large: it matched the official answers but took 35.75s /
1.71GB and settled 206.40M pops / scanned 746.56M arcs, versus 28.65s retained.
On local2d_dev, the same local-root concept with exact global ALT only for long
queries matched all outputs and took 0.83s / 114MiB versus 0.89s / about
110MiB, but increased search from about 1.06M / 3.83M to 1.21M / 4.36M
pops/scans. The dev preprocessing saving is too small to offset larger-tier
query work; reject before local fresh/large testing. All variants are scratch
only and `solver.cpp` remains unchanged.

### Local2d capped-root large-tier falsification (2026-09-27)

Because the retained 256 full-grid SSSPs take 5.002s of the 13.66s
local2d_large path, I re-tested the existing exact radius-by-settlement cap at
the large tier. The scratch source `/tmp/local2d_capped_landmarks.cpp` builds
24 global landmarks and stores only the first 32,768 settled vertices for each
of 256 regular local roots. It compiled under C++17 and matched all 10,000
local2d_dev answers. On local2d_large it also matched every official answer,
but took 31.43s / 366MiB versus the retained path's 13.66s / 379MiB. Local
search expanded 246.33M pops / 903.09M arc scans; the capped preprocessing
saved about 4.34s, but missing local-root distances caused much larger query
work. Reject; no production change and no fresh seed is warranted after this
large exact regression.

### Road CH feasible corridor incumbent screen (2026-09-27)

For each query, a candidate upper bound was formed from exact original
backbone-edge prefix sums: the best path through any structurally recognized
fast row or fast column. Since it is a concrete directed path, initializing
the retained CH query's incumbent with this value is safe; the existing
conservative CH cutoff still certifies any improvement. The scratch candidate
compiled and matched every road2d_dev answer. Against an instrumented copy of
the same CH path, dev time was 0.91s versus 0.94s, while work was effectively
unchanged: 755,450 versus 755,551 valid pops, 3.401M versus 3.402M stall scans,
and 2.902M versus 2.902M relaxation scans. The route bound is usually no
tighter than the early CH meeting incumbent, so reject before large testing.
Scratch sources: `/tmp/road_ch_route_ub.cpp` and
`/tmp/road_ch_base_profile.cpp`; production remains unchanged.

### Wide64 offline-pair geometric landmark selection screen (2026-09-27)

The 24 landmark roots were selected greedily from a 32x32 spatial candidate
grid to maximize the sampled queries' maximum Manhattan-distance endpoint
separation, then exact SSSP tables were built for those selected roots. This
uses the observable query batch only for preprocessing; all query lower bounds
still use exact graph distances. It compiled and matched every wide64_dev
answer, but the proxy did not select useful weighted-metric roots: 4.418M pops
/ 15.949M scans and 0.46s versus about 3.379M / 12.228M and 0.36s for the
retained farthest-point roots. Reject before fresh-seed or large testing. Scratch
source `/tmp/wide64_query_landmarks.cpp`; production unchanged.

### Road CH edge-difference priority screen (2026-09-27)

The road CH node priority was changed from estimated incoming-times-outgoing
work to the classic degree-based edge-difference proxy `in*out - in - out`,
while retaining the verified corridor-class ordering and exact witness/stall
logic. The candidate compiled and matched every road2d_dev answer, but added
shortcuts (226,694 versus 224,335) and did not improve work or time: instrumented
query work was 752,656 pops / 3.434M stall scans / 2.928M relaxation scans,
versus 755,551 / 3.402M / 2.902M retained, at 0.88s versus 0.94s. The tiny pop
change is offset by more scans and no material end-to-end evidence; reject
before large testing. Scratch source `/tmp/road_ch_edge_diff_profile.cpp`;
production is unchanged.

### hugeq fifth separator level screen (2026-09-27)

The current profile shows 19.31M grouped-search pops / 104.70M arc scans in
25,912 restricted searches after separator tables for 224 top, 223 second,
468 third, and 560 fourth-level hubs. A fifth coordinate bisection was added
inside each fourth-level interior, with exact directed distances restricted to
the parent leaf; same-child grouped search was restricted to the fifth-level
child. The first forced-separator dev check found a real omission: same-parent
queries also need the parent-micro boundary table because their shortest path
may leave and re-enter that child. After adding that candidate, 260,000 forced
hugeq_dev answers matched exactly. The official hugeq_large run was also exact,
but took 8.75s / 1,991MiB versus the retained 9.07s / 1,184MiB profiling run.
The roughly 3.5% time gain costs about 807MiB extra RSS, so reject for the
150x objective. Scratch source `/tmp/hugeq_depth5.cpp`; production unchanged.

### Wide64 torus-route incumbent screen (2026-09-27)

For each wide64 pair, the best of the two feasible L-shaped routes around the
verified torus was computed from exact original edge-weight cycle prefixes and
used as an incumbent for ALT4. The exact lower-bound cutoff returned the
route only after proving no smaller path remained. The candidate compiled and
matched all wide64_dev answers, but work barely moved: 3.379M pops / 12.190M
scans versus 3.379M / 12.228M retained; time was 0.34s versus about 0.36s.
Reject before fresh/large testing. Scratch source
`/tmp/wide64_route_upper.cpp`; production remains unchanged.

The road corridor incumbent was extended to enumerate routes through a nearby
fast-row/fast-column intersection, using the original edge prefix sums. The
first scratch run segfaulted because it iterated fixed-size candidate arrays
past their populated counts; the loops were corrected and the rebuilt
candidate matched road2d_dev. CH work remained unchanged (755,445 pops /
3.401M stall scans / 2.902M relaxation scans) and time was 0.92s versus 0.94s
instrumented baseline. Reject before large testing. Scratch source
`/tmp/road_ch_route_ub.cpp`; production unchanged.

### Road CH actual shortcut edge-difference screen (2026-09-27)

The contraction score was then based on the actual witness-generated shortcut
count for each candidate, using signed edge difference plus the existing
level and corridor penalties. The zero-clamped variant recreated the retained
224,335-shortcut hierarchy in 0.90s. An offset signed variant produced
218,865 shortcuts (2.4% fewer) and remained exact on road2d_dev, but build
time rose to 2.90s / 37MiB versus about 0.9s / 32MiB; the measured fill
reduction does not justify more than tripling dev preprocessing and is not a
multi-fold query opportunity. Reject before large testing. Scratch sources:
`/tmp/road_ch_actual_ed.cpp` and `/tmp/road_ch_actual_ed_offset.cpp`;
production remains unchanged.

### Wide64 query-midpoint quadrant landmark screen (2026-09-27)

For each query, the strongest exact ALT landmark was selected from each
available spatial quadrant around the torus midpoint, then the remaining
slots were filled by endpoint distance difference. Root selection used only
verified torus coordinates; all potentials remained exact. The candidate
compiled and matched all wide64_dev answers, but expanded 3.617M pops /
13.049M scans and took 0.38s versus 3.379M / 12.228M and about 0.36s for the
retained top-four endpoint-separation rule. Reject before fresh/large testing.
Scratch source `/tmp/wide64_diverse_landmarks.cpp`; production unchanged.

### Road CH no-stall query profile (2026-09-27)

The instrumented large baseline spends 8.14s building CH and 8.40s in
200,000 queries; the latter checks 183.73M stall arcs and scans 147.26M
relaxation arcs. Removing stall-on-demand remains exact: road2d_dev and fresh
seed 20260927 matched, and road2d_large matched all 200,000 official answers.
On large, no-stall performed 36.42M pops / 258.82M relaxation scans versus
28.78M pops / 183.73M stall checks / 147.26M relax scans with stall. The
instrumented total fell from 17.25s to 16.24s; uninstrumented same-run totals
were 16.50s retained and 16.18s no-stall, only a 1.9% gain. Since contraction
still dominates and the real end-to-end gain is small, do not retain this path
for the 150x campaign. Scratch source `/tmp/solver_nostall.cpp`.

### Wide64 balanced bidirectional ALT recheck (2026-09-27)

The retained wide64 path uses the strongest four of 24 exact landmark tables
in a forward ALT search. A current-source candidate instead used the existing
balanced bidirectional ALT engine with its exact minimum-key termination
condition. It matched all wide64_dev answers, but dev work was 3.357M pops /
12.160M scans and 0.42s versus 3.379M / 12.228M and 0.36s retained. Because
large queries are more local, it was also checked on wide64_large: all official
answers matched, but it took 37.10s / 647MiB with 210.997M pops / 763.352M
scans. The retained large result is 28.70s / 632MiB with about 213.081M /
770.732M work. The roughly 1% reduction in scans does not pay for two-sided
queue and heuristic overhead; reject. Scratch source
`/tmp/wide64_bialt_current.cpp`; the certified solver remains unchanged.

### Wide64 vertex-major ALT table layout (2026-09-27)

The 24 exact wide64 landmark arrays were transposed from root-major to
vertex-major storage, and the same four strongest per-query potentials were
read from each vertex row. This compiled and matched all wide64_dev and
wide64_large answers, but was slower at both sizes: 0.40s on dev and 35.20s /
996MiB on large, versus about 0.36s and 28.70s / 632MiB retained. The added
memory traffic and larger peak RSS outweigh the more contiguous heuristic
loads; reject. Scratch source `/tmp/wide64_vertex_major.cpp`; production
remains unchanged.

### Wide64 conservative 32-bit landmark compression (2026-09-27)

Exact root-major landmark distances were stored as floor(distance / scale) in
32 bits. The A* potential subtracts one quantization bucket and uses
`(scale-1)` as its unit; with the observed minimum edge weight and chosen
scale, this remains an admissible consistent lower bound. The candidate
matched wide64_dev and wide64_large exactly (scales 16 and 64), but ran 0.70s
on dev and 45.00s / 815MiB on large versus about 0.36s and 28.70s / 632MiB
retained. Quantized root ranking and extra per-pop arithmetic outweighed the
smaller final table; reject. Scratch source
`/tmp/wide64_compressed_alt.cpp`; production remains unchanged.

### Local2d score-selected anchor screen (2026-09-27)

For short local2d pairs, the four geometric-nearest grid anchors were replaced
by the four anchors with the largest exact source-target distance difference
from the existing 256 tables. The candidate matched every local2d_dev answer
and slightly reduced work from 1.062M pops / 3.828M scans to 1.032M / 3.763M,
but dev runtime was 0.92s versus about 0.89s retained. This does not show a
material gain; reject before large testing. Scratch source
`/tmp/local_score_landmarks.cpp`; production remains unchanged.

### Local2d global-landmark bidirectional screen (2026-09-27)

This candidate replaced the 256 exact local-grid SSSP tables with the standard
24 farthest-point tables, sent pairs within toroidal Manhattan distance 96 to
plain bidirectional Dijkstra, and used exact balanced ALT for the far tail. It
compiled and matched local2d_dev, improving the small tier from about 0.89s to
0.58s through lower preprocessing. On local2d_large it remained exact but took
42.15s / 147MiB versus 13.12s / 379MiB retained. The generic bidirectional
search loses too much on the 90% rank-local batch; reject. Scratch source
`/tmp/local_global_bidir.cpp`; certified source remains unchanged.

### Wide64 nested-dissection CH screen (2026-09-27)

A 2-D recursive separator class was used as a contraction priority for the
existing exact witness-CH builder on the verified wide torus. The candidate
compiled and matched all wide64_dev answers, but contracting only 50k nodes
already took 27.14s and generated 1.24M shortcuts (about 55MiB RSS), versus
subsecond retained dev processing. Reject before large testing; this CH order
has prohibitive witness/fill cost. Scratch source
`/tmp/wide64_nested_ch.cpp`.

### Road CH ALT frontier correctness screen (2026-09-27)

Adding 12 directed ALT landmark pairs directly to the road CH query and using
a doubled bidirectional min-key bound returned a wrong road2d_dev answer
(query 14: 5,928,988 versus 4,322,855); disabling stall did not repair it.
A second variant built exact landmarks on the symmetric minimum-cost
relaxation, making the heuristic Lipschitz in both arc directions, but its
min-key and a conservative g-frontier stop still mismatched dev. The
upward/downward hierarchy plus the query frontier bound needs a separately
proved stopping rule; reject this candidate family before large testing.
Scratch sources `/tmp/road_ch_alt.cpp` and
`/tmp/road_ch_relaxed_alt_gstop.cpp`; production remains unchanged.

### Repeated ALT-key evaluation screens (2026-09-27)

For wide64 forward ALT, the current consistent key was saved per vertex so a
heap pop can reject stale entries without rereading four landmark tables. It
matched dev and both large answer checks. Large timings were 27.48s and 28.02s
for the candidate versus 28.07s for a same-session certified run; a third
`/usr/bin/time` print of 4,056s disagreed with the process wall clock and is
discarded as an invalid timing sample. The two comparable runs show no stable
material gain; reject. Scratch source `/tmp/wide64_cached_h.cpp`.

The same saved-key idea was applied to both sides of lattice3d balanced ALT,
avoiding a potential recomputation when checking popped entries. It compiled,
matched lattice3d_dev, and took 1.16s versus 1.17s for the certified binary in
same-session dev runs. This is within noise and was rejected before large
testing. Scratch source `/tmp/lattice_cached_alt_key.cpp`.

### Local2d bounded bidirectional fast-path screen (2026-09-27)

Pairs with torus Manhattan displacement at most 96 first attempted at most 32
plain bidirectional pops; if the exact min-key proof had not completed, the
retained 256-landmark A* fallback ran from scratch. The candidate matched all
local2d_dev answers, but took 0.90s versus about 0.89s retained. It hit 2,626
queries after spending 257,571 exploratory pops, so the bounded phase did not
save end-to-end time. Reject before large; scratch source
`/tmp/local_bounded_bi_first.cpp`.

### Wide64 no-witness nested-dissection fill screen (2026-09-27)

The exact witness CH builder was changed to add every in/out neighbor shortcut
at each node in the same verified 2-D recursive ordering, skipping witness
Dijkstra. All added edges represent real two-edge paths, so this can only add
valid routes. The candidate compiled and matched 16 independently
Dijkstra-checked queries on a fresh 32x32 weighted torus. On wide64_dev,
however, it reached 40,000/50,176 contractions after over 60s with 884,626
shortcuts and had not produced answers; the run was interrupted. This fill
growth and preprocessing cost reject the architecture before large testing.
Scratch source `/tmp/wide64_nested_ch_nowitness.cpp`.

### Wide64 cut-minimum coordinate potential screen (2026-09-27)

The minimum retained edge weight at every horizontal/vertical torus cut was
summed along the cheaper coordinate arc to form an exact consistent lower
bound, then max-composed with the four-landmark ALT potential. It compiled
and matched wide64_dev. The cut bound exceeded ALT on only 143,802 of 9.67M
heuristic evaluations (1.5%); dev time rose from about 0.36s to 0.39s. Reject
before large testing. Scratch source `/tmp/wide64_cut_alt.cpp`.

### Retained checkpoint after 2026-09-27 autonomous screens

No new candidate met the stable end-to-end retention gate, so production was
not changed and no redundant full grade was run. `solver.cpp` and
`research/checkpoints/certified-integrated-4610587.cpp` both remain SHA256
`4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`; the
matching formal six-large result remains
`result-integrated-4610587-full.json` (Overall 88.35080960833166x, all six
exact). The 150x goal is still unmet. All new scratch implementations are in
`/tmp`; recent experiment outcomes and rejection reasons are recorded above.

### Bounded architecture comparison and radius-label screen (2026-09-27)

User scope: compare 2-3 new directions, time-box the strongest prototype,
retain only dev/fresh/large-exact stable end-to-end winners, and stop ineffective
fine tuning when no credible multi-fold route remains. Existing fifth-level
hugeq, ALT/CH variants, full PLL, tile DDG and full separator-table failures
were read before selecting a candidate. No previously rejected tuning was run.

The six-instance Overall is an unweighted geometric mean. From 88.3508096x,
a single 2x instance gain projects 99.1704x; a single 3x gain projects 106.1038x.
Reaching 150x requires a product of per-instance time improvements of about
23.9488. These are score projections using the certified baselines, not grades.

| New direction | Potential mechanism / impact on all six | End-to-end feasibility and memory | Exactness requirement / decision |
|---|---|---|---|
| Radius-truncated exact two-hop labels for local2d | Replace most short-query searches with intersections; hypothesized 2-3x local improvement -> Overall 99.17-106.10x, other five paths unchanged | About 30M labels projected from dev, 240MB logical 8-byte labels plus vector capacity and existing 379MiB state; build must be cheap enough to pay back query savings | Complete all roots within observable metric radius; only accept intersection <= radius; otherwise exact existing ALT fallback. Selected for bounded prototype. |
| Exact target-region arc flags for wide64 | Potential 2-3x if query scans collapse; same Overall projection, other five paths unchanged | 16x16 regions imply about 45k boundary-root global SSSPs, versus 24 retained roots. Scaling the measured 5.50s landmark construction gives roughly 10,000s, before queries. About 230MB nominal 256-bit arc flags in addition to graph/index state. Restricted/sampled construction would need a different exactness argument and measured benefit. | Must retain a shortest route to every target in each region, including ties. Reject the full exact construction on preprocessing cost; no code built. |
| Query-target compressed first-move database for road2d | Could replace two-frontier searches with exact route following; a hypothetical 2x road improvement -> Overall 99.17x, other five paths unchanged | 300,304 vertices x 135,113 observed unique targets gives 40.57B entries, about 15.2GB even at nominal 3 bits per entry before compression/overhead. Requires about 135k full target SSSPs. No evidence of sufficient exact compression or fast construction. | Each move must follow a shortest-path tree toward its target; route following must account for every weight. Reject before implementation: preprocessing and unproven compression dominate. |

Prototype: `/tmp/local_radius_pll.cpp`, compiled with `g++ -O2 -std=c++17`.
It reuses the existing separator-first PLL order but bounds every search by
`16 * median(retained edge weight)` (observable, no instance identity). A
root-distance cache makes pruning scan only the candidate vertex's label list.
This is a truncated exact-cover index, distinct from capped ALT lower-bound
landmarks and the rejected full unbounded PLL index. The index has a 12s
construction guard and a vertex-scaled label-count guard; an incomplete index
is discarded and the exact production path answers every query.

Correctness: labels encode real positive-weight routes. For any pair with true
distance <= R, take the earliest-ranked vertex on a shortest path. Both endpoint
subpaths stay within R. Earlier-root pruning either records that cover or
substitutes an already-existing equally short cover; truncation cannot remove
it. Therefore an intersection <= R is the true shortest distance. A larger or
missing intersection uses the retained exact ALT fallback. The proof requires
a completed index; partially built labels are never used to certify answers.

All six official dev outputs matched their reference files. local2d_dev built
4,913,874 labels in 0.884-0.906s, certified 6,991/10,000 queries and took
1.77-1.78s / 121,592KiB, versus 0.88s for the certified binary in the same
session. Fresh seed 2026092701, independently generated 48x48 positive-weight
torus, matched heapq Dijkstra for all 264 queries: 229,061 labels, 0.046s build,
143 certified hits and 0.08s / 10,940KiB end-to-end. The seed includes self-pairs
and queries outside the index radius, exercising exact fallback.

A single targeted large screen confirmed the mechanism's failure rather than
relying only on extrapolation. All 300,000 local2d_large answers matched:
30,752,036 labels, 6.022s label construction, 37,948,995 preprocessing pops,
113,221,446 arc scans, 2,135,352,839 label checks, 209,859 certified queries.
Existing grid-landmark construction still cost 5.035s. Total was **18.90s /
751,620KiB**, versus the sequential same-session certified run **13.47s /
388,452KiB**, also all exact. Remaining parsing/intersection/fallback/output
cost was about 7.84s, near the baseline's 8.46s outside grid construction.
The 70% query-count coverage primarily avoids cheap searches, while adding
6s construction and nearly doubling RSS. Reject; no production edit or
full-grade rerun. Validation metadata is in
`research/logs/radius-pll-dev-fresh-20260927.json` and
`research/logs/radius-pll-large-20260927.json`.

No credible multi-fold path emerged from this bounded comparison. Stop this
run's ineffective tuning as explicitly requested. Preserve the exact 88.3508x
source/binary/result above; the formal 150x goal remains unmet. A future run
needs a new evidence-backed architecture, not radius sweeps or another ALT/CH
variation. No benchmark remains running.

### New retained sparse separator label candidate (2026-09-27)

Production now includes a new exact partial-label engine only behind the
existing coordinate/high-Q separator dispatch. `solver.cpp` SHA256 is
`817fe03416a0c8d529e074b876d0a0429bc6e1bdd47b8cf73e1f9fb18414d599`;
binary SHA256 is `b2d7df0ff8c15fdcf567f1067671cbeb3541d8c1b39f19fa74baa991d67661a3`.
This is a targeted validated candidate, NOT a new formally certified Overall.
The authoritative formal result remains the exact six-large 88.3508096x
artifact for source 4610587e. Its source, original binary and matching JSON
are preserved at `research/checkpoints/certified-integrated-4610587.{cpp,bin,json}`.
New candidate source is also saved as
`research/checkpoints/candidate-sparse-sep-817fe034.cpp`.

Mechanism: retain all four current geometric separator vertex covers and
membership codes, but collect their 1,475 roots before distance construction.
Order roots by verified road speed class (fastest first), then observed degree.
Build directed forward/reverse partial pruned labels using exact route-cover
pruning and a root-distance cache. Labels store actual route lengths; every
shortest path through any indexed separator retains a two-hop cover by the
standard earliest-root induction. Cross-region queries are therefore exact
label intersections. Same-leaf interior routes retain the original exact
restricted grouped ALT fallback. Original dense separator construction remains
available if the index exceeds 20 seconds or 30 million labels. Ordering is
purely observable graph structure; no filenames/seeds/hashes/identity dispatch.
Only solver.cpp source changed. No other workload's dispatch or query engine
was changed. Existing builders now have a topology-only collection mode,
avoiding duplicate partition logic and shared-state conflicts.

Measurements (all exact):
- Natural separator order: forced hugeq_dev 260k queries, 24,207,092 labels,
  11.763s index / 12.28s total / 670,288KiB; reject natural order.
- Fast-corridor-first roots: same forced dev 4,651,191 labels,
  1.277s index / 1.62s total / 154,788KiB. Preprocessing work falls from
  24.945M to 5.107M pops, 130.962M to 25.163M arc scans and 3.152B to
  130.302M prior-label checks.
- Fresh seed 2026092702, salt sparse-sep-corridor: generated road graph
  V=50,176 E=271,475; refsolve answered all 10,000 nontrivial uniform pairs;
  appended 250k self-pairs force separator dispatch. All 260k match; labels
  5,048,483, index 1.354s, total 1.53s / 158,820KiB.
- All six ordinary dev answer files exact for scratch, cleaned candidate and
  production binary. See research/logs/sparse-sep-dev-fresh-20260927.json,
  sparse-sep-clean-validation-20260927.json and
  production-sparse-sep-817fe034-validation.json.
- hugeq_large: 600,000/600,000 exact at 4.71s / 176,292KiB, repeat 4.68s /
  176,484KiB; cleaned implementation 4.72s / 176,484KiB. Sequential same-session
  certified binary 9.17s / 1,212,968KiB, all exact. Approximately 1.95x gain
  and 85% lower peak RSS. Production recheck later took 5.67s / 176,528KiB;
  every ordinary dev timing in that block also increased by about 10-20%,
  so a new sequential certified-binary check is pending to resolve host drift.

A related full-label extension indexed all 50,176 vertices, aiming to remove
all grouped leaf search. It was exact on forced dev but built 17,817,926
labels with 2.471B label checks, taking 8.383s construction / 8.82s total /
472,804KiB. Construction alone exceeds the retained partial-label total on
large; reject before fresh/large. Prototype /tmp/hugeq_full_corridor_labels.cpp.

Earlier screens this run:
- Exact landmark-tree jump proposals cannot skip any vertex whose fixed
  consistent A* key is below the optimum. wide64_dev profiling found only
  846,689/3,379,244 pops (25.06%) on the final equal-key plateau; even ideal
  plateau removal supports only about 1.33x query work, before preprocessing
  and new tables. Probe was dev-exact; reject before building tree jump tables.
- A large confirmation of the previously screened degree-2 lattice reduction
  removed 61,827/300,763 vertices but only 12.45% arcs; do not repeat it.
- New degree<=3 exact elimination feasibility screen on sparsified lattice3d
  removed 137,719/300,763 vertices (45.79%), while arcs fell from 993,580 to
  866,100 (12.83%). The remaining core has higher degrees. This does not support
  a 2x end-to-end improvement given unchanged landmark table/output costs;
  reject before query-engine implementation. /tmp/torus_degree3_screen.cpp.

The new projected Overall is roughly 98.6x if other five certified times stay
unchanged and hugeq remains near 4.7s. This is not a grade and is far from 150x.
Do not run full six-large grade solely for this one ~12% projected Overall win;
accumulate another large architectural gain first, as requested. Next research
must attack the weak local/wide paths through a genuinely different mechanism,
not repeat ALT/CH variants, degree suppression or full PLL.

### Sparse separator continuation: road screen and final paired check

The production recheck's host drift was resolved by a sequential baseline
run: certified 4610587e binary 10.33s / 1,212,920KiB, candidate 817fe034
5.67s / 176,528KiB, both all 600k exact. Thus the retained targeted hugeq
improvement is 1.82-1.96x across comparable blocks, with about 85% lower RSS.
Projected Overall with otherwise unchanged timings is about 97.7-98.8x;
this is explicitly uncertified. Validation JSON now records both versions,
paired baseline and all targeted repeats. Candidate source, binary and metadata
are preserved as research/checkpoints/candidate-sparse-sep-817fe034.{cpp,bin,json}.

Road applicability was tested rather than assumed. The prototype
/tmp/road_sparse_sep_candidate.cpp uses the sparse separator engine at legal
coordinate/directed/degree/query-density dispatch (Q/V>=0.5) and preserves
CH or directed ALT fallback if the index guard fires. Forced road2d_dev with
40k appended self-pairs and the new fresh seed's corresponding forced batch
were exact: 1.57s / 145,176KiB and 1.42s / 139,248KiB. On road2d_large,
however, just 601 roots generated 33,214,019 labels, 34.129M preprocessing
pops, 148.778M scans and 980.653M checks in 11.318s. The 30M-label guard
discarded the index and restored CH. All 200k final answers were exact, but
total was 30.83s / 951,500KiB. Reject this road dispatch; it was never integrated.
Production still uses the certified targeted-witness CH/stall road path.

The meaningful retained change in this run is therefore hugeq only. All
six dev, fresh separator seed and full hugeq_large comparisons are exact.
No new full six-large grade has been run: only one large winner accumulated,
and the projected Overall gain is about 11%, below the requested roughly
25% re-certification trigger. Do not label the new source as 88.3508x-certified;
that result belongs to the preserved 4610587e baseline. Formal >=150x remains
open. Preserve the current candidate and prioritize a different local/wide
architecture next; the failed road and full-label routes are closed.

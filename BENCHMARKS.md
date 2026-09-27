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

## 150x continuation checkpoint

The certified source remains unchanged at SHA256
`4923eab6abca0401805e94a2b0a6112ddfe26e3926c17247c30a0cae7ba109f0` and its
best complete official result is Overall 12.460011312089689, all six `ok`.
The following isolated exact prototypes were recovered from the lab stash and
validated independently; none has been installed into `solver.cpp` yet.

| candidate | workload | total | correctness / memory |
|---|---|---:|---|
| targeted-witness CH + stall | road2d_large | 27.87 s | 200k/200k exact, 233 MB |
| balanced ALT, 24 landmarks / 4 selected | lattice3d_large | 45.36 s | 30k/30k exact, 124 MB |
| balanced ALT, undirected grouping disabled | local2d_large | 35.64 s | 300k/300k exact, 145 MB |
| pruned hub labels | scalefree_large | 48.66 s | 50k/50k exact, 1.59 GB |
| core contraction + hub labels | scalefree_large | 45.68 s | 50k/50k exact, 453 MB |
| cell-ordered witnessed CH | wide64_dev | 30.53 s | all dev answers exact, 59 MB |
| dominated-edge sparsification + CH | lattice3d_dev | 4.80 s | all dev answers exact, 31 MB |

On the full 3-D lattice, the same exact dominated-edge sparsification removed
44.941% of undirected arcs (each removed edge has a positive 2-/3-edge detour)
and reduced balanced ALT24/4 to **42.68 s**, all 30,000 answers exact, 111 MB
RSS. The unsparsified ALT run was 45.36 s / 124 MB. This is a safe memory and
small speed gain; it does not explain the remaining gap to 150x.

Fresh random validation includes 80 directed graphs for CH/labels and 40
undirected graphs for sparsification, covering duplicate arcs, self-loops,
unreachable pairs, and 64-bit weights. These are exploratory per-workload
results, not an Overall certification; combined dispatch and fresh-seed full
validation remain required before replacing the checkpoint.

## Combined full certification attempt

The integrated candidate was compiled from the exact prototypes and graded
with the required full command. All six large instances returned `ok`:

| instance | T_base | T_solver | speedup | RSS |
|---|---:|---:|---:|---:|
| road2d_large | 953.257s | 28.217s | 33.783x | 227MB |
| lattice3d_large | 1679.830s | 35.671s | 47.092x | 108MB |
| local2d_large | 647.955s | 37.494s | 17.281x | 141MB |
| scalefree_large | 1943.555s | 52.841s | 36.781x | 1552MB |
| hugeq_large | 2151.363s | 17.844s | 120.565x | 745MB |
| wide64_large | 1176.110s | 38.665s | 30.418x | 594MB |

Global geomean **51.537x**, Local geomean **22.927x**, Overall geomean
**39.343x**. `result-integrated.json` is the matching result artifact and the
integrated solver SHA256 is
`5a05741a3dcc0ebd7088a00cf4f6939cabc1e21082d032ee762b069f029652fe`.

Fresh generator seed 913771 with salt `integrated150` was checked through
independent `refsolve`: all six dev families (`road2d`, `lattice3d`,
`local2d`, `scalefree`, `hugeq`, `wide64`) matched exactly under the integrated
dispatch. The combined checkpoint is copied to
`research/checkpoints/certified-integrated-39.34.cpp` with its matching JSON.

## Local-region follow-up

A structurally verified 2-D torus prototype used a canonical torus route as an
upper bound and `minimum_weight * Manhattan(v,target)` as an exact lower-bound
pruning rule. It matched all local2d dev answers (0.28s, including graph
preprocessing), but the large 300,304-vertex / 300,000-query run did not finish
within the existing ALT candidate's runtime and was stopped. The prototype is
kept as `research/prototypes/local_region.cpp` and is rejected for integration;
the current local ALT path remains the certified combined choice.


## Scalefree regression repair

The integrated hub-label dispatch regressed scalefree_large to 52.841 s (36.781x). Historical commit `908a82d` was recovered and its exact relaxed-arc work-balanced bidirectional search restored: directed sparse scale-free graphs now select the side with fewer relaxed arcs, while retaining the exact `min_key(forward)+min_key(reverse) >= best` termination condition. The repaired integrated solver measured scalefree_large at 15.11 s (RSS 52 MB), with exact answers. A fresh seed 24681357 (`scale-repair`) also matched refsolve on 10,000 queries.


Direct post-repair large rerun (same integrated binary, exact comparisons all passed): road2d 35.84 s / 233 MB; lattice3d 47.65 s / 111 MB; local2d 45.15 s / 144 MB; scalefree 22.42 s / 52 MB; hugeq 22.12 s / 763 MB; wide64 46.95 s / 608 MB. The six-way grade wrapper was attempted repeatedly but did not emit a result file in this sandbox.


## Retained-winner integration audit (2026-09-25)

| workload | strongest historical exact path / targeted time | current production path / dispatch | retained optimizations and shadow check | current targeted time |
|---|---|---|---|---:|
| road2d | targeted witness CH + stall, 27.87s | same CH query, directed + coordinates + average arcs >4.5 + Q/V <5 | CH witness/stall present; directed ALT heuristic memoization restored for fallback; no hub-label shadow | 30.31s |
| lattice3d | dominated-edge sparsification + biALT24/4, 42.68s | undirected 24-landmark balanced ALT after sparsification | sparsification and 4-landmark balanced ALT present; relaxed-arc balancing is disabled for undirected low-skew graphs, so no compatible loss | 47.65s |
| local2d | biALT24/4 without groups, 35.64s | undirected 24-landmark balanced ALT after sparsification | same exact balanced ALT; no stronger measured local memoized path exists; no shadow branch | 45.15s |
| scalefree | relaxed-arc work-balanced bidirectional search, 14.39s isolated | directed coordinate-free sparse path; grouped targets first, then work-balanced bidirectional queries | work balancing and threshold >=4 grouping present; hub-label/core code is unreachable from production dispatch | 22.42s |
| hugeq | recursive three-level separator overlay, 40.340s historical | coordinates + Q/V >=5 -> three-level separator and restricted grouped searches | recursive hierarchy and restricted grouping present; no one-level fallback is selected | 22.12s |
| wide64 | biALT24, 41.42s | undirected 24-landmark balanced ALT after sparsification | full 24-landmark path and balanced query constants present; no newer branch shadows it | 46.95s |

All six large outputs matched official answers, and all six dev outputs matched their answer files after restoring the directed ALT heuristic cache. Historical times and current times vary with runner load; source-level path comparison confirms the retained winner composition.

## Fourth-level hugeq separator experiment

Instrumented the retained three-level separator: 37,822 restricted grouped searches answered 72,551 queries and performed about 199.8 million popped vertices. A fourth exact separator level was prototyped by splitting each of the eight existing leaf interiors and storing restricted forward/reverse tables for their crossing hubs (counts 63,62,127,55,64,64,63,62; 520 hubs). The prototype answered all 600,000 hugeq_large queries exactly in 11.68 s / 1.20 GB RSS versus 17.9 s for the instrumented three-level path, a 1.53x targeted gain. Fresh seed 777777 with salt `huge4` matched refsolve exactly on hugeq_dev. The fourth level is now integrated in solver.cpp.

A fifth separator level was prototyped in isolation. It reached 9.5s / 2.0GB but failed exactness on hugeq_large, so it was rejected and not integrated. The exact fourth-level candidate remains production.

A torus-Manhattan lower-bound composition (with topology recognition for low-weight local2d) was tested against balanced ALT. It was exact but slower on both dev workloads (local2d 0.38s, wide64 0.59s), and the pure torus bidirectional path failed to finish wide64_large in the allotted run. Rejected.

Same-microregion restricted grouping plus fourth-level separator-table composition was exact at 9.97s / 1.20GB on hugeq_large (1.19x over fourth-level production). This sub-2x refinement is rejected for now; production remains the 11.89s exact fourth-level solver.

A one-level separator overlay inferred from local2d torus IDs was tested on dev. It was exact but took 6.32s / 1.29GB versus ~0.26s current ALT; rejected before large testing due to preprocessing and memory mechanism.

## Formal hugeq4 full grade

The integrated fourth-level hugeq candidate was formally graded with all
limits enabled and three solver repeats. Every instance returned `ok`:

| workload | T_base | T_solver | speedup | peak RSS |
|---|---:|---:|---:|---:|
| road2d | 975.222s | 28.858s | 33.794x | 227MB |
| lattice3d | 1731.543s | 36.928s | 46.890x | 112MB |
| local2d | 636.906s | 37.633s | 16.924x | 144MB |
| scalefree | 1921.349s | 15.259s | 125.913x | 51MB |
| hugeq | 2236.073s | 11.755s | 190.225x | 1174MB |
| wide64 | 1163.932s | 38.047s | 30.592x | 616MB |

Global geomean is 78.490x, Local geomean is 22.754x, and Overall is
**51.947x**. `result-hugeq4-full.json` and
`research/checkpoints/certified-hugeq4-candidate.{cpp,json}` preserve the
matching source and result.

Same-run scalefree comparison resolved the historical timing gap: commit 908a82d took 17.79s / 52MB, while the current repaired integrated solver took 16.24s / 52MB, both exact. The prior 14s versus 22s measurements were runner variation, not an integration loss.

Disabling ALT for the short-rank, low-weight degree-4 local2d structure was
exact but took 1.21s on dev versus 0.27s current. Plain bidirectional Dijkstra
therefore does not remove enough search work and was rejected without a large
run.

Cell-ordered witnessed CH was exact on local2d_dev but generated 1,638,144
shortcuts, took 27.37s to preprocess, and 28.93s total. Its roughly 1.56s query
phase alone is slower than the complete 0.27s balanced-ALT run, so it was
rejected as an architecture rather than tuned.

An exact local tile certificate combined restricted Dijkstra with a lower
bound on every possible first exit from the tile. On local2d_dev, 32x32 tiles
certified 4,990/10,000 queries but took 0.34s; 16x16 tiles certified 3,177 and
took 0.28s, versus 0.27s current. High certificate coverage did not translate
to reduced work, so no large run was justified.

Full undirected PLL is infeasible on local2d. Degree order reached 69.6m labels
at just 2,000 dev hubs and timed out at 90s/1.75GB. A verified torus
nested-dissection order completed exactly but still required 34.1m logical
labels, 32.66s, and 1.64GB on dev, projecting beyond the useful large-tier
time/memory envelope.

Source-sorted offline execution of the unchanged local biALT path improved dev
by only about 7% (0.28--0.29s to 0.26--0.27s). Rejected as insufficient.

Using all 24 existing landmark tables in every wide64 balanced-potential
evaluation was exact but slowed dev from 0.46s to 0.91s. The retained top-4
query subset is preferable; the stronger bound is dominated by memory traffic.

One-sided top-4 ALT exploits the forward-rank query definition on verified
high-weight 2-D tori. It was exact at 30.31s / 631MB on wide64_large versus
38.047s in the formal hugeq4 grade, and passed fresh seed 888888 plus all six
dev sets after structural integration. The 1.26x gain is retained but is not
large enough by itself to trigger a new full grade.

On the large wide64 graph, a zero-query run measured 6.68s for parsing,
sparsification, and 24 landmark SSSPs. The remaining ~23.6s is query work, so
preprocessing-only acceleration has insufficient upside for a 2x total gain.

Applying one-sided ALT to uniform lattice3d queries slowed dev from 2.02s to
2.31s, so lattice3d retains balanced bidirectional ALT.

An exact 4,001-bucket Dial queue for local2d balanced-ALT reduced keys slowed
dev from 0.29s to 0.33s. The retained radix heap is faster for these short
queries.

Regular-grid exact landmark preprocessing produced the first multi-fold
local2d gain.  On a structurally verified low-weight 2-D torus, 256 full SSSP
tables are rooted at a 16x16 grid; each query uses the four anchors nearest the
target in one-sided exact A*.  local2d_large measured 18.37s / 689MB with all
300,000 answers exact, versus 37.633s in the formal checkpoint (2.05x).
Fresh seed 97531024 / `local-grid256` and all six official dev sets are exact.
The 64-anchor alternative took 21.75s / 300MB, and was rejected.  A hybrid
selector then kept coordinate-nearest anchors for short displacement and
scanned all 256 endpoint differences only for the long tail.  It measured
14.20s / 689MB exact on local2d_large, a 2.65x gain over the formal 37.633s.
This result is targeted, not a new Overall certification.

Lattice3d was tested with 128 farthest-point global landmarks while retaining
the existing exact sparsification and balanced ALT search. It measured 25.21s /
361MB and returned all 30,000 answers exactly (1.46x against the formal
36.928s). Fresh seed 97531025 / `lattice128` and all six dev answer files
passed. Dispatch verifies a cubic vertex count plus every edge's 3-D torus
adjacency before using 128; all other workloads retain existing dispatch.
The combined candidate completed the official full grade with all six `ok`:

| workload | T_base | T_solver | speedup | peak RSS |
|---|---:|---:|---:|---:|
| road2d | 973.155s | 28.861s | 33.718x | 230MB |
| lattice3d | 1682.717s | 23.531s | 71.510x | 353MB |
| local2d | 627.114s | 14.375s | 43.626x | 673MB |
| scalefree | 1843.211s | 14.511s | 127.022x | 53MB |
| hugeq | 2081.328s | 11.428s | 182.119x | 1174MB |
| wide64 | 1146.788s | 28.511s | 40.223x | 632MB |

Global geomean is 86.420x, Local geomean 41.890x, and certified Overall is
**67.886x**. Grade command:
`python3 -u grade.py --solver ./solver --instances instances.txt --json result-local256-lattice128-widefwd-full.json`.
The report enforces 3 solver repeats, 5 baseline repeats, and the 4GiB cap.
Source SHA256 is
`062942028d547557130c899270facfeaf2b0191f7164e51fed6c369022308234`;
JSON SHA256 is
`d5f5d58e87e0d5a870b5ff6155cd2a4f128ee5294aa90398e729a2dc7bd314c7`.
Matching artifacts are preserved in `research/checkpoints/`.

A fifth-level hugeq separator was reconstructed with explicit parent-region
membership and exact boundary fallback. It returned all 600,000 official
answers exactly in 9.98s / 1.89GB, versus the formal level-four 11.428s (1.15x);
the gain is below the 20% research cutoff, so it remains isolated. Combining
24 global and 36 spatial ALT landmarks on wide64 cut dev pops 21% but increased
serial end-to-end dev time from 0.39s to 0.46s; rejected before large testing.

A denser 32x32 local2d landmark layout with exact bounded-radius tables answered
the official large set exactly but took 44.27s / 1.29GB, so the retained full
16x16 grid remains faster at 14.375s / 673MB. On road2d_dev, directed ALT
inside CH failed exactness; a symmetric relaxation variant also failed the
balanced CH stopping condition. Full CH-graph single-sided ALT was exact but
took 3.05s versus 1.53s. Fixed geometric nested-dissection witness CH took
12.65s dev and made 1.36M shortcuts versus 363,811 dynamic-CH shortcuts; a
no-witness customization variant took 29.50s and made 4.47M shortcuts. All
road variants were rejected before large grading.

For road2d, a query-specific geometric potential was derived only from the
observable coordinates and directed edge weights. Reduced-cost CH search was
exact on all road2d_dev queries after using a conservative CH frontier cutoff,
but took 1.84s versus 1.53s for the retained CH path. One-sided geometric A*
was exact but took 4.48s and expanded 39.98M vertices. Both are rejected; the
potential is not strong enough on the variable-speed road network.

An exact tile portal oracle for the regular local2d torus replaced each tile
with its within-tile boundary distance clique and used portal-set A*. A 16x16
dev prototype was exact, 0.73s / 54MB, but scanned 110.25M skeleton arcs for
10k queries, versus 3.87M original arcs in the current 0.94s instrumented dev
run. Its dense portal degree makes large-query work uncompetitive. Adding CH
to the skeleton took 54.63s dev (921k shortcuts). Rejected before large tests.

## 2026-09-26 single-thread follow-up

An exact 8-worker offline query batch matched all six official large answer
files and all six fresh-seed dev answer files (seed 20260926). One-pass large
times were road2d 18.73s, lattice3d 12.01s, local2d 8.13s, scalefree 13.48s,
hugeq 7.62s, and wide64 14.46s; RSS peaked at 1.36GB. A fresh-graph hugeq
separator test also matched 300,000 reference answers. The required grade
rejected all rows as `THREADS` because aggregate CPU exceeded 1.4x wall time.
This candidate is not eligible for production. Source:
`research/prototypes/local_grid_parallel.cpp`; rejected grade:
`result-parallel-batch-rejected-threads.json`.

Three single-thread architecture probes were rejected before large testing:
target-aware road CH priority increased road2d_dev shortcuts 13% (363,811 to
410,477) and time 1.59s to 2.05s; four-edge strict-weight detours removed no
extra arcs on wide64_dev (still 9.574%, preprocessing 0.007s versus 0.005s);
and coordinate-nearest local-grid anchors for every local2d_dev query were
exact but indistinguishable in time (0.95s versus 0.96s), while discarding the
stronger endpoint-selected landmarks for long queries.

The last formally certified source remains SHA256
`062942028d547557130c899270facfeaf2b0191f7164e51fed6c369022308234`; the
working `solver.cpp` additionally contains ungraded exact lattice
work-balancing and directed road sparsification. No new full-grade result was
produced.

## Fail-fast architecture tests (2026-09-26)

Exact 2/3-edge directed sparsification was composed with the hugeq recursive
separator path on a fresh generated graph, using a forced 300,000-query
separator batch. It removed 53,976 of 271,129 arcs (19.908%) and answered all
queries exactly, but runtime improved only from 6.54s to 5.29s (1.24x); the
prototype remains isolated in `research/prototypes/hugeq_sparse_separator.cpp`.

A nested-dissection CH for verified local2d torus structure returned exact
answers on dev but was rejected before large testing. Full contraction created
1.83M shortcuts on 50,176 vertices and took 27.82s. Partial contraction of 25%
reduced fill to 480,072 shortcuts but still took 9.54s. Prototype:
`research/prototypes/local_nested_ch.cpp`.

Two wide64 alternatives were exact but slower on wide64_dev: work-balanced
bidirectional ALT took 0.44s versus 0.38s for the retained forward four-table
ALT, and full 24-landmark one-sided ALT took 0.51s. They were rejected before
large testing; prototypes are `research/prototypes/wide64_workbalanced.cpp`
and `research/prototypes/wide64_full24_alt.cpp`.

On the first 251,000 official hugeq_large pairs (separator dispatch, Q/V=5.002),
same-leaf reverse searches grouped by target took 8.94s / 1.19GB and settled
29.34M vertices; forward searches grouped by source took 9.12s / 1.19GB and
settled 29.71M. Both were exact; source grouping was rejected. A checked
64-to-32-bit separator-table conversion was exact on a fresh forced 300k
separator batch but raised total time from 6.54s to 6.94s, so it was rejected.

Multi-goal ALT was then added only to the restricted reverse searches for
same-target, same-leaf fallback groups. It builds 12 directed landmark tables
and uses a consistent lower bound to the nearest requested source; every
endpoint is still settled at its exact distance. On the 251k official prefix,
group-search work fell from 29.34M pops / 158.92M arcs to 7.00M / 37.97M and
time from 8.94s to 7.65s. On all 600k official hugeq_large queries, the
prototype was exact in 10.10s / 1.21GB against 12.58s / 1.20GB baseline; the
integrated source recheck was exact in 10.92s / 1.21GB. A fresh-seed forced
separator batch and all six official dev answer sets were exact after
integration. Current `solver.cpp` SHA256 is
`e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`. Formal
Overall remains 67.886x; no full grade was run.

Extending the same multi-goal ALT idea to scale-free repeated-target groups
was exact on all scalefree_dev answers after correcting a direction-term bug,
but took 29.01s / 25MB versus 0.46s / 13MB retained. It settled 2.43M grouped
vertices and scanned 7.13M arcs; landmark construction and group heuristic
cost dominate. Rejected before fresh-seed or large testing. Prototype:
`research/prototypes/scalefree_multigoal_alt.cpp`.

## Exact work-balanced lattice ALT

Using cumulative relaxed-arc work to choose the next frontier on the verified
cubic 3D torus retained the exact ALT stopping bound and passed official and
fresh-seed dev answer comparisons. On lattice3d_dev, work fell from 9.46M pops
/ 32.41M arc scans to 5.29M / 18.32M, and runtime from 1.69s to 1.27s. On a
fresh seed, the candidate was exact at 5.38M pops / 18.61M scans. On
lattice3d_large, it was exact at 17.46s / 362MB versus 24.96s / 362MB for the
instrumented key-balanced path; work fell from 120.46M / 410.61M to 68.15M /
234.28M. Integrated source is `research/checkpoints/candidate-lattice-workbalanced.cpp`
(SHA256 `06c286d118ac3b21b8d0710debce348ca67b6b51c3e056d97137cefce8c5d28a`).
It is a 1.43x single-workload candidate; the existing formal Overall remains
67.886x pending additional accumulated wins.

Applying cumulative stall-plus-relaxation arc-work balancing to road CH kept
the exact conservative min-key cutoff and matched road2d_dev answers, but
increased settled nodes from 1.06M to 1.13M and scanned arcs from 13.70M to
14.45M; runtime was 1.62s versus 1.59s. Rejected before large testing.

Classic edge-difference priority was tested for road CH. It remained exact on
road2d_dev but produced 366,946 shortcuts versus 363,811 for the retained
priority and took 1.60s versus 1.59s. Rejected before large testing.

Local2d A* tie-breaking toward larger g inside equal-f shells remained exact,
but settled 1.065M nodes / 3.844M arcs versus the retained path's roughly
1.06M / 3.87M; dev time increased from 0.96s to 1.01s with the binary heap.
Rejected before large testing.

## Directed road exact sparsification

For structurally recognized directed coordinate road graphs, remove an arc
only when a directed 2- or 3-edge replacement has total weight no greater and
each replacement arc is strictly lighter. By induction on edge weight, these
simultaneous removals preserve all pairwise distances. On road2d_dev, 19.8% of
arcs and about 11% of CH shortcuts were removed; exact runtime fell 1.59s to
1.09s. A fresh-seed dev graph was exact with 19.9% removed. road2d_large was
exact at 23.49s / 213MB, removing 17.5% of arcs and lowering CH shortcuts
2.765M to 2.489M (certified baseline 28.86s / 227MB).

The working solver now combines this with exact work-balanced ALT on
lattice3d; SHA256 is
`132c5ed50923ce6fb49543fc2a8f02d50f492c73f33939c9a614f680d3998fd7`.
All six dev answers pass. The formal 67.886x checkpoint remains authoritative;
no full grade has been run for this combined candidate.

The directed 4-edge extension added only 0.17 percentage points of removals
on road2d_dev (19.812% to 19.979%) and slowed end-to-end time from 1.09s to
1.11s; rejected before large testing. A 4-edge strict-lower-weight extension
to lattice3d found no additional removed arcs. The scale-free 64-hub upper
bound was exact on dev but took 0.84s total (0.369s hub preprocessing) versus
0.45s retained, so it was rejected before large testing.

An exact CH upper bound from 16 high-degree road intersections remained exact
on road2d_dev but added 0.132s preprocessing and increased total time from
1.09s to 1.23s; rejected before large testing. Extending directed road
sparsification to four-edge witnesses raised removals only 19.812% to 19.979%,
lowered shortcuts by 554, and slowed total time from 1.09s to 1.11s; rejected.

Work-balanced CH on the sparsified road graph also matched road2d_dev answers,
but took 1.11s versus 1.09s; it settled 937k CH nodes and scanned 10.02M
arcs. Rejected before large testing.

The same directed 2/3-edge strict-weight sparsification on scalefree_dev was
exact but removed only 31/145,587 arcs (0.021%) and took 0.46s versus 0.45s;
rejected before large testing.

Caching a complete reverse-upward CH search for each repeated road target and
reusing it across source queries was exact on road2d_dev, but repeated-target
groups covered only 233/10,000 queries (37 targets). Shared work was 46.5k
settled nodes / 341k arcs; overall runtime stayed 1.09s. Rejected before large
testing.

## Additional architecture trials (2026-09-26)

The local2d 4x4-block coarse relaxed metric was exact on dev but left work at
1.061M settled vertices / 3.86M scanned arcs and increased time from 1.01s to
1.64s; rejected before large testing. Dynamic-priority witness CH on verified
2D torus structure was also exact but generated 367,998 shortcuts on 50,176
vertices, scanned 28.71M query arcs, and took 1.46s / 33MB versus 1.01s ALT.
Both prototypes remain isolated.

wide64's 2x2-cell coarse ALT was exact but reduced dev work only 0.6% (3.379M
to 3.358M pops; 12.228M to 12.155M arcs) and increased time 0.37s to 0.50s.
The hugeq ALT equality certificate matched only 50 of the first 251k queries
and changed time 7.65s to 7.62s. Both are rejected before large testing.

The same-target local2d ALT batch used consistent interval bounds from four
exact spatial landmark tables and matched local2d_dev answers. It took 1.33s
versus 0.97s for retained A*, with 4.05M grouped pops / 14.88M scanned arcs.
On local2d_large, after 5.41s of landmark preprocessing, grouped search passed
90s without finishing and was interrupted. This is slower than the retained
14.4s candidate and is rejected. Prototype:
`research/prototypes/local_target_group_alt.cpp`.

Wide64 one-sided ALT was compared with work-balanced bidirectional ALT using
the same exact min-key termination bound. The bidirectional candidate matched
all wide64_dev answers, settling 3.07M nodes / scanning 11.12M arcs, but took
0.53s versus 0.37s for production and 0.46s for ordinary biALT. Rejected
before large testing. Prototype:
`research/prototypes/wide64_workbalanced_alt.cpp`.

Wide64 exact 5-edge replacement-path sparsification remained exact on dev,
but increased removed arcs from 9.574% to only 9.753% (+358 directed arcs)
and changed runtime from 0.36s to 0.37s. Rejected before large testing;
prototype `research/prototypes/wide64_sparsify5.cpp`.

Compressing local2d's exact 256-grid landmark tables to uint32 under a proven
simple-path distance bound remained exact on dev, reduced peak RSS 113MB to
63MB, and changed runtime 0.92s to 0.88s. Rejected without large testing as
below the useful speed threshold; prototype `research/prototypes/local_grid_u32.cpp`.

Road CH target batching at multiplicity >=2 matched all road2d_large answers,
but took 36.54s / 213MB and scanned 82.10M nodes / 1.024B arcs. This is slower
than the retained sparse-CH path (23.49s); rejected. Prototype:
`research/prototypes/road_ch_target_batch2.cpp`.

The formal Overall is the geometric mean of the six per-instance speedups.
Combining targeted road2d 23.49s, lattice3d 17.46s, and hugeq 10.92s with
the other four formally graded times projects to 74.40x Overall; this is not a
formal result. The ratio improves the six-instance product by 1.733x but the
geomean by only 9.6%, so no grade was run for this candidate.

Raising road CH witness depth from 200 to 2,000 settled nodes remained exact
on road2d_dev but removed only one additional shortcut (322,816 to 322,815)
and slowed the run 1.05s to 1.06s. Rejected; prototype
`research/prototypes/road_witness2000.cpp`.

An exact relaxed 3-D block metric (2,197 coarse cells, 36.8MB all-pairs table)
composed with lattice3d work-balanced biALT matched dev answers, but left
search work unchanged at 5.286M pops / 18.314M arcs and raised runtime from
1.18s to 1.60s (RSS 66MB to 104MB). Rejected before large testing; prototype
`research/prototypes/lattice_coarse_exact_alt.cpp`.

Local2d canonical torus routes were tested as feasible upper bounds against
the four-anchor ALT lower bound. Exact dev answers matched, but no query had
an equality certificate; runtime was 0.90s versus 0.92s baseline with unchanged
search. Rejected before large testing; prototype
`research/prototypes/local_canonical_cert.cpp`.

Lattice3d additive cost decomposition (per-interface minimum baseline plus
residual-edge ALT) was exact on dev but expanded 22.82M nodes / 78.14M arcs,
versus 5.29M / 18.32M retained. Runtime was 4.39s versus 1.21s, including
0.39s preprocessing. Rejected before large testing; prototype
`research/prototypes/lattice_additive_metric.cpp`.

Wide64 2x2-block interface-baseline plus residual ALT was exact on dev, but
expanded 11.69M nodes / 42.22M arcs versus 3.38M / 12.23M production, and
raised time 0.36s to 1.33s including 0.124s preprocessing. Rejected before
large testing; prototype `research/prototypes/wide64_additive_metric.cpp`.

## Official full-grade checkpoint: integrated e0527341

The combined candidate was graded with
`python3 -u grade.py --solver ./solver --instances instances.txt --json result-integrated-e052-full.json`
after compiling the matching `solver.cpp` using `-O2 -std=c++17`. The grade
used three solver repeats, five baseline repeats, and enforced the 4 GiB cap.
All six large answer files were exact (`ok`):

| instance | T_base | T_solver | speedup | peak RSS |
|---|---:|---:|---:|---:|
| road2d_large | 977.890s | 22.046s | 44.356x | 208.0MB |
| lattice3d_large | 1843.036s | 16.678s | 110.509x | 353.4MB |
| local2d_large | 659.271s | 15.275s | 43.160x | 672.5MB |
| scalefree_large | 2043.978s | 15.999s | 127.756x | 53.2MB |
| hugeq_large | 2215.767s | 9.992s | 221.749x | 1184.4MB |
| wide64_large | 1215.145s | 29.692s | 40.925x | 631.9MB |

Global geomean is 108.555x, Local geomean 42.027x, and Overall is
**79.11815862212096x**. The source digest is
`e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`; matching
source and JSON are preserved as
`research/checkpoints/certified-integrated-e0527341.{cpp,json}`. The JSON
digest is `0c69474470bd5022d213a1cfef0866ed869de6d637b47f5b28564789da16c768`.
This formally improves the previous 67.886x checkpoint but remains below the
150x target.

## Resumable road-target frontier rejection

An exact lazy backward CH frontier shared across repeated targets matched all
road2d_dev answers and a fresh seed (603426, salt `shared-target-test`). It
also matched all road2d_large answers, but ran in 26.41s / 213MB versus the
certified 22.046s / 208MB. The isolated candidate is
`research/prototypes/road_shared_target_ch.cpp`; production remains unchanged.

As an upper-bound check for local2d tile certificates, the standalone
32x32-tile restricted-search probe on local2d_large found 152,255 same-tile
queries and 149,642 whose restricted path equaled the official global answer
(49.88% of the batch). It settled 26.96M nodes in 3.37s, but this reads the
answer file only for research measurement; it is not a legal solver path. A
production certificate would still need an admissible lower bound and could
only avoid search on about half the batch, so the all-pairs-table extension
was rejected before implementation.

For hugeq, a forward multi-goal ALT search for repeated source queries was
compared with current reverse target grouping on a fresh 100,000-query
balanced-source batch (seed 603427, salt `hugeq-source-alt`). Both forced
separator paths were exact. Source grouping took 4.94s / 1.056GB and 2.059M
pops / 8.814M arcs, versus 5.04s / 1.056GB and 2.111M / 9.043M for target
grouping. The source candidate also matched all hugeq_dev answers under forced
separator dispatch. A roughly 2% gain is not a credible multi-fold route, so
the prototype remains isolated and was not run on large.

Directed ALT potentials were composed with road CH. The 12-term potential
was exact on six dev sets, a fresh seed, and road2d_large, but large took
28.09s / 273MB versus 22.046s for production. A fixed strongest-four subset
(`research/prototypes/road_ch_bialt4.cpp`) remained exact on all those checks
and took 21.15s / 273MB on road2d_large. This is a 4.2% road-only gain and
projects to only 79.65x Overall if the other five rows are held fixed; no
full grade was run, and the candidate remains isolated.

A balanced two-ended directed ALT potential on CH was exact on six dev sets,
fresh road seed 603429, and road2d_large. It reduced large search work to
14.94M pops / 264.19M arcs but took 21.42s / 273MB, slightly slower than the
strongest-four unbalanced potential at 21.15s. It remains isolated in
`research/prototypes/road_ch_balanced_alt.cpp`.

For local2d, transposing its 256 exact grid-landmark tables from
landmark-major to vertex-major was hypothesized to reduce four scattered ALT
loads per heuristic to one contiguous cache line. The isolated
`research/prototypes/local_grid_vertex_major.cpp` remained exact on local2d_dev
and local2d_large, but measured 1.14s / 113MB on dev versus 0.94s production,
and 24.16s / 673MB on large versus 14.18s on the same-source production
binary (formal row 15.275s). Preprocessing increased 5.04s to 6.03s. The
candidate regressed substantially and was rejected; `solver.cpp` and the
certified checkpoint were not changed.

A 2,001-slot circular Dial queue was then tested for local2d A*, using the
consistent ALT key's maximum per-edge increment of twice the observed maximum
edge weight (2,000). It matched local2d_dev exactly and measured 0.89s versus
0.94s production, settling 1.159M nodes and scanning 3.831M arcs; preprocessing
was 0.77s. The total gain is only about 1.06x, so it was rejected before a
large run. Source: `research/prototypes/local_grid_dial.cpp`.

For local2d's short-displacement queries, expanding the nearest-anchor set
from 2x2 to 4x4 was exact on dev and reduced search from 1.06M / 3.87M to
916k / 3.09M pops/arcs. Total dev runtime improved only from 0.94s to 0.88s
because heuristic cost quadrupled. This is not a multi-fold route, so the
16-anchor prototype `research/prototypes/local_grid_16anchors.cpp` was not
tested on large and was rejected.

An offline query-aware wide64 landmark placement was screened against the
large query batch before implementation. The 30,000 sources are all unique,
29,775 targets are unique, and the most frequent 32x32 torus source cell holds
only 0.15% of queries; torus-Manhattan displacement p50/p90/p99 is 188/630/799
on side 1,414. This broad long-range mix offers no endpoint concentration for
query-weighted roots to exploit, so the scheme was rejected before coding.

For a distinct wide64 hypothesis, each edge weight was decomposed into global
minimum `m` plus residual `w-m`; `m * torus-Manhattan + residual ALT` is an
admissible consistent lower bound. The exact dev prototype
`research/prototypes/wide64_residual_alt.cpp` was slower: 0.59s, 4.684M pops,
16.943M arcs, 21MB RSS versus about 0.38s, 3.38M pops, 12.23M arcs for
production. It was rejected before fresh/large testing; production stayed
unchanged.

Screened a mixed road2d CH / cached-hot-target path using actual target
multiplicities. In road2d_large, just 2,424/200,000 queries (1.21%) have a
target repeated at least 32 times; only 425 (0.21%) are in groups of at least
64. road2d_dev has no group of size 32. Even free answers for all hot groups
cannot yield a multi-fold road gain, so the mixed target cache was rejected
before implementation.

Tested one-landmark directed ALT on scalefree while preserving work-balanced
bidirectional expansion and the exact reduced-key termination rule. It matched
all scalefree_dev answers but took 0.50s / 15MB versus 0.46s / 13MB production;
the non-grouped portion settled 2.680M nodes and scanned 12.844M arcs. Since
the dev result regressed, it was not run on fresh or large instances. Prototype:
`research/prototypes/scalefree_balanced_alt1.cpp`.

The 64-highest-degree hub upper bound equaled the exact answer for 8,155/10,000
scalefree_dev queries, but production target grouping had already handled
3,010. Among the remaining 6,990 queries, directed ALT lower bounds certified
only 60 with 64 hubs (0.86%), 12 with 16 hubs, and two with four hubs. Thus the
raw hub hit rate is redundant with current grouped search and gives no useful
additional direct answers. The diagnostic prototype matched all dev answers;
no fresh/large run or production change was warranted.
Reproduction: compile `research/prototypes/scalefree_hub_hit_probe.cpp` with
`g++ -O2 -std=c++17` and run it on `instances/scalefree_dev.{graph,queries}`;
the 64-table diagnostic took 0.80s / 65MB including 0.33s preprocessing,
versus 0.45s / 13MB production.

Tested a CH ordering that retains nodes on the fastest normalized-cost road
corridors in the core. It classified the bottom 1/32 of weight/Euclidean
ratios and prioritized vertices incident to multiple such arcs. Outputs were
exact on road2d_dev and road2d_large. Shortcuts fell from 322,816 to 296,475
on dev and about 2.49M to 2.095M on large; runtimes were 1.13s vs 1.04s dev
and 20.84s / 211MB vs formal 22.046s / 208MB large. This 5.5% road-only gain
projects to about 79.86x Overall, so it was rejected before fresh-seed/full
grade. Prototype: `research/prototypes/road_fast_corridor_ch.cpp`.

Screened SCC-condensation reachability on scalefree for cheap exact rejection
of unreachable queries. The iterative SCC probe found 9,005 components on
large, including a 290,996-node giant SCC; all 1,559 unreachable large queries
come from sink-component sources, which the retained bidirectional search
rejects as soon as its forward frontier empties. Topological ordering would
reject 1,529. Dev shows the same result (289 unreachable, all sink sources).
Preprocessing took 0.10s / 44.6MB large and 0.01s / 14.1MB dev, so this adds
cost without addressing expensive reachable searches. Rejected before solver
integration. Prototype: `research/prototypes/scalefree_scc_probe.cpp`.

An undirected local2d separator-label prototype reused the exact multilevel
separator query path after deriving coordinates from verified torus topology.
It matched all local2d_dev answers but took 6.40s / 1,255 MiB, versus 0.94s for
the retained solver. Its full-graph tables per recursive region project beyond
the 4 GiB memory cap at the 548x548 large tier. Rejected before fresh-seed or
large testing; the temporary source was `/tmp/local_separator_test.cpp`.

The local2d 256-anchor grid was then replaced by 64 exact distance-to-set
tables, each built by multi-source Dijkstra from a nearby 2x2 anchor group.
Because distance to a set is 1-Lipschitz, max absolute table differences are
consistent admissible potentials. The candidate was exact at 0.34s / 38MB on
dev, versus 0.94s / 113MB retained, but took 19.86s / 234MB on large versus
15.275s formal. A 64-single-anchor alternative selected the strongest four
per query; it was exact at 0.34s dev and 18.33s / 234MB large. Both are
rejected. Temporary sources: `/tmp/local_grouped_anchors.cpp` and
`/tmp/local_64single.cpp`.

A max-weight-1,000 circular Dial queue for the 256 local-grid landmark SSSPs
reduced dev preprocessing from about 0.77s to 0.686s and total time from
0.94s to 0.78s, but affects only preprocessing and has insufficient projected
overall upside. Selecting the best four among the 16 nearby anchors was exact
at 0.91s dev; no large run was warranted. Temporary probes:
`/tmp/local_dial_pre.cpp` and `/tmp/local_select16.cpp`.

An exact large counter run for wide64's forward ALT settled 213.08M vertices
and scanned 770.84M arcs across 30,000 queries, taking 28.83s / 647MB. The
existing relaxed-arc-balanced bidirectional ALT was rechecked because the
large rank range doubles the dev maximum; it matched all official answers but
took 36.06s / 647MB despite reducing work to 190.75M pops / 690.14M arcs.
The one-sided path remains selected.

A proposed tighter road-CH cutoff using `min_forward + min_backward >= best`
was incorrect on road2d_dev at query 2 (expected 20,469,040, produced
22,413,649); disabling stall-on-demand did not fix it. The cutoff is not
valid for this restricted CH query. Updating the incumbent as soon as one
relaxation touches an opposite-side-labeled vertex remained exact on dev but
slowed total dev time from about 1.04s to 1.08s. Neither change is integrated.

The exact 16-high-degree-hub route upper bound was rechecked on the complete
road2d_large batch after its dev-only result. It matched all 200,000 answers,
but took 22.71s / 288MB (0.892s hub preprocessing, 36.32M CH pops / 541.21M
arcs) versus 22.046s / 208MB formal. Rejected; no production source change.

Wide64 landmark distances were then rounded downward by a fixed scale before
landmark construction. Multiplying the resulting ALT potential by the scale
preserves admissibility and consistency; verified torus topology bounds all
distances to a `uint32_t` table. The exact prototype matched wide64_dev and
wide64_large, but large took 30.93s / 493MB at scale 465,521, 31.80s / 493MB
at the tighter torus-bound scale 330, and 31.49s / 492MB at scale 100,000.
The certified path took 29.692s / 632MB. It is a memory improvement with a
runtime regression and is rejected. Reproducible source:
`research/prototypes/wide_scaled_alt.cpp`.
Using the formal wide64 baseline time (1,215.145s), the 31.49s candidate row
would score 38.588x and project to only 78.35x Overall with the other five
formal rows held fixed, so no full grade was justified.

Caching each wide64 query's four-landmark heuristic by vertex/epoch was exact
on wide64_dev but measured 0.37s / 23MB versus about 0.38s / 19MB retained.
This is within timing noise with a memory increase, so it was rejected before
large testing; isolated source: `/tmp/wide64_hcache.cpp`.


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

A repaired temporary prototype computed exact reverse multi-source distances to each active 32x32 coordinate-rank cell and used the values as a CH query potential. The initial `minF+minB >= best-h(s)` termination produced 2,373/10,000 wrong road2d_dev answers, including with stall disabled; the no-early-stop run was exact. Replacing it with the conservative condition that both reduced minima individually reach `best-h(s)` matched official road2d_dev and fresh seed 20260927 / salt `road-region-alt-repair`. The prototype also passed all 200,000 road2d_large answers, but took 26.99s / 951.5MiB versus the certified 22.046s / 208MiB. Profile generation alone was 8.783s and its 324 tables occupied 742.3MiB. Query arc scans fell to 162.49M, but the full runtime regressed 22%; reject and leave `solver.cpp` at SHA256 `e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`.

The preceding status 127 was caused by attempting to time a nonexistent `/tmp/road_ch_region_alt` binary after its compile failed: the old `ch_query` still called removed `road_region_arc_may_be_shortest`. The compile error was repaired in a `/tmp` copy, and all subsequent Python commands use `python3`.


### Lattice3d full separator-table feasibility screen (2026-09-26)

A recursive exact 3-D separator oracle was screened before implementation. The authoritative lattice3d_large header is 300,763 vertices (side 67), not the 32k scale used in an initial rough estimate. A torus median cut crosses two 67x67 planes, yielding about 8,978 boundary endpoints; one full-graph uint32 distance row per endpoint would require roughly 10.0 GiB (the exact table estimate is 8,978 * 300,763 * 4 bytes, about 10.0 GiB / 10.7 GB), before graph state. This already exceeds the enforced 4 GiB cap at the root level, and root-level per-hub SSSP preprocessing would also be prohibitive. Reject this full separator-table architecture before coding; no solver or candidate source changed.


### Local2d target-local-anchor balanced bidirectional ALT (2026-09-26)

The current local2d path uses four target-near members of the retained 256 exact grid-landmark tables in one-sided A*. This prototype selected the same four tables, formed the exact doubled feasible potential `p(v)=h_t(v)-h_s(v)`, and searched both directions with the exact reduced-key termination `minF+minB >= 2*best+p(t)-p(s)`. It compiled from the certified `solver.cpp` as `/tmp/local_grid_bialt_anchors.cpp`, matched all 10,000 local2d_dev answers, and reduced query work to 877,130 settled nodes / 3.19M scans. The certified one-sided path took 0.89s end-to-end; this variant took 0.91s with the same 0.80s landmark preprocessing and 113MiB RSS. The 18% query-work reduction is erased by two-frontier overhead, so reject before fresh/large testing. Production remains at e052 SHA.


### Local2d target-strip relaxed-metric ALT screen (2026-09-26)

A new exact lower bound was tested for local-rank queries. For each active target x/y strip of width 8, the prototype builds a multi-source SSSP in a relaxed torus metric: retain actual weights along one axis and replace orthogonal edges by the graph-wide minimum weight. Distances to the target strip are admissible and consistent; the maximum of x-strip, y-strip, and the existing four-anchor ALT bounds is also admissible and consistent. On local2d_dev, 28 x-strips and 28 y-strips added 21.4MiB and 0.139s preprocessing. The candidate compiled from certified `solver.cpp`, matched all dev answers, but search work was unchanged at 1,061,535 pops / 3.828M scans; total time rose to 1.02s versus 0.89s certified. The regional relaxation is dominated by the existing anchor ALT, so reject without fresh/large testing. Production remains unchanged at e052.


### Local2d exact target-strip distance oracle screen (2026-09-26)

A second strip variant replaced the 256 single-vertex landmark SSSPs with exact multi-source distances to active width-8 target columns and rows in the original graph. The query heuristic is `max(dist(v,target-column), dist(v,target-row))`, which is a consistent lower bound for every target in that column/row; it avoids per-query landmark selection and cuts the table build from 256 SSSPs to 56 on dev / 138 on large. This prototype matched all 10,000 local2d_dev answers and reduced total dev time from 0.89s to 0.55s, with 21.4MiB tables and 0.167s preprocessing. However, query search grew from the retained 1.06M pops / 3.83M arcs to 5.53M / 20.24M; the measured dev query phase itself rose from roughly 0.09s to 0.38s. The apparent total win comes from the smaller dev preprocessing workload and does not scale to 300,000 queries; reject before large testing. Production remains e052.


### Local2d tile-feasible-upper-bound screen (2026-09-26)

Command: `g++ -O2 -std=c++17 /tmp/local_grid_tile_bound.cpp -o /tmp/local_grid_tile_bound`; then `/usr/bin/time -v /tmp/local_grid_tile_bound instances/local2d_dev.graph instances/local2d_dev.queries /tmp/local_grid_tile_bound_dev.out`. The output was byte-compared with `instances/local2d_dev.answers` and exact. The same-32x32-tile restricted search attempted 5,151 queries, certified 1,965/10,000, and used 921,108 settled states / 3,362,068 arc scans. It took 1.06s / 108MiB including 0.845s landmark preprocessing. A fresh compile/run of e052 source took 0.94s / 108MiB including 0.831s preprocessing. Reject: the extra restricted searches exceed the cost of the queries certified. The e052 official full result remains six `ok`, Overall 79.11815862212096x; this candidate has no measured or projected Overall improvement.


### Local2d repeated-target grouped Dijkstra screen (2026-09-26)

The large query batch contains 190,709 queries in 79,344 target groups of size >=2, suggesting amortized reverse search. Command: create `/tmp/local_target_group.cpp` by enabling `grouped(kv.second,false)` for local-grid target groups >=2 and adding settlement/scan counters; compile with `g++ -O2 -std=c++17 /tmp/local_target_group.cpp -o /tmp/local_target_group`; run `/usr/bin/time -v /tmp/local_target_group instances/local2d_dev.graph instances/local2d_dev.queries /tmp/local_target_group_dev.out`. Official dev output comparison was exact. On dev, grouped queries alone settled 4,994,984 nodes / scanned 18,366,690 arcs, and total time was 1.26s / 108MiB versus 0.94s / 108MiB retained. Since the grouped search work exceeds total retained dev search work several times over for a subset, reject before large/fresh testing. Formal Overall remains 79.11815862212096x; production e052 unchanged.


### Local2d block-cut relaxed ALT screen (2026-09-26)

Command: compile `/tmp/local_coarse_tile_alt.cpp` with `g++ -O2 -std=c++17`, then run `/usr/bin/time -v /tmp/local_coarse_tile_alt instances/local2d_dev.graph instances/local2d_dev.queries /tmp/local_coarse_tile_alt_dev.out`; output matched `instances/local2d_dev.answers`. The 8x8-block graph contained 784 cells / 23,010 crossing arcs and preprocessed in 0.066s. The potential exceeded the existing ALT source bound on 0/9,990 non-self queries; search work was unchanged (1,062,244 pops / 3,830,190 scans). Total was 1.01s / 115MiB versus 0.94s / 110MiB checkpoint. Reject before large testing. Formal Overall remains 79.11815862212096x and production `solver.cpp` hash remains e0527341.


### Wide64 median-clipped ALT screen (2026-09-26)

Command: compile temporary `/tmp/wide64_clamped_alt.cpp` with `g++ -O2 -std=c++17`; run `/usr/bin/time -v /tmp/wide64_clamped_alt instances/wide64_dev.graph instances/wide64_dev.queries /tmp/wide64_clamped_dev.out`; compare against `instances/wide64_dev.answers` (exact). Median edge cap was 313,668,693 and 24 relaxed SSSP tables took 0.089s. Only 135/9,990 non-self query source potentials exceeded existing ALT. Settled work was 3.264M pops / 11.776M scans versus retained 3.379M / 12.228M; end-to-end 0.53s / 36MiB versus 0.37s / 23MiB. Reject before large testing; no projected Overall improvement. The six-large checkpoint remains 79.11815862212096x at source SHA e0527341.


### Lattice3d one-sided ALT4 screen (2026-09-26)

Command: compile `/tmp/lattice_forward4.cpp` with `g++ -O2 -std=c++17`, then run `/usr/bin/time -v /tmp/lattice_forward4 instances/lattice3d_dev.graph instances/lattice3d_dev.queries /tmp/lattice_forward4_dev.out`; byte-compare against `instances/lattice3d_dev.answers` (exact). The one-sided candidate settled 15.22M nodes / scanned 51.66M arcs and took 1.83s / 65MiB, versus 1.30s / 65MiB for a fresh e052 compile. Reject before large testing. Formal Overall remains 79.11815862212096x; production source hash unchanged.


### Road corridor-level CH ordering (2026-09-26; integrated targeted candidate)

The temporary `/tmp/road_corridor_hierarchy.cpp` candidate recognizes a
directed coordinate grid with verified neighboring topology, classifies
faster row/column corridors from normalized edge-weight medians, and delays
their contraction in the exact witness/stall CH preprocessing. On road2d_dev
it produced 224,335 shortcuts versus 322,816 in the checkpoint, with the same
19.812% exact sparsification; official dev answers were exact. A fresh seed
(20260928, `road-hierarchy`) matched refsolve exactly and produced 224,779
shortcuts. The candidate all-six dev grade returned `ok` for every workload.

Road2d_large was run twice in the prototype and both outputs byte-matched
official answers: 17.83s / 202,892 KiB and 17.86s / 202,956 KiB. After
integration, `solver.cpp` was byte-identical to the prototype source, compiled
with `g++ -O2 -std=c++17`, and again matched all 200,000 answers at 18.36s /
202,888 KiB with 1,940,888 shortcuts. The e052 formal row is 22.046s / 218MiB,
so this is ~1.20x targeted. Holding the other five official rows fixed
projects to ~81.6x Overall, not a new grade result. The certified rollback
source remains `research/checkpoints/certified-integrated-e0527341.cpp` at
SHA256 `e0527341ff04d467dd68de78361624813019b350a3c980c823039005925967ca`;
its formal matching result `result-integrated-e052-full.json` is 79.118x. The
working candidate source is SHA256
`af27699a5416968ba9a4d840e87822c01eaa27b170822568f2ac82a5af4943d3`.

The candidate all-six dev grade returned `ok` for every workload and a fresh
road2d_dev seed matched refsolve exactly. A redundant second dev grade on the
byte-identical integrated source was manually interrupted after 130 seconds
without producing JSON; the previous identical-source dev JSON remains the
validation record. No full six-large grade was run for this single-row change.


### Wide64 ALT landmark-route incumbent screen (2026-09-26)

The temporary `/tmp/wide64_landmark_upper.cpp` initializes a feasible route
bound from the minimum `d(s,L)+d(L,t)` over the 24 exact landmark tables and
terminates when the minimum open ALT key reaches that bound. The four selected
ALT landmark differences stay fixed per query, so the potential remains
consistent; the incumbent itself is a real route. It compiled with
`g++ -O2 -std=c++17`, matched all wide64_dev answers, but certified only
78/9,990 non-self queries. It took 0.42s / 22.4MiB and popped 4.02M heap
entries, versus about 0.37s / 23MiB retained. The route bound is rarely tight
and adds overhead; reject before fresh/large testing. Formal Overall is still
79.11815862212096x, while the road candidate projects to ~81.6x only.


### Wide64 reverse-oriented ALT4 screen (2026-09-26)

`/tmp/wide64_reverse_alt4.cpp` reverses the source/target orientation of the
one-sided exact ALT4 search on the verified undirected wide64 torus. The fixed
landmark potential remains consistent and the candidate byte-matched all
wide64_dev answers. It took 0.40s / 22.4MiB versus ~0.37s / 23MiB retained,
so endpoint orientation shows no useful dev benefit; reject before fresh/large
testing. No change to the working solver. The formal Overall stays
79.11815862212096x, with the road candidate only projected at ~81.6x.


### Wide64 contraction-hierarchy screen (2026-09-26)

The temporary `/tmp/wide64_ch.cpp` runs the exact dynamic witness/stall CH
preprocessing and query engine on the verified high-weight 2-D torus after
exact sparsification. It compiled with `g++ -O2 -std=c++17`, generated
385,979 shortcuts on wide64_dev, and byte-matched all answers. End-to-end was
2.20s / 32MiB versus ~0.37s / 23MiB retained. Preprocessing dominates even
with one third of the large query count; at 2M vertices it cannot amortize
against the 30s target. Reject before large/fresh testing. No source change.


### Local2d reverse-oriented grid ALT screen (2026-09-26)

The temporary `/tmp/local_grid_reverse.cpp` swaps each local-grid ALT query's
source and target and selects anchors near the original source. It compiled
with `g++ -O2 -std=c++17` and byte-matched every local2d_dev answer, but took
0.99s / 113MiB (including 0.883s landmark preprocessing) versus ~0.94s /
108MiB retained. Reject before fresh/large testing; no production change.


### Wide64 coordinate-monotone route upper-bound screen (2026-09-26)

The temporary `/tmp/wide64_torus_route_ub.cpp` combines existing exact ALT
with the best feasible route among eight coordinate-monotone torus paths.
Original edge weights are accumulated before sparsification. It compiled
with `g++ -O2 -std=c++17` and byte-matched wide64_dev, but certified only
118/9,990 non-self queries (1.18%), popped 4.016M heap entries, and took
0.42s / 23.6MiB versus ~0.37s / 23MiB retained. The path incumbent is too
weak; reject before fresh/large testing. No production change.

### Additional regional ALT screens (2026-09-26)

Exact partial regional landmark balls did not show a large-tier win. On
wide64, 64 full regional SSSPs took 16.995s on the 2M-vertex graph and raised
total from the 29.692s formal row to 34.70s / 1.65GB despite cutting query
work from 213.08M to 104.19M pops. A 64-table radius-capped variant settled
only 2.10M vertices in preprocessing and remained exact on dev, a fresh seed,
and wide64_large, but total was 33.66s / 1.65GB. The regional potential does
not repay its per-search cost.

The same exact capped-ball construction on lattice3d used 4x4x4 regions and
32,768 settled vertices per region. It was exact on lattice3d_dev and reduced
search to 4.67M pops / 16.24M scans, but total dev time was 1.35s versus 1.27s
retained, with RSS rising from 65MiB to 90MiB. Both variants remain rejected;
no production path was changed by these experiments.

### 2026-09-26 follow-up candidate screens

A local2d coordinate-monotone route incumbent matched dev but took 0.89s /
111MiB, indistinguishable from retained. Per-query heuristic memoization had
only 1.20x reuse (1.985M calls, 1.651M misses) and took 0.92s versus 0.89s.
A short one-sided / long balanced-biALT hybrid was exact on dev and large but
large regressed to 15.70s / 674MiB versus 14.34s / 672MiB; all three are
rejected.

The integrated local2d table now stores exact landmark distances as uint32
only after checking `(V-1)*max_edge < UINT32_MAX`, plus a per-table finite and
range check. Official dev, fresh seed 20260930 / `local-u32`, and large outputs
were exact. The integrated large recheck took 13.44s / 379MiB versus 14.34s /
672MiB for the prior 64-bit table solver.

A 32-bit lattice3d ALT-table variant used a certified root eccentricity bound
but was slower on lattice3d_large: 20.69s / 203MiB versus 16.18s / 353MiB;
reject. A relaxed per-axis minimum-edge torus bound exceeded the current ALT
source bound on 0/10,000 lattice3d_dev queries (means 27.59 vs 759.15), so it
was rejected before adding query-time work. These probes did not change the
formal checkpoint.

### hugeq singleton restricted-search screen (2026-09-26)

Swapping the 17,332 singleton target/leaf groups to consistent bidirectional
directed ALT was exact on all 600,000 hugeq_large answers, but regressed to
11.24s / 1.21GB from 9.23s / 1.21GB. The remaining 8,580 multi-source groups
were unchanged. Reject; no production change.

### Scale-free grouped-target regression removal (2026-09-26)

Instrumented scale-free profiling attributed 126.35M pops / 254.71M scans to
303 grouped searches for 15,027 queries, while 34,923 work-balanced point
searches used 59.29M pops / 161.03M scans. Disabling target batching on
reverse-degree-skew graphs matched all 50,000 large answers and reduced time
from 15.62s to 9.20s. The integrated structural dispatch keeps batching for
non-skew directed graphs; it passed all six dev sets, fresh seed 20260930 /
`scale-no-target-batch`, and a 9.43s / 40MiB exact scalefree_large recheck.

A fixed regular 6x4 set of 24 wide64 landmarks matched dev but regressed to
5.23M pops / 15.22M scans and 0.46s versus about 3.38M / 12.23M and 0.38s
for farthest-point roots. Reject.

The combined road/local2d/scalefree targeted rows project roughly 91.0x Overall
when other formal rows are held fixed. This is not an official grade.

### Additional architecture screens (2026-09-26)

The status-127 log was traced to a failed temporary C++ compile followed by
`/usr/bin/time` trying to execute the missing binary; the compiler error was
an obsolete helper call, not a missing Python interpreter. Candidate execution
is now gated on successful compilation.

An exact reverse out-degree-one chain contraction for scale-free search passed
dev and large comparisons, but large regressed to 13.35s / 40MiB against an
8.36s same-session baseline. Although 147,929 vertices have reverse
out-degree one, they form 129,013 runs with median length one. Reject.

A max-in-degree landmark incumbent with per-arc directed ALT pruning skipped
15.99M of 25.10M high-degree reverse arcs and remained exact on dev and large,
but took 9.10s large and 0.38s dev versus same-session baselines of 8.36s and
0.34s. Arc reads and landmark checks outweighed heap savings. Reject.

Radius-100,000 clipped local-grid tables preserve a consistent admissible ALT
potential. They reduced local-table preprocessing settlements by only 11%; the
large total rose from 13.44s / 388MiB to 28.77s / 451MiB, despite exact dev
and large answers. Reject.

Four-anchor local-grid bidirectional ALT matched official dev, a fresh seed,
and all large answers. It changed large work from 116.59M pops / 370.23M scans
to 115.32M / 369.39M and elapsed time from 13.44s to 13.66s, so the roughly
1% search reduction does not pay for two-sided heuristic evaluation. Reject.
All four prototypes were isolated under `/tmp`; `solver.cpp` was not changed.

### Official six-large grade for source SHA 4610587e (2026-09-26)

After recompiling the combined source and checking all six dev answers, the
official run `python3 -u grade.py --solver ./solver --instances instances.txt
--json result-integrated-4610587-full.json` reported all six large instances
exact (`ok`):

| Instance | T_base | T_solver | Speedup | Peak RSS |
|---|---:|---:|---:|---:|
| road2d_large | 945.350s | 16.719s | 56.544x | 198 MiB |
| lattice3d_large | 1637.251s | 15.964s | 102.559x | 353 MiB |
| local2d_large | 620.957s | 13.124s | 47.315x | 379 MiB |
| scalefree_large | 1755.288s | 8.835s | 198.665x | 41 MiB |
| hugeq_large | 2029.355s | 9.098s | 223.065x | 1184 MiB |
| wide64_large | 1122.656s | 28.701s | 39.116x | 632 MiB |

Global geomean 126.613x, Local geomean 43.020x, Overall 88.35080960833166x.
The result was generated in that same run and records binary SHA256
`2582aed37dab141b5d52d618ca4a14879c2d8747e7579c7b30a63bce9ad5401d`, source
SHA256 `4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`,
and result SHA256
`fe329dbec22b822e62dc24d2843bcb549c3c843000638782cf6739375948b972`.
The solver source is preserved as
`research/checkpoints/certified-integrated-4610587.cpp`; the 150x target is
still open.

### Wide64 landmark-pair potential screen (2026-09-27)

An exact pairwise-landmark triangle lower bound was added to ALT4 in an
isolated prototype. It matched wide64_dev, but settled 4.024M heap entries /
scanned 12.192M arcs versus 3.379M / 12.228M for production; 0.36s dev time
was indistinguishable from about 0.37s retained. Reject before large testing;
the formally certified solver remains unchanged.

### Local and wide64 structural-potential screens (2026-09-27)

Selecting all-query top-4 local-grid landmarks instead of the target-near
anchors was exact on local2d_dev, but a same-session run took 0.93s / 61.3MiB
and 1.087M pops / 3.726M scans, versus 0.87s / 61.5MiB on the certified
path. Reject before large testing.

On wide64_dev, a weighted-axis torus relaxation derived from per-cut minimum
edge weights was exact but took 2.25s and expanded 28.20M pops / 101.95M arcs,
versus 0.37s and 3.38M / 12.23M for 24-landmark ALT4. A second exact relaxed
graph collapsed 2x2 cells and used per-boundary minimum crossing weights;
24 coarse ALT tables took 0.024s to build, but total was 3.41s / 15.3MiB
with 40.73M pops / 147.27M scans. Both candidates are rejected before large
testing. Scratch sources remain under `/tmp`; production SHA is unchanged.

Two additional exact local2d batch-profile ideas failed dev work gates. Using
24 global farthest-point landmarks only for long-displacement queries cost an
extra 0.076s preprocessing and raised total dev time from 0.88s to 1.03s; the
625 long-branch queries expanded 1.368M / 5.010M pops/scans versus 0.471M /
1.713M using the retained 256-grid top-4. A multi-source distance profile to
each spatial target cell used one lookup per node but was too weak: it expanded
3.881M / 14.160M pops/scans and took 1.03s total, also versus 0.88s retained.
Both matched all local2d_dev answers and remain scratch-only; no large run.

Two endpoint-local ALT variants also failed the dev time gate. Selecting the
best four from the 4x4 nearest-anchor pool reduced work to 980,929 pops /
3,533,064 scans but still took 0.88s versus 0.87s. Separate source-near and
target-near potentials in bidirectional ALT were exact, with 552,877 short
query pops / 2,006,224 scans and 0.90s total versus 0.87s retained. Reject
without large testing; source and checkpoint are unchanged.

Scheduling that endpoint-specific bidirectional search by cumulative arc work
reduced short-query work further to 505,509 pops / 1,836,416 scans (balanced
918,208 scans per side) and remained exact, but dev total was 0.91s versus
0.87s retained. Reject before fresh/large; the extra potential work still
dominates.

Using 24 global landmarks with balanced biALT only for long-displacement
local2d queries was exact but took 1.11s / 71.4MiB total, including 0.082s
global preprocessing. Its 625 long queries expanded 1.375M pops / 5.033M
scans, so reject before large testing.

A 128-settlement exact witness search through strictly lighter edges on
local2d_dev found only 352 additional undirected replacements, raising edge
removal from 8.009% to 8.360%. It settled 438,638 vertices / scanned 1.753M
arcs, increased preprocessing from 0.005s to 0.027s, and total time from
0.87s to 0.91s. All dev answers were exact; reject before large testing.

On directed road2d_dev, bounded strict-lighter witnesses found only 553 extra
undirected replacements (0.203 percentage points). They required 1.053M
settlements / 5.746M scans; CH shortcut count fell by only 173, while
sparsification grew from 0.027s to 0.096s and total dev time from 0.93s to
0.98s. Exact but rejected before large testing.

An exact CH DAG-label index on road2d_dev built 11.77M labels in 1.224s
(179.6MiB compact storage) and returned exact answers, but total dev was
2.02s versus 0.93s retained. On road2d_large the 120M-label guard aborted
construction after 18.39s at about 2.07GiB RSS; exact CH fallback returned
all answers, but total was 35.91s versus 16.72s. Reject the full-label index
for time and scale; no production code changed.

### Formal Overall comparison and rank-Dijkstra screen (2026-09-27)

| Formal candidate | Source SHA256 prefix | Large exact rows | Overall geomean | Matching result |
|---|---|---:|---:|---|
| Current retained integrated solver | `4610587e83f4` | 6/6 | 88.35080960833166x | `result-integrated-4610587-full.json` |
| Earlier hugeq4 integrated solver | `a9e6cc622981` | 6/6 | 51.946922866870345x | `result-hugeq4-full.json` |

Both results are official full six-large outputs using the same `grade.py`
geometric mean over the six speedups. For the current row-by-row runtimes,
RSS and exactness, see the full table in the 4610587e certification section
above. The current source matches
`research/checkpoints/certified-integrated-4610587.cpp`; its matching result
was produced from binary SHA256
`2582aed37dab141b5d52d618ca4a14879c2d8747e7579c7b30a63bce9ad5401d`.

The requested `python` command itself exits 127 here because it is not
installed as a command name. `/usr/bin/python3` is available (Python 3.14.4),
so the repaired invocation is `python3 ...`; all subsequent Python checks use
that executable. There was a separate earlier 127 incident in which a stale
road prototype failed compilation on the removed symbol
`road_region_arc_may_be_shortest`, and `/usr/bin/time` then tried to execute
the absent binary. The two causes are distinct.

An exact ordinary one-sided Dijkstra candidate was screened for wide64 using
only the verified high-weight 2-D torus structure to skip landmark building.
It compiled and matched all six official dev outputs. On wide64_dev the
candidate took 6.016s / 12MiB (6.10s first run), compared with the retained
ALT path's recorded ~0.37s dev run. Rank queries select powers-of-two settle
positions, so this does not produce a shallow search. Reject before fresh or
large testing; `solver.cpp` was not changed.

The current solver recheck on the same wide64_dev graph took 0.36s / 23MiB
and was exact. A separate exact distance-preserving shortcut graph added
two-edge straight-line arcs after the 24 landmark tables were built. It
compiled and matched all six dev answer files, but wide64_dev took 0.44s /
32MiB; it added 164,570 arcs, increasing total arcs from 181,488 to 346,058,
and query work from 3.38M heap pops / 12.23M scans to 4.26M / 22.15M. Reject
before fresh-seed or large testing. Scratch source:
`/tmp/wide64_two_step_shortcuts.cpp`; production source remains unchanged.

An exact local2d tile-portal oracle (B=8) also passed all six dev comparisons.
Tile APSP and portal overlay preprocessing took 0.207s for 784 tiles, 20,361
portals, and 532,248 directed arcs; the existing 256-grid landmark build took
0.798s. Query work was 464,712 portal pops and 10.562M scans. End-to-end
local2d_dev was 1.11s/85MiB, slower and larger than the same-session retained
0.87s/62MiB. Dense overlay scans erased the pop reduction; reject before
fresh/large validation. Scratch source:
`/tmp/local_tile_portal_oracle.cpp`; no production source changed.

Road CH cumulative-work frontier scheduling passed road2d_dev exactness but
was 0.89s versus 0.89s retained and increased actual scans: 3.69M versus 3.40M
stall arcs and 3.08M versus 2.90M relaxation arcs. Reject before fresh/large.
Two variants using `minF + minB >= best` in the upward-only CH query were
incorrect: the first had 4,159/10,000 wrong road2d_dev answers with stall, and
disabling stall still left the first answer wrong. The generic bidirectional
bound is not valid for this CH frontier implementation as applied; no
large/fresh test and no production source change.

A structural wide64 64-landmark candidate retained four-anchor per-query ALT
selection, compiled, and matched all six dev outputs, fresh seed 987654321 /
`lm64-screen`, and wide64_large exactly. Large settled work fell from the
instrumented 24-landmark baseline's 213.08M pops / 770.73M scans to 110.42M /
399.30M, while large runtime improved only from 28.65s to 26.12s and 26.68s on
two candidate runs. RSS rose from 647MiB to 1,272MiB; wide64_dev regressed from
0.36s to 0.44-0.46s. Reject: the end-to-end 7-9% large gain is too small for
the 150x objective and the extra 625MiB. Production remains source SHA
`4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`.

A 128-settlement strict-lighter Dijkstra witness pass on lattice3d_dev found
7,701 additional undirected replacements, moving sparsification from 44.956%
to 50.024%. Query scans fell 10.1% (18.315M to 16.458M) and pops stayed near
5.28M, but witness preprocessing added 0.43s and total dev time rose from
1.22s to 1.87s. Reject before fresh/large testing.

An exact same-source, same-leaf multi-goal forward ALT variant on
hugeq_large retained separator answers as feasible alternatives and matched
all official answers, but tied the retained path at 9.63s / 1.21GB (about
1,185MiB RSS). It
processed 23,568 grouped searches (20.92M pops / 113.41M arc inspections).
Reject before fresh-seed validation. Scratch source:
`/tmp/hugeq_source_group_alt.cpp`; production unchanged.

On lattice3d_dev, composing three exact 2-D coordinate-projection relaxations
with the retained 128-landmark balanced ALT was exact but reduced work by less
than 0.02% (5.286M pops / 18.314M scans to 5.286M / 18.314M). The 24-landmark
tables on each small quotient graph added negligible memory, but every query
heuristic evaluation cost more; runtime rose from 1.22s to 1.83s. Reject before
fresh/large testing.

A feasible-intersection update on each relaxed CH endpoint did not repair the
`minF + minB >= best` stopping candidate: with stall it still returned a wrong
road2d_dev answer (first observed at query 2). Retain the conservative CH stop.

Sixty-four 32,768-settlement clipped wide64 ALT roots were exact on all six dev
sets, fresh wide64 seed 987654321 / `lm64-screen`, and wide64_large. Large
search work barely changed (213.08M / 770.73M to 211.02M / 763.26M pops/scans),
while time regressed from 28.65s to 35.03s and RSS from 647MiB to 1,647MiB.
Reject; the truncated global cover does not scale to the large graph.

A sparse 16x16 grid of clipped roots, each capped at 32,768 settled nodes,
matched wide64_large but took 35.75s / 1.71GB with 206.40M pops / 746.56M scans;
reject. On local2d_dev, the same local-root method plus 24 global landmarks
matched all answers at 0.83s / 114MiB versus 0.89s / ~110MiB, but raised query
work from about 1.06M / 3.83M to 1.21M / 4.36M pops/scans. The small dev-time
gain comes from preprocessing only and does not justify large testing.

The clipped-local-root candidate was nevertheless checked on local2d_large
because the retained 256 full-root preprocessing alone takes 5.00s. With 256
32,768-settlement local roots plus 24 global landmarks, the candidate matched
all large answers but took 31.43s / 366MiB; the exact retained run was 13.66s /
379MiB. It performed 246.33M pops / 903.09M arc scans. Saving roughly 4.34s
of full local-grid preprocessing did not offset the weaker clipped tables;
reject. Scratch source `/tmp/local2d_capped_landmarks.cpp`, no production edit.

For road2d, the exact feasible corridor incumbent (best original backbone
path through a structurally recognized fast row/column) compiled and matched
road2d_dev. It initialized the safe CH search with a concrete route cost, but
instrumented dev work was unchanged: 755,450 versus 755,551 pops, 3.401M
versus 3.402M stall scans, and 2.902M versus 2.902M relax scans. Runtime was
0.91s versus 0.94s, within noise; reject before large testing. Production
source remains unchanged.

The road CH edge-difference priority proxy compiled and matched road2d_dev,
but raised shortcut count from 224,335 to 226,694. Query work was 752,656 pops
/ 3.434M stall scans / 2.928M relax scans versus 755,551 / 3.402M / 2.902M
retained. At 0.88s versus 0.94s dev, the candidate showed no material
end-to-end win and was rejected before large testing. Scratch source:
`/tmp/road_ch_edge_diff_profile.cpp`; production unchanged.

For wide64, 24 exact landmark roots were selected greedily from a 32x32
spatial candidate grid by sampled pairwise Manhattan endpoint separation.
Exact SSSP tables were then built for the selected roots. The candidate
compiled and matched every wide64_dev answer, but the geometry proxy selected
poor weighted-metric roots: 4.418M pops / 15.949M scans and 0.46s versus
3.379M / 12.228M and about 0.36s with retained farthest-point roots. Reject
before fresh-seed or large testing; production is unchanged.

hugeq profiling showed 19.31M pops / 104.70M arc scans in 25,912 restricted
groups after 224 top, 223 second, 468 third, and 560 fourth-level hubs. A fifth
separator level initially missed shortest paths that leave and re-enter the
same fourth-level region; adding the parent-micro boundary candidate repaired
that omission. The fixed candidate matched 260,000 forced hugeq_dev answers
and all hugeq_large answers, but took 8.75s / 1,991MiB versus 9.07s / 1,184MiB
for the retained profiled run. Reject the 3.5% gain at +807MiB. Scratch source
`/tmp/hugeq_depth5.cpp`; no production edit.

On wide64, exact original row/column-cycle prefixes supplied the best feasible
L-shaped torus route as an initial ALT4 upper bound. The candidate matched all
wide64_dev answers, but changed work only from 3.379M pops / 12.228M scans to
3.379M / 12.190M; dev time was 0.34s versus about 0.36s. Reject before fresh
seed or large testing. Scratch source `/tmp/wide64_route_upper.cpp`; production
unchanged.

Adding nearby fast-row/fast-column intersection routes to the road CH
incumbent compiled and matched road2d_dev after fixing a scratch-only loop
bound error. The candidate still had 755,445 pops / 3.401M stall scans /
2.902M relax scans and took 0.92s, effectively unchanged from baseline;
reject before large testing. Scratch source `/tmp/road_ch_route_ub.cpp`;
production unchanged.

Using actual witness-generated shortcut counts in a signed edge-difference
priority produced 218,865 road2d_dev shortcuts versus 224,335 retained and
matched all answers, but required 2.90s / 37MiB to build the hierarchy versus
about 0.9s / 32MiB. A zero-clamped score instead reproduced the retained
224,335 shortcuts in 0.90s. Reject the actual-count ordering before large
testing; scratch sources `/tmp/road_ch_actual_ed.cpp` and
`/tmp/road_ch_actual_ed_offset.cpp`; production unchanged.

Wide64's exact top-four ALT selection was changed to select at most one
landmark from each spatial quadrant around the source-target midpoint. It
matched all dev answers but expanded 3.617M pops / 13.049M scans and took
0.38s, compared with 3.379M / 12.228M and about 0.36s retained. Reject before
fresh-seed/large testing; scratch source `/tmp/wide64_diverse_landmarks.cpp`.

Profiling road CH on large split 8.14s contraction from 8.40s query time;
query work was 183.73M stall-arc checks plus 147.26M relaxation scans. Removing
stall remained exact on official dev, fresh road2d_dev seed 20260927, and all
road2d_large answers. It changed large query work to 36.42M pops / 258.82M
relax scans; total was 16.24s with instrumentation. Uninstrumented same-run
totals were 16.50s retained and 16.18s no-stall, a 1.9% gain; reject for the
150x campaign. Scratch source `/tmp/solver_nostall.cpp`.

Wide64 balanced bidirectional ALT matched dev and large exactly but was slower
end-to-end: dev 0.42s versus 0.36s retained; large 37.10s / 647MiB versus
28.70s / 632MiB. Its 210.997M pops / 763.352M scans reduced scans by only
about 1% from the retained 213.081M / 770.732M. Reject; certified source
unchanged. Scratch source `/tmp/wide64_bialt_current.cpp`.


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
The vertex-major storage candidate also matched wide64_dev and wide64_large,
but took 0.40s on dev and 35.20s / 996MiB on large versus about 0.36s and
28.70s / 632MiB retained. Reject; scratch source
`/tmp/wide64_vertex_major.cpp`.

Replacing local2d's 256 local-grid tables with 24 global landmarks and routing
Manhattan-near pairs to plain bidirectional Dijkstra matched both local2d_dev
and local2d_large exactly. It took 0.58s / 23MiB on dev but regressed the
large workload to 42.15s / 147MiB versus 13.12s / 379MiB retained; reject.
Scratch source `/tmp/local_global_bidir.cpp`.

Wide64 24-landmark distances were also conservatively quantized into 32-bit
root-major tables. The admissible consistent bucket potential matched dev and
large, but took 0.70s / 28MiB and 45.00s / 815MiB versus about 0.36s / 32MiB
and 28.70s / 632MiB retained; reject. Scratch `/tmp/wide64_compressed_alt.cpp`.

Selecting local2d's top four of 256 exact anchors by endpoint distance gap
instead of nearby geometric anchors matched dev and reduced search work by
about 2.8%, but total dev time was 0.92s versus 0.89s. Reject before large;
scratch `/tmp/local_score_landmarks.cpp`.

Wide64 nested-dissection contraction hierarchy was exact on dev but took
27.14s / 55MiB and built 1.24M shortcuts on only 50k vertices; reject before
large (retained dev path is subsecond). Road CH with direct directed ALT
frontier potentials and a symmetric-relaxation ALT variant both returned
wrong road2d_dev answers under their tested stopping rules; reject before
large. Scratch `/tmp/wide64_nested_ch.cpp`, `/tmp/road_ch_alt.cpp`, and
`/tmp/road_ch_relaxed_alt_gstop.cpp`.

Saving wide64's exact ALT key per vertex matched dev and large. Two comparable
large runs were 27.48s and 28.02s against a same-session 28.07s baseline; a
single 4,056s `time` reading contradicted wall-clock elapsed and was discarded.
No stable gain; reject. Lattice3d balanced-ALT saved-key validation was exact
at 1.16s versus 1.17s dev and was rejected before large. A local2d 32-pop
bidirectional first stage matched dev but took 0.90s versus 0.89s retained,
despite 2,626/10,000 exact hits; reject before large.

No-witness nested-dissection CH added only valid shortcuts and matched 16
independent Dijkstra answers on a fresh 32x32 weighted torus. On wide64_dev,
40,000/50,176 contractions and 884,626 shortcuts took over 60s without an
answer file; interrupted and rejected before large. Scratch
`/tmp/wide64_nested_ch_nowitness.cpp`.

The per-coordinate-cut minimum edge weights formed a consistent torus lower
bound when summed on the shorter ring arcs and maxed with ALT. It matched
wide64_dev, but exceeded ALT on only 1.5% of heuristic evaluations and slowed
dev from about 0.36s to 0.39s; reject before large. Scratch
`/tmp/wide64_cut_alt.cpp`.

No new candidate was retained in this research round; production remains
`solver.cpp` SHA256 `4610587e83f4bd6e8ec92db0ae1efe7cd9ddf4167e6535b04439cff4f8b03fc0`.
The corresponding official full six-large result is still exact at Overall
88.35080960833166x in `result-integrated-4610587-full.json`. The 150x target
remains unmet; no redundant full grade was run without a retained change.

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

### WSL formal sparse-separator result and next architecture screens (2026-09-27)

The WSL full six-large run for commit `7063c19` completed with all six rows
exact at **Overall 94.295x**; `hugeq_large` took **5.087 s**. This is now the
formal baseline to beat. It replaces the earlier 97.64--98.83x projection,
which was never a formal score. The matching WSL JSON is not available in this
cloud checkout, so no additional digits, per-row times, or RSS numbers are
claimed. Keep the prior `result-integrated-4610587-full.json` as the preserved
88.35080960833166x rollback artifact rather than overwriting it.

After `0adf4a4`, a bounded screen revisited the 3-D lattice landmark-build/query
tradeoff because this is one of the remaining plausible preprocessing/query
composition changes. All tested counts (64, 72, 80, 88, 96, 104, 128 and 160)
matched every `lattice3d_dev` answer. A representative sequential screen was
1.791, 1.985, 1.790, 1.706, 1.683, 1.716, 1.780 and 1.938 seconds respectively;
96 tables used 51,264 KiB versus 63,940 KiB for 128. This supports only a
roughly 5--11% dev improvement, not a multi-fold end-to-end route, and timings
were noisy. Do not integrate or request a full WSL run for it yet.

The wide64 two-active-table screen was exact but consistently regressed dev
from about 0.80 s to 0.94--1.01 s. Six and eight active tables reduced work;
the retained eight-table candidate measured 2,891,040 pops / 9,012,338 scans
versus 4,024,305 / 12,191,972 for four tables, but this remains a modest
optimization currently undergoing WSL testing, not a new formal Overall.
No credible new multi-fold architecture was found in this bounded continuation.

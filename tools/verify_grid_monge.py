#!/usr/bin/env python3
"""Verify the boundary-distance Monge matrices needed by a grid DDG.

This is deliberately a research tool, not part of the submitted solver.  It
cuts square regions out of an observable row-major 2-D grid, discards edges
leaving each region (including torus wrap edges), computes exact internal
boundary distances, and checks pairs of disjoint boundary intervals.
"""

import argparse
import heapq
import math
import resource
import time


def read_graph(path):
    with open(path) as f:
        v, e, flags = map(int, f.readline().split())
        directed = bool(flags & 2)
        adj = [[] for _ in range(v)]
        for _ in range(e):
            a, b, w = map(int, f.readline().split())
            adj[a].append((b, w))
            if not directed:
                adj[b].append((a, w))
    n = math.isqrt(v)
    if n * n != v or directed:
        raise ValueError("verifier requires an undirected square row-major grid")
    return n, adj


def sides(x0, y0, w, h, n):
    # Four non-overlapping cyclic intervals in clockwise face order.  Keeping
    # corners in exactly one interval avoids duplicate matrix rows/columns.
    return [
        [y0 * n + x for x in range(x0, x0 + w)],
        [y * n + x0 + w - 1 for y in range(y0 + 1, y0 + h)],
        [(y0 + h - 1) * n + x for x in range(x0 + w - 2, x0 - 1, -1)],
        [y * n + x0 for y in range(y0 + h - 2, y0, -1)],
    ]


def distances(source, allowed, adj):
    dist = {source: 0}
    queue = [(0, source)]
    while queue:
        d, u = heapq.heappop(queue)
        if d != dist[u]:
            continue
        for v, weight in adj[u]:
            if v not in allowed:
                continue
            nd = d + weight
            if nd < dist.get(v, 1 << 63):
                dist[v] = nd
                heapq.heappush(queue, (nd, v))
    return dist


def verify_region(x0, y0, w, h, n, adj):
    allowed = {y * n + x for y in range(y0, y0 + h)
               for x in range(x0, x0 + w)}
    arcs = sides(x0, y0, w, h, n)
    boundary = [v for arc in arcs for v in arc]
    table = {v: distances(v, allowed, adj) for v in boundary}
    tests = violations = matrices = 0
    # With both intervals listed clockwise, reverse the column interval.  The
    # four endpoints of each adjacent 2x2 minor then have the non-crossing
    # cyclic order used by the Monge inequality in the assignment.
    for ai in range(4):
        for bi in range(ai + 1, 4):
            rows, cols = arcs[ai], list(reversed(arcs[bi]))
            if len(rows) < 2 or len(cols) < 2:
                continue
            matrices += 1
            for i in range(len(rows) - 1):
                for j in range(len(cols) - 1):
                    a = table[rows[i]][cols[j]]
                    b = table[rows[i + 1]][cols[j + 1]]
                    c = table[rows[i]][cols[j + 1]]
                    d = table[rows[i + 1]][cols[j]]
                    tests += 1
                    violations += a + b > c + d
    return len(boundary), matrices, tests, violations


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("graph")
    ap.add_argument("--side", type=int, default=16)
    ap.add_argument("--limit", type=int, default=0,
                    help="maximum regions (zero means every complete region)")
    args = ap.parse_args()
    started = time.monotonic()
    n, adj = read_graph(args.graph)
    totals = [0, 0, 0, 0]
    regions = 0
    for y in range(0, n - args.side + 1, args.side):
        for x in range(0, n - args.side + 1, args.side):
            result = verify_region(x, y, args.side, args.side, n, adj)
            totals = [a + b for a, b in zip(totals, result)]
            regions += 1
            if args.limit and regions >= args.limit:
                break
        if args.limit and regions >= args.limit:
            break
    rss_kib = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    print(f"regions={regions} side={args.side} boundary_total={totals[0]} "
          f"matrices={totals[1]} inequalities={totals[2]} "
          f"violations={totals[3]} elapsed={time.monotonic()-started:.3f}s "
          f"peak_rss={rss_kib}KiB")
    return bool(totals[3])


if __name__ == "__main__":
    raise SystemExit(main())

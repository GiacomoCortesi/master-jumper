"""Offline mirror of AMJPlayer's kinematic jump, used to tune the single jump window.

Mirrors the defaults in Source/MasterJumper/Public/MJJumpCharge.h and the level layout in
Source/MasterJumper/Public/MJLayout.h. Run:  python3 Tools/jump_sim.py
"""
import math, random

# ---- Layout (cm, X forward, Z up, floor at 0) -- keep in sync with MJLayout.h ----
OBST_X0, OBST_X1, OBST_TOP = 2400.0, 2620.0, 165.0
FLOOR_END_X = 3150.0          # stairwell gap starts here
GATE_X0, GATE_X1 = 2640.0, 3150.0
CEILING_Z = 330.0

# ---- Player / jump tuning -- keep in sync with MJJumpCharge.h / MJPlayer.h ----
RUN_SPEED = 600.0
RADIUS = 30.0
STAND_HH = 90.0
DIVE_HH = 45.0
FLOOR_GAP = 2.0              # AMJPlayer::FloorGap (capsule hovers this far above the floor)
TAKEOFF_S = 0.12
GRAVITY = 980.0 * 1.9
H_IMPULSE = 820.0
V_IMPULSE = 800.0
MIN_HOLD, OPT_HOLD, MAX_HOLD = 0.20, 0.55, 0.90
# hold seconds -> strength (linear keys)
CURVE = [(0.0, 0.30), (MIN_HOLD, 0.62), (OPT_HOLD, 1.0), (MAX_HOLD, 1.22)]
FUMBLE_STRENGTH = 0.30

DT = 1.0 / 120.0


def strength(h):
    if h < MIN_HOLD:
        return FUMBLE_STRENGTH
    for (x0, y0), (x1, y1) in zip(CURVE, CURVE[1:]):
        if h <= x1:
            return y0 + (y1 - y0) * (h - x0) / (x1 - x0)
    return CURVE[-1][1]


def hits_obstacle(x, z, hh):
    # capsule (vertical segment + radius) vs AABB in XZ
    seg_lo, seg_hi = z - hh + RADIUS, z + hh - RADIUS
    dx = max(OBST_X0 - x, 0.0, x - OBST_X1)
    if dx >= RADIUS:
        return False
    # closest z on segment to box [0, OBST_TOP]
    if seg_lo > OBST_TOP:
        dz = seg_lo - OBST_TOP
    else:
        dz = 0.0
    return dx * dx + dz * dz < RADIUS * RADIUS


def simulate(t_press, t_release):
    """Returns 'success' or a failure reason."""
    t, x, z = 0.0, 0.0, STAND_HH + FLOOR_GAP
    # grounded running until release + takeoff
    t_launch = t_release + TAKEOFF_S
    while t < t_launch:
        x += RUN_SPEED * DT
        t += DT
        if hits_obstacle(x, z, STAND_HH):
            return 'hit_running'
    s = strength(t_release - t_press)
    vx, vz = H_IMPULSE * s, V_IMPULSE * s
    hh = DIVE_HH
    while True:
        vz -= GRAVITY * DT
        x += vx * DT
        z += vz * DT
        if z + hh >= CEILING_Z:
            return 'ceiling'
        if hits_obstacle(x, z, hh):
            return 'hit_air'
        if z - hh <= 0.0:
            if x > FLOOR_END_X:
                return 'gap'
            break
    # landed
    if x < OBST_X0:
        return 'short'   # will run into the stack with the jump spent
    if GATE_X0 <= x <= GATE_X1:
        return 'success'
    return 'hit_air'


def window_report():
    grid = []
    for ip in range(0, 500):
        tp = 2.0 + ip * 0.004
        for ir in range(0, 300):
            h = ir * 0.004
            grid.append((tp, h, simulate(tp, tp + h) == 'success'))
    ok = [(tp, h) for tp, h, s in grid if s]
    print(f'success cells: {len(ok)}')
    if ok:
        hs = sorted(set(round(h, 3) for _, h in ok))
        print(f'hold range that can succeed: {hs[0]:.3f}..{hs[-1]:.3f}s')
        # for a few holds, the press-time window
        for h in [0.30, 0.40, 0.45, 0.50, 0.55, 0.60, 0.65, 0.70, 0.80]:
            tps = [tp for tp, hh in ok if abs(hh - h) < 1e-6]
            if tps:
                print(f'  hold {h:.2f}: press window {min(tps):.3f}..{max(tps):.3f}s ({(max(tps)-min(tps))*1000:.0f} ms), release at x={RUN_SPEED*(min(tps)+h):.0f}..{RUN_SPEED*(max(tps)+h):.0f}')


def practiced_player(trials=10, seeds=200, sigma=0.06):
    """A practiced player aims at the centre of the window with Gaussian timing noise on press and release."""
    # aim: optimal hold, release so the launch lands mid-gate
    best = None
    for ir in range(300):
        tr = 3.0 + ir * 0.004
        tp = tr - OPT_HOLD
        if simulate(tp, tr) == 'success':
            best = (best or []) + [tr]
    aim_r = sum(best) / len(best)
    aim_p = aim_r - OPT_HOLD
    rng = random.Random(1)
    counts = []
    reasons = {}
    for _ in range(seeds):
        c = 0
        for _ in range(trials):
            tp = aim_p + rng.gauss(0, sigma)
            tr = max(tp + 0.01, aim_r + rng.gauss(0, sigma))
            r = simulate(tp, tr)
            reasons[r] = reasons.get(r, 0) + 1
            c += r == 'success'
        counts.append(c)
    counts.sort()
    print(f'practiced player (sigma {sigma*1000:.0f} ms, aim press {aim_p:.3f}s / release {aim_r:.3f}s):'
          f' successes per 10 -> median {counts[len(counts)//2]}, p10 {counts[len(counts)//10]}, p90 {counts[9*len(counts)//10]}')
    print('  outcomes:', dict(sorted(reasons.items(), key=lambda kv: -kv[1])))
    return counts[len(counts) // 2]


if __name__ == '__main__':
    window_report()
    for sigma in (0.03, 0.05, 0.07, 0.12):
        practiced_player(sigma=sigma)
    # sanity: never jumping fails, mashing fails
    assert simulate(99, 99.01) != 'success'
    assert simulate(3.5, 3.55) != 'success'

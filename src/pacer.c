// Horários dos quadros extras (ver pacer.h)
#include "pacer.h"

long long pacer_period(long long freq, int fps) {
    return fps > 0 ? (freq + fps / 2) / fps : 0;
}

static long long ceil_div(long long a, long long b) {   // b > 0
    return a >= 0 ? (a + b - 1) / b : -((-a) / b);
}

long long pacer_next(long long g0, long long period, long long start, long long after, long long end, long long margin) {
    if (period <= 0) return -1;
    long long lo = start + period / 2, hi = end - (margin > period / 2 ? margin : period / 2);
    if (after + 1 > lo) lo = after + 1;                   // estritamente depois de after
    long long t = g0 + ceil_div(lo - g0, period) * period;
    return t < hi ? t : -1;
}

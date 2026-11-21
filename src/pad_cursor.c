#include "pad_cursor.h"
#define PADCUR_SLACK 2                               // tremida tolerada, em pixels
int padcur_nav(PADCUR *s, long x, long y) {
    s->x = x; s->y = y;
    if (s->hidden) return PADCUR_KEEP;
    s->hidden = 1;
    return PADCUR_HIDE;
}
int padcur_mouse(PADCUR *s, long x, long y) {
    if (!s->hidden) return PADCUR_KEEP;
    long dx = x - s->x, dy = y - s->y;
    if (dx >= -PADCUR_SLACK && dx <= PADCUR_SLACK && dy >= -PADCUR_SLACK && dy <= PADCUR_SLACK) return PADCUR_KEEP;
    s->hidden = 0;
    return PADCUR_SHOW;
}
int padcur_leave(PADCUR *s) {
    if (!s->hidden) return PADCUR_KEEP;
    s->hidden = 0;
    return PADCUR_SHOW;
}

/* VELOCITY - vecteurs 3D, bornes, aleatoire
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Maths                                                               */
/* ------------------------------------------------------------------ */
typedef struct { float x, y, z; } V3;
static V3 v3(float x, float y, float z) { V3 r = { x, y, z }; return r; }
static V3 vadd(V3 a, V3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static V3 vsub(V3 a, V3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static V3 vmul(V3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
static float vdot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static V3 vcross(V3 a, V3 b) { return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
static float vlen(V3 a) { return sqrtf(vdot(a, a)); }
static V3 vnorm(V3 a) { float l = vlen(a); return l > 1e-6f ? vmul(a, 1.0f / l) : v3(0, 0, 0); }
static V3 vlerp(V3 a, V3 b, float t) { return vadd(a, vmul(vsub(b, a), t)); }
static float *vc(V3 *v, int i) { return &v->x + i; }
static float clampf(float x, float a, float b) { return x < a ? a : x > b ? b : x; }
static float hlen(V3 v) { return sqrtf(v.x * v.x + v.z * v.z); }

static unsigned int rng = 12345;
static float frand(void) { rng = rng * 1664525u + 1013904223u; return (rng >> 8) * (1.0f / 16777216.0f); }
static float frange(float a, float b) { return a + (b - a) * frand(); }
static V3 rand_dir(void) {
    for (;;) {
        V3 v = v3(frange(-1, 1), frange(-1, 1), frange(-1, 1));
        float l = vdot(v, v);
        if (l > 0.01f && l <= 1.0f) return vmul(v, 1.0f / sqrtf(l));
    }
}

/* VELOCITY - polices et matrices de camera
   Morceau de main.c : inclus dans l'ordre, ne se compile pas seul (voir main.c). */

/* ------------------------------------------------------------------ */
/* Rendu                                                               */
/* ------------------------------------------------------------------ */
static GLuint texGrid, texWall, texGrime, texDot, texStone, texPlaster, worldList, fontBase[3];
static int fontW[3], fontH[3];
static V3 camR, camU, camEye;
static GLfloat mvMat[16], prMat[16];                      /* matrices de la camera (projections a l'ecran, soleil de l'arme) */
static float viewSun = 1;                                 /* le joueur est-il au soleil (lisse) : eclaire l'arme en main */
static float fps;

/* generateur local, isole du rng de jeu : les textures doivent rester reproductibles sans
 * jamais toucher a la graine qui sert a placer les blocs de couverture de la carte */
static float tnoise(unsigned int *s) {
    *s = *s * 1664525u + 1013904223u;
    return (float)(*s >> 8) * (1.0f / 16777216.0f);
}

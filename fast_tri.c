#include "fast_tri.h"

#include <math.h>

// [0, PI/2] -> [0, 2^FAST_SIN_TABLE_BITS]
fast_tri_ret_type fast_sin_table[FAST_SIN_TABLE_LEN];

/*
 * @param omega_nom [0,2pi] mapped to [0, 2^(FAST_SIN_TABLE_BITS+2)]
 * @return [-1,1] mapped to [0, FAST_SIN_TABLE_SCALE]
 */
fast_tri_ret_type fast_sin(fast_tri_omega_type omega_nom) {
    // Normalize to [0,2PI]
    omega_nom = omega_nom & (FAST_SIN_MAPPED_2PI-1);

    // Normalize to [0, PI]
    fast_tri_omega_type omega_lookup = omega_nom & (FAST_SIN_MAPPED_PI-1);
    fast_tri_omega_type val_sign = omega_nom & FAST_SIN_MAPPED_PI;

    // Normalize to [0, PI/2]
    omega_lookup = (omega_lookup & FAST_SIN_MAPPED_PI_2) ? FAST_SIN_MAPPED_PI-omega_lookup : omega_lookup;

    // Lookup and interpolation
    fast_tri_omega_type index = omega_lookup >> FAST_SIN_TABLE_DECS;
    fast_tri_omega_type val = fast_sin_table[index];
    fast_tri_omega_type residual = omega_lookup & ((1<<FAST_SIN_TABLE_DECS)-1);
    fast_tri_omega_type val_1 = fast_sin_table[index+1];
    val = val + (((val_1-val)*residual)>>FAST_SIN_TABLE_DECS);

    return val_sign ? -val : val;
}
volatile float dummy = (fast_tri_omega_type)(((float)FAST_SIN_MAPPED_PI)/M_PI);
static void test_fast(float omega) {
    float ref = cos(omega);
    float est = fast_cos(omega /M_PI*FAST_SIN_MAPPED_PI)/65536.0F;
    dummy = est;
}

static void test_single_cos_sin() {
//    test_fast(-M_PI);
//    test_fast(-M_PI/3*2);
//    test_fast(-M_PI/2);
//    test_fast(-M_PI/3);
//    test_fast(-M_PI/4);
//    test_fast(-M_PI/15);
//    test_fast(-0.01);
//    test_fast(0.0);
//    test_fast(0.01);
//    test_fast(M_PI/15);
//    test_fast(M_PI/4);
    test_fast(M_PI/3);
//    test_fast(M_PI/2);
//    test_fast(M_PI/3*2);
//    test_fast(M_PI);
}

void fast_tri_init(void) {
    int i;
    for (i=0; i<FAST_SIN_TABLE_LEN; i++) {
        fast_sin_table[i] = sin(i * M_PI_2 / (FAST_SIN_TABLE_LEN-1))*FAST_SIN_TABLE_SCALE;
    }

    test_single_cos_sin();
}

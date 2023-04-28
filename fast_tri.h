#ifndef INC_FAST_TRI_H_
#define INC_FAST_TRI_H_

#include <stdint.h>

typedef int32_t fast_tri_omega_type;
typedef int32_t fast_tri_ret_type;

// Omega, number of bits mapped to PI/2
#define FAST_SIN_TABLE_BITS 8
// Omega, number of decimal points
#define FAST_SIN_TABLE_DECS 8
// Scale
#define FAST_SIN_TABLE_RESOLUTION 16

// Constants
#define FAST_SIN_TABLE_SCALE (((fast_tri_ret_type)1)<<FAST_SIN_TABLE_RESOLUTION)
#define FAST_SIN_MAPPED_2PI (((fast_tri_omega_type)1)<<(FAST_SIN_TABLE_BITS+2+FAST_SIN_TABLE_DECS))
#define FAST_SIN_MAPPED_PI (((fast_tri_omega_type)1)<<(FAST_SIN_TABLE_BITS+1+FAST_SIN_TABLE_DECS))
#define FAST_SIN_MAPPED_PI_2 (((fast_tri_omega_type)1)<<(FAST_SIN_TABLE_BITS+FAST_SIN_TABLE_DECS))
#define FAST_SIN_TABLE_LEN ((((fast_tri_omega_type)1)<<FAST_SIN_TABLE_BITS)+1)

/*
 * @param omega_nom [0,2pi] mapped to [0, 2^(FAST_SIN_TABLE_BITS+2)]
 * @return [-1,1] mapped to [-FAST_SIN_TABLE_SCALE, FAST_SIN_TABLE_SCALE]
 */
fast_tri_ret_type fast_sin(fast_tri_omega_type omega_nom);
static inline fast_tri_ret_type fast_cos(fast_tri_omega_type omega_nom) {
    return fast_sin(omega_nom + FAST_SIN_MAPPED_PI_2);
}

void fast_tri_init(void);

#endif /* INC_FAST_TRI_H_ */

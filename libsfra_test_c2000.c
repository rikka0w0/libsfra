#include "libsfra.h"

#include <math.h>

#include "F28x_Project.h"
#define IO_SET() GpioDataRegs.GPASET.bit.GPIO16 = 1
#define IO_CLR() GpioDataRegs.GPACLEAR.bit.GPIO16 = 1
// #define RAMFUNC __attribute__((ramfunc))

#ifndef RAMFUNC
#define RAMFUNC
#endif

#define FS 400000
#define FC 100.0F
#define THETA_FC (2.0F*M_PI*FC)
#define DENZ (THETA_FC + 2.0F*FS)
#define B0 (THETA_FC / DENZ)             // *z, num
#define B1 B0                           // *1, num
#define A1 ((THETA_FC - 2.0F*FS) / DENZ)    // *1, den

#define VECT_LEN 21
static float test_freq_vect[VECT_LEN], test_mag_vect[VECT_LEN], test_phase_vect[VECT_LEN];

static sfra_t sfra_struct = {
    .config.isrFreq = FS,
    .config.freqStart = 10,
    .config.freqStep = 1.584893192461113,
    .config.vecLength = VECT_LEN,
    .results.freqVect = test_freq_vect,
    .results.magnitudeVect = test_mag_vect,
    .results.phaseVect = test_phase_vect
};

RAMFUNC
static float iir_run(float input) {
    static float input_prev = 0;
    static float ouput_prev = 0;

    float output = (B0*input) + (B1*input_prev) - (A1*ouput_prev);
    input_prev = input;
    ouput_prev = output;

    return output;
}

RAMFUNC
void sfra_test_run(void) {
    IO_SET();
#if(SFRA_INT)
    fast_tri_ret_type vp = sfra_inject_int32(&sfra_struct);
#else
    // * (FAST_SIN_TABLE_BITS+2)
    sfra_float_t vp = 1024.0F * sfra_inject(&sfra_struct);
#endif
    IO_CLR();

    float vref = 10.0F;
    float vref_preturb = vref + vp;
    float vcrtl = iir_run(vref_preturb);

#if(SFRA_INT)
    // Convert floats to integers
    int32_t vref_preturb_int = vref_preturb;
    int32_t vcrtl_int = vcrtl; // assume plant TF = 1
#endif

    IO_SET();
#if(SFRA_INT)
    sfra_monitor_int32(&sfra_struct, vref_preturb_int, vcrtl_int);
#else
    sfra_monitor(&sfra_struct, vref_preturb, vcrtl);
#endif
    IO_CLR();
}

void sfra_test_background_task(void) {
    sfra_background_task(&sfra_struct);
}

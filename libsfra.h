#ifndef INC_LIBSFRA_H_
#define INC_LIBSFRA_H_

#include <stdint.h>

#include "libsfra_config.h"

#if(SFRA_INT)
#include "fast_tri.h"

#define SFRA_OMEGA_DEC_BITS 8

typedef int32_t sfra_signal_t;
typedef int64_t sfra_integral_t;
#else // #if(SFRA_INT)
#include <math.h>
// Use float accelerated trigonometric functions.
#define SFRA_FLOAT_SIN(omega) sinf(omega)
#define SFRA_FLOAT_COS(omega) cosf(omega)
#endif // #if(SFRA_INT)

typedef float sfra_float_t;
typedef uint32_t sfra_size_t;
typedef int_least8_t sfra_flag_t;

typedef struct {
    sfra_float_t *ctrl_real;
    sfra_float_t *ctrl_nimg;
    sfra_float_t *fb_real;
    sfra_float_t *fb_nimg;
    sfra_float_t *magnitudeVect;
    sfra_float_t *phaseVect;
    sfra_float_t *freqVect;      //!< Frequency Vector
} sfra_result;

typedef struct {
    sfra_float_t isrFreq;        //!< SFRA ISR frequency
    sfra_float_t freqStart;      //!< Start frequency of SFRA sweep
    sfra_float_t freqStep;       //!< Log space between frequency points (optional)
    sfra_size_t vecLength;       //!< No. of Points in the SFRA
} sfra_setup;

typedef struct {
    // Outer loop & state machine
    sfra_flag_t start;
    sfra_flag_t running;
    sfra_flag_t done;
    sfra_size_t freqIndex;    //!< Index of the frequency vector

    // Inner loop & state machine
    volatile sfra_flag_t dtft_running;      // Set and monitored by main(), clear by sfra_monitor() [in interrupt]
    sfra_size_t data_count;                 // Set by main(), read by sfra_monitor() [in interrupt]
    sfra_size_t data_index;                 // Clear by main(), read and write by sfra_monitor() [in interrupt]

#if(SFRA_INT)
    fast_tri_omega_type foi_rad;            // Set by main(), read by sfra_inject() [in interrupt]
    fast_tri_ret_type foi_sin;              // Pass intermediate value from sfra_inject() to sfra_monitor()
    fast_tri_ret_type foi_cos;              // Pass intermediate value from sfra_inject() to sfra_monitor()
    volatile sfra_integral_t dtft_real_num; // Real part of x_out
    volatile sfra_integral_t dtft_nimg_num; // Negative of imaginary part of x_out
    volatile sfra_integral_t dtft_real_den; // Real part of x_control
    volatile sfra_integral_t dtft_nimg_den; // Negative of imaginary part of x_control
#else
    sfra_float_t foi_rad;                   // Set by main(), read by sfra_inject() [in interrupt]
    sfra_float_t foi_sin;                   // Pass intermediate value from sfra_inject() to sfra_monitor()
    sfra_float_t foi_cos;                   // Pass intermediate value from sfra_inject() to sfra_monitor()
    volatile sfra_float_t dtft_real_num;    // Real part of x_out
    volatile sfra_float_t dtft_nimg_num;    // Negative of imaginary part of x_out
    volatile sfra_float_t dtft_real_den;    // Real part of x_control
    volatile sfra_float_t dtft_nimg_den;    // Negative of imaginary part of x_control
#endif
} sfra_internal_state;

typedef struct {
    sfra_result results;
    sfra_setup config;
    sfra_internal_state internal_state;
} sfra_t;

void sfra_init_all(void);
void sfra_start(sfra_t* sfra);
sfra_flag_t sfra_is_running(sfra_t* sfra);
sfra_flag_t sfra_is_done(sfra_t* sfra);
void sfra_clear_done(sfra_t* sfra);
void sfra_background_task(sfra_t* sfra);

#if(SFRA_INT)
/*
 * Generate a sinusoidal perturbation with unit magnitude.
 *
 * @param sfra a pointer to an initialized sfra_t struct
 * @return the sinusoidal perturbation, [-1,1] mapped to [-FAST_SIN_TABLE_SCALE, FAST_SIN_TABLE_SCALE]
 */
fast_tri_ret_type sfra_inject_int32(sfra_t* sfra);

/*
 * Pass the signals to the "bode plotter". When measuring the open-loop transfer function of a closed-loop system,
 * "control" and "feedback" should use the same base as the return value of sfra_inject_int32().
 *
 * @param sfra a pointer to an initialized sfra_t struct
 * @param control the control output if measuring the open-loop transfer function of a closed-loop system.
 *  In a plant transfer function measurement, the perturbed reference signal (usually the duty-cycle) should be passed.
 * @param feedback the feedback signal, usually is the ADC reading of the output voltage or current.
 */
void sfra_monitor_int32(sfra_t* sfra, sfra_signal_t control, sfra_signal_t feedback);
#else
sfra_float_t sfra_inject(sfra_t* sfra);
void sfra_monitor(sfra_t* sfra, sfra_float_t input, sfra_float_t output);
#endif

#if(SFRA_HAS_TEST)
void sfra_test_run(void);
void sfra_test_background_task(void);
#endif

#endif /* INC_LIBSFRA_H_ */

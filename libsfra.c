#include "libsfra.h"
#include <math.h>

void sfra_init_all(void) {
#if(SFRA_INT)
    fast_tri_init();
#endif
}

void sfra_start(sfra_t* sfra) {
    sfra->internal_state.start = 1;
}

sfra_flag_t sfra_is_running(sfra_t* sfra) {
    return sfra->internal_state.running;
}

sfra_flag_t sfra_is_done(sfra_t* sfra) {
    return sfra->internal_state.done;
}

void sfra_clear_done(sfra_t* sfra) {
    sfra->internal_state.done = 0;
}


static void set_float_if_nonnull(void* base, sfra_size_t offset, sfra_float_t value) {
    if (base != 0) {
        sfra_float_t* target = (sfra_float_t*) base;
        target += offset;
        *target = value;
    }
}

static void process_foi_data(sfra_t* sfra) {
    sfra_size_t freqIndex = sfra->internal_state.freqIndex;
    sfra_float_t dtft_real_num = sfra->internal_state.dtft_real_num;
    sfra_float_t dtft_nimg_num = sfra->internal_state.dtft_nimg_num;
    sfra_float_t dtft_real_den = sfra->internal_state.dtft_real_den;
    sfra_float_t dtft_nimg_den = sfra->internal_state.dtft_nimg_den;

    set_float_if_nonnull(sfra->results.ctrl_real, freqIndex, dtft_real_den);
    set_float_if_nonnull(sfra->results.ctrl_nimg, freqIndex, dtft_nimg_den);
    set_float_if_nonnull(sfra->results.fb_real, freqIndex, dtft_real_num);
    set_float_if_nonnull(sfra->results.fb_nimg, freqIndex, dtft_nimg_num);

    // Calculate gain in dB
    sfra_float_t mag = 10.0F * log10(
            (dtft_real_num*dtft_real_num + dtft_nimg_num*dtft_nimg_num) /
            (dtft_real_den*dtft_real_den + dtft_nimg_den*dtft_nimg_den)
    );
    /*
    sfra_float_t mag_num = sqrt(dtft_real_num*dtft_real_num + dtft_nimg_num*dtft_nimg_num)
            / sfra->internal_state.data_count * 2;
    sfra_float_t mag_den = sqrt(dtft_real_den*dtft_real_den + dtft_nimg_den*dtft_nimg_den)
            / sfra->internal_state.data_count * 2;
    sfra_float_t mag = 20*log10(mag_num / mag_den);
    */

    // Calculate phase in degrees
    sfra_float_t re = dtft_real_num*dtft_real_den + dtft_nimg_num*dtft_nimg_den;
    sfra_float_t im = dtft_real_num*dtft_nimg_den - dtft_nimg_num*dtft_real_den;
    sfra_float_t phase = atan(im/re) * 180.0F / M_PI;
    if (re < 0.0F) {
        if (im < 0.0F) {
            phase = phase - 180.0F;
        } else {
            phase = phase + 180.0F;
        }
    }

    set_float_if_nonnull(sfra->results.magnitudeVect, freqIndex, mag);
    set_float_if_nonnull(sfra->results.phaseVect, freqIndex, phase);
}

static sfra_size_t calc_cycles(sfra_float_t foi_hz, sfra_float_t isrFreq) {
    if (foi_hz < 10.0) {
        // DC - 10Hz, attempt to reduce time consumption
        return 10;
    } else if (foi_hz < 100.0) {
        // 10Hz - 100Hz, approximately 1 seconds per foi
        return ceilf(foi_hz);
    } else {
        // 100Hz and above, collect 100 cycles
        return 100;
    }
}

static void setup_freq_point(sfra_t* sfra) {
    sfra_float_t foi_hz = sfra->results.freqVect[sfra->internal_state.freqIndex];
    sfra_size_t cycles = calc_cycles(foi_hz, sfra->config.isrFreq);

    sfra->internal_state.data_count = ceilf(sfra->config.isrFreq * (float)cycles / foi_hz);

    sfra->internal_state.foi_rad = (float)cycles / (float)sfra->internal_state.data_count *
#if(SFRA_INT)
    (float) FAST_SIN_MAPPED_PI * (float) (1 << SFRA_OMEGA_DEC_BITS)
#else
    2.0F * M_PI
#endif
    ;

    sfra->internal_state.data_index = 0;
    sfra->internal_state.dtft_real_num = 0;
    sfra->internal_state.dtft_nimg_num = 0;
    sfra->internal_state.dtft_real_den = 0;
    sfra->internal_state.dtft_nimg_den = 0;
    sfra->internal_state.dtft_running = 1;
}

void sfra_background_task(sfra_t* sfra) {
    if (!sfra->internal_state.running) {
        if (sfra->internal_state.start) {
            // Start a frequency sweep
            sfra->internal_state.start = 0;
            sfra->internal_state.running = 1;
            sfra->internal_state.freqIndex = 0;
            sfra->results.freqVect[0] = sfra->config.freqStart;
            setup_freq_point(sfra);
        } else {
            return;
        }
    }

    if (sfra->internal_state.dtft_running) {
        return;
    }

    process_foi_data(sfra);

    sfra->internal_state.freqIndex++;
    if (sfra->internal_state.freqIndex < sfra->config.vecLength) {
        sfra->results.freqVect[sfra->internal_state.freqIndex] =
                sfra->results.freqVect[sfra->internal_state.freqIndex - 1] *
                sfra->config.freqStep;
        setup_freq_point(sfra);
    } else {
        // End of frequency sweep
        sfra->internal_state.running = 0;
        sfra->internal_state.done = 1;
    }
}

#if (SFRA_INT)
SFRA_RAMFUNC(sfra_inject_int32)
fast_tri_ret_type sfra_inject_int32(sfra_t* sfra) {
    if (sfra->internal_state.dtft_running) {
        fast_tri_omega_type omega = sfra->internal_state.foi_rad * sfra->internal_state.data_index;
        omega >>= SFRA_OMEGA_DEC_BITS-1;
        fast_tri_ret_type foi_cos = fast_cos(omega);
        sfra->internal_state.foi_sin = fast_sin(omega);
        sfra->internal_state.foi_cos = foi_cos;
        return foi_cos;
    } else {
        return 0;
    }
}

SFRA_RAMFUNC(sfra_monitor_int32)
void sfra_monitor_int32(sfra_t* sfra, sfra_signal_t input, sfra_signal_t output) {
    if (!sfra->internal_state.dtft_running)
        return;

    fast_tri_ret_type foi_cos = sfra->internal_state.foi_cos;
    fast_tri_ret_type foi_sin = sfra->internal_state.foi_sin;
    sfra->internal_state.dtft_real_num += (sfra_integral_t)output * (sfra_integral_t)foi_cos;
    sfra->internal_state.dtft_nimg_num += (sfra_integral_t)output * (sfra_integral_t)foi_sin;
    sfra->internal_state.dtft_real_den += (sfra_integral_t)input * (sfra_integral_t)foi_cos;
    sfra->internal_state.dtft_nimg_den += (sfra_integral_t)input * (sfra_integral_t)foi_sin;

    sfra->internal_state.data_index++;
    if (sfra->internal_state.data_index >= sfra->internal_state.data_count) {
        sfra->internal_state.dtft_running = 0;
    }
}
#else
SFRA_RAMFUNC(sfra_inject)
sfra_float_t sfra_inject(sfra_t* sfra) {
    if (sfra->internal_state.dtft_running) {
        sfra_float_t omega = sfra->internal_state.foi_rad * sfra->internal_state.data_index;
        sfra_float_t foi_cos = SFRA_FLOAT_COS(omega);
        sfra->internal_state.foi_sin = SFRA_FLOAT_SIN(omega);
        sfra->internal_state.foi_cos = foi_cos;
        return foi_cos;
    } else {
        return 0.0F;
    }
}

SFRA_RAMFUNC(sfra_monitor)
void sfra_monitor(sfra_t* sfra, sfra_float_t input, sfra_float_t output) {
    if (!sfra->internal_state.dtft_running)
        return;

    sfra_float_t foi_cos = sfra->internal_state.foi_cos;
    sfra_float_t foi_sin = sfra->internal_state.foi_sin;
    sfra->internal_state.dtft_real_num += output * foi_cos;
    sfra->internal_state.dtft_nimg_num += output * foi_sin;
    sfra->internal_state.dtft_real_den += input * foi_cos;
    sfra->internal_state.dtft_nimg_den += input * foi_sin;

    sfra->internal_state.data_index++;
    if (sfra->internal_state.data_index >= sfra->internal_state.data_count) {
        sfra->internal_state.dtft_running = 0;
    }
}
#endif

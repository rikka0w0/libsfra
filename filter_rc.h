#ifndef INC_FILTER_RC_H_
#define INC_FILTER_RC_H_

#include <math.h>

typedef struct {
    // Internal state
    float fState;

    // Parameters
    float fGain;
    float fDb1n;   // Negative of the z^-1 term on the denominator
} RCFilter_t;

/**
 * Initialize a RCFilter_t struct based on the given cut-off and sampling frequency.
 *
 * pxFilter: A pointer to a RCFilter_t struct that holds the internal state and parameters of the filter
 * fFsampling: The sampling frequency, unit is Hz
 * fFcut: The -3db cut-off frequency, unit is Hz
 */
static inline void vFilterRCInit(RCFilter_t *pxFilter, float fFsampling, float fFcut) {
    pxFilter->fGain = (M_PI * fFcut) / (M_PI * fFcut + fFsampling);
    pxFilter->fDb1n = (fFsampling - M_PI * fFcut) / (M_PI * fFcut + fFsampling);
    pxFilter->fState = 0;
}

/**
 * Calculates the next filter output, should be called at the sampling rate.
 *
 * pxFilter: A pointer to a RCFilter_t struct that holds the internal state and parameters of the filter
 * fIn: Filter input
 * Return: Filter output
 */
static inline float vFilterRCRun(RCFilter_t *pxFilter, float fIn) {
    float fStateNext = pxFilter->fState * pxFilter->fDb1n + fIn;
    float fOutputSum = pxFilter->fState + fStateNext;
    pxFilter->fState = fStateNext;
    return pxFilter->fGain * fOutputSum;
}

#endif /* INC_FILTER_RC_H_ */

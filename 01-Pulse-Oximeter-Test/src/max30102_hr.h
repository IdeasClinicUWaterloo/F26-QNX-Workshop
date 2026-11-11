/********************************************************************
 * Peak Detection: Adaptive Threshold (ADT) - Vmax
 *
 * Detects local maxima (Vmax) in the PPG waveform using an
 * adaptive threshold to identify pulse peaks for BPM calculation.
 *
 * Based on:
 *   Shin, H. S., Lee, C., & Lee, M. (2009).
 *   "Adaptive threshold method for the peak detection of
 *   photoplethysmographic waveform."
 *   Computers in Biology and Medicine, 39(12), 1145-1152.
 *
 * DOI: https://doi.org/10.1016/j.compbiomed.2009.10.006
 *
 * Implementation adapted for this project.
 ********************************************************************/

#define FreqS 25 
#define BUFFER_LEN (FreqS * 4) 

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define REFACTORY_PERIOD_SAMPLES 10

#define ALPHA 0.25 
#define IR_THRESHOLD 40000

typedef struct{
    bool initialized;
    double y1;
} IIR_Smooth_t;

typedef struct {
    bool initialized;
    double x1, x2;
    double w, w1, w2;
    double y1, y2;
} IIR_2nd_Order_t;

typedef struct{
    bool initialized;
    double y1;
    double x1;
} IIR_HPF_t;

typedef struct{
    IIR_Smooth_t iir_smooth;
    IIR_HPF_t iir_hpf; 
    IIR_2nd_Order_t iir_bpf; 
} MAX30102_DSP_t; 

typedef enum {
    IN_SENSING = 0,
    OUT_OF_SENSING,
    REFACTORY_PERIOD
} Adaptive_Threshold_State_t;

typedef struct{
    double slope;
    double threshold;
    double prev_peak;
    int refractory_sample_count;
    Adaptive_Threshold_State_t state;
} Adaptive_Threshold_t;

typedef struct{
    MAX30102_DSP_t ir_ppg_dsp;
    Adaptive_Threshold_t adt;
    double heartrate;
} MAX30102_t;

void init_max3012_heartrate(MAX30102_t *ppg);
void max3012_heartrate(MAX30102_t *ppg, int32_t *ir_data);
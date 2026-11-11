/********************************************************************
 * Peak Detection: Adaptive Threshold (ADT) - Vmax
 *
 * Description:
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
 * 
 * *********************************************************************
 * 
 *  Signal Processing Pipeline for MAX30102
 *
 * Description:
 *   Preprocesses raw PPG signals using digital filters to reduce
 *   noise and prepare the signal for peak detection.
 *
 * Reference:
 *   https://pmc.ncbi.nlm.nih.gov/articles/PMC13120635/
 *
 * Note:
 *   Signal processing approach based on the referenced paper.
 *   Implementation adapted for this project.
 ********************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <max30102_hr.h>

/*****************************************/
/*         IIR Smoothing
/*****************************************/

void init_iir_smooth(IIR_Smooth_t *iir, double start_value) {
    iir->initialized = false;
    iir->y1 = start_value;
}

double iir_smooth_update(IIR_Smooth_t *iir, double input) {
    if(!(iir->initialized)){
        init_iir_smooth(iir, input);
        iir->initialized = true;
    }

    double output = ALPHA * input + (1 - ALPHA) * iir->y1;
    iir->y1 = output;
    return output;
}

/*************************************************/
/*         High Pass Filter (Fc = 0.1 Hz)
/*************************************************/

void init_iir_hpf(IIR_HPF_t *iir, double start_value) {
    iir->initialized = false;
    iir->y1 = 0;
    iir->x1 = start_value;
}

double iir_hpf_update(IIR_HPF_t *iir, double input) {
    if(!(iir->initialized)){
        init_iir_hpf(iir, input);
        iir->initialized = true;
    }

    double output = 0.9876 * input - 0.9876 * iir->x1 + 0.9752 * iir->y1;
    iir->x1 = input;
    iir->y1 = output;
    return output;
}

/*************************************************************/
/*         Bandpass Filter (Passband: 0.1 -> 6.0 Hz)
/*************************************************************/

void iir_bpf_init(IIR_2nd_Order_t *iir) {
    iir->initialized = false;

    iir->x1 = 0;
    iir->x2 = 0;

    iir->w  = 0.0;
    iir->w1 = 0.0;
    iir->w2 = 0.0;

    iir->y1 = 0.0;
    iir->y2 = 0.0;
}

double iir_bpf_update(IIR_2nd_Order_t *iir, double input) {
    if(!(iir->initialized)){
        iir_bpf_init(iir);
        iir->initialized = true;
    }

    input *= 0.06689684;
    iir->w = 2*input - 2*iir->x2 + 1.964469518119*iir->w1 - 0.965106706108*iir->w2;
    double output = 2*iir->w - 2*iir->w2 + 0.089560544537*iir->y1 - 0.179723310739*iir->y2;

    iir->x2 = iir->x1;
    iir->x1 = input;

    iir->w2 = iir->w1;
    iir->w1 = iir->w;
    
    iir->y2 = iir->y1;
    iir->y1 = output;

    return output;
}

/*****************************************************************/
/*     Signal Processing for MAX30102 (AC components)
/*****************************************************************/

void max30102_dsp(MAX30102_DSP_t *ppg_dsp, int32_t *raw_data, double *ac_data) {

    // IIR Smoothing
    double smoothed_data[BUFFER_LEN];
    for(int i = 0; i < BUFFER_LEN; i++) {
        smoothed_data[i] = iir_smooth_update(&ppg_dsp->iir_smooth, (double)raw_data[i]);
    }

    /*****************************************/
    /* AC data
    /*****************************************/
    // High Pass Filter (Fc = 0.1 Hz)
    for(int i = 0; i < BUFFER_LEN; i++) {
        ac_data[i] = iir_hpf_update(&ppg_dsp->iir_hpf, smoothed_data[i]);
    }

    // Bandpass Filter (Passband: 0.1 -> 6.0 Hz)
    for(int i = 0; i < BUFFER_LEN; i++) {
        ac_data[i] = iir_bpf_update(&ppg_dsp->iir_bpf, ac_data[i]);
    }
}

/*****************************************************************/
/*         Peak Detection: Adaptive Threshold (Shin)
/*****************************************************************/

double max_ppg(const double *samples) {
    double ppg = samples[0]; 
    for(int i = 1; i < BUFFER_LEN; i++) ppg = MAX(ppg, samples[i]);
    return ppg;
}

double standard_deviation(double *samples) {
    double mean = 0.0;
    for(int i = 0; i < BUFFER_LEN; i++)  mean += samples[i];
    mean /= BUFFER_LEN;

    double var = 0.0;
    for(int i = 0; i < BUFFER_LEN; i++) {
        double d = samples[i] - mean;
        var += d * d;
    }
    var /= BUFFER_LEN;

    return sqrt(var);
}

void init_adt(Adaptive_Threshold_t *adt, double *samples) {
    double peak_ppg = max_ppg(samples);
    adt->slope = 0.2 * peak_ppg;
    adt->threshold = 1.2 * peak_ppg;

    adt->prev_peak = max_ppg(samples);
    adt->state = OUT_OF_SENSING;
    adt->refractory_sample_count = 0; 
}

int adaptive_threshold(Adaptive_Threshold_t *adt, double *samples, int *peaks) {
    double std_ppg = standard_deviation(samples);
    int peak_index = 0;

    for(int i = 0; i < BUFFER_LEN; i++) {

        switch(adt->state) {
            case(OUT_OF_SENSING):
                // Decrease the threshold with time using the current slope
                adt->threshold -= adt->slope;

                // When the threshold meets the PPG signal
                if(adt->threshold <= samples[i]) adt->state = IN_SENSING;

                break; 

            case(IN_SENSING):
                // Threshold follows the PPG waveform
                adt->threshold = samples[i];
                if(i < 2) continue;

                bool  vmax = samples[i-1] > samples[i-2] &&samples[i-1] > samples[i];
                if(vmax) {
                    
                    // Update slope and V_n-1
                    adt->slope = adt->slope - 0.6 * (std_ppg + adt->prev_peak) / (double)FreqS;
                    adt->prev_peak = samples[i-1];

                    // Save peaks
                    peaks[peak_index] = i-1;
                    peak_index++;

                    adt->state = REFACTORY_PERIOD;

                }

                break;
            
            case(REFACTORY_PERIOD):
                adt->refractory_sample_count++;

                if(adt->refractory_sample_count >= REFACTORY_PERIOD_SAMPLES) {
                    adt->state = OUT_OF_SENSING;
                    adt->refractory_sample_count = 0; 
                }
                //printf("IR: %f, Peak: no\n", samples[i-1]);
                break; 
        }
    }

    return peak_index; 
}

/***********************************************/
/*              HEARTRATE (BPM)
/***********************************************/

double heartrate(int *peaks, int size) {
    if (size < 2) return NAN;

    double heartrate_sum = 0;
    for (int i = 0; i < size - 1; i++) {

        // Calculate IBI in samples
        int interval = peaks[i+1] - peaks[i];

        if (interval <= 0) return NAN;

        // Convert interval to BPM
        double bpm = (60.0 * FreqS) / interval;

        heartrate_sum += bpm;
    }

    return heartrate_sum / (size - 1);
}

/***********************************************/
/*             Heartrate
/***********************************************/
void init_max3012_heartrate(MAX30102_t *ppg) {
    ppg->ir_ppg_dsp.iir_smooth.initialized = false;
    ppg->ir_ppg_dsp.iir_hpf.initialized = false;
    ppg->ir_ppg_dsp.iir_bpf.initialized = false;
}

void max3012_heartrate(MAX30102_t *ppg, int32_t *ir_data){
    double ir_ac[BUFFER_LEN];

    // Clean up ir signals before heartrate calculations 
    max30102_dsp(&(ppg->ir_ppg_dsp), ir_data, ir_ac);

    // Peak detection for heartrate
    init_adt(&(ppg->adt), ir_ac);
    int peaks[BUFFER_LEN];
    int peaks_size = adaptive_threshold(&(ppg->adt), ir_ac, peaks);

    // BPM calculation
    (ppg->heartrate) = heartrate(peaks, peaks_size);
}
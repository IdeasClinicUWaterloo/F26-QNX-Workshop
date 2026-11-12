#include <stdbool.h>
#include <stdint.h>
#include <max30102_dsp.h>

void init_iir(IIR_4th_t *iir) {
    iir->xn = 0;
    iir->xn_1 = 0;
    iir->xn_2 = 0;

    iir->yn = 0;
    iir->yn_1 = 0;
    iir->yn_2 = 0;
}

void init_bpf(IIR_4th_t iir[2]) {
    // Section 1
    iir[0].b0 = 0.002985392180862;
    iir[0].b1 = 0;
    iir[0].b2 = -0.002985392180862;

    iir[0].a1 = -1.967238881055;
    iir[0].a2 = 0.9680018039918;

    // Section 2
    iir[1].b0 = 2.0;
    iir[1].b1 = 0;
    iir[1].b2 = -2.0;

    iir[1].a1 = -1.793652442395;
    iir[1].a2 = 0.8199561260632;

    init_iir(&iir[0]);
    init_iir(&iir[1]);
}

double update_iir(IIR_4th_t *iir, double x) {
    double y = iir->b0*x
             + iir->b1*iir->xn_1
             + iir->b2*iir->xn_2
             - iir->a1*iir->yn_1
             - iir->a2*iir->yn_2;

    // Update history
    iir->xn_2 = iir->xn_1;
    iir->xn_1 = x;
    iir->xn = x;

    iir->yn_2 = iir->yn_1;
    iir->yn_1 = y;
    iir->yn = y;

    return y;
}

int32_t update_bpf(IIR_4th_t iir[2], int32_t input) {
    double y1 = update_iir(&iir[0], (double)input);
    double y2 = update_iir(&iir[1], y1);

    return (int32_t)y2;
}
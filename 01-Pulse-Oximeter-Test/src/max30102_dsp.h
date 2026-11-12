#include <stdbool.h>
#include <stdint.h>

typedef struct {
    double b0, b1, b2, a1, a2;
    double xn, xn_1, xn_2;
    double yn, yn_1, yn_2;
} IIR_4th_t;

void init_bpf(IIR_4th_t iir[2]);
int32_t update_bpf(IIR_4th_t iir[2], int32_t input);
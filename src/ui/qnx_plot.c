/*
 * qnx_plot.c -- Live MAX30102 IR/RED plotter for LVGL 9.4 + QNX Screen.
 *
 * stdin lines:
 *     ir=54231, red=48120, spo2=98, bpm=75
 *
 * Run on the Raspberry Pi (QNX):
 *     ./pulse-oximeter | ./qnx_plot
 *
 * The program must run on QNX with Screen configured for HDMI.
 * All input reading and LVGL rendering occur in the LVGL event-loop thread.
 */

#include <lvgl.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SCREEN_W         800
#define SCREEN_H         480
#define CHART_POINTS     500     /* 2.5 seconds at 200 samples/second */
#define UPDATE_MS        40      /* Read stdin / refresh display at 25 Hz */
#define READ_CHUNK       4096
#define READS_PER_TICK   32      /* Bound work done per LVGL timer callback */
#define LINE_SIZE        160

typedef struct {
    lv_obj_t *chart;
    lv_obj_t *readout;
    lv_chart_series_t *series;
} trace_t;

static trace_t ir_trace;
static trace_t red_trace;
static lv_obj_t *spo2_label;
static lv_obj_t *bpm_label;
static lv_obj_t *status_label;

static char line[LINE_SIZE];
static size_t line_length = 0;
static bool dropping_line = false;
static bool input_ended = false;
static bool received_any = false;
static bool updated_this_tick = false;

static int latest_ir = 0;
static int latest_red = 0;
static int latest_spo2 = -999;
static int latest_bpm = -999;
static unsigned long long sample_count = 0;

/* LVGL's chart stores the last CHART_POINTS values, in shift mode.
 * We scan them to fit the visible vertical range to each signal. */
static void autoscale_trace(trace_t *trace)
{
    int32_t *values = lv_chart_get_series_y_array(trace->chart,
                                                    trace->series);
    if (values == NULL) return;

    int64_t lo = INT32_MAX;
    int64_t hi = INT32_MIN;
    for (int i = 0; i < CHART_POINTS; ++i) {
        int32_t value = values[i];
        if (value == LV_CHART_POINT_NONE) continue;
        if (value < lo) lo = value;
        if (value > hi) hi = value;
    }
    if (lo > hi) return;

    int64_t padding = (hi - lo) / 8;
    if (padding < 10) padding = 10;
    lo -= padding;
    hi += padding;

    if (lo < INT32_MIN) lo = INT32_MIN;
    if (hi > INT32_MAX) hi = INT32_MAX;
    if (lo >= hi) return;

    lv_chart_set_axis_range(trace->chart, LV_CHART_AXIS_PRIMARY_Y,
                            (int32_t)lo, (int32_t)hi);
}

/* Parse one complete sample and add both values to the charts.
 * The printf() format must match the format in sscanf(). */
static void parse_line(const char *s)
{
    int ir, red, spo2, bpm;
    if (sscanf(s, "ir=%d, red=%d, spo2=%d, bpm=%d",
               &ir, &red, &spo2, &bpm) != 4) {
        return; /* Ignore unrelated diagnostic lines. */
    }

    latest_ir = ir;
    latest_red = red;
    latest_spo2 = spo2;
    latest_bpm = bpm;
    received_any = true;
    updated_this_tick = true;
    sample_count++;

    /* Don't discard intermediate samples between graphical refreshes. */
    lv_chart_set_next_value(ir_trace.chart, ir_trace.series, ir);
    lv_chart_set_next_value(red_trace.chart, red_trace.series, red);
}

static void receive_bytes(const char *bytes, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        char c = bytes[i];
        if (c == '\n') {
            if (!dropping_line) {
                line[line_length] = '\0';
                parse_line(line);
            }
            line_length = 0;
            dropping_line = false;
        }
        else if (!dropping_line) {
            if (line_length < sizeof(line) - 1) {
                line[line_length++] = c;
            }
            else {
                /* Ignore an overlong line entirely. */
                dropping_line = true;
            }
        }
    }
}

/* Nonblocking stdin prevents the QNX Screen GUI from freezing while
 * waiting for the next sensor reading. */
static void receive_from_stdin(void)
{
    char bytes[READ_CHUNK];
    for (int attempt = 0; attempt < READS_PER_TICK; ++attempt) {
        ssize_t n = read(STDIN_FILENO, bytes, sizeof(bytes));
        if (n > 0) {
            receive_bytes(bytes, (size_t)n);
        }
        else if (n == 0) {
            input_ended = true; /* Pipe was closed; keep graph visible. */
            break;
        }
        else if (errno == EINTR) {
            continue;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }
        else {
            perror("read(stdin)");
            input_ended = true;
            break;
        }
    }
}

static void timer_callback(lv_timer_t *timer)
{
    (void)timer;
    updated_this_tick = false;
    if (!input_ended) receive_from_stdin();

    if (updated_this_tick) {
        autoscale_trace(&ir_trace);
        autoscale_trace(&red_trace);
        lv_label_set_text_fmt(ir_trace.readout, "IR: %d", latest_ir);
        lv_label_set_text_fmt(red_trace.readout, "RED: %d", latest_red);

        if (latest_spo2 >= 0 && latest_spo2 <= 100)
            lv_label_set_text_fmt(spo2_label, "SpO2  %d%%", latest_spo2);
        else
            lv_label_set_text(spo2_label, "SpO2  --");

        if (latest_bpm > 0)
            lv_label_set_text_fmt(bpm_label, "BPM  %d", latest_bpm);
        else
            lv_label_set_text(bpm_label, "BPM  --");
    }

    if (input_ended) {
        lv_label_set_text(status_label, "INPUT ENDED");
    }
    else if (received_any) {
        lv_label_set_text_fmt(status_label, "LIVE  (%llu samples)",
                              sample_count);
    }
    /* Otherwise status remains 'Waiting for sensor data'. */
}

static void create_trace(trace_t *trace, const char *title,
                         int title_y, int chart_y, uint32_t color)
{
    lv_obj_t *screen = lv_screen_active();

    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_color(title_label, lv_color_hex(color), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 24, title_y);

    trace->readout = lv_label_create(screen);
    lv_label_set_text(trace->readout, "Waiting...");
    lv_obj_set_style_text_color(trace->readout,
                                lv_color_hex(0xDDE5ED), 0);
    lv_obj_align(trace->readout, LV_ALIGN_TOP_RIGHT, -24, title_y);

    trace->chart = lv_chart_create(screen);
    lv_obj_set_size(trace->chart, 750, 128);
    lv_obj_align(trace->chart, LV_ALIGN_TOP_MID, 0, chart_y);
    lv_obj_set_style_bg_color(trace->chart,
                              lv_color_hex(0x1B2735), 0);
    lv_obj_set_style_line_width(trace->chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(trace->chart, 0, 0, LV_PART_INDICATOR);

    lv_chart_set_type(trace->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(trace->chart, CHART_POINTS);
    lv_chart_set_update_mode(trace->chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_axis_range(trace->chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(trace->chart, 3, 5);

    trace->series = lv_chart_add_series(trace->chart,
                        lv_color_hex(color), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_values(trace->chart, trace->series,
                            LV_CHART_POINT_NONE);
}

int main(void)
{
    lv_init();
    lv_display_t *display = lv_qnx_window_create(SCREEN_W, SCREEN_H);
    if (display == NULL) {
        fprintf(stderr, "Failed to create a QNX Screen window.\n");
        return 1;
    }
    lv_qnx_window_set_title(display, "QNX MAX30102 Monitor");

    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101827), 0);

    lv_obj_t *heading = lv_label_create(screen);
    lv_label_set_text(heading, "MAX30102 - QNX Live Monitor");
    lv_obj_set_style_text_color(heading, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(heading, LV_ALIGN_TOP_LEFT, 24, 14);

    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Waiting for sensor data");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xB7C4D2), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_RIGHT, -24, 16);

    spo2_label = lv_label_create(screen);
    lv_label_set_text(spo2_label, "SpO2  --");
    lv_obj_set_style_text_color(spo2_label,
                                lv_color_hex(0x79B8FF), 0);
    lv_obj_align(spo2_label, LV_ALIGN_TOP_LEFT, 24, 66);

    bpm_label = lv_label_create(screen);
    lv_label_set_text(bpm_label, "BPM  --");
    lv_obj_set_style_text_color(bpm_label,
                                lv_color_hex(0xFFD479), 0);
    lv_obj_align(bpm_label, LV_ALIGN_TOP_RIGHT, -24, 66);

    create_trace(&ir_trace, "Infrared (IR)", 112, 138, 0x34D399);
    create_trace(&red_trace, "Red light", 284, 310, 0xFB7185);

    lv_obj_t *footer = lv_label_create(screen);
    lv_label_set_text(footer, "QNX Screen / LVGL 9.4 | live stdin");
    lv_obj_set_style_text_color(footer, lv_color_hex(0xB7C4D2), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_LEFT, 24, -10);

    int flags = fcntl(STDIN_FILENO, F_GETFL);
    if (flags == -1 ||
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(stdin)");
        return 1;
    }

    lv_timer_create(timer_callback, UPDATE_MS, NULL);
    return lv_qnx_event_loop(display);
}

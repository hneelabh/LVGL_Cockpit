/*******************************************************************
 *
 * main.c - LVGL Modern Motorcycle Speedometer for RPi 4B + 5" HDMI
 *
 * Enhanced version with turn signals and color/timing fixes
 *
 ******************************************************************/
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

/* Internal functions */
static void configure_simulator(int argc, char ** argv);
static void print_lvgl_version(void);
static void print_usage(void);

/* contains the name of the selected backend if user
 * has specified one on the command line */
static char * selected_backend;

/* Global simulator settings, defined in lv_linux_backend.c */
extern simulator_settings_t settings;

/* ==================================================================== */
/* MODERN PICO-PORT COCKPIT DEMO CODE START                             */
/* ==================================================================== */

// Global UI Pointers
#define SEGMENT_COUNT 40  // Number of segments (matching original Pico)
lv_obj_t * speed_arc_segments[SEGMENT_COUNT];  // Array of individual arc segments
lv_obj_t * speed_label;
lv_obj_t * gear_label;
lv_obj_t * rpm_label;
lv_obj_t * n_indicator;
lv_obj_t * left_turn_signal;
lv_obj_t * right_turn_signal;

// Colors - GREEN → YELLOW → ORANGE → RED speed zones
#define COLOR_DARK_BG    lv_color_hex(0x101010)
#define COLOR_PANEL_BG   lv_color_hex(0x202025)
#define COLOR_DARK_GREY  lv_color_hex(0x555555)
#define COLOR_NEON_BLUE  lv_color_hex(0x00BFFF)
#define COLOR_NEON_GREEN lv_color_hex(0x32CD32)
#define COLOR_NEON_YEL   lv_color_hex(0xFFFF00)
#define COLOR_NEON_ORG   lv_color_hex(0xFFA500)
#define COLOR_NEON_RED   lv_color_hex(0xFF4500)

// Helper: Calculate Gear (from original)
static int calculate_gear(int speed) {
    if (speed == 0) return 0;  // Neutral
    if (speed < 25) return 1;
    if (speed < 50) return 2;
    if (speed < 80) return 3;
    if (speed < 120) return 4;
    if (speed < 160) return 5;
    return 6;
}

// Helper: Calculate RPM (FIXED from original - realistic values)
static int calculate_rpm(int speed, int gear) {
    if (gear == 0 || speed == 0) return 1;  // Idle RPM
    
    float base_rpm;
    switch(gear) {
        case 1: base_rpm = speed * 0.20; break;   // High RPM in 1st
        case 2: base_rpm = speed * 0.12; break;
        case 3: base_rpm = speed * 0.08; break;
        case 4: base_rpm = speed * 0.06; break;
        case 5: base_rpm = speed * 0.05; break;
        case 6: base_rpm = speed * 0.04; break;   // Low RPM in 6th
        default: base_rpm = speed * 0.06; break;
    }
    
    int rpm = (int)(base_rpm + 2.0);  // Add minimum RPM
    return (rpm < 1) ? 1 : ((rpm > 13) ? 13 : rpm);
}

// Helper: Get speed zone color - GREEN → YELLOW → ORANGE → RED
static lv_color_t get_speed_color(int speed) {
    if (speed > 160) return COLOR_NEON_RED;     // 160-200: RED
    if (speed > 120) return COLOR_NEON_ORG;     // 120-160: ORANGE
    if (speed > 60)  return COLOR_NEON_YEL;     // 60-120: YELLOW
    return COLOR_NEON_GREEN;                    // 0-60: GREEN
}

// Turn signal blink state
static bool left_blink_state = false;
static bool right_blink_state = false;
static int blink_counter = 0;

// Turn signal blink task (500ms on/off cycle)
static void turn_signal_blink_task(lv_timer_t * timer)
{
    (void)timer;  // Unused parameter
    blink_counter++;
    
    // Toggle every 12 ticks (~500ms at 40ms intervals)
    if (blink_counter >= 12) {
        blink_counter = 0;
        
        // Left turn signal
        if (left_blink_state) {
            static bool left_on = false;
            left_on = !left_on;
            lv_obj_set_style_text_color(left_turn_signal, 
                left_on ? COLOR_NEON_GREEN : COLOR_DARK_GREY, 0);
        }
        
        // Right turn signal
        if (right_blink_state) {
            static bool right_on = false;
            right_on = !right_on;
            lv_obj_set_style_text_color(right_turn_signal, 
                right_on ? COLOR_NEON_GREEN : COLOR_DARK_GREY, 0);
        }
    }
}

// The Simulation State Machine (Replaces the Pico while(1) loops)
static void engine_sim_task(lv_timer_t * timer)
{
    (void)timer;  // Unused parameter
    static int speed = 0;
    static int state = 0; // 0:Accel, 1:Brake, 2:SportAccel, 3:FullStop
    static int pause_timer = 0;

    // Handle Gear Shift Pauses
    if (pause_timer > 0) {
        pause_timer--;
        return; // Skip speed update during pause
    }

    // State Machine logic (matching original demo flow)
    switch(state) {
        case 0: // Normal Accel
            speed += 3;
            // Gear shifts trigger pause (FIXED: exact gear change points)
            if (speed == 24 || speed == 48 || speed == 78 || speed == 117 || speed == 156) {
                pause_timer = 5;  // 5 frames * 40ms = 200ms pause
            }
            if (speed >= 180) { 
                speed = 180; 
                state = 1; 
                pause_timer = 50;  // 2 second wait
            }
            break;
            
        case 1: // Brake
            speed -= 5;
            if (speed <= 0) { 
                speed = 0; 
                state = 2; 
                pause_timer = 25;  // 1 second wait
            }
            break;
            
        case 2: // Sport Mode Accel
            speed += 5;
            if (speed >= 200) { 
                speed = 200; 
                state = 3; 
                pause_timer = 50;  // 2 second wait
            }
            break;
            
        case 3: // Full Stop
            speed -= 4;
            if (speed <= 0) { 
                speed = 0; 
                state = 0; 
                pause_timer = 75;  // 3 second wait
            }
            break;
    }

    // Simulate turn signals during demo
    // Left signal during acceleration, right during braking
    if (state == 0 || state == 2) {
        left_blink_state = true;
        right_blink_state = false;
    } else if (state == 1 || state == 3) {
        left_blink_state = false;
        right_blink_state = true;
    }

    // 1. Calculate Values
    int gear = calculate_gear(speed);
    int rpm = calculate_rpm(speed, gear);

    // 2. Get appropriate color based on speed zone
    lv_color_t zone_color = get_speed_color(speed);

    // 3. Update the UI
    
    // Calculate how many segments should be lit
    int active_segments = (speed * SEGMENT_COUNT) / 200;  // 0-40 segments
    
    // Speed text
    lv_label_set_text_fmt(speed_label, "%d", speed);
    
    // Update each segment
    for (int i = 0; i < SEGMENT_COUNT; i++) {
        if (i < active_segments) {
            // Active segment - use speed zone color
            lv_obj_set_style_arc_color(speed_arc_segments[i], zone_color, LV_PART_INDICATOR);
        } else {
            // Inactive segment - dark grey
            lv_obj_set_style_arc_color(speed_arc_segments[i], lv_color_hex(0x1E1E28), LV_PART_INDICATOR);
        }
    }
    
    // Speed label color
    lv_obj_set_style_text_color(speed_label, zone_color, 0);

    // Gear & Neutral indicator update
    if (gear == 0) {
        lv_label_set_text(gear_label, "N");
        lv_obj_set_style_text_color(gear_label, COLOR_NEON_GREEN, 0);
        lv_obj_set_style_text_color(n_indicator, COLOR_NEON_GREEN, 0);
    } else {
        lv_label_set_text_fmt(gear_label, "%d", gear);
        lv_obj_set_style_text_color(gear_label, zone_color, 0);
        lv_obj_set_style_text_color(n_indicator, COLOR_DARK_GREY, 0);
    }

    // RPM update with color coding
    lv_label_set_text_fmt(rpm_label, "%d", rpm);
    lv_color_t rpm_color = (rpm > 10) ? COLOR_NEON_RED : 
                          (rpm > 7)  ? COLOR_NEON_ORG : 
                                       COLOR_NEON_GREEN;
    lv_obj_set_style_text_color(rpm_label, rpm_color, 0);
}

// Draw the UI (Optimized for 800x480)
void create_cockpit_ui(void)
{
    // Set dark background
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_DARK_BG, LV_PART_MAIN);

    // --- TOP PANEL (ODO / TRIP / FUEL / N) ---
    lv_obj_t * top_panel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(top_panel, 800, 70);
    lv_obj_set_style_bg_color(top_panel, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(top_panel, COLOR_DARK_GREY, 0);
    lv_obj_set_style_border_width(top_panel, 1, 0);
    lv_obj_align(top_panel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(top_panel, LV_OBJ_FLAG_SCROLLABLE);

    // ODO Label
    lv_obj_t * odo_lbl = lv_label_create(top_panel);
    lv_label_set_text(odo_lbl, "ODO\n#FFFFFF 12345#");
    lv_obj_set_style_text_color(odo_lbl, COLOR_DARK_GREY, 0);
    lv_label_set_recolor(odo_lbl, true);
    lv_obj_align(odo_lbl, LV_ALIGN_LEFT_MID, 20, 0);

    // TRIP Label
    lv_obj_t * trip_lbl = lv_label_create(top_panel);
    lv_label_set_text(trip_lbl, "TRIP\n#FFFFFF 123.4#");
    lv_obj_set_style_text_color(trip_lbl, COLOR_DARK_GREY, 0);
    lv_label_set_recolor(trip_lbl, true);
    lv_obj_align(trip_lbl, LV_ALIGN_LEFT_MID, 150, 0);

    // FUEL Label
    lv_obj_t * fuel_lbl = lv_label_create(top_panel);
    lv_label_set_text(fuel_lbl, "FUEL\n#FFFFFF [#####  ]#");
    lv_obj_set_style_text_color(fuel_lbl, COLOR_DARK_GREY, 0);
    lv_label_set_recolor(fuel_lbl, true);
    lv_obj_align(fuel_lbl, LV_ALIGN_RIGHT_MID, -100, 0);

    // Neutral Indicator (top right)
    n_indicator = lv_label_create(top_panel);
    lv_label_set_text(n_indicator, "N");
    lv_obj_set_style_text_font(n_indicator, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(n_indicator, COLOR_DARK_GREY, 0);
    lv_obj_align(n_indicator, LV_ALIGN_RIGHT_MID, -20, 0);

    // --- TURN SIGNALS (Below top panel, left and right) ---
    
    // Left Turn Signal
    left_turn_signal = lv_label_create(lv_scr_act());
    lv_label_set_text(left_turn_signal, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(left_turn_signal, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(left_turn_signal, COLOR_DARK_GREY, 0);
    lv_obj_align(left_turn_signal, LV_ALIGN_TOP_LEFT, 50, 90);

    // Right Turn Signal
    right_turn_signal = lv_label_create(lv_scr_act());
    lv_label_set_text(right_turn_signal, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(right_turn_signal, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(right_turn_signal, COLOR_DARK_GREY, 0);
    lv_obj_align(right_turn_signal, LV_ALIGN_TOP_RIGHT, -50, 90);

    // --- SEGMENTED SPEED ARC (40 segments like original Pico) ---
    int arc_center_x = 400;  // Center X coordinate
    int arc_center_y = 310;  // Center Y coordinate (adjusted for 800x480)
    int arc_radius = 190;
    int arc_width = 24;
    
    float start_angle = 135.0;  // Starting angle (bottom left)
    float total_sweep = 270.0;  // Total arc sweep (270 degrees)
    float segment_angle = total_sweep / SEGMENT_COUNT;  // ~6.75 degrees per segment
    float gap_angle = 2.0;  // Gap between segments
    
    // Create all 40 segments
    for (int i = 0; i < SEGMENT_COUNT; i++) {
        speed_arc_segments[i] = lv_arc_create(lv_scr_act());
        
        // Calculate this segment's angles
        float seg_start = start_angle + (i * segment_angle);
        float seg_end = seg_start + segment_angle - gap_angle;
        
        // Position and size
        lv_obj_set_size(speed_arc_segments[i], arc_radius * 2, arc_radius * 2);
        lv_obj_set_pos(speed_arc_segments[i], arc_center_x - arc_radius, arc_center_y - arc_radius);
        
        // Set angles for this segment
        lv_arc_set_rotation(speed_arc_segments[i], (int)seg_start);
        lv_arc_set_bg_angles(speed_arc_segments[i], 0, (int)(seg_end - seg_start));
        
        // Set to max value so it's always "on" (color controlled separately)
        lv_arc_set_range(speed_arc_segments[i], 0, 100);
        lv_arc_set_value(speed_arc_segments[i], 100);
        
        // Styling - CRITICAL: Make background completely transparent!
        lv_obj_set_style_bg_opa(speed_arc_segments[i], LV_OPA_TRANSP, 0);  // Transparent background
        lv_obj_set_style_arc_opa(speed_arc_segments[i], LV_OPA_TRANSP, LV_PART_MAIN);  // Hide main arc
        lv_obj_set_style_arc_color(speed_arc_segments[i], lv_color_hex(0x1E1E28), LV_PART_INDICATOR);  // Start inactive
        lv_obj_set_style_arc_width(speed_arc_segments[i], arc_width, LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(speed_arc_segments[i], false, LV_PART_INDICATOR);  // Sharp edges
        lv_obj_remove_style(speed_arc_segments[i], NULL, LV_PART_KNOB);  // Remove knob
        
        // Make it non-clickable
        lv_obj_clear_flag(speed_arc_segments[i], LV_OBJ_FLAG_CLICKABLE);
    }

    // --- DIGITAL SPEEDOMETER (Center) ---
    lv_obj_t * speed_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(speed_bg, 200, 100);
    lv_obj_set_style_bg_color(speed_bg, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(speed_bg, COLOR_DARK_GREY, 0);
    lv_obj_set_style_border_width(speed_bg, 2, 0);
    lv_obj_align(speed_bg, LV_ALIGN_CENTER, 0, 20);
    lv_obj_clear_flag(speed_bg, LV_OBJ_FLAG_SCROLLABLE);

    speed_label = lv_label_create(speed_bg);
    lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(speed_label, "0");
    lv_obj_set_style_text_color(speed_label, COLOR_NEON_RED, 0);
    lv_obj_align(speed_label, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t * kmh_label = lv_label_create(speed_bg);
    lv_label_set_text(kmh_label, "km/h");
    lv_obj_set_style_text_color(kmh_label, COLOR_DARK_GREY, 0);
    lv_obj_align(kmh_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    // --- GEAR INDICATOR (Bottom center) ---
    lv_obj_t * gear_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(gear_bg, 100, 90);
    lv_obj_set_style_bg_color(gear_bg, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(gear_bg, COLOR_DARK_GREY, 0);
    lv_obj_set_style_border_width(gear_bg, 2, 0);
    lv_obj_align(gear_bg, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_clear_flag(gear_bg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * gear_text = lv_label_create(gear_bg);
    lv_label_set_text(gear_text, "GEAR");
    lv_obj_set_style_text_color(gear_text, COLOR_DARK_GREY, 0);
    lv_obj_align(gear_text, LV_ALIGN_TOP_MID, 0, 5);

    gear_label = lv_label_create(gear_bg);
    lv_obj_set_style_text_font(gear_label, &lv_font_montserrat_36, 0);
    lv_label_set_text(gear_label, "N");
    lv_obj_set_style_text_color(gear_label, COLOR_NEON_GREEN, 0);
    lv_obj_align(gear_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    // --- BOTTOM LEFT: RPM ---
    lv_obj_t * rpm_info = lv_label_create(lv_scr_act());
    lv_label_set_text(rpm_info, "RPM\nx1000");
    lv_obj_set_style_text_color(rpm_info, COLOR_DARK_GREY, 0);
    lv_obj_align(rpm_info, LV_ALIGN_BOTTOM_LEFT, 40, -50);

    rpm_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(rpm_label, &lv_font_montserrat_36, 0);
    lv_label_set_text(rpm_label, "1");
    lv_obj_set_style_text_color(rpm_label, COLOR_NEON_GREEN, 0);
    lv_obj_align(rpm_label, LV_ALIGN_BOTTOM_LEFT, 120, -45);

    // --- BOTTOM RIGHT: TEMP ---
    lv_obj_t * temp_info = lv_label_create(lv_scr_act());
    lv_label_set_text(temp_info, "TEMP\n---C");
    lv_obj_set_style_text_color(temp_info, COLOR_DARK_GREY, 0);
    lv_obj_align(temp_info, LV_ALIGN_BOTTOM_RIGHT, -40, -50);

    // --- Start Simulation Timers ---
    // Engine simulation: 40ms (25 FPS, matching original Pico code)
    lv_timer_create(engine_sim_task, 40, NULL);
    
    // Turn signal blink: 40ms (checks every 40ms, toggles every 500ms)
    lv_timer_create(turn_signal_blink_task, 40, NULL);
}

/* ==================================================================== */
/* MODERN PICO-PORT COCKPIT DEMO CODE END                               */
/* ==================================================================== */

/**
 * @brief Print LVGL version
 */
static void print_lvgl_version(void)
{
    fprintf(stdout, "%d.%d.%d-%s\n",
            LVGL_VERSION_MAJOR,
            LVGL_VERSION_MINOR,
            LVGL_VERSION_PATCH,
            LVGL_VERSION_INFO);
}

/**
 * @brief Print usage information
 */
static void print_usage(void)
{
    fprintf(stdout, "\nlvglsim [-V] [-B] [-f] [-m] [-b backend_name] [-W window_width] [-H window_height]\n\n");
    fprintf(stdout, "-V print LVGL version\n");
    fprintf(stdout, "-B list supported backends\n");
    fprintf(stdout, "-f fullscreen\n");
    fprintf(stdout, "-m maximize\n");
}

/**
 * @brief Configure simulator
 */
static void configure_simulator(int argc, char ** argv)
{
    int opt = 0;

    selected_backend = NULL;
    driver_backends_register();

    const char * env_w = getenv("LV_SIM_WINDOW_WIDTH");
    const char * env_h = getenv("LV_SIM_WINDOW_HEIGHT");
    /* Default values for 800x480 display */
    settings.window_width = atoi(env_w ? env_w : "800");
    settings.window_height = atoi(env_h ? env_h : "480");

    /* Parse the command-line options. */
    while((opt = getopt(argc, argv, "b:fmW:H:BVh")) != -1) {
        switch(opt) {
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
                break;
            case 'V':
                print_lvgl_version();
                exit(EXIT_SUCCESS);
                break;
            case 'B':
                driver_backends_print_supported();
                exit(EXIT_SUCCESS);
                break;
            case 'b':
                if(driver_backends_is_supported(optarg) == 0) {
                    die("error no such backend: %s\n", optarg);
                }
                selected_backend = strdup(optarg);
                break;
            case 'f':
                settings.fullscreen = true;
                break;
            case 'm':
                settings.maximize = true;
                break;
            case 'W':
                settings.window_width = atoi(optarg);
                break;
            case 'H':
                settings.window_height = atoi(optarg);
                break;
            case ':':
                print_usage();
                die("Option -%c requires an argument.\n", optopt);
                break;
            case '?':
                print_usage();
                die("Unknown option -%c.\n", optopt);
        }
    }
}

/**
 * @brief entry point
 */
int main(int argc, char ** argv)
{
    configure_simulator(argc, argv);

    /* Initialize LVGL. */
    lv_init();

    /* Initialize the configured backend */
    if(driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }

    /* Enable for EVDEV support */
#if LV_USE_EVDEV
    if(driver_backends_init_backend("EVDEV") == -1) {
        die("Failed to initialize evdev");
    }
#endif

    /* ------------------------------------------------------------- */
    /* Start the Custom Cockpit UI                                  */
    /* ------------------------------------------------------------- */
    create_cockpit_ui();

    /* Enter the run loop of the selected backend */
    driver_backends_run_loop();

    return 0;
}
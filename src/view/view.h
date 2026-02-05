/**
 * @file view.h
 * @brief View layer - UI components and rendering
 * 
 * Handles all LVGL UI creation and updates
 */

#ifndef VIEW_H
#define VIEW_H

#include "lvgl/lvgl.h"
#include "model.h"

/* ========================================================================
 * Constants
 * ======================================================================== */

#define SEGMENT_COUNT 40  // Number of arc segments

/* Color definitions - GREEN → YELLOW → ORANGE → RED */
#define COLOR_DARK_BG    lv_color_hex(0x101010)
#define COLOR_PANEL_BG   lv_color_hex(0x202025)
#define COLOR_DARK_GREY  lv_color_hex(0x555555)
#define COLOR_NEON_BLUE  lv_color_hex(0x00BFFF)
#define COLOR_NEON_GREEN lv_color_hex(0x32CD32)
#define COLOR_NEON_YEL   lv_color_hex(0xFFFF00)
#define COLOR_NEON_ORG   lv_color_hex(0xF17600)
#define COLOR_NEON_RED   lv_color_hex(0xCA0000)

/* ========================================================================
 * Data Structures
 * ======================================================================== */

/**
 * @brief View components structure - holds all UI element references
 */
typedef struct {
    lv_obj_t *master_container;                   // MAster COntainer for the UI
    lv_obj_t *splash_container;                   // Logo Boot-up Sequence for the UI

    // --- NEW: The Swipe Container ---
    lv_obj_t * tileview;
    lv_obj_t * tile_gauges;      // Page 1
    lv_obj_t * tile_music;       // Page 2

    lv_obj_t *speed_arc_segments[SEGMENT_COUNT];  // Segmented arc display
    lv_obj_t *speed_label;                        // Digital speed number
    lv_obj_t *gear_label;                         // Gear indicator
    lv_obj_t *rpm_label;                          // RPM display
    lv_obj_t *n_indicator;                        // Neutral light
    lv_obj_t *left_turn_signal;                   // Left turn arrow
    lv_obj_t *right_turn_signal;                  // Right turn arrow
    lv_obj_t *odo_label;                          // Odometer
    lv_obj_t *trip_label;                         // Trip meter
    lv_obj_t *fuel_label;                         // Fuel gauge
    lv_obj_t *temp_label;                         // Temperature

    // Now on page 2
    lv_obj_t * music_cont;                        // Music Content
    lv_obj_t * label_title;                       // Music Title
    lv_obj_t * label_artist;                      // Music Artist
    lv_obj_t * label_album;                       // Music Album
} view_components_t;

/* ========================================================================
 * Function Prototypes
 * ======================================================================== */

/**
 * @brief Initialize and create all UI components
 * @param components Pointer to view components structure
 */
void view_init(view_components_t *components);

/**
 * @brief Update speed display (arc and digital)
 * @param components Pointer to view components
 * @param state Current speedometer state
 */
void view_update_speed(view_components_t *components, const speedometer_state_t *state);

/**
 * @brief Update gear indicator
 * @param components Pointer to view components
 * @param gear Current gear (0=N, 1-6)
 * @param zone_color Color based on speed zone
 */
void view_update_gear(view_components_t *components, int gear, lv_color_t zone_color);

/**
 * @brief Update RPM display
 * @param components Pointer to view components
 * @param rpm Current RPM value
 */
void view_update_rpm(view_components_t *components, int rpm);

/**
 * @brief Update turn signal indicators
 * @param components Pointer to view components
 * @param turn_signals Turn signal state
 */
void view_update_turn_signals(view_components_t *components, const turn_signal_state_t *turn_signals);

/**
 * @brief Get color based on speed zone
 * @param zone Zone index (0-3)
 * @return LVGL color
 */
lv_color_t view_get_zone_color(int zone);

/**
 * @brief Get color based on RPM zone
 * @param zone Zone index (0-2)
 * @return LVGL color
 */
lv_color_t view_get_rpm_color(int zone);

#endif // VIEW_H
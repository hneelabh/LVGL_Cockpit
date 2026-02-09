/**
 * @file view.h
 * @brief View layer - UI components and rendering
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

/* Color definitions */
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
    lv_obj_t *master_container;                   // Master Container for the UI
    lv_obj_t *splash_container;                   // Logo Boot-up Sequence

    // --- Swipe Container ---
    lv_obj_t * tileview;
    lv_obj_t * tile_gauges;      // Page 1
    lv_obj_t * tile_music;       // Page 2

    // --- Driving Page Elements ---
    lv_obj_t *speed_arc_segments[SEGMENT_COUNT];  
    lv_obj_t *speed_label;                        
    lv_obj_t *gear_label;                         
    lv_obj_t *rpm_label;                          
    lv_obj_t *n_indicator;                        
    lv_obj_t *left_turn_signal;                   
    lv_obj_t *right_turn_signal;                  
    lv_obj_t *odo_label;                          
    lv_obj_t *trip_label;                         
    lv_obj_t *fuel_label;                         
    lv_obj_t *temp_label;                         

    // --- Music Components ---
    lv_obj_t * music_cont;                        
    lv_obj_t * label_title;                       
    lv_obj_t * label_artist;                      
    lv_obj_t * label_album;
    lv_obj_t * label_time_current;
    lv_obj_t * label_time_total;                  
    lv_obj_t * bar_progress;
    lv_obj_t * btn_prev;
    lv_obj_t * btn_next;
    lv_obj_t * btn_play;
    lv_obj_t * btn_play_label; // Pointer to the icon label itself

    // --- Notifications overlay ---
    lv_obj_t * notification_panel;
    lv_obj_t * notif_label;
    lv_obj_t * notif_icon;
    bool is_alert_active;                          
    
    // --- Navigation Components ---
    lv_obj_t * label_nav_icon;   
    lv_obj_t * label_nav_dist;   
    lv_obj_t * label_nav_street; 
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
 */
void view_update_speed(view_components_t *components, const speedometer_state_t *state);

/**
 * @brief Update gear indicator
 */
void view_update_gear(view_components_t *components, int gear, lv_color_t zone_color);

/**
 * @brief Update RPM display
 */
void view_update_rpm(view_components_t *components, int rpm);

/**
 * @brief Update turn signal indicators
 */
void view_update_turn_signals(view_components_t *components, const turn_signal_state_t *turn_signals);

/**
 * @brief Get color based on speed zone
 */
lv_color_t view_get_zone_color(int zone);

/**
 * @brief Get color based on RPM zone
 */
lv_color_t view_get_rpm_color(int zone);

/**
 * @brief Show/Hide the top notification bar
 */
void view_set_alert_state(view_components_t *components, bool is_active, const char *text);

#endif // VIEW_H
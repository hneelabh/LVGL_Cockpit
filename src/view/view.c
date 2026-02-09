/**
 * @file view.c
 * @brief View layer implementation
 * Implements all LVGL UI creation and update functions
 */

#include "view.h"
#include <stdio.h>

// Declaration of the image variable
LV_IMG_DECLARE(visteon_logo);

/* ========================================================================
 * Animation Helpers
 * ======================================================================== */

static void fade_anim_cb(void * obj, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)obj, v, 0);
}

static void run_fade_anim(lv_obj_t * target, int start_val, int end_val, int duration, int delay) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, target);
    lv_anim_set_values(&a, start_val, end_val);
    lv_anim_set_time(&a, duration);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_exec_cb(&a, fade_anim_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

static void trigger_boot_sequence(view_components_t *components) {
    // 1. Fade IN Splash
    run_fade_anim(components->splash_container, LV_OPA_TRANSP, LV_OPA_COVER, 500, 0);
    // 2. Fade OUT Splash
    run_fade_anim(components->splash_container, LV_OPA_COVER, LV_OPA_TRANSP, 500, 2000);
    // 3. Fade IN Main UI
    run_fade_anim(components->master_container, LV_OPA_TRANSP, LV_OPA_COVER, 1000, 2300);
}

// Animation callback for Notification Slide
static void anim_y_cb(void * var, int32_t v) {
    lv_obj_set_y((lv_obj_t*)var, v);
}

/* ========================================================================
 * Component Creation Helpers
 * ======================================================================== */

static void create_top_panel(view_components_t *components, lv_obj_t *parent)
{
    lv_obj_t *top_panel = lv_obj_create(parent); 
    lv_obj_set_size(top_panel, 800, 70);
    lv_obj_set_style_bg_color(top_panel, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(top_panel, COLOR_DARK_GREY, 0);
    lv_obj_set_style_border_width(top_panel, 1, 0);
    lv_obj_align(top_panel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(top_panel, LV_OBJ_FLAG_SCROLLABLE);

    // ODO (Left)
    components->odo_label = lv_label_create(top_panel);
    lv_label_set_text(components->odo_label, "ODO\n#FFFFFF ---#"); 
    lv_obj_set_style_text_color(components->odo_label, COLOR_DARK_GREY, 0);
    lv_label_set_recolor(components->odo_label, true);
    lv_obj_align(components->odo_label, LV_ALIGN_LEFT_MID, 20, 0);

    // TRIP (Left-Center)
    components->trip_label = lv_label_create(top_panel);
    lv_label_set_text(components->trip_label, "TRIP\n#FFFFFF --.-#");
    lv_obj_set_style_text_color(components->trip_label, COLOR_DARK_GREY, 0);
    lv_label_set_recolor(components->trip_label, true);
    lv_obj_align(components->trip_label, LV_ALIGN_LEFT_MID, 150, 0);

    // FUEL (Right-Center)
    components->fuel_label = lv_label_create(top_panel);
    lv_label_set_text(components->fuel_label, "FUEL\n#FFFFFF [#####  ]#");
    lv_obj_set_style_text_color(components->fuel_label, COLOR_DARK_GREY, 0);
    lv_label_set_recolor(components->fuel_label, true);
    lv_obj_align(components->fuel_label, LV_ALIGN_RIGHT_MID, -180, 0); 

    // --- BLUETOOTH ICON (The Glow Logic) ---
    components->bluetooth_icon = lv_label_create(top_panel);
    lv_label_set_text(components->bluetooth_icon, LV_SYMBOL_BLUETOOTH);
    
    // Size: slightly smaller than 'N' but visible
    lv_obj_set_style_text_font(components->bluetooth_icon, &lv_font_montserrat_24, 0);
    
    // Default Color: DARK GREY (OFF)
    lv_obj_set_style_text_color(components->bluetooth_icon, COLOR_DARK_GREY, 0);
    
    // Position: To the left of the 'N'
    lv_obj_align(components->bluetooth_icon, LV_ALIGN_RIGHT_MID, -80, 0); 

    // NEUTRAL INDICATOR (Far Right)
    components->n_indicator = lv_label_create(top_panel);
    lv_label_set_text(components->n_indicator, "N");
    lv_obj_set_style_text_font(components->n_indicator, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(components->n_indicator, COLOR_DARK_GREY, 0);
    lv_obj_align(components->n_indicator, LV_ALIGN_RIGHT_MID, -20, 0);
}

static void create_turn_signals(view_components_t *components, lv_obj_t *parent)
{
    components->left_turn_signal = lv_label_create(parent);
    lv_label_set_text(components->left_turn_signal, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(components->left_turn_signal, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(components->left_turn_signal, COLOR_DARK_GREY, 0);
    lv_obj_align(components->left_turn_signal, LV_ALIGN_TOP_LEFT, 50, 90);

    components->right_turn_signal = lv_label_create(parent);
    lv_label_set_text(components->right_turn_signal, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(components->right_turn_signal, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(components->right_turn_signal, COLOR_DARK_GREY, 0);
    lv_obj_align(components->right_turn_signal, LV_ALIGN_TOP_RIGHT, -50, 90);
}

static void create_segmented_arc(view_components_t *components, lv_obj_t *parent)
{
    int arc_center_x = 400;
    int arc_center_y = 310;
    int arc_radius = 190;
    int arc_width = 24;
    
    float start_angle = 135.0f;
    float total_sweep = 270.0f;
    float segment_angle = total_sweep / SEGMENT_COUNT;
    float gap_angle = 2.0f;
    
    for (int i = 0; i < SEGMENT_COUNT; i++) {
        components->speed_arc_segments[i] = lv_arc_create(parent);
        
        float seg_start = start_angle + (i * segment_angle);
        float seg_end = seg_start + segment_angle - gap_angle;
        
        lv_obj_set_size(components->speed_arc_segments[i], arc_radius * 2, arc_radius * 2);
        lv_obj_set_pos(components->speed_arc_segments[i], arc_center_x - arc_radius, arc_center_y - arc_radius);
        
        lv_arc_set_rotation(components->speed_arc_segments[i], (int)seg_start);
        lv_arc_set_bg_angles(components->speed_arc_segments[i], 0, (int)(seg_end - seg_start));
        lv_arc_set_range(components->speed_arc_segments[i], 0, 100);
        lv_arc_set_value(components->speed_arc_segments[i], 100);
        
        lv_obj_set_style_bg_opa(components->speed_arc_segments[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_arc_opa(components->speed_arc_segments[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_arc_color(components->speed_arc_segments[i], lv_color_hex(0x1E1E28), LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(components->speed_arc_segments[i], arc_width, LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(components->speed_arc_segments[i], false, LV_PART_INDICATOR);
        lv_obj_remove_style(components->speed_arc_segments[i], NULL, LV_PART_KNOB);
        lv_obj_clear_flag(components->speed_arc_segments[i], LV_OBJ_FLAG_CLICKABLE);
    }
}

static void create_speed_display(view_components_t *components, lv_obj_t *parent)
{
    lv_obj_t *speed_bg = lv_obj_create(parent);
    lv_obj_set_size(speed_bg, 200, 100);
    lv_obj_set_style_bg_color(speed_bg, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(speed_bg, COLOR_DARK_GREY, 0);
    lv_obj_set_style_border_width(speed_bg, 2, 0);
    lv_obj_align(speed_bg, LV_ALIGN_CENTER, 0, 20);
    lv_obj_clear_flag(speed_bg, LV_OBJ_FLAG_SCROLLABLE);

    components->speed_label = lv_label_create(speed_bg);
    lv_obj_set_style_text_font(components->speed_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(components->speed_label, "0");
    lv_obj_set_style_text_color(components->speed_label, COLOR_NEON_GREEN, 0);
    lv_obj_align(components->speed_label, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *kmh_label = lv_label_create(speed_bg);
    lv_label_set_text(kmh_label, "km/h");
    lv_obj_set_style_text_color(kmh_label, COLOR_DARK_GREY, 0);
    lv_obj_align(kmh_label, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void create_gear_display(view_components_t *components, lv_obj_t *parent)
{
    lv_obj_t *gear_bg = lv_obj_create(parent);
    lv_obj_set_size(gear_bg, 100, 90);
    lv_obj_set_style_bg_color(gear_bg, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(gear_bg, COLOR_DARK_GREY, 0);
    lv_obj_set_style_border_width(gear_bg, 2, 0);
    lv_obj_align(gear_bg, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_clear_flag(gear_bg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *gear_text = lv_label_create(gear_bg);
    lv_label_set_text(gear_text, "GEAR");
    lv_obj_set_style_text_color(gear_text, COLOR_DARK_GREY, 0);
    lv_obj_align(gear_text, LV_ALIGN_TOP_MID, 0, 5);

    components->gear_label = lv_label_create(gear_bg);
    lv_obj_set_style_text_font(components->gear_label, &lv_font_montserrat_36, 0);
    lv_label_set_text(components->gear_label, "N");
    lv_obj_set_style_text_color(components->gear_label, COLOR_NEON_GREEN, 0);
    lv_obj_align(components->gear_label, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void create_rpm_display(view_components_t *components, lv_obj_t *parent)
{
    lv_obj_t *rpm_info = lv_label_create(parent);
    lv_label_set_text(rpm_info, "RPM\nx1000");
    lv_obj_set_style_text_color(rpm_info, COLOR_DARK_GREY, 0);
    lv_obj_align(rpm_info, LV_ALIGN_BOTTOM_LEFT, 40, -50);

    components->rpm_label = lv_label_create(parent);
    lv_obj_set_style_text_font(components->rpm_label, &lv_font_montserrat_36, 0);
    lv_label_set_text(components->rpm_label, "1");
    lv_obj_set_style_text_color(components->rpm_label, COLOR_NEON_GREEN, 0);
    lv_obj_align(components->rpm_label, LV_ALIGN_BOTTOM_LEFT, 120, -45);
}

static void create_temp_display(view_components_t *components, lv_obj_t *parent)
{
    components->temp_label = lv_label_create(parent);
    lv_label_set_text(components->temp_label, "TEMP\n---C");
    lv_obj_set_style_text_color(components->temp_label, COLOR_DARK_GREY, 0);
    lv_obj_align(components->temp_label, LV_ALIGN_BOTTOM_RIGHT, -40, -50);
}

static void create_notification_overlay(view_components_t *components, lv_obj_t *parent) {
    components->notification_panel = lv_obj_create(parent);
    lv_obj_set_size(components->notification_panel, 400, 80);
    lv_obj_align(components->notification_panel, LV_ALIGN_TOP_MID, 0, -100); // Start OFF SCREEN
    
    // Style
    lv_obj_set_style_bg_color(components->notification_panel, lv_color_hex(0xAA0000), 0);
    lv_obj_set_style_bg_grad_color(components->notification_panel, lv_color_hex(0x550000), 0);
    lv_obj_set_style_bg_grad_dir(components->notification_panel, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(components->notification_panel, 10, 0);
    lv_obj_set_style_shadow_width(components->notification_panel, 20, 0);
    lv_obj_clear_flag(components->notification_panel, LV_OBJ_FLAG_SCROLLABLE);

    components->notif_icon = lv_label_create(components->notification_panel);
    lv_label_set_text(components->notif_icon, LV_SYMBOL_WARNING);
    lv_obj_align(components->notif_icon, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_font(components->notif_icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(components->notif_icon, lv_color_white(), 0);

    components->notif_label = lv_label_create(components->notification_panel);
    lv_label_set_text(components->notif_label, "System Alert");
    lv_obj_align(components->notif_label, LV_ALIGN_LEFT_MID, 60, 0);
    lv_obj_set_style_text_font(components->notif_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(components->notif_label, lv_color_white(), 0);
}

static void create_music_player(view_components_t *components, lv_obj_t *parent) {
    // 1. Container
    components->music_cont = lv_obj_create(parent);
    lv_obj_set_size(components->music_cont, 380, 320);
    lv_obj_align(components->music_cont, LV_ALIGN_LEFT_MID, 10, 30);
    lv_obj_set_style_bg_color(components->music_cont, lv_color_hex(0x181818), 0);
    lv_obj_set_style_radius(components->music_cont, 15, 0);
    lv_obj_set_style_border_width(components->music_cont, 0, 0);
    lv_obj_clear_flag(components->music_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Art Box
    lv_obj_t * art_box = lv_obj_create(components->music_cont);
    lv_obj_set_size(art_box, 100, 100);
    lv_obj_align(art_box, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_bg_color(art_box, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(art_box, 0, 0);
    
    lv_obj_t * icon = lv_label_create(art_box);
    lv_label_set_text(icon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
    lv_obj_center(icon);

    // 3. Text Info (With Scrolling)
    // Common Width for text (Container width - padding)
    int text_width = 340; 

    // TITLE
    components->label_title = lv_label_create(components->music_cont);
    lv_label_set_text(components->label_title, "Not Playing");
    lv_obj_set_width(components->label_title, text_width); // Must set width for scrolling
    lv_label_set_long_mode(components->label_title, LV_LABEL_LONG_SCROLL_CIRCULAR); // <--- SCROLL
    lv_obj_set_style_text_align(components->label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(components->label_title, LV_ALIGN_TOP_MID, 0, 125);
    lv_obj_set_style_text_font(components->label_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(components->label_title, lv_color_white(), 0);

    // ARTIST
    components->label_artist = lv_label_create(components->music_cont);
    lv_label_set_text(components->label_artist, "Connect Device");
    lv_obj_set_width(components->label_artist, text_width);
    lv_label_set_long_mode(components->label_artist, LV_LABEL_LONG_SCROLL_CIRCULAR); // <--- SCROLL
    lv_obj_set_style_text_align(components->label_artist, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(components->label_artist, LV_ALIGN_TOP_MID, 0, 155);
    lv_obj_set_style_text_font(components->label_artist, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(components->label_artist, lv_color_hex(0xAAAAAA), 0);

    // ALBUM (Fixed: Added Back!)
    components->label_album = lv_label_create(components->music_cont);
    lv_label_set_text(components->label_album, "");
    lv_obj_set_width(components->label_album, text_width);
    lv_label_set_long_mode(components->label_album, LV_LABEL_LONG_SCROLL_CIRCULAR); // <--- SCROLL
    lv_obj_set_style_text_align(components->label_album, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(components->label_album, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_set_style_text_font(components->label_album, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(components->label_album, lv_color_hex(0x888888), 0);

// 4. Time & Progress
    
    // CURRENT TIME (Left)
    components->label_time_current = lv_label_create(components->music_cont);
    lv_label_set_text(components->label_time_current, "0:00");
    lv_obj_set_style_text_font(components->label_time_current, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(components->label_time_current, lv_color_hex(0xCCCCCC), 0);
    // Align: Bottom Left, 20px padding (matches bar start), -80px up
    lv_obj_align(components->label_time_current, LV_ALIGN_BOTTOM_LEFT, 20, -80); 

    // TOTAL TIME (Right)
    components->label_time_total = lv_label_create(components->music_cont);
    lv_label_set_text(components->label_time_total, "0:00");
    lv_obj_set_style_text_font(components->label_time_total, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(components->label_time_total, lv_color_hex(0xCCCCCC), 0);
    // Align: Bottom Right, -20px padding (matches bar end), -80px up
    lv_obj_align(components->label_time_total, LV_ALIGN_BOTTOM_RIGHT, -20, -80);

    // PROGRESS BAR
    components->bar_progress = lv_bar_create(components->music_cont);
    lv_obj_set_size(components->bar_progress, 340, 6); // Width matches text alignment
    lv_obj_align(components->bar_progress, LV_ALIGN_BOTTOM_MID, 0, -65);
    lv_obj_set_style_bg_color(components->bar_progress, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_bg_color(components->bar_progress, COLOR_NEON_GREEN, LV_PART_INDICATOR);
    lv_bar_set_range(components->bar_progress, 0, 100);

    // 5. Buttons
    // PREV
    components->btn_prev = lv_btn_create(components->music_cont);
    lv_obj_set_size(components->btn_prev, 40, 40);
    lv_obj_align(components->btn_prev, LV_ALIGN_BOTTOM_MID, -60, -10);
    lv_obj_set_style_bg_color(components->btn_prev, lv_color_hex(0x333333), 0);
    lv_obj_t * lbl_prev = lv_label_create(components->btn_prev);
    lv_label_set_text(lbl_prev, LV_SYMBOL_PREV);
    lv_obj_center(lbl_prev);

    // NEXT
    components->btn_next = lv_btn_create(components->music_cont);
    lv_obj_set_size(components->btn_next, 40, 40);
    lv_obj_align(components->btn_next, LV_ALIGN_BOTTOM_MID, 60, -10);
    lv_obj_set_style_bg_color(components->btn_next, lv_color_hex(0x333333), 0);
    lv_obj_t * lbl_next = lv_label_create(components->btn_next);
    lv_label_set_text(lbl_next, LV_SYMBOL_NEXT);
    lv_obj_center(lbl_next);

    // PLAY/PAUSE (Center)
    components->btn_play = lv_btn_create(components->music_cont);
    lv_obj_set_size(components->btn_play, 50, 50);
    lv_obj_align(components->btn_play, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(components->btn_play, COLOR_NEON_GREEN, 0);
    lv_obj_set_style_radius(components->btn_play, 25, 0); 
    lv_obj_clear_flag(components->btn_play, LV_OBJ_FLAG_SCROLLABLE);

    components->btn_play_label = lv_label_create(components->btn_play);
    lv_label_set_text(components->btn_play_label, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(components->btn_play_label, lv_color_black(), 0);
    lv_obj_center(components->btn_play_label);
    
    lv_obj_add_flag(components->btn_play_label, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_clear_flag(components->btn_play_label, LV_OBJ_FLAG_CLICKABLE);
}

static void create_navigation_card(view_components_t *components, lv_obj_t *parent) {
    // 1. The Container
    lv_obj_t * nav_cont = lv_obj_create(parent);
    lv_obj_set_size(nav_cont, 380, 320);
    lv_obj_align(nav_cont, LV_ALIGN_RIGHT_MID, -10, 30); 
    lv_obj_set_style_bg_color(nav_cont, lv_color_hex(0x181818), 0);
    lv_obj_set_style_radius(nav_cont, 15, 0);
    lv_obj_set_style_border_width(nav_cont, 0, 0);
    lv_obj_clear_flag(nav_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Header
    lv_obj_t * header = lv_label_create(nav_cont);
    lv_label_set_text(header, "NEXT TURN");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(header, COLOR_NEON_GREEN, 0);

    // 3. Arrow Icon
    components->label_nav_icon = lv_label_create(nav_cont);
    lv_label_set_text(components->label_nav_icon, LV_SYMBOL_LEFT); 
    lv_obj_set_style_text_font(components->label_nav_icon, &lv_font_montserrat_24, 0); 
    lv_obj_set_style_transform_zoom(components->label_nav_icon, 512, 0); // 2x Zoom
    lv_obj_align(components->label_nav_icon, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_text_color(components->label_nav_icon, COLOR_NEON_GREEN, 0);

    // 4. Distance
    components->label_nav_dist = lv_label_create(nav_cont);
    lv_label_set_text(components->label_nav_dist, "--- m");
    lv_obj_set_style_text_font(components->label_nav_dist, &lv_font_montserrat_24, 0);
    lv_obj_align(components->label_nav_dist, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_color(components->label_nav_dist, lv_color_white(), 0);

    // 5. Street Name
    components->label_nav_street = lv_label_create(nav_cont);
    lv_label_set_text(components->label_nav_street, "Route Calculating...");
    lv_obj_align(components->label_nav_street, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_text_color(components->label_nav_street, lv_color_hex(0xAAAAAA), 0);
}

/* ========================================================================
 * Public Functions
 * ======================================================================== */

void view_init(view_components_t *components)
{
    // Background
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_DARK_BG, LV_PART_MAIN);

    // 1. MAIN WRAPPER
    lv_obj_t * root_wrapper = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root_wrapper, 800, 480);
    lv_obj_set_style_bg_opa(root_wrapper, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root_wrapper, 0, 0);
    lv_obj_set_style_pad_all(root_wrapper, 0, 0);
    lv_obj_clear_flag(root_wrapper, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_clear_flag(root_wrapper, LV_OBJ_FLAG_SCROLLABLE);

    components->master_container = root_wrapper;

    // 2. TILEVIEW (Swipe Logic)
    lv_obj_t * tileview = lv_tileview_create(root_wrapper);
    lv_obj_set_size(tileview, 800, 480);
    lv_obj_set_style_bg_color(tileview, lv_color_hex(0x000000), 0);
    lv_obj_remove_style(tileview, NULL, LV_PART_SCROLLBAR);

    lv_obj_t * tile_drive = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_RIGHT);
    lv_obj_t * tile_music = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_LEFT);
    
    // 3. GLOBAL ELEMENTS (Top Panel + Alerts)
    create_top_panel(components, root_wrapper);
    create_notification_overlay(components, root_wrapper); 

    // 4. POPULATE PAGE 1: DRIVING (Attached to tile_drive)
    create_segmented_arc(components, tile_drive);
    create_speed_display(components, tile_drive);
    create_gear_display(components, tile_drive);
    create_rpm_display(components, tile_drive);
    create_temp_display(components, tile_drive);
    create_turn_signals(components, tile_drive);

    // 5. POPULATE PAGE 2: MUSIC/NAV (Attached to tile_music)
    create_music_player(components, tile_music);     
    create_navigation_card(components, tile_music);  

    // 6. SPLASH SCREEN
    components->splash_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(components->splash_container, 800, 480);
    lv_obj_set_style_bg_color(components->splash_container, COLOR_DARK_BG, 0);
    lv_obj_set_style_border_width(components->splash_container, 0, 0);
    lv_obj_clear_flag(components->splash_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(components->splash_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * logo_img = lv_img_create(components->splash_container);
    lv_img_set_src(logo_img, &visteon_logo);
    lv_obj_align(logo_img, LV_ALIGN_CENTER, 0, 0);

    trigger_boot_sequence(components);
}

void view_update_speed(view_components_t *components, const speedometer_state_t *state)
{
    int active_segments = (state->speed * SEGMENT_COUNT) / 200;
    int zone = model_get_speed_zone(state->speed);
    lv_color_t zone_color = view_get_zone_color(zone);
    
    // Update Arc
    for (int i = 0; i < SEGMENT_COUNT; i++) {
        if (i < active_segments) {
            lv_obj_set_style_arc_color(components->speed_arc_segments[i], zone_color, LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_arc_color(components->speed_arc_segments[i], lv_color_hex(0x1E1E28), LV_PART_INDICATOR);
        }
    }
    
    // Update Text
    lv_label_set_text_fmt(components->speed_label, "%d", state->speed);
    lv_obj_set_style_text_color(components->speed_label, zone_color, 0);
}

void view_update_gear(view_components_t *components, int gear, lv_color_t zone_color)
{
    if (gear == 0) {
        lv_label_set_text(components->gear_label, "N");
        lv_obj_set_style_text_color(components->gear_label, COLOR_NEON_GREEN, 0);
        lv_obj_set_style_text_color(components->n_indicator, COLOR_NEON_GREEN, 0);
    } else {
        lv_label_set_text_fmt(components->gear_label, "%d", gear);
        lv_obj_set_style_text_color(components->gear_label, zone_color, 0);
        lv_obj_set_style_text_color(components->n_indicator, COLOR_DARK_GREY, 0);
    }
}

void view_update_rpm(view_components_t *components, int rpm)
{
    lv_label_set_text_fmt(components->rpm_label, "%d", rpm);
    int zone = model_get_rpm_zone(rpm);
    lv_color_t rpm_color = view_get_rpm_color(zone);
    lv_obj_set_style_text_color(components->rpm_label, rpm_color, 0);
}

void view_update_turn_signals(view_components_t *components, const turn_signal_state_t *turn_signals)
{
    if (turn_signals->left_active && turn_signals->left_blink) {
        lv_obj_set_style_text_color(components->left_turn_signal, COLOR_NEON_GREEN, 0);
    } else {
        lv_obj_set_style_text_color(components->left_turn_signal, COLOR_DARK_GREY, 0);
    }
    
    if (turn_signals->right_active && turn_signals->right_blink) {
        lv_obj_set_style_text_color(components->right_turn_signal, COLOR_NEON_GREEN, 0);
    } else {
        lv_obj_set_style_text_color(components->right_turn_signal, COLOR_DARK_GREY, 0);
    }
}

void view_set_alert_state(view_components_t *components, bool is_active, const char *text) {
    if (is_active && !components->is_alert_active) {
        // Show
        lv_label_set_text(components->notif_label, text);
        lv_anim_del(components->notification_panel, anim_y_cb);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, components->notification_panel);
        lv_anim_set_values(&a, -100, 20); 
        lv_anim_set_time(&a, 300); 
        lv_anim_set_exec_cb(&a, anim_y_cb);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);
        components->is_alert_active = true;
    }
    else if (!is_active && components->is_alert_active) {
        // Hide
        lv_anim_del(components->notification_panel, anim_y_cb);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, components->notification_panel);
        lv_anim_set_values(&a, 20, -100); 
        lv_anim_set_time(&a, 300);
        lv_anim_set_exec_cb(&a, anim_y_cb);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in); 
        lv_anim_start(&a);
        components->is_alert_active = false;
    }
}

// Color Helpers
lv_color_t view_get_zone_color(int zone) {
    switch(zone) {
        case 0: return COLOR_NEON_GREEN; 
        case 1: return COLOR_NEON_YEL;    
        case 2: return COLOR_NEON_ORG;    
        case 3: return COLOR_NEON_RED;    
        default: return COLOR_NEON_GREEN;
    }
}

lv_color_t view_get_rpm_color(int zone) {
    switch(zone) {
        case 0: return COLOR_NEON_GREEN; 
        case 1: return COLOR_NEON_ORG;   
        case 2: return COLOR_NEON_RED;    
        default: return COLOR_NEON_GREEN;
    }
}
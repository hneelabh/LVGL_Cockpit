/**
 * @file view.c
 * @brief View layer implementation
 * * Implements all LVGL UI creation and update functions
 */

#include "view.h"
#include <stdio.h>

// Declaration of the image variable
LV_IMG_DECLARE(visteon_logo);

/* ========================================================================
 * Splash Screen & Boot Animation Logic
 * ======================================================================== */

// The single callback function for all opacity animations
static void fade_anim_cb(void * obj, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)obj, v, 0);
}

// Helper to create a timed fade animation
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

// The Master Sequence
static void trigger_boot_sequence(view_components_t *components) {
    // 1. Fade IN the Splash Screen (Fast: 500ms)
    run_fade_anim(components->splash_container, LV_OPA_TRANSP, LV_OPA_COVER, 500, 0);

    // 2. Fade OUT the Splash Screen (After waiting 2 seconds)
    run_fade_anim(components->splash_container, LV_OPA_COVER, LV_OPA_TRANSP, 500, 2000);

    // 3. Fade IN the Cockpit (Starts just as the splash is fading out)
    run_fade_anim(components->master_container, LV_OPA_TRANSP, LV_OPA_COVER, 1000, 2300);
}

/* ========================================================================
 * Private Helper Functions
 * ======================================================================== */

static void create_top_panel(view_components_t *components)
{
    // CHANGED: Attached to master_container
    lv_obj_t *top_panel = lv_obj_create(components->master_container); 
    lv_obj_set_size(top_panel, 800, 70);
    lv_obj_set_style_bg_color(top_panel, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(top_panel, COLOR_DARK_GREY, 0);
    lv_obj_set_style_border_width(top_panel, 1, 0);
    lv_obj_align(top_panel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(top_panel, LV_OBJ_FLAG_SCROLLABLE);

    // ODO
    components->odo_label = lv_label_create(top_panel);
    lv_label_set_text(components->odo_label, "ODO\n#FFFFFF 12345#");
    lv_obj_set_style_text_color(components->odo_label, COLOR_DARK_GREY, 0);
    lv_label_set_recolor(components->odo_label, true);
    lv_obj_align(components->odo_label, LV_ALIGN_LEFT_MID, 20, 0);

    // TRIP
    components->trip_label = lv_label_create(top_panel);
    lv_label_set_text(components->trip_label, "TRIP\n#FFFFFF 123.4#");
    lv_obj_set_style_text_color(components->trip_label, COLOR_DARK_GREY, 0);
    lv_label_set_recolor(components->trip_label, true);
    lv_obj_align(components->trip_label, LV_ALIGN_LEFT_MID, 150, 0);

    // FUEL
    components->fuel_label = lv_label_create(top_panel);
    lv_label_set_text(components->fuel_label, "FUEL\n#FFFFFF [#####  ]#");
    lv_obj_set_style_text_color(components->fuel_label, COLOR_DARK_GREY, 0);
    lv_label_set_recolor(components->fuel_label, true);
    lv_obj_align(components->fuel_label, LV_ALIGN_RIGHT_MID, -100, 0);

    // Neutral indicator
    components->n_indicator = lv_label_create(top_panel);
    lv_label_set_text(components->n_indicator, "N");
    lv_obj_set_style_text_font(components->n_indicator, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(components->n_indicator, COLOR_DARK_GREY, 0);
    lv_obj_align(components->n_indicator, LV_ALIGN_RIGHT_MID, -20, 0);
}

static void create_turn_signals(view_components_t *components)
{
    // CHANGED: Attached to master_container
    components->left_turn_signal = lv_label_create(components->master_container);
    lv_label_set_text(components->left_turn_signal, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(components->left_turn_signal, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(components->left_turn_signal, COLOR_DARK_GREY, 0);
    lv_obj_align(components->left_turn_signal, LV_ALIGN_TOP_LEFT, 50, 90);

    // CHANGED: Attached to master_container
    components->right_turn_signal = lv_label_create(components->master_container);
    lv_label_set_text(components->right_turn_signal, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(components->right_turn_signal, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(components->right_turn_signal, COLOR_DARK_GREY, 0);
    lv_obj_align(components->right_turn_signal, LV_ALIGN_TOP_RIGHT, -50, 90);
}

static void create_segmented_arc(view_components_t *components)
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
        // CHANGED: Attached to master_container
        components->speed_arc_segments[i] = lv_arc_create(components->master_container);
        
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

static void create_speed_display(view_components_t *components)
{
    // CHANGED: Attached to master_container
    lv_obj_t *speed_bg = lv_obj_create(components->master_container);
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

static void create_gear_display(view_components_t *components)
{
    // CHANGED: Attached to master_container
    lv_obj_t *gear_bg = lv_obj_create(components->master_container);
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

static void create_rpm_display(view_components_t *components)
{
    // CHANGED: Attached to master_container
    lv_obj_t *rpm_info = lv_label_create(components->master_container);
    lv_label_set_text(rpm_info, "RPM\nx1000");
    lv_obj_set_style_text_color(rpm_info, COLOR_DARK_GREY, 0);
    lv_obj_align(rpm_info, LV_ALIGN_BOTTOM_LEFT, 40, -50);

    // CHANGED: Attached to master_container
    components->rpm_label = lv_label_create(components->master_container);
    lv_obj_set_style_text_font(components->rpm_label, &lv_font_montserrat_36, 0);
    lv_label_set_text(components->rpm_label, "1");
    lv_obj_set_style_text_color(components->rpm_label, COLOR_NEON_GREEN, 0);
    lv_obj_align(components->rpm_label, LV_ALIGN_BOTTOM_LEFT, 120, -45);
}

static void create_temp_display(view_components_t *components)
{
    // CHANGED: Attached to master_container
    components->temp_label = lv_label_create(components->master_container);
    lv_label_set_text(components->temp_label, "TEMP\n---C");
    lv_obj_set_style_text_color(components->temp_label, COLOR_DARK_GREY, 0);
    lv_obj_align(components->temp_label, LV_ALIGN_BOTTOM_RIGHT, -40, -50);
}

// static void create_music_player(view_components_t *components) {
//     // 1. The Container (Card Style)
//     components->music_cont = lv_obj_create(components->master_container);
//     lv_obj_set_size(components->music_cont, 300, 90);
//     lv_obj_align(components->music_cont, LV_ALIGN_BOTTOM_MID, 0, -20);
    
//     // Semi-transparent dark background
//     lv_obj_set_style_bg_color(components->music_cont, lv_color_hex(0x101010), 0);
//     lv_obj_set_style_bg_opa(components->music_cont, LV_OPA_90, 0);
//     lv_obj_set_style_border_color(components->music_cont, lv_color_hex(0x404040), 0);
//     lv_obj_set_style_border_width(components->music_cont, 2, 0);
//     lv_obj_set_style_radius(components->music_cont, 15, 0);
//     lv_obj_clear_flag(components->music_cont, LV_OBJ_FLAG_SCROLLABLE);

//     // 2. The Icon (Left Side)
//     lv_obj_t * icon_box = lv_obj_create(components->music_cont);
//     lv_obj_set_size(icon_box, 60, 60);
//     lv_obj_align(icon_box, LV_ALIGN_LEFT_MID, -10, 0);
//     lv_obj_set_style_bg_color(icon_box, lv_color_hex(0x333333), 0);
//     lv_obj_set_style_radius(icon_box, 10, 0);
//     lv_obj_clear_flag(icon_box, LV_OBJ_FLAG_SCROLLABLE);

//     lv_obj_t * icon_label = lv_label_create(icon_box);
//     lv_label_set_text(icon_label, LV_SYMBOL_AUDIO);
//     lv_obj_center(icon_label);
//     lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_24, 0);
//     lv_obj_set_style_text_color(icon_label, COLOR_NEON_GREEN, 0);

//     // 3. Track Title (Big & White)
//     components->label_title = lv_label_create(components->music_cont);
//     lv_label_set_text(components->label_title, "Not Playing");
//     lv_obj_set_width(components->label_title, 200);
//     lv_obj_align(components->label_title, LV_ALIGN_TOP_LEFT, 60, 5);
//     lv_obj_set_style_text_color(components->label_title, lv_color_white(), 0);
//     lv_obj_set_style_text_font(components->label_title, &lv_font_montserrat_20, 0);
//     lv_label_set_long_mode(components->label_title, LV_LABEL_LONG_SCROLL_CIRCULAR);

//     // 4. Artist (Smaller & Grey)
//     components->label_artist = lv_label_create(components->music_cont);
//     lv_label_set_text(components->label_artist, "Connect Device");
//     lv_obj_set_width(components->label_artist, 200);
//     lv_obj_align(components->label_artist, LV_ALIGN_TOP_LEFT, 60, 30);
//     lv_obj_set_style_text_color(components->label_artist, lv_color_hex(0xAAAAAA), 0);
//     lv_obj_set_style_text_font(components->label_artist, &lv_font_montserrat_16, 0);
//     lv_label_set_long_mode(components->label_artist, LV_LABEL_LONG_DOT);
    
//     // 5. Album (Tiny & Italic-ish)
//     components->label_album = lv_label_create(components->music_cont);
//     lv_label_set_text(components->label_album, "");
//     lv_obj_set_width(components->label_album, 200);
//     lv_obj_align(components->label_album, LV_ALIGN_TOP_LEFT, 60, 52);
//     lv_obj_set_style_text_color(components->label_album, lv_color_hex(0x777777), 0);
//     lv_obj_set_style_text_font(components->label_album, &lv_font_montserrat_12, 0);
//     lv_label_set_long_mode(components->label_album, LV_LABEL_LONG_DOT);
// }

static void create_music_player(view_components_t *components) {
    // 1. Get the Parent
    // If we are in the 'Pointer Swap' init, master_container is pointing to the Music Tile.
    lv_obj_t * parent = components->master_container; 

    // 2. Big Album Art Placeholder (Gray Box)
    lv_obj_t * art_box = lv_obj_create(parent);
    lv_obj_set_size(art_box, 150, 150);
    lv_obj_center(art_box);
    lv_obj_align(art_box, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_bg_color(art_box, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(art_box, 0, 0);
    lv_obj_clear_flag(art_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * icon = lv_label_create(art_box);
    lv_label_set_text(icon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
    lv_obj_center(icon);

    // 3. Title (CRITICAL: Must assign to components->label_title)
    components->label_title = lv_label_create(parent);
    lv_label_set_text(components->label_title, "Not Playing");
    lv_obj_set_width(components->label_title, 300);
    lv_obj_align(components->label_title, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_text_align(components->label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(components->label_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(components->label_title, lv_color_white(), 0);

    // 4. Artist (CRITICAL: Must assign to components->label_artist)
    components->label_artist = lv_label_create(parent);
    lv_label_set_text(components->label_artist, "Connect Device");
    lv_obj_align(components->label_artist, LV_ALIGN_CENTER, 0, 90);
    lv_obj_set_style_text_font(components->label_artist, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(components->label_artist, lv_color_hex(0xAAAAAA), 0);
    
    // 5. Album (CRITICAL: Must assign to components->label_album)
    components->label_album = lv_label_create(parent);
    lv_label_set_text(components->label_album, "");
    lv_obj_align(components->label_album, LV_ALIGN_CENTER, 0, 115);
    lv_obj_set_style_text_font(components->label_album, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(components->label_album, lv_color_hex(0x777777), 0);

    // 6. Header Label
    lv_obj_t * header = lv_label_create(parent);
    lv_label_set_text(header, "MEDIA PLAYER");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(header, COLOR_NEON_GREEN, 0);
}

/* ========================================================================
 * Public Functions
 * ======================================================================== */

void view_init(view_components_t *components)
{
    // Set absolute background
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_DARK_BG, LV_PART_MAIN);

    // --- 1. CREATE MAIN WRAPPER (The object that fades in) ---
    // This effectively replaces your old 'master_container' as the root
    lv_obj_t * root_wrapper = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root_wrapper, 800, 480);
    lv_obj_set_style_bg_opa(root_wrapper, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root_wrapper, 0, 0);
    lv_obj_set_style_pad_all(root_wrapper, 0, 0);
    lv_obj_set_style_opa(root_wrapper, LV_OPA_TRANSP, 0); // Start hidden (Boot sequence targets this)

    // Save the real root so we can restore it later
    components->master_container = root_wrapper;

    // --- FIX 1: Make wrapper transparent to clicks ---
    lv_obj_clear_flag(root_wrapper, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_clear_flag(root_wrapper, LV_OBJ_FLAG_SCROLLABLE);

    // --- 2. CREATE GLOBAL ELEMENTS (Stay simulated on all screens) ---
    // We call this NOW so it attaches to the root_wrapper.
    // Use this for Top Bar, Turn Signals, Warnings so they never swipe away.
    create_top_panel(components);
    create_turn_signals(components);

    // --- 3. CREATE TILEVIEW (The Swiping Engine) ---
    // We create this INSIDE the wrapper, but BELOW the top panel
    lv_obj_t * tileview = lv_tileview_create(root_wrapper);
    lv_obj_set_size(tileview, 800, 480);
    lv_obj_set_style_bg_color(tileview, lv_color_hex(0x000000), 0); // Black background
    lv_obj_remove_style(tileview, NULL, LV_PART_SCROLLBAR); // Hide scrollbars
    lv_obj_move_background(tileview); // Push to back so Top Panel sits on top

    // Create the Two Pages (Tiles)
    lv_obj_t * tile_drive = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_RIGHT); // Page 1
    lv_obj_t * tile_music = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_LEFT);  // Page 2

    // --- 4. POPULATE PAGE 1: DRIVING (The "Pointer Swap" Trick) ---
    // We temporarily trick the creation functions into thinking 'tile_drive' is the master container.
    components->master_container = tile_drive;

    create_segmented_arc(components);
    create_speed_display(components);
    create_gear_display(components);
    create_rpm_display(components);
    create_temp_display(components);
    // Note: Top panel and Turn signals were already created on the global wrapper!

    // --- 5. POPULATE PAGE 2: MUSIC ---
    // Now we trick the music player into building on 'tile_music'
    components->master_container = tile_music;
    
    create_music_player(components);

    // --- 6. RESTORE REAL MASTER POINTER ---
    // Important! The boot sequence expects 'master_container' to be the top-level object
    // to fade it in. We point it back to the wrapper we created at the start.
    components->master_container = root_wrapper;

    // --- 7. CREATE OEM SPLASH SCREEN (Overlay) ---
    components->splash_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(components->splash_container, 800, 480);
    lv_obj_set_style_bg_color(components->splash_container, COLOR_DARK_BG, 0);
    lv_obj_set_style_border_width(components->splash_container, 0, 0);
    lv_obj_set_style_opa(components->splash_container, LV_OPA_TRANSP, 0); // Start hidden (Boot seq handles this)

    lv_obj_t * logo_img = lv_img_create(components->splash_container);
    lv_img_set_src(logo_img, &visteon_logo);
    lv_obj_align(logo_img, LV_ALIGN_CENTER, 0, 0);

    // --- FIX 2: Make Splash Screen transparent to clicks ---
    // If you don't do this, the splash screen blocks everything even when invisible!
    lv_obj_clear_flag(components->splash_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(components->splash_container, LV_OBJ_FLAG_SCROLLABLE);

    // --- 8. START THE SEQUENCE ---
    trigger_boot_sequence(components);
}

/* The rest of the file (view_update_speed, view_update_gear, etc.) remains EXACTLY the same */
void view_update_speed(view_components_t *components, const speedometer_state_t *state)
{
    int active_segments = (state->speed * SEGMENT_COUNT) / 200;
    int zone = model_get_speed_zone(state->speed);
    lv_color_t zone_color = view_get_zone_color(zone);
    
    for (int i = 0; i < SEGMENT_COUNT; i++) {
        if (i < active_segments) {
            lv_obj_set_style_arc_color(components->speed_arc_segments[i], zone_color, LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_arc_color(components->speed_arc_segments[i], lv_color_hex(0x1E1E28), LV_PART_INDICATOR);
        }
    }
    
    lv_label_set_text_fmt(components->speed_label, "%d", state->speed);
    lv_obj_set_style_text_color(components->speed_label, zone_color, 0);

    // Only update IF the label was created successfully
    if (components->label_title != NULL) {
        lv_label_set_text(components->label_title, state->track_title);
    }
    if (components->label_artist != NULL) {
        lv_label_set_text(components->label_artist, state->track_artist);
    }
    if (components->label_album != NULL) {
        lv_label_set_text(components->label_album, state->track_album);
    }
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

lv_color_t view_get_zone_color(int zone)
{
    switch(zone) {
        case 0: return COLOR_NEON_GREEN;  // 0-60
        case 1: return COLOR_NEON_YEL;    // 60-120
        case 2: return COLOR_NEON_ORG;    // 120-160
        case 3: return COLOR_NEON_RED;    // 160-200
        default: return COLOR_NEON_GREEN;
    }
}

lv_color_t view_get_rpm_color(int zone)
{
    switch(zone) {
        case 0: return COLOR_NEON_GREEN;  // <7k
        case 1: return COLOR_NEON_ORG;    // 7-10k
        case 2: return COLOR_NEON_RED;    // >10k
        default: return COLOR_NEON_GREEN;
    }
}
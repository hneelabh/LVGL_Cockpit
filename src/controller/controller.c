/**
 * @file controller.c
 * @brief Controller layer - Handles Input, Logic, and View Updates
 */

#include "controller.h"
#include <string.h>
#include <stdio.h>

static controller_context_t *g_ctx = NULL;

/* ========================================================================
 * Button Handler
 * ======================================================================== */
void music_btn_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    controller_context_t * ctx = (controller_context_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        // Just send the command. Do not update UI here.
        // The UI updates when the "Real Status" comes back from the Model.
        
        if (btn == ctx->view.btn_next) {
            model_send_music_cmd("NEXT");  
        }
        else if (btn == ctx->view.btn_prev) {
             model_send_music_cmd("PREV");
        }
        else if (btn == ctx->view.btn_play) {
            model_send_music_cmd("PLAYPAUSE");
        }
    }
}

/* ========================================================================
 * Timer Callbacks
 * ======================================================================== */

static void turn_signal_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (g_ctx == NULL) return;
    
    // Sync Blinkers
    g_ctx->turn_signals.left_active = g_ctx->speedometer.left_signal;
    g_ctx->turn_signals.right_active = g_ctx->speedometer.right_signal;
    g_ctx->turn_signals.blink_counter++;
    
    // Blink Speed: Every ~30 frames
    if (g_ctx->turn_signals.blink_counter >= 30) {
        g_ctx->turn_signals.blink_counter = 0;
        
        if (g_ctx->turn_signals.left_active) 
            g_ctx->turn_signals.left_blink = !g_ctx->turn_signals.left_blink;
        else 
            g_ctx->turn_signals.left_blink = false; 
        
        if (g_ctx->turn_signals.right_active) 
            g_ctx->turn_signals.right_blink = !g_ctx->turn_signals.right_blink;
        else 
            g_ctx->turn_signals.right_blink = false; 
        
        view_update_turn_signals(&g_ctx->view, &g_ctx->turn_signals);
    }
}

static void engine_sim_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (g_ctx == NULL) return;
    
    // 1. Update Physics (Speed, RPM, Odo)
    model_update_speed(&g_ctx->speedometer, 0);
    
    // 2. Check Safety Notification
    bool is_unsafe = (g_ctx->speedometer.speed > 160);
    if (g_ctx->view.notif_label != NULL) {
        view_set_alert_state(&g_ctx->view, is_unsafe, "OVERSPEED WARNING");
    }
    
    // 3. Refresh Screen
    controller_update_display(g_ctx);
}

/* ========================================================================
 * Public Functions
 * ======================================================================== */

void controller_init(controller_context_t *ctx)
{
    g_ctx = ctx;
    
    model_init(&ctx->speedometer);
    memset(&ctx->turn_signals, 0, sizeof(turn_signal_state_t));
    
    view_init(&ctx->view);
    
    // Attach Listeners
    if (ctx->view.btn_next) lv_obj_add_event_cb(ctx->view.btn_next, music_btn_handler, LV_EVENT_CLICKED, ctx);
    if (ctx->view.btn_prev) lv_obj_add_event_cb(ctx->view.btn_prev, music_btn_handler, LV_EVENT_CLICKED, ctx);
    if (ctx->view.btn_play) lv_obj_add_event_cb(ctx->view.btn_play, music_btn_handler, LV_EVENT_CLICKED, ctx);

    controller_update_display(ctx);
}

void controller_start_demo(controller_context_t *ctx)
{
    (void)ctx;
    lv_timer_create(engine_sim_timer_cb, 16, NULL);
    lv_timer_create(turn_signal_timer_cb, 16, NULL); 
}

void controller_update_display(controller_context_t *ctx)
{
    // 1. Driving Data
    view_update_speed(&ctx->view, &ctx->speedometer);
    
    int zone = model_get_speed_zone(ctx->speedometer.speed);
    lv_color_t zone_color = view_get_zone_color(zone);
    
    view_update_gear(&ctx->view, ctx->speedometer.gear, zone_color);
    view_update_rpm(&ctx->view, ctx->speedometer.rpm);

    // 2. Odometer & Trip
    if (ctx->view.odo_label != NULL) {
        char buf[32];
        sprintf(buf, "ODO\n#FFFFFF %d#", ctx->speedometer.odometer);
        lv_label_set_text(ctx->view.odo_label, buf);
    }
    if (ctx->view.trip_label != NULL) {
        char buf[32];
        sprintf(buf, "TRIP\n#FFFFFF %.1f#", ctx->speedometer.trip);
        lv_label_set_text(ctx->view.trip_label, buf);
    }
    
    // 3. Music Data
    if (ctx->view.label_title != NULL) {
        lv_label_set_text(ctx->view.label_title, ctx->speedometer.track_title);
        
        if (ctx->view.label_artist) {
            lv_label_set_text(ctx->view.label_artist, ctx->speedometer.track_artist);
        }
        
        // Fix: Update the Album Label
        if (ctx->view.label_album) {
            lv_label_set_text(ctx->view.label_album, ctx->speedometer.track_album);
        }

        // Update Time Label on Status Bar
        int cur = ctx->speedometer.position_sec;
        int tot = ctx->speedometer.duration_sec;
        
        // Safety check
        if (cur < 0) cur = 0;
        if (tot < 0) tot = 0;

        // Update Left Label (Current)
        if (ctx->view.label_time_current) {
            lv_label_set_text_fmt(ctx->view.label_time_current, "%d:%02d", cur / 60, cur % 60);
        }

        // Update Right Label (Total)
        if (ctx->view.label_time_total) {
            lv_label_set_text_fmt(ctx->view.label_time_total, "%d:%02d", tot / 60, tot % 60);
        }
        
        // Progress Bar
        if (ctx->view.bar_progress) {
            int pct = 0;
            if (ctx->speedometer.duration_sec > 0) {
                pct = (ctx->speedometer.position_sec * 100) / ctx->speedometer.duration_sec;
            }
            if (pct > 100) pct = 100;
            if (pct < 0) pct = 0;
            lv_bar_set_value(ctx->view.bar_progress, pct, LV_ANIM_ON);
        }
    }

    // 4. Music Play/Pause Icon (Direct Pointer Check)
    if (ctx->view.btn_play_label != NULL) {
        if (ctx->speedometer.is_playing) {
            // Real state is PLAYING -> Show Pause Button
            if (strcmp(lv_label_get_text(ctx->view.btn_play_label), LV_SYMBOL_PAUSE) != 0)
                lv_label_set_text(ctx->view.btn_play_label, LV_SYMBOL_PAUSE);
        } else {
            // Real state is PAUSED -> Show Play Button
            if (strcmp(lv_label_get_text(ctx->view.btn_play_label), LV_SYMBOL_PLAY) != 0)
                lv_label_set_text(ctx->view.btn_play_label, LV_SYMBOL_PLAY);
        }
    }

    // 5. Navigation Data
    if (ctx->view.label_nav_dist != NULL) {
        char dist_buf[16];
        sprintf(dist_buf, "%d m", ctx->speedometer.nav_distance);
        lv_label_set_text(ctx->view.label_nav_dist, dist_buf);
        if (ctx->view.label_nav_street) lv_label_set_text(ctx->view.label_nav_street, ctx->speedometer.nav_street);
    }
}

void controller_set_turn_signals(controller_context_t *ctx, bool left, bool right)
{
    ctx->turn_signals.left_active = left;
    ctx->turn_signals.right_active = right;
}
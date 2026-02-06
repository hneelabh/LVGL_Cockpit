/**
 * @file controller.c
 * @brief Controller layer (Stability Checks Enabled)
 */

#include "controller.h"
#include <string.h>
#include <stdio.h>

static controller_context_t *g_ctx = NULL;

/* ========================================================================
 * Timer Callbacks
 * ======================================================================== */

/**
 * @brief Turn signal blink timer callback (Every 16ms)
 */
static void turn_signal_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (g_ctx == NULL) return;
    
    // 1. Sync Model Requests to Controller State
    g_ctx->turn_signals.left_active = g_ctx->speedometer.left_signal;
    g_ctx->turn_signals.right_active = g_ctx->speedometer.right_signal;
    
    // 2. Handle Blink Animation
    g_ctx->turn_signals.blink_counter++;
    
    // Toggle every ~500ms (30 frames * 16ms = 480ms)
    if (g_ctx->turn_signals.blink_counter >= 30) {
        g_ctx->turn_signals.blink_counter = 0;
        
        // Left
        if (g_ctx->turn_signals.left_active) {
            g_ctx->turn_signals.left_blink = !g_ctx->turn_signals.left_blink;
        } else {
            g_ctx->turn_signals.left_blink = false; 
        }
        
        // Right
        if (g_ctx->turn_signals.right_active) {
            g_ctx->turn_signals.right_blink = !g_ctx->turn_signals.right_blink;
        } else {
            g_ctx->turn_signals.right_blink = false; 
        }
        
        // Update View
        view_update_turn_signals(&g_ctx->view, &g_ctx->turn_signals);
    }
}

/**
 * @brief Engine physics timer (Runs at 60FPS / 16ms)
 */
static void engine_sim_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (g_ctx == NULL) return;
    
    // 1. Update Physics (Model handles speed/pauses internally)
    model_update_speed(&g_ctx->speedometer, 0);
    
    // 2. Check Speed Threshold for Notification (Overspeed Logic)
    bool is_unsafe = (g_ctx->speedometer.speed > 160);
    
    // Only call this if the notification label actually exists
    if (g_ctx->view.notif_label != NULL) {
        view_set_alert_state(&g_ctx->view, is_unsafe, "OVERSPEED WARNING");
    }
    
    // 3. Sync Display
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
    controller_update_display(ctx);
}

void controller_start_demo(controller_context_t *ctx)
{
    (void)ctx;
    
    // Both timers run at 60 FPS (16ms) for smooth animation
    lv_timer_create(engine_sim_timer_cb, 16, NULL);
    lv_timer_create(turn_signal_timer_cb, 16, NULL); 
}

void controller_update_display(controller_context_t *ctx)
{
    // --- 1. Driving Data ---
    view_update_speed(&ctx->view, &ctx->speedometer);
    
    int zone = model_get_speed_zone(ctx->speedometer.speed);
    lv_color_t zone_color = view_get_zone_color(zone);
    
    view_update_gear(&ctx->view, ctx->speedometer.gear, zone_color);
    view_update_rpm(&ctx->view, ctx->speedometer.rpm);
    
    // --- 2. Music Data (Safety Checked) ---
    // If initialization failed or page isn't created, skip this to avoid crash
    if (ctx->view.label_title != NULL) {
        lv_label_set_text(ctx->view.label_title, ctx->speedometer.track_title);
        
        if (ctx->view.label_artist) {
            lv_label_set_text(ctx->view.label_artist, ctx->speedometer.track_artist);
        }
        if (ctx->view.label_album) {
            lv_label_set_text(ctx->view.label_album, ctx->speedometer.track_album);
        }
    }

    // --- 3. Navigation Data (Safety Checked) ---
    if (ctx->view.label_nav_dist != NULL) {
        char dist_buf[16];
        sprintf(dist_buf, "%d m", ctx->speedometer.nav_distance);
        
        lv_label_set_text(ctx->view.label_nav_dist, dist_buf);
        
        if (ctx->view.label_nav_street) {
            lv_label_set_text(ctx->view.label_nav_street, ctx->speedometer.nav_street);
        }
    }
}

void controller_set_turn_signals(controller_context_t *ctx, bool left, bool right)
{
    ctx->turn_signals.left_active = left;
    ctx->turn_signals.right_active = right;
}
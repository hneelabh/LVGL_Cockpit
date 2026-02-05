/**
 * @file controller.c
 * @brief Controller layer implementation - Syncs Model to View
 */

#include "controller.h"
#include <string.h>

/* Global context for timer callbacks */
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
    
    // --- 1. SYNC MODEL TO CONTROLLER ---
    // The model decides IF we are turning (based on accel/decel)
    g_ctx->turn_signals.left_active = g_ctx->speedometer.left_signal;
    g_ctx->turn_signals.right_active = g_ctx->speedometer.right_signal;
    
    // --- 2. HANDLE BLINKING ANIMATION ---
    g_ctx->turn_signals.blink_counter++;
    
    // Toggle blink state every ~500ms (30 frames * 16ms = 480ms)
    if (g_ctx->turn_signals.blink_counter >= 30) {
        g_ctx->turn_signals.blink_counter = 0;
        
        // Handle Left
        if (g_ctx->turn_signals.left_active) {
            g_ctx->turn_signals.left_blink = !g_ctx->turn_signals.left_blink;
        } else {
            g_ctx->turn_signals.left_blink = false; // Force OFF if not active
        }
        
        // Handle Right
        if (g_ctx->turn_signals.right_active) {
            g_ctx->turn_signals.right_blink = !g_ctx->turn_signals.right_blink;
        } else {
            g_ctx->turn_signals.right_blink = false; // Force OFF if not active
        }
        
        // Send to View
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
    
    // 1. Update Physics (Model handles speed/pauses internaly)
    model_update_speed(&g_ctx->speedometer, 0);
    
    // 2. Sync Display
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
    
    // Run both timers at 60 FPS (16ms) for smooth animation
    lv_timer_create(engine_sim_timer_cb, 16, NULL);
    lv_timer_create(turn_signal_timer_cb, 16, NULL); 
}

void controller_update_display(controller_context_t *ctx)
{
    // Update speed and arc
    view_update_speed(&ctx->view, &ctx->speedometer);
    
    // Update gear logic
    int zone = model_get_speed_zone(ctx->speedometer.speed);
    lv_color_t zone_color = view_get_zone_color(zone);
    
    view_update_gear(&ctx->view, ctx->speedometer.gear, zone_color);
    view_update_rpm(&ctx->view, ctx->speedometer.rpm);
    
    // Update Music Labels
    if (ctx->view.label_title != NULL) {
        lv_label_set_text(ctx->view.label_title, ctx->speedometer.track_title);
        lv_label_set_text(ctx->view.label_artist, ctx->speedometer.track_artist);
        lv_label_set_text(ctx->view.label_album, ctx->speedometer.track_album);
    }
}

void controller_set_turn_signals(controller_context_t *ctx, bool left, bool right)
{
    ctx->turn_signals.left_active = left;
    ctx->turn_signals.right_active = right;
}
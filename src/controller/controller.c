/**
 * @file controller.c
 * @brief Controller layer implementation
 * 
 * Implements application logic, timers, and coordinates model-view updates
 */

#include "controller.h"
#include <string.h>

/* Global context for timer callbacks */
static controller_context_t *g_ctx = NULL;

/* ========================================================================
 * Timer Callbacks
 * ======================================================================== */

/**
 * @brief Turn signal blink timer callback (500ms cycle)
 */
static void turn_signal_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    
    if (g_ctx == NULL) return;
    
    g_ctx->turn_signals.blink_counter++;
    
    // Toggle every 12 ticks (12 * 40ms = 480ms ≈ 500ms)
    if (g_ctx->turn_signals.blink_counter >= 12) {
        g_ctx->turn_signals.blink_counter = 0;
        
        // Toggle blink states
        if (g_ctx->turn_signals.left_active) {
            g_ctx->turn_signals.left_blink = !g_ctx->turn_signals.left_blink;
        } else {
            g_ctx->turn_signals.left_blink = false;
        }
        
        if (g_ctx->turn_signals.right_active) {
            g_ctx->turn_signals.right_blink = !g_ctx->turn_signals.right_blink;
        } else {
            g_ctx->turn_signals.right_blink = false;
        }
        
        // Update view
        view_update_turn_signals(&g_ctx->view, &g_ctx->turn_signals);
    }
}

/**
 * @brief Engine simulation timer callback (40ms)
 */
static void engine_sim_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    
    if (g_ctx == NULL) return;
    
    static int speed = 0;
    
    // Handle pause timer
    if (g_ctx->demo.pause_timer > 0) {
        g_ctx->demo.pause_timer--;
        return;
    }
    
    // State machine
    switch(g_ctx->demo.state) {
        case 0: // Normal acceleration
            speed += 3;
            // Pause at gear changes
            if (speed == 24 || speed == 48 || speed == 78 || 
                speed == 117 || speed == 156) {
                g_ctx->demo.pause_timer = 5;  // 200ms pause
            }
            if (speed >= 180) {
                speed = 180;
                g_ctx->demo.state = 1;
                g_ctx->demo.pause_timer = 50;  // 2 second pause
            }
            break;
            
        case 1: // Deceleration
            speed -= 5;
            if (speed <= 0) {
                speed = 0;
                g_ctx->demo.state = 2;
                g_ctx->demo.pause_timer = 25;  // 1 second pause
            }
            break;
            
        case 2: // Sport mode acceleration
            speed += 5;
            if (speed >= 200) {
                speed = 200;
                g_ctx->demo.state = 3;
                g_ctx->demo.pause_timer = 50;  // 2 second pause
            }
            break;
            
        case 3: // Full stop
            speed -= 4;
            if (speed <= 0) {
                speed = 0;
                g_ctx->demo.state = 0;
                g_ctx->demo.pause_timer = 75;  // 3 second pause
            }
            break;
    }
    
    // Update turn signals based on state
    if (g_ctx->demo.state == 0 || g_ctx->demo.state == 2) {
        // Accelerating - left signal
        g_ctx->turn_signals.left_active = true;
        g_ctx->turn_signals.right_active = false;
    } else {
        // Braking - right signal
        g_ctx->turn_signals.left_active = false;
        g_ctx->turn_signals.right_active = true;
    }
    
    // Update model
    model_update_speed(&g_ctx->speedometer, speed);
    
    // Update display
    controller_update_display(g_ctx);
}

/* ========================================================================
 * Public Functions
 * ======================================================================== */

void controller_init(controller_context_t *ctx)
{
    // Store global context for timer callbacks
    g_ctx = ctx;
    
    // Initialize model
    model_init(&ctx->speedometer);
    
    // Initialize turn signals
    memset(&ctx->turn_signals, 0, sizeof(turn_signal_state_t));
    
    // Initialize demo state
    ctx->demo.state = 0;
    ctx->demo.pause_timer = 0;
    
    // Create UI
    view_init(&ctx->view);
    
    // Initial display update
    controller_update_display(ctx);
}

void controller_start_demo(controller_context_t *ctx)
{
    (void)ctx;
    
    // Start simulation timers
    lv_timer_create(engine_sim_timer_cb, 40, NULL);      // 40ms = 25 FPS
    lv_timer_create(turn_signal_timer_cb, 40, NULL);     // 40ms check
}

void controller_update_display(controller_context_t *ctx)
{
    // Update speed and arc
    view_update_speed(&ctx->view, &ctx->speedometer);
    
    // Get zone color for gear display
    int zone = model_get_speed_zone(ctx->speedometer.speed);
    lv_color_t zone_color = view_get_zone_color(zone);
    
    // Update gear
    view_update_gear(&ctx->view, ctx->speedometer.gear, zone_color);
    
    // Update RPM
    view_update_rpm(&ctx->view, ctx->speedometer.rpm);
    
    // Turn signals are updated in their own timer
}

void controller_set_turn_signals(controller_context_t *ctx, bool left, bool right)
{
    ctx->turn_signals.left_active = left;
    ctx->turn_signals.right_active = right;
}
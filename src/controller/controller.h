/**
 * @file controller.h
 * @brief Controller layer - Application logic and coordination
 * 
 * Coordinates between model and view, handles timers and events
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "model.h"
#include "view.h"
#include "lvgl/lvgl.h"

/* ========================================================================
 * Data Structures
 * ======================================================================== */

/**
 * @brief Main controller context
 */
typedef struct {
    speedometer_state_t speedometer;     // Model state
    turn_signal_state_t turn_signals;    // Turn signal state
    demo_state_t demo;                   // Demo simulation state
    view_components_t view;              // View components
} controller_context_t;

/* ========================================================================
 * Function Prototypes
 * ======================================================================== */

/**
 * @brief Initialize the controller and create UI
 * @param ctx Pointer to controller context
 */
void controller_init(controller_context_t *ctx);

/**
 * @brief Start demo simulation timers
 * @param ctx Pointer to controller context
 */
void controller_start_demo(controller_context_t *ctx);

/**
 * @brief Update display with current state
 * @param ctx Pointer to controller context
 */
void controller_update_display(controller_context_t *ctx);

/**
 * @brief Set turn signal state
 * @param ctx Pointer to controller context
 * @param left Left signal on/off
 * @param right Right signal on/off
 */
void controller_set_turn_signals(controller_context_t *ctx, bool left, bool right);

#endif // CONTROLLER_H
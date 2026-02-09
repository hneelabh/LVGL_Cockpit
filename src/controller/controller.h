/**
 * @file controller.h
 * @brief Controller layer - Interface
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../view/view.h"
#include "../model/model.h" 
#include "lvgl/lvgl.h"

/* ========================================================================
 * Data Structures
 * ======================================================================== */

/**
 * @brief Main Controller Context
 * Stores the entire state of the application (Model + View + Logic)
 */
typedef struct {
    speedometer_state_t speedometer;     // The Data Model
    view_components_t view;              // The GUI Elements
    turn_signal_state_t turn_signals;    // Blinker logic
} controller_context_t;

/* ========================================================================
 * Function Prototypes
 * ======================================================================== */

/**
 * @brief Initialize the controller
 * @param ctx Pointer to the context to initialize
 */
void controller_init(controller_context_t *ctx);

/**
 * @brief Start the demo simulation
 * @param ctx Pointer to the context
 */
void controller_start_demo(controller_context_t *ctx);

/**
 * @brief Update the display based on current model state
 * @param ctx Pointer to the context
 */
void controller_update_display(controller_context_t *ctx);

/**
 * @brief Set turn signal state (called by Model or Input)
 * @param ctx Pointer to context
 * @param left Left signal active
 * @param right Right signal active
 */
void controller_set_turn_signals(controller_context_t *ctx, bool left, bool right);

/**
 * @brief Handle button clicks from the Music Player
 */
void music_btn_handler(lv_event_t * e);

#endif // CONTROLLER_H
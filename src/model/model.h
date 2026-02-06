/**
 * @file model.h
 * @brief Model layer - Data structures and business logic
 */

#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * Data Structures
 * ======================================================================== */

/**
 * @brief Speedometer state structure
 */
typedef struct {
    int speed;              // Current speed (0-200 km/h)
    int gear;               // Current gear (0=N, 1-6)
    int rpm;                // Current RPM (1-13, representing x1000)
    int odometer;           // Odometer reading
    float trip;             // Trip meter reading
    int fuel_level;         // Fuel level (0-8 bars)
    int temperature;        // Engine temperature (Celsius)

    // --- SIGNAL REQUEST FLAGS ---
    bool left_signal;       // Model requests Left Blinker
    bool right_signal;      // Model requests Right Blinker

    // --- MUSIC DATA ---
    char track_title[64];
    char track_artist[64];
    char track_album[64];

    // --- NAVIGATION DATA ---
    char nav_street[64];    // "Main Street"
    int nav_distance;       // Meters to turn (e.g. 200)
    char nav_icon[8];       // Arrow Symbol (LV_SYMBOL_LEFT)
} speedometer_state_t;

/**
 * @brief Turn signal state (Used by Controller/View)
 */
typedef struct {
    bool left_active;       // Left turn signal on/off
    bool right_active;      // Right turn signal on/off
    bool left_blink;        // Current blink state (for animation)
    bool right_blink;       // Current blink state (for animation)
    int blink_counter;      // Counter for blink timing
} turn_signal_state_t;

/**
 * @brief Demo simulation state
 */
typedef struct {
    int state;              // 0=Accel, 1=Brake, 2=SportAccel, 3=FullStop
    int pause_timer;        // Pause counter for state transitions
} demo_state_t;

/* ========================================================================
 * Function Prototypes
 * ======================================================================== */

void model_init(speedometer_state_t *state);
int model_calculate_gear(int speed);
int model_calculate_rpm(int speed, int gear);
void model_update_speed(speedometer_state_t *state, int new_speed);
int model_get_speed_zone(int speed);
int model_get_rpm_zone(int rpm);

#endif // MODEL_H
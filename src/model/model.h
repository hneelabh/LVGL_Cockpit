/**
 * @file model.h
 * @brief Model layer - Data structures and business logic
 * 
 * Contains speedometer state, calculations, and data management
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

    // MUSIC DATA
    char track_title[64];
    char track_artist[64];
    char track_album[64];
} speedometer_state_t;

/**
 * @brief Turn signal state
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

/**
 * @brief Initialize speedometer state
 * @param state Pointer to speedometer state structure
 */
void model_init(speedometer_state_t *state);

/**
 * @brief Calculate gear based on speed
 * @param speed Current speed in km/h
 * @return Gear number (0=N, 1-6)
 */
int model_calculate_gear(int speed);

/**
 * @brief Calculate RPM based on speed and gear
 * @param speed Current speed in km/h
 * @param gear Current gear (0-6)
 * @return RPM value (1-13, representing x1000)
 */
int model_calculate_rpm(int speed, int gear);

/**
 * @brief Update speedometer state with new speed
 * @param state Pointer to speedometer state
 * @param new_speed New speed value
 */
void model_update_speed(speedometer_state_t *state, int new_speed);

/**
 * @brief Get speed zone index (for color determination)
 * @param speed Current speed
 * @return Zone index (0=green, 1=yellow, 2=orange, 3=red)
 */
int model_get_speed_zone(int speed);

/**
 * @brief Get RPM zone index (for color determination)
 * @param rpm Current RPM
 * @return Zone index (0=green, 1=orange, 2=red)
 */
int model_get_rpm_zone(int rpm);

#endif // MODEL_H
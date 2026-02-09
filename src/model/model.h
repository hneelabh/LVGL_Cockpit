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
    int duration_sec;
    int position_sec;
    bool is_playing;

    // --- NAVIGATION DATA ---
    char nav_street[64];    
    int nav_distance;       
    char nav_icon[8];       

} speedometer_state_t;

/**
 * @brief Turn signal state
 */
typedef struct {
    bool left_active;       
    bool right_active;      
    bool left_blink;        
    bool right_blink;       
    int blink_counter;      
} turn_signal_state_t;

/* ========================================================================
 * Function Prototypes
 * ======================================================================== */

void model_init(speedometer_state_t *state);

// Calculations
int model_calculate_gear(int speed);
int model_calculate_rpm(int speed, int gear);
void model_update_speed(speedometer_state_t *state, int new_speed);

int model_get_speed_zone(int speed);
int model_get_rpm_zone(int rpm);

// Command Sender
void model_send_music_cmd(const char *cmd);

#endif // MODEL_H
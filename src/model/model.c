/**
 * @file model.c
 * @brief Model layer implementation with Pauses and Blinkers
 */

#include "model.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

// --- DEFINITIONS & GLOBALS ---
#define SPEED_SOCKET "/tmp/lvgl_speed.sock"
#define MUSIC_SOCKET "/tmp/lvgl_music.sock"

// Left symbol from LVGL
#define LV_SYMBOL_LEFT "\xEF\x81\x93"

static int speed_sock = -1;
static int music_sock = -1;

// --- HELPER FUNCTION ---
static int open_socket(const char* path) {
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) return -1;
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    unlink(path); 
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    chmod(path, 0666); 
    return fd;
}

/* ========================================================================
 * Public Functions
 * ======================================================================== */

void model_init(speedometer_state_t *state)
{
    memset(state, 0, sizeof(speedometer_state_t));
    state->speed = 0;
    state->gear = 0; 
    state->rpm = 1;
    state->odometer = 12345;
    state->trip = 123.4f;
    state->fuel_level = 5;
    state->temperature = 0;

    // Set default music text
    strcpy(state->track_title, "Not Playing");
    strcpy(state->track_artist, "Connect Phone");
    strcpy(state->track_album, "");

    // Open BOTH sockets
    speed_sock = open_socket(SPEED_SOCKET);
    music_sock = open_socket(MUSIC_SOCKET);

    printf("🚗 Model: Simulation Active (Asymmetric + Pauses)\n");

    // Navigation
    strcpy(state->nav_street, "Kings Road");
    state->nav_distance = 500;
    strcpy(state->nav_icon, LV_SYMBOL_LEFT); // Need string for symbol
}

int model_calculate_gear(int speed)
{
    if (speed == 0) return 0;
    if (speed < 25) return 1;
    if (speed < 50) return 2;
    if (speed < 80) return 3;
    if (speed < 120) return 4;
    if (speed < 160) return 5;
    return 6;
}

int model_calculate_rpm(int speed, int gear)
{
    if (gear == 0 || speed == 0) return 1;
    float base_rpm;
    switch(gear) {
        case 1: base_rpm = speed * 0.20f; break;
        case 2: base_rpm = speed * 0.12f; break;
        case 3: base_rpm = speed * 0.08f; break;
        case 4: base_rpm = speed * 0.06f; break;
        case 5: base_rpm = speed * 0.05f; break;
        case 6: base_rpm = speed * 0.04f; break;
        default: base_rpm = speed * 0.06f; break;
    }
    int rpm = (int)(base_rpm + 2.0f);
    if (rpm < 1) rpm = 1;
    if (rpm > 13) rpm = 13;
    return rpm;
}

void model_update_speed(speedometer_state_t *state, int sim_speed)
{
    (void)sim_speed; // Silence unused parameter warning

    // Static variables preserve their value between function calls
    static float precise_speed = 0.0f; 
    static int direction = 1; // 1 = Accelerate, -1 = Decelerate
    static int pause_timer = 0; // Frames to wait

    // --- 1. HANDLE PAUSES ---
    if (pause_timer > 0) {
        pause_timer--;
        return; // Don't change speed while paused
    }

    // --- 2. ASYMMETRIC SIMULATION LOGIC ---
    if (direction == 1) {
        // Accelerating (Left Turn Signal ON)
        state->left_signal = true;   
        state->right_signal = false;
        
        precise_speed += 0.66f; // Rise in ~5 sec
        
        if (precise_speed >= 200.0f) {
            precise_speed = 200.0f;
            direction = -1; // Switch to braking
            
            // PAUSE AT TOP: 0.5s * 60FPS = 30 frames
            pause_timer = 30; 
        }
    } 
    else {
        // Decelerating (Right Turn Signal ON)
        state->left_signal = false;
        state->right_signal = true; 
        
        precise_speed -= 1.11f; // Fall in ~3 sec
        
        if (precise_speed <= 0.0f) {
            precise_speed = 0.0f;
            direction = 1; // Switch to accelerating
            
            // PAUSE AT BOTTOM: 1.0s * 60FPS = 60 frames
            pause_timer = 60;
            
            // Turn off all signals when stopped
            state->left_signal = false;
            state->right_signal = false;
        }
    }

    // Apply to state (Cast float to int)
    state->speed = (int)precise_speed;

    // --- 3. MUSIC DATA READING ---
    if (music_sock >= 0) {
        char buffer[128];
        int bytes = read(music_sock, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            char *token = strtok(buffer, "|");
            if (token) strncpy(state->track_title, token, 63);
            token = strtok(NULL, "|");
            if (token) strncpy(state->track_artist, token, 63);
            token = strtok(NULL, "|");
            if (token) strncpy(state->track_album, token, 63);
        }
    }

    // --- 4. UPDATE PHYSICS ---
    state->gear = model_calculate_gear(state->speed);
    state->rpm = model_calculate_rpm(state->speed, state->gear);

    // --- 5. NAVIGATION MOCK ---
    // Decrement distance if moving
    if (state->speed > 0) {
        // Decrease distance roughly based on speed (mock logic)
        // 100km/h = ~27m/s. At 60fps, that's ~0.45m per frame.
        static float dist_counter = 500.0f;
        dist_counter -= (state->speed * 0.005f); // Scale factor
        
        if (dist_counter <= 0) {
            dist_counter = 500.0f; // Reset for demo loop
            // Optional: Flip arrow
            // if (strcmp(state->nav_icon, LV_SYMBOL_LEFT) == 0) ...
        }
        state->nav_distance = (int)dist_counter;
    }
}

// --- MISSING FUNCTIONS (This was the cause of the error!) ---

int model_get_speed_zone(int speed)
{
    if (speed > 160) return 3; // Red
    if (speed > 120) return 2; // Orange
    if (speed > 60)  return 1; // Yellow
    return 0; // Green
}

int model_get_rpm_zone(int rpm)
{
    if (rpm > 10) return 2; // Red
    if (rpm > 7)  return 1; // Orange
    return 0; // Green
}
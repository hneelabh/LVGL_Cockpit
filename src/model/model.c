/**
 * @file model.c
 * @brief Model layer implementation with Bluetooth Socket Support
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

// --- DEFINITIONS & GLOBALS (The missing part!) ---
#define SPEED_SOCKET "/tmp/lvgl_speed.sock"
#define MUSIC_SOCKET "/tmp/lvgl_music.sock"

static int speed_sock = -1;
static int music_sock = -1;

// --- HELPER FUNCTION ---
static int open_socket(const char* path) {
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) return -1;

    // Set non-blocking so it doesn't freeze the UI
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    unlink(path); // Remove old file
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    
    // IMPORTANT: Allow Python (piuser) to write to this root-owned file
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

    printf("🚗 Model: Listening on Speed & Music sockets.\n");
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
    // 1. Read Speed from Python
    // if (speed_sock >= 0) {
    //     uint32_t received_speed = 0;
    //     int bytes = read(speed_sock, &received_speed, sizeof(received_speed));
        
    //     if (bytes > 0) {
    //         state->speed = (int)received_speed;
    //     }
    // }

    // --- 1.1. SIMULATION LOGIC (0->MAX->0) ---
    // 'static' means these variables remember their value between function calls
    static int direction = 1; // 1 = Accelerate, -1 = Decelerate
    
    // Change speed
    state->speed += direction;

    // Check bounds
    if (state->speed >= 200) {
        state->speed = 200;
        direction = -1; // Start slowing down
    } else if (state->speed <= 0) {
        state->speed = 0;
        direction = 1; // Start speeding up
    }

    // NOTE: We deliberately IGNORE 'speed_sock' here so the simulation runs.

    // 2. Read Music from Python
    if (music_sock >= 0) {
        char buffer[128];
        int bytes = read(music_sock, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0'; // Null terminate
            
            // Parse: "Title|Artist|Album"
            char *token = strtok(buffer, "|");
            if (token) strncpy(state->track_title, token, 63);
            
            token = strtok(NULL, "|");
            if (token) strncpy(state->track_artist, token, 63);
            
            token = strtok(NULL, "|");
            if (token) strncpy(state->track_album, token, 63);
        }
    }

    // 3. Update Physics
    state->gear = model_calculate_gear(state->speed);
    state->rpm = model_calculate_rpm(state->speed, state->gear);
}

int model_get_speed_zone(int speed)
{
    if (speed > 160) return 3;
    if (speed > 120) return 2;
    if (speed > 60)  return 1;
    return 0;
}

int model_get_rpm_zone(int rpm)
{
    if (rpm > 10) return 2;
    if (rpm > 7)  return 1;
    return 0;
}
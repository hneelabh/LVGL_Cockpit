/**
 * @file model.c
 * @brief Model layer implementation - Data Logic, Sockets, and Persistence
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

// --- CONFIGURATION ---
#define SPEED_SOCKET "/tmp/lvgl_speed.sock"
#define MUSIC_SOCKET "/tmp/lvgl_music.sock"
#define CMD_SOCKET   "/tmp/lvgl_cmd.sock"  
#define DATA_FILE    "vehicle_data.txt"

#define LV_SYMBOL_LEFT "\xEF\x81\x93" 

static int speed_sock = -1;
static int music_sock = -1;

/* ========================================================================
 * Helpers
 * ======================================================================== */

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

static void load_vehicle_data(speedometer_state_t *state) {
    FILE *f = fopen(DATA_FILE, "r");
    if (f) {
        if (fscanf(f, "%d %f", &state->odometer, &state->trip) != 2) {
            state->odometer = 0;
            state->trip = 0.0f;
        }
        fclose(f);
    } else {
        state->odometer = 0;
        state->trip = 0.0f;
    }
}

static void save_vehicle_data(const speedometer_state_t *state) {
    FILE *f = fopen(DATA_FILE, "w");
    if (f) {
        fprintf(f, "%d %.1f", state->odometer, state->trip);
        fclose(f);
    }
}

/* ========================================================================
 * Public Functions
 * ======================================================================== */

void model_send_music_cmd(const char *cmd) {
    int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) return;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, CMD_SOCKET);

    sendto(sock, cmd, strlen(cmd), 0, (struct sockaddr *)&addr, sizeof(addr));
    close(sock);
}

void model_init(speedometer_state_t *state)
{
    memset(state, 0, sizeof(speedometer_state_t));
    
    // Load Persistence
    load_vehicle_data(state);
    
    // Defaults
    state->rpm = 1;
    state->fuel_level = 5;
    
    // Navigation Defaults
    strcpy(state->nav_street, "Kings Road");
    state->nav_distance = 500;
    strcpy(state->nav_icon, LV_SYMBOL_LEFT);

    // Music Defaults
    strcpy(state->track_title, "Not Playing");
    strcpy(state->track_artist, "Connect Phone");
    strcpy(state->track_album, "");

    // Sockets
    speed_sock = open_socket(SPEED_SOCKET);
    music_sock = open_socket(MUSIC_SOCKET);

    printf("🚗 Model Initialized.\n");
}

int model_calculate_gear(int speed) {
    if (speed == 0) return 0;
    if (speed < 25) return 1;
    if (speed < 50) return 2;
    if (speed < 80) return 3;
    if (speed < 120) return 4;
    if (speed < 160) return 5;
    return 6;
}

int model_calculate_rpm(int speed, int gear) {
    if (gear == 0 || speed == 0) return 1;
    float base_rpm = speed * 0.06f; // Fallback
    switch(gear) {
        case 1: base_rpm = speed * 0.20f; break;
        case 2: base_rpm = speed * 0.12f; break;
        case 3: base_rpm = speed * 0.08f; break;
        case 4: base_rpm = speed * 0.06f; break;
        case 5: base_rpm = speed * 0.05f; break;
        case 6: base_rpm = speed * 0.04f; break;
    }
    int rpm = (int)(base_rpm + 2.0f);
    return (rpm > 13) ? 13 : ((rpm < 1) ? 1 : rpm);
}

void model_update_speed(speedometer_state_t *state, int sim_speed)
{
    (void)sim_speed; 
    static float precise_speed = 0.0f; 
    static int direction = 1; 
    static int pause_timer = 0; 

    // 1. Simulation Logic (Acceleration/Deceleration)
    if (pause_timer > 0) {
        pause_timer--;
    } else {
        if (direction == 1) { // Accelerate
            state->left_signal = true;   
            state->right_signal = false;
            precise_speed += 0.66f; 
            if (precise_speed >= 200.0f) {
                precise_speed = 200.0f;
                direction = -1; 
                pause_timer = 30; 
            }
        } else { // Decelerate
            state->left_signal = false;
            state->right_signal = true; 
            precise_speed -= 1.11f; 
            if (precise_speed <= 0.0f) {
                precise_speed = 0.0f;
                direction = 1; 
                pause_timer = 60; 
                state->left_signal = false;
                state->right_signal = false;
            }
        }
    }
    state->speed = (int)precise_speed;

    // 2. Odometer Logic
    if (state->speed > 0) {
        float dist_frame = state->speed * 0.00000444f;
        state->trip += dist_frame;
        
        static float odo_accumulator = 0.0f;
        odo_accumulator += dist_frame;
        if (odo_accumulator >= 0.2f) { 
            state->odometer++;
            odo_accumulator -= 0.2f; 
            save_vehicle_data(state);
        }
    }

    // 3. Socket Reading (Music)
    if (music_sock >= 0) {
        char buffer[512]; // Sufficient for long titles
        int bytes = read(music_sock, buffer, sizeof(buffer) - 1);
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            
            // Parse: Title | Artist | Album | Dur | Pos | Status
            char *token = strtok(buffer, "|");
            if (token) strncpy(state->track_title, token, 63);
            
            token = strtok(NULL, "|");
            if (token) strncpy(state->track_artist, token, 63);
            
            token = strtok(NULL, "|");
            if (token) strncpy(state->track_album, token, 63);

            token = strtok(NULL, "|");
            if (token) state->duration_sec = atoi(token);
            
            token = strtok(NULL, "|");
            if (token) state->position_sec = atoi(token);

            // Status Parsing (Case Insensitive Fix)
            token = strtok(NULL, "|");
            if (token) {
                // Accepts "Playing", "playing", "PLAYING"
                if (strstr(token, "laying") != NULL) { 
                    state->is_playing = true;
                } else {
                    state->is_playing = false;
                }
            } else {
                 state->is_playing = false;
            }
        }
    }

    // 4. Update Dependent States
    state->gear = model_calculate_gear(state->speed);
    state->rpm = model_calculate_rpm(state->speed, state->gear);

    // 5. Navigation Mock
    if (state->speed > 0) {
        static float dist_counter = 500.0f;
        dist_counter -= (state->speed * 0.005f); 
        if (dist_counter <= 0) dist_counter = 500.0f; 
        state->nav_distance = (int)dist_counter;
    }
}

// Getters
int model_get_speed_zone(int speed) {
    if (speed > 160) return 3; 
    if (speed > 120) return 2; 
    if (speed > 60)  return 1; 
    return 0; 
}

int model_get_rpm_zone(int rpm) {
    if (rpm > 10) return 2; 
    if (rpm > 7)  return 1; 
    return 0; 
}
/*
 * camera_streamer.c
 *
 * Downscales and simultaneously packs the pixel format from 4-byte BGRX to 3-byte BGR, removing 25% of the data
 * for the same resolution to achieve maximum speed over TCP.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <termios.h>
#include <camera/camera_api.h>

// --- Configuration ---
#define SERVER_IP "" // The IP address of the host PC running the Python server
#define SERVER_PORT 12345

/*
                 DOWNSCALE_FACTOR      Resolution (W x H)
    High Quality        8                  288 x 162
    Balanced            12                 192 x 108
    High Speed          18                 128 x 72
    Extreme Speed       24                 96 x 54
*/
#define DOWNSCALE_WIDTH 192
#define DOWNSCALE_HEIGHT 108
#define DOWNSCALE_FACTOR 12

// --- Global variables ---
// g_sock is the socket file descriptor for the TCP connection.
static int g_sock = -1;
// g_small_frame_buffer holds the downscaled and packed BGR frame data.
static uint8_t* g_small_frame_buffer = NULL;

/**
 * @brief Downscales a large BGRX frame to a smaller BGR frame.
 * @param big_frame The original high-resolution camera buffer.
 * @param small_buffer The destination buffer for the processed frame.
 */
void downscale_and_pack_frame(const camera_buffer_t* big_frame, uint8_t* small_buffer) {
    uint32_t src_width = big_frame->framedesc.bgr8888.width;
    const uint8_t* src_buf = (const uint8_t*)big_frame->framebuf;
    int small_buf_idx = 0;

    // Iterate through each pixel of the target downscaled image.
    for (int y = 0; y < DOWNSCALE_HEIGHT; ++y) {
        for (int x = 0; x < DOWNSCALE_WIDTH; ++x) {
            // Calculate the corresponding pixel in the source image using the downscale factor.
            const uint8_t* src_pixel_ptr = &src_buf[(y * DOWNSCALE_FACTOR) * src_width * 4 + (x * DOWNSCALE_FACTOR) * 4];
            
            // Manually copy B, G, R bytes, skipping the 4th (X/alpha) byte to pack the data.
            small_buffer[small_buf_idx++] = src_pixel_ptr[0]; // Blue
            small_buffer[small_buf_idx++] = src_pixel_ptr[1]; // Green
            small_buffer[small_buf_idx++] = src_pixel_ptr[2]; // Red
        }
    }
}

/**
 * @brief Callback function executed by the camera API for each new frame.
 * @param handle The handle to the camera.
 * @param buffer The buffer containing the new frame data.
 * @param arg A user-defined argument (not used).
 */
static void processCameraData(camera_handle_t handle, camera_buffer_t* buffer, void* arg) {
    static int frame_counter = 0;
    if (g_sock < 0 || g_small_frame_buffer == NULL) return;

    // Process every other frame to reduce CPU load and network traffic.
    if (++frame_counter % 2 != 0) {
        return;
    }

    // Ensure the frame is in the expected BGR8888 format.
    if (buffer->frametype == CAMERA_FRAMETYPE_BGR8888) {
        // Downscale and pack the frame.
        downscale_and_pack_frame(buffer, g_small_frame_buffer);
        // Send the processed frame over the TCP socket.
        uint32_t small_frame_size = DOWNSCALE_WIDTH * DOWNSCALE_HEIGHT * 3;
        write(g_sock, g_small_frame_buffer, small_frame_size);
    }
}

int main(void) {
    camera_handle_t handle = CAMERA_HANDLE_INVALID;
    struct sockaddr_in server;
    struct hostent *hp;

    // Allocate memory for the downscaled frame buffer.
    g_small_frame_buffer = malloc(DOWNSCALE_WIDTH * DOWNSCALE_HEIGHT * 3);
    if (!g_small_frame_buffer) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return EXIT_FAILURE;
    }

    // --- Setup TCP Socket Connection ---
    printf("Connecting to %s:%d...\n", SERVER_IP, SERVER_PORT);
    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    hp = gethostbyname(SERVER_IP);
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_port = htons(SERVER_PORT);
    if (connect(g_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("ERROR connecting");
        free(g_small_frame_buffer);
        return EXIT_FAILURE;
    }

    // Disable Nagle's algorithm to reduce latency for small packets.
    int flag = 1;
    setsockopt(g_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    printf(" -> Connected.\n");

    // --- Setup Camera ---
    if (camera_open(CAMERA_UNIT_1, CAMERA_MODE_RW, &handle) != CAMERA_EOK) {
        fprintf(stderr, "Failed to open camera.\n");
        free(g_small_frame_buffer);
        return EXIT_FAILURE;
    }
    printf(" -> Camera opened.\n");

    // Start capturing frames and processing them with the callback.
    if (camera_start_viewfinder(handle, &processCameraData, NULL, NULL) != CAMERA_EOK) {
        fprintf(stderr, "Failed to start viewfinder.\n");
        camera_close(handle);
        free(g_small_frame_buffer);
        return EXIT_FAILURE;
    }

    // --- Wait for user input to terminate ---
    printf("\nStreaming... Press any key to stop.\n");
    struct termios oldterm, newterm;
    tcgetattr(STDIN_FILENO, &oldterm);
    newterm = oldterm;
    newterm.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode.
    tcsetattr(STDIN_FILENO, TCSANOW, &newterm);
    read(STDIN_FILENO, NULL, 1); // Block until a key is pressed.
    tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);

    // --- Cleanup ---
    printf("\nStopping...\n");
    camera_stop_viewfinder(handle);
    camera_close(handle);
    close(g_sock);
    free(g_small_frame_buffer);
    return EXIT_SUCCESS;
}

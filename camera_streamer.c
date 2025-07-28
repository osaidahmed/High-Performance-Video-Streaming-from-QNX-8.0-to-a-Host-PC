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

#define SERVER_IP "" 
#define SERVER_PORT 12345

/*
                	DOWNSCALE_FACTOR	    Resolution (W x H)
    High Quality	    8	                    288 x 162
    Balanced	        12	                    192 x 108
    High Speed	        18	                    128 x 72
    Extreme Speed	    24	                    96 x 54
*/

#define DOWNSCALE_WIDTH 192
#define DOWNSCALE_HEIGHT 108
#define DOWNSCALE_FACTOR 12 

static int g_sock = -1;
static uint8_t* g_small_frame_buffer = NULL; // Now uint8_t for byte access

void downscale_and_pack_frame(const camera_buffer_t* big_frame, uint8_t* small_buffer) {
    uint32_t src_width = big_frame->framedesc.bgr8888.width;
    const uint8_t* src_buf = (const uint8_t*)big_frame->framebuf;
    int small_buf_idx = 0;

    for (int y = 0; y < DOWNSCALE_HEIGHT; ++y) {
        for (int x = 0; x < DOWNSCALE_WIDTH; ++x) {
            // Get the memory location of the source pixel
            const uint8_t* src_pixel_ptr = &src_buf[(y * DOWNSCALE_FACTOR) * src_width * 4 + (x * DOWNSCALE_FACTOR) * 4];
            
            // Copy only the B, G, and R bytes, skipping the 4th byte
            small_buffer[small_buf_idx++] = src_pixel_ptr[0]; // Blue
            small_buffer[small_buf_idx++] = src_pixel_ptr[1]; // Green
            small_buffer[small_buf_idx++] = src_pixel_ptr[2]; // Red
        }
    }
}

static void processCameraData(camera_handle_t handle, camera_buffer_t* buffer, void* arg) {
    static int frame_counter = 0;
    if (g_sock < 0 || g_small_frame_buffer == NULL) return;

    // Skip every other frame to reduce CPU load
    if (++frame_counter % 2 != 0) {
        return;
    }

    if (buffer->frametype == CAMERA_FRAMETYPE_BGR8888) {
        downscale_and_pack_frame(buffer, g_small_frame_buffer);
        uint32_t small_frame_size = DOWNSCALE_WIDTH * DOWNSCALE_HEIGHT * 3; // Note: size is now * 3
        write(g_sock, g_small_frame_buffer, small_frame_size);
    }
}

int main(void) {
    camera_handle_t handle = CAMERA_HANDLE_INVALID;
    struct sockaddr_in server;
    struct hostent *hp;

    // Buffer is now 3 bytes per pixel
    g_small_frame_buffer = malloc(DOWNSCALE_WIDTH * DOWNSCALE_HEIGHT * 3);
    if (!g_small_frame_buffer) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return EXIT_FAILURE;
    }

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

    int flag = 1;
    setsockopt(g_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    printf(" -> Connected.\n");

    if (camera_open(CAMERA_UNIT_1, CAMERA_MODE_RW, &handle) != CAMERA_EOK) {
        fprintf(stderr, "Failed to open camera.\n");
        free(g_small_frame_buffer);
        return EXIT_FAILURE;
    }
    printf(" -> Camera opened.\n");

    if (camera_start_viewfinder(handle, &processCameraData, NULL, NULL) != CAMERA_EOK) {
        fprintf(stderr, "Failed to start viewfinder.\n");
        camera_close(handle);
        free(g_small_frame_buffer);
        return EXIT_FAILURE;
    }

    printf("\nStreaming... Press any key to stop.\n");
    struct termios oldterm, newterm;
    tcgetattr(STDIN_FILENO, &oldterm);
    newterm = oldterm;
    newterm.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newterm);
    read(STDIN_FILENO, NULL, 1);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);

    printf("\nStopping...\n");
    camera_stop_viewfinder(handle);
    camera_close(handle);
    close(g_sock);
    free(g_small_frame_buffer);
    return EXIT_SUCCESS;
}
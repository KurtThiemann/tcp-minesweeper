#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.c"
#include "server.c"
#include "util.c"
#include "game.c"

int server_port = 4242;

void client_send_welcome(int client_id) {
    char msg[512] = {0};
    int len = sprintf(msg, "Welcome to TCP Minesweeper\r\n"
                           "Controls:\r\n"
                           "    W, A, S, D / Arrow Keys -> Move cursor\r\n"
                           "    Space -> Reveal field\r\n"
                           "    F -> Flag field\r\n"
                           "    Q -> exit\r\n"
                           "Your ID is: %d\r\n", client_id);
    server_send_client(client_id, msg, len);
    game_client_send_space(client_id);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("%s [<width> <height> <mines> <port>]\n", argv[0]);
            return 0;
        }
        if (argc >= 2) {
            game_size.x = clamp(atoi(argv[1]), 8, 256);
        }
        if (argc >= 3) {
            game_size.y = clamp(atoi(argv[2]), 8, 256);
        }
        if (argc >= 4) {
            game_mines = clamp(atoi(argv[3]), 1, (game_size.x * game_size.y) / 2);
        }
        if (argc >= 5) {
            server_port = clamp(atoi(argv[4]), 1, 999999);
        }
    }

    game_reset();
    server_start(server_port);
    atexit(server_close);

    char *render_buffer = malloc(game_get_render_buffer_size());

    int render_next_frame = 1;
    while (1) {
        render_next_frame = render_next_frame || server_accept_client();
        render_next_frame = game_update(render_next_frame, render_buffer);
    }
    return 0;
}

#pragma clang diagnostic pop

char char_field_numbers[] = " 12345678";
char char_flag[] = "\u2691";
char char_mine[] = "\u25cf";

int background_color_open[] = {47, 40};
int foreground_color_open[] = {30, 37};

Field *game;
Vector2 game_size = {30, 20};
int game_over = 0;
int game_mines = 80;
int game_opened_fields = 0;
Client *game_leading_client;

/**
 * Get a pointer to the Field at pos
 *
 * @param pos
 * @return
 */
Field *game_get_field(Vector2 pos) {
    return (Field *) (game + (game_size.x * pos.y + pos.x) * 1);
}

/**
 * Check if a position in the game is valid
 *
 * @param pos
 * @return
 */
int game_pos_valid(Vector2 pos) {
    return !(pos.x < 0 || pos.x >= game_size.x || pos.y < 0 || pos.y >= game_size.y);
}

/**
 * Get the amount of mines around the Field at pos
 *
 * @param pos
 * @return
 */
int game_field_get_mines(Vector2 pos) {
    int m = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            Vector2 f_pos = {pos.x + x, pos.y + y};
            if (!game_pos_valid(f_pos)) {
                continue;
            }
            m += (signed) (game_get_field(f_pos)->mine);
        }
    }
    return m;
}

/**
 * Place mines in the game and calculate near mines for all fields
 * Don't place a mine at start point
 *
 * @param start
 */
void game_populate(Vector2 start) {
    srand(time(NULL));
    for (int i = 0; i < game_mines; i++) {
        Vector2 pos = {random_int(0, game_size.x - 1), random_int(0, game_size.y - 1)};
        Field *field = game_get_field(pos);
        if (field->mine == (unsigned) 1 || (pos.x == start.x && pos.y == start.y)) {
            i--;
            continue;
        }
        field->mine = (unsigned) 1;
    }
    for (int y = 0; y < game_size.y; y++) {
        for (int x = 0; x < game_size.x; x++) {
            Vector2 pos = {x, y};
            Field *field = game_get_field(pos);
            field->value = game_field_get_mines(pos);
        }
    }
}

/**
 * Check if a position is the cursor position of a client
 *
 * @param pos
 * @return
 */
int game_pos_is_cursor(Vector2 pos) {
    for (int cid = 0; cid < server_max_clients; cid++) {
        if (!server_client_active(cid)) {
            continue;
        }
        Vector2 cursor = server_get_client(cid)->cursor;
        if (cursor.x == pos.x && cursor.y == pos.y) {
            return 1;
        }
    }
    return 0;
}

/**
 * Print the game to a buffer
 */
int game_render(int client_id, char *render_buffer) {
    Client *client = server_get_client(client_id);
    int background_color = background_color_open[0];
    int foreground_color = foreground_color_open[0];

    char *rbp = render_buffer;
    int len = 0;
    len += sprintf(rbp + len, "\033[%dA\r\033[%dm\033[%dm", game_size.y + 1, background_color, foreground_color);
    for (int y = 0; y < game_size.y; y++) {
        for (int x = 0; x < game_size.x; x++) {
            Vector2 pos = {x, y};
            Field *field = game_get_field(pos);

            int bg = background_color_open[(unsigned) field->opened && !(unsigned) field->mine];
            if (x == client->cursor.x && y == client->cursor.y) {
                bg = 43;
            } else if (game_pos_is_cursor(pos)) {
                bg = 44;
            }
            if (bg != background_color) {
                background_color = bg;
                len += sprintf(rbp + len, "\033[%dm", bg);
            }
            int fg = foreground_color_open[(unsigned) field->opened && !(unsigned) field->mine];
            if (fg != foreground_color) {
                foreground_color = fg;
                len += sprintf(rbp + len, "\033[%dm", fg);
            }

            if ((unsigned) field->opened) {
                if ((unsigned) field->mine) {
                    len += sprintf(rbp + len, "%s ", char_mine);
                } else {
                    len += sprintf(rbp + len, "%c ", char_field_numbers[(unsigned) field->value]);
                }
            } else if ((unsigned) field->flagged) {
                len += sprintf(rbp + len, "%s ", char_flag);
            } else {
                len += sprintf(rbp + len, "  ");
            }
        }
        len += sprintf(rbp + len, "\r\n");
    }
    len += sprintf(rbp + len, "\033[0mScore: %d   Leading player: #%d (Score: %d)   Fields remaining: %d      \r\n",
                   client->score,
                   game_leading_client->cid,
                   game_leading_client->score,
                   (game_size.x * game_size.y) - game_mines - game_opened_fields);
    return len;
}

/**
 * Reveal a field in the game
 * If the field has a value of 0, all surrounding fields are revealed as well
 *
 * @param pos
 */
void game_reveal_field(Vector2 pos, Client *client) {
    Field *field = game_get_field(pos);
    if (field->opened == (unsigned) 1 || field->flagged == (unsigned) 1) {
        return;
    }
    field->opened = 1;

    if (field->mine) {
        client->score = 0;
        client->cursor = (Vector2) {0, 0};
        return;
    }

    game_opened_fields++;
    client->score += (signed) field->value;
    if (game_opened_fields == (game_size.x * game_size.y) - game_mines) {
        game_over = 2;
    }

    if (field->value == (unsigned) 0) {
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                if (x == 0 && y == 0) {
                    continue;
                }
                Vector2 new_pos = {pos.x + x, pos.y + y};
                if (!game_pos_valid(new_pos)) {
                    continue;
                }
                game_reveal_field(new_pos, client);
            }
        }
    }
}

/**
 * Flag a field
 *
 * @param pos
 */
void game_flag_field(Client *client) {
    game_get_field(client->cursor)->flagged ^= (unsigned) 1;
}

/**
 * Prevent the cursor from leaving the game area
 */
void game_overflow_cursor(Client *client) {
    client->cursor.x = (game_size.x + client->cursor.x) % game_size.x;
    client->cursor.y = (game_size.y + client->cursor.y) % game_size.y;
}

/**
 * Process the user input
 *
 * @param key
 */
int game_process_input(int client_id) {
    Client *client = server_get_client(client_id);
    switch (server_client_get_key(client_id)) {
        case 'w':
            client->cursor.y--;
            game_overflow_cursor(client);
            return 1;
        case 's':
            client->cursor.y++;
            game_overflow_cursor(client);
            return 1;
        case 'a':
            client->cursor.x--;
            game_overflow_cursor(client);
            return 1;
        case 'd':
            client->cursor.x++;
            game_overflow_cursor(client);
            return 1;
        case ' ':
            game_reveal_field(client->cursor, client);
            return 1;
        case 'f':
            game_flag_field(client);
            return 1;
        case 'q':
            server_client_disconnect(client_id);
            return 1;
        default:
            return 0;
    }
}

/**
 * Reset the game
 */
void game_reset() {
    printf("Starting new game...\n");
    game_over = 0;
    game_opened_fields = 0;
    game = malloc(game_size.x * game_size.y * sizeof(Field));
    memset(game, 0, game_size.x * game_size.y * sizeof(Field));

    game_populate((Vector2) {0, 0});
}

/**
 * Send (game height) line breaks to a client
 *
 * @param client_id
 */
void game_client_send_space(int client_id) {
    char msg[256];
    memset(msg, '\n', game_size.y + 2);
    server_send_client(client_id, msg, game_size.y + 2);
}

/**
 * Update the current leading player/winner
 */
void game_update_winner() {
    Client *best_client = server_get_client(0);
    for (int cid = 0; cid < server_max_clients; cid++) {
        if (!server_client_active(cid)) {
            continue;
        }
        Client *client = server_get_client(cid);
        if (client->score >= best_client->score) {
            best_client = client;
        }
    }
    game_leading_client = best_client;
}

/**
 * Get the game render buffer size
 * @return
 */
int game_get_render_buffer_size(){
    return game_size.x * 2 * game_size.y * 4;
}

/**
 * Update the game
 * - process new input from all clients
 * - render and send game to all clients if necessary
 * - check if game ended, announce winner, reset game
 *
 * @param render_next_frame
 * @param game_render_buffer
 * @return
 */
int game_update(int render_next_frame, char *game_render_buffer){
    for (int cid = 0; cid < server_max_clients; cid++) {
        if (!server_client_active(cid) || !server_update_client(cid)) {
            continue;
        }
        render_next_frame = render_next_frame || game_process_input(cid);
    }
    if (render_next_frame) {
        game_update_winner();
        for (int cid = 0; cid < server_max_clients; cid++) {
            if (!server_client_active(cid)) {
                continue;
            }
            bzero(game_render_buffer, game_get_render_buffer_size());
            int len = game_render(cid, game_render_buffer);
            server_send_client(cid, game_render_buffer, len);
        }
        render_next_frame = 0;
    }
    if (game_over) {
        char win_msg[64] = {0};
        game_update_winner();
        int win_msg_len = sprintf(win_msg, "\r\nWinner: Player #%d (Score: %d)\r\n", game_leading_client->cid,
                                  game_leading_client->score);
        for (int cid = 0; cid < server_max_clients; cid++) {
            if (!server_client_active(cid)) {
                continue;
            }
            Client *client = server_get_client(cid);
            client->score = 0;
            client->cursor = (Vector2) {0, 0};
            server_send_client(cid, win_msg, win_msg_len);
            game_client_send_space(cid);
        }
        game_reset();
        render_next_frame = 1;
    }
    return render_next_frame;
}
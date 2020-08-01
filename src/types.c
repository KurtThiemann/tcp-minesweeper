typedef struct Vector2 {
    signed int x;
    signed int y;
} Vector2;

typedef struct Field {
    unsigned int mine: 1;
    unsigned int flagged: 1;
    unsigned int opened: 1;
    unsigned int value: 3;
} Field;

typedef struct Client {
    int cid;
    int fd;
    int disconnect;
    int score;
    long last_update;
    Vector2 cursor;
} Client;
/**
 * Generate a random int in range min -> max
 *
 * @param min
 * @param max
 * @return
 */
int random_int(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

/**
 * Get the smaller of two values
 *
 * @param n1
 * @param n2
 * @return
 */
int min(int n1, int n2) {
    if (n1 < n2) {
        return n1;
    }
    return n2;
}

/**
 * Get the bigger of two values
 *
 * @param n1
 * @param n2
 * @return
 */
int max(int n1, int n2) {
    if (n1 > n2) {
        return n1;
    }
    return n2;
}

/**
 * Clamp a value between min and max
 *
 * @param number
 * @param min_n
 * @param max_n
 * @return
 */
int clamp(int number, int min_n, int max_n) {
    return max(min_n, min(number, max_n));
}
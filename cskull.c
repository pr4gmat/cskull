#define NCURSES_WIDECHAR 1

#include <ncurses.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINES 200
#define MAX_LEN 200

// Represents one falling stream of characters
typedef struct {
    int y;        // current vertical position
    int speed;    // falling speed (lower = faster)
    int length;   // length of the stream
    int counter;  // frame counter
} Stream;

static char skull_lines[MAX_LINES][MAX_LEN];
static int skull_height = 0;
static int skull_width = 0;

// Japanese-style characters for the rain effect
static const wchar_t *matrix_chars[] = {
    L"日", L"本", L"語", L"力", L"夢", L"電", L"光", L"心", L"流",
    L"界", L"神", L"無", L"空", L"天", L"黒", L"白", L"龍", L"星"
};
static const int num_matrix_chars = sizeof(matrix_chars) / sizeof(matrix_chars[0]);

// Load skull ASCII art from file
void load_skull(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Failed to open skull.txt");
        exit(1);
    }
    skull_height = 0;
    skull_width = 0;
    while (fgets(skull_lines[skull_height], MAX_LEN, file)) {
        skull_lines[skull_height][strcspn(skull_lines[skull_height], "\n")] = '\0';
        int w = strlen(skull_lines[skull_height]);
        if (w > skull_width) skull_width = w;
        skull_height++;
    }
    fclose(file);
}

// Convert color name to color pair ID
int color_from_name(const char *name) {
    if (!name) return 1;
    if (!strcasecmp(name, "green"))   return 1;
    if (!strcasecmp(name, "red"))     return 2;
    if (!strcasecmp(name, "blue"))    return 3;
    if (!strcasecmp(name, "yellow"))  return 4;
    if (!strcasecmp(name, "magenta")) return 5;
    if (!strcasecmp(name, "cyan"))    return 6;
    if (!strcasecmp(name, "white"))   return 7;
    return 1;
}

// Display help message
void print_help() {
    printf("Usage:\n");
    printf("  ./cskull -r <rain_color> -s <skull_color>\n\n");
    printf("Available colors: green, red, blue, yellow, magenta, cyan, white\n\n");
    printf("Example:\n");
    printf("  ./cskull -r green -s red\n");
}

int main(int argc, char **argv) {
    srand(time(NULL));
    setlocale(LC_ALL, "");

    // Show help if no parameters are provided
    if (argc < 5) {
        print_help();
        return 0;
    }

    const char *rain_name = NULL;
    const char *skull_name = NULL;

    // Parse CLI arguments
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-r") && i + 1 < argc) rain_name = argv[++i];
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) skull_name = argv[++i];
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_help();
            return 0;
        }
    }

    if (!rain_name || !skull_name) {
        print_help();
        return 0;
    }

    int rain_color = color_from_name(rain_name);
    int skull_color = color_from_name(skull_name);

    load_skull("assets/skull.txt");

    // Initialize ncurses
    initscr();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();

    // Define color pairs
    init_pair(1, COLOR_GREEN, -1);
    init_pair(2, COLOR_RED, -1);
    init_pair(3, COLOR_BLUE, -1);
    init_pair(4, COLOR_YELLOW, -1);
    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_CYAN, -1);
    init_pair(7, COLOR_WHITE, -1);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Slightly denser rain (spacing = 2)
    int spacing = 2;
    int num_streams = max_x / spacing;

    Stream streams[num_streams];
    for (int i = 0; i < num_streams; i++) {
        streams[i].y = rand() % max_y;
        streams[i].speed = 1 + rand() % 3;
        streams[i].length = 8 + rand() % 10;
        streams[i].counter = 0;
    }

    // Center the skull on screen
    int skull_y = max_y / 2 - skull_height / 2;
    int skull_x = max_x / 2 - skull_width / 2;

    // Main animation loop
    while (1) {
        for (int i = 0; i < num_streams; i++) {
            Stream *s = &streams[i];
            s->counter++;
            int x = i * spacing;

            if (s->counter % s->speed == 0) {
                // Clear tail of the stream
                int tail = s->y - s->length;
                if (tail >= 0 && tail < max_y)
                    mvaddch(tail, x, ' ');

                // Draw head of the stream
                if (s->y >= 0 && s->y < max_y) {
                    int in_skull =
                        (s->y >= skull_y && s->y < skull_y + skull_height &&
                         x >= skull_x && x < skull_x + skull_width);
                    if (!in_skull) {
                        attron(COLOR_PAIR(rain_color));
                        const wchar_t *wc = matrix_chars[rand() % num_matrix_chars];
                        mvaddwstr(s->y, x, wc);
                        attroff(COLOR_PAIR(rain_color));
                    }
                }

                // Move the stream down
                s->y++;
                if (s->y - s->length > max_y) {
                    s->y = -rand() % 20;
                    s->length = 8 + rand() % 10;
                }
            }
        }

        // Draw skull ASCII art in the center
        attron(COLOR_PAIR(skull_color));
        for (int i = 0; i < skull_height; i++) {
            mvprintw(skull_y + i, skull_x, "%s", skull_lines[i]);
        }
        attroff(COLOR_PAIR(skull_color));

        refresh();
        usleep(66666); // ~15 FPS

        // Exit on any key press
        if (getch() != ERR) break;
    }

    endwin();
    return 0;
}
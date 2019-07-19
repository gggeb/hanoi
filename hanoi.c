#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <curses.h>

#define EMPTY   0
#define NO_DISK 0

#define POLES 3

#define POS  (cursor / power)
#define DISK (cursor % power)

#define MIN_MOVES ((int)(pow(2, disks) + 0.5) - 1)

#define PADDING 2
#define DISK_M  2

#define DISK_WIDTH(i)  (i * DISK_M + 1)
#define MAX_DISK_WIDTH DISK_WIDTH(disks)

#define REQ_WIDTH  (POLES * (disks * DISK_M) + (POLES + 1) * PADDING)
#define REQ_HEIGHT (disks + PADDING * 2 + 1)

#define POLE_CHAR   '|'
#define DISK_CHAR   'X'
#define CURSOR_CHAR '+'

#define EVEN_CHAR 'E'
#define ODD_CHAR  'O'

// GLOBALS

int *poles[POLES], power = 10, disks = 3, using_colors = 0, cursor, moves;

// LOGIC

void allocate_poles(void) {
    int i;
    for (i = 0; i < POLES; i++)
        poles[i] = malloc(disks * sizeof(int));
}

void free_poles(void) {
    int i;
    for (i = 0; i < POLES; i++)
        free(poles[i]);
}

void init_data(void) {
    cursor = 0;
    moves = 0;

    int i, j;
    for (i = disks; i > 0; i--)
        poles[0][disks - i] = i;

    for (i = 1; i < POLES; i++)
        for (j = 0; j < disks; j++)
            poles[i][j] = EMPTY;
}

void cursor_left(void) {
    if (cursor >= power)
        cursor -= power;
}

void cursor_right(void) {
    if (cursor < power * (POLES - 1))
        cursor += power;
}

int *top_pos(void) {
    int i;
    for (i = 0; poles[POS][i] != EMPTY && i < disks; i++) ;

    return &poles[POS][i - 1];
}

void raise_disk(void) {
    if (DISK == NO_DISK)
        if (*top_pos() != EMPTY) {
            cursor += *top_pos();
            *top_pos() = EMPTY;
        }
}

void lower_disk(void) {
    if (DISK != NO_DISK && (*top_pos() > DISK || *top_pos() == EMPTY)) {
        *(top_pos() + 1) = DISK;
        cursor -= DISK;

        moves++;
    }
}

int solved(void) {
    int i;
    for (i = 0; i < disks; i++)
        if (poles[POLES - 1][i] != disks - i)
            return 0;
    return 1;
}

void handle_key(int k) {
    switch (k) {
    case KEY_LEFT:
        cursor_left();
        break;
    case KEY_RIGHT:
        cursor_right();
        break;
    case KEY_UP:
        raise_disk();
        break;
    case KEY_DOWN:
        lower_disk();
        break;
    case 'r':
        init_data();
        break;
    }
}

// TERMINAL

void calculate_margins(int *xmp, int *ymp) {
    int w, h;
    getmaxyx(stdscr, h, w);
    
    *xmp = (w - REQ_WIDTH) / 2 - 1;
    *ymp = (h - REQ_HEIGHT) / 2 - 2;
}

void init_ncurses(void) {
    initscr();

    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    if (has_colors() == TRUE && using_colors != -1) {
        start_color();

        using_colors = 1;

        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_BLUE, COLOR_BLACK);
    } else
        using_colors = 0;
}

void render_info(void) {
    int w, h;
    getmaxyx(stdscr, h, w);

    if (using_colors)
        attron(COLOR_PAIR(1));

    move(h - 1, 0);
    printw("MOVES: %d/%d, CURSOR: %d. WIDTH: %d, HEIGHT: %d.",
           moves, MIN_MOVES, cursor, w, h);
    
    if (using_colors)
        attroff(COLOR_PAIR(1));
}

void render_disk(int pole, int height, int disk, char empty_char)
{
    int xm, ym, x, y, k;
    
    calculate_margins(&xm, &ym);
    
    x = xm + pole * MAX_DISK_WIDTH + (pole + 1) * PADDING;
    y = ym + PADDING + (disks - height) + 2;
   
    if (disk == EMPTY) {
        x += MAX_DISK_WIDTH / 2;
       
        if (using_colors)
            attron(COLOR_PAIR(1));

        move(y, x);
        printw("%c", empty_char);
        
        if (using_colors)
            attroff(COLOR_PAIR(1));
    } else {
        x += (MAX_DISK_WIDTH - DISK_WIDTH(disk)) / 2;

        if (using_colors)
            attron(COLOR_PAIR(disk % 2 ? 3 : 2));

        for (k = 0; k < DISK_WIDTH(disk); k++, x++) {
            move(y, x);
            printw("%c", using_colors ? DISK_CHAR
                                      : (disk % 2 ? ODD_CHAR : EVEN_CHAR));
        }

        if (using_colors)
            attroff(COLOR_PAIR(disk % 2 ? 3 : 2));
    }
}

void render(void) {
    int w, h;
    getmaxyx(stdscr, h, w);

    clear();
    if (w < REQ_WIDTH || h < REQ_HEIGHT)
        printw("window is too small to render");
    else {
        render_info();

        int i, j;
        for (i = 0; i < POLES; i++)
            for (j = 0; j < disks; j++)
                render_disk(i, j, poles[i][j], POLE_CHAR);

        render_disk(POS, disks + 1, DISK, CURSOR_CHAR);
    }

    refresh();
}

void end_ncurses(void) {
    endwin();
}

// RUNNING

void usage(char *prog_name) {
    printf("USAGE: %s [-h] [-nc] [-d <NUM>]\n"
           "CONTROLS:\n"
           "\tLeft & right arrow keys to move the cursor.\n"
           "\tUp & down arrow keys to raise/lower disks.\n"
           "\tR to reset.\n"
           "\tQ to exit.\n", prog_name);
}


int main(int argc, char *argv[]) {
    char *prog_name = *argv++;
    argc--;

    int nd;
    char *end;
    for (; argc > 0; argc--, argv++)
        if (!strcmp(*argv, "-h")) {
            usage(prog_name);
            return 0;
        } else if (!strcmp(*argv, "-nc"))
            using_colors = -1;
        else if (!strcmp(*argv, "-d"))
            if (argc-- > 1) {
                nd = strtol(*(++argv), &end, 10);
                if (end == *argv) {
                    fprintf(stderr, "invalid number provided!\n");
                    exit(1);
                } else if (nd < 1) {
                    fprintf(stderr, "disk number cannot be below 1!\n");
                    exit(1);
                } else {
                    disks = nd;
                    power = (int) pow(10, strlen(*argv)) + 0.5;
                    printf("%d\n", power);
                }
            } else {
                fprintf(stderr, "no disk number provided!\n");
                exit(1);
            }
        else {
            fprintf(stderr, "invalid argument '%s'!\n", *argv);
            exit(1);
        }

    allocate_poles();
    init_data();

    init_ncurses();

    int c = '\0';
    do {
        handle_key(c);
        
        if (solved())
            break;

        render();
    } while ((c = getch()) != 'q');

    end_ncurses();


    if (solved())
        printf("%s! Completed in %d moves!\n",
               moves == MIN_MOVES ? "Perfect" : "Well done", moves);
    
    free_poles();

    return 0;
}

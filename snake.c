/* -------------------------------------------------------------------------- */
/* Includes                                                                   */
/* -------------------------------------------------------------------------- */
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

/* -------------------------------------------------------------------------- */
/* Primitives                                                                 */
/* -------------------------------------------------------------------------- */
typedef char            i8;
typedef short           i16;
typedef int             i32;
typedef long            i64;
typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long   u64;


/* -------------------------------------------------------------------------- */
/* Defines                                                                    */
/* -------------------------------------------------------------------------- */
#define TRUE    1
#define FALSE   (!TRUE)

#define HEAD    (NULL)
#define TAIL    (NULL)

#define INIT_BODY_COUNT (3 + 1)

#define RENDER_TIMEOUT  0.15

#define HEAD_SYM    "\u2592"
#define BODY_SYM    "\u2588"

#define SNACK_SYM   "\x1b[32m\u2588\x1b[0m"


/* -------------------------------------------------------------------------- */
/* Macros                                                                     */
/* -------------------------------------------------------------------------- */
#define getTickCount()  clock()

#define resizeCheck()   if (board.width != winTerminal.ws_col ||    \
                            board.height != winTerminal.ws_row)     \
                        {                                           \
                            gameState = GAME_OVER;                  \
                            break;                                  \
                        }

#define boundaryCheck() if (snake.head->pos.x > board.width ||      \
                            snake.head->pos.y > board.height ||     \
                            snake.head->pos.x < 1 ||                \
                            snake.head->pos.y < 1)                  \
                        {                                           \
                            gameState = GAME_OVER;                  \
                            break;                                  \
                        }

#define collisionCheck()    snake.body = snake.tail;                            \
                            while (snake.body != snake.head)                    \
                            {                                                   \
                                if (snake.body->pos.x == snake.head->pos.x &&   \
                                    snake.body->pos.y == snake.head->pos.y)     \
                                {                                               \
                                    gameState = GAME_OVER;                      \
                                }                                               \
                                if (snake.body->prev != HEAD)                   \
                                    snake.body = snake.body->prev;              \
                            }

#define snackCheck()    if (board.snackPos.x == snake.head->pos.x &&    \
                            board.snackPos.y == snake.head->pos.y)      \
                        {                                               \
                            createSnack(&board, &snake);                \
                            addBody(&board, &snake);                    \
                        }

#define nextBody()  if (snake->body->next != TAIL)\
                        snake->body = snake->body->next;

#define prevBody()  if (snake->body->prev != HEAD)\
                        snake->body = snake->body->prev;

#define gotoHead()  snake->body = snake->head;

#define gotoTail()  snake->body = snake->tail;

                            
/* -------------------------------------------------------------------------- */
/* Enumerations and Unions                                                    */
/* -------------------------------------------------------------------------- */
/* ------------------------------- Game states ------------------------------ */
typedef enum GameStates_e {
    IDLE = 0,
    LANDING,
    GAME_OVER,
    EXIT,
} GameStates_t;

/* ----------------------------- Snake direction ---------------------------- */
typedef enum Direction_e {
    NONE = 0,
    UP = 0x41,
    DOWN,
    RIGHT,
    LEFT,
} Direction_t;


/* -------------------------------------------------------------------------- */
/* Structures                                                                 */
/* -------------------------------------------------------------------------- */
/* -------------------------- Positional structure -------------------------- */
typedef struct Point_s {
    u32 x;
    u32 y;
} Point_t;

/* ----------------------------- Board structure ---------------------------- */
typedef struct Board_s {
    u32 width;
    u32 height;

    u32 availSpace;

    Point_t snackPos;
} Board_t;

/* ---------------------------- Snake structures ---------------------------- */
typedef struct SnakeBody_s {
    struct SnakeBody_s * prev;
    struct SnakeBody_s * next;

    Point_t pos;
    Point_t oldPos;
} SnakeBody_t;

typedef struct Snake_s {
    SnakeBody_t * body;

    SnakeBody_t * head;
    SnakeBody_t * tail;

    u32 bodyLength;

    Direction_t direction;
} Snake_t;


/* -------------------------------------------------------------------------- */
/* Prototypes                                                                 */
/* -------------------------------------------------------------------------- */
/* -------------------------- Game logic functions -------------------------- */
void detectKeypress(GameStates_t * gameState, Snake_t * snake);

/* ----------------------------- Board functions ---------------------------- */
void initBoard(Board_t * board);
void deinitBoard(Board_t * board);
void createSnack(Board_t * board, Snake_t * snake);

/* ----------------------------- Snake functions ---------------------------- */
void initSnake(Board_t * board, Snake_t * snake);
void deinitSnake(Snake_t * snake);
void addBody(Board_t * board, Snake_t * snake);
void moveSnake(Snake_t * snake);

/* --------------------------- Rendering functions -------------------------- */
void initRender();
void deinitRender();
void drawScreen(Board_t * board, Snake_t * snake, u8 landing);


/* -------------------------------------------------------------------------- */
/* Globals                                                                    */
/* -------------------------------------------------------------------------- */
struct winsize winTerminal;
struct termios terminal, oldTerminal;

i8 * landingString = "Press \u2190\u2191\u2192\u2193 to start, or Enter to exit";

u8 exitPressed;

/* -------------------------------------------------------------------------- */
/* Function Definitions                                                       */
/* -------------------------------------------------------------------------- */
/* ---------------------------------- Main ---------------------------------- */
i32 main(i32 argc, i8 ** argv)
{
    srand(time(NULL));

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &winTerminal);

    Board_t board;
    initBoard(&board);

    Snake_t snake;
    initSnake(&board, &snake);

    initRender();

    GameStates_t gameState;
    gameState = LANDING;

    clock_t start;

    u8 once = TRUE;

    while (TRUE)
    {
        switch (gameState)
        {
            case IDLE:
            {
                resizeCheck();

                detectKeypress(&gameState, &snake);

                if ((((double) (getTickCount() - start)) / CLOCKS_PER_SEC) >= (double) RENDER_TIMEOUT)
                {
                    moveSnake(&snake);

                    resizeCheck();
                    boundaryCheck();
                    collisionCheck();
                    snackCheck();
 
                    drawScreen(&board, &snake, FALSE);

                    start = getTickCount();
                }

                break;
            }

            case LANDING:
            {
                if (once)
                {
                    drawScreen(&board, &snake, TRUE);
                    once = FALSE;
                }
                
                resizeCheck();
                
                detectKeypress(&gameState, &snake);

                if (gameState == IDLE)
                {
                    createSnack(&board, &snake);
                    drawScreen(&board, &snake, FALSE);

                    start = getTickCount();
                }
                break;
            }

            case GAME_OVER:
            {
                deinitBoard(&board);

                deinitSnake(&snake);

                if (exitPressed)
                {
                    gameState = EXIT;
                }
                else
                {
                    gameState = LANDING;
                    once = TRUE;
                    initBoard(&board);

                    initSnake(&board, &snake);
                }

                break;
            }

            case EXIT:
            {
                deinitRender();
                return 0;
            }

            default:
            {
                break;
            }
        }
    }

    return 0;
}

/* -------------------------- Game logic functions -------------------------- */
void detectKeypress(GameStates_t * gameState, Snake_t * snake)
{
    i8 key = getchar();
    if (key == '\e')
    {
        getchar();
        switch (getchar())
        {
            case UP:
            {
                *gameState = IDLE;

                if (snake->direction != DOWN)
                    snake->direction = UP;

                break;
            }

            case DOWN:
            {
                *gameState = IDLE;
                
                if (snake->direction != UP)
                    snake->direction = DOWN;

                break;
            }

            case RIGHT:
            {
                *gameState = IDLE;

                if (snake->direction != LEFT)
                    snake->direction = RIGHT;

                break;
            }

            case LEFT:
            {
                *gameState = IDLE;

                if (snake->direction != RIGHT)
                    snake->direction = LEFT;

                break;
            }

            default:
            {
                break;
            }
        }
    }
    else if (key == '\n')
    {
        *gameState = GAME_OVER;
        snake->direction = NONE;

        exitPressed = TRUE;
    }
}

/* ----------------------------- Board functions ---------------------------- */
void initBoard(Board_t * board)
{
    board->width    = winTerminal.ws_col;
    board->height   = winTerminal.ws_row;

    board->availSpace = board->width * board->height;

    board->snackPos.x = 0;
    board->snackPos.y = 0;
}

void deinitBoard(Board_t * board)
{
    board->width    = 0;
    board->height   = 0;

    board->availSpace = 0;

    board->snackPos.x = 0;
    board->snackPos.y = 0;
}

void createSnack(Board_t * board, Snake_t * snake)
{
    if (board->availSpace <= 1)
        return;
    
    board->snackPos.x = (rand() % board->width) + 1;
    board->snackPos.y = (rand() % board->height) + 1;

    gotoHead();

    while (snake->body->pos.x == board->snackPos.x ||
           snake->body->pos.y == board->snackPos.y)
    {
        board->snackPos.x++;
        if (board->snackPos.x > board->width)
        {
            board->snackPos.x = 1;

            board->snackPos.y++;
            if (board->snackPos.y > board->height)
                board->snackPos.y = 1;
        }

        if (snake->body == snake->tail)
            gotoHead();

        nextBody();
    }
}

/* ----------------------------- Snake functions ---------------------------- */
void initSnake(Board_t * board, Snake_t * snake)
{
    snake->direction = NONE;

    snake->bodyLength = 0;

    snake->body = NULL;
    snake->head = HEAD;
    snake->tail = TAIL;

    for (u8 i = 0; i < INIT_BODY_COUNT; i++)
        addBody(board, snake);
}

void deinitSnake(Snake_t * snake)
{
    gotoTail();

    snake->tail = TAIL;

    snake->bodyLength = 0;

    snake->direction = NONE;

    while (snake->body != snake->head)
    {
        snake->body->pos.x = 0;
        snake->body->pos.y = 0;
        snake->body->oldPos.x = 0;
        snake->body->oldPos.y = 0;

        prevBody();

        free(snake->body->next);
        snake->body->next = TAIL;
    }

    snake->body->pos.x = 0;
    snake->body->pos.y = 0;
    snake->body->oldPos.x = 0;
    snake->body->oldPos.y = 0;

    free(snake->body);
    snake->body = NULL;

    snake->head = HEAD;

}

void addBody(Board_t * board, Snake_t * snake)
{
    if (snake->body == NULL)
    {
        snake->body = (SnakeBody_t *) malloc(sizeof(SnakeBody_t));
        snake->body->prev = HEAD;
        snake->body->next = TAIL;

        snake->body->pos.x = board->width / 2;
        snake->body->pos.y = board->height / 2;

        snake->body->oldPos = snake->body->pos;

        snake->head = snake->body;
        snake->tail = snake->body;
    }
    else
    {
        gotoTail();

        snake->body->next = (SnakeBody_t *) malloc(sizeof(SnakeBody_t));
        snake->body->next->prev = snake->body;
        snake->body->next->next = TAIL;
        snake->body = snake->body->next;

        snake->body->pos = snake->body->prev->oldPos;
        snake->body->oldPos = snake->body->pos;

        snake->tail = snake->body;
    }

    snake->bodyLength++;

    board->availSpace--;
}

void moveSnake(Snake_t * snake)
{
    gotoHead();
    snake->head->oldPos = snake->head->pos;
    switch (snake->direction)
    {
        case UP:
        {
            snake->head->pos.y--;
            break;
        }

        case DOWN:
        {
            snake->head->pos.y++;
            break;
        }

        case RIGHT:
        {
            snake->head->pos.x++;
            break;
        }

        case LEFT:
        {
            snake->head->pos.x--;
            break;
        }

        default:
        {
            break;
        }
    }

    nextBody();

    while (snake->body != snake->tail)
    {
        snake->body->oldPos = snake->body->pos;
        snake->body->pos = snake->body->prev->oldPos;

        nextBody();
    }

    snake->body->oldPos = snake->body->pos;
    snake->body->pos = snake->body->prev->oldPos;
}


/* ---------------------------- Render functions ---------------------------- */
void initRender()
{
    tcgetattr(0, &oldTerminal);
    terminal = oldTerminal;
    terminal.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &terminal);

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    printf("\e[?25l");
}

void deinitRender()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminal);
    printf("\x1b[0m");
    printf("\e[?25h");
    printf("\e[2J");
}

void drawScreen(Board_t * board, Snake_t * snake, u8 landing)
{
    printf("\e[2J");
    printf("\e[H");

    if (landing)
    {
        printf("\e[1;%dH%s", (u32)(snake->head->pos.x - (strlen(landingString) / 2)), landingString);
        printf("\e[%d;%dH"HEAD_SYM, snake->head->pos.y, snake->head->pos.x);
    }
    else
    {
        printf("\e[%d;%dH"SNACK_SYM, board->snackPos.y, board->snackPos.x);

        gotoHead();
        while (snake->body != snake->tail)
        {
            if (snake->body == snake->head)
            {
                printf("\e[%d;%dH"HEAD_SYM, snake->head->pos.y, snake->head->pos.x);
            }
            else
            {
                printf("\e[%d;%dH"BODY_SYM, snake->body->pos.y, snake->body->pos.x);
            }

            nextBody();
        }

        printf("\e[%d;%dH"BODY_SYM, snake->tail->pos.y, snake->tail->pos.x);
    }
}

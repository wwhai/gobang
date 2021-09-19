#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <math.h>
#define MAP_WIDTH 640                   // 棋盘长
#define MAP_HEIGHT 640                  // 棋盘宽
#define BLOCK_SIZE 35                   // 棋子大小
#define HOT_AREA_SHIFT (BLOCK_SIZE / 2) // 坐标系偏移量，用来辅助显示UI
//-----------------------------------------------------------------
#define T_SELF 0
#define T_AI 1
#define T_NONE 2
#define L0 0
#define L1 1
#define L2 2
#define L3 3
//-----------------------------------------------------------------
#define string(_) (#_)
#define LENA(_) ((sizeof _) / (sizeof _[0]))
typedef struct
{
    int x, y, w, h; // Area
    int r, g, b, a; // Color
    int type;       // Type
    int score;      // Score
} HoldArea;
// 落子区
HoldArea holdAreas[MAP_WIDTH / BLOCK_SIZE][MAP_HEIGHT / BLOCK_SIZE];
// 评估表,其实是4根线上的打分记录表
HoldArea scoreTable[4][10];
//

void drawText(SDL_Renderer *main_renderer,
              TTF_Font *font,
              int x, int y,
              char text[],
              int r, int g, int b)
{
    SDL_Color color = {r, g, b, 255};
    SDL_Rect text_area = {x, y, 10 * strlen(text), TTF_FontHeight(font) * 1.5};
    SDL_Surface *text_surface = TTF_RenderUTF8_Solid(font, text, color);
    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(main_renderer, text_surface);
    SDL_RenderCopy(main_renderer, text_texture, NULL, &text_area);
}
/**
 *
 * @param window_surface
 */

void drawMainView(SDL_Renderer *main_renderer)
{
    SDL_SetRenderDrawColor(main_renderer, 0, 100, 50, SDL_ALPHA_OPAQUE);
    for (int x = BLOCK_SIZE; x <= MAP_WIDTH; x += BLOCK_SIZE)
    {
        SDL_RenderDrawLine(main_renderer, x, BLOCK_SIZE, x, MAP_HEIGHT);
    }
    for (int y = BLOCK_SIZE; y <= MAP_HEIGHT; y += BLOCK_SIZE)
    {
        SDL_RenderDrawLine(main_renderer, BLOCK_SIZE, y, MAP_WIDTH, y);
    }
}

/**
 *
 * @param window_surface
 */
void loadHoldArea()
{
    for (int i = 0; i <= (MAP_WIDTH / BLOCK_SIZE); ++i)
    {
        for (int j = 0; j <= (MAP_HEIGHT / BLOCK_SIZE); ++j)
        {
            HoldArea holdArea = {i * BLOCK_SIZE + HOT_AREA_SHIFT,
                                 j * BLOCK_SIZE + HOT_AREA_SHIFT,
                                 BLOCK_SIZE,
                                 BLOCK_SIZE,
                                 50, 45, 23,
                                 SDL_ALPHA_OPAQUE};
            holdAreas[i][j] = holdArea;
        }
    }
}

/**
 *
 * @param main_renderer
 * @param cx
 * @param cy
 * @param radius
 * @param r
 * @param g
 * @param b
 * @param a
 */
void fillCircle(SDL_Renderer *main_renderer, int cx, int cy, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    for (int dy = 1; dy <= radius; dy += 1)
    {
        int dx = floor(sqrt((2.0 * radius * dy) - (dy * dy)));
        SDL_SetRenderDrawColor(main_renderer, r, g, b, a);
        SDL_RenderDrawLine(main_renderer, cx - dx, cy + dy - radius, cx + dx, cy + dy - radius);
        SDL_RenderDrawLine(main_renderer, cx - dx, cy - dy + radius, cx + dx, cy - dy + radius);
    }
}

/**
 *
 * @param rend
 * @param x
 * @param y
 * @param r
 * @param g
 * @param b
 * @param a
 */
void setPixel(SDL_Renderer *rend, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_SetRenderDrawColor(rend, r, g, b, a);
    SDL_RenderDrawPoint(rend, x, y);
}

/**
 *
 * @param surface
 * @param n_cx
 * @param n_cy
 * @param radius
 * @param r
 * @param g
 * @param b
 * @param a
 */

void drawCircle(SDL_Renderer *surface, int n_cx, int n_cy, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    double error = (double)-radius;
    double x = (double)radius - 0.5;
    double y = (double)0.5;
    double cx = n_cx - 0.5;
    double cy = n_cy - 0.5;

    while (x >= y)
    {
        setPixel(surface, (int)(cx + x), (int)(cy + y), r, g, b, a);
        setPixel(surface, (int)(cx + y), (int)(cy + x), r, g, b, a);

        if (x != 0)
        {
            setPixel(surface, (int)(cx - x), (int)(cy + y), r, g, b, a);
            setPixel(surface, (int)(cx + y), (int)(cy - x), r, g, b, a);
        }

        if (y != 0)
        {
            setPixel(surface, (int)(cx + x), (int)(cy - y), r, g, b, a);
            setPixel(surface, (int)(cx - y), (int)(cy + x), r, g, b, a);
        }

        if (x != 0 && y != 0)
        {
            setPixel(surface, (int)(cx - x), (int)(cy - y), r, g, b, a);
            setPixel(surface, (int)(cx - y), (int)(cy - x), r, g, b, a);
        }

        error += y;
        ++y;
        error += y;

        if (error >= 0)
        {
            --x;
            error -= x;
            error -= x;
        }
    }
}

/**
 *
 * @param main_renderer
 */
void drawHoldArea(SDL_Renderer *main_renderer)
{
    for (int i = 0; i < (MAP_WIDTH / BLOCK_SIZE); ++i)
    {
        for (int j = 0; j < (MAP_HEIGHT / BLOCK_SIZE); ++j)
        {
            HoldArea holdArea = holdAreas[i][j];
            SDL_SetRenderDrawColor(main_renderer, holdArea.r, holdArea.g, holdArea.b, SDL_ALPHA_OPAQUE);
            fillCircle(main_renderer,
                       holdArea.x + HOT_AREA_SHIFT,        // 坐标平移 HOT_AREA_SHIFT
                       holdArea.y + HOT_AREA_SHIFT,        // 坐标平移 HOT_AREA_SHIFT
                       HOT_AREA_SHIFT,                     // 半径
                       holdArea.r, holdArea.g, holdArea.b, // 棋子颜色
                       SDL_ALPHA_OPAQUE);
        }
    }
}

/**
 *
 * @param main_renderer
 */
void drawEValueLine(SDL_Renderer *main_renderer, int x, int y)
{

    SDL_SetRenderDrawColor(main_renderer, 0, 255, 255, SDL_ALPHA_OPAQUE);
    // -X轴上的这根线
    SDL_RenderDrawLine(main_renderer,
                       x + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT,
                       x - BLOCK_SIZE * 5 + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT);
    // X轴上的这根线
    SDL_RenderDrawLine(main_renderer,
                       x + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT,
                       x + BLOCK_SIZE * 5 + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT);
    // -Y轴上的这根线
    SDL_RenderDrawLine(main_renderer,
                       x + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT,
                       x + HOT_AREA_SHIFT,
                       y - BLOCK_SIZE * 5 + HOT_AREA_SHIFT);
    // Y轴上的这根线
    SDL_RenderDrawLine(main_renderer,
                       x + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT,
                       x + HOT_AREA_SHIFT,
                       y + BLOCK_SIZE * 5 + HOT_AREA_SHIFT);

    // 第1象限对角线
    SDL_RenderDrawLine(main_renderer,
                       x + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT,
                       x + BLOCK_SIZE * 5 + HOT_AREA_SHIFT,
                       y - BLOCK_SIZE * 5 + HOT_AREA_SHIFT);
    // 第2象限对角线
    SDL_RenderDrawLine(main_renderer,
                       x + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT,
                       x - BLOCK_SIZE * 5 + HOT_AREA_SHIFT,
                       y - BLOCK_SIZE * 5 + HOT_AREA_SHIFT);
    // 第3象限对角线
    SDL_RenderDrawLine(main_renderer,
                       x + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT,
                       x - BLOCK_SIZE * 5 + HOT_AREA_SHIFT,
                       y + BLOCK_SIZE * 5 + HOT_AREA_SHIFT);

    // 第4象限对角线
    SDL_RenderDrawLine(main_renderer,
                       x + HOT_AREA_SHIFT,
                       y + HOT_AREA_SHIFT,
                       x + BLOCK_SIZE * 5 + HOT_AREA_SHIFT,
                       y + BLOCK_SIZE * 5 + HOT_AREA_SHIFT);
}

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    SDL_Window *main_window = NULL;
    SDL_Renderer *main_renderer = NULL;
    TTF_Font *font = NULL;
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL_Init failed!\n");
        return -1;
    }
    if (TTF_Init() < 0)
    {
        printf("TTF_Init failed!\n");
        return -1;
    }
    font = TTF_OpenFont("font.ttf", 10);
    if (font == NULL)
    {
        printf("TTF_OpenFont failed!\n");
        return -1;
    }
    main_window = SDL_CreateWindow("GOBANG",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   MAP_WIDTH + BLOCK_SIZE,
                                   MAP_HEIGHT + BLOCK_SIZE,
                                   SDL_WINDOW_SHOWN);
    if (main_window == NULL)
    {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return -1;
    }
    //Create renderer for window
    main_renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_ACCELERATED);
    if (main_renderer == NULL)
    {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetRenderDrawBlendMode(main_renderer, SDL_BLENDMODE_BLEND);
    loadHoldArea();
    SDL_bool keep_window_open = SDL_TRUE;
    //----------------------------------------------------------------------------
    SDL_bool mouse_left = SDL_FALSE; // 是否是鼠标按下
    int currentBlockX;               // 当前选中的X坐标
    int currentBlockY;               // 当前选中的Y坐标
    HoldArea *currentHoldArea;       // 当前落子区块
    while (keep_window_open)
    {

        int mouseX, mouseY;
        //----------------------------------------------------------------------------
        SDL_Event e;
        SDL_PumpEvents();
        SDL_PollEvent(&e);
        int row, col;
        char window_title[20];
        switch (e.type)
        {
        case SDL_QUIT:
            keep_window_open = SDL_FALSE;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e.button.button == SDL_BUTTON_LEFT)
            {
                mouse_left = SDL_TRUE;
                row = (e.motion.y - HOT_AREA_SHIFT) / BLOCK_SIZE;
                col = (e.motion.x - HOT_AREA_SHIFT) / BLOCK_SIZE;
                sprintf(window_title, "POS: R=%d,C=%d", row, col);
                SDL_SetWindowTitle(main_window, window_title);
                printf("POS: R=%d, C=%d\n", row, col);
                currentHoldArea = &holdAreas[col][row];
                currentHoldArea->r = 255;
                currentHoldArea->g = 0;
                currentHoldArea->b = 0;
                currentHoldArea->type = T_SELF;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT)
                break;
        case SDL_MOUSEMOTION:
            row = (e.motion.y - HOT_AREA_SHIFT) / BLOCK_SIZE;
            col = (e.motion.x - HOT_AREA_SHIFT) / BLOCK_SIZE;
            currentHoldArea = &holdAreas[col][row];
            currentBlockX = currentHoldArea->x;
            currentBlockY = currentHoldArea->y;
            mouseX = currentHoldArea->x;
            mouseY = currentHoldArea->y;
            HoldArea *l0 = &scoreTable[L0][0];
            for (size_t i = 0; i < 5; i++)
            {
                l0[5 - i] = holdAreas[col + i][row];
            }
            for (size_t i = 0; i < 5; i++)
            {
                l0[5 + i] = holdAreas[col - i][row];
            }
        default:
            break;
        }
        if ((SDL_GetTicks() - SDL_GetTicks()) < 10)
        {
            SDL_Delay(10);
        }
        SDL_SetRenderDrawColor(main_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(main_renderer);

        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        drawHoldArea(main_renderer);
        drawMainView(main_renderer);
        drawEValueLine(main_renderer, mouseX, mouseY);
        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        for (size_t i = 0; i < 10; i++)
        {
            HoldArea h = scoreTable[L0][i];
            char text[8];
            sprintf(text, "%d", h.score);
            drawText(main_renderer, font, h.x, h.y, text, 255, 0, 0);
        }
        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        SDL_RenderPresent(main_renderer);
    }

    SDL_DestroyRenderer(main_renderer);
    SDL_DestroyWindow(main_window);
    SDL_Quit();
    return 0;
}
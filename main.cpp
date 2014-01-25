#include <cstdio>
#include <cstdlib>     /* srand, rand */
#include <ctime>
#include <cassert>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_physfs.h>

#include <physfs.h>

#include "level1.h"

//#define NO_SCALING

// from: http://stackoverflow.com/questions/3437404/min-and-max-in-c
// Note: __typeof__ operator may be GCC specific
#ifndef max
 #define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#endif

#ifndef min
 #define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#endif

static const char *ORGANIZATION_NAME = "jdooley.org";
static const char *APPLICATION_NAME = "SAM4";

static const signed int TILE_WIDTH_PIXELS_UNSCALED  = 32;
static const signed int TILE_HEIGHT_PIXELS_UNSCALED = 32;

static const signed int VIEWPORT_WIDTH_TILES  = 20;
static const signed int VIEWPORT_HEIGHT_TILES = 15;

static const signed int VIEWPORT_WIDTH_PIXELS_UNSCALED  = VIEWPORT_WIDTH_TILES  * TILE_WIDTH_PIXELS_UNSCALED;
static const signed int VIEWPORT_HEIGHT_PIXELS_UNSCALED = VIEWPORT_HEIGHT_TILES * TILE_HEIGHT_PIXELS_UNSCALED;

static const signed int LEVEL_WIDTH_VIEWPORTS  = 2; // two screens wide
static const signed int LEVEL_HEIGHT_VIEWPORTS = 2; // two screens high

static const signed int LEVEL_WIDTH_TILES  = LEVEL_WIDTH_VIEWPORTS  * VIEWPORT_WIDTH_TILES;
static const signed int LEVEL_HEIGHT_TILES = LEVEL_HEIGHT_VIEWPORTS * VIEWPORT_HEIGHT_TILES;

static const signed int PLAYER_MAX_JUMP_HEIGHT_UNSCALED = (TILE_HEIGHT_PIXELS_UNSCALED * 2) + (TILE_HEIGHT_PIXELS_UNSCALED / 2);

#ifdef NO_SCALING
static const signed int SCALE_FACTOR = 1;
#else
static const signed int SCALE_FACTOR = 2;
#endif

// scaled
static const signed int LEVEL_WIDTH_PIXELS_UNSCALED  = LEVEL_WIDTH_TILES  * TILE_WIDTH_PIXELS_UNSCALED;
static const signed int LEVEL_HEIGHT_PIXELS_UNSCALED = LEVEL_HEIGHT_TILES * TILE_HEIGHT_PIXELS_UNSCALED;

static const signed int SCREEN_WIDTH_PIXELS_SCALED  = VIEWPORT_WIDTH_PIXELS_UNSCALED  * SCALE_FACTOR;
static const signed int SCREEN_HEIGHT_PIXELS_SCALED = VIEWPORT_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR;

// how many seconds is each frame of the player's animation displayed for
static const double ANIMATION_RATE = 0.25;

/* bit flags for whether a block is 'solid' on a particular surface (e.g. cannot be entered from that side).
    MUST MATCH TileStudio definitions */
#define SOLID_TOP     (1 << 0)
#define SOLID_LEFT    (1 << 1)
#define SOLID_BOTTOM  (1 << 2)
#define SOLID_RIGHT   (1 << 3)

typedef enum
{
    eACTION_MOVE_LEFT   = 0,
    eACTION_MOVE_RIGHT  = 1,
    eACTION_JUMP        = 2,
    eACTION_FIRE        = 3,
} action_t;

typedef enum _TFacing
{
    eFACING_LEFT = 0,
    eFACING_RIGHT = 1
} TFacing;

typedef enum _TMapCode
{
    eCODE_PLAYER_SPAWN = 1,
    eCODE_DEATH        = 2
} TMapCode;

class TObject
{
public:
    TObject(unsigned int tileID) : m_tileID(tileID) {};

    virtual void Tick(double delta_seconds) {};

    virtual unsigned int TileID() const { return m_tileID; };
    virtual signed int DrawWidth() const { return TILE_WIDTH_PIXELS_UNSCALED; };

    // unscaled
    signed int x, y;

protected:
    unsigned int m_tileID;
};

class TMobile : public TObject
{
public:
    TMobile(unsigned int tileID) : TObject(tileID), xVelocity(0), yVelocity(0), jumping(false), jumpedSoFar(0), facing(eFACING_RIGHT) {};
    signed int xVelocity, yVelocity;

    // if jumping, how many unscaled pixels up has the player moved so far
    // (stop jumping when this reaches the limit of how far to jump)
    bool jumping;
    signed int jumpedSoFar;

    TFacing facing;
};

class TPlayer : public TMobile
{
public:
    TPlayer() : TMobile(366), frameIndex(0), seconds_since_last_frame_change(0.0) {};
    virtual void Tick(double delta_seconds);

    virtual unsigned int TileID() const;
    virtual signed int DrawWidth() const;

private:
    unsigned int frameIndex;
    double seconds_since_last_frame_change;

    enum
    {
        eANIM_STANDING_LEFT = 0,
        eANIM_STANDING_RIGHT,
        eANIM_JUMPING_LEFT,
        eANIM_JUMPING_RIGHT,
        eANIM_SHOOTING_LEFT,
        eANIM_SHOOTING_RIGHT,

        // must be last
        eNUM_PLAYER_ANIMATIONS,

        eFRAMES_PER_ANIMATION = 4
    };

    static const unsigned int frames[eNUM_PLAYER_ANIMATIONS][eFRAMES_PER_ANIMATION];
    static const signed int widths[eNUM_PLAYER_ANIMATIONS][eFRAMES_PER_ANIMATION];
};


const unsigned int TPlayer::frames[eNUM_PLAYER_ANIMATIONS][eFRAMES_PER_ANIMATION] =
{
    { 389, 390, 391, 392 },
    { 366, 367, 368, 369 },
    { 436, 436, 436, 436 },
    { 435, 435, 435, 435 },
    { 413, 415, 415, 413 },
    { 412, 414, 414, 412 }
};

const signed int TPlayer::widths[eNUM_PLAYER_ANIMATIONS][eFRAMES_PER_ANIMATION] =
{
    { 20, 24, 20, 22 },
    { 20, 24, 20, 22 },
    { 28, 28, 28, 28 },
    { 28, 28, 28, 28 },
    { 20, 32, 32, 20 },
    { 20, 32, 32, 20 }
};


void TPlayer::Tick(double delta_seconds) 
{
    seconds_since_last_frame_change += delta_seconds;

    if (seconds_since_last_frame_change >= ANIMATION_RATE)
    {
        frameIndex = (frameIndex + 1) % eFRAMES_PER_ANIMATION;
        seconds_since_last_frame_change = 0.0;
    }
}

unsigned int TPlayer::TileID() const
{
    if (facing == eFACING_RIGHT)
    {
        if (jumping)
            return frames[eANIM_JUMPING_RIGHT][frameIndex];
        else
            return frames[eANIM_STANDING_RIGHT][frameIndex];
    }
    else if (facing == eFACING_LEFT)
    {
        if (jumping)
            return frames[eANIM_JUMPING_LEFT][frameIndex];
        else
            return frames[eANIM_STANDING_LEFT][frameIndex];
    }
    else
        assert(false); // unknown state
}

signed int TPlayer::DrawWidth() const
{
    if (facing == eFACING_RIGHT)
    {
        if (jumping)
            return widths[eANIM_JUMPING_RIGHT][frameIndex];
        else
            return widths[eANIM_STANDING_RIGHT][frameIndex];
    }
    else if (facing == eFACING_LEFT)
    {
        if (jumping)
            return widths[eANIM_JUMPING_LEFT][frameIndex];
        else
            return widths[eANIM_STANDING_LEFT][frameIndex];
    }
    else
        assert(false); // unknown state
}

namespace GLOBALS
{
    ALLEGRO_EVENT_QUEUE *events;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *tileAtlas_unscaled;
    ALLEGRO_FONT *defaultFont;

    ALLEGRO_BITMAP *background_scaled;

    TPlayer player;
}

/* create a wrapper to throw away the int return value of PHYSFS_deinit() */
static void atexitwrapper_PhysFS_deinit(void) { PHYSFS_deinit(); }

static bool InitGame(int argc, char **argv);
static void DoTitleScreen(void);
static void DoMainMenu(void);
static void PlayGame(void);
static void ShutdownGame(void);
static void ResetLevel(void);

static void RedrawScreen(void);
static void ProcessAction(action_t action);
static void CreateBackgroundImage(void);

static bool OnSolidGround(void);
static bool CanMoveUp(void);
static bool InDeathSquare(void);

int main(int argc, char **argv)
{
    srand(time(NULL));
    
    if (!InitGame(argc, argv))
    {
        fprintf(stderr, "\nInitialization failed.\n");
        return -1;        
    }

    DoTitleScreen();
    DoMainMenu();

    PlayGame();

    ShutdownGame();
    
    return 0;
}


bool InitGame(int argc, char **argv)
{
    if (!al_init())
    {
        fprintf(stderr, "\nERROR: Failed to initialize Allegro\n");
        return false;
    }
    al_set_org_name(ORGANIZATION_NAME);
    al_set_app_name(APPLICATION_NAME);

    if (!PHYSFS_init(argv[0]))
    {
        fprintf(stderr, "\nERROR: Failed to initialize PhysicsFS\n");
        return false;
    }

    atexit(atexitwrapper_PhysFS_deinit);
    
    if (!PHYSFS_setSaneConfig(ORGANIZATION_NAME, APPLICATION_NAME, "ZIP", 0, 1))
    {
        fprintf(stderr, "\nERROR: Unable to configure PhysFS. Specifically:\n\t'%s'",
                        PHYSFS_getLastError());
        return false;
    }

    al_set_physfs_file_interface();
    
    if (!al_install_keyboard())
        return false;
    
    if (!al_install_audio())
        return false;

    if (!al_init_acodec_addon())
        return false;

    al_reserve_samples(3);
    
    al_init_font_addon();
    al_init_ttf_addon();

    if (!al_init_primitives_addon())
        return false;

    if (!al_init_image_addon())
        return false;

//#ifndef NO_SCALING
    al_set_new_display_flags(ALLEGRO_FULLSCREEN | ALLEGRO_OPENGL | ALLEGRO_OPENGL_3_0);
//#endif

    al_set_new_display_option(ALLEGRO_COMPATIBLE_DISPLAY, 1, ALLEGRO_REQUIRE);
    al_set_new_display_option(ALLEGRO_CAN_DRAW_INTO_BITMAP, 1, ALLEGRO_REQUIRE);
    al_set_new_display_option(ALLEGRO_RENDER_METHOD, 1, ALLEGRO_REQUIRE);
    
#ifdef NO_SCALING
    GLOBALS::display = al_create_display(VIEWPORT_WIDTH_PIXELS, VIEWPORT_HEIGHT_PIXELS);
#else
    GLOBALS::display = al_create_display(1920, 1080);
#endif

    if (GLOBALS::display == NULL)
        return false;
    al_clear_to_color(al_map_rgb(0,0,0));

    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
    al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);

    GLOBALS::background_scaled = al_create_bitmap(LEVEL_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, LEVEL_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR);
    if (GLOBALS::background_scaled == NULL)
        return false;

    GLOBALS::events = al_create_event_queue();
    al_register_event_source(GLOBALS::events, al_get_keyboard_event_source());

    GLOBALS::defaultFont = al_create_builtin_font();

    // I happen to know that the original Sam tiles are 16x16, so need to do a scaling to get them up to the 32x32 "unscaled" expected size.
    // If they get replaced in the future with natively 32x32 tiles, this initial prescaling would be removed.
    ALLEGRO_BITMAP *tileAtlas_temp = al_load_bitmap("tiles.png");
    if (tileAtlas_temp == NULL)
        return false;
    
    GLOBALS::tileAtlas_unscaled = al_create_bitmap(al_get_bitmap_width(tileAtlas_temp) * 2, al_get_bitmap_height(tileAtlas_temp) * 2);
    if (GLOBALS::tileAtlas_unscaled == NULL)
        return false;

    al_set_target_bitmap(GLOBALS::tileAtlas_unscaled);
    al_draw_scaled_bitmap(tileAtlas_temp,
        0, 0,
        al_get_bitmap_width(tileAtlas_temp), al_get_bitmap_height(tileAtlas_temp),
        0, 0,
        al_get_bitmap_width(GLOBALS::tileAtlas_unscaled), al_get_bitmap_height(GLOBALS::tileAtlas_unscaled),
        0);

    // done with the original 16x16 tile atlas
    al_destroy_bitmap(tileAtlas_temp);

    return true;
}

void PlayGame(void)
{
    bool done = false;
    int i;
    ALLEGRO_EVENT event;
    bool wants_left = false, wants_right = false, wants_jump = false, wants_fire = false;
    double time_of_last_frame = al_get_time();
    double delta_time;

    CreateBackgroundImage( /* level number? */ );

    ResetLevel();

    while (!done)
    {
        // NOTE: locked to vsync rate by RedrawScreen() which calls al_flip_display()

        if (al_get_next_event(GLOBALS::events, &event))
        {
            switch (event.type)
            {
                /* clicked the window-close button ('X' window dressing) */
                case ALLEGRO_EVENT_DISPLAY_CLOSE:
                    done = true;
                    break;

                /* translate key-press input into internal events, with manual handling of repeats */
                case ALLEGRO_EVENT_KEY_DOWN:
                    switch (event.keyboard.keycode)
                    {
                        case ALLEGRO_KEY_ESCAPE:
                            done = true;
                            break;

                        case ALLEGRO_KEY_LEFT:
                        case ALLEGRO_KEY_PAD_4:
                            wants_left = true;
                            break;

                        case ALLEGRO_KEY_RIGHT:
                        case ALLEGRO_KEY_PAD_6:
                            wants_right = true;
                            break;

                        case ALLEGRO_KEY_X:
                            wants_fire = true;
                            break;

                        case ALLEGRO_KEY_Z:
                            wants_jump = true;
                            break;
                    }
                    break;

                case ALLEGRO_EVENT_KEY_UP:
                    switch (event.keyboard.keycode)
                    {
                        case ALLEGRO_KEY_LEFT:
                        case ALLEGRO_KEY_PAD_4:
                            wants_left = false;
                            break;

                        case ALLEGRO_KEY_RIGHT:
                        case ALLEGRO_KEY_PAD_6:
                            wants_right = false;
                            break;

                        case ALLEGRO_KEY_X:
                            wants_fire = false;
                            break;

                        case ALLEGRO_KEY_Z:
                            wants_jump = false;
                            break;
                    }
                    break;
                // TODO: make the keys configurable (remap input option)
            } /* switch(event type) */
        }

        // Call the ticks here so that animation frames (and therefore drawing widths) are updated prior to allowing movement,
        // which relying on the drawing widths for bounds-checking
        // TODO: update each enemy and any shots fired

        delta_time = al_get_time() - time_of_last_frame;
        time_of_last_frame = al_get_time();

        GLOBALS::player.Tick(delta_time);


        if (wants_left)
            ProcessAction(eACTION_MOVE_LEFT);
        if (wants_right)
            ProcessAction(eACTION_MOVE_RIGHT);
        if (wants_fire)
            ProcessAction(eACTION_FIRE);
        if (wants_jump)
            ProcessAction(eACTION_JUMP);

        if (InDeathSquare())
        {
            ResetLevel();
            continue;
        }


        // player is either jumping or falling
        if (GLOBALS::player.jumping)
        {
            for (i = 0; i < GLOBALS::player.yVelocity; ++i)
            {
                if (CanMoveUp())
                {
                    GLOBALS::player.y -= 1;
                    GLOBALS::player.jumpedSoFar += 1;

                    if (GLOBALS::player.jumpedSoFar >= PLAYER_MAX_JUMP_HEIGHT_UNSCALED)
                        GLOBALS::player.jumping = false;
                }
                else // solid blocks above
                    GLOBALS::player.jumping = false;
            }
        }
        else // try to apply gravity
        {
            for (i = 0; i < GLOBALS::player.yVelocity; ++i)
            {
                if (!OnSolidGround())
                {
                    GLOBALS::player.y += 1;
                }
            }
        }

        RedrawScreen();
    } /* while(!bDone) */
}

void DoTitleScreen(void)
{
    // TODO:
    //    load title screen bitmap
    //    load title screen music
    //    display bitmap
    //    play music
    //    wait for keypress
    //    stop music
    //    unload music
    //    unload bitmap
}

void DoMainMenu(void)
{
    // TODO:
    //    load title screen bitmap
    //    load title screen music
    //    display bitmap
    //    play music
    
    //    menu-related stuffs
    
    //    stop music
    //    unload music
    //    unload bitmap
}

void RedrawScreen(void)
{
    // unscaled!
    signed int worldX, worldY;

    al_set_target_backbuffer(GLOBALS::display);


    // keep viewable region centered around the player if possible. This means the level is split into three
    // regions:

    //   - player is in middle of level (so enough left and right to center about player)
    worldX = (GLOBALS::player.x + (TILE_WIDTH_PIXELS_UNSCALED / 2)) - (VIEWPORT_WIDTH_PIXELS_UNSCALED / 2);
    worldY = (GLOBALS::player.y + (TILE_HEIGHT_PIXELS_UNSCALED / 2)) - (VIEWPORT_HEIGHT_PIXELS_UNSCALED / 2);

    //   - player is too far left to center level (not enough world to the left of the player)
    if (worldX < 0)
        worldX = 0;
    //   - player is too far right to center level (not enough world to the right of the player)
    else if (worldX > (LEVEL_WIDTH_PIXELS_UNSCALED - VIEWPORT_WIDTH_PIXELS_UNSCALED))
        worldX = LEVEL_WIDTH_PIXELS_UNSCALED - VIEWPORT_WIDTH_PIXELS_UNSCALED;


    if (worldY < 0)
        worldY = 0;
    else if (worldY > (LEVEL_HEIGHT_PIXELS_UNSCALED - VIEWPORT_HEIGHT_PIXELS_UNSCALED))
        worldY = LEVEL_HEIGHT_PIXELS_UNSCALED - VIEWPORT_HEIGHT_PIXELS_UNSCALED;


    //    copy appropriate region of background bitmap to screen
    al_draw_bitmap_region(GLOBALS::background_scaled, worldX * SCALE_FACTOR, worldY * SCALE_FACTOR, /* source x, y */
                                                      SCREEN_WIDTH_PIXELS_SCALED, SCREEN_HEIGHT_PIXELS_SCALED, /* width, height */
                                                      0, 0, /* dest x, y */
                                                      0);

    // TODO:
    //    draw all enemies and the player and shots
    const unsigned int playerTileID = GLOBALS::player.TileID();
    const unsigned int atlasWidth_tiles = al_get_bitmap_width(GLOBALS::tileAtlas_unscaled) / TILE_WIDTH_PIXELS_UNSCALED;
    al_draw_scaled_bitmap(GLOBALS::tileAtlas_unscaled,
                                      (playerTileID % atlasWidth_tiles) * TILE_WIDTH_PIXELS_UNSCALED, (playerTileID / atlasWidth_tiles) * TILE_HEIGHT_PIXELS_UNSCALED,
                                      TILE_WIDTH_PIXELS_UNSCALED, TILE_HEIGHT_PIXELS_UNSCALED,
                                      (GLOBALS::player.x - worldX) * SCALE_FACTOR, (GLOBALS::player.y - worldY) * SCALE_FACTOR,
                                      TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR,
                                      0);
    //    copy appropriate region of foreground bitmap to screen (eventually, if there is one)

    // display some debugging information
#if 1
    al_draw_textf(GLOBALS::defaultFont, al_map_rgb(255,255,255), TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED, 0,
                  "onGround(%d) canMoveUp(%d), jumping(%d)",
                  OnSolidGround(), CanMoveUp(), GLOBALS::player.jumping);
#endif

    al_flip_display();
}

void ProcessAction(action_t action)
{
    int tileX, tileYtop, tileYbottom, i;

    tileYtop = GLOBALS::player.y / TILE_HEIGHT_PIXELS_UNSCALED;
    tileYbottom = (GLOBALS::player.y + TILE_HEIGHT_PIXELS_UNSCALED - 1) / TILE_HEIGHT_PIXELS_UNSCALED;

    // IMPORTANT: ALL MOVEMENTS ARE PERFORMED IN THE UNSCALED PIXEL WORLD
    switch (action)
    {
        case eACTION_MOVE_LEFT:
            GLOBALS::player.facing = eFACING_LEFT;


            for (i = 0; i < GLOBALS::player.xVelocity; ++i)
            {
                tileX = max(0, GLOBALS::player.x - 1) / TILE_WIDTH_PIXELS_UNSCALED;

                if (!(level1MapData.bounds[tileYtop    * LEVEL_WIDTH_TILES + tileX] & SOLID_RIGHT) &&
                    !(level1MapData.bounds[tileYbottom * LEVEL_WIDTH_TILES + tileX] & SOLID_RIGHT))
                    GLOBALS::player.x -= 1;
                else
                    break;
            }
            break;

        case eACTION_MOVE_RIGHT:
            GLOBALS::player.facing = eFACING_RIGHT;

            for (i = 0; i < GLOBALS::player.xVelocity; ++i)
            {
                tileX = min(LEVEL_WIDTH_PIXELS_UNSCALED - 1, GLOBALS::player.x + GLOBALS::player.DrawWidth()) / TILE_WIDTH_PIXELS_UNSCALED;

                if (!(level1MapData.bounds[tileYtop    * LEVEL_WIDTH_TILES + tileX] & SOLID_LEFT) &&
                    !(level1MapData.bounds[tileYbottom * LEVEL_WIDTH_TILES + tileX] & SOLID_LEFT))
                    GLOBALS::player.x += 1;
                else
                    break;
            }
            break;

        case eACTION_JUMP:
            if (!GLOBALS::player.jumping && OnSolidGround())
            {
                GLOBALS::player.jumping = true;
                GLOBALS::player.jumpedSoFar = 0;
            }

            break;

        case eACTION_FIRE:
            break;
    }
}

void ShutdownGame(void)
{
    al_stop_samples();

    if (GLOBALS::defaultFont)
        al_destroy_font(GLOBALS::defaultFont);

    if (GLOBALS::display)
        al_destroy_display(GLOBALS::display);

    if (GLOBALS::tileAtlas_unscaled)
        al_destroy_bitmap(GLOBALS::tileAtlas_unscaled);

    al_shutdown_ttf_addon();
    al_shutdown_font_addon();
    al_shutdown_image_addon();
    al_shutdown_primitives_addon();
    al_uninstall_audio();
    al_uninstall_keyboard();
}

void CreateBackgroundImage(void)
{
    const unsigned int atlasWidth_tiles = al_get_bitmap_width(GLOBALS::tileAtlas_unscaled) / TILE_WIDTH_PIXELS_UNSCALED;
    signed int tileID;

    assert(GLOBALS::background_scaled != NULL);


    al_set_target_bitmap(GLOBALS::background_scaled);

    // clear to a reasonable sky blue color so that the background layer of the map is not required to be completely filled in.
    al_clear_to_color(al_map_rgb(50,50,200));

    for (unsigned int y = 0; y < LEVEL_HEIGHT_TILES; ++y)
    {
        for (unsigned int x = 0; x < LEVEL_WIDTH_TILES; ++x)
        {
            tileID = level1MapData.backTiles[(y*LEVEL_WIDTH_TILES)+x];

            if (tileID != -1)
                al_draw_scaled_bitmap(GLOBALS::tileAtlas_unscaled,
                                      (tileID % atlasWidth_tiles) * TILE_WIDTH_PIXELS_UNSCALED, (tileID / atlasWidth_tiles) * TILE_HEIGHT_PIXELS_UNSCALED,
                                      TILE_WIDTH_PIXELS_UNSCALED, TILE_HEIGHT_PIXELS_UNSCALED,
                                      (TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR) * x, (TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR) * y,
                                      TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR,
                                      0);

            tileID = level1MapData.midTiles[(y*LEVEL_WIDTH_TILES)+x];

            if (tileID != -1)
                al_draw_scaled_bitmap(GLOBALS::tileAtlas_unscaled,
                                      (tileID % atlasWidth_tiles) * TILE_WIDTH_PIXELS_UNSCALED, (tileID / atlasWidth_tiles) * TILE_HEIGHT_PIXELS_UNSCALED,
                                      TILE_WIDTH_PIXELS_UNSCALED, TILE_HEIGHT_PIXELS_UNSCALED,
                                      (TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR) * x, (TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR) * y,
                                      TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR,
                                      0);
        }
    }
}

bool OnSolidGround(void)
{
    // can only possibly be on solid ground on a tile boundary
    if ((GLOBALS::player.y % TILE_HEIGHT_PIXELS_UNSCALED) != 0)
        return false;

    // X coord of the left-most column of the player
    int tileX = GLOBALS::player.x / TILE_WIDTH_PIXELS_UNSCALED;

    // X coord of the right-most column of the player
    int tileXright = (GLOBALS::player.x + GLOBALS::player.DrawWidth() - 1) / TILE_WIDTH_PIXELS_UNSCALED;

    // the Y coord of the row immediately below the player
    int tileY = (GLOBALS::player.y + TILE_HEIGHT_PIXELS_UNSCALED) / TILE_HEIGHT_PIXELS_UNSCALED;

    // need to check both left- and right-edged tiles below player in case player is straddling two tiles
    // (which is the usual case)
    bool onSolidGround = ((level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileX]      & SOLID_TOP) ||
                          (level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileXright] & SOLID_TOP));    

    return onSolidGround;
}

bool CanMoveUp(void)
{
    // X coord of the left-most column of the player
    int tileX = GLOBALS::player.x / TILE_WIDTH_PIXELS_UNSCALED;

    // X coord of the right-most column of the player
    int tileXright = (GLOBALS::player.x + GLOBALS::player.DrawWidth() - 1) / TILE_WIDTH_PIXELS_UNSCALED;

    // the Y coord of the row immediately above the player
    int tileY = (GLOBALS::player.y - 1) / TILE_HEIGHT_PIXELS_UNSCALED;

    // need to check both left- and right-edged tiles above player in case player is straddling two tiles
    // (which is the usual case)
    bool canMoveUp = (!(level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileX]      & SOLID_BOTTOM) &&
                      !(level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileXright] & SOLID_BOTTOM));        

    return canMoveUp;
}


// Pixel Perfect collision detector
// from: https://www.allegro.cc/forums/thread/606547
#if 0
bool sprite_collide(const sprite *object1, const sprite *object2)
{
    int left1, left2, over_left;
    int right1, right2, over_right;
    int top1, top2, over_top;
    int bottom1, bottom2, over_bottom;
    int over_width, over_height;
    int cx, cy;
    ALLEGRO_COLOR pixel[2];
    ALLEGRO_COLOR trans = al_map_rgba(0,0,0,0);
    bool collision = false;

    left1 = object1->x;
    left2 = object2->x;
    right1 = object1->x + al_get_bitmap_width(object1->img);
    right2 = object2->x + al_get_bitmap_width(object2->img);
    top1 = object1->y;
    top2 = object2->y;
    bottom1 = object1->y + al_get_bitmap_height(object1->img);
    bottom2 = object2->y + al_get_bitmap_height(object2->img);


    // First we'll test if the bounding boxes overlap.
    // If they don't overlap at all, there's no sense in checking further.
    if(bottom1 < top2) return(false);
    if(top1 > bottom2) return(false);
    if(right1 < left2) return(false);
    if(left1 > right2) return(false);

    // The bounding boxes overlap, so there's a potential collision.
    // We'll store the location of the actual overlap
    if(bottom1 > bottom2) over_bottom = bottom2;
    else over_bottom = bottom1;

    if(top1 < top2) over_top = top2;
    else over_top = top1;

    if(right1 > right2) over_right = right2;
    else over_right = right1;

    if(left1 < left2) over_left = left2;
    else over_left = left1;

    over_height = over_bottom - over_top;
    over_width = over_right - over_left;

    al_lock_bitmap(object1->img , al_get_bitmap_format(object1->img) , ALLEGRO_LOCK_READONLY);
    al_lock_bitmap(object2->img , al_get_bitmap_format(object2->img) , ALLEGRO_LOCK_READONLY);
    // Okay, we found where the overlap occured and we'll now only check within that area for any
    // collisions.
    for(cy=0; cy < over_height; cy++)
    {
        for(cx=0; cx < over_width; cx++)
        {
            // sample a pixel from each object
            pixel[0] = al_get_pixel(object1->img, (over_left-object1->x)+cx, (over_top-object1->y)+cy);
            pixel[1] = al_get_pixel(object2->img, (over_left-object2->x)+cx, (over_top-object2->y)+cy);

            if(memcmp(&pixel[0], &trans, sizeof(trans)) && memcmp(&pixel[1], &trans, sizeof(trans)))
            {
                collision=true;
                break;
            }
        }
    
    if(collision)
        break;
    }

    al_unlock_bitmap(object1->img);
    al_unlock_bitmap(object2->img);

    return collision;
}
#endif


void ResetLevel(void)
{
    unsigned int i;

    /* starting tile position is mapcode 1 */
    for (i = 0; i < (LEVEL_HEIGHT_TILES * LEVEL_WIDTH_TILES); ++i)
        if (level1MapData.codes[i] == eCODE_PLAYER_SPAWN)
        {
            GLOBALS::player.x = (i % LEVEL_WIDTH_TILES) * TILE_WIDTH_PIXELS_UNSCALED;
            GLOBALS::player.y = (i / LEVEL_WIDTH_TILES) * TILE_HEIGHT_PIXELS_UNSCALED;
            break;
        }

    // unscaled pixels per step
    GLOBALS::player.xVelocity = 2;
    GLOBALS::player.yVelocity = 2;

    GLOBALS::player.jumping = false;
    GLOBALS::player.jumpedSoFar = 0;
    GLOBALS::player.facing = eFACING_RIGHT;

    RedrawScreen();
}

bool InDeathSquare(void)
{
    unsigned int tileX, tileY, tileIndex;

    // upper-left corner of player
    tileY = GLOBALS::player.y / TILE_HEIGHT_PIXELS_UNSCALED;
    tileX = GLOBALS::player.x / TILE_WIDTH_PIXELS_UNSCALED;
    tileIndex = (tileY * LEVEL_WIDTH_TILES) + tileX;
    if (level1MapData.codes[tileIndex] == eCODE_DEATH)
        return true;

    // upper-right corner of player
    tileX = (GLOBALS::player.x + GLOBALS::player.DrawWidth() - 1) / TILE_WIDTH_PIXELS_UNSCALED;
    tileIndex = (tileY * LEVEL_WIDTH_TILES) + tileX;
    if (level1MapData.codes[tileIndex] == eCODE_DEATH)
        return true;

    // lower-left corner of player
    tileY = (GLOBALS::player.y + TILE_HEIGHT_PIXELS_UNSCALED - 1) / TILE_HEIGHT_PIXELS_UNSCALED;
    tileX = GLOBALS::player.x / TILE_WIDTH_PIXELS_UNSCALED;
    tileIndex = (tileY * LEVEL_WIDTH_TILES) + tileX;
    if (level1MapData.codes[tileIndex] == eCODE_DEATH)
        return true;

    // lower-right corner of player
    tileX = (GLOBALS::player.x + GLOBALS::player.DrawWidth() - 1) / TILE_WIDTH_PIXELS_UNSCALED;
    tileIndex = (tileY * LEVEL_WIDTH_TILES) + tileX;
    if (level1MapData.codes[tileIndex] == eCODE_DEATH)
        return true;

    return false;
}

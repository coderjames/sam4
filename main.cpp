#include <cstdio>
#include <cstdlib>     /* srand, rand */
#include <ctime>
#include <cassert>
#include <list>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_physfs.h>

#include <physfs.h>

#include "sam_shared.hpp"

#include "interactives.hpp"

#include "level1.h"

const char *ORGANIZATION_NAME = "jdooley.org";
const char *APPLICATION_NAME = "SAM4";

const signed int TILE_WIDTH_PIXELS_UNSCALED  = 32;
const signed int TILE_HEIGHT_PIXELS_UNSCALED = 32;

const signed int VIEWPORT_WIDTH_TILES  = 20;
const signed int VIEWPORT_HEIGHT_TILES = 15;

const signed int VIEWPORT_WIDTH_PIXELS_UNSCALED  = VIEWPORT_WIDTH_TILES  * TILE_WIDTH_PIXELS_UNSCALED;
const signed int VIEWPORT_HEIGHT_PIXELS_UNSCALED = VIEWPORT_HEIGHT_TILES * TILE_HEIGHT_PIXELS_UNSCALED;

const signed int LEVEL_WIDTH_VIEWPORTS  = 2; // two screens wide
const signed int LEVEL_HEIGHT_VIEWPORTS = 2; // two screens high

const signed int LEVEL_WIDTH_TILES  = LEVEL_WIDTH_VIEWPORTS  * VIEWPORT_WIDTH_TILES;
const signed int LEVEL_HEIGHT_TILES = LEVEL_HEIGHT_VIEWPORTS * VIEWPORT_HEIGHT_TILES;

const signed int PLAYER_MAX_JUMP_HEIGHT_UNSCALED = (TILE_HEIGHT_PIXELS_UNSCALED * 2) + (TILE_HEIGHT_PIXELS_UNSCALED / 2);

const signed int LEVEL_WIDTH_PIXELS_UNSCALED  = LEVEL_WIDTH_TILES  * TILE_WIDTH_PIXELS_UNSCALED;
const signed int LEVEL_HEIGHT_PIXELS_UNSCALED = LEVEL_HEIGHT_TILES * TILE_HEIGHT_PIXELS_UNSCALED;

const signed int SCALE_FACTOR = 2;
const signed int SCREEN_WIDTH_PIXELS_SCALED  = VIEWPORT_WIDTH_PIXELS_UNSCALED  * SCALE_FACTOR;
const signed int SCREEN_HEIGHT_PIXELS_SCALED = VIEWPORT_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR;

// how many seconds is each frame of the player's animation displayed for
const double ANIMATION_RATE = 0.125;


namespace GLOBALS
{
    ALLEGRO_EVENT_QUEUE *events;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *tileAtlas_unscaled;
    ALLEGRO_FONT *defaultFont;

    ALLEGRO_BITMAP *background_scaled;

    TPlayer player;
    std::list<TObject *> interactives;
}

/* create a wrapper to throw away the int return value of PHYSFS_deinit() */
static void atexitwrapper_PhysFS_deinit(void) { PHYSFS_deinit(); }

static bool InitGame(int argc, char **argv);
static void DoTitleScreen(void);
static void DoMainMenu(void);
static void PlayGame(void);
static void ShutdownGame(void);
static void ResetLevel(void);
static void ProcessAction(action_t action);
static void DrawStatusBar(void);


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

    al_set_new_display_flags(ALLEGRO_FULLSCREEN | ALLEGRO_OPENGL | ALLEGRO_OPENGL_3_0);

    al_set_new_display_option(ALLEGRO_COMPATIBLE_DISPLAY, 1, ALLEGRO_REQUIRE);
    al_set_new_display_option(ALLEGRO_CAN_DRAW_INTO_BITMAP, 1, ALLEGRO_REQUIRE);
    al_set_new_display_option(ALLEGRO_RENDER_METHOD, 1, ALLEGRO_REQUIRE);
    
    GLOBALS::display = al_create_display(1920, 1080);

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
    {
        fprintf(stderr, "\nERROR: unable to load tilesheet.\n");
        return false;
    }
    
    GLOBALS::tileAtlas_unscaled = al_create_bitmap(al_get_bitmap_width(tileAtlas_temp) * 2, al_get_bitmap_height(tileAtlas_temp) * 2);
    if (GLOBALS::tileAtlas_unscaled == NULL)
    {
        fprintf(stderr, "\nERROR: unable to create scaled tilesheet");
        return false;
    }

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
    bool remove;
    std::list<TObject *>::iterator it_remover;
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
        delta_time = al_get_time() - time_of_last_frame;
        time_of_last_frame = al_get_time();

        GLOBALS::player.Tick(delta_time);
        for (std::list<TObject *>::iterator it = GLOBALS::interactives.begin(); it != GLOBALS::interactives.end(); ++it)
        {
            assert(*it);
            (*it)->Tick(delta_time);
        }

        if (wants_left)
            ProcessAction(eACTION_MOVE_LEFT);
        if (wants_right)
            ProcessAction(eACTION_MOVE_RIGHT);
        if (wants_fire)
            ProcessAction(eACTION_FIRE);
        if (wants_jump)
            ProcessAction(eACTION_JUMP);


        // check for collisions
        for (std::list<TObject *>::iterator it = GLOBALS::interactives.begin(); it != GLOBALS::interactives.end(); ++it)
        {
            assert(*it);

            if (ObjectCollide(*it, &(GLOBALS::player)))
            {                
                remove = (*it)->CollidedWith(GLOBALS::player);

                if (remove)
                {
                    // remove the object from the interactives list. It was a one-shot interaction
                    it_remover = it;
                    --it; // back up the iterator, since it will be incremented by the for() loop
                          // after this, it points to the item before it_remover.
                    GLOBALS::interactives.erase(it_remover);
                          // now it_remover (the colliding object) has been removed from the list
                          // so when the for() loop increments the iterator, it will point to the
                          // item after colliding object that was just removed from the list
                    delete (*it_remover);
                }
            }

            //if (there is an active shot)
                // if it collided with the object
                    // call CollidedWith(shot)
        }

        if (InDeathSquare())
        {
            ResetLevel();
            continue;
        }


        // player is either jumping or falling
        if (GLOBALS::player.m_jumping)
        {
            for (i = 0; i < GLOBALS::player.m_yVelocity; ++i)
            {
                if (CanMoveUp())
                {
                    GLOBALS::player.m_y -= 1;
                    GLOBALS::player.m_jumpedSoFar += 1;

                    if (GLOBALS::player.m_jumpedSoFar >= PLAYER_MAX_JUMP_HEIGHT_UNSCALED)
                        GLOBALS::player.m_jumping = false;
                }
                else // solid blocks above
                    GLOBALS::player.m_jumping = false;
            }
        }
        else // try to apply gravity
        {
            for (i = 0; i < GLOBALS::player.m_yVelocity; ++i)
            {
                if (!OnSolidGround())
                {
                    GLOBALS::player.m_y += 1;
                }
            }
        }

        RedrawScreen();
    } /* while(!bDone) */
}

void DoTitleScreen(void)
{
    // TODO: title screen
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
    // TODO: main menu
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
    worldX = (GLOBALS::player.m_x + (TILE_WIDTH_PIXELS_UNSCALED / 2)) - (VIEWPORT_WIDTH_PIXELS_UNSCALED / 2);
    worldY = (GLOBALS::player.m_y + (TILE_HEIGHT_PIXELS_UNSCALED / 2)) - (VIEWPORT_HEIGHT_PIXELS_UNSCALED / 2);

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

    // draw the player
    const unsigned int playerTileID = GLOBALS::player.TileID();
    const unsigned int atlasWidth_tiles = al_get_bitmap_width(GLOBALS::tileAtlas_unscaled) / TILE_WIDTH_PIXELS_UNSCALED;
    al_draw_scaled_bitmap(GLOBALS::tileAtlas_unscaled,
                                      (playerTileID % atlasWidth_tiles) * TILE_WIDTH_PIXELS_UNSCALED, (playerTileID / atlasWidth_tiles) * TILE_HEIGHT_PIXELS_UNSCALED,
                                      TILE_WIDTH_PIXELS_UNSCALED, TILE_HEIGHT_PIXELS_UNSCALED,
                                      (GLOBALS::player.m_x - worldX) * SCALE_FACTOR, (GLOBALS::player.m_y - worldY) * SCALE_FACTOR,
                                      TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR,
                                      0);

    // and all the interactives
    unsigned int tileID;
    signed int x, y;
    for (std::list<TObject *>::iterator it = GLOBALS::interactives.begin(); it != GLOBALS::interactives.end(); ++it)
    {
        assert(*it);

        tileID = (*it)->TileID();
        x = (*it)->m_x;
        y = (*it)->m_y;
        unsigned int relativeX_unscaled = x - worldX;
        unsigned int relativeX_scaled = relativeX_unscaled * SCALE_FACTOR;

        // only draw the thing if it is currently on the screen
        if (relativeX_scaled < SCREEN_WIDTH_PIXELS_SCALED)
        {
            // TODO: only draw the visible portion, not the whole tile

            al_draw_scaled_bitmap(GLOBALS::tileAtlas_unscaled,
                                  (tileID % atlasWidth_tiles) * TILE_WIDTH_PIXELS_UNSCALED, (tileID / atlasWidth_tiles) * TILE_HEIGHT_PIXELS_UNSCALED,
                                  TILE_WIDTH_PIXELS_UNSCALED, TILE_HEIGHT_PIXELS_UNSCALED,
                                  (x - worldX) * SCALE_FACTOR, (y - worldY) * SCALE_FACTOR,
                                  TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR,
                                  0);
        }
    }

    // TODO: copy appropriate region of foreground bitmap to screen (eventually, if there is one)


    // display some debugging information
#if 0
    al_draw_textf(GLOBALS::defaultFont, al_map_rgb(255,255,255), TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED, 0,
                  "onGround(%d) canMoveUp(%d), jumping(%d)",
                  OnSolidGround(), CanMoveUp(), GLOBALS::player.m_jumping);
#endif

    DrawStatusBar();

    al_flip_display();
}

void ProcessAction(action_t action)
{
    int tileX, tileYtop, tileYbottom, i;

    tileYtop = GLOBALS::player.m_y / TILE_HEIGHT_PIXELS_UNSCALED;
    tileYbottom = (GLOBALS::player.m_y + TILE_HEIGHT_PIXELS_UNSCALED - 1) / TILE_HEIGHT_PIXELS_UNSCALED;

    // IMPORTANT: ALL MOVEMENTS ARE PERFORMED IN THE UNSCALED PIXEL WORLD
    switch (action)
    {
        case eACTION_MOVE_LEFT:
            GLOBALS::player.m_facing = eFACING_LEFT;


            for (i = 0; i < GLOBALS::player.m_xVelocity; ++i)
            {
                tileX = max(0, GLOBALS::player.m_x - 1) / TILE_WIDTH_PIXELS_UNSCALED;

                if (!(level1MapData.bounds[tileYtop    * LEVEL_WIDTH_TILES + tileX] & SOLID_RIGHT) &&
                    !(level1MapData.bounds[tileYbottom * LEVEL_WIDTH_TILES + tileX] & SOLID_RIGHT))
                    GLOBALS::player.m_x -= 1;
                else
                    break;
            }
            break;

        case eACTION_MOVE_RIGHT:
            GLOBALS::player.m_facing = eFACING_RIGHT;

            for (i = 0; i < GLOBALS::player.m_xVelocity; ++i)
            {
                tileX = min(LEVEL_WIDTH_PIXELS_UNSCALED - 1, GLOBALS::player.m_x + GLOBALS::player.DrawWidth()) / TILE_WIDTH_PIXELS_UNSCALED;

                if (!(level1MapData.bounds[tileYtop    * LEVEL_WIDTH_TILES + tileX] & SOLID_LEFT) &&
                    !(level1MapData.bounds[tileYbottom * LEVEL_WIDTH_TILES + tileX] & SOLID_LEFT))
                    GLOBALS::player.m_x += 1;
                else
                    break;
            }
            break;

        case eACTION_JUMP:
            if ((!GLOBALS::player.m_jumping) && OnSolidGround())
            {
                GLOBALS::player.m_jumping = true;
                GLOBALS::player.m_jumpedSoFar = 0;
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
    if ((GLOBALS::player.m_y % TILE_HEIGHT_PIXELS_UNSCALED) != 0)
        return false;

    // X coord of the left-most column of the player
    int tileX = GLOBALS::player.m_x / TILE_WIDTH_PIXELS_UNSCALED;

    // X coord of the right-most column of the player
    int tileXright = (GLOBALS::player.m_x + GLOBALS::player.DrawWidth() - 1) / TILE_WIDTH_PIXELS_UNSCALED;

    // the Y coord of the row immediately below the player
    int tileY = (GLOBALS::player.m_y + TILE_HEIGHT_PIXELS_UNSCALED) / TILE_HEIGHT_PIXELS_UNSCALED;

    // need to check both left- and right-edged tiles below player in case player is straddling two tiles
    // (which is the usual case)
    bool onSolidGround = ((level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileX]      & SOLID_TOP) ||
                          (level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileXright] & SOLID_TOP));    

    return onSolidGround;
}

bool CanMoveUp(void)
{
    // X coord of the left-most column of the player
    int tileX = GLOBALS::player.m_x / TILE_WIDTH_PIXELS_UNSCALED;

    // X coord of the right-most column of the player
    int tileXright = (GLOBALS::player.m_x + GLOBALS::player.DrawWidth() - 1) / TILE_WIDTH_PIXELS_UNSCALED;

    // the Y coord of the row immediately above the player
    int tileY = (GLOBALS::player.m_y - 1) / TILE_HEIGHT_PIXELS_UNSCALED;

    // need to check both left- and right-edged tiles above player in case player is straddling two tiles
    // (which is the usual case)
    bool canMoveUp = (!(level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileX]      & SOLID_BOTTOM) &&
                      !(level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileXright] & SOLID_BOTTOM));        

    return canMoveUp;
}


// Pixel Perfect collision detector
// from: https://www.allegro.cc/forums/thread/606547
bool ObjectCollide(const TObject *object1, const TObject *object2)
{
    assert(object1);
    assert(object2);

    if (object1 == object2)
        return true;

    int left1, left2, over_left;
    int right1, right2, over_right;
    int top1, top2, over_top;
    int bottom1, bottom2, over_bottom;
    int over_width, over_height;
    int cx, cy;
    ALLEGRO_COLOR pixel[2];
    ALLEGRO_COLOR trans = al_map_rgba(255,0,255,0);
    bool collision = false;

    ALLEGRO_BITMAP *obj1_img = BitmapOfTile(object1->TileID());
    ALLEGRO_BITMAP *obj2_img = BitmapOfTile(object2->TileID());

    assert(obj1_img);
    assert(obj2_img);

    left1 = object1->m_x;
    left2 = object2->m_x;
    right1 = object1->m_x + al_get_bitmap_width(obj1_img);
    right2 = object2->m_x + al_get_bitmap_width(obj2_img);
    top1 = object1->m_y;
    top2 = object2->m_y;
    bottom1 = object1->m_y + al_get_bitmap_height(obj1_img);
    bottom2 = object2->m_y + al_get_bitmap_height(obj2_img);


    // First we'll test if the bounding boxes overlap.
    // If they don't overlap at all, there's no sense in checking further.
    if((bottom1 < top2) ||
       (top1 > bottom2) ||
       (right1 < left2) ||
       (left1 > right2))
    {
        collision = false;
    }
    else
    {
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

        al_lock_bitmap(obj1_img, al_get_bitmap_format(obj1_img), ALLEGRO_LOCK_READONLY);
        al_lock_bitmap(obj2_img, al_get_bitmap_format(obj2_img), ALLEGRO_LOCK_READONLY);
        // Okay, we found where the overlap occured and we'll now only check within that area for any
        // collisions.
        for(cy=0; cy < over_height; cy++)
        {
            for(cx=0; cx < over_width; cx++)
            {
                // sample a pixel from each object
                pixel[0] = al_get_pixel(obj1_img, (over_left-object1->m_x)+cx, (over_top-object1->m_y)+cy);
                pixel[1] = al_get_pixel(obj2_img, (over_left-object2->m_x)+cx, (over_top-object2->m_y)+cy);

                if((memcmp(&pixel[0], &trans, sizeof(trans)) != 0) &&
                   (memcmp(&pixel[1], &trans, sizeof(trans)) != 0))
                {
                    collision=true;
                    break;
                }
            }
        
        if(collision)
            break;
        }

        al_unlock_bitmap(obj1_img);
        al_unlock_bitmap(obj2_img);
    }

    al_destroy_bitmap(obj1_img);
    al_destroy_bitmap(obj2_img);

    return collision;
}

void ResetLevel(void)
{
    unsigned int i;
    signed int x, y;

    GLOBALS::interactives.clear();

    /* starting tile position is mapcode 1 */
    for (i = 0; i < (LEVEL_HEIGHT_TILES * LEVEL_WIDTH_TILES); ++i)
    {
        x = (i % LEVEL_WIDTH_TILES) * TILE_WIDTH_PIXELS_UNSCALED;
        y = (i / LEVEL_WIDTH_TILES) * TILE_HEIGHT_PIXELS_UNSCALED;

        switch(level1MapData.codes[i])
        {
            case eCODE_PLAYER_SPAWN:
                GLOBALS::player.m_x = x;
                GLOBALS::player.m_y = y;
                printf("\nDBUG: spawned player at (%d, %d)", x, y);
                break;

            case eCODE_INVISIBLE_PLATFORM:
                // nothing - handled when glasses are picked up
                break;

            case eCODE_GLASSES:
                level1MapData.midTiles[i] = -1;
                GLOBALS::interactives.push_back(new TGlasses(x,y));
                printf("\nDBUG: created glasses at (%d, %d)", x, y);
                break;

            case eCODE_TNT:
                // create new TTnt interactive
                break;

            case eCODE_PUSHABLE:
                // create new pushable interactive with the tile ID of what's in the mid-layer of this square
                GLOBALS::interactives.push_back(new TPushable(level1MapData.midTiles[i], x, y));
                level1MapData.midTiles[i] = -1;
                printf("\nDBUG: created pushable at (%d, %d)", x, y);
                break;

            case eCODE_AMMO:
                level1MapData.midTiles[i] = -1;
                GLOBALS::interactives.push_back(new TAmmo(x,y));
                printf("\nDBUG: created ammo at (%d, %d)", x, y);
                break;

            case eCODE_SATELLITE_DISH:
                level1MapData.midTiles[i] = -1;
                GLOBALS::interactives.push_back(new TSatelliteDish(x,y));
                printf("\nDBUG: created satellite dish at (%d, %d)", x, y);
                break;
        }
    }

    // unscaled pixels per step
    GLOBALS::player.m_xVelocity = 2;
    GLOBALS::player.m_yVelocity = 2;

    GLOBALS::player.m_jumping = false;
    GLOBALS::player.m_jumpedSoFar = 0;
    GLOBALS::player.m_facing = eFACING_RIGHT;

    CreateBackgroundImage();

    RedrawScreen();
}

bool InDeathSquare(void)
{
    unsigned int tileX, tileY, tileIndex;

    // upper-left corner of player
    tileY = GLOBALS::player.m_y / TILE_HEIGHT_PIXELS_UNSCALED;
    tileX = GLOBALS::player.m_x / TILE_WIDTH_PIXELS_UNSCALED;
    tileIndex = (tileY * LEVEL_WIDTH_TILES) + tileX;
    if (level1MapData.codes[tileIndex] == eCODE_DEATH)
        return true;

    // upper-right corner of player
    tileX = (GLOBALS::player.m_x + GLOBALS::player.DrawWidth() - 1) / TILE_WIDTH_PIXELS_UNSCALED;
    tileIndex = (tileY * LEVEL_WIDTH_TILES) + tileX;
    if (level1MapData.codes[tileIndex] == eCODE_DEATH)
        return true;

    // lower-left corner of player
    tileY = (GLOBALS::player.m_y + TILE_HEIGHT_PIXELS_UNSCALED - 1) / TILE_HEIGHT_PIXELS_UNSCALED;
    tileX = GLOBALS::player.m_x / TILE_WIDTH_PIXELS_UNSCALED;
    tileIndex = (tileY * LEVEL_WIDTH_TILES) + tileX;
    if (level1MapData.codes[tileIndex] == eCODE_DEATH)
        return true;

    // lower-right corner of player
    tileX = (GLOBALS::player.m_x + GLOBALS::player.DrawWidth() - 1) / TILE_WIDTH_PIXELS_UNSCALED;
    tileIndex = (tileY * LEVEL_WIDTH_TILES) + tileX;
    if (level1MapData.codes[tileIndex] == eCODE_DEATH)
        return true;

    return false;
}

ALLEGRO_BITMAP *BitmapOfTile(unsigned int tileID)
{
    unsigned int atlasWidth_tiles = al_get_bitmap_width(GLOBALS::tileAtlas_unscaled) / TILE_WIDTH_PIXELS_UNSCALED;

    ALLEGRO_BITMAP *bmp = al_create_sub_bitmap(GLOBALS::tileAtlas_unscaled,
                                               (tileID % atlasWidth_tiles) * TILE_WIDTH_PIXELS_UNSCALED, (tileID / atlasWidth_tiles) * TILE_HEIGHT_PIXELS_UNSCALED,
                                               TILE_WIDTH_PIXELS_UNSCALED, TILE_HEIGHT_PIXELS_UNSCALED);

    assert(bmp);

    return bmp;
}

void DrawStatusBar(void)
{
    al_draw_filled_rectangle(SCREEN_WIDTH_PIXELS_SCALED, 0, SCREEN_WIDTH_PIXELS_SCALED + (TILE_HEIGHT_PIXELS_UNSCALED * 3 * SCALE_FACTOR), SCREEN_HEIGHT_PIXELS_SCALED, al_map_rgb(10,10,150));

    al_draw_textf(GLOBALS::defaultFont, al_map_rgb(255,255,255), SCREEN_WIDTH_PIXELS_SCALED + (TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR), TILE_HEIGHT_PIXELS_UNSCALED    , 0,
                  "Score: %d",
                  GLOBALS::player.Score());

    al_draw_textf(GLOBALS::defaultFont, al_map_rgb(255,255,255), SCREEN_WIDTH_PIXELS_SCALED + (TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR), TILE_HEIGHT_PIXELS_UNSCALED * 2, 0,
                  "Shots: %d",
                  GLOBALS::player.Ammo());

    al_draw_textf(GLOBALS::defaultFont, al_map_rgb(255,255,255), SCREEN_WIDTH_PIXELS_SCALED + (TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR), TILE_HEIGHT_PIXELS_UNSCALED * 3, 0,
                  "Lives: %d",
                  0);

}

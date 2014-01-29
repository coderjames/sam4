#ifndef _SAM_SHARED_HPP_
#define _SAM_SHARED_HPP_

#include <list>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

extern const char *ORGANIZATION_NAME;
extern const char *APPLICATION_NAME;

extern const signed int TILE_WIDTH_PIXELS_UNSCALED;
extern const signed int TILE_HEIGHT_PIXELS_UNSCALED;

extern const signed int VIEWPORT_WIDTH_TILES;
extern const signed int VIEWPORT_HEIGHT_TILES;

extern const signed int VIEWPORT_WIDTH_PIXELS_UNSCALED;
extern const signed int VIEWPORT_HEIGHT_PIXELS_UNSCALED;

extern const signed int LEVEL_WIDTH_VIEWPORTS;
extern const signed int LEVEL_HEIGHT_VIEWPORTS;

extern const signed int LEVEL_WIDTH_TILES;
extern const signed int LEVEL_HEIGHT_TILES;

extern const signed int PLAYER_MAX_JUMP_HEIGHT_UNSCALED;

extern const signed int SCALE_FACTOR;

extern const signed int LEVEL_WIDTH_PIXELS_UNSCALED;
extern const signed int LEVEL_HEIGHT_PIXELS_UNSCALED;

extern const signed int SCREEN_WIDTH_PIXELS_SCALED;
extern const signed int SCREEN_HEIGHT_PIXELS_SCALED;

// how many seconds is each frame of the player's animation displayed for
extern const double ANIMATION_RATE;

// forward declarations of interactives
class TObject;
class TMobile;
class TPlayer;
class TGlasses;

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

/* bit flags for whether a block is 'solid' on a particular surface (e.g. cannot be entered from that side).
    MUST MATCH TileStudio definitions */
#define SOLID_TOP     (1 << 0)
#define SOLID_LEFT    (1 << 1)
#define SOLID_BOTTOM  (1 << 2)
#define SOLID_RIGHT   (1 << 3)

typedef enum _TFacing
{
    eFACING_LEFT = 0,
    eFACING_RIGHT = 1
} TFacing;

typedef enum _TMapCode
{
    eCODE_PLAYER_SPAWN       = 1,
    eCODE_DEATH              = 2,
    
    eCODE_INVISIBLE_PLATFORM = 3,
    
    eCODE_GLASSES            = 4,    
    eCODE_TNT                = 5,
    eCODE_PUSHABLE           = 6,
    eCODE_AMMO               = 7,
    eCODE_SATELLITE          = 8,

    eCODE_USE_TNT            = 9   /* this code is placed immediately adjacent to the exit door, where TNT can be used */
} TMapCode;

typedef enum
{
    eACTION_MOVE_LEFT   = 0,
    eACTION_MOVE_RIGHT  = 1,
    eACTION_JUMP        = 2,
    eACTION_FIRE        = 3,
} action_t;


namespace GLOBALS
{
    extern ALLEGRO_EVENT_QUEUE *events;
    extern ALLEGRO_DISPLAY *display;
    extern ALLEGRO_BITMAP *tileAtlas_unscaled;
    extern ALLEGRO_FONT *defaultFont;

    extern ALLEGRO_BITMAP *background_scaled;

    extern TPlayer player;
    extern std::list<TObject *> interactives;
}


void RedrawScreen(void);
void CreateBackgroundImage(void);

bool OnSolidGround(void);
bool CanMoveUp(void);
bool InDeathSquare(void);

bool ObjectCollide(const TObject *object1, const TObject *object2);

ALLEGRO_BITMAP *BitmapOfTile(unsigned int tileID);


#endif

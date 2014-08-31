#include <cassert>
#include <cstdio>

#include "sam_shared.hpp"
#include "interactives.hpp"

#include "level1.h"

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
    { 22, 22, 22, 22 }, /* standing left  */
    { 22, 22, 22, 22 }, /* standing right */

    { 22, 22, 22, 22 }, /* standing left  */
    { 22, 22, 22, 22 }, /* standing right */
    //{ 28, 28, 28, 28 }, /* jumping left   */
    //{ 28, 28, 28, 28 }, /* jumping right  */

    { 20, 32, 32, 20 }, /* shooting left  */
    { 20, 32, 32, 20 }  /* shooting right */
};

void TPlayer::Tick(double delta_seconds) 
{
    m_seconds_since_last_frame_change += delta_seconds;

    if (m_seconds_since_last_frame_change >= ANIMATION_RATE)
    {
        m_frameIndex = (m_frameIndex + 1) % eFRAMES_PER_ANIMATION;
        m_seconds_since_last_frame_change = 0.0;
    }

    /* select the current animation */
    if (m_facing == eFACING_RIGHT)
    {
        if (m_jumping || !OnSolidGround())
        	m_animation = eANIM_JUMPING_RIGHT;
        // else if (m_shooting)
        //  m_animation = eANIM_SHOOTING_RIGHT;
        else
        	m_animation = eANIM_STANDING_RIGHT;
	}
    else if (m_facing == eFACING_LEFT)
    {
        if (m_jumping || !OnSolidGround())
        	m_animation = eANIM_JUMPING_LEFT;
        // else if (m_shooting)
        //  m_animation = eANIM_SHOOTING_LEFT;
        else
        	m_animation = eANIM_STANDING_LEFT;
    }
}


unsigned int TPlayer::TileID() const
{
    return frames[m_animation][m_frameIndex];
}

signed int TPlayer::DrawWidth() const
{
    return widths[m_animation][m_frameIndex];
}


bool TPlayer::CanFireBullet() const
{
	return (m_bulletsFlying < m_maxBulletsFlying) && (m_ammo > 0);
}


void TPlayer::FireBullet()
{
	assert(m_ammo > 0);
	assert(m_bulletsFlying >= 0);
	assert(m_bulletsFlying < m_maxBulletsFlying);

	--m_ammo;
	++m_bulletsFlying;

	signed int x = m_x;
	if (m_facing == eFACING_LEFT) // need to put the bullet to the left of the player
		x -= (TILE_WIDTH_PIXELS_UNSCALED / 2);
	else // facing right, so put bullet to the right of the player
		x += (DrawWidth() + (TILE_WIDTH_PIXELS_UNSCALED / 2));

	signed int y = m_y + (TILE_HEIGHT_PIXELS_UNSCALED / 6);

	GLOBALS::interactives.push_back(new TBullet(x, y, m_facing));
}

void TPlayer::BulletDied()
{
	assert(m_bulletsFlying);
	--m_bulletsFlying;
}

bool TGlasses::CollidedWith(TObject &obj)
{
    if (&obj == &GLOBALS::player)
    {
        const unsigned int atlasWidth_tiles = al_get_bitmap_width(GLOBALS::tileAtlas_unscaled) / TILE_WIDTH_PIXELS_UNSCALED;
        unsigned int tileY, tileX;
        signed int tileID, tileIndex;

        al_set_target_bitmap(GLOBALS::background_scaled);

        // turn on the invisible platforms
        for (tileIndex = 0; tileIndex < (LEVEL_HEIGHT_TILES * LEVEL_WIDTH_TILES); ++tileIndex)
        {
            if (level1MapData.codes[tileIndex] == eCODE_INVISIBLE_PLATFORM)
            {
                level1MapData.codes[tileIndex] = 0;

                tileID = 53;

                level1MapData.bounds[tileIndex] = SOLID_TOP;
                level1MapData.midTiles[tileIndex] = tileID;

                tileY = tileIndex / LEVEL_WIDTH_TILES;
                tileX = tileIndex % LEVEL_WIDTH_TILES;
                al_draw_scaled_bitmap(GLOBALS::tileAtlas_unscaled,
                                      (tileID % atlasWidth_tiles) * TILE_WIDTH_PIXELS_UNSCALED, (tileID / atlasWidth_tiles) * TILE_HEIGHT_PIXELS_UNSCALED,
                                      TILE_WIDTH_PIXELS_UNSCALED, TILE_HEIGHT_PIXELS_UNSCALED,
                                      (TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR) * tileX, (TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR) * tileY,
                                      TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR,
                                      0);
            }
        }
        // paint over glasses graphic in background image
        tileY = m_y/TILE_HEIGHT_PIXELS_UNSCALED;
        tileX = m_x/TILE_WIDTH_PIXELS_UNSCALED;

/*
        tileID = level1MapData.backTiles[(tileY*LEVEL_WIDTH_TILES)+tileX];

        al_draw_scaled_bitmap(GLOBALS::tileAtlas_unscaled,
                              (tileID % atlasWidth_tiles) * TILE_WIDTH_PIXELS_UNSCALED, (tileID / atlasWidth_tiles) * TILE_HEIGHT_PIXELS_UNSCALED,
                              TILE_WIDTH_PIXELS_UNSCALED, TILE_HEIGHT_PIXELS_UNSCALED,
                              (TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR) * tileX, (TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR) * tileY,
                              TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR,
                              0);
*/
        return true;
    }

    return false;
}



const unsigned int TSatelliteDish::frames[eFRAMES_PER_ANIMATION] = {357, 358, 357, 359}; /* center, right, center, left */
const   signed int TSatelliteDish::widths[eFRAMES_PER_ANIMATION] = {TILE_WIDTH_PIXELS_UNSCALED, TILE_WIDTH_PIXELS_UNSCALED, TILE_WIDTH_PIXELS_UNSCALED, TILE_WIDTH_PIXELS_UNSCALED};

void TSatelliteDish::Tick(double delta_seconds)
{
	m_seconds_since_last_frame_change += delta_seconds;

	if (m_seconds_since_last_frame_change >= 0.33)
	{
		m_frameIndex = (m_frameIndex + 1) % eFRAMES_PER_ANIMATION;
		m_seconds_since_last_frame_change = 0.0;
	}
}

bool TSatelliteDish::CollidedWith(TObject &obj)
{
	/*
	if (obj == player's bullet)
		++m_timeShot;

		if (m_timesShot >= hit points)
			remove from level
			give player points
			record that dish is destroyed, so player is allowed to exit
			return true;
	*/

	return false;
}


unsigned int TSatelliteDish::TileID() const
{
    return frames[m_frameIndex];
}

signed int TSatelliteDish::DrawWidth() const
{
    return widths[m_frameIndex];
}



bool TAmmo::CollidedWith(TObject &obj)
{
    if (&obj == &GLOBALS::player)
    {
        GLOBALS::player.AddAmmo(5); // 5 = number of shots awarded for each ammo collected
        return true;
    }

    return false;
}


TPushable::TPushable(unsigned int tileID, signed int x, signed int y) : TObject(tileID, x, y)
{
}

bool TPushable::CollidedWith(TObject &obj)
{
    int oldX = m_x;
    int oldY = m_y;
	int tileX = oldX / TILE_WIDTH_PIXELS_UNSCALED;
    int tileXright = (oldX + DrawWidth() - 1) / TILE_WIDTH_PIXELS_UNSCALED;
    int tileY = oldY / TILE_HEIGHT_PIXELS_UNSCALED;

    if (&obj == &GLOBALS::player)
    {
        if ((GLOBALS::player.m_x < m_x) && (GLOBALS::player.m_facing == eFACING_RIGHT)) // player is on left, trying to push right
        {
        	tileXright = (oldX + DrawWidth()) / TILE_WIDTH_PIXELS_UNSCALED;
        	if (!(level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileXright] & SOLID_LEFT))
        	{
        		m_x = GLOBALS::player.m_x + GLOBALS::player.DrawWidth();
        	}
        }
        else if ((GLOBALS::player.m_x > m_x) && (GLOBALS::player.m_facing == eFACING_LEFT)) // player is on right, trying to push left
        {
        	tileX = (oldX-1) / TILE_WIDTH_PIXELS_UNSCALED;
            if (!(level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileX] & SOLID_RIGHT))
            {
            	m_x = GLOBALS::player.m_x - DrawWidth();
            }
        }
    }

    // always returns false because pushables are never removed, even when touched
    return false;
}


TBullet::TBullet(signed int x, signed int y, TFacing directionMoving) : TObject(280, x, y), m_xReal(x), m_xVelocity(TILE_WIDTH_PIXELS_UNSCALED * 3)
{
	if (directionMoving == eFACING_LEFT)
		m_xVelocity *= -1.0;

    printf("\nDBUG: created bullet at (%d, %d) moving %d", x, y, directionMoving);
};

void TBullet::Tick(double delta_seconds)
{
	m_xReal += (delta_seconds * m_xVelocity); // velocity in pixels per second
	m_x = int(m_xReal);

	// if colliding with a map bounding border, need to delete myself
}

bool TBullet::CollidedWith(TObject __attribute__ ((unused)) &obj)
{
	// always return true because bullets are always removed when they collide with something,
	// regardless of what it was.
	return true;
}

TBullet::~TBullet()
{
	GLOBALS::player.BulletDied();
}

#include <cassert>

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
    { 24, 24, 24, 24 },
    { 24, 24, 24, 24 },
    { 28, 28, 28, 28 },
    { 28, 28, 28, 28 },
    { 20, 32, 32, 20 },
    { 20, 32, 32, 20 }
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

        // remove the tile from the map, in case something regenerates the background image
        level1MapData.midTiles[(tileY * LEVEL_WIDTH_TILES) + tileX] = -1;

        tileID = level1MapData.backTiles[(tileY*LEVEL_WIDTH_TILES)+tileX];

        al_draw_scaled_bitmap(GLOBALS::tileAtlas_unscaled,
                              (tileID % atlasWidth_tiles) * TILE_WIDTH_PIXELS_UNSCALED, (tileID / atlasWidth_tiles) * TILE_HEIGHT_PIXELS_UNSCALED,
                              TILE_WIDTH_PIXELS_UNSCALED, TILE_HEIGHT_PIXELS_UNSCALED,
                              (TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR) * tileX, (TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR) * tileY,
                              TILE_WIDTH_PIXELS_UNSCALED * SCALE_FACTOR, TILE_HEIGHT_PIXELS_UNSCALED * SCALE_FACTOR,
                              0);

        return true;
    }

    return false;
}

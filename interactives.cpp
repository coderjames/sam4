#include <cassert>
#include <cstdio>
#include <cmath>

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

const char *TPlayer::m_stateStrings[eSTATE_COUNT] =
{
	"STANDING",
	"WALKING ",
	"JUMPING ",
	"FALLING ",
	"FIRING  ",
};

void TPlayer::Tick(double delta_seconds) 
{
    m_seconds_since_last_frame_change += delta_seconds;

    // don't animate if standing still
    if (m_state == eSTATE_STANDING)
    	m_frameIndex = 0;
    else if (m_seconds_since_last_frame_change >= ANIMATION_RATE)
    {
        m_frameIndex = (m_frameIndex + 1) % eFRAMES_PER_ANIMATION;
        m_seconds_since_last_frame_change = 0.0;
    }

    switch (m_state)
    {
    case eSTATE_WALKING:
    	m_animation = (m_facing == eFACING_RIGHT) ?
    						eANIM_STANDING_RIGHT :
    						eANIM_STANDING_LEFT;
    	MoveHorizontal(delta_seconds);
		if (OnSolidGround())
			ChangeToState(eSTATE_STANDING);
		else
			ChangeToState(eSTATE_FALLING);
		break;

    case eSTATE_STANDING:
    	m_animation = (m_facing == eFACING_RIGHT) ?
    						eANIM_STANDING_RIGHT :
    						eANIM_STANDING_LEFT;

    	// in case the ground disappears out from under the player
    	//if (!OnSolidGround())
    	//	ChangeToState(eSTATE_FALLING);
    	break;

    case eSTATE_JUMPING:
    	// FALL THROUGH
    case eSTATE_FALLING:
    	m_animation = (m_facing == eFACING_RIGHT) ?
    						eANIM_JUMPING_RIGHT :
    						eANIM_JUMPING_LEFT;
    	MoveVertical(delta_seconds);
    	MoveHorizontal(delta_seconds);
    	break;

    case eSTATE_FIRING:
    	m_animation = (m_facing == eFACING_RIGHT) ?
    						eANIM_SHOOTING_RIGHT :
    						eANIM_SHOOTING_LEFT;
    	MoveVertical(delta_seconds);
    	MoveHorizontal(delta_seconds);
    	// stay in the firing state for [x] amount of time in order to display the animation
    	// and prevent rapid-fire (full auto)
    	// after enough time has passed, go to the other states depending on velocities.
    	break;
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

void TPlayer::ProcessAction(action_t action)
{
	// assume there will not be a state transition unless otherwise determined
	TPlayer::TPlayerState newState = m_state;

	// what the player is allowed to do depends on which state player is currently in.
	switch (m_state)
	{
	case eSTATE_STANDING:
		// all actions are valid from the standing state
		switch (action)
		{
		case eACTION_MOVE_LEFT:
			m_facing = eFACING_LEFT;
			newState = eSTATE_WALKING;
			break;

		case eACTION_MOVE_RIGHT:
			m_facing = eFACING_RIGHT;
			newState = eSTATE_WALKING;
			break;

		case eACTION_JUMP:
			if (OnSolidGround())
				newState = eSTATE_JUMPING;
			break;

		case eACTION_FIRE:
			if (m_ammo)
				newState = eSTATE_FIRING;
			break;

		default:
			assert(0);
		}
		break;

	case eSTATE_WALKING:
		// all actions are valid from the walking state
		switch (action)
		{
		case eACTION_MOVE_LEFT:
			m_facing = eFACING_LEFT;
			newState = eSTATE_WALKING;
			break;

		case eACTION_MOVE_RIGHT:
			m_facing = eFACING_RIGHT;
			newState = eSTATE_WALKING;
			break;

		case eACTION_JUMP:
			if (OnSolidGround())
				newState = eSTATE_JUMPING;
			break;

		case eACTION_FIRE:
			if (m_ammo)
				newState = eSTATE_FIRING;
			break;

		default:
			assert(0);
		}
		break;

	case eSTATE_JUMPING:
		// from the jumping state, you are allowed to switch directions or land
		switch (action)
		{
		case eACTION_MOVE_LEFT:
			m_facing = eFACING_LEFT;
			m_xVelocityPerSecond = MAX_X_VELOCITY_PER_SECOND;
			break;

		case eACTION_MOVE_RIGHT:
			m_facing = eFACING_RIGHT;
			m_xVelocityPerSecond = MAX_X_VELOCITY_PER_SECOND;
			break;

		case eACTION_JUMP:
			// no double-jumping allowed
			break;

		case eACTION_FIRE:
			// currently not allowed to fire in mid-air.
			break;

		default:
			assert(0);
		}
		break;

	case eSTATE_FALLING:
		// while falling you are allowed to switch directions, shoot, or land
		switch (action)
		{
		case eACTION_MOVE_LEFT:
			m_facing = eFACING_LEFT;
			m_xVelocityPerSecond = MAX_X_VELOCITY_PER_SECOND;
			break;

		case eACTION_MOVE_RIGHT:
			m_facing = eFACING_RIGHT;
			m_xVelocityPerSecond = MAX_X_VELOCITY_PER_SECOND;
			break;

		case eACTION_JUMP:
			// no double-jumping allowed
			break;

		case eACTION_FIRE:
			if (m_ammo)
				newState = eSTATE_FIRING;
			break;

		default:
			assert(0);
		}
		break;

	case eSTATE_FIRING:
		// only thing you can do is turn around.
		switch (action)
		{
		case eACTION_MOVE_LEFT:
			m_facing = eFACING_LEFT;
			break;

		case eACTION_MOVE_RIGHT:
			m_facing = eFACING_RIGHT;
			break;

		case eACTION_JUMP:
			break;

		case eACTION_FIRE:
			// no rapid-firing at the moment
			break;

		default:
			assert(0);
		}
		break;

	default:
		assert(0);
	}

	ChangeToState(newState);
}

void TPlayer::ChangeToState(TPlayer::TPlayerState newState)
{
	if (newState != m_state)
	{
		if (m_stateExit[m_state] != NULL)
			(this->*m_stateExit[m_state])();

		m_state = newState;

		if (m_stateEnter[m_state] != NULL)
			(this->*m_stateEnter[m_state])();
	}
}

void TPlayer::FireBullet()
{
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

void TPlayer::StartJumping()
{
	m_yVelocityPerSecond = -MAX_Y_VELOCITY_PER_SECOND;
	m_x = trunc(m_x);
	m_y = trunc(m_y);
}

void TPlayer::StartWalking()
{
	m_xVelocityPerSecond = MAX_X_VELOCITY_PER_SECOND;
	m_x = trunc(m_x);
	m_y = trunc(m_y);
}

void TPlayer::StartStanding()
{
	m_xVelocityPerSecond = 0;
	m_yVelocityPerSecond = 0;
	m_x = trunc(m_x);
	m_y = trunc(m_y);
}

void TPlayer::MoveHorizontal(double secondsThisTick)
{
	// IMPORTANT: ALL MOVEMENTS ARE PERFORMED IN THE UNSCALED PIXEL WORLD

	double xVelocityThisTick = secondsThisTick * m_xVelocityPerSecond;
	int tileX, tileYtop, tileYbottom;

	// tileYtop is the Y row of the tiles where the player's head is
	tileYtop =     m_y                                    / TILE_HEIGHT_PIXELS_UNSCALED;
	// tileYbottom is the Y row of the tiles where the player's feet are
	tileYbottom = (m_y + TILE_HEIGHT_PIXELS_UNSCALED - 1) / TILE_HEIGHT_PIXELS_UNSCALED;

	// getting less than one tick per second? something's gone very wrong or player's
	// computer is way too slow.
	assert(secondsThisTick <= 1.0);

	if (m_facing == eFACING_LEFT)
	{
		// tileX is the X column of the tiles into which the player wants to move
		tileX = max(0.0                            , m_x - xVelocityThisTick              ) / TILE_WIDTH_PIXELS_UNSCALED;

		if (!(level1MapData.bounds[tileYtop    * LEVEL_WIDTH_TILES + tileX] & SOLID_RIGHT) &&
			!(level1MapData.bounds[tileYbottom * LEVEL_WIDTH_TILES + tileX] & SOLID_RIGHT))
			m_x -= xVelocityThisTick;
		else
			m_xVelocityPerSecond = 0;
	}
	else // moving right
	{
		// tileX is the X column of the tiles into which the player wants to move
		tileX = min(LEVEL_WIDTH_PIXELS_UNSCALED - 1, m_x + xVelocityThisTick + DrawWidth()) / TILE_WIDTH_PIXELS_UNSCALED;

		if (!(level1MapData.bounds[tileYtop    * LEVEL_WIDTH_TILES + tileX] & SOLID_LEFT) &&
			!(level1MapData.bounds[tileYbottom * LEVEL_WIDTH_TILES + tileX] & SOLID_LEFT))
			m_x += xVelocityThisTick;
		else
			m_xVelocityPerSecond = 0;
	}
}

void TPlayer::MoveVertical(double secondsThisTick)
{
	double yVelocityThisTick = secondsThisTick * m_yVelocityPerSecond;
	// getting less than one tick per second? something's gone very wrong or player's
	// computer is way too slow.
	assert(secondsThisTick <= 1.0);

	// player is either jumping or falling
    if (m_yVelocityPerSecond < 0) // negative Y velocity meaning moving upward.
    {
		if (CanMoveVerticalBy(yVelocityThisTick))
		{
			m_y += yVelocityThisTick;

			// velocity starts out at max negative and slows down (by getting closer to zero) as jump progresses
			// to form something slightly resembling parabolic motion
			m_yVelocityPerSecond += ((ACCELERATION_PER_SECOND / 2) * secondsThisTick);

			if (m_yVelocityPerSecond >= 0) // positive velocity means falling
				ChangeToState(eSTATE_FALLING);
		}
		else // solid blocks somewhere above
		{
			// determine how far we actually can move
			double canMove;
			for (canMove = 1.0 + yVelocityThisTick;
				 CanMoveVerticalBy(canMove) && (canMove <= 0);
				 ++canMove)
				; // no more work to do. canMove determines how far player can move.
			if (canMove < 0)
				m_y += canMove;

			ChangeToState(eSTATE_FALLING);
		}
    }
    else // apply gravity. positive Y velocity means falling. (may be falling or firing)
    {
		printf("\nDBUG: MoveVertical Path 2");
		if (CanMoveVerticalBy(yVelocityThisTick+TILE_HEIGHT_PIXELS_UNSCALED))
			// need to account for player height when trying to move downward since
			// player's m_y is the top of the head and feet are what touch the ground.
		{
			printf("\nDBUG: MoveVertical Path 2a");
			m_y += yVelocityThisTick;

			if (OnSolidGround()) // might be now that we fell down some
	        {
				printf("\nDBUG: MoveVertical Path 2a1");

				m_yVelocityPerSecond = 0;
	    		if (m_xVelocityPerSecond != 0)
	    		{
					printf("\nDBUG: MoveVertical Path 2a1a");
	    			ChangeToState(eSTATE_WALKING);
	    		}
	    		else
	    		{
					printf("\nDBUG: MoveVertical Path 2a1b");
	    			ChangeToState(eSTATE_STANDING);
	    		}
	        }
	        else // still falling
	        {
				printf("\nDBUG: MoveVertical Path 2a2");

				// player falls faster the farther they fall (up to a terminal velocity)
	    		m_yVelocityPerSecond += (ACCELERATION_PER_SECOND * secondsThisTick);
	            if (m_yVelocityPerSecond > MAX_Y_VELOCITY_PER_SECOND)
	                m_yVelocityPerSecond = MAX_Y_VELOCITY_PER_SECOND;
	        }
		}
		else // ground is closer than our present velocity. find out where it is and stop there
		{
			printf("\nDBUG: MoveVertical Path 2b");

			m_y = trunc(m_y);
			double canMove;
			for (canMove = TILE_HEIGHT_PIXELS_UNSCALED; // one pixel below current position
				 CanMoveVerticalBy(canMove);            // can player move that far?
			     ++canMove)                             // try even a bit further
				;

			m_y = trunc(m_y + (canMove - TILE_HEIGHT_PIXELS_UNSCALED));

        	m_yVelocityPerSecond = 0;
    		if (m_xVelocityPerSecond != 0)
    		{
				printf("\nDBUG: MoveVertical Path 2b1");
    			ChangeToState(eSTATE_WALKING);
    		}
    		else
    		{
				printf("\nDBUG: MoveVertical Path 2b2");
    			ChangeToState(eSTATE_STANDING);
    		}
		}
    }
}

const char *TPlayer::StateAsString() const
{
	assert (m_state < eSTATE_COUNT);

	return m_stateStrings[m_state];
}

TPlayer::TPlayer() :
        TMobile(366, 0, 0),
        m_frameIndex(0),
        m_seconds_since_last_frame_change(0.0),
        m_bulletsFlying(0),
        m_ammo(0),
        m_hasTNT(false),
        m_hasDisk(false),
        m_score(0),
        m_animation(eANIM_STANDING_RIGHT),
        m_state(eSTATE_STANDING)
{
	m_stateEnter[eSTATE_STANDING] = &TPlayer::StartStanding;
	m_stateEnter[eSTATE_WALKING ] = &TPlayer::StartWalking;
	m_stateEnter[eSTATE_JUMPING ] = &TPlayer::StartJumping;
	m_stateEnter[eSTATE_FIRING  ] = &TPlayer::FireBullet;
	m_stateEnter[eSTATE_FALLING ] = &TPlayer::StartFalling;

	Reset(0, 0);
}

void TPlayer::StartFalling()
{
	m_yVelocityPerSecond = ACCELERATION_PER_SECOND;
}

void TPlayer::Reset(signed int x, signed int y)
{
	m_frameIndex = (0);
    m_seconds_since_last_frame_change = (0.0);
    m_bulletsFlying = (0);
    m_animation = (eANIM_STANDING_RIGHT);
    m_state = (eSTATE_STANDING);

    m_x = x;
    m_y = y;

    // unscaled pixels
    m_xVelocityPerSecond = 0.0;
    m_yVelocityPerSecond = 0.0;

    m_facing = eFACING_RIGHT;

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
        if ((GLOBALS::player.m_x < m_x) && (GLOBALS::player.Facing() == eFACING_RIGHT)) // player is on left, trying to push right
        {
        	tileXright = (oldX + DrawWidth()) / TILE_WIDTH_PIXELS_UNSCALED;
        	if (!(level1MapData.bounds[tileY * LEVEL_WIDTH_TILES + tileXright] & SOLID_LEFT))
        	{
        		m_x = GLOBALS::player.m_x + GLOBALS::player.DrawWidth();
        	}
        }
        else if ((GLOBALS::player.m_x > m_x) && (GLOBALS::player.Facing() == eFACING_LEFT)) // player is on right, trying to push left
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

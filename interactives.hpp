#ifndef _INTERACTIVES_HPP_
#define _INTERACTIVES_HPP_

#include "sam_shared.hpp"

class TObject
{
public:
    TObject(unsigned int tileID, signed int x, signed int y) : m_x(x), m_y(y), m_tileID(tileID) {};
    virtual ~TObject() {}; // don't need to do anything for this class,
                           // but child classes may have more complex destruction needs

    virtual void Tick(double __attribute__ ((unused)) delta_seconds) {};

    virtual unsigned int TileID() const { return m_tileID; };
    virtual signed int DrawWidth() const { return TILE_WIDTH_PIXELS_UNSCALED; };
    virtual bool CollidedWith(TObject __attribute__ ((unused)) &obj) { return false; };

    // unscaled pixels
    double m_x, m_y;

protected:
    unsigned int m_tileID;
};

class TMobile : public TObject
{
public:
    TMobile(unsigned int tileID, signed int x, signed int y) :
                TObject(tileID, x, y),
                m_xVelocityPerSecond(0),
                m_yVelocityPerSecond(0),
                m_facing(eFACING_RIGHT)
        {};

    TFacing Facing() const { return m_facing; };

protected:
    double m_xVelocityPerSecond, m_yVelocityPerSecond;

    TFacing m_facing;
};

class TPlayer;
typedef void (TPlayer::*TStateRoutine)(void);

class TPlayer : public TMobile
{
public:
    typedef enum
    {
    	eSTATE_STANDING,
    	eSTATE_WALKING,
    	eSTATE_JUMPING,
    	eSTATE_FALLING,
    	eSTATE_FIRING,
    	//eSTATE_DYING,
    	//eSTATE_EXITING_LEVEL,

    	eSTATE_COUNT // ALWAYS LAST - is the number of states in the enum
    } TPlayerState;

    TPlayer();
    void Reset(signed int x, signed int y);

    virtual void Tick(double delta_seconds);

    virtual unsigned int TileID() const;
    virtual signed int DrawWidth() const;

    void ProcessAction(action_t action);

    void BulletDied();
    
    unsigned int Score() const { return m_score; };
    unsigned int Ammo() const { return m_ammo; };
    void AddAmmo(unsigned int shots) { m_ammo += shots; };
    
    const char *StateAsString() const;

    // class constants
    enum
    {
        MAX_Y_VELOCITY_PER_SECOND = 80, // positive means down, negative means up
        MAX_X_VELOCITY_PER_SECOND = 64, //
        ACCELERATION_PER_SECOND   = 80, // positive means down, negative means up
        m_maxBulletsFlying = 1,

        eFRAMES_PER_ANIMATION = 4
    };

private:
    unsigned int m_frameIndex;
    double m_seconds_since_last_frame_change;
    unsigned int m_bulletsFlying;

    typedef enum
    {
        eANIM_STANDING_LEFT = 0,
        eANIM_STANDING_RIGHT,
        eANIM_JUMPING_LEFT,
        eANIM_JUMPING_RIGHT,
        eANIM_SHOOTING_LEFT,
        eANIM_SHOOTING_RIGHT,

        // must be last
        eNUM_PLAYER_ANIMATIONS,
    } TPlayerAnimation;
    
    unsigned int m_ammo;
    bool m_hasTNT;
    bool m_hasDisk;
    unsigned int m_score;

    TPlayerAnimation m_animation;
    TPlayerState m_state;

    void ChangeToState(TPlayerState newState);
    void FireBullet();
    void StartJumping();
    void StartWalking();
    void StartStanding();
    void StartFalling();
    void MoveHorizontal(double deltaSeconds);
    void MoveVertical(double deltaSeconds);

    TStateRoutine m_stateEnter[eSTATE_COUNT];
    TStateRoutine m_stateExit[eSTATE_COUNT];
    TStateRoutine m_stateTick[eSTATE_COUNT];

    static const char *m_stateStrings[eSTATE_COUNT];
    static const unsigned int frames[eNUM_PLAYER_ANIMATIONS][eFRAMES_PER_ANIMATION];
    static const signed int widths[eNUM_PLAYER_ANIMATIONS][eFRAMES_PER_ANIMATION];

};

class TGlasses : public TObject
{
public:
    TGlasses(signed int x, signed int y) : TObject(52, x, y) {};

    virtual bool CollidedWith(TObject &obj);
};

class TSatelliteDish : public TObject
{
public:
	TSatelliteDish(signed int x, signed int y) : TObject(357, x, y), m_frameIndex(0), m_seconds_since_last_frame_change(0.0), m_timesShot(0) {};
    virtual void Tick(double delta_seconds);

    virtual bool CollidedWith(TObject &obj);

    virtual unsigned int TileID() const;
    virtual signed int DrawWidth() const;

private:
	enum /* class-static definitions */
	{
		eFRAMES_PER_ANIMATION = 4
	};

    unsigned int m_frameIndex;
    double m_seconds_since_last_frame_change;
    
    unsigned int m_timesShot;

    static const unsigned int frames[eFRAMES_PER_ANIMATION];
    static const signed int widths[eFRAMES_PER_ANIMATION];
};


class TAmmo : public TObject
{
public:
    TAmmo(signed int x, signed int y) : TObject(354, x, y) {};

    virtual bool CollidedWith(TObject &obj) override;
};


class TPushable : public TObject
{
public:
    TPushable(unsigned int tileID, signed int x, signed int y);

    virtual bool CollidedWith(TObject &obj) override;
};


class TBullet : public TObject
{
public:
    TBullet(signed int x, signed int y, TFacing directionMoving);
    virtual ~TBullet();

    virtual void Tick(double delta_seconds) override;
    virtual signed int DrawWidth() const override { return 7; };
    virtual bool CollidedWith(TObject &obj) override;

private:
    double m_xReal, m_xVelocity;

    TBullet(const TBullet&) = delete; /* disable copy constructor [C++1] */
    TBullet& operator=(const TBullet&) = delete; /* disable assignment operator [C++11] */
};

#endif

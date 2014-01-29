#ifndef _INTERACTIVES_HPP_
#define _INTERACTIVES_HPP_

class TObject
{
public:
    TObject(unsigned int tileID, signed int x, signed int y) : m_x(x), m_y(y), m_tileID(tileID) {};
    virtual ~TObject() {}; // don't need to do anything, but child classes may have more complex destruction needs

    virtual void Tick(double delta_seconds) {};

    virtual unsigned int TileID() const { return m_tileID; };
    virtual signed int DrawWidth() const { return TILE_WIDTH_PIXELS_UNSCALED; };
    virtual bool CollidedWith(TObject &obj) { return false; };

    // unscaled
    signed int m_x, m_y;

protected:
    unsigned int m_tileID;
};

class TMobile : public TObject
{
public:
    TMobile(unsigned int tileID, signed int x, signed int y) : TObject(tileID, x, y), m_xVelocity(0), m_yVelocity(0), m_jumping(false), m_jumpedSoFar(0), m_facing(eFACING_RIGHT) {};
    signed int m_xVelocity, m_yVelocity;

    // if jumping, how many unscaled pixels up has the player moved so far
    // (stop jumping when this reaches the limit of how far to jump)
    bool m_jumping;
    signed int m_jumpedSoFar;

    TFacing m_facing;
};

class TPlayer : public TMobile
{
public:
    TPlayer() : TMobile(366, 0, 0), m_frameIndex(0), m_seconds_since_last_frame_change(0.0), m_ammo(0), m_hasTNT(false), m_hasDisk(false), m_animation(eANIM_STANDING_RIGHT) {};
    virtual void Tick(double delta_seconds);

    virtual unsigned int TileID() const;
    virtual signed int DrawWidth() const;

private:
    unsigned int m_frameIndex;
    double m_seconds_since_last_frame_change;

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

        eFRAMES_PER_ANIMATION = 4
    } TPlayerAnimation;
    
    unsigned int m_ammo;
    bool m_hasTNT;
    bool m_hasDisk;

    TPlayerAnimation m_animation;

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

#endif

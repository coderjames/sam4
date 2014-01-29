#ifndef _level1_H_
#define _level1_H_

typedef struct {
    signed short backTiles[30*40];
    signed short midTiles[30*40];
    signed short frontTiles[30*40];
    signed short bounds[30*40];
    signed short codes[30*40];
} TMapData;

extern TMapData level1MapData;

#endif
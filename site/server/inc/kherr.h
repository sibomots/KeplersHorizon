#ifndef __KH_ERRORS_H__
#define __KH_ERRORS_H__

#define S_OK (0)
#define KH_SUCCEEDED(x) ((x)==(S_OK))
#define KH_FAILED(x) (!KH_SUCCEEDED(x))

#define E_NEG_SHIP_ATTR (1 << 0)
#define E_SR_ON_SYSTEM_SHIP (1 << 1)
#define E_MISSILE_BY_THREE (1 << 2)
#define E_MISSILE_TUBE_MISMATCH (1 << 3)

#endif


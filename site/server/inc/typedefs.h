#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

// Belongs in the typedefs collection
typedef enum : int {
    UNDEFINED,
    LEARNING,
    BASIC,
    ADVANCED,
} ScenarioType;

#ifndef SafeDelete
#define SafeDelete(x) do { if ((x) != NULL) { delete (x); (x) = NULL; }} while(0)
#endif

#ifndef SafeDeleteA
#define SafeDeleteA(x) do { if ((x) != NULL) { delete [] (x); (x) = NULL; }} while(0)
#endif

#endif



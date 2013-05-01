/**CFile****************************************************************

  FileName    [vecWec.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Resizable vector of resizable vectors.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecWec.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vecWec_h
#define ABC__misc__vec__vecWec_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Vec_Wec_t_       Vec_Wec_t;
struct Vec_Wec_t_ 
{
    int              nCap;
    int              nSize;
    Vec_Int_t *      pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// iterators through levels 
#define Vec_WecForEachLevel( vGlob, vVec, i )                                              \
    for ( i = 0; (i < Vec_WecSize(vGlob)) && (((vVec) = Vec_WecEntry(vGlob, i)), 1); i++ )
#define Vec_WecForEachLevelStart( vGlob, vVec, i, LevelStart )                             \
    for ( i = LevelStart; (i < Vec_WecSize(vGlob)) && (((vVec) = Vec_WecEntry(vGlob, i)), 1); i++ )
#define Vec_WecForEachLevelStop( vGlob, vVec, i, LevelStop )                               \
    for ( i = 0; (i < LevelStop) && (((vVec) = Vec_WecEntry(vGlob, i)), 1); i++ )
#define Vec_WecForEachLevelStartStop( vGlob, vVec, i, LevelStart, LevelStop )              \
    for ( i = LevelStart; (i < LevelStop) && (((vVec) = Vec_WecEntry(vGlob, i)), 1); i++ )
#define Vec_WecForEachLevelReverse( vGlob, vVec, i )                                       \
    for ( i = Vec_WecSize(vGlob)-1; (i >= 0) && (((vVec) = Vec_WecEntry(vGlob, i)), 1); i-- )
#define Vec_WecForEachLevelReverseStartStop( vGlob, vVec, i, LevelStart, LevelStop )       \
    for ( i = LevelStart-1; (i >= LevelStop) && (((vVec) = Vec_WecEntry(vGlob, i)), 1); i-- )
#define Vec_WecForEachLevelTwo( vGlob1, vGlob2, vVec1, vVec2, i )                          \
    for ( i = 0; (i < Vec_WecSize(vGlob1)) && (((vVec1) = Vec_WecEntry(vGlob1, i)), 1) && (((vVec2) = Vec_WecEntry(vGlob2, i)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wec_t * Vec_WecAlloc( int nCap )
{
    Vec_Wec_t * p;
    p = ABC_ALLOC( Vec_Wec_t, 1 );
    if ( nCap > 0 && nCap < 8 )
        nCap = 8;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_CALLOC( Vec_Int_t, p->nCap ) : NULL;
    return p;
}
static inline Vec_Wec_t * Vec_WecStart( int nSize )
{
    Vec_Wec_t * p;
    p = Vec_WecAlloc( nSize );
    p->nSize = nSize;
    return p;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WecGrow( Vec_Wec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( Vec_Int_t, p->pArray, nCapMin ); 
    memset( p->pArray + p->nCap, 0, sizeof(Vec_Int_t) * (nCapMin - p->nCap) );
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_WecEntry( Vec_Wec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray + i;
}
static inline Vec_Int_t * Vec_WecEntryLast( Vec_Wec_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray + p->nSize - 1;
}
static inline int Vec_WecEntryEntry( Vec_Wec_t * p, int i, int k )
{
    return Vec_IntEntry( Vec_WecEntry(p, i), k );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_WecCap( Vec_Wec_t * p )
{
    return p->nCap;
}
static inline int Vec_WecSize( Vec_Wec_t * p )
{
    return p->nSize;
}
static inline int Vec_WecLevelSize( Vec_Wec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return Vec_IntSize( p->pArray + i );
}
static inline int Vec_WecSizeSize( Vec_Wec_t * p )
{
    Vec_Int_t * vVec;
    int i, Counter = 0;
    Vec_WecForEachLevel( p, vVec, i )
        Counter += Vec_IntSize(vVec);
    return Counter;
}
static inline int Vec_WecSizeUsed( Vec_Wec_t * p )
{
    Vec_Int_t * vVec;
    int i, Counter = 0;
    Vec_WecForEachLevel( p, vVec, i )
        Counter += (int)(Vec_IntSize(vVec) > 0);
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WecShrink( Vec_Wec_t * p, int nSizeNew )
{
    assert( p->nSize >= nSizeNew );
    p->nSize = nSizeNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WecClear( Vec_Wec_t * p )
{
    Vec_Int_t * vVec;
    int i;
    Vec_WecForEachLevel( p, vVec, i )
        Vec_IntClear( vVec );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WecPush( Vec_Wec_t * p, int Level, int Entry )
{
    if ( p->nSize < Level + 1 )
    {
        Vec_WecGrow( p, Level + 1 );
        p->nSize = Level + 1;
    }
    Vec_IntPush( Vec_WecEntry(p, Level), Entry );
}
static inline Vec_Int_t * Vec_WecPushLevel( Vec_Wec_t * p )
{
    Vec_WecGrow( p, ++p->nSize );
    return Vec_WecEntryLast( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Vec_WecMemory( Vec_Wec_t * p )
{
    int i;
    double Mem;
    if ( p == NULL )  return 0.0;
    Mem = sizeof(Vec_Int_t) * Vec_WecCap(p);
    for ( i = 0; i < p->nSize; i++ )
        Mem += sizeof(int) * Vec_IntCap( Vec_WecEntry(p, i) );
    return Mem;
}

/**Function*************************************************************

  Synopsis    [Frees the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WecFree( Vec_Wec_t * p )
{
    Vec_Int_t * vVec;
    int i;
    Vec_WecForEachLevel( p, vVec, i )
        ABC_FREE( vVec->pArray );
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}
static inline void Vec_WecFreeP( Vec_Wec_t ** p )
{
    if ( *p == NULL )
        return;
    Vec_WecFree( *p );
    *p = NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WecPushUnique( Vec_Wec_t * p, int Level, int Entry )
{
    if ( p->nSize < Level + 1 )
        Vec_WecPush( p, Level, Entry );
    else
        Vec_IntPushUnique( Vec_WecEntry(p, Level), Entry );
}

/**Function*************************************************************

  Synopsis    [Frees the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wec_t * Vec_WecDup( Vec_Wec_t * p )
{
    Vec_Wec_t * vNew;
    Vec_Int_t * vVec;
    int i, k, Entry;
    vNew = Vec_WecAlloc( Vec_WecSize(p) );
    Vec_WecForEachLevel( p, vVec, i )
        Vec_IntForEachEntry( vVec, Entry, k )
            Vec_WecPush( vNew, i, Entry );
    return vNew;
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two arrays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_WecSortCompare1( Vec_Int_t * p1, Vec_Int_t * p2 )
{
    if ( Vec_IntSize(p1) < Vec_IntSize(p2) )
        return -1;
    if ( Vec_IntSize(p1) > Vec_IntSize(p2) ) 
        return 1;
    return 0; 
}
static int Vec_WecSortCompare2( Vec_Int_t * p1, Vec_Int_t * p2 )
{
    if ( Vec_IntSize(p1) > Vec_IntSize(p2) )
        return -1;
    if ( Vec_IntSize(p1) < Vec_IntSize(p2) ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WecSort( Vec_Wec_t * p, int fReverse )
{
    if ( fReverse ) 
        qsort( (void *)p->pArray, p->nSize, sizeof(Vec_Int_t), 
                (int (*)(const void *, const void *)) Vec_WecSortCompare2 );
    else
        qsort( (void *)p->pArray, p->nSize, sizeof(Vec_Int_t), 
                (int (*)(const void *, const void *)) Vec_WecSortCompare1 );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_WecSortCompare3( Vec_Int_t * p1, Vec_Int_t * p2 )
{
    if ( Vec_IntEntry(p1,0) < Vec_IntEntry(p2,0) )
        return -1;
    if ( Vec_IntEntry(p1,0) > Vec_IntEntry(p2,0) ) 
        return 1;
    return 0; 
}
static int Vec_WecSortCompare4( Vec_Int_t * p1, Vec_Int_t * p2 )
{
    if ( Vec_IntEntry(p1,0) > Vec_IntEntry(p2,0) )
        return -1;
    if ( Vec_IntEntry(p1,0) < Vec_IntEntry(p2,0) ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WecSortByFirstInt( Vec_Wec_t * p, int fReverse )
{
    if ( fReverse ) 
        qsort( (void *)p->pArray, p->nSize, sizeof(Vec_Int_t), 
                (int (*)(const void *, const void *)) Vec_WecSortCompare4 );
    else
        qsort( (void *)p->pArray, p->nSize, sizeof(Vec_Int_t), 
                (int (*)(const void *, const void *)) Vec_WecSortCompare3 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WecPrint( Vec_Wec_t * p, int fSkipSingles )
{
    Vec_Int_t * vVec;
    int i, k, Entry;
    Vec_WecForEachLevel( p, vVec, i )
    {
        if ( fSkipSingles && Vec_IntSize(vVec) == 1 )
            continue;
        printf( " %4d : {", i );
        Vec_IntForEachEntry( vVec, Entry, k )
            printf( " %d", Entry );
        printf( " }\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Derives the set of equivalence classes.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wec_t * Vec_WecCreateClasses( Vec_Int_t * vMap )
{
    Vec_Wec_t * vClasses;
    int i, Entry;
    vClasses = Vec_WecStart( Vec_IntFindMax(vMap) + 1 );
    Vec_IntForEachEntry( vMap, Entry, i )
        Vec_WecPush( vClasses, Entry, i );
    return vClasses;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_WecCountNonTrivial( Vec_Wec_t * p, int * pnUsed )
{
    Vec_Int_t * vClass;
    int i, nClasses = 0;
    *pnUsed = 0;
    Vec_WecForEachLevel( p, vClass, i )
    {
        if ( Vec_IntSize(vClass) < 2 )
            continue;
        nClasses++;
        (*pnUsed) += Vec_IntSize(vClass);
    }
    return nClasses;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_WecCollectFirsts( Vec_Wec_t * p )
{
    Vec_Int_t * vFirsts, * vLevel;
    int i;
    vFirsts = Vec_IntAlloc( Vec_WecSize(p) );
    Vec_WecForEachLevel( p, vLevel, i )
        if ( Vec_IntSize(vLevel) > 0 )
            Vec_IntPush( vFirsts, Vec_IntEntry(vLevel, 0) );
    return vFirsts;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Ptr_t * Vec_WecConvertToVecPtr( Vec_Wec_t * p )
{
    Vec_Ptr_t * vCopy;
    Vec_Int_t * vLevel;
    int i;
    vCopy = Vec_PtrAlloc( Vec_WecSize(p) );
    Vec_WecForEachLevel( p, vLevel, i )
        Vec_PtrPush( vCopy, Vec_IntDup(vLevel) );
    return vCopy;
}

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

/**CFile****************************************************************

  FileName    [cecSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Detection of structural isomorphism.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecSat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/gia/gia.h"
#include "misc/util/utilTruth.h"
#include "sat/satoko/satoko.h"
#include "sat/satoko/solver.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// sweeping manager
typedef struct Cec2_Par_t_ Cec2_Par_t;
struct Cec2_Par_t_
{
    int              nSimWords;     // simulation words
    int              nSimRounds;    // simulation rounds
    int              nConfLimit;    // SAT solver conflict limit
    int              fIsMiter;      // this is a miter
    int              fVeryVerbose;  // verbose stats
    int              fVerbose;      // verbose stats
};

// SAT solving manager
typedef struct Cec2_Man_t_ Cec2_Man_t;
struct Cec2_Man_t_
{
    Cec2_Par_t *     pPars;          // parameters
    Gia_Man_t *      pAig;           // user's AIG
    Gia_Man_t *      pNew;           // internal AIG
    // SAT solving
    satoko_t *       pSat;           // SAT solver
    Vec_Ptr_t *      vFrontier;      // CNF construction
    Vec_Ptr_t *      vFanins;        // CNF construction
    Vec_Wrd_t *      vSims;          // CI simulation info
    Vec_Int_t *      vNodesNew;      // nodes
    Vec_Int_t *      vObjSatPairs;   // nodes
    Vec_Int_t *      vCexTriples;    // nodes
};

static inline int    Cec2_ObjSatId( Gia_Man_t * p, Gia_Obj_t * pObj )             { return Gia_ObjCopyArray(p, Gia_ObjId(p, pObj));                                                     }
static inline int    Cec2_ObjSetSatId( Gia_Man_t * p, Gia_Obj_t * pObj, int Num ) { assert(Cec2_ObjSatId(p, pObj) == -1); Gia_ObjSetCopyArray(p, Gia_ObjId(p, pObj), Num); return Num;  }
static inline void   Cec2_ObjCleanSatId( Gia_Man_t * p, Gia_Obj_t * pObj )        { assert(Cec2_ObjSatId(p, pObj) != -1); Gia_ObjSetCopyArray(p, Gia_ObjId(p, pObj), -1);               }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sets parameter defaults.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec2_SetDefaultParams( Cec2_Par_t * p )
{
    memset( p, 0, sizeof(Cec2_Par_t) );
    p->nSimWords      =       8;    // simulation words
    p->nSimRounds     =       4;    // simulation rounds
    p->nConfLimit     =    1000;    // conflict limit at a node
    p->fIsMiter       =       0;    // this is a miter
    p->fVeryVerbose   =       0;    // verbose stats
    p->fVerbose       =       1;    // verbose stats
}  

/**Function*************************************************************

  Synopsis    [Adds clauses to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec2_AddClausesMux( Gia_Man_t * p, Gia_Obj_t * pNode, satoko_t * pSat )
{
    int fPolarFlip = 0;
    Gia_Obj_t * pNodeI, * pNodeT, * pNodeE;
    int pLits[4], RetValue, VarF, VarI, VarT, VarE, fCompT, fCompE;

    assert( !Gia_IsComplement( pNode ) );
    assert( pNode->fMark0 );
    // get nodes (I = if, T = then, E = else)
    pNodeI = Gia_ObjRecognizeMux( pNode, &pNodeT, &pNodeE );
    // get the variable numbers
    VarF = Cec2_ObjSatId(p, pNode);
    VarI = Cec2_ObjSatId(p, pNodeI);
    VarT = Cec2_ObjSatId(p, Gia_Regular(pNodeT));
    VarE = Cec2_ObjSatId(p, Gia_Regular(pNodeE));
    // get the complementation flags
    fCompT = Gia_IsComplement(pNodeT);
    fCompE = Gia_IsComplement(pNodeE);

    // f = ITE(i, t, e)

    // i' + t' + f
    // i' + t  + f'
    // i  + e' + f
    // i  + e  + f'

    // create four clauses
    pLits[0] = Abc_Var2Lit(VarI, 1);
    pLits[1] = Abc_Var2Lit(VarT, 1^fCompT);
    pLits[2] = Abc_Var2Lit(VarF, 0);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = satoko_add_clause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarI, 1);
    pLits[1] = Abc_Var2Lit(VarT, 0^fCompT);
    pLits[2] = Abc_Var2Lit(VarF, 1);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = satoko_add_clause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarI, 0);
    pLits[1] = Abc_Var2Lit(VarE, 1^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 0);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = satoko_add_clause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarI, 0);
    pLits[1] = Abc_Var2Lit(VarE, 0^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 1);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = satoko_add_clause( pSat, pLits, 3 );
    assert( RetValue );

    // two additional clauses
    // t' & e' -> f'
    // t  & e  -> f 

    // t  + e   + f'
    // t' + e'  + f 

    if ( VarT == VarE )
    {
//        assert( fCompT == !fCompE );
        return;
    }

    pLits[0] = Abc_Var2Lit(VarT, 0^fCompT);
    pLits[1] = Abc_Var2Lit(VarE, 0^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 1);
    if ( fPolarFlip )
    {
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = satoko_add_clause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarT, 1^fCompT);
    pLits[1] = Abc_Var2Lit(VarE, 1^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 0);
    if ( fPolarFlip )
    {
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = satoko_add_clause( pSat, pLits, 3 );
    assert( RetValue );
}
void Cec2_AddClausesSuper( Gia_Man_t * p, Gia_Obj_t * pNode, Vec_Ptr_t * vSuper, satoko_t * pSat )
{
    int fPolarFlip = 0;
    Gia_Obj_t * pFanin;
    int * pLits, nLits, RetValue, i;
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsAnd( pNode ) );
    // create storage for literals
    nLits = Vec_PtrSize(vSuper) + 1;
    pLits = ABC_ALLOC( int, nLits );
    // suppose AND-gate is A & B = C
    // add !A => !C   or   A + !C
    Vec_PtrForEachEntry( Gia_Obj_t *, vSuper, pFanin, i )
    {
        pLits[0] = Abc_Var2Lit(Cec2_ObjSatId(p, Gia_Regular(pFanin)), Gia_IsComplement(pFanin));
        pLits[1] = Abc_Var2Lit(Cec2_ObjSatId(p, pNode), 1);
        if ( fPolarFlip )
        {
            if ( Gia_Regular(pFanin)->fPhase )  pLits[0] = Abc_LitNot( pLits[0] );
            if ( pNode->fPhase )                pLits[1] = Abc_LitNot( pLits[1] );
        }
        RetValue = satoko_add_clause( pSat, pLits, 2 );
        assert( RetValue );
    }
    // add A & B => C   or   !A + !B + C
    Vec_PtrForEachEntry( Gia_Obj_t *, vSuper, pFanin, i )
    {
        pLits[i] = Abc_Var2Lit(Cec2_ObjSatId(p, Gia_Regular(pFanin)), !Gia_IsComplement(pFanin));
        if ( fPolarFlip )
        {
            if ( Gia_Regular(pFanin)->fPhase )  pLits[i] = Abc_LitNot( pLits[i] );
        }
    }
    pLits[nLits-1] = Abc_Var2Lit(Cec2_ObjSatId(p, pNode), 0);
    if ( fPolarFlip )
    {
        if ( pNode->fPhase )  pLits[nLits-1] = Abc_LitNot( pLits[nLits-1] );
    }
    RetValue = satoko_add_clause( pSat, pLits, nLits );
    assert( RetValue );
    ABC_FREE( pLits );
}

/**Function*************************************************************

  Synopsis    [Adds clauses and returns CNF variable of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec2_CollectSuper_rec( Gia_Obj_t * pObj, Vec_Ptr_t * vSuper, int fFirst, int fUseMuxes )
{
    // if the new node is complemented or a PI, another gate begins
    if ( Gia_IsComplement(pObj) || Gia_ObjIsCi(pObj) || 
         (!fFirst && Gia_ObjValue(pObj) > 1) || 
         (fUseMuxes && pObj->fMark0) )
    {
        Vec_PtrPushUnique( vSuper, pObj );
        return;
    }
    // go through the branches
    Cec2_CollectSuper_rec( Gia_ObjChild0(pObj), vSuper, 0, fUseMuxes );
    Cec2_CollectSuper_rec( Gia_ObjChild1(pObj), vSuper, 0, fUseMuxes );
}
void Cec2_CollectSuper( Gia_Obj_t * pObj, int fUseMuxes, Vec_Ptr_t * vSuper )
{
    assert( !Gia_IsComplement(pObj) );
    assert( !Gia_ObjIsCi(pObj) );
    Vec_PtrClear( vSuper );
    Cec2_CollectSuper_rec( pObj, vSuper, 1, fUseMuxes );
}
void Cec2_ObjAddToFrontier( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Ptr_t * vFrontier, satoko_t * pSat )
{
    assert( !Gia_IsComplement(pObj) );
    assert( !Gia_ObjIsConst0(pObj) );
    if ( Cec2_ObjSatId(p, pObj) >= 0 )
        return;
    assert( Cec2_ObjSatId(p, pObj) == -1 );
    Cec2_ObjSetSatId( p, pObj, satoko_add_variable(pSat, 0) );
    if ( Gia_ObjIsAnd(pObj) )
        Vec_PtrPush( vFrontier, pObj );
}
int Cec2_ObjGetCnfVar( Cec2_Man_t * p, int iObj )
{ 
    Gia_Obj_t * pNode, * pFanin;
    Gia_Obj_t * pObj = Gia_ManObj(p->pNew, iObj);
    int i, k, fUseMuxes = 1;
    // quit if CNF is ready
    if ( Cec2_ObjSatId(p->pNew,pObj) >= 0 )
        return Cec2_ObjSatId(p->pNew,pObj);
    assert( iObj > 0 );
    if ( Gia_ObjIsCi(pObj) )
        return Cec2_ObjSetSatId( p->pNew, pObj, satoko_add_variable(p->pSat, 0) );
    assert( Gia_ObjIsAnd(pObj) );
    // start the frontier
    Vec_PtrClear( p->vFrontier );
    Cec2_ObjAddToFrontier( p->pNew, pObj, p->vFrontier, p->pSat );
    // explore nodes in the frontier
    Vec_PtrForEachEntry( Gia_Obj_t *, p->vFrontier, pNode, i )
    {
        // create the supergate
        assert( Cec2_ObjSatId(p->pNew,pNode) >= 0 );
        if ( fUseMuxes && pNode->fMark0 )
        {
            Vec_PtrClear( p->vFanins );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin0( Gia_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin0( Gia_ObjFanin1(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin1( Gia_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin1( Gia_ObjFanin1(pNode) ) );
            Vec_PtrForEachEntry( Gia_Obj_t *, p->vFanins, pFanin, k )
                Cec2_ObjAddToFrontier( p->pNew, Gia_Regular(pFanin), p->vFrontier, p->pSat );
            Cec2_AddClausesMux( p->pNew, pNode, p->pSat );
        }
        else
        {
            Cec2_CollectSuper( pNode, fUseMuxes, p->vFanins );
            Vec_PtrForEachEntry( Gia_Obj_t *, p->vFanins, pFanin, k )
                Cec2_ObjAddToFrontier( p->pNew, Gia_Regular(pFanin), p->vFrontier, p->pSat );
            Cec2_AddClausesSuper( p->pNew, pNode, p->vFanins, p->pSat );
        }
        assert( Vec_PtrSize(p->vFanins) > 1 );
    }
    return Cec2_ObjSatId(p->pNew,pObj);
}



/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Cec2_ObjSim( Gia_Man_t * p, int iObj )
{
    return Vec_WrdEntryP( p->vSims, p->nSimWords * iObj );
}
static inline void Cec2_ObjSimSetInputBit( Gia_Man_t * p, int iObj, int Bit )
{
    word * pSim = Cec2_ObjSim( p, iObj );
    if ( Abc_InfoHasBit( (unsigned*)pSim, p->iPatsPi ) != Bit )
        Abc_InfoXorBit( (unsigned*)pSim, p->iPatsPi );
}
static inline void Cec2_ObjSimRo( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSimRo = Cec2_ObjSim( p, iObj );
    word * pSimRi = Cec2_ObjSim( p, Gia_ObjRoToRiId(p, iObj) );
    for ( w = 0; w < p->nSimWords; w++ )
        pSimRo[w] = pSimRi[w];
}
static inline void Cec2_ObjSimCo( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSimCo  = Cec2_ObjSim( p, iObj );
    word * pSimDri = Cec2_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] = ~pSimDri[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] =  pSimDri[w];
}
static inline void Cec2_ObjSimAnd( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSim  = Cec2_ObjSim( p, iObj );
    word * pSim0 = Cec2_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSim1 = Cec2_ObjSim( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & ~pSim1[w];
    else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & pSim1[w];
    else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & ~pSim1[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & pSim1[w];
}
static inline int Cec2_ObjSimEqual( Gia_Man_t * p, int iObj0, int iObj1 )
{
    int w;
    word * pSim0 = Cec2_ObjSim( p, iObj0 );
    word * pSim1 = Cec2_ObjSim( p, iObj1 );
    if ( (pSim0[0] & 1) == (pSim1[0] & 1) )
    {
        for ( w = 0; w < p->nSimWords; w++ )
            if ( pSim0[w] != pSim1[w] )
                return 0;
        return 1;
    }
    else
    {
        for ( w = 0; w < p->nSimWords; w++ )
            if ( pSim0[w] != ~pSim1[w] )
                return 0;
        return 1;
    }
}
static inline void Cec2_ObjSimCi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSim = Cec2_ObjSim( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSim[w] = Gia_ManRandomW( 0 );
    pSim[0] <<= 1;
}
void Cec2_ManSimulateCis( Gia_Man_t * p )
{
    int i, Id;
    Gia_ManForEachCiId( p, Id, i )
        Cec2_ObjSimCi( p, Id );
    p->iPatsPi = 1;
}
Abc_Cex_t * Cec2_ManDeriveCex( Gia_Man_t * p, int iOut, int iPat )
{
    Abc_Cex_t * pCex;
    int i, Id;
    pCex = Abc_CexAlloc( 0, Gia_ManCiNum(p), 1 );
    pCex->iPo = iOut;
    if ( iPat == -1 )
        return pCex;
    Gia_ManForEachCiId( p, Id, i )
        if ( Abc_InfoHasBit((unsigned *)Cec2_ObjSim(p, Id), iPat) )
            Abc_InfoSetBit( pCex->pData, i );
    return pCex;
}
int Cec2_ManSimulateCos( Gia_Man_t * p )
{
    int i, Id;
    // check outputs and generate CEX if they fail
    Gia_ManForEachCoId( p, Id, i )
    {
        Cec2_ObjSimCo( p, Id );
        if ( Cec2_ObjSimEqual(p, Id, 0) )
            continue;
        p->pCexSeq = Cec2_ManDeriveCex( p, i, Abc_TtFindFirstBit2(Cec2_ObjSim(p, Id), p->nSimWords) );
        return 0;
    }
    return 1;
}
void Cec2_ManSaveCis( Gia_Man_t * p )
{
    int w, i, Id;
    assert( p->vSimsPi != NULL );
    for ( w = 0; w < p->nSimWords; w++ )
        Gia_ManForEachCiId( p, Id, i )
            Vec_WrdPush( p->vSimsPi, Cec2_ObjSim(p, Id)[w] );
}
void Cec2_ManSimulate( Gia_Man_t * p, Vec_Int_t * vTriples )
{
    extern void Cec2_ManSimClassRefineOne( Gia_Man_t * p, int iRepr );
    Gia_Obj_t * pObj; 
    int i, iRepr, iObj, Entry;
    //Cec2_ManSaveCis( p );
    Gia_ManForEachAnd( p, pObj, i )
        Cec2_ObjSimAnd( p, i );
    if ( p->pReprs == NULL )
        return;
    if ( vTriples )
    {
        Vec_IntForEachEntryTriple( vTriples, iRepr, iObj, Entry, i )
        {
            word * pSim0 = Cec2_ObjSim( p, iRepr );
            word * pSim1 = Cec2_ObjSim( p, iObj );
            int iPat     = Abc_Lit2Var(Entry);
            int fPhase   = Abc_LitIsCompl(Entry);
            if ( (fPhase ^ Abc_InfoHasBit((unsigned *)pSim0, iPat)) == Abc_InfoHasBit((unsigned *)pSim1, iPat) )
                printf( "ERROR:  Pattern %d did not disprove pair %d and %d.\n", iPat, iRepr, iObj );
        }
    }
    Gia_ManForEachClass0( p, i )
        Cec2_ManSimClassRefineOne( p, i );
}
void Cec2_ManSimAlloc( Gia_Man_t * p, int nWords )
{
    Vec_WrdFreeP( &p->vSims );
    Vec_WrdFreeP( &p->vSimsPi );
    p->vSims   = Vec_WrdStart( Gia_ManObjNum(p) * nWords );
    p->vSimsPi = Vec_WrdAlloc( Gia_ManCiNum(p) * nWords * 4 ); // storage for CI patterns
    p->nSimWords = nWords;
}


/**Function*************************************************************

  Synopsis    [Computes hash key of the simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec2_ManSimHashKey( word * pSim, int nSims, int nTableSize )
{
    static int s_Primes[16] = { 
        1291, 1699, 1999, 2357, 2953, 3313, 3907, 4177, 
        4831, 5147, 5647, 6343, 6899, 7103, 7873, 8147 };
    unsigned uHash = 0, * pSimU = (unsigned *)pSim;
    int i, nSimsU = 2 * nSims;
    if ( pSimU[0] & 1 )
        for ( i = 0; i < nSimsU; i++ )
            uHash ^= ~pSimU[i] * s_Primes[i & 0xf];
    else
        for ( i = 0; i < nSimsU; i++ )
            uHash ^= pSimU[i] * s_Primes[i & 0xf];
    return (int)(uHash % nTableSize);

}

/**Function*************************************************************

  Synopsis    [Creating initial equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec2_ManSimClassRefineOne( Gia_Man_t * p, int iRepr )
{
    int iObj, iPrev = iRepr, iPrev2, iRepr2;
    Gia_ClassForEachObj1( p, iRepr, iRepr2 )
        if ( Cec2_ObjSimEqual(p, iRepr, iRepr2) )
            iPrev = iRepr2;
        else
            break;
    if ( iRepr2 <= 0 ) // no refinement
        return;
    // relink remaining nodes of the class
    // nodes that are equal to iRepr, remain in the class of iRepr
    // nodes that are not equal to iRepr, move to the class of iRepr2
    Gia_ObjSetRepr( p, iRepr2, GIA_VOID );
    iPrev2 = iRepr2;
    for ( iObj = Gia_ObjNext(p, iRepr2); iObj > 0; iObj = Gia_ObjNext(p, iObj) )
    {
        if ( Cec2_ObjSimEqual(p, iRepr, iObj) ) // remains with iRepr
        {
            Gia_ObjSetNext( p, iPrev, iObj );
            iPrev = iObj;
        }
        else // moves to iRepr2
        {
            Gia_ObjSetRepr( p, iObj, iRepr2 );
            Gia_ObjSetNext( p, iPrev2, iObj );
            iPrev2 = iObj;
        }
    }
    Gia_ObjSetNext( p, iPrev, -1 );
    Gia_ObjSetNext( p, iPrev2, -1 );
}
void Cec2_ManCreateClasses( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int nWords = p->nSimWords;
    int * pTable, nTableSize, i, Key;
    // allocate representation
    ABC_FREE( p->pReprs );
    ABC_FREE( p->pNexts );
    p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
    p->pNexts = ABC_FALLOC( int, Gia_ManObjNum(p) );
    // hash each node by its simulation info
    nTableSize = Abc_PrimeCudd( Gia_ManObjNum(p) );
    pTable = ABC_FALLOC( int, nTableSize );
    Gia_ManForEachObj( p, pObj, i )
    {
        p->pReprs[i].iRepr = GIA_VOID;
        if ( Gia_ObjIsCo(pObj) )
            continue;
        Key = Cec2_ManSimHashKey( Cec2_ObjSim(p, i), nWords, nTableSize );
        assert( Key >= 0 && Key < nTableSize );
        if ( pTable[Key] == -1 )
            pTable[Key] = i;
        else
            Gia_ObjSetRepr( p, i, pTable[Key] );
    }
    // create classes
    for ( i = Gia_ManObjNum(p) - 1; i >= 0; i-- )
    {
        int iRepr = Gia_ObjRepr(p, i);
        if ( iRepr == GIA_VOID )
            continue;
        Gia_ObjSetNext( p, i, Gia_ObjNext(p, iRepr) );
        Gia_ObjSetNext( p, iRepr, i );
    }
    ABC_FREE( pTable );
    Gia_ManForEachClass0( p, i )
        Cec2_ManSimClassRefineOne( p, i );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec2_Man_t * Cec2_ManCreate( Gia_Man_t * pAig, Cec2_Par_t * pPars )
{
    Cec2_Man_t * p;
    Gia_Obj_t * pObj; int i;
    //assert( Gia_ManRegNum(pAig) == 0 );
    p = ABC_CALLOC( Cec2_Man_t, 1 );
    memset( p, 0, sizeof(Cec2_Man_t) );
    p->pPars        = pPars;
    p->pAig         = pAig;
    // create new manager
    p->pNew         = Gia_ManStart( Gia_ManObjNum(pAig) );
    Gia_ManFillValue( pAig );
    Gia_ManConst0(pAig)->Value = 0;
    Gia_ManForEachCi( pAig, pObj, i )
        pObj->Value = Gia_ManAppendCi( p->pNew );
    Gia_ManHashAlloc( p->pNew );
    Vec_IntFill( &p->pNew->vCopies, Gia_ManObjNum(p->pNew), -1 );
    // SAT solving
    p->pSat         = satoko_create();
    p->vFrontier    = Vec_PtrAlloc( 1000 );
    p->vFanins      = Vec_PtrAlloc( 100 );
    p->vNodesNew    = Vec_IntAlloc( 100 );
    p->vObjSatPairs = Vec_IntAlloc( 100 );
    p->vCexTriples  = Vec_IntAlloc( 100 );
    // remember pointer to the solver in the AIG manager
    pAig->pData     = p->pSat;
    return p;
}
void Cec2_ManDestroy( Cec2_Man_t * p )
{
    Vec_WrdFreeP( &p->pAig->vSims );
    //Vec_WrdFreeP( &p->pAig->vSimsPi );
    Gia_ManCleanMark01( p->pAig );
    satoko_destroy( p->pSat );
    Gia_ManStopP( &p->pNew );
    Vec_PtrFreeP( &p->vFrontier );
    Vec_PtrFreeP( &p->vFanins );
    Vec_IntFreeP( &p->vNodesNew );
    Vec_IntFreeP( &p->vObjSatPairs );
    Vec_IntFreeP( &p->vCexTriples );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Verify counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec2_ManVerify_rec( Gia_Man_t * p, int iObj, satoko_t * pSat )
{
    int Value0, Value1;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( iObj == 0 ) return 0;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return pObj->fMark1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return pObj->fMark1 = var_polarity(pSat, Cec2_ObjSatId(p, pObj)) == LIT_TRUE;
    assert( Gia_ObjIsAnd(pObj) );
    Value0 = Cec2_ManVerify_rec( p, Gia_ObjFaninId0(pObj, iObj), pSat ) ^ Gia_ObjFaninC0(pObj);
    Value1 = Cec2_ManVerify_rec( p, Gia_ObjFaninId1(pObj, iObj), pSat ) ^ Gia_ObjFaninC1(pObj);
    return pObj->fMark1 = Value0 & Value1;
}
void Cec2_ManVerify( Gia_Man_t * p, int iObj0, int iObj1, int fPhase, satoko_t * pSat )
{
//    int val0 = var_polarity(pSat, Cec2_ObjSatId(p, Gia_ManObj(p, iObj0))) == LIT_TRUE;
//    int val1 = var_polarity(pSat, Cec2_ObjSatId(p, Gia_ManObj(p, iObj1))) == LIT_TRUE;
    int Value0, Value1;
    Gia_ManIncrementTravId( p );
    Value0 = Cec2_ManVerify_rec( p, iObj0, pSat );
    Value1 = Cec2_ManVerify_rec( p, iObj1, pSat );
    if ( (Value0 ^ Value1) == fPhase )
        printf( "CEX verification FAILED for obj %d and obj %d.\n", iObj0, iObj1 );
//    else
//        printf( "CEX verification succeeded for obj %d and obj %d.\n", iObj0, iObj1 );;
}

/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec2_ManCollect_rec( Cec2_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p->pNew, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p->pNew, iObj);
    pObj = Gia_ManObj( p->pNew, iObj );
    if ( Cec2_ObjSatId(p->pNew, pObj) >= 0 )
        Vec_IntPush( p->vNodesNew, iObj );
    if ( !iObj )
        return;
    if ( Gia_ObjIsAnd(pObj) )
    {
        Cec2_ManCollect_rec( p, Gia_ObjFaninId0(pObj, iObj) );
        Cec2_ManCollect_rec( p, Gia_ObjFaninId1(pObj, iObj) );
    }
    else
    {
        assert( Cec2_ObjSatId(p->pNew, pObj) >= 0 );
        Vec_IntPushTwo( p->vObjSatPairs, Gia_ManCiIdToId(p->pAig, Gia_ObjCioId(pObj)), Cec2_ObjSatId(p->pNew, pObj) ); // SAT var
    }
}
int Cec2_ManSolveTwo( Cec2_Man_t * p, int iObj0, int iObj1, int fPhase )
{
    Gia_Obj_t * pObj;
    int status, i, iVar0, iVar1;
	if (iObj1 < iObj0) 
        iObj1 ^= iObj0, iObj0 ^= iObj1, iObj1 ^= iObj0;
    assert( iObj0 < iObj1 );
    assert( solver_varnum(p->pSat) == 0 );
    if ( !iObj0 )
        Cec2_ObjSetSatId( p->pNew, Gia_ManConst0(p->pNew), satoko_add_variable(p->pSat, 0) );
    iVar0 = Cec2_ObjGetCnfVar( p, iObj0 );
    iVar1 = Cec2_ObjGetCnfVar( p, iObj1 );
    // collect inputs and internal nodes
    Vec_IntClear( p->vNodesNew );
    Vec_IntClear( p->vObjSatPairs );
    Gia_ManIncrementTravId( p->pNew );
    Cec2_ManCollect_rec( p, iObj0 );
    Cec2_ManCollect_rec( p, iObj1 );
//printf( "%d ", Vec_IntSize(p->vNodesNew) );
    // solve direct
    satoko_assump_push( p->pSat, Abc_Var2Lit(iVar0, 1) );
    satoko_assump_push( p->pSat, Abc_Var2Lit(iVar1, fPhase) );
    status = satoko_solve( p->pSat );
    satoko_assump_pop( p->pSat );
    satoko_assump_pop( p->pSat );
    if ( status == SATOKO_UNSAT && iObj0 > 0 )
    {
        // solve reverse
        satoko_assump_push( p->pSat, Abc_Var2Lit(iVar0, 0) );
        satoko_assump_push( p->pSat, Abc_Var2Lit(iVar1, !fPhase) );
        status = satoko_solve( p->pSat );
        satoko_assump_pop( p->pSat );
        satoko_assump_pop( p->pSat );
    }
    //if ( status == SATOKO_SAT )
    //    Cec2_ManVerify( p->pNew, iObj0, iObj1, fPhase, p->pSat );
    Gia_ManForEachObjVec( p->vNodesNew, p->pNew, pObj, i )
        Cec2_ObjCleanSatId( p->pNew, pObj );
    return status;
}
int Cec2_ManSweepNode( Cec2_Man_t * p, int iObj )
{
    int i, IdAig, IdSat, status, RetValue = 1;
    Gia_Obj_t * pObj = Gia_ManObj( p->pAig, iObj );
    Gia_Obj_t * pRepr = Gia_ObjReprObj( p->pAig, iObj );
    int fCompl = Abc_LitIsCompl(pObj->Value) ^ Abc_LitIsCompl(pRepr->Value) ^ pObj->fPhase ^ pRepr->fPhase;
    status = Cec2_ManSolveTwo( p, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl );
    if ( status == SATOKO_SAT )
    {
        p->pAig->iPatsPi = (p->pAig->iPatsPi == 64 * p->pAig->nSimWords - 1) ? 1 : p->pAig->iPatsPi + 1;
        assert( p->pAig->iPatsPi > 0 && p->pAig->iPatsPi < 64 * p->pAig->nSimWords );
        Vec_IntForEachEntryDouble( p->vObjSatPairs, IdAig, IdSat, i )
            Cec2_ObjSimSetInputBit( p->pAig, IdAig, var_polarity(p->pSat, IdSat) == LIT_TRUE );
        RetValue = 0;
    }
    else if ( status == SATOKO_UNSAT )
    {
        pObj->Value = Abc_LitNotCond( pRepr->Value, fCompl );
        Gia_ObjSetProved( p->pAig, iObj );
    }
    else 
    {
        assert( status == SATOKO_UNDEC );
        Gia_ObjSetFailed( p->pAig, iObj );
        assert( 0 );
    }
    satoko_rollback( p->pSat );
    p->pSat->stats.n_conflicts = 0;
    return RetValue;
}
int Cec2_ManPerformSweeping( Gia_Man_t * p, Cec2_Par_t * pPars )
{
    Cec2_Man_t * pMan; 
    Gia_Obj_t * pObj, * pRepr, * pObjNew; 
    int i, Iter, fDisproved = 1;

    // check if any output trivially fails under all-0 pattern
    Gia_ManSetPhase( p );
    if ( pPars->fIsMiter ) 
    {
        Gia_ManForEachCo( p, pObj, i )
            if ( pObj->fPhase )
            {
                p->pCexSeq = Cec2_ManDeriveCex( p, i, -1 );
                return 0;
            }
    }

    // simulate one round and create classes
    Cec2_ManSimAlloc( p, pPars->nSimWords );
    Cec2_ManSimulateCis( p );
    Cec2_ManSimulate( p, NULL );
    if ( pPars->fIsMiter && !Cec2_ManSimulateCos(p) ) // cex detected
        return 0;
    Cec2_ManCreateClasses( p );
    if ( pPars->fVerbose )
        Gia_ManEquivPrintClasses( p, pPars->fVeryVerbose, 0 );

    // perform additinal simulation
    for ( i = 0; i < pPars->nSimRounds; i++ )
    {
        Cec2_ManSimulateCis( p );
        Cec2_ManSimulate( p, NULL );
        if ( pPars->fIsMiter && !Cec2_ManSimulateCos(p) ) // cex detected
            return 0;
        if ( pPars->fVerbose )
            Gia_ManEquivPrintClasses( p, pPars->fVeryVerbose, 0 );
    }
    // perform sweeping
    pMan = Cec2_ManCreate( p, pPars );
    for ( Iter = 0; fDisproved; Iter++ )
    {
        fDisproved = 0;
        Cec2_ManSimulateCis( p );
        Vec_IntClear( pMan->vCexTriples );
        Gia_ManForEachAnd( p, pObj, i )
        {
            pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1 || Gia_ObjFanin1(pObj)->fMark1;
            if ( pObj->fMark1 ) // skip nodes in the TFO of a disproved one
                continue;
            if ( ~pObj->Value ) // skip swept nodes
                continue;
            if ( !~Gia_ObjFanin0(pObj)->Value || !~Gia_ObjFanin1(pObj)->Value ) // skip fanouts of non-swept nodes
                continue;
            assert( !Gia_ObjProved(p, i) && !Gia_ObjFailed(p, i) );
            // duplicate the node
            pObj->Value = Gia_ManHashAnd( pMan->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
            if ( Vec_IntSize(&pMan->pNew->vCopies) == Abc_Lit2Var(pObj->Value) )
            {
                pObjNew = Gia_ManObj( pMan->pNew, Abc_Lit2Var(pObj->Value) );
                pObjNew->fMark0 = Gia_ObjIsMuxType( pObjNew );
                Gia_ObjSetPhase( pMan->pNew, pObjNew );
                Vec_IntPush( &pMan->pNew->vCopies, -1 );
            }
            assert( Vec_IntSize(&pMan->pNew->vCopies) == Gia_ManObjNum(pMan->pNew) );
            pRepr = Gia_ObjReprObj( p, i );
            if ( pRepr == NULL || pRepr->fMark1 )
                continue;
            if ( Abc_Lit2Var(pObj->Value) == Abc_Lit2Var(pRepr->Value) )
            {
                assert( (pObj->Value ^ pRepr->Value) == (pObj->fPhase ^ pRepr->fPhase) );
                Gia_ObjSetProved( p, i );
                continue;
            }
            if ( Cec2_ManSweepNode(pMan, i) )
                continue;
            pObj->Value = ~0;
            //Vec_IntPushThree( pMan->vCexTriples, Gia_ObjId(p, pRepr), i, Abc_Var2Lit(p->iPatsPi, pObj->fPhase ^ pRepr->fPhase) );
            // mark nodes as disproved
            fDisproved = 1;
            if ( Iter > 5 )
                continue;
            if ( Gia_ObjIsAnd(pRepr) )
                pRepr->fMark1 = 1;
            pObj->fMark1 = 1;
        }
        if ( fDisproved )
        {
            //printf( "The number of pattern = %d.\n", p->iPatsPi );
            Cec2_ManSimulate( p, pMan->vCexTriples );
            if ( pPars->fIsMiter && !Cec2_ManSimulateCos(p) ) // cex detected
                break;
        }
        if ( pPars->fVerbose )
            Gia_ManEquivPrintClasses( p, pPars->fVeryVerbose, 0 );
    }
    Cec2_ManDestroy( pMan );
    //Gia_ManEquivPrintClasses( p, 1, 0 );
    return p->pCexSeq ? 0 : 1;
}
void Cec2_ManSimulateTest( Gia_Man_t * p )
{
    abctime clk = Abc_Clock();
    Cec2_Par_t Pars, * pPars = &Pars;
    Cec2_SetDefaultParams( pPars );
//    Gia_ManComputeGiaEquivs( p, 100000, 0 );
//    Gia_ManEquivPrintClasses( p, 1, 0 );
    Cec2_ManPerformSweeping( p, pPars );
    Abc_PrintTime( 1, "SAT sweeping time", Abc_Clock() - clk );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


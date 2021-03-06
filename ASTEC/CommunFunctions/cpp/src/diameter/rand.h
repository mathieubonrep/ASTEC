/*************************************************************************
 *  -
 *
 * $Id: rand.h,v 1.2 2004/06/10 09:23:33 greg Exp $
 *
 * Copyright INRIA
 *
 * AUTHOR:
 * Gregoire Malandain (greg@sophia.inria.fr)
 * 
 * CREATION DATE: 
 * Tue May 16 2000
 *
 *
 * ADDITIONS, CHANGES
 * 
 *
 */

#ifndef _rand_h_
#define _rand_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdlib.h>

extern long int _GetRandomCalls();
extern long int _GetRandomSeed();

#ifdef WIN32
extern void   _SetRandomSeed( unsigned int seed );
#else
extern void   _SetRandomSeed( long int seed );
#endif

/* return a double number d | 0 <= d < 1
 */
extern double _GetRandomDoubleNb( );

/* return a int number i | min <= i <= max 
 */
extern int    _GetRandomIntNb( int min, int max );

#ifdef __cplusplus
}
#endif

#endif

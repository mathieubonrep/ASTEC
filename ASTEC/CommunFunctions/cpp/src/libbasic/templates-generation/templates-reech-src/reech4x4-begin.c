/*************************************************************************
 * reech4x4.c -
 *
 * $Id: reech4x4.begin,v 1.3 2001/12/10 08:45:17 greg Exp $
 *
 * Copyright (c) INRIA 1999
 *
 * AUTHOR:
 * Gregoire Malandain (greg@sophia.inria.fr)
 * 
 * CREATION DATE: 
 *
 *
 * ADDITIONS, CHANGES
 *	
 *	
 *	
 *
 */


/* CAUTION
   DO NOT EDIT THIS FILE,
   UNLESS YOU HAVE A VERY GOOD REASON 
 */

#include <stdio.h>
#include <stdlib.h>

#include <typedefs.h>
#include <chunks.h>

#include <reech4x4.h>



#define _CONVERTR_(R) ( R )
#define _CONVERTI_(R) ( (R) >= 0.0 ? ((int)((R)+0.5)) : ((int)((R)-0.5)) )
static int _verbose_ = 1;




void setVerboseInReech4x4( int v )
{
  _verbose_ = v;
}

void incrementVerboseInReech4x4(  )
{
  _verbose_ ++;
}

void decrementVerboseInReech4x4(  )
{
  _verbose_ --;
  if ( _verbose_ < 0 ) _verbose_ = 0;
}





typedef struct {
  void* theBuf; /* buffer to be resampled */
  int *theDim; /* dimensions of this buffer */
  void* resBuf; /* result buffer */
  int *resDim;  /* dimensions of this buffer */
  double* mat;   /* transformation matrix */
  float gain;
  float bias;
} _LinearResamplingParam;
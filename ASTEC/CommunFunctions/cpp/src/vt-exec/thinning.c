/*************************************************************************
 * thinning.c -
 *
 * $Id: thinning.c,v 1.4 2000/10/20 13:40:23 greg Exp $
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
 */

#include <vt_common.h>
#include <chamferdistance-mask.h>
#include <thinning.h>

typedef struct local_par {

  vt_names names;
  int type;

  int threshold;
  int anchorThreshold;

  /* calculated mask
   */
  int chamfercoef_dim;
  int chamfercoef_size;
  int chamfercoef_max;

  /* pre-calculated mask 
   */
  enumDistance predefineddistance;

  /* user-provided mask
   */
  char *userprovidedmask;

  /* distance normalization
     ie smallest distance value is 1 
  */
  int normalizedistance;

  /* distances
   */
  char *userprovideddistance;
  char *saveddistance;

  /* thinning parameters
   */
  typeThinningParameters thinpar;

} local_par;




/*------- Definition des fonctions statiques ----------*/
static void VT_Parse( int argc, char *argv[], local_par *par );
static void VT_ErrorParse( char *str, int l );
static void VT_InitParam( local_par *par );





static char *usage = "[image-in] [image-out]\n\
 [-t|-threshold|-sb %d] [-at|-anchor-threshold %d]\n\
 [-chamfer-mask-dim | -cm-dim %d] [-2D] \n\
 [-chamfer-mask-size |-cm-size %d]\n\
 [-chamfer-mask-max |-cm-max %d]\n\
 [-predefined-chamfer-mask|-pcm 4|8|3x3|5x5|6|18|26|3x3x3|5x5x5]\n\
 [-connectivity|-con|-c %d]\n\
 [-user-provided-mask|-mask %s]\n\
 [-no-normalization|-nn] [-normalization|-n]\n\
 [-read-distance-image|-read-distance|-rdimage|-rdistance %s]\n\
 [-saved-distance-image|-saved-distance|-sdimage|-sdistance %s]\n\
 [-cbe|-cycles-before-end %d] [-dbe|-distance-before-end %d] \n\
 [-t|-thick|-thickness %d] [-shrink]\n\
 [-curve(s)|-pure-curve(s)|-surface(s)|-pure-surface(s)]\n\
 [-inv] [-swap] [-v] [-D] [-help] [options-de-type]";

static char *detail = "\n\
[-threshold | -t | -sb %lf] # threshold to binarize the input image\n\
[-chamfer-mask-dim | -cm-dim %d]  # dimension of the chamfer mask\n\
              # 3 (default)\n\
              # 2 for slice-by-slice independent computation\n\
[-2D]         # id. '-chamfer-mask-dim 2'\n\
[-chamfer-mask-size | -cm-size %d] # size of the chamfer mask\n\
              # 3 (default), 5, 7, ...\n\
[-chamfer-mask-max | -cm-max  %d]  # maximum value of the minimum\n\
              # value of the chamfer mask. The larger this value, \n\
              # the longer the computation of the mask\n\
              # a rule of thumb is to specify the size of the mask\n\
[-predefined-chamfer-mask | -pcm ... ] \n\
              # 4 or 8 : 2D adjacencies\n\
              # 6, 10, 18, or 26 : 3D adjacencies\n\
              # 3x3 : (3,4) mask\n\
              # 5x5 : (5,7,11) mask \n\
              # 3x3x3 : (3,4,5) mask \n\
              # 5x5x5 : (5,7,11,9,12,15) mask \n\
[-connectivity|-con %d] # not used yet\n\
[-chamfer %d] # id. -predefined-chamfer-mask %dx%dx%d for 3, 5\n\
              # or -predefined-chamfer-mask %d for 4, 8, 6, 18, 26\n\
[-user-provided-mask|-mask %s]    # user-provided mask image\n\
[-at|-anchor-threshold %d]   # threshold value for anchor points\n\
  these points will not be deleted\n\
[-read-distance-image|-rdimage %s]  # user-provided distance image\n\
[-saved-distance-image|-sdimage %s] # saved distance image\n\
\n\
[-cbe|-cycles-before-end %d] # number of cycles before extracting\n\
  endpoints. A cycle is done when all directions have been considered\n\
  for a given distance value.\n\
[-dbe|-distance-before-end %d] # minimal distance before\n\
  extracting endpoints.\n\
[-t|-thick|-thickness %d]    # adjacency parsed to reduce\n\
  thickness (default is 26)\n\
[-shrink]                    # no endpoint condition\n\
[-curve(s)]        # end condition is t8 or t26 >= 2\n\
[-pure-curve(s)]   # end condition is t8 or t26  = 2\n\
[-surface(s)]      # end condition is t4 or t6  >= 2\n\
[-pure-surface(s)] # end condition is t4 or t6   = 2\n\
[-inv]        # invert 'image-in'\n\
[-swap]       # swap 'image-in' (if encoded on 2 bytes)\n\
[-v|-nv]      # verbose | no verbose\n\
[-D]          #  debug\n\
 $Revision: 1.4 $ $Date: 2000/10/20 13:40:23 $ $Author: greg $\n";

static char program[STRINGLENGTH];






int main( int argc, char *argv[] )
{
  local_par par;
  vt_image *image = (vt_image*)NULL;
  vt_image imres;
  int thinValue = 200;
  int anchorValue = 255;

  vt_image *imdist = (vt_image*)NULL;
  vt_image imtmp;
  vt_image *thedist = (vt_image*)NULL;

  vt_image *immask;
  typeChamferMask theMask;
  double voxel_size[3];

  int theDim[3];


  /*--- initialisation des parametres ---*/
  VT_InitParam( &par );
  
  /*--- lecture des parametres ---*/
  VT_Parse( argc, argv, &par );
  
  par.thinpar.anchorValue = anchorValue;



  /*--- lecture de l'image d'entree ---*/
  image = _VT_Inrimage( par.names.in );
  if ( image == (vt_image*)NULL ) 
    VT_ErrorParse("unable to read input image\n", 0);
  if ( image->type != UCHAR ) 
    VT_ErrorParse("unable to deal with such image type\n", 0);

  
  /*--- operations eventuelles sur l'image d'entree ---*/
  if ( par.names.inv == 1 )  VT_InverseImage( image );
  if ( par.names.swap == 1 ) VT_SwapImage( image );
  
  /*--- initialisation de l'image resultat ---*/
  VT_Image( &imres );
  VT_InitFromImage( &imres, image, par.names.out, UCHAR );

  if ( VT_AllocImage( &imres ) != 1 ) {
    VT_FreeImage( image );
    VT_Free( (void**)&image );
    VT_ErrorParse("unable to allocate output image\n", 0);
  }
  




  /* input image threshold
   */
  switch ( image->type ) {
  default :
    VT_FreeImage( &imres );
    VT_FreeImage( image );
    VT_Free( (void**)&image );
    VT_ErrorParse("such image type not handled for input images", 0);
    break;
  case UCHAR :
    {
      int i;
      int v = image->dim.x * image->dim.y * image->dim.z;
      unsigned char *theBuf = (unsigned char *)image->buf;
      unsigned char *resBuf = (unsigned char *)imres.buf;
      for ( i=0; i<v; i++ ) {
	if ( theBuf[i] < par.threshold ) {
	  resBuf[i] = 0;
	} else if ( theBuf[i] < par.anchorThreshold ) {
	  resBuf[i] = thinValue;
	} else {
	  resBuf[i] = anchorValue;
	}
      }
    }
    break;
  }
  
  VT_FreeImage( image );
  VT_Free( (void**)&image );





  /* distance image
   */
  if ( par.userprovideddistance != (char*)NULL ) {

    imdist = _VT_Inrimage( par.userprovideddistance );
    if ( imdist == (vt_image*)NULL ) {
      VT_FreeImage( &imres );
      VT_ErrorParse("unable to read distance image\n", 0);
    }

    if ( imdist->dim.x != imres.dim.x || 
	 imdist->dim.y != imres.dim.y || 
	 imdist->dim.z != imres.dim.z ) {
      VT_FreeImage( imdist );
      VT_Free( (void**)&imdist );
      VT_FreeImage( &imres );
      VT_ErrorParse("distance and input images have different dimensions\n", 0);
    }

    if ( imdist->type == USHORT ) {
      thedist = imdist;
    }
    else {
      VT_Image( &imtmp );
      VT_InitFromImage( &imtmp, &imres, par.saveddistance, USHORT );
      if ( VT_AllocImage( &imtmp ) != 1 ) {
	VT_FreeImage( imdist );
	VT_Free( (void**)&imdist );
	VT_FreeImage( &imres );
	VT_ErrorParse("unable to allocate auxiliary image\n", 0);
      }
      if ( VT_CopyImage( imdist, &imtmp ) != 1 ) {
	VT_FreeImage( &imtmp );
	VT_FreeImage( imdist );
	VT_Free( (void**)&imdist );
	VT_FreeImage( &imres );
	VT_ErrorParse("unable to copy distance image\n", 0);
      }
    }
    thedist = &imtmp;
    VT_FreeImage( imdist );
    VT_Free( (void**)&imdist );
    imdist = (vt_image*)NULL;
  }
  else {
    
    /* adjust mask dimension w.r.t. image dimension
     */
    if ( imres.dim.z >= 2 && par.chamfercoef_dim == 0 ) {
      par.chamfercoef_dim = 3;
    } 
    else if ( imres.dim.z == 1 ) {
      par.chamfercoef_dim = 2;
    }

    /* mask computation
     */
  
    initChamferMask( &theMask );
    
    if ( par.userprovidedmask != (char*)NULL ) {
      
      immask = _VT_Inrimage( par.userprovidedmask );
      if ( immask == (vt_image*)NULL ) {
	VT_FreeImage( &imres );
	VT_ErrorParse("unable to read mask image\n", 0);
      }
      
      theDim[0] = immask->dim.x;
      theDim[1] = immask->dim.y;
      theDim[2] = immask->dim.z;
      
      if ( buildChamferMaskFromImage( immask->buf, immask->type, theDim, &theMask ) <= 0 ) {
	VT_FreeImage( &imres );
	VT_FreeImage( immask );
	VT_Free( (void**)&immask );
	VT_ErrorParse("unable to build mask from image\n", 0);
      }
      
      VT_FreeImage( immask );
      VT_Free( (void**)&immask );
      
    }
    else {
      voxel_size[0] = 1.0;
      voxel_size[1] = imres.siz.y / imres.siz.x;
      voxel_size[2] = imres.siz.z / imres.siz.x;
    
      if ( buildChamferMask( voxel_size, par.chamfercoef_dim, par.chamfercoef_size,
			     par.chamfercoef_max, par.predefineddistance, &theMask ) != 1 ) {
	freeChamferMask( &theMask );
	VT_FreeImage( &imres );
	VT_ErrorParse( "error when computing masks\n", 0);
      }
    }

    
    
    /* distance computation
     */

    VT_Image( &imtmp );
    VT_InitFromImage( &imtmp, &imres, par.saveddistance, USHORT );
    if ( VT_AllocImage( &imtmp ) != 1 ) {
      freeChamferMask( &theMask );
      VT_FreeImage( &imres );
      VT_ErrorParse("unable to allocate output image\n", 0);
    }

    theDim[0] = imres.dim.x;
    theDim[1] = imres.dim.y;
    theDim[2] = imres.dim.z;
    

    /* distance initialization
     */

    switch ( imres.type ) {
    default :
      VT_FreeImage( &imtmp );
      freeChamferMask( &theMask );
      VT_FreeImage( &imres );
      VT_ErrorParse( "such result image type not handled for distance initialization\n", 0);
      break;
    case UCHAR :
      {
	unsigned char *resBuf = (unsigned char*)imres.buf;
	size_t i, v = imres.dim.x * imres.dim.y * imres.dim.z;
	switch ( imtmp.type ) {
	default :
	  VT_FreeImage( &imtmp );
	  freeChamferMask( &theMask );
	  VT_FreeImage( &imres );
	  VT_ErrorParse( "such distance image type not handled for distance initialization\n", 0);
	  break;
	case USHORT :
	  {
	    unsigned short int *dstBuf = (unsigned short int*)imtmp.buf;
	    for ( i=0; i<v; i++ )
	      dstBuf[i] = (unsigned short)( ( resBuf[i] > 0 ) ? 0 : 65535 );
	  }
	  break;
	}
      }
      break;
    }

    /* last arg indicates whether the chamfer map should be normalized
       ie divided by the min value, so that the min value in the result image
       is 1
       0 means that normalization is not done
    */

    if ( _ComputePositiveChamferMap( imtmp.buf, imtmp.type,
				     imtmp.buf, imtmp.type,
				     theDim,
				     1,
				     &theMask, 0 ) < 0 ) {
      VT_FreeImage( &imtmp );
      freeChamferMask( &theMask );
      VT_FreeImage( &imres );
      VT_ErrorParse( "error when computing distance\n", 0);
      
    }

    thedist = &imtmp;
  }

  
  /* distance has been either computed or read
     do the thinning
   */

  theDim[0] = imres.dim.x;
  theDim[1] = imres.dim.y;
  theDim[2] = imres.dim.z;
  
  if ( ThinImageWithDistanceAndAllParams( (unsigned char *)imres.buf, 
					  (unsigned char *)imres.buf,
					  (unsigned short int *)thedist->buf, 
					  theDim, &(par.thinpar) ) != 1 ) {
    if ( par.userprovideddistance == (char*)NULL ) 
      freeChamferMask( &theMask );
    VT_FreeImage( thedist );
    if ( imdist != (vt_image*)NULL ) 
      VT_Free( (void**)&imdist );
    VT_FreeImage( &imres );
    VT_ErrorParse("error in processing\n", 0);
  }
    

  /* save distance if requested
   */
  if ( par.userprovideddistance == (char*)NULL &&
       par.saveddistance != (char*)NULL ) {
    if ( par.normalizedistance ) {
      if ( _NormalizePositiveChamferMap( thedist->buf, thedist->type,
					 theDim,  &theMask ) != 1 ) {
	if ( par.userprovideddistance == (char*)NULL ) 
	  freeChamferMask( &theMask );
	VT_FreeImage( thedist );
	if ( imdist != (vt_image*)NULL ) 
	  VT_Free( (void**)&imdist );
	VT_FreeImage( &imres );
	VT_ErrorParse("unable to normalize distance\n", 0);
      }
    }
    if ( VT_WriteInrimage( thedist ) == -1 ) {
      if ( par.userprovideddistance == (char*)NULL ) 
	freeChamferMask( &theMask );
      VT_FreeImage( thedist );
      if ( imdist != (vt_image*)NULL ) 
	VT_Free( (void**)&imdist );
      VT_FreeImage( &imres );
      VT_ErrorParse("unable to write distance image\n", 0);
    }
  }

  if ( par.userprovideddistance == (char*)NULL ) 
    freeChamferMask( &theMask );
  VT_FreeImage( thedist );
  if ( imdist != (vt_image*)NULL ) 
    VT_Free( (void**)&imdist );



  /*--- ecriture de l'image resultat ---*/
  if ( VT_WriteInrimage( &imres ) == -1 ) {
    VT_FreeImage( &imres );
    VT_ErrorParse("unable to write output image\n", 0);
  }
  
  /*--- liberations memoires ---*/
  VT_FreeImage( &imres );
  return( 1 );
}








static void VT_Parse( int argc, 
		      char *argv[], 
		      local_par *par )
{
  int i, nb, status;
  int o=0, s=0, r=0;
  int connectivity = 0;
  int chamfer = 0;
  int t=0;
  char text[STRINGLENGTH];
  
  if ( VT_CopyName( program, argv[0] ) != 1 )
    VT_Error("Error while copying program name", (char*)NULL);
  if ( argc == 1 ) VT_ErrorParse("\n", 0 );
  
  /*--- lecture des parametres ---*/
  i = 1; nb = 0;
  while ( i < argc ) {
    if ( argv[i][0] == '-' ) {
      if ( argv[i][1] == '\0' ) {
	if ( nb == 0 ) {
	  /*--- standart input ---*/
	  strcpy( par->names.in, "<" );
	  nb += 1;
	}
      }
      /*--- arguments generaux ---*/
      else if ( strcmp ( argv[i], "-help" ) == 0 ) {
	VT_ErrorParse("\n", 1);
      }
      else if ( strcmp ( argv[i], "-v" ) == 0 && argv[i][2] == '\0' ) {
	_VT_VERBOSE_ = 1;
	incrementVerboseInChamferDistanceMask();
	incrementVerboseInChamferDistance();
	Thinning_verbose();
	Thinning_timeVerbose();
      }
      else if ( strcmp ( argv[i], "-nv" ) == 0 && argv[i][3] == '\0' ) {
	_VT_VERBOSE_ = 1;
	setVerboseInChamferDistanceMask( 0 );
	setVerboseInChamferDistance( 0 );
	Thinning_noverbose();
	Thinning_timeNoverbose();
      }
      else if ( strcmp ( argv[i], "-D" ) == 0 ) {
	_VT_DEBUG_ = 1;
      }
      /*--- traitement eventuel de l'image d'entree ---*/
      else if ( strcmp ( argv[i], "-inv" ) == 0 ) {
	par->names.inv = 1;
      }
      else if ( strcmp ( argv[i], "-swap" ) == 0 ) {
	par->names.swap = 1;
      }



      /* image threshold
       */
      else if ( (strcmp ( argv[i], "-threshold" ) == 0) || 
		(strcmp ( argv[i], "-t" ) == 0 && argv[i][2] == '\0') || 
		(strcmp ( argv[i], "-sb" ) == 0 && argv[i][3] == '\0') ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -threshold...\n", 0 );
	status = sscanf( argv[i], "%d", &par->threshold );
	if ( status <= 0 ) VT_ErrorParse( "parsing -threshold...\n", 0 );
      }

      else if ( (strcmp ( argv[i], "-anchor-threshold" ) == 0) || 
		(strcmp ( argv[i], "-at" ) == 0) ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -anchor-threshold...\n", 0 );
	status = sscanf( argv[i], "%d", &par->anchorThreshold );
	if ( status <= 0 ) VT_ErrorParse( "parsing -anchor-threshold...\n", 0 );
      }

      /* parameters for mask computation
       */

      else if ( (strcmp ( argv[i], "-chamfer-mask-dim" ) == 0) ||
		(strcmp ( argv[i], "-cm-dim" ) == 0) ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -chamfer-mask-dim...\n", 0 );
	status = sscanf( argv[i],"%d", &(par->chamfercoef_dim) );
	if ( status <= 0 ) VT_ErrorParse( "parsing -chamfer-mask-dim...\n", 0 );
      }
      else if ( strcmp ( argv[i], "-2D" ) == 0 && argv[i][3] == '\0' ) {
	par->chamfercoef_dim = 2;
      }
      else if ( (strcmp ( argv[i], "-chamfer-mask-size" ) == 0) ||
		(strcmp ( argv[i], "-cm-size" ) == 0) ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -chamfer-mask-size...\n", 0 );
	status = sscanf( argv[i],"%d", &(par->chamfercoef_size) );
	if ( status <= 0 ) VT_ErrorParse( "parsing -chamfer-mask-size...\n", 0 );
      }

      else if ( (strcmp ( argv[i], "-chamfer-mask-max" ) == 0) ||
		(strcmp ( argv[i], "-cm-max" ) == 0) ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -chamfer-mask-max...\n", 0 );
	status = sscanf( argv[i],"%d", &(par->chamfercoef_max) );
	if ( status <= 0 ) VT_ErrorParse( "parsing -chamfer-mask-max...\n", 0 );
      }

      /* predefined masks
       */

      else if ( (strcmp ( argv[i], "-predefined-chamfer-mask" ) == 0) ||
		(strcmp ( argv[i], "-pcm" ) == 0 && argv[i][4] == '\0') ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -predefined-chamfer-mask...\n", 0 );
	if ( strcmp( argv[i], "4" ) == 0 && argv[i][1] == '\0' ) {
	  par->predefineddistance = _DISTANCE04_;
	}
	else if ( strcmp( argv[i], "8" ) == 0 && argv[i][1] == '\0' ) {
	  par->predefineddistance = _DISTANCE08_;
	}
	else if ( strcmp( argv[i], "6" ) == 0 && argv[i][1] == '\0' ) {
	  par->predefineddistance = _DISTANCE06_;
	}
	else if ( strcmp( argv[i], "10" ) == 0 && argv[i][2] == '\0' ) {
	  par->predefineddistance = _DISTANCE10_;
	}
	else if ( strcmp( argv[i], "18" ) == 0 && argv[i][2] == '\0' ) {
	  par->predefineddistance = _DISTANCE18_;
	}
	else if ( strcmp( argv[i], "26" ) == 0 && argv[i][2] == '\0' ) {
	  par->predefineddistance = _DISTANCE26_;
	}
	else if ( strcmp( argv[i], "3x3" ) == 0 && argv[i][3] == '\0' ) {
	  par->predefineddistance = _CHAMFER3x3_;
	}
	else if ( strcmp( argv[i], "5x5" ) == 0 && argv[i][3] == '\0' ) {
	  par->predefineddistance = _CHAMFER5x5_;
	}
	else if ( strcmp( argv[i], "3x3x3" ) == 0 && argv[i][5] == '\0' ) {
	  par->predefineddistance = _CHAMFER3x3x3_;
	}
	else if ( strcmp( argv[i], "5x5x5" ) == 0 && argv[i][5] == '\0' ) {
	  par->predefineddistance = _CHAMFER5x5x5_;
	}
	else {
	  VT_ErrorParse( "parsing -predefined-chamfer-mask: unknown distance type ...\n", 0 );
	}
      }

      else if ( strcmp ( argv[i], "-connectivity" ) == 0 || 
		(strcmp ( argv[i], "-c" ) == 0  && argv[i][2] == '\0') || 
		(strcmp ( argv[i], "-con" ) == 0 && argv[i][4] == '\0')  ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -connectivity...\n", 0 );
	status = sscanf( argv[i],"%d",&connectivity );
	if ( status <= 0 ) VT_ErrorParse( "parsing -connectivity...\n", 0 );
	switch ( connectivity ) {
	case  4 : 
	case  8 : 
	case  6 : 
	case 10 : 
	case 18 : 
	case 26 : 
	  par->thinpar.connectivity = connectivity;
	  break;
	default :
	  VT_ErrorParse( "parsing -connectivity: connectivity is not handled...\n", 0 );
	}
      }

      else if ( strcmp ( argv[i], "-chamfer" ) == 0 && argv[i][8] == '\0' ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -chamfer...\n", 0 );
	status = sscanf( argv[i],"%d", &chamfer );
	if ( status <= 0 ) VT_ErrorParse( "parsing -chamfer...\n", 0 );
	switch ( chamfer ) {
	case  3 : par->predefineddistance = _CHAMFER3x3x3_; break;
	case  5 : par->predefineddistance = _CHAMFER5x5x5_; break;
	case  4 : par->predefineddistance = _DISTANCE04_; break;
	case  8 : par->predefineddistance = _DISTANCE08_; break;
	case  6 : par->predefineddistance = _DISTANCE06_; break;
	case 10 : par->predefineddistance = _DISTANCE10_; break;
	case 18 : par->predefineddistance = _DISTANCE18_; break;
	case 26 : par->predefineddistance = _DISTANCE26_; break;
	default :
	  VT_ErrorParse( "parsing -chamfer: mask is not handled...\n", 0 );
	}
      }

      /* user provided mask
       */

      else if ( strcmp ( argv[i], "-user-provided-mask" ) == 0
		|| (strcmp ( argv[i], "-mask" ) == 0  && argv[i][5] == '\0') ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -mask...\n", 0 );
	par->userprovidedmask = argv[i];  
      }

      /* distance post-processing
       */ 

      else if ( strcmp ( argv[i], "-normalization" ) == 0 || 
		(strcmp ( argv[i], "-n" ) == 0 && argv[i][2] == '\0') ) {
	par->normalizedistance = 1;
      }

      else if ( strcmp ( argv[i], "-no-normalization" ) == 0 || 
		(strcmp ( argv[i], "-nn" ) == 0  && argv[i][3] == '\0') ) {
	par->normalizedistance = 0;
      }

      /* distance images
       */

      else if ( strcmp ( argv[i], "-read-distance-image" ) == 0 ||
		strcmp ( argv[i], "-read-distance" ) == 0 ||
		strcmp ( argv[i], "-rdimage" ) == 0 ||
		strcmp ( argv[i], "-rdistance" ) == 0 ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -read-distance-image...\n", 0 );
	par->userprovideddistance = argv[i];  
      }

      else if ( strcmp ( argv[i], "-saved-distance-image" ) == 0 ||
		strcmp ( argv[i], "-saved-distance" ) == 0 ||
		strcmp ( argv[i], "-sdimage" ) == 0 ||
		strcmp ( argv[i], "-sdistance" ) == 0 ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -saved-distance-image...\n", 0 );
	par->saveddistance = argv[i];  
      }

      /* thinning parameters
       */
      
      else if ( (strcmp ( argv[i], "-cycles-before-end" ) == 0) || 
		(strcmp ( argv[i], "-cbe" ) == 0) ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -cycles-before-end...\n", 0 );
	status = sscanf( argv[i], "%d", &(par->thinpar.cyclesBeforeEnding) );
	if ( status <= 0 ) VT_ErrorParse( "parsing -cycles-before-end...\n", 0 );
      }


      else if ( (strcmp ( argv[i], "-distance-before-end" ) == 0) || 
		(strcmp ( argv[i], "-dbe" ) == 0) ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -distance-before-end...\n", 0 );
	status = sscanf( argv[i], "%d", &(par->thinpar.distanceBeforeEnding) );
	if ( status <= 0 ) VT_ErrorParse( "parsing -distance-before-end...\n", 0 );
      }


      else if ( (strcmp ( argv[i], "-thickness" ) == 0) || 
		(strcmp ( argv[i], "-thick" ) == 0) || 
		(strcmp ( argv[i], "-t" ) == 0) ) {
		i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -thickness...\n", 0 );
	status = sscanf( argv[i], "%d", &t );
	if ( status <= 0 ) VT_ErrorParse( "parsing -thickness...\n", 0 );
	switch ( t ) {
	default :
	  VT_ErrorParse( "parsing -thickness...\n", 0 );
	  break;
	case  4 : par->thinpar.typeThickness = _04THICKNESS_; break;
	case  8 : par->thinpar.typeThickness = _08THICKNESS_; break;
	case  6 : par->thinpar.typeThickness = _06THICKNESS_; break;
	case 18 : par->thinpar.typeThickness = _18THICKNESS_; break;
	case 26 : par->thinpar.typeThickness = _26THICKNESS_; break;
	}
      }



      else if ( (strcmp ( argv[i], "-shrink" ) == 0) ) {
	par->thinpar.typeEndPoint = _NO_END_POINT_;
      }

      else if ( (strcmp ( argv[i], "-curve" ) == 0) || 
		(strcmp ( argv[i], "-curves" ) == 0) ) {
	par->thinpar.typeEndPoint = _CURVE_;
      }

      else if ( (strcmp ( argv[i], "-pure-curve" ) == 0) || 
		(strcmp ( argv[i], "-pure-curves" ) == 0) ) {
	par->thinpar.typeEndPoint = _PURE_CURVE_;
      }

      else if ( (strcmp ( argv[i], "-surface" ) == 0) || 
		(strcmp ( argv[i], "-surfaces" ) == 0) ) {
	par->thinpar.typeEndPoint = _SURFACE_;
      }

      else if ( (strcmp ( argv[i], "-pure-surface" ) == 0) || 
		(strcmp ( argv[i], "-pure-surfaces" ) == 0) ) {
	par->thinpar.typeEndPoint = _PURE_SURFACE_;
      }


      /*--- lecture du type de l'image de sortie ---*/
      else if ( strcmp ( argv[i], "-r" ) == 0 ) {
	r = 1;
      }
      else if ( strcmp ( argv[i], "-s" ) == 0 ) {
	s = 1;
      }
      else if ( strcmp ( argv[i], "-o" ) == 0 ) {
	i += 1;
	if ( i >= argc)    VT_ErrorParse( "parsing -o...\n", 0 );
	status = sscanf( argv[i],"%d",&o );
	if ( status <= 0 ) VT_ErrorParse( "parsing -o...\n", 0 );
      }

      /*--- option inconnue ---*/
      else {
	sprintf(text,"unknown option %s\n",argv[i]);
	VT_ErrorParse(text, 0);
      }
    }
    /*--- saisie des noms d'images ---*/
    else if ( argv[i][0] != 0 ) {
      if ( nb == 0 ) { 
	strncpy( par->names.in, argv[i], STRINGLENGTH );  
	nb += 1;
      }
      else if ( nb == 1 ) {
	strncpy( par->names.out, argv[i], STRINGLENGTH );  
	nb += 1;
      }
      else 
	VT_ErrorParse("too much file names when parsing\n", 0 );
    }
    i += 1;
  }

  

   if ( par->chamfercoef_dim == 2 ) {
    switch ( par->predefineddistance ) {
    default :
      break;
    case _DISTANCE06_ :
      par->predefineddistance = _DISTANCE04_;
      break;
    case _DISTANCE10_ :
    case _DISTANCE18_ :
    case _DISTANCE26_ :
      par->predefineddistance = _DISTANCE08_;
      break;
    case _CHAMFER3x3x3_ :
      par->predefineddistance = _CHAMFER3x3_;
      break;
    case _CHAMFER5x5x5_ :
      par->predefineddistance = _CHAMFER5x5_;
      break;
    }
  }



  /*--- s'il n'y a pas assez de noms ... ---*/
  if (nb == 0) {
    strcpy( par->names.in,  "<" );  /* standart input */
    strcpy( par->names.out, ">" );  /* standart output */
  }
  if (nb == 1)
    strcpy( par->names.out, ">" );  /* standart output */
  
}






static void VT_ErrorParse( char *str, int flag )
{
  (void)fprintf(stderr,"Usage : %s %s\n",program, usage);
  if ( flag == 1 ) (void)fprintf(stderr,"%s",detail);
  (void)fprintf(stderr,"Erreur : %s",str);
  exit(0);
}








static void VT_InitParam( local_par *par )
{
  VT_Names( &(par->names) );
  par->type = TYPE_UNKNOWN;

  par->threshold = 1;
  par->anchorThreshold = 256;

  par->chamfercoef_dim = 0;
  par->chamfercoef_size = 3;
  par->chamfercoef_max = 3;

  par->predefineddistance = _UNDEFINED_DISTANCE_;

  par->userprovidedmask = (char*)NULL;

  par->normalizedistance = 1;

  par->userprovideddistance = (char*)NULL;
  par->saveddistance = (char*)NULL;

  initTypeThinningParameters( &(par->thinpar) );

  par->thinpar.cyclesBeforeEnding = 4;
  par->thinpar.distanceBeforeEnding = 1;

  par->thinpar.typeThickness = _26THICKNESS_;
  par->thinpar.typeEndPoint = _SURFACE_;
}

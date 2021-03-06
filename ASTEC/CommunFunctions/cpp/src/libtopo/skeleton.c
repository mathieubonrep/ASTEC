/*************************************************************************
 * skeleton.c
 *
 * $Id: skeleton.c,v 1.1 2000/07/26 07:25:02 greg Exp $
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
#include <vt_copy.h>

#include <vt_distance.h>
#include <vt_daneucmapsc.h>
#include <vt_daneucmapss.h>

#include <vt_skeleton.h>


typedef enum {
  _RAGNEMALM_,
  _DANIELSSON_ 
} enumDistance;



typedef struct local_par {
  vt_names names;
  vt_distance dpar;
  enumDistance type_distance;
  enumCoefficient type_param;
  int type;
} local_par;

/*------- Definition des fonctions statiques ----------*/
#ifndef NO_PROTO
static void VT_Parse( int argc, char *argv[], local_par *par );
static void VT_ErrorParse( char *str, int l );
static void VT_InitParam( local_par *par );
#else 
static void VT_Parse();
static void VT_ErrorParse();
static void VT_InitParam();
#endif

#ifndef NO_PROTO

static char *usage = "[image-in] [image-out] [-rec %s] [-sb %f] [-euc_type %d] \n\
\t [-angle | -distance | -produit | -prodlog | -logprod]\n\
\t [-ragnemalm | -danielsson]\n\
\t [-inv] [-swap] [-v] [-D] [-help] [options-de-type]";

static char *detail = "\
\t si 'image-in' est '-', on prendra stdin\n\
\t si 'image-out' est absent, on prendra stdout\n\
\t si les deux sont absents, on prendra stdin et stdout\n\
\t -sb %f : seuil (0.5 par defaut)\n\
\t -euc_type %d : codage pour le calcul de l'euclidean mapping\n\
\t                1 = signed char, 2 = signed short (defaut)\n\
\t -angle    : calcul de l'angle (en degres)\n\
\t -distance : calcul de la distance (en pixels)\n\
\t -produit  : produit = distance * angle\n\
\t -prodlog  : angle * log( distance )\n\
\t -logprod  : log( angle ) * distance\n\
\t -produit2 : angle * distance^(.2630344061). On a (180,1) = (150,2) = 180\n\
\t -produit3 : angle * distance^(.3690702462). On a (180,1) = (120,3) = 180\n\
\t -produit4 : angle * sqrt(distance). On a (180,1) = (90,4) = 180\n\
\t -produit5 : angle * distance^(1.2223924213). On a (180,1) = 180 et (90,2) = 210\n\
\t -produit6 : angle * distance^(1.4150374992). On a (180,1) = 180 et (90,2) = 240\n\
\t -produit7 : angle * distance^(1.5)\n\
\t -inv : inverse 'image-in'\n\
\t -swap : swap 'image-in' (si elle est codee sur 2 octets)\n\
\t -v : mode verbose\n\
\t -D : mode debug\n\
\t options-de-type : -o 1    : unsigned char\n\
\t                   -o 2    : unsigned short int\n\
\t                   -o 2 -s : short int\n\
\t                   -o 4 -s : int\n\
\t                   -r      : float\n\
\t si aucune de ces options n'est presente, on prend le type de 'image-in'\n";

#else

static char *usage = "";
static char *detail = "";

#endif

static char program[STRINGLENGTH];

#ifndef NO_PROTO
int main( int argc, char *argv[] )
#else
int main( argc, argv )
int argc;
char *argv[];
#endif
{
	local_par par;
	vt_image *image, imaux, imneigh, imres;
	vt_image imx, imy, imz;

	/*--- initialisation des parametres ---*/
	VT_InitParam( &par );
	/*--- lecture des parametres ---*/
	VT_Parse( argc, argv, &par );

	/*--- lecture de l'image d'entree ---*/
	image = _VT_Inrimage( par.names.in );
	if ( image == (vt_image*)NULL ) 
		VT_ErrorParse("unable to read input image\n", 0);

	/*--- operations eventuelles sur l'image d'entree ---*/
	if ( par.names.inv == 1 )  VT_InverseImage( image );
	if ( par.names.swap == 1 ) VT_SwapImage( image );
	
	/*--- calcul du vecteur pointant vers le plus proche point ---*/
	/*--- initialisation des images resultat ---*/
	VT_Image( &imx );
	VT_InitFromImage( &imx, image, par.names.out, par.dpar.type_image_eucmap );
	VT_Image( &imy );
	VT_InitFromImage( &imy, image, par.names.out, par.dpar.type_image_eucmap );
	VT_Image( &imz );
	VT_InitFromImage( &imz, image, par.names.out, par.dpar.type_image_eucmap );
	(void)strcat( imx.name, ".x.inr" );
	(void)strcat( imy.name, ".y.inr" );
	(void)strcat( imz.name, ".z.inr" );
	if ( VT_AllocImage( &imx ) != 1 ) {
	    VT_FreeImage( image );
	    VT_Free( (void**)&image );
	    VT_ErrorParse("unable to allocate first output image\n", 0);
	}
	if ( VT_AllocImage( &imy ) != 1 ) {
	    VT_FreeImage( &imx );
	    VT_FreeImage( image );
	    VT_Free( (void**)&image );
	    VT_ErrorParse("unable to allocate first second image\n", 0);
	}
	if ( VT_AllocImage( &imz ) != 1 ) {
	    VT_FreeImage( &imx );
	    VT_FreeImage( &imy );
	    VT_FreeImage( image );
	    VT_Free( (void**)&image );
	    VT_ErrorParse("unable to allocate first third image\n", 0);
	}
	
	/*--- initialisation de l'image auxiliaire ---*/
	VT_Image( &imaux );
	VT_InitFromImage( &imaux, image, par.names.out, FLOAT );
	(void)strcat( imaux.name, ".aux.inr" );
	if ( VT_AllocImage( &imaux ) != 1 ) {
	    VT_FreeImage( &imx );
	    VT_FreeImage( &imy );
	    VT_FreeImage( &imz );
	    VT_FreeImage( image );
	    VT_Free( (void**)&image );
	    VT_ErrorParse("unable to allocate auxiliary image\n", 0);
	}
	
	VT_Image( &imneigh );
	VT_InitFromImage( &imneigh, image, par.names.out, UCHAR );
	(void)strcat( imneigh.name, ".neigh.inr" );
	if ( VT_AllocImage( &imneigh ) != 1 ) {
	    VT_FreeImage( &imaux );
	    VT_FreeImage( &imx );
	    VT_FreeImage( &imy );
	    VT_FreeImage( &imz );
	    VT_FreeImage( image );
	    VT_Free( (void**)&image );
	    VT_ErrorParse("unable to allocate neighbors image\n", 0);
	}



	
	/*--- calcul ---*/
	switch ( par.dpar.type_image_eucmap ) {
	case SCHAR :
	  {
	    switch ( par.type_distance ) {
	    case _DANIELSSON_ :
	      if ( VT_DanVecteurPPP_SC( image, &imx, &imy, &imz, &(par.dpar) ) != 1 ) {
		VT_FreeImage( &imx );
		VT_FreeImage( &imy );
		VT_FreeImage( &imz );
		VT_FreeImage( &imaux );
		VT_FreeImage( image );
		VT_Free( (void**)&image );
		VT_ErrorParse("unable to compute nearest point\n", 0);
	      }
	      break;
	    case _RAGNEMALM_ :
	    default :
	      if ( VT_VecteurPPP_SC( image, &imx, &imy, &imz, &(par.dpar) ) != 1 ) {
		VT_FreeImage( &imx );
		VT_FreeImage( &imy );
		VT_FreeImage( &imz );
		VT_FreeImage( &imaux );
		VT_FreeImage( image );
		VT_Free( (void**)&image );
		VT_ErrorParse("unable to compute nearest point\n", 0);
	      }
	    }
	  }
	  break;

	case SSHORT :
	  {
	    switch ( par.type_distance ) {
	    case _DANIELSSON_ :
	      if ( VT_DanVecteurPPP_SS( image, &imx, &imy, &imz, &(par.dpar) ) != 1 ) {
		VT_FreeImage( &imx );
		VT_FreeImage( &imy );
		VT_FreeImage( &imz );
		VT_FreeImage( &imaux );
		VT_FreeImage( image );
		VT_Free( (void**)&image );
		VT_ErrorParse("unable to compute nearest point\n", 0);
	      }
	      break;
	    case _RAGNEMALM_ :
	    default :
	      if ( VT_VecteurPPP_SS( image, &imx, &imy, &imz, &(par.dpar) ) != 1 ) {
		VT_FreeImage( &imx );
		VT_FreeImage( &imy );
		VT_FreeImage( &imz );
		VT_FreeImage( &imaux );
		VT_FreeImage( image );
		VT_Free( (void**)&image );
		VT_ErrorParse("unable to compute nearest point\n", 0);
	      }
	    }
	  }
	}




	VT_Compute2DSkeletonCoefficient( &imx, &imy, &imaux, &imneigh, par.type_param );




	(void)VT_WriteInrimage( &imx );
	(void)VT_WriteInrimage( &imy );
	(void)VT_WriteInrimage( &imz );
	(void)VT_WriteInrimage( &imneigh );
	    
	/*--- liberations memoires ---*/
	VT_FreeImage( &imx );
	VT_FreeImage( &imy );
	VT_FreeImage( &imz );

	/*--- initialisation de l'image resultat ---*/
	VT_Image( &imres );
	VT_InitFromImage( &imres, image, par.names.out, image->type );
	(void)strcat( imres.name, ".coef.inr" );
	if ( par.type != TYPE_UNKNOWN ) imres.type = par.type;
	if ( VT_AllocImage( &imres ) != 1 ) {
	    VT_FreeImage( &imaux );
	    VT_FreeImage( image );
	    VT_Free( (void**)&image );
	    VT_ErrorParse("unable to allocate output image\n", 0);
	}
	if ( VT_CopyImage( &imaux, &imres ) != 1 ) {
	    VT_FreeImage( &imaux );
	    VT_FreeImage( &imres );
	    VT_ErrorParse("unable to copy output image\n", 0);
	}

	/*--- liberations memoires ---*/ 
	VT_FreeImage( &imaux );

	/*--- ecriture de l'image resultat ---*/
	if ( VT_WriteInrimage( &imres ) == -1 ) {
	    VT_FreeImage( &imres );
	    VT_ErrorParse("unable to write output image\n", 0);
	}

	/*--- liberations memoires ---*/
	VT_FreeImage( &imres );
	

	/*--- liberations memoires ---*/
	VT_FreeImage( image );
	VT_Free( (void**)&image );

	return( 1 );
}

#ifndef NO_PROTO
static void VT_Parse( int argc, char *argv[], local_par *par )
#else
static void VT_Parse( argc, argv, par )
int argc;
char *argv[];
local_par *par;
#endif
{
	int i, nb, status;
	int o=0, s=0, r=0;
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
			else if ( strcmp ( argv[i], "-help" ) == 0 ) {
                                VT_ErrorParse("\n", 1);
                        }
                        else if ( strcmp ( argv[i], "-inv" ) == 0 ) {
                                par->names.inv = 1;
			}
                        else if ( strcmp ( argv[i], "-swap" ) == 0 ) {
                                par->names.swap = 1;
			}
                        else if ( strcmp ( argv[i], "-v" ) == 0 ) {
                                _VT_VERBOSE_ = 1;
			}
                        else if ( strcmp ( argv[i], "-D" ) == 0 ) {
                                _VT_DEBUG_ = 1;
			}
                        /*--- seuil ---*/
                        else if ( strcmp ( argv[i], "-sb" ) == 0 ) {
                                i += 1;
                                if ( i >= argc)    VT_ErrorParse( "parsing -sb...\n", 0 );
                                status = sscanf( argv[i],"%f",&(par->dpar.seuil) );
                                if ( status <= 0 ) VT_ErrorParse( "parsing -sb...\n", 0 );
                        }
			/*--- dimension du traitement ---*/
			else if ( strcmp ( argv[i], "-2D" ) == 0 ) {
                                par->dpar.dim = VT_2D;
			}
			/*--- type pour le calcul de l'euclidean mapping ---*/
                        else if ( strcmp ( argv[i], "-euc_type" ) == 0 ) {
                                i += 1;
                                if ( i >= argc)    VT_ErrorParse( "parsing -euc_type...\n", 0 );
                                status = sscanf( argv[i],"%d",&(par->dpar.type_image_eucmap) );
                                if ( status <= 0 ) VT_ErrorParse( "parsing -euc_type...\n", 0 );
                        }
			/*--- traitement ---*/
			else if ( strcmp ( argv[i], "-angle" ) == 0 ) {
                                par->type_param = _ANGLE_;
                        }
			else if ( strcmp ( argv[i], "-distance" ) == 0 ) {
                                par->type_param = _DISTANCE_;
                        }
			else if ( strcmp ( argv[i], "-produit" ) == 0 ) {
			  par->type_param = _PRODUIT_;
                        }
			else if ( strcmp ( argv[i], "-produit2" ) == 0 ) {
			  par->type_param = _PRODUIT_2_;
                        }
			else if ( strcmp ( argv[i], "-produit3" ) == 0 ) {
			  par->type_param = _PRODUIT_3_;
                        }
			else if ( strcmp ( argv[i], "-produit4" ) == 0 ) {
			  par->type_param = _PRODUIT_4_;
                        }
			else if ( strcmp ( argv[i], "-produit5" ) == 0 ) {
			  par->type_param = _PRODUIT_5_;
                        }
			else if ( strcmp ( argv[i], "-produit6" ) == 0 ) {
			  par->type_param = _PRODUIT_6_;
                        }
			else if ( strcmp ( argv[i], "-produit7" ) == 0 ) {
			  par->type_param = _PRODUIT_7_;
                        }
			else if ( strcmp ( argv[i], "-logprod" ) == 0 ) {
                                par->type_param = _LOG_PRODUIT_;
                        }
			else if ( strcmp ( argv[i], "-prodlog" ) == 0 ) {
                                par->type_param = _PRODUIT_LOG_;
                        }
			else if ( strcmp ( argv[i], "-ragnemalm" ) == 0 ) {
                                par->type_distance = _RAGNEMALM_;
                        }
			else if ( strcmp ( argv[i], "-danielsson" ) == 0 ) {
                                par->type_distance = _DANIELSSON_;
                        }

                        /*--- lecture du type de l'image ---*/
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
			else {
				sprintf(text,"unknown option %s\n",argv[i]);
				VT_ErrorParse(text, 0);
			}
		}
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
        if (nb == 0) {
                strcpy( par->names.in,  "<" );  /* standart input */
                strcpy( par->names.out, ">" );  /* standart output */
        }
        if (nb == 1)
                strcpy( par->names.out, ">" );  /* standart output */

	/*--- conversion du type pour le calcul de l'euclidean mapping ---*/
	switch ( par->dpar.type_image_eucmap ) {
	case 1 :
	    par->dpar.type_image_eucmap = SCHAR;
	    break;
	case 2 :
	default :
	    par->dpar.type_image_eucmap = SSHORT;
	}

	/*--- type de l'image resultat ---*/
	if ( (o == 1) && (s == 0) && (r == 0) ) par->type = UCHAR;
	if ( (o == 2) && (s == 0) && (r == 0) ) par->type = USHORT;
	if ( (o == 2) && (s == 1) && (r == 0) )  par->type = SSHORT;
	if ( (o == 4) && (s == 1) && (r == 0) )  par->type = SINT;
	if ( (o == 0) && (s == 0) && (r == 1) )  par->type = FLOAT;
	if ( par->type == TYPE_UNKNOWN ) VT_Warning("no specified type", program);
}

#ifndef NO_PROTO
static void VT_ErrorParse( char *str, int flag )
#else
static void VT_ErrorParse( str, flag )
char *str;
int flag;
#endif
{
	(void)fprintf(stderr,"Usage : %s %s\n",program, usage);
        if ( flag == 1 ) (void)fprintf(stderr,"%s",detail);
        (void)fprintf(stderr,"Erreur : %s",str);
        exit(0);
}

#ifndef NO_PROTO
static void VT_InitParam( local_par *par )
#else
static void VT_InitParam( par )
local_par *par;
#endif
{
	VT_Names( &(par->names) );
	VT_Distance( &(par->dpar) );
	par->type_distance = _RAGNEMALM_;
	par->type_param = _PRODUIT_;
	par->type = FLOAT;
}

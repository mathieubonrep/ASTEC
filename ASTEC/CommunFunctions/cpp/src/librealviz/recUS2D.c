#include <vt_common.h>
#include <vt_recUS2D.h>

typedef struct local_par {
  vt_names names;
  
  typeGeometryUS2D theGeom;
  
  int dim[3];
  float voxelSize[3];


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

static char *usage = "[image-in] [image-out]\n\
\t [-inv] [-swap] [-v] [-D] [-help] [options-de-type]";

static char *detail = "\
\t si 'image-in' est '-', on prendra stdin\n\
\t si 'image-out' est absent, on prendra stdout\n\
\t si les deux sont absents, on prendra stdin et stdout\n\
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

static char program[STRINGLENGTH];

#if defined(_ANSI_)
int main( int argc, char *argv[] )
#else
  int main( argc, argv )
  int argc;
char *argv[];
#endif
{
  local_par par;
  vt_image *image, imres;
  
  /*--- initialisation des parametres ---*/
  VT_InitParam( &par );
  
  /*--- lecture des parametres ---*/
  VT_Parse( argc, argv, &par );
  
  /*--- lecture de l'image d'entree ---*/
  image = _VT_Inrimage( par.names.in );
  if ( image == (vt_image*)NULL ) 
    VT_ErrorParse("unable to read input image\n", 0);
  
  /*--- operations eventuelles sur l'image d'entree ---*/
  /*
  if ( par.names.inv == 1 )  VT_InverseImage( image );
  if ( par.names.swap == 1 ) VT_SwapImage( image );
  */

  /*--- initialisation de l'image resultat ---*/
  VT_Image( &imres );
  VT_InitImage( &imres, par.names.out, par.dim[0], par.dim[1], par.dim[2], image->type );
  if ( VT_AllocImage( &imres ) != 1 ) {
    VT_FreeImage( image );
    VT_Free( (void**)&image );
    VT_ErrorParse("unable to allocate output image\n", 0);
  }
  imres.siz.x = par.voxelSize[0];
  imres.siz.y = par.voxelSize[1];
  imres.siz.z = par.voxelSize[2];


  if ( _ReconstructUS2D( image, &imres, &(par.theGeom) ) != 1 ) {
    VT_FreeImage( &imres );
    VT_FreeImage( image );
    VT_Free( (void**)&image );
    VT_ErrorParse("error in computing\n", 0);
  }
  

  VT_FreeImage( image );
  VT_Free( (void**)&image );
  /*--- ecriture de l'image resultat ---*/
  if ( VT_WriteInrimage( &imres ) == -1 ) {
    VT_FreeImage( &imres );
    VT_ErrorParse("unable to write output image\n", 0);
  }
  
  /*--- liberations memoires ---*/
  VT_FreeImage( &imres );
  return( 1 );
}



#if defined(_ANSI_)
static void VT_Parse( int argc, char *argv[], local_par *par )
#else
  static void VT_Parse( argc, argv, par )
  int argc;
char *argv[];
local_par *par;
#endif
{
  int i, nb, status;
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
      else if ( strcmp ( argv[i], "-v" ) == 0 ) {
	_VT_VERBOSE_ = 1;
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




      else if ( strcmp ( argv[i], "-theta" ) == 0 ) {
	if ( i+2 >= argc)    VT_ErrorParse( "parsing -theta...\n", 0 );
	status = sscanf( argv[i+1], "%lf", &par->theGeom.thetaMin );
	if ( status <= 0 ) VT_ErrorParse( "parsing -theta...\n", 0 );
	status = sscanf( argv[i+2], "%lf", &par->theGeom.thetaMax );
	if ( status <= 0 ) VT_ErrorParse( "parsing -theta...\n", 0 );
	i += 2;
      }
      else if ( strcmp ( argv[i], "-radius" ) == 0 ) {
	if ( i+2 >= argc)    VT_ErrorParse( "parsing -radius...\n", 0 );
	status = sscanf( argv[i+1], "%lf", &par->theGeom.radiusMin );
	if ( status <= 0 ) VT_ErrorParse( "parsing -radius...\n", 0 );
	status = sscanf( argv[i+2], "%lf", &par->theGeom.radiusMax );
	if ( status <= 0 ) VT_ErrorParse( "parsing -radius...\n", 0 );
	i += 2;
      }



      else if ( strcmp ( argv[i], "-dim" ) == 0 ) {
	if ( i+3 >= argc)    VT_ErrorParse( "parsing -dim...\n", 0 );
	status = sscanf( argv[i+1], "%d", &par->dim[0] );
	if ( status <= 0 ) VT_ErrorParse( "parsing -dim...\n", 0 );
	status = sscanf( argv[i+2], "%d", &par->dim[1] );
	if ( status <= 0 ) VT_ErrorParse( "parsing -dim...\n", 0 );
	status = sscanf( argv[i+3], "%d", &par->dim[2] );
	if ( status <= 0 ) VT_ErrorParse( "parsing -dim...\n", 0 );
	i += 3;
      }
      else if ( strcmp ( argv[i], "-vz" ) == 0 ) {
	if ( i+3 >= argc)    VT_ErrorParse( "parsing -vz...\n", 0 );
	status = sscanf( argv[i+1], "%f", &par->voxelSize[0] );
	if ( status <= 0 ) VT_ErrorParse( "parsing -vz...\n", 0 );
	status = sscanf( argv[i+2], "%f", &par->voxelSize[1] );
	if ( status <= 0 ) VT_ErrorParse( "parsing -vz...\n", 0 );
	status = sscanf( argv[i+3], "%f", &par->voxelSize[2] );
	if ( status <= 0 ) VT_ErrorParse( "parsing -vz...\n", 0 );
	i += 3;
      }







      /*--- lecture du type de l'image de sortie ---*/
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
  
  /*--- s'il n'y a pas assez de noms ... ---*/
  if (nb == 0) {
    strcpy( par->names.in,  "<" );  /* standart input */
    strcpy( par->names.out, ">" );  /* standart output */
  }
  if (nb == 1)
    strcpy( par->names.out, ">" );  /* standart output */
  
  /*--- type de l'image resultat ---*/
}

#if defined(_ANSI_)
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

#if defined(_ANSI_)
static void VT_InitParam( local_par *par )
#else
  static void VT_InitParam( par )
  local_par *par;
#endif
{
  VT_Names( &(par->names) );

  par->dim[0] = 256;
  par->dim[1] = 256;
  par->dim[2] = 1;

  par->voxelSize[0] = 0.5;
  par->voxelSize[1] = 0.5;
  par->voxelSize[2] = 1.0;

  par->theGeom.radiusMin = 1.7;
  par->theGeom.radiusMax = 129;
  par->theGeom.thetaMin = -33.3;
  par->theGeom.thetaMax = 33.3;
  
}

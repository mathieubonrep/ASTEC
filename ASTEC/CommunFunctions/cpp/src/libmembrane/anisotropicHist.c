/*************************************************************************
 * anisotropicHist.c -
 *
 * $Id: anisotropicHist.c,v 1.0 2013/10/22 17:05:00 gael Exp $
 *
 * AUTHOR:
 * Gael Michelin (gael.michelin@inria.fr)
 *
 * CREATION DATE:
 * 2013/10/22
 *
 * ADDITIONS, CHANGES
 *
 */

#include <vt_common.h>
#include <mt_anisotropicHist.h>
#include <time.h>
#include <vt_connexe.h>

#include <levenberg.h>
#include <chunks.h>


#define NBMAXGAUSS 7
#define NBMAXRAYLEIGH 2
#define NBMAXXN 2
#define NBMAXPARAM 33
#define NBMAXLM 11

#define NMAXRATIO 256
#define NMAXBIN 256

#define NMAXMSF 256

typedef struct local_par {

  vt_names names;
  vt_histparam inithistparam;
  int withAngles;
  int writeHisto;
  vt_histmode modehisto;
  int wi;
  double ratio[NMAXRATIO];
  double nratio;
  vt_binmode binarise[NMAXBIN];
  int nbin;

  int cptLevenberg;
  double lmin;
  double lmax;
  int initlmin;
  int initlmax;

  double membraneSurFond[NMAXMSF];
  double membraneTaux[NMAXMSF];
  int nMSF;
  int nMT;

  int nbGauss;
  int nbRayleigh;
  int nbRayleighPos;
  int nbRayleighCentered;
  int nbXn;
  double leven[NBMAXPARAM];
  vt_LM_type LM_types[NBMAXLM];
  int nbleven;
  
  int normalize;

} local_par;



/*------- Definition des fonctions statiques ----------*/
static void MT_Parse( int argc, char *argv[], local_par *par );
static void MT_ErrorParse( char *str, int l );
static void MT_InitParam( local_par *par );





static char *usage = "[image-in] [filename-out] [-histo-proj] [-histo-proj-2] [-histo-n] [-histo-n-2]\n\
\t [-ncell %d] [-minhist %lf] [-maxhist %lf] [-precision %lf] \n\
\t [-linit %s] [-rayleighcentered %lf %lf] \n\
\t [-gauss %lf %lf %lf] [-rayleigh %lf %lf %lf] [-rayleighpos %lf %lf %lf] [-xn %lf %lf] \n\
\t [-lmin %lf] [-lmax %lf] \n\
\t [-[lMSF | lMT] %lf [%lf [...]]] \n\
\t [-lMSFmin %lf -lMSFmax %lf -lMSFnb %d] \n\
\t [-lMTmin %lf -lMTmax %lf -lMTnb %d] \n\
\t [-lbin [-proj | -projection | -dot | -dot2 | -or | -and | -all]] \n\
\t [-lbinproj] [-lbindot] [-lbindot2] [-lbinor] [-lbinand] [-lbinall] \n\
\t [-normalize] \n\
\t [-parallel|-no-parallel] [-max-chunks %d]\n\
\t [-parallel-scheduling|-ps default|static|dynamic-one|dynamic|guided]\n\
\t [-wi] [-v] [-D] [-help]";

static char *detail = "\
\t si 'image-in' est '-', on prendra stdin\n\
\t si 'filename-out' est absent, on prendra stdout\n\
\t si les deux sont absents, on prendra stdin et stdout\n\
\t -histo-proj : histogrammes selon X,Y,Z calcules par \"projection\"\n\
\t -histo-proj-2 : histogrammes selon X,Y,Z calcules par \"projection 2\"\n\
\t -histo-n : histogrammes selon X,Y,Z calcules par somme des \"contributions\"\n\
\t -histo-n-2 : histogrammes selon X,Y,Z calcules par somme des \"contributions\" au carre\n\
\t -ncell %d : nombre de cellules de l'histogramme\n\
\t -minhist %lf : borne inferieure de l'histogramme\n\
\t -maxhist %lf : borne superieure de l'histogramme\n\
\t -precision %lf : precision de l'histogramme\n\
\t -linit %s : initialise les parametres de fitting des histogrammes\n\
\t -rayleighcentered %lf %lf : fitting avec une fonction centree de rayleigh initialisee selon les arguments \n\
\t -rayleigh %lf %lf %lf : fitting avec une fonction de rayleigh initialisee selon les arguments \n\
\t -rayleighpos %lf %lf %lf : fitting avec une fonction positive de rayleigh initialisee selon les arguments \n\
\t -gauss %lf %lf %lf : fitting avec une fonction gaussienne initialisee selon les arguments \n\
\t -xn %lf %lf : fitting avec une fonction type X -> A/X^n initialisee selon les arguments \n\
\t -lmin %lf : fit l'histogramme a partir de la valeur lmin\n\
\t -lmax %lf : fit l'histogramme a partir de la valeur lmax\n\
\t -lMSF %lf [%lf [...]] : calcule les seuils anisotropiques tq seuil = #(classe membrane)/(#(classe membrane)+#(classe fond))\n\
\t -lMT %lf [%lf [...]] : calcule les seuils anisotropiques tq seuil = #(classe membrane>=seuil)/#(classe membrane)\n\
\t                        convention : 1ere fonction du fill correspond a (classe membrane)\n\
\t -[lMTmin | lMSFmin] %lf : ratio minimal pour les thres anisotropiques\n\
\t -[lMTmax | lMSFmax] %lf : ratio maximal pour les thres anisotropiques\n\
\t -[lMTnb | lMSFnb] %d : nombre de ratios entre lMTmin | lMSFmin et lMTmax | lMSFmax (lineaire)\n\
\t -lbin  <option>: realise la binarisation anisotropique de l'image via les seuils calcules par lMSF ou lMT et par la methode stipulee en option\n\
\t -normalize : normalise les histogrammes directionnels pour le fitting selon la valeur max entre lmin et lmax\n\
\t -wi : ecrit les images reponses projetees sur X, Y, Z + eventuelles binarisations\n\
\t -v : mode verbose\n\
\t -D : mode debug\n";


static char program[STRINGLENGTH];






int main( int argc, char *argv[] )
{
  local_par par;
  vt_Rep_Angles imageIn;
  vt_anisotropicHisto histo;

  vt_image *imRep=NULL, *imTht=NULL, *imPhi=NULL;
  vt_image imRepAnisotropes[3];
  vt_image imBin;
  char name[256];
  char prefix[256];

  double x;
  double *theBuf;
  double *theX, *theY, *theC, *theS;
  double parLevenOut[3][NBMAXPARAM];
  int axe;
  double xmin[3],xmax[3];
  int xlength[3];
  double lthres[3][NMAXMSF];

  double *thresholds[3];
  int i,j;
  int writeRepAnis = 0;
  double **ratios;


  clock_t start, stop;
  double elapsed;

  start = clock();



  /*--- initialisation des parametres ---*/
  MT_InitParam( &par );

  /*--- lecture des parametres ---*/
  MT_Parse( argc, argv, &par );

  /*--- lecture des images d'entree ---*/
  if(par.withAngles == 0)
  {
    sprintf( name, "%s", par.names.in );
    imRep = _VT_Inrimage( name );
    if ( imRep == (vt_image*)NULL )
    {
      //format .hdr
      i=0;
      while (name[i]!='\0')
      {
        i++;
      }
      name[i++] = '.';
      name[i++] = 'h';
      name[i++] = 'd';
      name[i++] = 'r';
      name[i]  = '\0';

      imRep = _VT_Inrimage( name );
      if ( imRep == (vt_image*)NULL )
      {
        //format .inr
        name[i-3] = 'i';
        name[i-2] = 'n';
        fprintf(stdout, "i=%d\n", i);
        fprintf(stdout, "name=%s\n", name);

        imRep = _VT_Inrimage( name );
        if ( imRep == (vt_image*)NULL )
        {
          fprintf(stderr, "%s unreadable\n", par.names.in );
          MT_ErrorParse("unable to read input imageA\n", 0);
        }
      }
    }
    fprintf(stdout, "Image d'entree : %s\n", name);
    imageIn.imrep = imRep;


    fprintf(stderr, "Allocation des angles non réalisée pour l'instant...");
    VT_FreeImage(imRep);
    return(-1);
  }
  else
  {
    sprintf( prefix, "%s", par.names.in );
    i=0;
    while (prefix[i]!='\0')
    {
      i++;
    }
    if (i>4 && prefix[i-4]=='.' &&
        ((prefix[i-3]=='h' && prefix[i-2]=='d' && prefix[i-1]=='r') ||
         (prefix[i-3]=='i' && prefix[i-2]=='n' && prefix[i-1]=='r')))
    {
      prefix[i-4]='\0';
      sprintf( name, "%s", par.names.in );
    }
    else
    {
      sprintf( name, "%s.hdr", prefix );
    }

    imRep = _VT_Inrimage( name );
    if ( imRep == (vt_image*)NULL )
    {
      //format .hdr
      i=0;
      while (name[i]!='\0')
      {
        i++;
      }
      name[i++] = '.';
      name[i++] = 'h';
      name[i++] = 'd';
      name[i++] = 'r';
      name[i]  = '\0';

      imRep = _VT_Inrimage( name );
      if ( imRep == (vt_image*)NULL )
      {
        //format .inr
        name[i-3] = 'i';
        name[i-2] = 'n';

        imRep = _VT_Inrimage( name );
        if ( imRep == (vt_image*)NULL )
        {
          fprintf(stderr, "%s unreadable\n", par.names.in );
          MT_ErrorParse("unable to read input imageA\n", 0);
        }
      }
    }
    if (_VT_VERBOSE_)
      fprintf(stdout, "Image d'entree : %s\n", name);
    imageIn.imrep = imRep;

    sprintf( name, "%s.theta.hdr", prefix );
    imTht = _VT_Inrimage( name );
    if ( imTht == (vt_image*)NULL )
    {
      i=0;
      while (name[i]!='\0')
      {
        i++;
      }
      name[i-3] = 'i';
      name[i-2] = 'n';
      imTht = _VT_Inrimage( name );
      if ( imTht == (vt_image*)NULL )
      {
        int j=0;
        i=0;
        while (prefix[i]!='\0')
        {
          if (prefix[i]=='.') j = i;
          i++;
        }
        if (j>0)
        {
          prefix[j]='\0';
          sprintf( name, "%s.theta.hdr", prefix );

          imTht = _VT_Inrimage( name );
          if ( imTht == (vt_image*)NULL )
          {
            i=0;
            while (name[i]!='\0')
            {
              i++;
            }
            name[i-3] = 'i';
            name[i-2] = 'n';
            imTht = _VT_Inrimage( name );
            if ( imTht == (vt_image*)NULL )
            {
                VT_FreeImage( imRep );
                fprintf(stderr, "%s unreadable\n", name);
                MT_ErrorParse("unable to read input imageC\n", 0);
            }
          }
        }
      }
    }
    if (_VT_VERBOSE_)
      fprintf(stdout, "Image d'entree theta : %s\n", name);
    imageIn.imtheta = imTht;

    sprintf( name, "%s.phi.hdr", prefix );
    imPhi = _VT_Inrimage( name );
    if ( imPhi == (vt_image*)NULL )
    {
      i=0;
      while (name[i]!='\0')
      {
        i++;
      }
      name[i-3] = 'i';
      name[i-2] = 'n';
      imPhi = _VT_Inrimage( name );
      if ( imPhi == (vt_image*)NULL )
      {
        VT_FreeImage( imTht );
        VT_FreeImage( imRep );
        fprintf(stderr, "%s unreadable\n", name);
        MT_ErrorParse("unable to read input imageD\n", 0);
      }
    }
    if (_VT_VERBOSE_)
      fprintf(stdout, "Image d'entree phi : %s\n", name);
    imageIn.imphi = imPhi;

  }


  if ( par.cptLevenberg == 1 && par.names.ext[0] != '\0' ) {     // initialisation de leven[] (tableau des parametres)
    if (_VT_VERBOSE_)
      fprintf(stdout, "Lecture du fichier %s...\n", par.names.ext);
    char text[256];
    char str[20];
    double par1, par2, par3;
    int s;
    int index=0;
    int nbGauss=par.nbGauss+0;
    int nbRayleigh=par.nbRayleigh+0;
    int nbRayleighPos=par.nbRayleighPos+0;
    int nbRayleighCentered=par.nbRayleighCentered+0;
    int nbXn=par.nbXn+0;
    FILE *fp = fopen( par.names.ext, "r" );
    if ( fp == NULL )
      MT_ErrorParse("unable to read param file\n", 0);
    while ( fgets( text, STRINGLENGTH, fp ) != NULL ) {
      index=nbGauss*3+nbRayleigh*3+nbXn*2+nbRayleighPos*3+nbRayleighCentered*2;
      if ( text[0] == '#' ) continue;
      s = sscanf( text, "%s %lf %lf %lf", str, &par1, &par2, &par3);
      if ( (strcmp(str, "Gauss") == 0) &&  (s == 4) ) {
        nbGauss ++;
        par.LM_types[nbGauss+nbRayleigh+nbXn+nbRayleighPos+nbRayleighCentered-1]=_GAUSS_;
        par.leven[index+0]=par1;
        par.leven[index+1]=par2;
        par.leven[index+2]=par3;
        continue;
      }
      if ( (strcmp(str, "Rayleigh") == 0) &&  (s == 4) ) {
        nbRayleigh ++;
        par.LM_types[nbGauss+nbRayleigh+nbXn+nbRayleighPos+nbRayleighCentered-1]=_RAYLEIGH_;
        par.leven[index+0]=par1;
        par.leven[index+1]=par2;
        par.leven[index+2]=par3;
        continue;
      }
      if ( (strcmp(str, "RayleighPos") == 0) &&  (s == 4) ) {
        nbRayleighPos ++;
        par.LM_types[nbGauss+nbRayleigh+nbXn+nbRayleighPos+nbRayleighCentered-1]=_RAYLEIGH_POS_;
        par.leven[index+0]=par1;
        par.leven[index+1]=par2;
        par.leven[index+2]=par3;
        continue;
      }
      if ( (strcmp(str, "RayleighCentered") == 0) &&  (s == 3) ) {
        nbRayleighCentered ++;
        par.LM_types[nbGauss+nbRayleigh+nbXn+nbRayleighPos+nbRayleighCentered-1]=_RAYLEIGH_CENTERED_;
        par.leven[index+0]=par1;
        par.leven[index+1]=par2;
        continue;
      }
      if ( (strcmp(str, "Xn") == 0) &&  (s == 3) ) {
        nbXn ++;
        par.LM_types[nbGauss+nbRayleigh+nbXn+nbRayleighPos+nbRayleighCentered-1]=_Xn_;
        par.leven[index+0]=par1;
        par.leven[index+1]=par2;
        continue;
      }
      fprintf( stderr, "lecture de '%s', probleme d'interpretation de '%s'... sscanf retourne %d \n",
                 par.names.ext, text, s);
      return(-1);
    }
    fclose( fp );
    par.nbGauss=nbGauss;
    par.nbRayleigh=nbRayleigh;
    par.nbRayleighPos=nbRayleighPos;
    par.nbRayleighCentered=nbRayleighCentered;
    par.nbXn=nbXn;
    par.nbleven=3*nbGauss+3*nbRayleigh+2*nbXn+3*nbRayleighPos+2*nbRayleighCentered;
    if (par.nbleven == 0)
      MT_ErrorParse("pas d'initialisation pour levenberg ?\n",0 );
  } else {
      if (par.cptLevenberg == 1)
      {
		//par.nbleven=3*par.nbGauss+3*par.nbRayleigh+2*par.nbXn+3*par.nbRayleighPos+2*par.nbRayleighCentered;
        if (par.nbleven == 0)
		  MT_ErrorParse("pas d'initialisation pour levenberg ?\n",0 );
	  }
  }

  if(_VT_VERBOSE_)
    fprintf(stdout, "Calculs histogrammes...\n");

  /*--- calculs ---*/
  if ( (par.wi == 1) || (par.binarise != _NO_BIN_))
    writeRepAnis = 1;

  if ( MT_ComputeHisto3D( imageIn, &histo, par.modehisto, par.inithistparam, par.normalize, par.lmin, par.initlmin, par.lmax, par.initlmax, writeRepAnis, imRepAnisotropes) != 1 ) {
    VT_FreeRep_Angles ( &imageIn );
    MT_ErrorParse("unable to compute histograms\n", 0 );
  }

  if(par.wi==1)
  {
    if(_VT_VERBOSE_)
      fprintf(stdout, "Ecriture des projections sur X, Y, Z...\n");

    VT_WriteInrimage( imRepAnisotropes);
    VT_WriteInrimage( imRepAnisotropes+1);
    VT_WriteInrimage( imRepAnisotropes+2);
  }


  if(par.cptLevenberg == 1)
  {
    if(_VT_VERBOSE_)
      fprintf(stdout, "Modelisation des histogrammes par methode Levenberg...\n");
    fprintf(stdout,"nbGauss=%d\tnbRayleigh=%d\tnbRayleighPos=%d\tnbRayleighCentered=%d\tnbXn=%d\n",par.nbGauss, par.nbRayleigh, par.nbRayleighPos, par.nbRayleighCentered, par.nbXn);
    fprintf(stdout, "leven={");
    for(i=0;i<par.nbleven;i++)
      fprintf(stdout, "%f  ",par.leven[i]);
    fprintf(stdout, "}\n");

    // X axis
    if (par.initlmin == 1)
      xmin[0] = histo.descriptorX.lower + ((double)((int)((par.lmin-histo.descriptorX.lower)/histo.descriptorX.precision)))*histo.descriptorX.precision;
    else
      xmin[0] = histo.descriptorX.lower;
    if (par.initlmax == 1)
      xmax[0] = histo.descriptorX.lower + ((double)((int)((par.lmax-histo.descriptorX.lower)/histo.descriptorX.precision)))*histo.descriptorX.precision;
    else
      xmax[0] = histo.descriptorX.upper;
    xlength[0] = histo.descriptorX.ncell;
    if (par.initlmin == 1 || par.initlmax == 1)
      xlength[0] = (int) ((xmax[0]-xmin[0])/histo.descriptorX.precision);

    // Y axis
    if (par.initlmin == 1)
      xmin[1] = histo.descriptorY.lower + ((double)((int)((par.lmin-histo.descriptorY.lower)/histo.descriptorY.precision)))*histo.descriptorY.precision;
    else
      xmin[1] = histo.descriptorY.lower;
    if (par.initlmax == 1)
      xmax[1] = histo.descriptorY.lower + ((double)((int)((par.lmax-histo.descriptorY.lower)/histo.descriptorY.precision)))*histo.descriptorY.precision;
    else
      xmax[1] = histo.descriptorY.upper;
    xlength[1] = histo.descriptorY.ncell;
    if (par.initlmin == 1 || par.initlmax == 1)
      xlength[1] = (int) ((xmax[1]-xmin[1])/histo.descriptorY.precision);


    // Z axis
    if (par.initlmax == 1)
      xmin[2] = histo.descriptorZ.lower + ((double)((int)((par.lmin-histo.descriptorZ.lower)/histo.descriptorZ.precision)))*histo.descriptorZ.precision;
    else
      xmin[2] = histo.descriptorZ.lower;
    if (par.initlmax == 1)
      xmax[2] = histo.descriptorZ.lower + ((double)((int)((par.lmax-histo.descriptorZ.lower)/histo.descriptorZ.precision)))*histo.descriptorZ.precision;
    else
      xmax[2] = histo.descriptorZ.upper;
    xlength[2] = histo.descriptorZ.ncell;
    if (par.initlmin == 1 || par.initlmax == 1)
      xlength[2] = (int) ((xmax[2]-xmin[2])/histo.descriptorZ.precision);



    /* 3 parametres pour la gaussienne
       amplitude -> gauss[0]
       moyenne   -> gauss[1]
       ecarttype -> gauss[2]
       2 parametres pour rayleigh 
       amplitude -> rayleigh[0]
       ecarttype -> rayleigh[1]
       2 parametres pour xn
       coeff     -> xn[0]
       exposant  -> xn[1]
    */
    for (i = 0 ; i < par.nbleven; i++)
      parLevenOut[0][i] = parLevenOut[1][i] = parLevenOut[2][i] = par.leven[i];

    for ( axe=0;axe<3;axe++)
    {
      theX =  (double*)malloc( 4*(xlength[axe])*sizeof(double) );
      theY =  theX;
      theY += xlength[axe];
      theC =  theY;
      theC += xlength[axe];
      theS =  theC;
      theS += xlength[axe];

      for ( i=0; i<xlength[axe]; i++ )
        theX[i] = theY[i] = theC[i] = theS[i] = 1.0;

      switch (axe) {
      case 0 :  // axe X
        if( _VT_VERBOSE_ )
          fprintf(stdout, "Axe X :\n");
        theBuf = (double*)histo.histX;
        for ( i=0, x=xmin[0]+histo.descriptorX.precision; i<xlength[0]; i++, x+=histo.descriptorX.precision) {
          theX[i] = x;
          theY[i] = theBuf[(int)((x-histo.descriptorX.lower)/histo.descriptorX.precision)];
        }
        break;
      case 1 :  // axe Y
        if( _VT_VERBOSE_ )
          fprintf(stdout, "Axe Y :\n");
        theBuf = (double*)histo.histY;
        for ( i=0, x=xmin[1]+histo.descriptorY.precision; i<xlength[1]; i++, x+=histo.descriptorY.precision) {
          theX[i] = x;
          theY[i] = theBuf[(int)((x-histo.descriptorY.lower)/histo.descriptorY.precision)];
        }
        break;
      case 2 :  // axe Z
        if( _VT_VERBOSE_ )
          fprintf(stdout, "Axe Z :\n");
        theBuf = (double*)histo.histZ;
        for ( i=0, x=xmin[2]+histo.descriptorZ.precision; i<xlength[2]; i++, x+=histo.descriptorZ.precision) {
          theX[i] = x;
          theY[i] = theBuf[(int)((x-histo.descriptorZ.lower)/histo.descriptorZ.precision)];
        }
        break;
      default :
        free(theX);
        theX=NULL;
        MT_ErrorParse("Unexpected axe value", -1);
      }

      j=0;
      for ( i=0 ; i<par.nbGauss+par.nbRayleigh+par.nbRayleighPos+par.nbRayleighCentered+par.nbXn ; i++)
      {
        switch ( par.LM_types[i] ) {
        case _GAUSS_ :
          fprintf( stdout, "#%2d INITIALISATION Gauss           : amplitude = %f moyenne = %f sigma = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1], parLevenOut[axe][j+2] );
          j+=3;
          break;
        case _RAYLEIGH_ :
          fprintf( stdout, "#%2d INITIALISATION Rayleigh        : amplitude = %f moyenne = %f sigma = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1], parLevenOut[axe][j+2]);
          j+=3;
          break;
        case _RAYLEIGH_POS_ :
          fprintf( stdout, "#%2d INITIALISATION RayleighPos     : amplitude = %f moyenne = %f sigma = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1], parLevenOut[axe][j+2]);
          j+=3;
          break;
        case _RAYLEIGH_CENTERED_ :
          fprintf( stdout, "#%2d INITIALISATION RayleighCentered: amplitude = %f sigma = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1]);
          j+=2;
          break;
        case _Xn_ :
          fprintf( stdout, "#%2d INITIALISATION Xn              : coeff = %f exposant = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1]);
          j+=2;
          break;
        default :
          free(theX);
          theX=NULL;
          MT_ErrorParse("Erreur, valeur inattendue dans LM_types[i]", -1);
        }
      }

      switch( par.nbGauss+par.nbRayleigh+par.nbXn+par.nbRayleighPos+par.nbRayleighCentered ) {
      default :
        MT_ErrorParse("unable to deal with such number of functions\n", 0);
      case 1 :
        switch ( par.LM_types[0] ) {
        case _GAUSS_ :
          (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                           theC, DOUBLE, theS, DOUBLE,
                                           xlength[axe],
                                           parLevenOut[axe], 3, _GaussianForLM );
          break;
        case _RAYLEIGH_ :
          (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                           theC, DOUBLE, theS, DOUBLE,
                                           xlength[axe],
                                           parLevenOut[axe], 3, _RayleighForLM );
          break;
        case _RAYLEIGH_POS_ :
          (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                           theC, DOUBLE, theS, DOUBLE,
                                           xlength[axe],
                                           parLevenOut[axe], 3, _RayleighPosForLM );
          break;
        case _RAYLEIGH_CENTERED_ :
          (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                           theC, DOUBLE, theS, DOUBLE,
                                           xlength[axe],
                                           parLevenOut[axe], 2, _RayleighCenteredForLM );
          break;
        case _Xn_ :
          (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                           theC, DOUBLE, theS, DOUBLE,
                                           xlength[axe],
                                           parLevenOut[axe], 2, _XnForLM );
          break;
        default :
          free(theX);
          theX=NULL;
          MT_ErrorParse("Mixture for LM not implemented yet", -1);
        }
        break;
      case 2 :
        switch ( par.LM_types[0] ) {
        case _GAUSS_ :
          switch ( par.LM_types[1] ) {
          case _GAUSS_ :
            (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                             theC, DOUBLE, theS, DOUBLE,
                                             xlength[axe],
                                             parLevenOut[axe], 6, _MixtureOf2GaussiansForLM );
            break;
          case _RAYLEIGH_ :
            (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                             theC, DOUBLE, theS, DOUBLE,
                                             xlength[axe],
                                             parLevenOut[axe], 6, _MixtureOf1Gaussian1RayleighForLM );
            break;
          case _RAYLEIGH_POS_ :
            (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                             theC, DOUBLE, theS, DOUBLE,
                                             xlength[axe],
                                             parLevenOut[axe], 6, _MixtureOf1Gaussian1RayleighPosForLM );
            break;
          case _RAYLEIGH_CENTERED_ :
            (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                             theC, DOUBLE, theS, DOUBLE,
                                             xlength[axe],
                                             parLevenOut[axe], 5, _MixtureOf1Gaussian1RayleighCenteredForLM );
            break;
          case _Xn_ :
            (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                             theC, DOUBLE, theS, DOUBLE,
                                             xlength[axe],
                                             parLevenOut[axe], 5, _MixtureOf1Gaussian1XnForLM );
            break;
          default :
            free(theX);
            theX=NULL;
            MT_ErrorParse("Mixture for LM not implemented yet", -1);
          }
          case _RAYLEIGH_ :
            switch ( par.LM_types[1] ) {
            case _RAYLEIGH_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 6, _MixtureOf2RayleighsForLM );
              break;
            case _RAYLEIGH_POS_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 6, _MixtureOf1Rayleigh1RayleighPosForLM );
              break;
            case _RAYLEIGH_CENTERED_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 5, _MixtureOf1Rayleigh1RayleighCenteredForLM );
              break;
            case _Xn_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 5, _MixtureOf1Rayleigh1XnForLM );
              break;
            case _GAUSS_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 6, _MixtureOf1Rayleigh1GaussianForLM );
              break;
            default :
              free(theX);
              theX=NULL;
              MT_ErrorParse("Mixture for LM not implemented yet", -1);
            }
            break;
          case _RAYLEIGH_POS_ :
            switch ( par.LM_types[1] ) {
            case _RAYLEIGH_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 6, _MixtureOf1RayleighPos1RayleighForLM );
              break;
            case _RAYLEIGH_POS_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 6, _MixtureOf2RayleighsPosForLM );
              break;
            case _RAYLEIGH_CENTERED_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 5, _MixtureOf1RayleighPos1RayleighCenteredForLM );
              break;
            case _Xn_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 5, _MixtureOf1RayleighPos1XnForLM );
              break;
            case _GAUSS_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 6, _MixtureOf1RayleighPos1GaussianForLM );
              break;
            default :
              free(theX);
              theX=NULL;
              MT_ErrorParse("Mixture for LM not implemented yet", -1);
            }
            break;
          case _RAYLEIGH_CENTERED_ :
            switch ( par.LM_types[1] ) {
            case _RAYLEIGH_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 5, _MixtureOf1RayleighCentered1RayleighForLM );
              break;
            case _RAYLEIGH_POS_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 5, _MixtureOf1RayleighCentered1RayleighPosForLM );
              break;
            case _RAYLEIGH_CENTERED_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 4, _MixtureOf2RayleighsCenteredForLM );
              break;
            case _Xn_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 4, _MixtureOf1RayleighCentered1XnForLM );
              break;
            case _GAUSS_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 5, _MixtureOf1RayleighCentered1GaussianForLM );
              break;
            default :
              free(theX);
              theX=NULL;
              MT_ErrorParse("Mixture for LM not implemented yet", -1);
            }
            break;
          case _Xn_ :
            switch ( par.LM_types[1] ) {
            case _Xn_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                                         theC, DOUBLE, theS, DOUBLE,
                                                         xlength[axe],
                                                         parLevenOut[axe], 4, _MixtureOf2XnsForLM );
              break;
            case _RAYLEIGH_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 5, _MixtureOf1Xn1RayleighForLM );
              break;
            case _RAYLEIGH_POS_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 5, _MixtureOf1Xn1RayleighPosForLM );
              break;
            case _RAYLEIGH_CENTERED_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 4, _MixtureOf1Xn1RayleighCenteredForLM );
              break;
            case _GAUSS_ :
              (void)Modeling1DDataWithLevenberg( theX, DOUBLE, theY, DOUBLE,
                                               theC, DOUBLE, theS, DOUBLE,
                                               xlength[axe],
                                               parLevenOut[axe], 5, _MixtureOf1Xn1GaussianForLM );
              break;
            default :
              free(theX);
              theX=NULL;
              MT_ErrorParse("Mixture for LM not implemented yet", -1);
            }
            break;
          default :
            free(theX);
            theX=NULL;
            MT_ErrorParse("Mixture for LM not implemented yet", -1);
        }
        break;
      }

      j=0;
      for ( i=0 ; i<par.nbGauss+par.nbRayleigh+par.nbXn+par.nbRayleighPos+par.nbRayleighCentered ; i++)
      {
        switch ( par.LM_types[i] ) {
        case _GAUSS_ :
          fprintf( stdout, "#%2d RESULTAT Gauss           : amplitude = %f moyenne = %f sigma = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1], parLevenOut[axe][j+2] );
          j+=3;
          break;
        case _RAYLEIGH_ :
          fprintf( stdout, "#%2d RESULTAT Rayleigh        : amplitude = %f moyenne = %f sigma = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1], parLevenOut[axe][j+2]);
          j+=3;
          break;
        case _RAYLEIGH_POS_ :
          fprintf( stdout, "#%2d RESULTAT RayleighPos     : amplitude = %f moyenne = %f sigma = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1], parLevenOut[axe][j+2]);
          j+=3;
          break;
        case _RAYLEIGH_CENTERED_ :
          fprintf( stdout, "#%2d RESULTAT RayleighCentered: amplitude = %f sigma = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1]);
          j+=2;
          break;
        case _Xn_ :
          fprintf( stdout, "#%2d RESULTAT Xn              : coeff = %f exposant = %f\n",
              i, parLevenOut[axe][j+0], parLevenOut[axe][j+1]);
          j+=2;
          break;
        default :
          free(theX);
          theX=NULL;
          MT_ErrorParse("Erreur, valeur inattendue dans LM_types[i]\n", -1);
        }
      }
      free(theX);
      theX=(double*)NULL;
    }
  }

  if (par.nMSF > 0 || par.nMT > 0)
  {
    if(par.cptLevenberg == 0)
      MT_ErrorParse("Trying to compute thresholds from filled histo without filling !\n", -1);

    double *buf0, *buf1, *bufX;
    double delta;
    double temp[3];

    double low;
    int ncell;

    vt_typeSeuil typeSeuil;
    if(par.nMSF > 0 ) typeSeuil = _MSF_;
    if(par.nMT > 0 ) typeSeuil = _MT_;
    if(par.nMSF > 0 && par.nMT > 0) typeSeuil = _UNKNOWN_;


    for( axe = 0 ; axe < 3 ; axe++)
    {
      switch (axe) {
      case 0 :
        low = histo.descriptorX.lower;
        delta = histo.descriptorX.precision;
        ncell = histo.descriptorX.ncell;
        break;
      case 1 :
        low = histo.descriptorY.lower;
        delta = histo.descriptorY.precision;
        ncell = histo.descriptorY.ncell;
        break;
      case 2 :
        low = histo.descriptorZ.lower;
        delta = histo.descriptorZ.precision;
        ncell = histo.descriptorZ.ncell;
        break;
      default :
        MT_ErrorParse("Axe inconnu\n", -1);
      }

      switch ( typeSeuil ) {
      case _MSF_ :
        buf0 = malloc(ncell*3*sizeof(double));
        buf1 = buf0;
        buf1+= ncell;
        bufX = buf1;
        bufX+= ncell;
        break;
      case _MT_ :
        buf0 = malloc(ncell*2*sizeof(double));
        bufX = buf0;
        bufX+= ncell;
        break;
      default :
        MT_ErrorParse("Type de seuil inconnu\n", -1);
      }

      switch( par.LM_types[0] ) {
      case _GAUSS_ :
        for(i = 0, x=low+0.5*delta; i < ncell; x+=delta, i++)
        {
          bufX[i] = x;
          buf0[i] = _GaussianForLM(x, parLevenOut[axe], temp);
          switch (typeSeuil) {
          case _MSF_ :
            buf1[i] = 0;
            int n=3;
            for (j = 1; j<par.nbGauss+par.nbRayleigh+par.nbXn+par.nbRayleighPos+par.nbRayleighCentered ; j++ ) {
              switch( par.LM_types[j] ) {
              case _GAUSS_ :
                  buf1[i] += _GaussianForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_ :
                  buf1[i] += _RayleighForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_POS_ :
                  buf1[i] += _RayleighPosForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_CENTERED_ :
                  buf1[i] += _RayleighCenteredForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              case _Xn_ :
                  buf1[i] += _XnForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              default :
                MT_ErrorParse("Erreur, valeur inattendue dans LM_types[n]", -1);
              }
            }
            break;
          default:
            break;
         }
        }
        break;
      case _RAYLEIGH_ :
        for(i = 0, x=low+0.5*delta; i < ncell; x+=delta, i++)
        {
          bufX[i] = x;
          buf0[i] = _RayleighForLM(x, parLevenOut[axe], temp);
          switch (typeSeuil) {
          case _MSF_ :
            buf1[i] = 0;
            int n=3;
            for (j = 1; j<par.nbGauss+par.nbRayleigh+par.nbXn+par.nbRayleighPos+par.nbRayleighCentered ; j++ ) {
              switch( par.LM_types[j] ) {
              case _GAUSS_ :
                  buf1[i] += _GaussianForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_ :
                  buf1[i] += _RayleighForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_POS_ :
                  buf1[i] += _RayleighPosForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_CENTERED_ :
                  buf1[i] += _RayleighCenteredForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              case _Xn_ :
                  buf1[i] += _XnForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              default :
                MT_ErrorParse("Erreur, valeur inattendue dans LM_types[n]", -1);
              }
            }
            break;
          default:
            break;
          }
        }
        break;
      case _RAYLEIGH_POS_ :
        for(i = 0, x=low+0.5*delta; i < ncell; x+=delta, i++)
        {
          bufX[i] = x;
          buf0[i] = _RayleighPosForLM(x, parLevenOut[axe], temp);
          switch (typeSeuil) {
          case _MSF_ :
            buf1[i] = 0;
            int n=3;
            for (j = 1; j<par.nbGauss+par.nbRayleigh+par.nbXn+par.nbRayleighPos+par.nbRayleighCentered ; j++ ) {
              switch( par.LM_types[j] ) {
              case _GAUSS_ :
                  buf1[i] += _GaussianForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_ :
                  buf1[i] += _RayleighForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_POS_ :
                  buf1[i] += _RayleighPosForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_CENTERED_ :
                  buf1[i] += _RayleighCenteredForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              case _Xn_ :
                  buf1[i] += _XnForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              default :
                MT_ErrorParse("Erreur, valeur inattendue dans LM_types[n]", -1);
              }
            }
            break;
          default:
            break;
          }
        }
        break;
      case _RAYLEIGH_CENTERED_ :
        for(i = 0, x=low+0.5*delta; i < ncell; x+=delta, i++)
        {
          bufX[i] = x;
          buf0[i] = _RayleighCenteredForLM(x, parLevenOut[axe], temp);
          switch (typeSeuil) {
          case _MSF_ :
            buf1[i] = 0;
            int n=2;
            for (j = 1; j<par.nbGauss+par.nbRayleigh+par.nbXn+par.nbRayleighPos+par.nbRayleighCentered ; j++ ) {
              switch( par.LM_types[j] ) {
              case _GAUSS_ :
                  buf1[i] += _GaussianForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_ :
                  buf1[i] += _RayleighForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_POS_ :
                  buf1[i] += _RayleighPosForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_CENTERED_ :
                  buf1[i] += _RayleighCenteredForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              case _Xn_ :
                  buf1[i] += _XnForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              default :
                MT_ErrorParse("Erreur, valeur inattendue dans LM_types[n]", -1);
              }
            }
            break;
          default:
            break;
          }
        }
        break;
      case _Xn_ :
        for(i = 0, x=low+0.5*delta; i < ncell; x+=delta, i++)
        {
          bufX[i] = x;
          buf0[i] = _XnForLM(x, parLevenOut[axe], temp);
          switch (typeSeuil) {
          case _MSF_ :
            buf1[i] = 0;
            int n=2;
            for (j = 1; j<par.nbGauss+par.nbRayleigh+par.nbXn+par.nbRayleighPos+par.nbRayleighCentered ; j++ ) {
              switch( par.LM_types[j] ) {
              case _GAUSS_ :
                  buf1[i] += _GaussianForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_ :
                  buf1[i] += _RayleighForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_POS_ :
                  buf1[i] += _RayleighPosForLM(x, parLevenOut[axe] + n, temp);
                  n+=3;
                break;
              case _RAYLEIGH_CENTERED_ :
                  buf1[i] += _RayleighCenteredForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              case _Xn_ :
                  buf1[i] += _XnForLM(x, parLevenOut[axe] + n, temp);
                  n+=2;
                break;
              default :
                MT_ErrorParse("Erreur, valeur inattendue dans LM_types[n]", -1);
              }
            }
            break;
          default :
            break;
          }
        }
        break;
      default :
        MT_ErrorParse("Erreur, valeur inattendue dans LM_types[0]", -1);
      }

      switch (typeSeuil) {
      case _MSF_ :
        for (i=0 ; i<par.nMSF ;i++)
        {
          double r0=buf0[0]/(buf0[0]+buf1[0]), r=buf0[1]/(buf0[1]+buf1[1]);

          j=1;
          while( (j<ncell-1) && (fabs(r0-par.membraneSurFond[i])<=fabs(r-par.membraneSurFond[i])))
          {
            j++;
            r0=r;
            r=buf0[j]/(buf0[j]+buf1[j]);
          }
          while( (j<ncell-1) && (fabs(r0-par.membraneSurFond[i])>=fabs(r-par.membraneSurFond[i])))
          {
            j++;
            r0=r;
            r=buf0[j]/(buf0[j]+buf1[j]);
          }

          if (j >= ncell)
          {
            MT_ErrorParse("Erreur, membrane/fond ne vérifie pas le ratio demandé\n", -1);
          }
          lthres[axe][i] = bufX[j];
        }
        break;
      case _MT_ :
        for (i=0 ; i<par.nMT ; i++)
        {
          j=0;
          double total = 0.0, sum = 0.0;
          while ( j < ncell  )
          {
            total += buf0[j++];
          }
          j=0;
          while ( j < ncell && ( sum / total < 1 - par.membraneTaux[i]) )
          {
            sum += buf0[j++];
          }
          if (j >= ncell)
          {
            MT_ErrorParse("Erreur, sum(membrane) ne vérifie pas le ratio demandé\n", -1);
          }
          lthres[axe][i] = bufX[j];
        }
        break;
      default :
        MT_ErrorParse("type de seuil inconnu\n\n", -1);
      }
      free(buf0);
      buf0=NULL;
    }

    fprintf(stdout, "Seuils anisotropiques :\n");
    switch (typeSeuil) {
    case _MSF_ :
      for (i = 0; i < par.nMSF ; i++ )
      {
        fprintf(stdout, "M/F : %f\n", par.membraneSurFond[i]);
        fprintf(stdout, "\tAxe X : %f\n", lthres[0][i]);
        fprintf(stdout, "\tAxe Y : %f\n", lthres[1][i]);
        fprintf(stdout, "\tAxe Z : %f\n", lthres[2][i]);
      }
      break;
    case _MT_ :
      for (i = 0; i < par.nMT ; i++ )
      {
        fprintf(stdout, "MT : %f\n", par.membraneTaux[i]);
        fprintf(stdout, "\tAxe X : %f\n", lthres[0][i]);
        fprintf(stdout, "\tAxe Y : %f\n", lthres[1][i]);
        fprintf(stdout, "\tAxe Z : %f\n", lthres[2][i]);
      }
      break;
    default :
      MT_ErrorParse("type de seuil inconnu\n\n", -1);
    }

    if ( 1 && par.nbin != 0 )
    {
      if(_VT_VERBOSE_)
        fprintf(stdout, "Calculs binarisations (Levenberg)...\n");
      double Temp[1];
      switch (typeSeuil) {
      case _MSF_ :
        for (i = 0; i < par.nMSF ; i++ )
        {
          for(j = 0 ; j < par.nbin ; j++)
          {
            switch ((par.binarise)[j]) {
            default :
            case _BIN_PROJ_ :
               sprintf( name, "%s.binProjMSF", prefix );
              break;
            case _BIN_DOT_ :
              sprintf( name, "%s.binDotMSF", prefix );
              break;
            case _BIN_DOT2_ :
              sprintf( name, "%s.binDot2MSF", prefix );
              break;
            case _BIN_AND_ :
              sprintf( name, "%s.binAndMSF", prefix );
              break;
            case _BIN_OR_ :
              sprintf( name, "%s.binOrMSF", prefix );
              break;
            }
            if (par.nMSF>1)
              sprintf(name, "%s.%1.4f.inr", name, par.membraneSurFond[i]);
            else
              sprintf(name, "%s.inr", name);

            if (MT_Binarise(imageIn, imRepAnisotropes,
                &imBin, name, lthres[0][i],lthres[1][i],lthres[2][i],
                par.binarise[j], Temp) < 0)
            {
              VT_FreeRep_Angles ( &imageIn );
              VT_FreeAnisotropicHisto(&histo);
              VT_FreeImage(imRepAnisotropes);
              VT_FreeImage(imRepAnisotropes+1);
              VT_FreeImage(imRepAnisotropes+2);
              MT_ErrorParse("unable to binarise the images\n", 0);
            }
            if(par.wi==1)
            {
              if (_VT_VERBOSE_)
                fprintf(stdout, "Ecriture de %s ...\n", name);
              VT_WriteInrimage( &imBin);
            }
            VT_FreeImage(&imBin);

          }
        }
        break;
      case _MT_ :
        for (i = 0; i < par.nMT ; i++ )
        {
          for(j = 0 ; j < par.nbin ; j++)
          {
            switch ((par.binarise)[j]) {
            default :
            case _BIN_PROJ_ :
               sprintf( name, "%s.binProjMT", prefix );
              break;
            case _BIN_DOT_ :
              sprintf( name, "%s.binDotMT", prefix );
              break;
            case _BIN_DOT2_ :
              sprintf( name, "%s.binDot2MT", prefix );
              break;
            case _BIN_AND_ :
              sprintf( name, "%s.binAndMT", prefix );
              break;
            case _BIN_OR_ :
              sprintf( name, "%s.binOrMT", prefix );
              break;
            }
            if (par.nMT>1)
              sprintf(name, "%s.%1.4f.inr", name, par.membraneTaux[i]);
            else
              sprintf(name, "%s.inr", name);
            if (MT_Binarise(imageIn, imRepAnisotropes,
                &imBin, name, lthres[0][i],lthres[1][i],lthres[2][i],
                par.binarise[j], Temp) < 0)
            {
              VT_FreeRep_Angles ( &imageIn );
              VT_FreeAnisotropicHisto(&histo);
              VT_FreeImage(imRepAnisotropes);
              VT_FreeImage(imRepAnisotropes+1);
              VT_FreeImage(imRepAnisotropes+2);
              MT_ErrorParse("unable to binarise the images\n", 0);
            }
            if(par.wi==1)
            {
              if (_VT_VERBOSE_)
                fprintf(stdout, "Ecriture de %s ...\n", name);
              VT_WriteInrimage( &imBin);
            }
            VT_FreeImage(&imBin);

          }
        }
        break;
      default :
        MT_ErrorParse("Type de seuil inconnu\n\n", -1);
      }
    }
  }





  if (0 && par.nratio > 0)
  {
    if(_VT_VERBOSE_)
      fprintf(stdout, "Calculs seuils anisotropes...\n");

    if( MT_ThresholdsFromHisto(thresholds, histo, par.ratio, par.nratio) != 1)
    {
      VT_FreeRep_Angles ( &imageIn );
      VT_FreeAnisotropicHisto(&histo);
      if (writeRepAnis == 1)
      {
        VT_FreeImage(imRepAnisotropes);
        VT_FreeImage(imRepAnisotropes+1);
        VT_FreeImage(imRepAnisotropes+2);
      }
      MT_ErrorParse("unable to compute the anisotropic thresholds\n", 0);
    }
  }

  if ( 0 && par.nbin != 0 )
  {
    if(_VT_VERBOSE_)
      fprintf(stdout, "Calculs binarisations...\n");

    if ( par.nratio <= 0)
    {
      VT_FreeRep_Angles ( &imageIn );
      VT_FreeAnisotropicHisto(&histo);
      VT_FreeImage(imRepAnisotropes);
      VT_FreeImage(imRepAnisotropes+1);
      VT_FreeImage(imRepAnisotropes+2);
      MT_ErrorParse("unable to binarise the images\n", 0);
    }
    else
    {
      ratios=malloc(par.nratio*sizeof(double*));
      for (i=0;i<par.nratio;i++)
      {
        if(_VT_VERBOSE_)
          fprintf(stdout, "\tRatio = %f...\n", par.ratio[i]);

        ratios[i]=malloc(par.nbin*sizeof(double));

//        retour = VT_Hysteresis( image, imres, par.low_threshold, par.high_threshold, &(par.cpar) );
        for (j=0;j<par.nbin;j++)
        {
          switch ((par.binarise)[j]) {
          default :
          case _BIN_PROJ_ :
            sprintf( name, "%s.binProj.%1.4f.inr", prefix, par.ratio[i] );
            break;
          case _BIN_DOT_ :
            sprintf( name, "%s.binDot.%1.4f.inr", prefix, par.ratio[i] );
            break;
          case _BIN_DOT2_ :
            sprintf( name, "%s.binDot2.%1.4f.inr", prefix, par.ratio[i] );
            break;
          case _BIN_AND_ :
            sprintf( name, "%s.binAnd.%1.4f.inr", prefix, par.ratio[i] );
            break;
          case _BIN_OR_ :
            sprintf( name, "%s.binOr.%1.4f.inr", prefix, par.ratio[i] );
            break;
          }
          if (MT_Binarise(imageIn, imRepAnisotropes,
              &imBin, name, thresholds[0][i],thresholds[1][i],thresholds[2][i],
              par.binarise[j], ratios[i]+j) < 0)
          {
            VT_FreeRep_Angles ( &imageIn );
            VT_FreeAnisotropicHisto(&histo);
            VT_FreeImage(imRepAnisotropes);
            VT_FreeImage(imRepAnisotropes+1);
            VT_FreeImage(imRepAnisotropes+2);
            for (i=0;i<par.nratio;i++)
            {
              free(ratios[i]);
              ratios[i]=NULL;
            }
            free(ratios);
            ratios=NULL;
            MT_ErrorParse("unable to binarise the images\n", 0);
          }
          if(par.wi==1)
          {
            if (_VT_VERBOSE_)
              fprintf(stdout, "Ecriture de %s ...\n", name);
            VT_WriteInrimage( &imBin);
          }
          VT_FreeImage(&imBin);
        }
      }
    }
  }
  else
  {
    ratios=(double**) NULL;
  }


  if ( par.writeHisto )
  {
    if (_VT_VERBOSE_)
      fprintf(stdout, "Ecriture de l'histogramme dans %s...\n", par.names.out);
    MT_WriteHisto3D( histo, par.cptLevenberg, par.LM_types, par.nbGauss+par.nbRayleigh+par.nbXn+par.nbRayleighPos+par.nbRayleighCentered, parLevenOut[0], parLevenOut[1], parLevenOut[2], thresholds, par.ratio, par.nratio, lthres[0], lthres[1], lthres[2], par.membraneSurFond, par.nMSF, par.membraneTaux, par.nMT, par.binarise, par.nbin, ratios, par.names.out, par.names.in);
  }

  /*--- liberations memoires ---*/
  VT_FreeRep_Angles ( &imageIn );
  VT_FreeAnisotropicHisto(&histo);
  if (writeRepAnis == 1)
  {
    VT_FreeImage(imRepAnisotropes);
    VT_FreeImage(imRepAnisotropes+1);
    VT_FreeImage(imRepAnisotropes+2);
  }
  if (par.nratio > 0)
    for (i=0;i<3;i++)
    {
      free(thresholds[i]);
      thresholds[i]=NULL;
    }
  if(par.nbin != 0)
  {
    for (i=0;i<par.nratio;i++)
    {
      free(ratios[i]);
      ratios[i]=NULL;
    }
    free(ratios);
    ratios=NULL;
  }
  stop = clock();
  elapsed = (double)(stop-start)/CLOCKS_PER_SEC;

  if(_VT_VERBOSE_)
    fprintf(stdout, "Elapsed time : \t%.1fs\n", elapsed);

  return( 1 );
}








static void MT_Parse( int argc,
                      char *argv[],
                      local_par *par )
{
  int i,j, nb, status, tmp;
  double tmin, tmax;
  int nt;
  char text[STRINGLENGTH];

  if ( VT_CopyName( program, argv[0] ) != 1 )
    VT_Error("Error while copying program name", (char*)NULL);
  if ( argc == 1 ) MT_ErrorParse("\n", 0 );

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
        MT_ErrorParse("\n", 1);
      }
      else if ( strcmp ( argv[i], "-v" ) == 0 ) {
        _VT_VERBOSE_ = 1;
      }
      else if ( strcmp ( argv[i], "-D" ) == 0 ) {
        _VT_DEBUG_ = 1;
      }

      /*--- traitement eventuel de l'image d'entree ---*/



      /*--- Paramètres d'input/output ---*/

      else if ( strcmp ( argv[i], "-without" ) == 0 ) {
        par->writeHisto = 0;
      }

      else if ( strcmp ( argv[i], "-wi" ) == 0 ) {
        par->wi = 1;
      }


      /* Parametres de calcul */

      // approximation histogramme (levenberg)

      else if ( strcmp ( argv[i], "-linit" ) == 0 ) {
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -linit...\n", 0 );
        strncpy( par->names.ext, argv[i], STRINGLENGTH );
        par->cptLevenberg = 1;
      }

      else if ( strcmp ( argv[i], "-gauss" ) == 0 ) { //
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -gauss...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 0]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -gauss...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -gauss...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 1]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -gauss...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -gauss...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 2]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -gauss...\n", 0 );
        par->nbleven += 3;
        par->nbGauss += 1;
		par->LM_types[par->nbGauss+par->nbRayleigh+par->nbRayleighPos+par->nbRayleighCentered+par->nbXn-1] = _GAUSS_;
        par->cptLevenberg = 1;
      }

      else if ( strcmp ( argv[i], "-rayleigh" ) == 0 ) { //
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -rayleigh...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 0]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -rayleigh...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -rayleigh...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 1]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -rayleigh...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -rayleigh...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 2]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -rayleigh...\n", 0 );
        par->nbleven += 3;
        par->nbRayleigh += 1;
		par->LM_types[par->nbGauss+par->nbRayleigh+par->nbRayleighPos+par->nbRayleighCentered+par->nbXn-1] = _RAYLEIGH_;
        par->cptLevenberg = 1;
      }

      else if ( strcmp ( argv[i], "-rayleighpos" ) == 0 ) { //
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -rayleighpos...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 0]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -rayleighpos...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -rayleighpos...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 1]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -rayleighpos...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -rayleighpos...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 2]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -rayleighpos...\n", 0 );
        par->nbleven += 3;
        par->nbRayleighPos += 1;
		par->LM_types[par->nbGauss+par->nbRayleigh+par->nbRayleighPos+par->nbRayleighCentered+par->nbXn-1] = _RAYLEIGH_POS_;
        par->cptLevenberg = 1;
      }

      else if ( strcmp ( argv[i], "-rayleighcentred" ) == 0 ||
				strcmp ( argv[i], "-rayleighcentered" ) == 0) { //
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -rayleighcentered...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 0]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -rayleighcentered...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -rayleighcentered...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 1]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -rayleighcentered...\n", 0 );
        par->nbleven += 2;
        par->nbRayleighCentered += 1;
		par->LM_types[par->nbGauss+par->nbRayleigh+par->nbRayleighPos+par->nbRayleighCentered+par->nbXn-1] = _RAYLEIGH_CENTERED_;
        par->cptLevenberg = 1;
      }

      else if ( strcmp ( argv[i], "-xn" ) == 0 ) { //
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -xn...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 0]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -xn...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -xn...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->leven[par->nbleven + 1]) ); // TODO
		if ( status <= 0 ) MT_ErrorParse( "parsing -xn...\n", 0 );
        par->nbleven += 2;
        par->nbXn += 1;
		par->LM_types[par->nbGauss+par->nbRayleigh+par->nbRayleighPos+par->nbRayleighCentered+par->nbXn-1] = _Xn_;
        par->cptLevenberg = 1;
      }

      else if ( strcmp ( argv[i], "-lmin" ) == 0 ) {
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -lmin...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->lmin) );
        par->cptLevenberg = 1;
        par->initlmin = 1;
        if ( status <= 0 ) MT_ErrorParse( "parsing -lmin...\n", 0 );
      }

      else if ( strcmp ( argv[i], "-lmax" ) == 0 ) {
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -lmax...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->lmax) );
        par->cptLevenberg = 1;
        par->initlmax = 1;
        if ( status <= 0 ) MT_ErrorParse( "parsing -lmax...\n", 0 );
      }

      else if ( (strcmp ( argv[i], "-lMSF" ) == 0) ) {
        i += 1;
        j=0;
        if ( i >= argc)    MT_ErrorParse( "parsing -lMSF ...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->membraneSurFond[0]) );
        if ( status <= 0 ) MT_ErrorParse( "parsing -lMSF ...\n", 0 );
        do {
          j ++;
          if (j>=NMAXMSF)
            MT_ErrorParse( "parsing -lMSF ... too much seuils\n", 0 );
          i++;

          if ( i < argc) {
            status = sscanf( argv[i], "%lf", &(par->membraneSurFond[j]) );
            if ( status <= 0 ) {
              i--;
              par->membraneSurFond[j] = -1;
            }
          }
          else {
            status=0;
          }
        } while(status>0);
        par->nMSF = j;
      }

      else if ( (strcmp ( argv[i], "-lMT" ) == 0) ) {
        i += 1;
        j=0;
        if ( i >= argc)    MT_ErrorParse( "parsing -lMT ...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->membraneTaux[0]) );
        if ( status <= 0 ) MT_ErrorParse( "parsing -lMT ...\n", 0 );
        do {
          j ++;
          if (j>=NMAXMSF)
            MT_ErrorParse( "parsing -lMT ... too much seuils\n", 0 );
          i++;

          if ( i < argc) {
            status = sscanf( argv[i], "%lf", &(par->membraneTaux[j]) );
            if ( status <= 0 ) {
              i--;
              par->membraneTaux[j] = -1;
            }
          }
          else {
            status=0;
          }
        } while(status>0);
        par->nMT = j;
      }

      else if ( strcmp ( argv[i], "-lMSFmin" ) == 0 ) {
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -lMSFmin...\n", 0 );
        status = sscanf( argv[i],"%lf",&(tmin) );
        if ( status <= 0 ) MT_ErrorParse( "parsing -lMSFmin...\n", 0 );
        i += 1;
        if ( strcmp ( argv[i], "-lMSFmax" ) != 0 ) MT_ErrorParse( "parsing -lMSFmin %lf -lMSFmax %lf...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -lMSFmax...\n", 0 );
        status = sscanf( argv[i],"%lf",&(tmax) );
        if ( status <= 0 ) MT_ErrorParse( "parsing -lMSFmax...\n", 0 );
        i += 1;
        if ( strcmp ( argv[i], "-lMSFnb" ) != 0 ) MT_ErrorParse( "parsing -lMSFmin %lf -lMSFmax %lf -lMSFnb %d...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -lMSFnb...\n", 0 );
        status = sscanf( argv[i],"%d",&(nt) );
        if ( status <= 0 ) MT_ErrorParse( "parsing -lMSFnb...\n", 0 );
        if (nt > NMAXMSF) MT_ErrorParse( "parsing -lMSFnb : too much ratios...\n", 0 );
        for (j=0 ; j<nt ; j++)
        {
          par->membraneSurFond[j]=tmin+(tmax-tmin)/(double)(nt-1)*(double)j;
        }
        par->nMSF = nt;
      }

      else if ( strcmp ( argv[i], "-lMTmin" ) == 0 ) {
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -lMTmin...\n", 0 );
        status = sscanf( argv[i],"%lf",&(tmin) );
        if ( status <= 0 ) MT_ErrorParse( "parsing -lMTmin...\n", 0 );
        i += 1;
        if ( strcmp ( argv[i], "-lMTmax" ) != 0 ) MT_ErrorParse( "parsing -lMTmin %lf -lMTmax %lf...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -lMTmax...\n", 0 );
        status = sscanf( argv[i],"%lf",&(tmax) );
        if ( status <= 0 ) MT_ErrorParse( "parsing -lMTmax...\n", 0 );
        i += 1;
        if ( strcmp ( argv[i], "-lMTnb" ) != 0 ) MT_ErrorParse( "parsing -lMTmin %lf -lMTmax %lf -lMTnb %d...\n", 0 );
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -lMTnb...\n", 0 );
        status = sscanf( argv[i],"%d",&(nt) );
        if ( status <= 0 ) MT_ErrorParse( "parsing -lMTnb...\n", 0 );
        if (nt > NMAXMSF) MT_ErrorParse( "parsing -lMTnb : too much ratios...\n", 0 );
        for (j=0 ; j<nt ; j++)
        {
          par->membraneTaux[j]=tmin+(tmax-tmin)/(double)(nt-1)*(double)j;
        }
        par->nMT = nt;
      }




      else if ( strcmp ( argv[i], "-lbin" ) == 0 ) {
       i += 1;
       if ( i >= argc)
       {
         i--;
         if (par->nbin>=NMAXBIN)
           MT_ErrorParse( "parsing -lbin ... too much binarisation modes\n", 0 );
         (par->binarise)[par->nbin++] = _BIN_PROJ_;
       }
       else
       {
         if ( (strcmp ( argv[i], "-proj") == 0) || (strcmp ( argv[i], "-projection" ) == 0) )
         {
           if (par->nbin>=NMAXBIN)
             MT_ErrorParse( "parsing -lbin ... too much binarisation modes\n", 0 );
           (par->binarise)[par->nbin++] = _BIN_PROJ_;
         }
         else {
           if ( (strcmp ( argv[i], "-dot2") == 0) )
           {
             if (par->nbin>=NMAXBIN)
               MT_ErrorParse( "parsing -lbin ... too much binarisation modes\n", 0 );
             (par->binarise)[par->nbin++] = _BIN_DOT2_;
           }
           else {
             if ( (strcmp ( argv[i], "-dot" ) == 0) )
             {
               if (par->nbin>=NMAXBIN)
                 MT_ErrorParse( "parsing -lbin ... too much binarisation modes\n", 0 );
               (par->binarise)[par->nbin++] = _BIN_DOT_;
             }
             else
               if ( (strcmp ( argv[i], "-and" ) == 0) )
               {
                 if (par->nbin>=NMAXBIN)
                   MT_ErrorParse( "parsing -lbin ... too much binarisation modes\n", 0 );
                 (par->binarise)[par->nbin++] = _BIN_AND_;
               }
               else
               {
                 if ( strcmp ( argv[i], "-or" ) == 0)
                 {
                   if (par->nbin>=NMAXBIN)
                     MT_ErrorParse( "parsing -lbin ... too much binarisation modes\n", 0 );
                   (par->binarise)[par->nbin++] = _BIN_OR_;
                 }
                 else
                 {
                   if ( (strcmp ( argv[i], "-all" ) == 0) ) {
                     par->nbin=0;
                     for (j=0;j<=NMAXBIN;j++)
                     {
                       if((j > (int)_NO_BIN_) && (j < (int)_BIN_ALL_) )
                       {
                         if (par->nbin>=NMAXBIN)
                           MT_ErrorParse( "parsing -lbin ... too much binarisation modes\n", 0 );
                         (par->binarise)[par->nbin++] = (vt_binmode)j;
                       }
                     }
                   }
                   else
                   {
                     i--;
                     if (par->nbin>=NMAXBIN)
                       MT_ErrorParse( "parsing -lbin ... too much binarisation modes\n", 0 );
                     (par->binarise)[par->nbin++] = _BIN_PROJ_;
                   }
                 }
               }
           }
         }
       }

     }

      else if ( strcmp ( argv[i], "-lbinproj" ) == 0 ) {
        if (par->nbin>=NMAXBIN)
          MT_ErrorParse( "parsing -lbinproj ... too much binarisation modes\n", 0 );
        (par->binarise)[par->nbin++] = _BIN_PROJ_;
      }

      else if ( strcmp ( argv[i], "-lbindot2" ) == 0 ) {
        if (par->nbin>=NMAXBIN)
          MT_ErrorParse( "parsing -lbindot2 ... too much binarisation modes\n", 0 );
        (par->binarise)[par->nbin++] = _BIN_DOT2_;
      }

     else if ( strcmp ( argv[i], "-lbindot" ) == 0 ) {
       if (par->nbin>=NMAXBIN)
         MT_ErrorParse( "parsing -lbindot ... too much binarisation modes\n", 0 );
       (par->binarise)[par->nbin++] = _BIN_DOT_;
     }

     else if ( strcmp ( argv[i], "-lbinor" ) == 0 ) {
       if (par->nbin>=NMAXBIN)
         MT_ErrorParse( "parsing -lbinor ... too much binarisation modes\n", 0 );
       (par->binarise)[par->nbin++] = _BIN_OR_;
     }

     else if ( strcmp ( argv[i], "-lbinand" ) == 0 ) {
       if (par->nbin>=NMAXBIN)
         MT_ErrorParse( "parsing -lbinand ... too much binarisation modes\n", 0 );
       (par->binarise)[par->nbin++] = _BIN_AND_;
     }

     else if ( strcmp ( argv[i], "-lbinall" ) == 0 ) {
       par->nbin=0;
       for (j=0;j<=NMAXBIN;j++)
       {
         if((j > (int)_NO_BIN_) && (j < (int)_BIN_ALL_) )
         {
           if (par->nbin>=NMAXBIN)
             MT_ErrorParse( "parsing -lbinall ... too much binarisation modes\n", 0 );
           (par->binarise)[par->nbin++] = (vt_binmode)j;
         }
       }
     }

      // calcul histogrammes

      else if ( strcmp ( argv[i], "-histo-proj" ) == 0 ) {
        par->modehisto = _HIST_PROJ_;
      }

      else if ( strcmp ( argv[i], "-histo-proj-2" ) == 0 ) {
        par->modehisto = _HIST_PROJ_2_;
      }

      else if ( strcmp ( argv[i], "-histo-n" ) == 0 ) {
        par->modehisto = _HIST_N_;
      }

      else if ( strcmp ( argv[i], "-histo-n-2" ) == 0 ) {
        par->modehisto = _HIST_N_2_;
      }

      else if ( strcmp ( argv[i], "-noangle" ) == 0 ) {
        par->withAngles = 0;
      }

      else if ( strcmp ( argv[i], "-ncell" ) == 0 ) {
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -ncell...\n", 0 );
        status = sscanf( argv[i],"%d",&(par->inithistparam.ncell) );
        par->inithistparam.initncell=1;
        if ( status <= 0 ) MT_ErrorParse( "parsing -ncell...\n", 0 );
      }

      else if ( strcmp ( argv[i], "-minhist" ) == 0 ) {
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -minhist...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->inithistparam.min) );
        par->inithistparam.initmin=1;
        if ( status <= 0 ) MT_ErrorParse( "parsing -minhist...\n", 0 );
      }

      else if ( strcmp ( argv[i], "-maxhist" ) == 0 ) {
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -maxhist...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->inithistparam.max) );
        par->inithistparam.initmax=1;
        if ( status <= 0 ) MT_ErrorParse( "parsing -maxhist...\n", 0 );
      }

      else if ( strcmp ( argv[i], "-precision" ) == 0 ) {
        i += 1;
        if ( i >= argc)    MT_ErrorParse( "parsing -precision...\n", 0 );
        status = sscanf( argv[i],"%lf",&(par->inithistparam.precision) );
        par->inithistparam.initprecision=1;
        if ( status <= 0 ) MT_ErrorParse( "parsing -precision...\n", 0 );
      }

      else if ( strcmp ( argv[i], "-normalize" ) == 0 ) {
        par->normalize = 1;
      }


      /* parallelism
       */
      else if ( strcmp ( argv[i], "-parallel" ) == 0 ) {
        setMaxChunks( 100 );
      }

      else if ( strcmp ( argv[i], "-no-parallel" ) == 0 ) {
        setMaxChunks( 1 );
      }

      else if ( strcmp ( argv[i], "-max-chunks" ) == 0 ) {
        i ++;
        if ( i >= argc)    MT_ErrorParse( "-max-chunks", 0 );
        status = sscanf( argv[i], "%d", &tmp );
        if ( status <= 0 ) MT_ErrorParse( "-max-chunks", 0 );
        if ( tmp >= 1 ) setMaxChunks( tmp );
      }

      else if ( strcmp ( argv[i], "-parallel-scheduling" ) == 0 ||
                ( strcmp ( argv[i], "-ps" ) == 0 && argv[i][3] == '\0') ) {
        i ++;
        if ( i >= argc)    MT_ErrorParse( "-parallel-scheduling", 0 );
        if ( strcmp ( argv[i], "default" ) == 0 ) {
          setOpenMPScheduling( _DEFAULT_SCHEDULING_ );
        }
        else if ( strcmp ( argv[i], "static" ) == 0 ) {
          setOpenMPScheduling( _STATIC_SCHEDULING_ );
        }
        else if ( strcmp ( argv[i], "dynamic-one" ) == 0 ) {
          setOpenMPScheduling( _DYNAMIC_ONE_SCHEDULING_ );
        }
        else if ( strcmp ( argv[i], "dynamic" ) == 0 ) {
          setOpenMPScheduling( _DYNAMIC_SCHEDULING_ );
        }
        else if ( strcmp ( argv[i], "guided" ) == 0 ) {
          setOpenMPScheduling( _GUIDED_SCHEDULING_ );
        }
        else {
          MT_ErrorParse( "-parallel-scheduling", 0 );
        }
      }


      /*--- option inconnue ---*/
      else {
        sprintf(text,"unknown option %s\n",argv[i]);
        MT_ErrorParse(text, 0);
      }
    }
    /*--- saisie des noms d'image in / fichier out ---*/
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
        MT_ErrorParse("too much file names when parsing\n", 0 );
    }
    i += 1;
  }

  /*--- s'il n'y a pas assez de noms ... ---*/
  if (nb == 0) {
    strcpy( par->names.in,  "<" );  /* standart input */
    strcpy( par->names.out, ">" );  /* standart output */
  }
  if (nb == 1)
  {
    strcpy( par->names.out, ">" );  /* standart output */
  }

}







static void MT_ErrorParse( char *str, int flag )
{
  (void)fprintf(stderr,"Usage : %s %s\n",program, usage);
  if ( flag == 1 ) (void)fprintf(stderr,"%s",detail);
  (void)fprintf(stderr,"Erreur : %s",str);
  exit(0);
}








static void MT_InitParam( local_par *par )
{
  int i;
  VT_Names( &(par->names) );

  par->writeHisto = 1;
  par->withAngles = 1;
  MT_InitHistParam(&(par->inithistparam));
  par->wi = 0;
  for (i=0;i<NMAXRATIO;i++)
    (par->ratio)[i] = -1;
  par->nratio=0;
  for (i=0;i<NMAXBIN;i++)
    (par->binarise)[i] = _NO_BIN_;
  par->nbin = 0;
  par->modehisto = _HIST_N_2_;
  par->cptLevenberg = 0;
  par->lmin = 0;
  par->lmax = 0;
  par->initlmin = 0;
  par->initlmax = 0;
  par->nbGauss = 0;
  par->nbRayleigh = 0;
  par->nbRayleighPos = 0;
  par->nbRayleighCentered = 0;
  par->nbXn = 0;
  for (i=0;i<NBMAXPARAM;i++)
    par->leven[i] = 0.0;
  for (i=0;i<NBMAXLM;i++)
    par->LM_types[i] = _NO_TYPE_;
  par->nbleven = 0;
  for (i=0;i<NMAXMSF;i++)
    par->membraneSurFond[i] = par->membraneTaux[i] = -1;
  par->nMSF = 0;
  par->nMT = 0;
  par->normalize = 0;
}


#include <vt_bdd_amincir.h>

#ifndef NO_PROTO
int _VT_BDD_GONGBERTRAND( vt_image *im /* image to be thinned */,
				 vt_amincir *par /* parameters */ )
#else
int _VT_BDD_GONGBERTRAND( im, par )
vt_image *im;    /* image to be thinned */
vt_amincir *par; /* parameters */
#endif
{
    vt_pt_amincir *liste;
    int ln, lnb;
    int *in, *up, *no, *ea, *bo, *so, *we;
    int tup[3], tno[3], tea[3], tbo[3], tso[3], twe[3];
    int vois[3][3][3], offset[3][3][3], voisins[26];
    register unsigned char *b;
    register int i, j, k, l, ind, dx, dxy;
    int nbPerm, cycle, nbd, nbdel;
    char message[256];
    int ret = 0;

    if ( VT_Test1Image( im, "_VT_BDD_GONGBERTRAND" ) == -1 ) return( -1 );
    if ( (im->type != UCHAR) ) return( -1 );
    
    liste = _VT_ThinPtList( im, (int)VT_3D, &lnb );
    if ( (liste == (vt_pt_amincir*)NULL) || (lnb <= 0) ) {
	VT_Error( "unable to compute list of points", "_VT_BDD_GONGBERTRAND" );
	return( 0 );
    }
    
    /*--- preparation ---*/
    b = (unsigned char*)(im->buf);
    tup[0] = 0;   tup[1] = 0;   tup[2] = 1;
    tno[0] = 0;   tno[1] = 1;   tno[2] = 0;
    tea[0] = 1;   tea[1] = 0;   tea[2] = 0;
    tbo[0] = 0;   tbo[1] = 0;   tbo[2] = -1;
    tso[0] = 0;   tso[1] = -1;  tso[2] = 0;
    twe[0] = -1;  twe[1] = 0;   twe[2] = 0;
    up = tup;   no = tno;   ea = tea;
    bo = tbo;   so = tso;   we = twe;
    nbPerm = 3;
    dx = im->dim.x;   dxy = im->dim.x * im->dim.y;
    offset[1][1][1] = 0;
    offset[1][0][1] = - dx;
    offset[1][2][1] = dx;
    offset[0][1][1] = - 1;
    offset[0][0][1] = offset[1][0][1] - 1;
    offset[0][2][1] = offset[1][2][1] - 1;
    offset[2][1][1] = 1;
    offset[2][0][1] = offset[1][0][1] + 1;
    offset[2][2][1] = offset[1][2][1] + 1;
    for ( i = 0; i < 3; i++ )
	for ( j = 0; j < 3; j++ ) {
	    offset[i][j][0] = offset[i][j][1] - dxy;
	    offset[i][j][2] = offset[i][j][1] + dxy;
	}
    ln = lnb;
    /*--- boucle generale ---*/
    do {
	/*--- boucle des 6 sous-cycles ---*/
	nbdel = 0;
	for (cycle = 0; cycle < 6; cycle++ ) {
	    nbd = 0;
	    /*--- liste des points ---*/
	    for ( l = 0; l < ln; l++ ) {
		/*--- saisie du voisinage ---*/
		ind = liste[l].pt.z * dxy + liste[l].pt.y * dx + liste[l].pt.x;
		if ( liste[l].inside == 1 ) {
		    for ( i = 0; i < 3; i++ )
		    for ( j = 0; j < 3; j++ )
		    for ( k = 0; k < 3; k++ )
			vois[i][j][k] = (int)(b[ ind + offset[i][j][k] ]);
		} else {
		    for ( i = -1; i < 2; i++ )
		    for ( j = -1; j < 2; j++ )
		    for ( k = -1; k < 2; k++ ) {
			if ( (liste[l].pt.x+i >= 0) && (liste[l].pt.x+i < dx) &&
			     (liste[l].pt.y+j >= 0) && (liste[l].pt.y+j < im->dim.y) &&
			     (liste[l].pt.z+k >= 0) && (liste[l].pt.z+k < im->dim.z) )
			    vois[1+i][1+j][1+k] = (int)(b[ ind + offset[1+i][1+j][1+k] ]);
			else
			    vois[1+i][1+j][1+k] = (int)0;
		    }
		}

		/*--- on change de representation de voisinage ---*/
		voisins[ 0] = vois[0][0][0];  voisins[ 1] = vois[1][0][0];   voisins[ 2] = vois[2][0][0];  
		voisins[ 3] = vois[0][1][0];  voisins[ 4] = vois[1][1][0];   voisins[ 5] = vois[2][1][0];  
		voisins[ 6] = vois[0][2][0];  voisins[ 7] = vois[1][2][0];   voisins[ 8] = vois[2][2][0];  
		voisins[ 9] = vois[0][0][1];  voisins[10] = vois[1][0][1];   voisins[11] = vois[2][0][1];  
		voisins[12] = vois[0][1][1];                                 voisins[13] = vois[2][1][1];  
		voisins[14] = vois[0][2][1];  voisins[15] = vois[1][2][1];   voisins[16] = vois[2][2][1];  
		voisins[17] = vois[0][0][2];  voisins[18] = vois[1][0][2];   voisins[19] = vois[2][0][2];  
		voisins[20] = vois[0][1][2];  voisins[21] = vois[1][1][2];   voisins[22] = vois[2][1][2];  
		voisins[23] = vois[0][2][2];  voisins[24] = vois[1][2][2];   voisins[25] = vois[2][2][2];  
		
		switch ( cycle ) {
		case 0 :
		    ret = _VT_GB_IsSimple0( voisins );
		    break;
		case 1 :
		    ret = _VT_GB_IsSimple1( voisins );
		    break;
		case 2 :
		    ret = _VT_GB_IsSimple2( voisins );
		    break;
		case 3 :
		    ret = _VT_GB_IsSimple3( voisins );
		    break;
		case 4 :
		    ret = _VT_GB_IsSimple4( voisins );
		    break;
		case 5 :
		    ret = _VT_GB_IsSimple5( voisins );
		}
		if ( ret == 1 ) liste[l].status = VT_TOBEDELETED;
		else            liste[l].status = VT_DELETABLE;
	    }
	    /*--- fin du traitement de la liste ---*/
	    /*--- on modifie l'image et on rearrange la liste ---*/
	    i = 0;
	    for ( l = 0; l < ln; l++ ) {
		switch ( liste[l].status ) {
		case VT_DELETABLE :
		    /*--- on garde le point dans la liste ---*/
		    liste[i++] = liste[l];
		    break;
		case VT_TOBEDELETED :
		    /*--- on efface le point ---*/
		    ind = liste[l].pt.z * dxy + liste[l].pt.y * dx + liste[l].pt.x;
		    b[ind] = (unsigned char)0;
		    nbd ++;
		    break;
		case VT_UNDELETABLE :
		    /*--- on ne fait rien ---*/
		    break;
		default :
		    /*--- on s'inquiete ---*/
		    VT_Error("point with unknown label","_VT_BDD_GONGBERTRAND");
		}
	    }
	    ln = i;
	    nbdel += nbd;
	    /*--- fin d'un sous-cycle : on prepare le suivant ---*/
	    for ( i = 0; i < nbPerm; i++ ) {
		in = up;
		up = no;
		no = ea;
		ea = bo;
		bo = so;
		so = we;
		we = in;
	    }
	    if (nbPerm == 3) nbPerm = 4;
	    else nbPerm = 3;
	    if (cycle == 4) nbPerm = 1;
	    /*--- message ---*/
	    sprintf( message,"     subiteration # %d, deleted points = %d", cycle+1, nbd );
	    VT_Message( message, "_VT_BDD_GONGBERTRAND" );
	}
	/*--- message ---*/
	sprintf( message," iteration, deleted points = %d", nbdel );
	VT_Message( message, "_VT_BDD_GONGBERTRAND" );
    } while ( (nbdel > 0) && (ln > 0) );
    VT_Free( (void**)&liste );
    return( 1 );
}

#ifndef NO_PROTO
int _VT_BDD_GREG_PLANES( vt_image *im /* image to be thinned */,
				vt_amincir *par /* parameters */ )
#else
int _VT_BDD_GREG_PLANES( im, par )
vt_image *im; /* image to be thinned */
vt_amincir *par; /* parameters */
#endif
{
    vt_pt_amincir *liste;
    int ln, lnb;
    int *in, *up, *no, *ea, *bo, *so, *we;
    int tup[3], tno[3], tea[3], tbo[3], tso[3], twe[3];
    int v[3][3][3], offset[3][3][3], w[3][3][3], voisins[26];
    register unsigned char *b;
    register int i, j, k, l, ind, dx, dxy;
    int nbPerm, cycle, nbd, nbdel, npt;
    char message[256];

    if ( VT_Test1Image( im, "_VT_BDD_GREG_PLANES" ) == -1 ) return( -1 );
	if ( (im->type != UCHAR) ) return( -1 );
	
    liste = _VT_ThinPtList( im, (int)VT_3D, &lnb );
    if ( (liste == (vt_pt_amincir*)NULL) || (lnb <= 0) ) {
	VT_Error( "unable to compute list of points", "_VT_BDD_GREG_PLANES" );
	return( 0 );
    }
    
    /*--- preparation ---*/
    b = (unsigned char*)(im->buf);
    tup[0] = 0;   tup[1] = 0;   tup[2] = 1;
    tno[0] = 0;   tno[1] = 1;   tno[2] = 0;
    tea[0] = 1;   tea[1] = 0;   tea[2] = 0;
    tbo[0] = 0;   tbo[1] = 0;   tbo[2] = -1;
    tso[0] = 0;   tso[1] = -1;  tso[2] = 0;
    twe[0] = -1;  twe[1] = 0;   twe[2] = 0;
    up = tup;   no = tno;   ea = tea;
    bo = tbo;   so = tso;   we = twe;
    nbPerm = 3;
    dx = im->dim.x;   dxy = im->dim.x * im->dim.y;
    /*--- tableaux d'offsets ---*/
    offset[1][1][1] = 0;
    offset[1][0][1] = - dx;
    offset[1][2][1] = dx;
    offset[0][1][1] = - 1;
    offset[0][0][1] = offset[1][0][1] - 1;
    offset[0][2][1] = offset[1][2][1] - 1;
    offset[2][1][1] = 1;
    offset[2][0][1] = offset[1][0][1] + 1;
    offset[2][2][1] = offset[1][2][1] + 1;
    for ( i = 0; i < 3; i++ )
    for ( j = 0; j < 3; j++ ) {
	offset[i][j][0] = offset[i][j][1] - dxy;
	offset[i][j][2] = offset[i][j][1] + dxy;
    }
    
    ln = lnb;
    /*--- boucle generale ---*/
    do {
	/*--- boucle des 6 sous-cycles ---*/
	nbdel = 0;
	for (cycle = 0; cycle < 6; cycle++ ) {
	    nbd = 0;
	    /*--- liste des points ---*/
	    for ( l = 0; l < ln; l++ ) {
		/*--- saisie du voisinage ---*/
		ind = liste[l].pt.z * dxy + liste[l].pt.y * dx + liste[l].pt.x;
		if ( liste[l].inside == 1 ) {
		    for ( i = 0; i < 3; i++ )
		    for ( j = 0; j < 3; j++ )
		    for ( k = 0; k < 3; k++ )
			v[i][j][k] = (int)(b[ ind + offset[i][j][k] ]);
		} else {
		    for ( i = -1; i < 2; i++ )
		    for ( j = -1; j < 2; j++ )
		    for ( k = -1; k < 2; k++ ) {
			if ( (liste[l].pt.x+i >= 0) && (liste[l].pt.x+i < dx) &&
			     (liste[l].pt.y+j >= 0) && (liste[l].pt.y+j < im->dim.y) &&
			     (liste[l].pt.z+k >= 0) && (liste[l].pt.z+k < im->dim.z) )
			    v[1+i][1+j][1+k] = (int)(b[ ind + offset[1+i][1+j][1+k] ]);
			else
			    v[1+i][1+j][1+k] = (int)0;
		    }
		}
		/*--- si la condition 2a n'est pas verifiee ---*/
		/*--- point + up appartient au fond         ---*/
		if ( v[1+up[0]][1+up[1]][1+up[2]] != 0 ) continue;
		
		/*--- si la condition 2b n'est pas verifiee ---*/
		/*--- point + bo appartient a l'objet       ---*/
		if ( v[1+bo[0]][1+bo[1]][1+bo[2]] == 0 ) continue;

		/*--- on change de representation de voisinage ---*/
		voisins[ 0] = v[0][0][0];  voisins[ 1] = v[1][0][0];   voisins[ 2] = v[2][0][0];  
		voisins[ 3] = v[0][1][0];  voisins[ 4] = v[1][1][0];   voisins[ 5] = v[2][1][0];  
		voisins[ 6] = v[0][2][0];  voisins[ 7] = v[1][2][0];   voisins[ 8] = v[2][2][0];  
		voisins[ 9] = v[0][0][1];  voisins[10] = v[1][0][1];   voisins[11] = v[2][0][1];  
		voisins[12] = v[0][1][1];                              voisins[13] = v[2][1][1];  
		voisins[14] = v[0][2][1];  voisins[15] = v[1][2][1];   voisins[16] = v[2][2][1];  
		voisins[17] = v[0][0][2];  voisins[18] = v[1][0][2];   voisins[19] = v[2][0][2];  
		voisins[20] = v[0][1][2];  voisins[21] = v[1][1][2];   voisins[22] = v[2][1][2];  
		voisins[23] = v[0][2][2];  voisins[24] = v[1][2][2];   voisins[25] = v[2][2][2];  
		
		/*--- ce n'est pas un point simple                            ---*/
		if ( _VT_IsSimple( voisins ) != 1 ) continue;
		
		/*--- on efface les points qui vont disparaitre ---*/
		for ( i = 0; i < 3; i++ )
		for ( j = 0; j < 3; j++ )
		for ( k = 0; k < 3; k++ )
		    w[i][j][k] = ( v[i][j][k] == VT_WILLBEDELETED ) ? 0 : v[i][j][k];
		
		/*--- on change de representation de voisinage ---*/
		voisins[ 0] = w[0][0][0];  voisins[ 1] = w[1][0][0];   voisins[ 2] = w[2][0][0];  
		voisins[ 3] = w[0][1][0];  voisins[ 4] = w[1][1][0];   voisins[ 5] = w[2][1][0];  
		voisins[ 6] = w[0][2][0];  voisins[ 7] = w[1][2][0];   voisins[ 8] = w[2][2][0];  
		voisins[ 9] = w[0][0][1];  voisins[10] = w[1][0][1];   voisins[11] = w[2][0][1];  
		voisins[12] = w[0][1][1];                              voisins[13] = w[2][1][1];  
		voisins[14] = w[0][2][1];  voisins[15] = w[1][2][1];   voisins[16] = w[2][2][1];  
		voisins[17] = w[0][0][2];  voisins[18] = w[1][0][2];   voisins[19] = w[2][0][2];  
		voisins[20] = w[0][1][2];  voisins[21] = w[1][1][2];   voisins[22] = w[2][1][2];  
		voisins[23] = w[0][2][2];  voisins[24] = w[1][2][2];   voisins[25] = w[2][2][2];  
				
		/*--- ce n'est pas un point simple                            ---*/
		if ( _VT_IsSimple( voisins ) != 1 ) continue;

		/*--- si on est arrive ici, c'est un point effacable ---*/
		if ( par->bool_shrink == 1 ) {
		    liste[l].status = VT_TOBEDELETED;
		    b[ind] = VT_WILLBEDELETED;
		    continue;
		}
		
		/*--- condition geometrique d'arret      ---*/
		/*--- Remarque :                                   ---*/
		/*--- Selon le voisinage sur lequel on fait porter ---*/
		/*--- cette condition (v ou w = v - pts effaces)   ---*/
		/*--- le resultat est different.                   ---*/
		/*--- En choisissant v, le resultat semble moins   ---*/
		/*--- "bruite" (moins de branches parasites).      ---*/
		/*--- nombre de points dans le voisinage ---*/
		npt = 0;
		for ( i = 0; i < 3; i++ )
		for ( j = 0; j < 3; j++ ) {
		    if ( v[i][j][0] != 0 ) npt ++;
		    if ( v[i][j][2] != 0 ) npt ++;
		}
		for ( i = 0; i < 3; i++ ) {
		    if ( v[i][0][1] != 0 ) npt ++;
		    if ( v[i][2][1] != 0 ) npt ++;
		}
		if ( v[0][1][1] != 0 ) npt ++;
		if ( v[2][1][1] != 0 ) npt ++;
		
		if ( npt < 4 ) {
		    liste[l].status = VT_DELETABLE;
		    continue;
		}
		if ( npt >= 8 ) {
		    liste[l].status = VT_TOBEDELETED;
		    b[ind] = VT_WILLBEDELETED;
		    continue;
		}
		if ( ( npt >= 4 ) && ( npt <= 7 ) ) {
		    if ( ( v[1+up[0]][1+up[1]][1+up[2]] != 0 ) && 
			 ( v[1+no[0]][1+no[1]][1+no[2]] != 0 ) &&
			 ( v[1+ea[0]][1+ea[1]][1+ea[2]] != 0 ) ) {
			liste[l].status = VT_TOBEDELETED;
			b[ind] = VT_WILLBEDELETED;
			continue;
		    }
		    if ( ( v[1+up[0]][1+up[1]][1+up[2]] != 0 ) &&
			 ( v[1+ea[0]][1+ea[1]][1+ea[2]] != 0 ) &&
			 ( v[1+so[0]][1+so[1]][1+so[2]] != 0 ) ) {
			liste[l].status = VT_TOBEDELETED;
			b[ind] = VT_WILLBEDELETED;
			continue;
		    }
		    if ( ( v[1+up[0]][1+up[1]][1+up[2]] != 0 ) &&
			 ( v[1+so[0]][1+so[1]][1+so[2]] != 0 ) &&
			 ( v[1+we[0]][1+we[1]][1+we[2]] != 0 ) ) {
			liste[l].status = VT_TOBEDELETED;
			b[ind] = VT_WILLBEDELETED;
			continue;
		    }
		    if ( ( v[1+up[0]][1+up[1]][1+up[2]] != 0 ) && 
			 ( v[1+we[0]][1+we[1]][1+we[2]] != 0 ) &&
			 ( v[1+no[0]][1+no[1]][1+no[2]] != 0 ) ) {
			liste[l].status = VT_TOBEDELETED;
			b[ind] = VT_WILLBEDELETED;
			continue;
		    }
		    if ( ( v[1+bo[0]][1+bo[1]][1+bo[2]] != 0 ) &&
			 ( v[1+no[0]][1+no[1]][1+no[2]] != 0 ) &&
			 ( v[1+ea[0]][1+ea[1]][1+ea[2]] != 0 ) ) {
			liste[l].status = VT_TOBEDELETED;
			b[ind] = VT_WILLBEDELETED;
			continue;
		    }
		    if ( ( v[1+bo[0]][1+bo[1]][1+bo[2]] != 0 ) &&
			 ( v[1+ea[0]][1+ea[1]][1+ea[2]] != 0 ) &&
			 ( v[1+so[0]][1+so[1]][1+so[2]] != 0 ) ) {
			liste[l].status = VT_TOBEDELETED;
			b[ind] = VT_WILLBEDELETED;
			continue;
		    }
		    if ( ( v[1+bo[0]][1+bo[1]][1+bo[2]] != 0 ) &&
			 ( v[1+so[0]][1+so[1]][1+so[2]] != 0 ) &&
			 ( v[1+we[0]][1+we[1]][1+we[2]] != 0 ) ) {
			liste[l].status = VT_TOBEDELETED;
			b[ind] = VT_WILLBEDELETED;
			continue;
		    }
		    if ( ( v[1+bo[0]][1+bo[1]][1+bo[2]] != 0 ) &&
			 ( v[1+we[0]][1+we[1]][1+we[2]] != 0 ) &&
			 ( v[1+no[0]][1+no[1]][1+no[2]] != 0 ) ) {
			liste[l].status = VT_TOBEDELETED;
			b[ind] = VT_WILLBEDELETED;
			continue;
		    }
		}
		liste[l].status = VT_DELETABLE;
	    }
	    /*--- fin du traitement de la liste ---*/
	    /*--- on modifie l'image et on rearrange la liste ---*/
	    i = 0;
	    for ( l = 0; l < ln; l++ ) {
		switch ( liste[l].status ) {
		case VT_DELETABLE :
		    /*--- on garde le point dans la liste ---*/
		    liste[i++] = liste[l];
		    break;
		case VT_TOBEDELETED :
		    /*--- on efface le point ---*/
		    ind = liste[l].pt.z * dxy + liste[l].pt.y * dx + liste[l].pt.x;
		    b[ind] = (unsigned char)0;
		    nbd ++;
		    break;
		case VT_UNDELETABLE :
		    /*--- on ne fait rien ---*/
		    break;
		default :
		    /*--- on s'inquiete ---*/
		    VT_Error("point with unknown label","_VT_BDD_GREG_PLANES");
		}
	    }
	    ln = i;
	    nbdel += nbd;
	    /*--- fin d'un sous-cycle : on prepare le suivant ---*/
	    for ( i = 0; i < nbPerm; i++ ) {
		in = up;
		up = no;
		no = ea;
		ea = bo;
		bo = so;
		so = we;
		we = in;
	    }
	    if (nbPerm == 3) nbPerm = 4;
	    else nbPerm = 3;
	    if (cycle == 4) nbPerm = 1;
	    /*--- message ---*/
	    sprintf( message,"     subiteration # %d, deleted points = %d", cycle+1, nbd );
	    VT_Message( message, "_VT_BDD_GREG_PLANES" );
	}
	/*--- message ---*/
	sprintf( message," iteration, deleted points = %d", nbdel );
	VT_Message( message, "_VT_BDD_GREG_PLANES" );
    } while ( (nbdel > 0) && (ln > 0) );
    VT_Free( (void**)&liste );
    return( 1 );
}

#ifndef NO_PROTO
int _VT_BDD_GREG_CURVES( vt_image *im /* image to be thinned */,
				vt_amincir *par /* parameters */ )
#else
int _VT_BDD_GREG_CURVES( im, par )
vt_image *im; /* image to be thinned */
vt_amincir *par; /* parameters */
#endif
{
    vt_pt_amincir *liste;
    int ln, lnb;
    int *in, *up, *no, *ea, *bo, *so, *we;
    int tup[3], tno[3], tea[3], tbo[3], tso[3], twe[3];
    int v[3][3][3], offset[3][3][3], w[3][3][3], voisins[26];
    register unsigned char *b;
    register int i, j, k, l, ind, dx, dxy;
    int nbPerm, cycle, nbd, nbdel, npt;
    char message[256];

    if ( VT_Test1Image( im, "_VT_BDD_GREG_CURVES" ) == -1 ) return( -1 );
    if ( (im->type != UCHAR) ) return( -1 );
	
    liste = _VT_ThinPtList( im, (int)VT_3D, &lnb );
    if ( (liste == (vt_pt_amincir*)NULL) || (lnb <= 0) ) {
	VT_Error( "unable to compute list of points", "_VT_BDD_GREG_CURVES" );
	return( 0 );
    }
    
    /*--- preparation ---*/
    b = (unsigned char*)(im->buf);
    tup[0] = 0;   tup[1] = 0;   tup[2] = 1;
    tno[0] = 0;   tno[1] = 1;   tno[2] = 0;
    tea[0] = 1;   tea[1] = 0;   tea[2] = 0;
    tbo[0] = 0;   tbo[1] = 0;   tbo[2] = -1;
    tso[0] = 0;   tso[1] = -1;  tso[2] = 0;
    twe[0] = -1;  twe[1] = 0;   twe[2] = 0;
    up = tup;   no = tno;   ea = tea;
    bo = tbo;   so = tso;   we = twe;
    nbPerm = 3;
    dx = im->dim.x;   dxy = im->dim.x * im->dim.y;
    /*--- tableaux d'offsets ---*/
    offset[1][1][1] = 0;
    offset[1][0][1] = - dx;
    offset[1][2][1] = dx;
    offset[0][1][1] = - 1;
    offset[0][0][1] = offset[1][0][1] - 1;
    offset[0][2][1] = offset[1][2][1] - 1;
    offset[2][1][1] = 1;
    offset[2][0][1] = offset[1][0][1] + 1;
    offset[2][2][1] = offset[1][2][1] + 1;
    for ( i = 0; i < 3; i++ )
    for ( j = 0; j < 3; j++ ) {
	offset[i][j][0] = offset[i][j][1] - dxy;
	offset[i][j][2] = offset[i][j][1] + dxy;
    }
    
    ln = lnb;
    /*--- boucle generale ---*/
    do {
	/*--- boucle des 6 sous-cycles ---*/
	nbdel = 0;
	for (cycle = 0; cycle < 6; cycle++ ) {
	    nbd = 0;
	    /*--- liste des points ---*/
	    for ( l = 0; l < ln; l++ ) {
		/*--- saisie du voisinage ---*/
		ind = liste[l].pt.z * dxy + liste[l].pt.y * dx + liste[l].pt.x;
		if ( liste[l].inside == 1 ) {
		    for ( i = 0; i < 3; i++ )
		    for ( j = 0; j < 3; j++ )
		    for ( k = 0; k < 3; k++ )
			v[i][j][k] = (int)(b[ ind + offset[i][j][k] ]);
		} else {
		    for ( i = -1; i < 2; i++ )
		    for ( j = -1; j < 2; j++ )
		    for ( k = -1; k < 2; k++ ) {
			if ( (liste[l].pt.x+i >= 0) && (liste[l].pt.x+i < dx) &&
			     (liste[l].pt.y+j >= 0) && (liste[l].pt.y+j < im->dim.y) &&
			     (liste[l].pt.z+k >= 0) && (liste[l].pt.z+k < im->dim.z) )
			    v[1+i][1+j][1+k] = (int)(b[ ind + offset[1+i][1+j][1+k] ]);
			else
			    v[1+i][1+j][1+k] = (int)0;
		    }
		}
		/*--- si la condition 2a n'est pas verifiee ---*/
		/*--- point + up appartient au fond         ---*/
		if ( v[1+up[0]][1+up[1]][1+up[2]] != 0 ) continue;

		/*--- si la condition 2b n'est pas verifiee ---*/
		/*--- un des 5 points ( bo, bo-no, bo-ea, bo-so, bo-we ) ---*/
		/*--- appartient a l'objet                               ---*/
		if ( ( v[1+bo[0]][1+bo[1]][1+bo[2]] == 0 ) &&
		     ( v[1+bo[0]+no[0]][1+bo[1]+no[1]][1+bo[2]+no[2]] == 0 ) &&
		     ( v[1+bo[0]+ea[0]][1+bo[1]+ea[1]][1+bo[2]+ea[2]] == 0 ) &&
		     ( v[1+bo[0]+so[0]][1+bo[1]+so[1]][1+bo[2]+so[2]] == 0 ) &&
		     ( v[1+bo[0]+we[0]][1+bo[1]+we[1]][1+bo[2]+we[2]] == 0 ) )
		    continue;
		
		/*--- on change de representation de voisinage ---*/
		voisins[ 0] = v[0][0][0];  voisins[ 1] = v[1][0][0];   voisins[ 2] = v[2][0][0];  
		voisins[ 3] = v[0][1][0];  voisins[ 4] = v[1][1][0];   voisins[ 5] = v[2][1][0];  
		voisins[ 6] = v[0][2][0];  voisins[ 7] = v[1][2][0];   voisins[ 8] = v[2][2][0];  
		voisins[ 9] = v[0][0][1];  voisins[10] = v[1][0][1];   voisins[11] = v[2][0][1];  
		voisins[12] = v[0][1][1];                              voisins[13] = v[2][1][1];  
		voisins[14] = v[0][2][1];  voisins[15] = v[1][2][1];   voisins[16] = v[2][2][1];  
		voisins[17] = v[0][0][2];  voisins[18] = v[1][0][2];   voisins[19] = v[2][0][2];  
		voisins[20] = v[0][1][2];  voisins[21] = v[1][1][2];   voisins[22] = v[2][1][2];  
		voisins[23] = v[0][2][2];  voisins[24] = v[1][2][2];   voisins[25] = v[2][2][2];  
				
		/*--- ce n'est pas un point simple                            ---*/
		if ( _VT_IsSimple( voisins ) != 1 ) continue;

		/*--- on efface les points qui vont disparaitre ---*/
		for ( i = 0; i < 3; i++ )
		for ( j = 0; j < 3; j++ )
		for ( k = 0; k < 3; k++ )
		    w[i][j][k] = ( v[i][j][k] == VT_WILLBEDELETED ) ? 0 : v[i][j][k];
		
		/*--- on change de representation de voisinage ---*/
		voisins[ 0] = w[0][0][0];  voisins[ 1] = w[1][0][0];   voisins[ 2] = w[2][0][0];  
		voisins[ 3] = w[0][1][0];  voisins[ 4] = w[1][1][0];   voisins[ 5] = w[2][1][0];  
		voisins[ 6] = w[0][2][0];  voisins[ 7] = w[1][2][0];   voisins[ 8] = w[2][2][0];  
		voisins[ 9] = w[0][0][1];  voisins[10] = w[1][0][1];   voisins[11] = w[2][0][1];  
		voisins[12] = w[0][1][1];                              voisins[13] = w[2][1][1];  
		voisins[14] = w[0][2][1];  voisins[15] = w[1][2][1];   voisins[16] = w[2][2][1];  
		voisins[17] = w[0][0][2];  voisins[18] = w[1][0][2];   voisins[19] = w[2][0][2];  
		voisins[20] = w[0][1][2];  voisins[21] = w[1][1][2];   voisins[22] = w[2][1][2];  
		voisins[23] = w[0][2][2];  voisins[24] = w[1][2][2];   voisins[25] = w[2][2][2];  
				
		/*--- ce n'est pas un point simple                            ---*/
		if ( _VT_IsSimple( voisins ) != 1 ) continue;

		/*--- si on est arrive ici, c'est un point effacable ---*/
		if ( par->bool_shrink == 1 ) {
		    liste[l].status = VT_TOBEDELETED;
		    b[ind] = VT_WILLBEDELETED;
		    continue;
		}
		
		/*--- condition geometrique d'arret      ---*/
		/*--- Remarque :                                   ---*/
		/*--- Selon le voisinage sur lequel on fait porter ---*/
		/*--- cette condition (v ou w = v - pts effaces)   ---*/
		/*--- le resultat est different.                   ---*/
		/*--- En choisissant v, le resultat semble moins   ---*/
		/*--- "bruite" (moins de branches parasites).      ---*/
		/*--- nombre de points dans le voisinage ---*/
		npt = 0;
		for ( i = 0; i < 3; i++ )
		for ( j = 0; j < 3; j++ ) {
		    if ( v[i][j][0] != 0 ) npt ++;
		    if ( v[i][j][2] != 0 ) npt ++;
		}
		for ( i = 0; i < 3; i++ ) {
		    if ( v[i][0][1] != 0 ) npt ++;
		    if ( v[i][2][1] != 0 ) npt ++;
		}
		if ( v[0][1][1] != 0 ) npt ++;
		if ( v[2][1][1] != 0 ) npt ++;
		
		if ( npt != 1 ) {
		    liste[l].status = VT_TOBEDELETED;
		    b[ind] = VT_WILLBEDELETED;
		    continue;
		}
		liste[l].status = VT_DELETABLE;
	    }
	    
	    /*--- fin du traitement de la liste ---*/
	    /*--- on modifie l'image et on rearrange la liste ---*/
	    i = 0;
	    for ( l = 0; l < ln; l++ ) {
		switch ( liste[l].status ) {
		case VT_DELETABLE :
		    /*--- on garde le point dans la liste ---*/
		    liste[i++] = liste[l];
		    break;
		case VT_TOBEDELETED :
		    /*--- on efface le point ---*/
		    ind = liste[l].pt.z * dxy + liste[l].pt.y * dx + liste[l].pt.x;
		    b[ind] = (unsigned char)0;
		    nbd ++;
		    break;
		case VT_UNDELETABLE :
		    /*--- on ne fait rien ---*/
		    break;
		default :
		    /*--- on s'inquiete ---*/
		    VT_Error("point with unknown label","_VT_BDD_GREG_CURVES");
		}
	    }
	    ln = i;
	    nbdel += nbd;
	    /*--- fin d'un sous-cycle : on prepare le suivant ---*/
	    for ( i = 0; i < nbPerm; i++ ) {
		in = up;
		up = no;
		no = ea;
		ea = bo;
		bo = so;
		so = we;
		we = in;
	    }
	    if (nbPerm == 3) nbPerm = 4;
	    else nbPerm = 3;
	    if (cycle == 4) nbPerm = 1;
	    /*--- message ---*/
	    sprintf( message,"     subiteration # %d, deleted points = %d", cycle+1, nbd );
	    VT_Message( message, "_VT_BDD_GREG_CURVES" );
	}
	/*--- message ---*/
	sprintf( message," iteration, deleted points = %d", nbdel );
	VT_Message( message, "_VT_BDD_GREG_CURVES" );
    } while ( (nbdel > 0) && (ln > 0) );

    /*--- si on n'est pas en mode shrinking ---*/
    if ( par->bool_shrink == 0 ) {
	VT_Free( (void**)&liste );
	return( 1 );
    }
    
    /*--- sinon on continue : boucle generale ---*/
    do {
	/*--- boucle des 6 sous-cycles ---*/
	nbdel = 0;
	for (cycle = 0; cycle < 6; cycle++ ) {
	    nbd = 0;
	    /*--- liste des points ---*/
	    for ( l = 0; l < ln; l++ ) {
				/*--- saisie du voisinage ---*/
		ind = liste[l].pt.z * dxy + liste[l].pt.y * dx + liste[l].pt.x;
		if ( liste[l].inside == 1 ) {
		    for ( i = 0; i < 3; i++ )
		    for ( j = 0; j < 3; j++ )
		    for ( k = 0; k < 3; k++ )
			v[i][j][k] = (int)(b[ ind + offset[i][j][k] ]);
		} else {
		    for ( i = -1; i < 2; i++ )
		    for ( j = -1; j < 2; j++ )
		    for ( k = -1; k < 2; k++ ) {
			if ( (liste[l].pt.x+i >= 0) && (liste[l].pt.x+i < dx) &&
			     (liste[l].pt.y+j >= 0) && (liste[l].pt.y+j < im->dim.y) &&
			     (liste[l].pt.z+k >= 0) && (liste[l].pt.z+k < im->dim.z) )
			    v[1+i][1+j][1+k] = (int)(b[ ind + offset[1+i][1+j][1+k] ]);
			else
			    v[1+i][1+j][1+k] = (int)0;
		    }
		}
		/*--- si la condition 2a n'est pas verifiee ---*/
		/*--- point + up appartient au fond         ---*/
		if ( v[1+up[0]][1+up[1]][1+up[2]] != 0 ) continue;
		
		/*--- si la condition 2b n'est pas verifiee ---*/
		/*--- un des 9 points ( bo, bo-no, bo-ea, bo-so, bo-we ) ---*/
		/*--- et ( bo-no-ea, bo-ea-so, bo-so-we, bo-we-no )      ---*/
		/*--- appartient a l'objet                               ---*/
		if ( ( v[1+bo[0]][1+bo[1]][1+bo[2]] == 0 ) &&
		     ( v[1+bo[0]+no[0]][1+bo[1]+no[1]][1+bo[2]+no[2]] == 0 ) &&
		     ( v[1+bo[0]+ea[0]][1+bo[1]+ea[1]][1+bo[2]+ea[2]] == 0 ) &&
		     ( v[1+bo[0]+so[0]][1+bo[1]+so[1]][1+bo[2]+so[2]] == 0 ) &&
		     ( v[1+bo[0]+we[0]][1+bo[1]+we[1]][1+bo[2]+we[2]] == 0 ) &&
		     ( v[1+bo[0]+no[0]+ea[0]][1+bo[1]+no[1]+ea[1]][1+bo[2]+no[2]+ea[2]] == 0 ) &&
		     ( v[1+bo[0]+ea[0]+so[0]][1+bo[1]+ea[1]+so[1]][1+bo[2]+ea[2]+so[2]] == 0 ) &&
		     ( v[1+bo[0]+so[0]+we[0]][1+bo[1]+so[1]+we[1]][1+bo[2]+so[2]+we[2]] == 0 ) &&
		     ( v[1+bo[0]+we[0]+no[0]][1+bo[1]+we[1]+no[1]][1+bo[2]+we[2]+no[2]] == 0 ) )
		    continue;
		
		/*--- on change de representation de voisinage ---*/
		voisins[ 0] = v[0][0][0];  voisins[ 1] = v[1][0][0];   voisins[ 2] = v[2][0][0];  
		voisins[ 3] = v[0][1][0];  voisins[ 4] = v[1][1][0];   voisins[ 5] = v[2][1][0];  
		voisins[ 6] = v[0][2][0];  voisins[ 7] = v[1][2][0];   voisins[ 8] = v[2][2][0];  
		voisins[ 9] = v[0][0][1];  voisins[10] = v[1][0][1];   voisins[11] = v[2][0][1];  
		voisins[12] = v[0][1][1];                              voisins[13] = v[2][1][1];  
		voisins[14] = v[0][2][1];  voisins[15] = v[1][2][1];   voisins[16] = v[2][2][1];  
		voisins[17] = v[0][0][2];  voisins[18] = v[1][0][2];   voisins[19] = v[2][0][2];  
		voisins[20] = v[0][1][2];  voisins[21] = v[1][1][2];   voisins[22] = v[2][1][2];  
		voisins[23] = v[0][2][2];  voisins[24] = v[1][2][2];   voisins[25] = v[2][2][2];  
		
		/*--- ce n'est pas un point simple                            ---*/
		if ( _VT_IsSimple( voisins ) != 1 ) continue;

		/*--- on efface les points qui vont disparaitre ---*/
		for ( i = 0; i < 3; i++ )
		for ( j = 0; j < 3; j++ )
		for ( k = 0; k < 3; k++ )
		    w[i][j][k] = ( v[i][j][k] == VT_WILLBEDELETED ) ? 0 : v[i][j][k];
		
		/*--- on change de representation de voisinage ---*/
		voisins[ 0] = w[0][0][0];  voisins[ 1] = w[1][0][0];   voisins[ 2] = w[2][0][0];  
		voisins[ 3] = w[0][1][0];  voisins[ 4] = w[1][1][0];   voisins[ 5] = w[2][1][0];  
		voisins[ 6] = w[0][2][0];  voisins[ 7] = w[1][2][0];   voisins[ 8] = w[2][2][0];  
		voisins[ 9] = w[0][0][1];  voisins[10] = w[1][0][1];   voisins[11] = w[2][0][1];  
		voisins[12] = w[0][1][1];                              voisins[13] = w[2][1][1];  
		voisins[14] = w[0][2][1];  voisins[15] = w[1][2][1];   voisins[16] = w[2][2][1];  
		voisins[17] = w[0][0][2];  voisins[18] = w[1][0][2];   voisins[19] = w[2][0][2];  
		voisins[20] = w[0][1][2];  voisins[21] = w[1][1][2];   voisins[22] = w[2][1][2];  
		voisins[23] = w[0][2][2];  voisins[24] = w[1][2][2];   voisins[25] = w[2][2][2];  
		
		/*--- ce n'est pas un point simple                            ---*/
		if ( _VT_IsSimple( voisins ) != 1 ) continue;
		
		/*--- si on est arrive ici, c'est un point effacable ---*/
		liste[l].status = VT_TOBEDELETED;
		b[ind] = VT_WILLBEDELETED;
	    }
	    
	    /*--- fin du traitement de la liste ---*/
	    /*--- on modifie l'image et on rearrange la liste ---*/
	    i = 0;
	    for ( l = 0; l < ln; l++ ) {
		switch ( liste[l].status ) {
		case VT_DELETABLE :
		    /*--- on garde le point dans la liste ---*/
		    liste[i++] = liste[l];
		    break;
		case VT_TOBEDELETED :
		    /*--- on efface le point ---*/
		    ind = liste[l].pt.z * dxy + liste[l].pt.y * dx + liste[l].pt.x;
		    b[ind] = (unsigned char)0;
		    nbd ++;
		    break;
		case VT_UNDELETABLE :
		    /*--- on ne fait rien ---*/
		    break;
		default :
		    /*--- on s'inquiete ---*/
		    VT_Error("point with unknown label","_VT_BDD_GREG_CURVES");
		}
	    }
	    ln = i;
	    nbdel += nbd;
	    /*--- fin d'un sous-cycle : on prepare le suivant ---*/
	    for ( i = 0; i < nbPerm; i++ ) {
		in = up;
		up = no;
		no = ea;
		ea = bo;
		bo = so;
		so = we;
		we = in;
	    }
	    if (nbPerm == 3) nbPerm = 4;
	    else nbPerm = 3;
	    if (cycle == 4) nbPerm = 1;
	    /*--- message ---*/
	    sprintf( message,"     subiteration # %d, deleted points = %d", cycle+1, nbd );
	    VT_Message( message, "_VT_BDD_GREG_CURVES" );
	}
	/*--- message ---*/
	sprintf( message," iteration, deleted points = %d", nbdel );
	VT_Message( message, "_VT_BDD_GREG_CURVES" );
    } while ( (nbdel > 0) && (ln > 0) );
    
    VT_Free( (void**)&liste );
    return( 1 );
}

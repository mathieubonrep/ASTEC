#ifndef _vt_geline3D_h_ 
#define _vt_geline3D_h_ 

#ifdef __cplusplus
extern "C" {
#endif



#include <vt_common.h>

#include <vt_recfilters.h>

#include <vt_geline.h>

#ifndef NO_PROTO 
extern int  VT_ComputeLine3D( vt_resline *res, vt_images *ims, vt_image *theim, vt_line *par );
#else
extern int  VT_ComputeLine3D();
#endif /* NO_PROTO */




#ifdef __cplusplus
}
#endif

#endif /* _vt_geline3D_h_ */ 

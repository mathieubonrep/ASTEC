#ifndef _vt_dst_amincir_h_
#define _vt_dst_amincir_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <vt_common.h>
#include <vt_amincir.h>
#include <vt_distance.h>
#include <vt_amliste.h>

#ifndef NO_PROTO
extern int _VT_DST_GREG_2D( vt_image *im, vt_amincir *par );
#else
extern int _VT_DST_GREG_2D();
#endif

#ifdef __cplusplus
}
#endif

#endif /* _vt_dst_amincir_h_ */

#ifndef  EVR_TIME_H
#define  EVR_TIME_H


#include <tprPatternCommon.h>

#ifdef __cplusplus
extern "C" {
#endif


/* For time ID */
typedef enum {
  evrTimeCurrent=0, evrTimeNext1=1, evrTimeNext2=2, evrTimeNext3=3, evrTimeActive =4
} evrTimeId_te;
#define MAX_EVR_TIME  4

/* For modifier array */
#define MAX_EVR_MODIFIER  6
typedef epicsUInt32 evrModifier_ta[MAX_EVR_MODIFIER];


/* Event codes - see mrfCommon.h for reserved internal event codes      */
#define EVENT_FIDUCIAL          1        /* Fiducial event code         */
#define EVENT_EXTERNAL_TRIG     100      /* External trigger event code */
#define EVENT_MODULO720         121      /* Modulo 720 event code       */
#define EVENT_MPG               122      /* MPG update event code       */
#define EVENT_MODULO36_MIN      201      /* Min modulo 36 event code    */
#define EVENT_MODULO36_MAX      236      /* Max modulo 36 event code    */
#define MODULO36_MAX            36       /* # modulo 36 event codes     */

int evrInitialize(void);

int evrTimeRegister       (FIDUCIALFUNCTION fiducialFunc,
                           void *           fiducialArg);
                           
int evrTimeGetFromPipeline(epicsTimeStamp  *epicsTime_ps,
                           evrTimeId_te     id,
                           evrModifier_ta   modifier_a,
                           epicsUInt32     *patternStatus_p,
                           epicsUInt32     *edefAvgDoneMask_p,
                           epicsUInt32     *edefMinorMask_p,
                           epicsUInt32     *edefMajorMask_p);
int evrTimeGetFromEdef    (unsigned int     edefIdx,
                           epicsTimeStamp  *edefTime_ps,
                           epicsTimeStamp  *edefTimeInit_ps,
                           int             *edefAvgDone_p,
                           epicsEnum16     *edefSevr_p);
int evrTimeGet            (epicsTimeStamp  *epicsTime_ps,
                           unsigned int     eventCode);



#ifdef __cplusplus
}    /* extern C */
#endif

#endif  /* EVR_TIME_H */

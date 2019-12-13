#ifndef TPR_PATTERN_ASYN_DRIVER_H
#define TPR_PATTERN_ASYN_DRIVER_H

#include <epicsTypes.h>
#include <epicsTime.h>
#include <sys/time.h>


#include <bsaCallbackApi.h>
#include <evrTime.h>


#define MAX_NUM_TRIGGER   16

// we may not need the following structure!
// we are going to remove it!
/* 
#pragma pack (push)
#pragma pack (1)


typedef struct {
    epicsUInt32      pulse_id;
    epicsUInt32      event_code[8];
    
    epicsUInt16      dtype;
    epicsUInt16      version;
    
    epicsUInt32      mod[6];
    epicsTimeStamp   time;
    epicsUInt32      edefAvgDoneMask;
    epicsUInt32      edefMinorMask;
    epicsUInt32      edefMajorMask;
    epicsUInt32      edefInitMask;
} tstream_LCLS1_pattern_t;

#define SIZE_LCLS1_PATTERN_STREAM  sizeof(tstream_LCLS1_pattern_t)


#pragma pack (pop)
*/


class tprPatternAsynDriver:asynPortDriver {
    public:
        tprPatternAsynDriver(const char *portName, const char *corePath, const char *streamPath, const char *named_root = NULL);
        int Report(int interest_level);
        int tprPatternTask(void);
        
        BsaTimingData *GetLastBsaTimingData(void);
        int BsaCallbackRegister(BsaTimingCallback callback, void *pUserPvt);
        int FiducialCallbackRegister(FIDUCIALFUNCTION fiducialFuc, void * fiducialArg);
        int tprPatternEventTimeGet_gtWrapper( epicsTimeStamp *time, int eventCode);
        int tprPatternEventTimeGetSystem_gtWrapper(epicsTimeStamp *time, int eventCode);
        
        Tpr::TprPatternYaml     * GetDriver(void)        { return p_drv; };
        TprEvent::PatternBuffer * GetPatternBuffer(void) { return p_patternBuffer; };
        
        
        asynStatus writeInt32(asynUser *pasynUser, epicsInt32 vlaue);
        
        
    private:
        char named_root[128];
        char port_name[128];
        char core_path[128];
        char stream_path[128];
        
        Tpr::TprPatternYaml *p_drv;
        TprEvent::PatternBuffer *p_patternBuffer;
        
        void CreateParameters(void);
        void TprDiagnostics(void);
        void UpdateDiagPVs(Tpr::TprStream **buf);
        
    protected:
    //
    // parameter section for asynPortDriver
    //
#if (ASYN_VERSION <<8 | ASYN_REVISION) < (4<<8 | 32)
        int  firstTprPtrnParam;
        #define FIRST_TPR_PTRN_PARAM       firstTprPtrnParam
#endif /* check asyn version, under 4.32 */
        
        int p_activeTS1;
        int p_activeTS2;
        
        struct {
            int event_code;
            int enable;
            int rate;         /* event rate in timing pattern */
            int ev_rate;      /* event generation rate */
        }  p_soft_event_select[MAX_NUM_TRIGGER];
        
        // parameter for pattern diagnostics
        struct {
            int     pulse_id;
            int     beam_code;
            int     timeslot;
            int     timestamp;
            int     second;
            int     nano_second;
            int     modifier1;
            int     modifier2;
            int     modifier3;
            int     modifier4;
            int     modifier5;
            int     modifier6;
            int     edef_avg_done;
            int     edef_minor;
            int     edef_major;
            int     edef_init;
            int     edef_active;
            int     edef_all_done;
            int     status;
        } p_patternDiag[4];

#if (ASYN_VERSION <<8 | ASYN_REVISION) < (4<<8 | 32)        
        int  lastTprPtrnParam;
        #define LAST_TPR_PTRN_PARAM        lastTprPtrnParam
#endif /* check asyn version, under 4.32 */
        
};

#if (ASYN_VERSION <<8 | ASYN_REVISION) < (4<<8 | 32)
#define NUM_TPR_PTRN_DET_PARAMS    ((int)(&LAST_TPR_PTRN_PARAM-&FIRST_TPR_PTRN_PARAM -1))
#endif /* check asyn version, under 4.32 */

#define activeTS1String                    "activeTS1"
#define activeTS2String                    "activeTS2"

#define softEventSelect_eventCodeString    "softEventSelect%d_eventCode"
#define softEventSelect_enableString       "softEventSelect%d_enable"
#define softEventSelect_rateString         "softEventSelect%d_rate"
#define softEventSelect_evRateString       "softEventSelect%d_evRate"


#define patternDiag_pulseIdString          "patternDiag%d_pulseId"
#define patternDiag_beamCodeString         "patternDiag%d_beamCode"
#define patternDiag_timeslotString         "patternDiag%d_timeslot"
#define patternDiag_timestampString        "patternDiag%d_timestamp"
#define patternDiag_secondString           "patternDiag%d_second"
#define patternDiag_nanoSecondString       "patternDiag%d_nanoSecond"
#define patternDiag_modifier1String        "patternDiag%d_modifier1"
#define patternDiag_modifier2String        "patternDiag%d_modifier2"
#define patternDiag_modifier3String        "patternDiag%d_modifier3"
#define patternDiag_modifier4String        "patternDiag%d_modifier4"
#define patternDiag_modifier5String        "patternDiag%d_modifier5"
#define patternDiag_modifier6String        "patternDiag%d_modifier6"
#define patternDiag_edefAvgDoneString      "patternDiag%d_edefAvgDone"
#define patternDiag_edefMinorString        "patternDiag%d_edefMinor"
#define patternDiag_edefMajorString        "patternDiag%d_edefMajor"
#define patternDiag_edefInitString         "patternDiag%d_edefInit"
#define patternDiag_edefActiveString       "patternDiag%d_edefActive"
#define patternDiag_edefAllDoneString      "patternDiag%d_edefAllDone"
#define patternDiag_statusString           "patternDiag%d_status"




extern "C" {
    tprPatternAsynDriver *Get_tprPatternAsynDriver(void);
}


#endif /* TPR_PATTERN_ASYN_DRIVER_H */

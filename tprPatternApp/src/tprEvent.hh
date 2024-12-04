//////////////////////////////////////////////////////////////////////////////
// This file is part of 'tprPattern'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'tprPattern', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#ifndef  TPR_EVENT_DRIVER_H
#define  TPR_EVENT_DRIVER_H

#include <alarm.h>
#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsMutex.h>
#include <epicsGeneralTime.h>
#include <dbScan.h>
#include <ellLib.h>

#include <tprPatternYaml.hh>

#include <bsaCallbackApi.h>

#include <tprPatternCommon.h>

namespace TprEvent {

class EventTimeRate {
    public:
        EVENTPVT          pevent;
        epicsTimeStamp    time;
        int               status;
        int               count;
        float             rate;
        int               enable;
        int               ev_count;
        float             ev_rate;
        
        
        EventTimeRate(void):pevent(NULL), status(0),count(0),rate(0.),enable(0),ev_count(0),ev_rate(0.) {  };
};


class EdefTimeTbl {
    public:
        epicsTimeStamp    time;
        epicsTimeStamp    timeInit;
        int               avgdone;
        epicsEnum16       sevr;
        
        
        EdefTimeTbl(void):  avgdone(0), sevr(INVALID_ALARM) { time.secPastEpoch = 0;
                                                              time.nsec = 0;
                                                              timeInit.secPastEpoch = 0;
                                                              timeInit.nsec = 0;   };
};



class FiducialCallback {
    private:
        epicsMutex    *plock;
        ELLLIST       *plist;
        
        
    public:
        FiducialCallback(void);
        int Register(FIDUCIALFUNCTION fiducialFuc, void * fiducialArg);
        int CallFunctions(void);
        int Report(void);
};


class BsaCallback {
    private:
        epicsMutex    *plock;
        ELLLIST       *plist;
        
        
    public:
        BsaCallback(void);
        int  Register(BsaTimingCallback callback, void *pUserPvt);
        int  CallFunctions(const BsaTimingData *bsa_data);
        int  Report(void);
};        
           

class PatternBuffer {
    private:
        Tpr::TprStream   *buf;
        Tpr::TprStream   *lastActiveBuf;
        EventTimeRate    *event_tbl;
        EdefTimeTbl      *edef_tbl;
        FiducialCallback *pFiducialCallback;
        
        BsaCallback      *pBsaCallback;
        BsaTimingData    *bsaTimingBuf;
        BsaTimingData    *pLastBsaBuf;
        
        uint32_t         index;
        uint32_t         length;
        
        int              ts1, ts2;   /* active timeslots */
        
        epicsMutex      *plock;
        
    public:
        PatternBuffer(void);
        Tpr::TprStream * GetNextBuf(void);
        Tpr::TprStream * EvolveBuf(void);
        Tpr::TprStream * GetPipelineBuf(int piplineIndex);  /* 0: current, 
                                                           1: 1 fiducial advanced, 
                                                           2: 2 fiducial advanced,
                                                           negative: past buffers */
        Tpr::TprStream * GetLastActiveBuf(void);
        void _debugPrint(void);
        
        void            Lock(void)   { plock->lock(); };
        void            Unlock(void) { plock->unlock(); };
        
        uint32_t        GetPulseId(Tpr::TprStream *buf);
        uint32_t        GetTimeslot(Tpr::TprStream *buf);
        uint32_t        GetBeamCode(Tpr::TprStream *buf);
        epicsTimeStamp* GetTimestamp(Tpr::TprStream *buf);
        uint32_t        GetSecond(Tpr::TprStream *buf);
        uint32_t        GetNanoSecond(Tpr::TprStream *buf);
        uint32_t        GetModifier1(Tpr::TprStream *buf);
        uint32_t        GetModifier2(Tpr::TprStream *buf);
        uint32_t        GetModifier3(Tpr::TprStream *buf);
        uint32_t        GetModifier4(Tpr::TprStream *buf);
        uint32_t        GetModifier5(Tpr::TprStream *buf);
        uint32_t        GetModifier6(Tpr::TprStream *buf);
        uint32_t        GetEdefAvgDone(Tpr::TprStream *buf);
        uint32_t        GetEdefMinor(Tpr::TprStream *buf);
        uint32_t        GetEdefMajor(Tpr::TprStream *buf);
        uint32_t        GetEdefInit(Tpr::TprStream *buf);
        uint32_t        GetEdefAllDone(Tpr::TprStream *buf);
        uint32_t        GetEdefActive(Tpr::TprStream *buf);
        
        EdefTimeTbl*    GetEdefTable(int index) { return (!edef_tbl)?NULL: edef_tbl + index; }
        epicsTimeStamp* GetEdefTimestamp(EdefTimeTbl *edef) { return &edef->time; }
        epicsTimeStamp* GetEdefInitTimestamp(EdefTimeTbl *edef) { return &edef->timeInit; }
        int             GetEdefAvgDone(EdefTimeTbl *edef) { return edef->avgdone; }
        epicsEnum16     GetEdefSevr(EdefTimeTbl *edef) { return edef->sevr; };
        
        void            EnableSoftEvent(uint32_t event, uint32_t enable);
        
        
        uint32_t        IsEvent(Tpr::TprStream *buf, uint32_t event);
        uint32_t        IsDiag(Tpr::TprStream *buf);
        uint32_t        IsActiveTimeslot(Tpr::TprStream *buf);
        
        void            SetActiveTimeslots(int t1, int t2);
        void            ActiveTimeslotProcessing(Tpr::TprStream *buf);
        void            EventProcessing(Tpr::TprStream *buf_advanced, Tpr::TprStream *buf_current);
        void            EdefProcessing(Tpr::TprStream *buf_advanced, Tpr::TprStream *buf_current);
        
        void            BsaTimingProcessing(Tpr::TprStream *buf_current);
        int             BsaCallbackProcessing(void);
        int             BsaCallbackRegister(BsaTimingCallback callback, void *pUserPvt);
        int             BsaCallbackReport(void);
        BsaTimingData*  GetLastBsaTimingData(void) { return pLastBsaBuf; };
        
        void            EventRateProcessing(void);
        int             FiducialCallbackProcessing(void);
        int             FiducialCallbackRegister(FIDUCIALFUNCTION fiducialFuc, void * fiducialArg);
        int             FiducialCallbackReport(void);
        
        void            GetEventRate(uint32_t event_code, float *rate, float *ev_rate);
        
        int             EventTimePutPulseId(epicsTimeStamp *time, uint32_t pulse_id);
        int             EventTimeGet(epicsTimeStamp *time, uint32_t event_code);
        int             EventTimeGetSystem(epicsTimeStamp *time, uint32_t event_code);
        
        
        // int             RegisterBsaTimingCallback(BsaTimingCallback callback, void *pUserPvt) { if(!pBsaCallback) return -1;  pBsaCallback->Register(callback, pUserPvt); };
        
};



};  /* namespace TprEvent */

#endif   /* TPR_EVENT_DRIVER_H */
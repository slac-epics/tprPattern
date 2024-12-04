//////////////////////////////////////////////////////////////////////////////
// This file is part of 'tprPattern'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'tprPattern', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#include <exception>
#include <epicsTime.h>

#include <tprEvent.hh>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsGeneralTime.h>
#include <epicsThread.h>
#include <dbScan.h>
#include <ellLib.h>

#include <errlog.h>

namespace TprEvent {



typedef struct {
    ELLNODE node;
    FIDUCIALFUNCTION func;
    void * arg;
} fiducialFunc_ts;


FiducialCallback::FiducialCallback(void)
{
    plock = new epicsMutex;
    plist = new ELLLIST;
    ellInit(plist);
    
}

int FiducialCallback::Register(FIDUCIALFUNCTION fiducialFunc, void * fiducialArg)
{
    if(!fiducialFunc) {
        errlogPrintf("FiducialCallback::Register: invalid fiducial function\n");
        return -1;
    }
    fiducialFunc_ts *pfiducial = new fiducialFunc_ts;
 
           
    pfiducial->func = fiducialFunc;
    pfiducial->arg  = fiducialArg;
    
    plock->lock();
    ellAdd(plist, &pfiducial->node);
    plock->unlock();
     
    return 0;
}


int FiducialCallback::CallFunctions(void)
{
    fiducialFunc_ts *p;
    
    plock->lock();
    p = (fiducialFunc_ts *) ellFirst(plist);
    while (p) {
        (*(p->func))(p->arg);
        p = (fiducialFunc_ts *) ellNext(&p->node);
    }
    plock->unlock();

    return 0;
}

int FiducialCallback::Report(void)
{
    fiducialFunc_ts *p;
    
    printf("Total %d fiducial function(s) are registered\n", ellCount(plist));
    p = (fiducialFunc_ts *) ellFirst(plist);
    while (p) {
        printf("Registered fiducial function %p, argument %p\n", p->func, p->arg);
        p = (fiducialFunc_ts *) ellNext(&p->node);
    }   
    

    return 0;
}


typedef struct {
    ELLNODE node;
    BsaTimingCallback bsa_cb;
    void *            p_bsa_usrPvt;
    unsigned          exception_cnt;
} bsaCallbackFunc_ts;


BsaCallback::BsaCallback(void)
{
    plock = new epicsMutex;
    plist = new ELLLIST;
    ellInit(plist);
}

int BsaCallback::Register(BsaTimingCallback callback, void *pUserPvt)
{
    if(!callback) {
        errlogPrintf("BsaCallback::Register: invalid callback function\n");
        return -1;
    }
    
    bsaCallbackFunc_ts *pBsaCallback = new bsaCallbackFunc_ts;
    
    pBsaCallback->bsa_cb = callback;
    pBsaCallback->p_bsa_usrPvt = pUserPvt;
    pBsaCallback->exception_cnt = 0;
    
    plock->lock();
    ellAdd(plist, &pBsaCallback->node);
    plock->unlock();

    return 0;
}

int BsaCallback::CallFunctions(const BsaTimingData *bsa_data)
{

    bsaCallbackFunc_ts *p;
    
    plock->lock();
    p = (bsaCallbackFunc_ts *) ellFirst(plist);
    while(p) {
        try {
            (*(p->bsa_cb))(p->p_bsa_usrPvt, bsa_data);
        } catch (std::exception &e) {
            /* nothing to do for the pattern buffer overrun,
               just ignore and skip current timing pattern 
               to avoid application crash */
            (p->exception_cnt)++;
        }
        p = (bsaCallbackFunc_ts *) ellNext(&p->node);
    }
    plock->unlock();
    
    return 0;
}


int BsaCallback::Report(void)
{
    bsaCallbackFunc_ts *p;
    // plock->lock():   /* we may not need locking for report function */
    printf("Total %d bsa callback functions(s) are registered\n", ellCount(plist));
    p = (bsaCallbackFunc_ts *) ellFirst(plist);
    while(p) {
        printf("Registered bsa function %p, argument %p\n", p->bsa_cb, p->p_bsa_usrPvt);
        printf("Exception Counter for PatternBuffer %lu\n", p->exception_cnt);
        p = (bsaCallbackFunc_ts *) ellNext(&p->node);
    }
    // plock->unlock(); /* we may not need locking for report function */
    

    return 0;
}






PatternBuffer::PatternBuffer (void)
{
    plock  = new epicsMutex;
    
    buf       = new Tpr::TprStream[MAX_BUF_SIZE];
    event_tbl = new EventTimeRate[MAX_NUM_EVENT];
    lastActiveBuf = NULL;
    
    for(int i =1; i < 256; i++) {
         char event_name[8];
         sprintf(event_name, "%d", i);
         (event_tbl +i)->pevent = eventNameToHandle((const char *) event_name);
    }
    
    
    
    
    
    edef_tbl  = new EdefTimeTbl[EDEF_MAX];
    pFiducialCallback = new FiducialCallback;

    pBsaCallback = new BsaCallback;
    bsaTimingBuf = new BsaTimingData[MAX_BUF_SIZE];
    pLastBsaBuf  = NULL;
    
    
     plock->lock();
        index  = 0;
        length = 0;
        
        /* set defulat active timeslot (for LCLS1), these will be override by the PVs */
        ts1 = 1;
        ts2 = 4; 
    plock->unlock();
}

Tpr::TprStream * PatternBuffer::GetNextBuf(void)
{
    uint32_t  i;
    
    // plock->lock(); 
    i = index +1; 
    // plock->unlock();
    
    i &= BUF_INDEX_MASK;
    
    return buf + i;

}

Tpr::TprStream * PatternBuffer::EvolveBuf(void)
{
    uint32_t i;
    
    // plock->lock();
        index ++;
        index &= BUF_INDEX_MASK;
        length ++;
        if(length > MAX_BUF_SIZE) length = MAX_BUF_SIZE;    
        i = index;
    // plock->unlock();
    
    return buf + i;

}


Tpr::TprStream * PatternBuffer::GetPipelineBuf(int pipelineIndex)
{
    uint32_t i, l;
    
    plock->lock(); i = index; l = length; plock->unlock();
    
        
    if( ((int)l -3 + pipelineIndex) < 0) return (Tpr::TprStream *) NULL;    /* do not have enough length of buffers */
    
    i += (pipelineIndex -2);
    i &= BUF_INDEX_MASK;
    
    return buf + i;
    
}

Tpr::TprStream * PatternBuffer::GetLastActiveBuf(void)
{
    return lastActiveBuf;
}

void PatternBuffer::_debugPrint(void)
{
    Tpr::TprStream * buf = GetPipelineBuf(0);
    Tpr::TprStream * buf_1 = GetPipelineBuf(2);
    
    if(!buf || !buf_1 || !IsDiag(buf_1)) return;
    
        
    int pulseId  =   !buf? -1: (int) GetPulseId(buf);
    int timeslot =   !buf? -1: (int) GetTimeslot(buf); 
    
    
    printf("write pointer %8.8x, length %8.8x, pulseid %d, timeslot %d\n", index, length, pulseId, timeslot); 
}

uint32_t PatternBuffer::GetPulseId(Tpr::TprStream *buf)
{
    epicsTimeStamp  *p_time = (epicsTimeStamp *) &buf->epics_time;
    uint32_t pulseId = PULSEID(*p_time);
        
    return pulseId;
    
}


uint32_t PatternBuffer::GetTimeslot(Tpr::TprStream *buf)
{
    uint32_t   timeslot = TIMESLOT(buf->dmod);
 
    return timeslot;
}

uint32_t PatternBuffer::IsEvent(Tpr::TprStream *buf, uint32_t event)
{
    uint32_t  one(1), zero(0);
 
    return (buf->event_code[event>>5] & ((uint32_t) 1 << (event & 0x0000001f))) ? one: zero; 
    
    
}


uint32_t PatternBuffer::IsDiag(Tpr::TprStream *buf)
{
    uint32_t one(1), zero(0);
    
    return (buf->dmod[MOD1_IDX] & MODULO720_MASK)? one: zero;
}


uint32_t PatternBuffer::IsActiveTimeslot(Tpr::TprStream *buf)
{
    uint32_t zero(0), one(1);
    if(!buf) return 0;
    
    uint32_t ts = GetTimeslot(buf);
    
    return ((uint32_t) ts1 == ts || (uint32_t)ts2 == ts)? one: zero; 
}



void PatternBuffer::SetActiveTimeslots(int t1, int t2)
{
    ts1 = t1; ts2 = t2;
}


void PatternBuffer::ActiveTimeslotProcessing(Tpr::TprStream *buf)
{
    if(!buf) return;
    
    epicsTimeStamp *p_time = GetTimestamp(buf);
    uint32_t       ts      = GetTimeslot(buf);
    EventTimeRate  *p;
    
    p = event_tbl + EVENT_FIDUCIAL;
    p->time = *p_time;
    p->count++;
    if(p->enable) p->ev_count ++;
    
    
    if((uint32_t) ts1 == ts || (uint32_t) ts2 == ts) {
        lastActiveBuf = buf;
        p = event_tbl + ACTIVE_TS;
        p->time = *p_time;
        p->count ++;
        if(p->enable) p->ev_count ++;
    } 
}

void PatternBuffer::EventProcessing(Tpr::TprStream *buf_advanced, Tpr::TprStream *buf_current)
{
    if(!buf_advanced || !buf_current) return;

    epicsTimeStamp *p_ts = GetTimestamp(buf_current);
    
    {     /* for fiducial event, event_code == 1 */
        EventTimeRate *p = event_tbl +1;
        EVENTPVT  pevent = p->pevent;
        
        /* counts and rate calculatations are already done by ActiveTimeslotProcessing() */
        
        if(p->enable && pevent) postEvent(pevent);
    }
    
    for(int i = 2; i < MAX_NUM_EVENT; i++) {    /* for normal event */
        if(IsEvent(buf_advanced, i)) {
            uint32_t flag = 0;
            EventTimeRate *p = event_tbl +i;
            EVENTPVT  pevent = p->pevent;
            
            p->time = *p_ts;
            p->count++;
            if(p->enable) {
                flag = 1;
                p->ev_count++;
            }
            
            // if(flag) post_event (i);
            if(flag && pevent) postEvent(pevent);
        }
    }
}


void PatternBuffer::EdefProcessing(Tpr::TprStream *buf_advanced, Tpr::TprStream *buf_current)
{
    if(!buf_advanced || ! buf_current) return; 
    
    
    for(int i = 0; i < EDEF_MAX; i++) {
        uint32_t  edefMask = 0x00000001 << i;
        
        if(buf_advanced && (GetEdefInit(buf_advanced) & edefMask)) (edef_tbl +i)->timeInit = * GetTimestamp(buf_advanced);

        if(buf_current && (GetModifier5(buf_current) & edefMask)) {
           (edef_tbl +i)->time = *GetTimestamp(buf_current); 

            if(GetEdefAvgDone(buf_current) & edefMask) (edef_tbl +i)->avgdone = 1;
            else                                       (edef_tbl +i)->avgdone = 0;
            
            if(GetEdefMinor(buf_current) & edefMask) (edef_tbl +i)->sevr = MINOR_ALARM;
            else
            if(GetEdefMajor(buf_current) & edefMask) (edef_tbl +i)->sevr = MAJOR_ALARM;
            else                                     (edef_tbl +i)->sevr = INVALID_ALARM;
            
        }
    }
}

void PatternBuffer::BsaTimingProcessing(Tpr::TprStream * buf_current)
{
    if(!buf_current) {
        pLastBsaBuf = NULL;
        return ;
    }
    
    BsaTimingData *bsa_current = bsaTimingBuf + index;
    
    bsa_current->pulseId         = (uint64_t) GetPulseId(buf_current); 
    bsa_current->timeStamp       = * GetTimestamp(buf_current);
    bsa_current->edefInitMask    = (uint64_t) GetEdefInit(buf_current);
    bsa_current->edefActiveMask  = (uint64_t) GetEdefActive(buf_current);
    bsa_current->edefAllDoneMask = (uint64_t) GetEdefAllDone(buf_current);
    bsa_current->edefAvgDoneMask  = (uint64_t) GetEdefAvgDone(buf_current);
    bsa_current->edefUpdateMask  = 0;
    bsa_current->edefMinorMask   = (uint64_t) GetEdefMinor(buf_current);
    bsa_current->edefMajorMask   = (uint64_t) GetEdefMajor(buf_current);
    
    pLastBsaBuf = bsa_current;
}

int PatternBuffer::BsaCallbackProcessing(void)
{
    if(pBsaCallback && pLastBsaBuf) return pBsaCallback->CallFunctions(pLastBsaBuf);
    
    return -1;
}

int PatternBuffer::BsaCallbackRegister(BsaTimingCallback callback, void *pUserPvt) {
    if(!pBsaCallback) return -1;  
    
    return pBsaCallback->Register(callback, pUserPvt); 
}

int PatternBuffer::BsaCallbackReport(void)
{
    return pBsaCallback->Report();
}


void PatternBuffer::EventRateProcessing(void)
{
    for(int i =0; i < MAX_NUM_EVENT; i++) {
        EventTimeRate *p = event_tbl +i;
        
        p->rate    = (float) p->count / (float) MODULO720_SECS;
        p->ev_rate = (float) p->ev_count / (float) MODULO720_SECS;
        p->count   = 0;
        p->ev_count= 0;
            
   }
}

int PatternBuffer::FiducialCallbackProcessing(void)
{
    if(pFiducialCallback) return pFiducialCallback->CallFunctions();
    
    return -1;
}


int PatternBuffer::FiducialCallbackRegister(FIDUCIALFUNCTION fiducialFuc, void * fiducialArg)
{
    return pFiducialCallback->Register(fiducialFuc, fiducialArg);
}

int PatternBuffer::FiducialCallbackReport(void)
{
    return pFiducialCallback->Report();
}

void PatternBuffer::GetEventRate(uint32_t event_code, float *rate, float *ev_rate)
{
    if(!event_code) {
        *rate =0.; *ev_rate =0;
        return;
    }
    
    EventTimeRate *p = event_tbl + event_code;
    
    *rate    = p->rate;
    *ev_rate = p->ev_rate;
}

int PatternBuffer::EventTimePutPulseId(epicsTimeStamp *time, uint32_t pulse_id)
{
    time->nsec &= UPPER_15_BIT_MASK;
    time->nsec |= pulse_id;
    
    if(time->nsec >= NSEC_PER_SEC) {
        time->secPastEpoch ++;
        time->nsec -= NSEC_PER_SEC;
        time->nsec &= UPPER_15_BIT_MASK;
        time->nsec |= pulse_id;
    }

    return 0;
}

int PatternBuffer::EventTimeGet(epicsTimeStamp *time, uint32_t event_code)
{
    int status;
    if((uint32_t) epicsTimeEventBestTime == event_code) event_code = 0;
    if(event_code > (MAX_NUM_EVENT-1)) return -1;
    
    *time  = (event_tbl + event_code)->time;
    status = (event_tbl + event_code)->status;
    
    // debugging
    // if(event_code ==1) printf("thread id %d, pointer %p, pulse id %d\n",  epicsThreadGetIdSelf(), &(event_tbl +event_code)->time, PULSEID((event_tbl +event_code)->time));
    
    
    return status;  
    
}

int PatternBuffer::EventTimeGetSystem(epicsTimeStamp *time, uint32_t event_code)
{
    if(epicsTimeGetCurrent(time)) return -1;
    
    EventTimePutPulseId(time, PULSEID_INVALID);
    
 
     return 0;   
}


uint32_t PatternBuffer::GetBeamCode(Tpr::TprStream *buf)
{
    uint32_t beam_code = BEAMCODE(buf->dmod);

    return beam_code;
}

epicsTimeStamp* PatternBuffer::GetTimestamp(Tpr::TprStream *buf)
{
    epicsTimeStamp * p_ts = (epicsTimeStamp*) &buf->epics_time;

    return p_ts;
    
}

uint32_t PatternBuffer::GetSecond(Tpr::TprStream *buf)
{
    return GetTimestamp(buf)->secPastEpoch;
}

uint32_t PatternBuffer::GetNanoSecond(Tpr::TprStream *buf)
{
    return GetTimestamp(buf)->nsec;
}

uint32_t PatternBuffer::GetModifier1(Tpr::TprStream *buf)
{
    return buf->dmod[0];
}

uint32_t PatternBuffer::GetModifier2(Tpr::TprStream *buf)
{
    return buf->dmod[1];
}

uint32_t PatternBuffer::GetModifier3(Tpr::TprStream *buf)
{
    return buf->dmod[2];
}

uint32_t PatternBuffer::GetModifier4(Tpr::TprStream *buf)
{
    return buf->dmod[3];
}

uint32_t PatternBuffer::GetModifier5(Tpr::TprStream *buf)
{
    return buf->dmod[4];
}

uint32_t PatternBuffer::GetModifier6(Tpr::TprStream *buf)
{
    return buf->dmod[5];
}

uint32_t PatternBuffer::GetEdefAvgDone(Tpr::TprStream *buf)
{
    return buf->edef_avg_done;
}


uint32_t PatternBuffer::GetEdefMinor(Tpr::TprStream *buf)
{
    return buf->edef_minor;
}


uint32_t PatternBuffer::GetEdefMajor(Tpr::TprStream *buf)
{
    return buf->edef_major;
}


uint32_t PatternBuffer::GetEdefInit(Tpr::TprStream *buf)
{
    return buf->edef_init;
}


uint32_t PatternBuffer::GetEdefAllDone(Tpr::TprStream *buf)
{
    return ((GetEdefMajor(buf)>> 10)& 0x000ffc00)|((GetEdefMinor(buf) >> 20 ) & 0x000003ff );
}


uint32_t PatternBuffer::GetEdefActive(Tpr::TprStream *buf)
{
    return GetModifier5(buf) & MOD5_EDEF_MASK;
}


void  PatternBuffer::EnableSoftEvent(uint32_t event, uint32_t enable)
{
    uint32_t _disable(0), _enable(1);
    
    if(enable) (event_tbl + event)->enable = _enable; 
    else       (event_tbl + event)->enable = _disable;
}



};   /* namespace TprEvent */

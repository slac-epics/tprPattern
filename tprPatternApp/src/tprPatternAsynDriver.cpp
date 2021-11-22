#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <epicsVersion.h>

#include <ellLib.h>
#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsGeneralTime.h>
#include <generalTimeSup.h>



#include <iocsh.h>

#include <drvSup.h>
#include <epicsExport.h>
#include <epicsExit.h>

#include <asynPortDriver.h>
#include <asynOctetSyncIO.h>

#include <yamlLoader.h>

#include <tprPatternYaml.hh>
#include <tprEvent.hh>
#include <tprPatternAsynDriver.h>

#include <bsaCallbackApi.h>
#include <evrTime.h>


static const char          *driverName = "tpgPatternAsynDriver";
static tprPatternAsynDriver *p_asynDrv = NULL;


tprPatternAsynDriver::tprPatternAsynDriver(const char *portName, const char *corePath, const char *streamPath, const char *named_root)
    : asynPortDriver(portName,
                     1,
#if (ASYN_VERSION <<8 | ASYN_REVISION) < (4<<8 | 32)
                     NUM_TPR_PTRN_DET_PARAMS,
#endif /* check asyn version, under 4.32 */
                     asynInt32Mask | asynFloat64Mask | asynOctetMask | asynDrvUserMask | asynInt32ArrayMask | asynInt16ArrayMask,
                     asynInt32Mask | asynFloat64Mask | asynOctetMask | asynEnumMask    | asynInt32ArrayMask | asynInt16ArrayMask,
                     1,
                     1,
                     0,
                     0)
{
    Path _core, _stream;
    busType = _atca;

    if(named_root && !strlen(named_root)) named_root = NULL;
    if(corePath && strlen(corePath)) {
        if(!strncmp(corePath, "PCIe:/", 6)  || !strncmp(corePath, "pcie:/", 6)) busType = _pcie;
        else                                                                    busType = _atca;
    } else {
        busType = _atca; // default bus
    }

    switch(busType) {
        case _atca:
            _core   = ((!named_root)?cpswGetRoot():cpswGetNamedRoot(named_root))->findByName(corePath);
            _stream = ((!named_root)?cpswGetRoot():cpswGetNamedRoot(named_root))->findByName(streamPath);
            p_drv   = new Tpr::TprPatternYaml(_core, _stream);
            break;
        case _pcie:
            break;
    }


    p_patternBuffer = new TprEvent::PatternBuffer;

    stopPatternTask = false;
#if EPICS_VERSION_INT < VERSION_INT(7, 0, 3, 1)
    shutdownEvent = epicsEventMustCreate(epicsEventEmpty);
#endif
    
    strcpy(this->named_root, (named_root)?named_root:cpswGetRootName()); 
    strcpy(this->port_name, portName);
    strcpy(this->core_path, corePath);
    strcpy(this->stream_path, streamPath);
    
    CreateParameters();
}



int tprPatternAsynDriver::Report(int interest_level)
{
    printf("\tnamed_root:      %s\n", named_root);
    printf("\tport name:       %s\n", port_name);
    printf("\tcore path:       %s\n", core_path);
    printf("\tstream path:     %s\n", stream_path);
    printf("\tdriver location: %p\n", this);
    printf("\tapi location:    %p\n", p_drv);
    
    if(interest_level > 5) {
        p_patternBuffer->BsaCallbackReport();
        p_patternBuffer->FiducialCallbackReport();
        p_drv->PrintPattern(p_drv->_p_stream_buf, p_drv->_stream_size);
    }   

    return 0;
}

int tprPatternAsynDriver::tprPatternTask(void)
{
    printf("tprPatternAsynDriver: launch tprPatternTask (%ld)\n", TPR_STREAM_SIZE);
    Tpr::TprStream *buf_current, *buf2_advanced;
    
    // p_drv->TrainingStream();

    patternTaskId = epicsThreadGetIdSelf();
    
    while(!stopPatternTask) {
        p_patternBuffer->Lock();   /* global locking for pattern stream evolution */
        
            p_drv->Read(p_patternBuffer->GetNextBuf(), TPR_STREAM_SIZE);
            p_patternBuffer->EvolveBuf();
        
        
            buf_current   = p_patternBuffer->GetPipelineBuf(0);
            buf2_advanced = p_patternBuffer->GetPipelineBuf(2);
        
            p_patternBuffer->ActiveTimeslotProcessing(buf_current);
            p_patternBuffer->EdefProcessing(buf2_advanced, buf_current);
            // if(p_patternBuffer->IsActiveTimeslot(buf_current)) p_patternBuffer->BsaTimingProcessing(buf_current);  /* bsa processing only for active timeslot 120Hz */
            p_patternBuffer->BsaTimingProcessing(buf2_advanced);  /*360Hz bsa processing with advanced timint pattern */
        
        p_patternBuffer->Unlock();  /* unlock the global locking */
        
        p_patternBuffer->FiducialCallbackProcessing();
        // if(p_patternBuffer->IsActiveTimeslot(buf_current)) p_patternBuffer->BsaCallbackProcessing();  /* bsa processing only for active timeslot 120Hz */
        p_patternBuffer->BsaCallbackProcessing();  /* 360Hz bsa processing with advanced timing pattern */
        p_patternBuffer->EventProcessing(buf2_advanced, buf_current);
        
        TprDiagnostics();
    
    }

#if EPICS_VERSION_INT < VERSION_INT(7, 0, 3, 1)
    epicsEventSignal(shutdownEvent);
#endif
    
    return 0;
}

int tprPatternAsynDriver::tprPatternTaskStop(void)
{
    stopPatternTask = true;

#if EPICS_VERSION_INT < VERSION_INT(7, 0, 3, 1)   /* epics version check for backward compatibility */
/* before epics R7.0.3.1,   no thread join implemented */
    epicsEventWait(shutdownEvent);
#else
/* after epics R7.0.3.1,    join the thread */
    epicsThreadMustJoin(patternTaskId);
#endif
    return 0;
}

BsaTimingData * tprPatternAsynDriver::GetLastBsaTimingData(void)
{
    return p_patternBuffer->GetLastBsaTimingData();
}

int tprPatternAsynDriver::BsaCallbackRegister(BsaTimingCallback callback, void *pUserPvt)
{
    return p_patternBuffer->BsaCallbackRegister(callback, pUserPvt);
}


int tprPatternAsynDriver::FiducialCallbackRegister(FIDUCIALFUNCTION fiducialFuc, void * fiducialArg)
{
    return p_patternBuffer->FiducialCallbackRegister(fiducialFuc, fiducialArg);
}


int tprPatternAsynDriver::tprPatternEventTimeGet_gtWrapper(epicsTimeStamp *time, int eventCode)
{
    if(!p_patternBuffer) return -1;
    
    return p_patternBuffer->EventTimeGet(time, (uint32_t) eventCode); 
}

int tprPatternAsynDriver::tprPatternEventTimeGetSystem_gtWrapper(epicsTimeStamp *time, int eventCode)
{
    if(!p_patternBuffer) return -1;
    
    return p_patternBuffer->EventTimeGetSystem(time, (uint32_t) 0);
    
}



void tprPatternAsynDriver::CreateParameters(void)
{
    char param_name[64];
    
    createParam(activeTS1String, asynParamInt32, &p_activeTS1);
    createParam(activeTS2String, asynParamInt32, &p_activeTS2);
    
    for(int i = 0; i < MAX_NUM_TRIGGER; i ++) {
        sprintf(param_name, softEventSelect_eventCodeString, i); createParam(param_name, asynParamInt32,   &(p_soft_event_select+i)->event_code);
        sprintf(param_name, softEventSelect_enableString,    i); createParam(param_name, asynParamInt32,   &(p_soft_event_select+i)->enable);
        sprintf(param_name, softEventSelect_rateString,      i); createParam(param_name, asynParamFloat64, &(p_soft_event_select+i)->rate);
        sprintf(param_name, softEventSelect_evRateString,    i); createParam(param_name, asynParamFloat64, &(p_soft_event_select+i)->ev_rate);
    }    
    
    for(int i = 0; i < 4; i ++) {
        sprintf(param_name, patternDiag_pulseIdString, i);    createParam(param_name, asynParamInt32, &(p_patternDiag+i)->pulse_id);
        sprintf(param_name, patternDiag_beamCodeString, i);   createParam(param_name, asynParamInt32, &(p_patternDiag+i)->beam_code);
        sprintf(param_name, patternDiag_timeslotString, i);   createParam(param_name, asynParamInt32, &(p_patternDiag+i)->timeslot);
        sprintf(param_name, patternDiag_timestampString, i);  createParam(param_name, asynParamInt32, &(p_patternDiag+i)->timestamp);
        sprintf(param_name, patternDiag_secondString, i);     createParam(param_name, asynParamInt32, &(p_patternDiag+i)->second);
        sprintf(param_name, patternDiag_nanoSecondString, i); createParam(param_name, asynParamInt32, &(p_patternDiag+i)->nano_second);
        sprintf(param_name, patternDiag_modifier1String, i);  createParam(param_name, asynParamInt32, &(p_patternDiag+i)->modifier1);
        sprintf(param_name, patternDiag_modifier2String, i);  createParam(param_name, asynParamInt32, &(p_patternDiag+i)->modifier2);
        sprintf(param_name, patternDiag_modifier3String, i);  createParam(param_name, asynParamInt32, &(p_patternDiag+i)->modifier3);
        sprintf(param_name, patternDiag_modifier4String, i);  createParam(param_name, asynParamInt32, &(p_patternDiag+i)->modifier4);
        sprintf(param_name, patternDiag_modifier5String, i);  createParam(param_name, asynParamInt32, &(p_patternDiag+i)->modifier5);
        sprintf(param_name, patternDiag_modifier6String, i);  createParam(param_name, asynParamInt32, &(p_patternDiag+i)->modifier6);
        sprintf(param_name, patternDiag_edefAvgDoneString, i);  createParam(param_name, asynParamInt32, &(p_patternDiag+i)->edef_avg_done);
        sprintf(param_name, patternDiag_edefMinorString, i);    createParam(param_name, asynParamInt32, &(p_patternDiag+i)->edef_minor);
        sprintf(param_name, patternDiag_edefMajorString, i);    createParam(param_name, asynParamInt32, &(p_patternDiag+i)->edef_major);
        sprintf(param_name, patternDiag_edefInitString, i);     createParam(param_name, asynParamInt32, &(p_patternDiag+i)->edef_init);
        sprintf(param_name, patternDiag_edefActiveString, i);   createParam(param_name, asynParamInt32, &(p_patternDiag+i)->edef_active);
        sprintf(param_name, patternDiag_edefAllDoneString, i);  createParam(param_name, asynParamInt32, &(p_patternDiag+i)->edef_all_done);
        sprintf(param_name, patternDiag_statusString, i);       createParam(param_name, asynParamInt32, &(p_patternDiag+i)->status);
    }
}

void tprPatternAsynDriver::TprDiagnostics(void)
{
    Tpr::TprStream *buf[4];
    buf[0] = p_patternBuffer->GetPipelineBuf(2);   /* 2 fiducial advanced pattern */
    buf[1] = p_patternBuffer->GetPipelineBuf(1);   /* 1 fiducial advanced pattern */
    buf[2] = p_patternBuffer->GetPipelineBuf(0);   /* current pattern */
    buf[3] = p_patternBuffer->GetPipelineBuf(-1);  /* 1 fiducial past pattern */
    
    if(!buf[0] || !buf[1] || !buf[2] || !buf[3] || !p_patternBuffer->IsDiag(buf[0])) return;
    
    
    
    p_patternBuffer->EventRateProcessing();
    UpdateDiagPVs(buf);

}

void tprPatternAsynDriver::UpdateDiagPVs(Tpr::TprStream **buf)
{
    for(int i = 0; i < 4; i++) {
        setIntegerParam((p_patternDiag +i)->pulse_id,    p_patternBuffer->GetPulseId(buf[i]));
        setIntegerParam((p_patternDiag +i)->beam_code,   p_patternBuffer->GetBeamCode(buf[i]));
        setIntegerParam((p_patternDiag +i)->timeslot,    p_patternBuffer->GetTimeslot(buf[i]));
        setIntegerParam((p_patternDiag +i)->second,      p_patternBuffer->GetSecond(buf[i]));
        setIntegerParam((p_patternDiag +i)->nano_second, p_patternBuffer->GetNanoSecond(buf[i]));
        setIntegerParam((p_patternDiag +i)->modifier1,   p_patternBuffer->GetModifier1(buf[i]));
        setIntegerParam((p_patternDiag +i)->modifier2,   p_patternBuffer->GetModifier2(buf[i]));
        setIntegerParam((p_patternDiag +i)->modifier3,   p_patternBuffer->GetModifier3(buf[i]));
        setIntegerParam((p_patternDiag +i)->modifier4,   p_patternBuffer->GetModifier4(buf[i]));
        setIntegerParam((p_patternDiag +i)->modifier5,   p_patternBuffer->GetModifier5(buf[i]));
        setIntegerParam((p_patternDiag +i)->modifier6,   p_patternBuffer->GetModifier6(buf[i]));
        setIntegerParam((p_patternDiag +i)->edef_avg_done, p_patternBuffer->GetEdefAvgDone(buf[i]));
        setIntegerParam((p_patternDiag +i)->edef_minor,    p_patternBuffer->GetEdefMinor(buf[i]));
        setIntegerParam((p_patternDiag +i)->edef_major,    p_patternBuffer->GetEdefMajor(buf[i]));
        setIntegerParam((p_patternDiag +i)->edef_init,     p_patternBuffer->GetEdefInit(buf[i]));
        setIntegerParam((p_patternDiag +i)->edef_active,   p_patternBuffer->GetEdefActive(buf[i]));
        setIntegerParam((p_patternDiag +i)->edef_all_done, p_patternBuffer->GetEdefAllDone(buf[i]));
        setIntegerParam((p_patternDiag +i)->status, 0); // need to add status in future
    }
    
    for(int i =0; i < MAX_NUM_TRIGGER; i++) {
        int event_code;
        float rate, ev_rate;
        
        getIntegerParam((p_soft_event_select +i)->event_code, &event_code);
        if(event_code > (MAX_NUM_EVENT-1)) event_code = 0;
        
        p_patternBuffer->GetEventRate(event_code, &rate, &ev_rate);
        setDoubleParam((p_soft_event_select+i)->rate,    (double) rate);
        setDoubleParam((p_soft_event_select+i)->ev_rate, (double) ev_rate);
        
    }
    
    callParamCallbacks();
}


asynStatus tprPatternAsynDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int           function        = pasynUser->reason;
    asynStatus    status          = asynSuccess;
    const char    *functionName   = "writeInt32";
    
    /* set parameter into parameter library */
    status = (asynStatus) setIntegerParam(function, value);
    
    switch(function) {
        default:
            break;
    }
    
    if(function == p_activeTS1 || function == p_activeTS2) {
        int ts1, ts2;
        getIntegerParam(p_activeTS1, &ts1);  if(ts1<0 || ts1>6) ts1=0;
        getIntegerParam(p_activeTS2, &ts2);  if(ts2<0 || ts2>6) ts2=0;
        p_patternBuffer->SetActiveTimeslots(ts1, ts2);
    }
    
    for(int i= 0; i < MAX_NUM_TRIGGER; i++) {
        if(function == (p_soft_event_select+i)->event_code || function == (p_soft_event_select+i)->enable) {
            int event_code, enable;
            getIntegerParam((p_soft_event_select+i)->event_code, &event_code);
            getIntegerParam((p_soft_event_select+i)->enable,     &enable);
            if(event_code < 1 || event_code > (MAX_NUM_EVENT-1)) break;
            p_patternBuffer->EnableSoftEvent((uint32_t) event_code, (uint32_t) (enable?1:0));
        }
    }
    
    
    
    /* Do callback so higher layers see any changes */
    status = (asynStatus) callParamCallbacks();
    
    if(status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                      "%s:%s: status=%d, function=%d, value=%d",
                      driverName, functionName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                  "%s:%s: function=%d, value=%d\n",
                  driverName, functionName, function, value);
    
    return status;
}         




extern "C" {

//
//   bsaCallback in Timing API
//

int RegisterBsaTimingCallback(BsaTimingCallback callback, void *pUserPvt)
{
    if(!p_asynDrv) return -1;
    
    return p_asynDrv->BsaCallbackRegister(callback, pUserPvt);
}

//
//   dummy initialization
//
int  evrInitialize(void)
{
	return 0;
}


//
//    evrTime API for backward compatibility
//
//

int evrTimeRegister(FIDUCIALFUNCTION fiducialFunc, void * fiducialArg)
{
    if(!p_asynDrv) return -1;
    
    return p_asynDrv->FiducialCallbackRegister(fiducialFunc, fiducialArg);
}

int evrTimeGetFromPipeline(epicsTimeStamp  *epicsTime_ps,
                           evrTimeId_te     id,
                           evrModifier_ta   modifier_a,
                           epicsUInt32     *patternStatus_p,
                           epicsUInt32     *edefAvgDoneMask_p,
                           epicsUInt32     *edefMinorMask_p,
                           epicsUInt32     *edefMajorMask_p)
{
    TprEvent::PatternBuffer *p_patternBuffer;
    Tpr::TprStream *buf;
    if(!p_asynDrv || 
       !(p_patternBuffer = p_asynDrv->GetPatternBuffer()) ||
       ((int) id ==3) || ((int) id > 4) ||
       !(buf = (((int)id != 4)? p_patternBuffer->GetPipelineBuf((int) id): p_patternBuffer->GetLastActiveBuf()))) {
       
        if(epicsTime_ps) {
            epicsTime_ps->secPastEpoch = 0;
            epicsTime_ps->nsec =0;
        }
        
        if(modifier_a) for(int i =0; i < MAX_MODIFIER; i++) modifier_a[i] = 0;
        if(patternStatus_p)   * patternStatus_p   = 0;
        if(edefAvgDoneMask_p) * edefAvgDoneMask_p = 0;
        if(edefMinorMask_p)   * edefMinorMask_p   = 0;
        if(edefMajorMask_p)   * edefMajorMask_p   = 0;
        
        return -1;
    }
    
    if(epicsTime_ps) *epicsTime_ps = *p_patternBuffer->GetTimestamp(buf);
    if(modifier_a) {
        modifier_a[0] = p_patternBuffer->GetModifier1(buf);
        modifier_a[1] = p_patternBuffer->GetModifier2(buf);
        modifier_a[2] = p_patternBuffer->GetModifier3(buf);
        modifier_a[3] = p_patternBuffer->GetModifier4(buf);
        modifier_a[4] = p_patternBuffer->GetModifier5(buf);
        modifier_a[5] = p_patternBuffer->GetModifier6(buf);
    }
    if(patternStatus_p)   * patternStatus_p   = 0;
    if(edefAvgDoneMask_p) * edefAvgDoneMask_p = p_patternBuffer->GetEdefAvgDone(buf);
    if(edefMinorMask_p)   * edefMinorMask_p   = p_patternBuffer->GetEdefMinor(buf);
    if(edefMajorMask_p)   * edefMajorMask_p   = p_patternBuffer->GetEdefMajor(buf);  
    
    

    return 0;
}

int evrTimeGetFromEdef    (unsigned int     edefIdx,
                           epicsTimeStamp  *edefTime_ps,
                           epicsTimeStamp  *edefTimeInit_ps,
                           int             *edefAvgDone_p,
                           epicsEnum16     *edefSevr_p)
{
    TprEvent::PatternBuffer *p_patternBuffer;
    TprEvent::EdefTimeTbl   *p_edef;
    if(!p_asynDrv  ||
       !(p_patternBuffer = p_asynDrv->GetPatternBuffer())  ||
       edefIdx >= EDEF_MAX  ||
       !(p_edef = p_patternBuffer->GetEdefTable(edefIdx))) {
        if(edefTime_ps) {
            edefTime_ps->secPastEpoch = 0;
            edefTime_ps->nsec         = 0;
        }
        if(edefTimeInit_ps) {
            edefTimeInit_ps->secPastEpoch = 0;
            edefTimeInit_ps->nsec         = 0;
        }
        
        if(edefAvgDone_p) *edefAvgDone_p = 0;
        if(edefSevr_p)    *edefSevr_p    = 0;
    
        return -1;
    }
    
    if(edefTime_ps)      *edefTime_ps     = * p_patternBuffer->GetEdefTimestamp(p_edef);
    if(edefTimeInit_ps)  *edefTimeInit_ps = * p_patternBuffer->GetEdefInitTimestamp(p_edef);
    if(edefAvgDone_p)    *edefAvgDone_p   = p_patternBuffer->GetEdefAvgDone(p_edef);
    if(edefSevr_p)       *edefSevr_p      = p_patternBuffer->GetEdefSevr(p_edef);

    return 0;
}


int evrTimeGet            (epicsTimeStamp  *epicsTime_ps,
                           unsigned int     eventCode)
{
    TprEvent::PatternBuffer  * p_patternBuffer;
    if(!p_asynDrv ||
        !(p_patternBuffer = p_asynDrv->GetPatternBuffer())) {
        epicsTime_ps->secPastEpoch = 0;
        epicsTime_ps->nsec         = 0;
        return -1;
    }

    return p_patternBuffer->EventTimeGet(epicsTime_ps, (uint32_t) eventCode);
}


int evrTimePutPulseID (epicsTimeStamp  *epicsTime_ps, unsigned int pulseID)
{
  epicsTime_ps->nsec &= UPPER_15_BIT_MASK;
  epicsTime_ps->nsec |= pulseID;
  if (epicsTime_ps->nsec >= NSEC_PER_SEC) {
    epicsTime_ps->secPastEpoch++;
    epicsTime_ps->nsec -= NSEC_PER_SEC;
    epicsTime_ps->nsec &= UPPER_15_BIT_MASK;
    epicsTime_ps->nsec |= pulseID;
  }
  return 0;
}





//
//
//
//


tprPatternAsynDriver * Get_tprPatternAsynDriver(void)
{
    return p_asynDrv;
}

static int tprPatternTask(void)
{
    if(!p_asynDrv) return -1;
    
    p_asynDrv->tprPatternTask();

    return 0;
}

static int tprPatternTaskStop(void)
{
    if(!p_asynDrv) return -1;

    p_asynDrv->tprPatternTaskStop();
    printf("tprPatternTask: stop\n");

    return 0;
}


static int tprPatternEventTimeGet_gtWrapper(epicsTimeStamp *time, int eventCode)
{
    if(!p_asynDrv) return -1;
    
    return p_asynDrv->tprPatternEventTimeGet_gtWrapper(time, eventCode);
}

static int tprPatternEventTimeGetSystem_gtWrapper(epicsTimeStamp *time, int eventCode)
{
    if(!p_asynDrv) return -1;
    
    return p_asynDrv->tprPatternEventTimeGetSystem_gtWrapper(time, eventCode);
}

static void tprPatternAsynDriverConfigure(const char *port_name, const char *core_path, const char *stream_path, const char *named_root)
{
    if(p_asynDrv) {
        printf("The tprPatternAsynDriver has been initialized already (%p)\n", p_asynDrv);
        return;
    }
    
    p_asynDrv = new tprPatternAsynDriver(port_name, core_path, stream_path, named_root);
}

static int tprPatternAsynDriverReport(int interest);
static int tprPatternAsynDriverInitialize(void);
static struct drvet tprPatternAsynDriver = {
    2,
    (DRVSUPFUN) tprPatternAsynDriverReport,
    (DRVSUPFUN) tprPatternAsynDriverInitialize
};

epicsExportAddress(drvet, tprPatternAsynDriver);



static int tprPatternAsynDriverReport(int interest)
{
    if(!p_asynDrv) {
        printf("The tprPatternAsynDrver never been intialized.\n");
        return 0;
    }
    
    p_asynDrv->Report(interest);
    
    return 0;
}

static int tprPatternAsynDriverInitialize(void)
{


    if(!p_asynDrv) return 0;

#if EPICS_VERSION_INT < VERSION_INT(7, 0, 3, 1)    /* epics version check for backward compatibility */
/* before epics R7.0.3.1,   use un-joinable thread */

    epicsThreadCreate("tprPatternTask", epicsThreadPriorityHigh + 5,
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      (EPICSTHREADFUNC) tprPatternTask ,0);

#else   
/* epics R7.0.3.1 or later,    use joinable thread */

    epicsThreadOpts opts = EPICS_THREAD_OPTS_INIT;
    opts.priority = epicsThreadPriorityHigh + 5;
    opts.stackSize = epicsThreadGetStackSize(epicsThreadStackMedium);
    opts.joinable = 1;

    epicsThreadCreateOpt("tprPatternTask", (EPICSTHREADFUNC) tprPatternTask, 0, &opts);

#endif

    epicsAtExit3((epicsExitFunc) tprPatternTaskStop, (void*) NULL, "tprPatternTaskStop");
    
    generalTimeRegisterEventProvider("patternTimeGet",       1000, (TIMEEVENTFUN) tprPatternEventTimeGet_gtWrapper);
    generalTimeRegisterEventProvider("patternTimeGetSystem", 2000, (TIMEEVENTFUN) tprPatternEventTimeGetSystem_gtWrapper);  
    

    return 0;
}





static const iocshArg initArg0 = { "port name",    iocshArgString };
static const iocshArg initArg1 = { "core path",    iocshArgString };
static const iocshArg initArg2 = { "sstream path", iocshArgString };
static const iocshArg initArg3 = { "named_root (optional)", iocshArgString };
static const iocshArg * const initArgs[] = { &initArg0, 
                                             &initArg1,
                                             &initArg2,
                                             &initArg3 };
static const iocshFuncDef initFuncDef = { "tprPatternAsynDriverConfigure", 4, initArgs };
static void  initCallFunc(const iocshArgBuf *args)
{
    tprPatternAsynDriverConfigure(args[0].sval, args[1].sval, args[2].sval,
                                 (args[3].sval && strlen(args[3].sval))?args[3].sval:NULL );
}


void tprPatternAsynDriverRegister(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
}

epicsExportRegistrar(tprPatternAsynDriverRegister);


} /* extern C */


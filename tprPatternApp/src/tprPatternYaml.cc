#include <cpsw_api_builder.h>
#include <cpsw_api_user.h>


#include <tprPatternYaml.hh>

#include <stdio.h>
#include <string.h>

using namespace Tpr;

TprPatternYaml::TprPatternYaml(Path core, Path tstream)
{
    _path             = core;
    _serverRemoteIp   = IScalVal_RO::create(_path->findByName("TimingUdpServer/ServerRemoteIp"));
    _serverRemotePort = IScalVal_RO::create(_path->findByName("TimingUdpServer/ServerRemotePort"));
    _msgNoDelay       = IScalVal   ::create(_path->findByName("AmcCarrierTiming/TimingFrameRx/MsgNoDelay"));
    _outputConfig0    = IScalVal   ::create(_path->findByName("AxiSy56040/OutputConfig[0]"));
    _outputConfig1    = IScalVal   ::create(_path->findByName("AxiSy56040/OutputConfig[1]"));
    _outputConfig2    = IScalVal   ::create(_path->findByName("AxiSy56040/OutputConfig[2]"));
    _outputConfig3    = IScalVal   ::create(_path->findByName("AxiSy56040/OutputConfig[3]"));
    
    _stream = IStream::create(tstream);
    _read_cnt = 0;
    
    // SetupTimingFrameInput(0); /* use TPGMini timing frame from FPGA */
    SetupTimingFrameInput(1); /* use timing from backplane */
    
    // SetupMsgNoDelay(0);    /* delay 3 fiducials for LCLS1 timing pattern delivery */
    SetupMsgNoDelay(1);    /* prompt delivery for LCLS1 timing pattern */
    
}

uint32_t TprPatternYaml::GetDestinationIp(void)
{
    uint32_t     val;
    IndexRange   rng(0);
    
    _serverRemoteIp->getVal(&val, 1, &rng);
    
    return val;
}

uint32_t TprPatternYaml::GetDestinationPort(void)
{
    uint32_t    val;
    IndexRange  rng(0);
    
    _serverRemotePort->getVal(&val, 1, &rng);
    
    return val;
}

uint32_t TprPatternYaml::GetOutputConfig0(void)
{
    uint32_t    val;
    IndexRange  rng(0);
    
    _outputConfig0->getVal(&val, 1, &rng);
    
    return val;
}

uint32_t TprPatternYaml::GetOutputConfig1(void)
{
    uint32_t    val;
    IndexRange  rng(0);
    
    _outputConfig1->getVal(&val, 1, &rng);
    
    return val;
}

uint32_t TprPatternYaml::GetOutputConfig2(void)
{
    uint32_t    val;
    IndexRange  rng(0);
    
    _outputConfig2->getVal(&val, 1, &rng);
    
    return val;
}

uint32_t TprPatternYaml::GetOutputConfig3(void)
{
    uint32_t    val;
    IndexRange  rng(0);
    
    _outputConfig3->getVal(&val, 1, &rng);
    
    return val;
}

void TprPatternYaml::SetupTimingFrameInput(uint32_t input)
{
    uint32_t zero(0), one(1), two(2), three(3);
    if(input == 0) {
        // using TpgMini /* 0, 1, 0, 0  */
         _outputConfig0->setVal(&zero);
         _outputConfig1->setVal(&one);
         _outputConfig2->setVal(&zero);
         _outputConfig3->setVal(&zero);
    } else if(input == 1) {
        // using EVG from backplaine  /* 0, 0, 1, 0 */
         _outputConfig0->setVal(&zero);
         _outputConfig1->setVal(&two);
         _outputConfig2->setVal(&zero);
         _outputConfig3->setVal(&zero);
    } 
}

void TprPatternYaml::SetupMsgNoDelay(uint32_t no_delay)
{
    uint32_t zero(0), one(1);
    
    if(no_delay == 1) _msgNoDelay->setVal(&one);  
    else              _msgNoDelay->setVal(&zero);
} 
 
void     TprPatternYaml::TrainingStream(void)
{
    char *_T_STREAM = ">>*TSTREAM*<<"; 
    _stream_size = _stream->write((uint8_t*) _T_STREAM, (uint64_t) strlen(_T_STREAM), CTimeout());
}
uint32_t TprPatternYaml::Read(void *buf, int stream_size)
{
    _stream_size  =  _stream->read((uint8_t*) buf, (uint64_t) stream_size, CTimeout());
    _p_stream_buf = (TprStream*) buf;
    _read_cnt ++;
}

int TprPatternYaml::PrintPattern(TprStream *p, uint32_t size)
{
    char *tcross[4] = { "RTM_IN0", "FPGA_IN", "BP_IN", "RTM_IN1" };
    printf("Timing Crossbar status: RTM_Out to %s, FPGA_Out to %s, BP_DIST0 to %s, BP_DIST1 to %s\n",
           tcross[GetOutputConfig0()],
           tcross[GetOutputConfig1()],
           tcross[GetOutputConfig2()],
           tcross[GetOutputConfig3()]);
    
    printf("Stream Training Status: (destination IP %u, destination port %u)\n", GetDestinationIp(), GetDestinationPort());
    printf("LCLS1 Timing Pattern (location: %8.8x, size: %u)\n", p, size);
    printf("receiving counter: %u\n", _read_cnt);
    printf("pulse id: %lu\n", p->pulse_id);
    for(int i =0; i< 8; i++) printf("event code mask [%d]: %8.8x\n", i, p->event_code[i]);
    printf("dtype/version: %u/%u\n", p->dtype, p->version);
    for(int i =0; i<6; i++) printf("modifier [%d]: %8.8x\n", i, p->dmod[i]);
    printf("epics timestamp %lu\n", p->epics_time);
    typedef struct {
        uint32_t sec;
        uint32_t nsec;
    } _ts;
    _ts *pts = (_ts*) &p->epics_time;
    printf("\tsec %8.8x, nsec %8.8x\n", pts->sec, pts->nsec); 
    printf("edef mask (avg done): %8.8x\n", p->edef_avg_done);
    printf("edef mask (minor):    %8.8x\n", p->edef_minor);
    printf("edef mask (major):    %8.8x\n", p->edef_major);
    printf("edef mask (init):     %8.8x\n", p->edef_init);
    
    
    return 0;
}
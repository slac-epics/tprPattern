//////////////////////////////////////////////////////////////////////////////
// This file is part of 'tprPattern'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'tprPattern', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#ifndef TPR_PATTERN_COMMON_H
#define TPR_PATTERN_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
/* Masks used to decode pulse ID from the nsec part of the timestamp   */
#define UPPER_15_BIT_MASK       (0xFFFE0000)    /* (2^32)-1 - (2^17)-1 */
#define LOWER_17_BIT_MASK       (0x0001FFFF)    /* (2^17)-1            */
/* Pulse ID Definitions */
#define PULSEID(time)           ((time).nsec & LOWER_17_BIT_MASK)
#define PULSEID_BIT17           (0x00020000)    /* Bit up from pulse ID*/
#define PULSEID_INVALID         (0x0001FFFF)    /* Invalid Pulse ID    */
#define PULSEID_MAX             (0x0001FFDF)    /* Rollover value      */
#define PULSEID_RESYNC          (0x0001E000)    /* Resync Pulse ID     */
#define NSEC_PER_SEC            1E9             /* # nsec in one sec   */

#define MODULO720_SECS          2               /* # sec for MODULO720 */
#define TIMESLOT_MIN            1               /* Minimum time slot   */
#define TIMESLOT_MAX            6               /* Maximum time slot   */
#define TIMESLOT_DIFF           3               /* Timeslot difference */

#define MAX_MODIFIER            6
/* Event codes - see mrfCommon.h for reserved internal event codes      */
#define EVENT_FIDUCIAL          1        /* Fiducial event code         */
#define EVENT_EXTERNAL_TRIG     100      /* External trigger event code */
#define EVENT_MODULO720         121      /* Modulo 720 event code       */
#define EVENT_MPG               122      /* MPG update event code       */
#define EVENT_MODULO36_MIN      201      /* Min modulo 36 event code    */
#define EVENT_MODULO36_MAX      236      /* Max modulo 36 event code    */
#define MODULO36_MAX            36       /* # modulo 36 event codes     */


/* Definitions and typedefs shared by evrPattern.c and mpgPattern.c  */
  
/* Masks used to decode beam code and YY from modifier1 */
#define MOD1_IDX                0  
#define BEAMCODE_BIT_MASK       (0x0000001F)  /* Beam code mask        */
                                              /* Left shift 8 first    */
#define BEAMCODE(mod_a)         ((((mod_a)[0]) >> 8) & BEAMCODE_BIT_MASK)
#define YY_BIT_MASK             (0x000000FF)  /* YY code mask          */
/* Other useful bits in modifier1                       */
#define MODULO720_MASK          (0x00008000)  /* Set to sync modulo 720*/
#define MPG_IPLING              (0x00004000)  /* Set on MPG/EVG problem*/
/* Bits in modifier 2                                                  */
#define MOD2_IDX                1  
#define EVG_BURST               (0x00000040)  /*Single-shot/burst pulse*/
#define EVG_HXR_BURST		(0x00000100)  /* BYKIK HXR Burst pulse */
#define EVG_SXR_BURST		(0x00000200)  /* BYKIKS SXR Burst pulse*/
#define BYKIKS                  (0x10000000)  /* BYKIKS trigger        */
#define BYKIK                   (0x20000000)  /* BYKIK trigger         */
#define TCAV3_PERM              (0x40000000)  /* TCAV3                 */
/* Mask used to decode timeslot 1 to 6 from modifier2   */
#define TIMESLOT_MASK           (0x0000003F)  /* timeslot   mask       */
#define TIMESLOT1_MASK          (0x00000001)  /* timeslot 1 mask       */
#define TIMESLOT2_MASK          (0x00000002)  /* timeslot 2 mask       */
#define TIMESLOT3_MASK          (0x00000004)  /* timeslot 3 mask       */
#define TIMESLOT4_MASK          (0x00000008)  /* timeslot 4 mask       */
#define TIMESLOT5_MASK          (0x00000010)  /* timeslot 5 mask       */
#define TIMESLOT6_MASK          (0x00000020)  /* timeslot 6 mask       */
/* Bits in modifier 3                                                  */
#define MOD3_IDX                2  
#define POCKCEL_PERM            (0x00080000)  /* Pockels cell permit   */
#define TCAV0_PERM              (0x80000000)  /* TCAV0                 */
//CLTS Project MPS Modifier Bits to set rates to two destinations:
/* Masks defined for CLTS timing MPS communication */
#define RATE_MPS_HXR_119HZ    (0x00000001)  /* RATE_MPS_HXR_119HZ  BITP 64   */
#define RATE_MPS_HXR_110HZ    (0x00000002)  /* RATE_MPS_HXR_110HZ  BITP 65   */
#define RATE_MPS_HXR_90HZ     (0x00000004)  /* RATE_MPS_HXR_90HZ   BITP 66   */
#define RATE_MPS_HXR_60HZ     (0x00000008)  /* RATE_MPS_HXR_60HZ   BITP 67   */
#define RATE_MPS_HXR_30HZ     (0x00000010)  /* RATE_MPS_HXR_30HZ   BITP 68   */
#define RATE_MPS_HXR_10HZ     (0x00000020)  /* RATE_MPS_HXR_10HZ   BITP 69   */
#define RATE_MPS_HXR_05HZ     (0x00000800)  /* RATE_MPS_HXR_05HZ   BITP 75   */
#define RATE_MPS_HXR_01HZ     (0x00001000)  /* RATE_MPS_HXR_01HZ   BITP 76   */
#define RATE_MPS_SXR_119HZ    (0x00002000)  /* RATE_ MPS_SXR_119HZ BITP 77   */
#define RATE_MPS_SXR_110HZ    (0x00004000)  /* RATE_ MPS_SXR_110HZ BITP 78   */
#define RATE_MPS_SXR_90HZ     (0x00008000)  /* RATE_ MPS_SXR_90HZ  BITP 79   */
#define RATE_MPS_SXR_60HZ     (0x00020000)  /* RATE_ MPS_SXR_60HZ  BITP 81   */
#define RATE_MPS_SXR_30HZ     (0x00040000)  /* RATE_ MPS_SXR_30HZ  BITP 82   */
#define RATE_MPS_SXR_10HZ     (0x00100000)  /* RATE_ MPS_SXR_10HZ  BITP 84   */
#define RATE_MPS_SXR_05HZ     (0x00200000)  /* RATE_ MPS_SXR_05HZ  BITP 85   */
#define RATE_MPS_SXR_01HZ     (0x00400000)  /* RATE_ MPS_SXR_01HZ  BITP 86   */
#define BKRCUS		      (0x00800000)  /* BKRCUS scheduled by EVGUI BITP 87*/

/* Mask used to get timeslot value from modifier4       */
#define MOD4_IDX                3  
#define TIMESLOT_VAL_MASK       (0x00000007)  /* Time slot value mask  */
                                              /* Left shift 29 first   */
#define TIMESLOT(mod_a)         ((((mod_a)[3]) >> 29) & TIMESLOT_VAL_MASK)

  
#define TIMESLOT_RATE_MAX         5           /* # limited rates       */
                                              /* 30, 10, 5, 1, 0.5hz   */

/* Masks defining modifier5 */  
#define MOD5_IDX                4  
#define EDEF_MAX                20            /* Max # event defns   */
#define MOD5_EDEF_MASK          (0x000FFFFF)  /* EDEF bits           */
#define MOD5_NOEDEF_MASK        (0xFFF00000)  /* Rate and User bits  */
#define MOD5_RATE_MASK          (0x01F00000)  /* Rate bits           */
#define MOD5_USER_MASK          (0x0E000000)  /* User-settable bits  */
#define MOD5_ACQ_MASK           (0xF0000000)  /* Acquisition bits    */
#define MOD5_EDEF1HZ_MASK       (0x00008000)  /* System EDEF - Beam & 1hz       */
#define MOD5_EDEF10HZ_MASK      (0x00010000)  /* System EDEF - Beam & 10hz      */
#define MOD5_EDEFFULL_MASK      (0x00020000)  /* System EDEF - Beam & full rate */
#define MOD5_30HZ_MASK          (0x00100000)  /* 30hz base rate        */
#define MOD5_10HZ_MASK          (0x00200000)  /* 10hz base rate        */
#define MOD5_5HZ_MASK           (0x00400000)  /* 5hz  base rate        */
#define MOD5_1HZ_MASK           (0x00800000)  /* 1hz  base rate        */
#define MOD5_HALFHZ_MASK        (0x01000000)  /* .5hz base rate        */
#define MOD5_BEAMFULL_MASK      (0x10000000)  /* Acquire at full rate  */

/* Masks defining modifier6 (MPS modifier) */  
#define MOD6_IDX                5  
#define MPS_DEST_MASK           (0x000FFFFE)  /* MPS Destination bits  */
#define MPS_PERM_MASK           (0xFFF00000)  /* MPS Permit bits       */
#define MPS_VALID               (0x00000001)  /* MPS Valid data        */

#define MPS_DEST_HXR_MASK   (0x00040000)  /* MPS Destination information to timing HXR  BITP 178 */
#define MPS_DEST_SXR_MASK   (0x00080000)  /* MPS Destination information to timing SXR  BITP 179 */

#define MPS_PERM_POCKCELL_MASK  (0x00100000)  /* MPSMitigationDevicePockelsCell        */
#define MPS_PERM_MECHSHUT_MASK  (0x00200000)  /* MPSMitigationDeviceMechanicalShutter  */
#define MPS_PERM_BYKIK_MASK     (0x00400000)  /* MPSMitigationDeviceBykik              */
#define MPS_PERM_BYKIKS_MASK     (0x00000002)  /* MPSMitigationDeviceBykiks	       */	
#define MPS_PERM_LHTRSHUT_MASK  (0x00800000)  /* MPSMitigationDeviceLaserHeaterShutter */

/* VAL values set by pattern subroutines */
#define PATTERN_OK                0
#define PATTERN_INVALID_WF        1
#define PATTERN_NO_DATA           2
#define PATTERN_INVALID_TIMESTAMP 3
#define PATTERN_MPG_IPLING        4
#define PATTERN_SEQ_CHECK1_ERR    5
#define PATTERN_SEQ_CHECK2_ERR    6
#define PATTERN_SEQ_CHECK3_ERR    7
#define PATTERN_PULSEID_NO_SYNC   8
#define PATTERN_MODULO720_NO_SYNC 9
#define PATTERN_TIMEOUT           10
#define PATTERN_ERROR             11




#define  BUF_INDEX_MASK   0x0000fff
#define  MAX_BUF_SIZE     (BUF_INDEX_MASK + 1)

#define  MAX_NUM_EVENT    256
#define  ACTIVE_TS        0


#ifndef DEF_FIDUCIALFUNCTION
#define DEF_FIDUCIALFUNCTION
typedef void (*FIDUCIALFUNCTION)(void *arg);
#endif  /* DEF_FIDUCIALFUNCTION */

  
#ifdef __cplusplus
}
#endif

#endif /* TPR_PATTERN_COMMON_H */


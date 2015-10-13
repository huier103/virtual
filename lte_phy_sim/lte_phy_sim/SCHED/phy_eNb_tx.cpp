#ifdef PHY_eNb_TX
#include "PHY/types.h"
#include "PHY/defs.h"
#include "LAYER2/MAC/defs.h"
#include "SCHED/defs.h"
#include "LAYER2/MAC/vars.h"
#include "SCHED/vars.h"
#include "SIMULATION/TOOLS/defs.h"
#include "PHY/LTE_TRANSPORT/defs.h"
#include "PHY/LTE_TRANSPORT/proto.h"
#include "PHY/INIT/defs.h"
#include "PHY/vars.h"
int number_of_cards=3;
PHY_VARS_eNB *PHY_vars_eNB;
int exit_openair =0;//..wanlin 此处定义在sched.c
#ifndef LINUX_LTE
__declspec(align(16))unsigned char dlsch_input_buffer[2700];
__declspec(align(16))int eNB_sync_buffer0[640*6];
__declspec(align(16))int eNB_sync_buffer1[640*6];
#else
unsigned char dlsch_input_buffer[2700] __attribute__ ((aligned(16)));
int eNB_sync_buffer0[640*6] __attribute__ ((aligned(16)));
int eNB_sync_buffer1[640*6] __attribute__ ((aligned(16)));
#endif

void fill_dci(DCI_PDU *DCI_pdu, u8 subframe, u8 cooperation_flag) {

  //u32 rballoc = (((1<<(openair_daq_vars.target_ue_ul_mcs/2))-1)<<(openair_daq_vars.target_ue_dl_mcs/2)) & 0x1FFF;
  u32 rballoc = 0x00F0;
  u32 rballoc2 = 0x000F;
  /*
  u32 rand = taus();
  if ((subframe==8) || (subframe==9) || (subframe==0))
    rand = (rand%5)+5;
  else
    rand = (rand%4)+5;
  */

  DCI_pdu->Num_common_dci = 0;
  DCI_pdu->Num_ue_spec_dci=0;

  switch (subframe) {

  case 5:
    DCI_pdu->Num_common_dci = 1;
    DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_5MHz_TDD_1_6_t;
    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = SI_RNTI;
    DCI_pdu->dci_alloc[0].format     = format1A;

    BCCH_alloc_pdu.type              = 1;
    BCCH_alloc_pdu.vrb_type          = 0;
    BCCH_alloc_pdu.rballoc           = computeRIV(25,10,3);
    BCCH_alloc_pdu.ndi               = 1;
    BCCH_alloc_pdu.rv                = 1;
    BCCH_alloc_pdu.mcs               = 1;
    BCCH_alloc_pdu.harq_pid          = 0;
    BCCH_alloc_pdu.TPC               = 1;      // set to 3 PRB
    memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&BCCH_alloc_pdu,sizeof(DCI1A_5MHz_TDD_1_6_t));
    break;
  case 6:
    /*
    DCI_pdu->Num_ue_spec_dci = 1;
    DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI2_5MHz_2A_M10PRB_TDD_t;
    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = 0x1236;
    DCI_pdu->dci_alloc[0].format     = format2_2A_M10PRB;

    DLSCH_alloc_pdu1.rballoc          = 0x00ff;
    DLSCH_alloc_pdu1.TPC              = 0;
    DLSCH_alloc_pdu1.dai              = 0;
    DLSCH_alloc_pdu1.harq_pid         = 0;
    DLSCH_alloc_pdu1.tb_swap          = 0;
    DLSCH_alloc_pdu1.mcs1             = 0;
    DLSCH_alloc_pdu1.ndi1             = 1;
    DLSCH_alloc_pdu1.rv1              = 0;
    DLSCH_alloc_pdu1.tpmi             = 0;
    memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&DLSCH_alloc_pdu1,sizeof(DCI2_5MHz_2A_M10PRB_TDD_t));
    */
    break;
  case 7:
    DCI_pdu->Num_ue_spec_dci = 2;

    DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_5MHz_TDD_t; 
    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = 0x1235;
    DCI_pdu->dci_alloc[0].format     = format1;
    
    DLSCH_alloc_pdu.rballoc          = rballoc;
    DLSCH_alloc_pdu.TPC              = 0;
    DLSCH_alloc_pdu.dai              = 0;
    DLSCH_alloc_pdu.harq_pid         = 1;
    DLSCH_alloc_pdu.mcs              = openair_daq_vars.target_ue_dl_mcs;
    DLSCH_alloc_pdu.ndi              = 1;
    DLSCH_alloc_pdu.rv               = 0;
    memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&DLSCH_alloc_pdu,sizeof(DCI1_5MHz_TDD_t));

    DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1_5MHz_TDD_t; 
    DCI_pdu->dci_alloc[1].L          = 2;
    DCI_pdu->dci_alloc[1].rnti       = 0x1236;
    DCI_pdu->dci_alloc[1].format     = format1;

    DLSCH_alloc_pdu.rballoc          = rballoc2;
    DLSCH_alloc_pdu.TPC              = 0;
    DLSCH_alloc_pdu.dai              = 0;
    DLSCH_alloc_pdu.harq_pid         = 1;
    DLSCH_alloc_pdu.mcs              = openair_daq_vars.target_ue_dl_mcs;
    DLSCH_alloc_pdu.ndi              = 1;
    DLSCH_alloc_pdu.rv               = 0;
    memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&DLSCH_alloc_pdu,sizeof(DCI1_5MHz_TDD_t));
    break;

  case 9:
    /*
    DCI_pdu->Num_common_dci = 1;
    DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_5MHz_TDD_1_6_t;
    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = RA_RNTI;
    DCI_pdu->dci_alloc[0].format     = format1A;

    RA_alloc_pdu.type                = 1;
    RA_alloc_pdu.vrb_type            = 0;
    RA_alloc_pdu.rballoc             = computeRIV(25,10,4);
    RA_alloc_pdu.ndi      = 1;
    RA_alloc_pdu.rv       = 1;
    RA_alloc_pdu.mcs      = 1;
    RA_alloc_pdu.harq_pid = 0;
    RA_alloc_pdu.TPC      = 1;

    memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&RA_alloc_pdu,sizeof(DCI1A_5MHz_TDD_1_6_t));
    */
    break;

  case 8:
    DCI_pdu->Num_ue_spec_dci = 2;

    //user 1
    DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI0_5MHz_TDD_1_6_t ; 
    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = 0x1235;
    DCI_pdu->dci_alloc[0].format     = format0;

    UL_alloc_pdu.type    = 0;
    UL_alloc_pdu.hopping = 0;
    UL_alloc_pdu.rballoc = computeRIV(25,4,openair_daq_vars.ue_ul_nb_rb);
    UL_alloc_pdu.mcs     = openair_daq_vars.target_ue_ul_mcs;
    UL_alloc_pdu.ndi     = 1;
    UL_alloc_pdu.TPC     = 0;
    UL_alloc_pdu.cshift  = 0;
    UL_alloc_pdu.dai     = 0;
    UL_alloc_pdu.cqi_req = 1;
    memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&UL_alloc_pdu,sizeof(DCI0_5MHz_TDD_1_6_t));

    //user 2
    DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI0_5MHz_TDD_1_6_t ; 
    DCI_pdu->dci_alloc[1].L          = 2;
    DCI_pdu->dci_alloc[1].rnti       = 0x1236;
    DCI_pdu->dci_alloc[1].format     = format0;

    UL_alloc_pdu.type    = 0;
    UL_alloc_pdu.hopping = 0;
    if (cooperation_flag==0)
      UL_alloc_pdu.rballoc = computeRIV(25,4+openair_daq_vars.ue_ul_nb_rb,openair_daq_vars.ue_ul_nb_rb);
    else 
      UL_alloc_pdu.rballoc = computeRIV(25,4,openair_daq_vars.ue_ul_nb_rb);
    UL_alloc_pdu.mcs     = openair_daq_vars.target_ue_ul_mcs;
    UL_alloc_pdu.ndi     = 1;
    UL_alloc_pdu.TPC     = 0;
    if ((cooperation_flag==0) || (cooperation_flag==1))
      UL_alloc_pdu.cshift  = 0;
    else
      UL_alloc_pdu.cshift  = 1;
    UL_alloc_pdu.dai     = 0;
    UL_alloc_pdu.cqi_req = 1;
    memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&UL_alloc_pdu,sizeof(DCI0_5MHz_TDD_1_6_t));
    break;

  default:
    break;
  }

}
void phy_procedures_eNB_TX(unsigned char next_slot,PHY_VARS_eNB *phy_vars_eNB,u8 abstraction_flag) {

  u8 *pbch_pdu=&phy_vars_eNB->pbch_pdu[0];
  //  unsigned int nb_dci_ue_spec = 0, nb_dci_common = 0;
  u16 input_buffer_length, re_allocated=0;
  u32 sect_id = 0,i,aa;
  u8 harq_pid;
  DCI_PDU *DCI_pdu;
  u8 *DLSCH_pdu=NULL;
#ifndef OPENAIR2
  DCI_PDU DCI_pdu_tmp;
  u8 DLSCH_pdu_tmp[768*8];
#endif
  s8 UE_id;
  u8 num_pdcch_symbols=0;
  s16 crnti;
  u16 frame_tx;
  s16 amp;
  u8 ul_subframe;
  u32 ul_frame;


  for (sect_id = 0 ; sect_id < number_of_cards; sect_id++) {

    if (abstraction_flag==0) {
      if (next_slot%2 == 0) {
	for (aa=0; aa<phy_vars_eNB->lte_frame_parms.nb_antennas_tx;aa++) {
	 
#ifdef IFFT_FPGA
	  memset(&phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id][aa][next_slot*(phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)],
		 0,(phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti)*sizeof(mod_sym_t));
#else
	  memset(&phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id][aa][next_slot*phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)],
		 0,phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*(phy_vars_eNB->lte_frame_parms.symbols_per_tti)*sizeof(mod_sym_t));
#endif
	}
      }
      generate_pilots_slot(phy_vars_eNB,
			   phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
			   AMP,
			   next_slot);
    
  

      if (next_slot == 0) {
	
	// First half of PSS/SSS (FDD)
	if (phy_vars_eNB->lte_frame_parms.frame_type == 0) {
	  generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       4*AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       (phy_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
		       next_slot);
	  generate_sss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       (phy_vars_eNB->lte_frame_parms.Ncp==0) ? 5 : 4,
		       next_slot);
	}
      }
    }      
    if (next_slot == 1) {
      
      if ((phy_vars_eNB->frame&3) == 0) {
	((u8*) pbch_pdu)[0] = 0;
	switch (phy_vars_eNB->lte_frame_parms.N_RB_DL) {
	case 6:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0x1f) | (0<<5);
	  break;
	case 15:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0x1f) | (1<<5);
	  break;
	case 25:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0x1f) | (2<<5);
	  break;
	case 50:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0x1f) | (3<<5);
	  break;
	case 100:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0x1f) | (4<<5);
	  break;
	default:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0x1f) | (2<<5);
	  break;
	}
	((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0xef) | 
	  ((phy_vars_eNB->lte_frame_parms.phich_config_common.phich_duration << 4)&0x10);
	
	switch (phy_vars_eNB->lte_frame_parms.phich_config_common.phich_resource) {
	case oneSixth:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0xf3) | (0<<2);
	  break;
	case half:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0xf3) | (1<<2);
	  break;
	case one:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0xf3) | (2<<2);
	  break;
	case two:
	  ((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0xf3) | (3<<2);
	  break;
	default:
	  break;
	}

	((u8*) pbch_pdu)[0] = (((u8*) pbch_pdu)[0]&0xfc) | ((phy_vars_eNB->frame>>8)&0x3);
	((u8*) pbch_pdu)[1] = phy_vars_eNB->frame&0xfc;
	((u8*) pbch_pdu)[2] = 0;
      }
      /// First half of SSS (TDD)
      if (abstraction_flag==0) {
	
	if (phy_vars_eNB->lte_frame_parms.frame_type == 1) {
	  generate_sss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       (phy_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
		       next_slot);
	}
      }
      

      
      frame_tx = (((int) (pbch_pdu[0]&0x3))<<8) + ((int) (pbch_pdu[1]&0xfc)) + phy_vars_eNB->frame%4;
   
#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d] Frame %d, slot %d: Calling generate_pbch, mode1_flag=%d, frame_tx=%d, pdu=%02x%02x%02x\n",
	    phy_vars_eNB->Mod_id,
	    phy_vars_eNB->frame, 
	    next_slot,
	    phy_vars_eNB->lte_frame_parms.mode1_flag,
	    frame_tx,
	    ((u8*) pbch_pdu)[0],
	    ((u8*) pbch_pdu)[1],
	    ((u8*) pbch_pdu)[2]);
#endif
      
      if (abstraction_flag==0) {

	generate_pbch(&phy_vars_eNB->lte_eNB_pbch,
		      phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		      AMP,
		      &phy_vars_eNB->lte_frame_parms,
		      pbch_pdu,
		      phy_vars_eNB->frame&3);

      }
#ifdef PHY_ABSTRACTION
      else {
#ifdef PHY_ABSTRACTION
	generate_pbch_emul(phy_vars_eNB,pbch_pdu); 
#endif
      }
#endif
    }


    if (next_slot == 2) {
	
      if (abstraction_flag==0) {
	
	if (phy_vars_eNB->lte_frame_parms.frame_type == 1) {
	  //	  printf("Generating PSS (frame %d, subframe %d)\n",phy_vars_eNB->frame,next_slot>>1);
	  generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       4*AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       2,
		       next_slot);
	}
      }
    } 

    // Second half of PSS/SSS (FDD)
    if (next_slot == 10) {
     
      if (abstraction_flag==0) {
       
	if (phy_vars_eNB->lte_frame_parms.frame_type == 0) {
	  generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       4*AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       (phy_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
		       next_slot);
	  generate_sss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       (phy_vars_eNB->lte_frame_parms.Ncp==0) ? 5 : 4,
		       next_slot);

	}
      }
    }
    //  Second-half of SSS (TDD)
    if (next_slot == 11) {
      if (abstraction_flag==0) {
       
	if (phy_vars_eNB->lte_frame_parms.frame_type == 1) {
	  generate_sss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       (phy_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
		       next_slot);
	}
      }
    }
    // Second half of PSS (TDD)
    if (next_slot == 12) {
     
      if (abstraction_flag==0) {
       
	if (phy_vars_eNB->lte_frame_parms.frame_type == 1) {
	  //	    printf("Generating PSS (frame %d, subframe %d)\n",phy_vars_eNB->frame,next_slot>>1);
	  generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       4*AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       2,
		       next_slot);
	}
      }
    }
  }

  //return;


  sect_id=0;

  if ((next_slot % 2)==0) {
    //#ifdef DEBUG_PHY_PROC
    //    msg("[PHY][eNB %d] UE %d: Mode %s\n",phy_vars_eNB->Mod_id,0,mode_string[phy_vars_eNB->eNB_UE_stats[0].mode]);
    //#endif

#ifdef OPENAIR2
      // if there are two users and we want to do cooperation
    if ((phy_vars_eNB->eNB_UE_stats[0].mode == PUSCH) && (phy_vars_eNB->eNB_UE_stats[1].mode == PUSCH))
      mac_xface->eNB_dlsch_ulsch_scheduler(phy_vars_eNB->Mod_id,phy_vars_eNB->cooperation_flag,phy_vars_eNB->frame,next_slot>>1);
    else
      mac_xface->eNB_dlsch_ulsch_scheduler(phy_vars_eNB->Mod_id,0,phy_vars_eNB->frame,next_slot>>1);

    // Parse DCI received from MAC
    DCI_pdu = mac_xface->get_dci_sdu(phy_vars_eNB->Mod_id,
				     phy_vars_eNB->frame,
				     next_slot>>1);
#else
    DCI_pdu = &DCI_pdu_tmp;
    fill_dci(DCI_pdu,next_slot>>1,phy_vars_eNB->cooperation_flag);
#endif

    // clear existing ulsch dci allocations before applying info from MAC  (this is table
    ul_subframe = pdcch_alloc2ul_subframe(&phy_vars_eNB->lte_frame_parms,next_slot>>1);
    ul_frame = pdcch_alloc2ul_frame(&phy_vars_eNB->lte_frame_parms,(((next_slot>>1)==0)?1:0)+phy_vars_eNB->frame,next_slot>>1);

    if ((subframe_select(&phy_vars_eNB->lte_frame_parms,ul_subframe)==SF_UL) ||
	(phy_vars_eNB->lte_frame_parms.frame_type == 0)) {
      harq_pid = subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,ul_frame,ul_subframe);
      for (i=0;i<NUMBER_OF_UE_MAX;i++)
	if (phy_vars_eNB->ulsch_eNB[i]) {
	  phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->dci_alloc=0;
	  phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->rar_alloc=0;
	}
    }
  
#ifdef EMOS
    emos_dump_eNB.dci_cnt[next_slot>>1] = DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci; //nb_dci_common+nb_dci_ue_spec;
#endif
    // clear previous allocation information for all UEs
    for (i=0;i<NUMBER_OF_UE_MAX;i++) {
      phy_vars_eNB->dlsch_eNB[i][0]->subframe_tx[next_slot>>1] = 0;
    }


    for (i=0;i<DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci ; i++) {
#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB] Subframe %d : Doing DCI index %d/%d\n",next_slot>>1,i,DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci);
      if (phy_vars_eNB->frame == 1)
	dump_dci(&phy_vars_eNB->lte_frame_parms,&DCI_pdu->dci_alloc[i]);
#endif

      if (DCI_pdu->dci_alloc[i].rnti == SI_RNTI) {
	//	LOG_D(PHY,"[eNB %d] SI generate_eNB_dlsch_params_from_dci\n", phy_vars_eNB->Mod_id);
	generate_eNB_dlsch_params_from_dci(next_slot>>1,
					   &DCI_pdu->dci_alloc[i].dci_pdu[0],
					   DCI_pdu->dci_alloc[i].rnti,
					   DCI_pdu->dci_alloc[i].format,
					   &phy_vars_eNB->dlsch_eNB_SI,
					   &phy_vars_eNB->lte_frame_parms,
					   SI_RNTI,
					   0,
					   P_RNTI,
					   phy_vars_eNB->eNB_UE_stats[0].DL_pmi_single);
      }
      else if (DCI_pdu->dci_alloc[i].ra_flag == 1) {
#ifdef DEBUG_PHY_PROC
	LOG_D(PHY,"[eNB %d] RA generate_eNB_dlsch_params_from_dci\n", phy_vars_eNB->Mod_id);
#endif
	generate_eNB_dlsch_params_from_dci(next_slot>>1,
					   &DCI_pdu->dci_alloc[i].dci_pdu[0],
					   DCI_pdu->dci_alloc[i].rnti,
					   DCI_pdu->dci_alloc[i].format,
					   &phy_vars_eNB->dlsch_eNB_ra,
					   &phy_vars_eNB->lte_frame_parms,
					   SI_RNTI,
					   DCI_pdu->dci_alloc[i].rnti,
					   P_RNTI,
					   phy_vars_eNB->eNB_UE_stats[0].DL_pmi_single);
      }
      else if (DCI_pdu->dci_alloc[i].format != format0){ // this is a normal DLSCH allocation

#ifdef OPENAIR2
#ifdef DEBUG_PHY_PROC
	LOG_D(PHY,"[eNB] Searching for RNTI %x\n",DCI_pdu->dci_alloc[i].rnti);
#endif
	UE_id = find_ue((s16)DCI_pdu->dci_alloc[i].rnti,phy_vars_eNB);
#else
	UE_id = i;
#endif
	if (UE_id>=0) {
	  //	  dump_dci(&phy_vars_eNB->lte_frame_parms,&DCI_pdu->dci_alloc[i]);
	  generate_eNB_dlsch_params_from_dci(next_slot>>1,
					     &DCI_pdu->dci_alloc[i].dci_pdu[0],
					     DCI_pdu->dci_alloc[i].rnti,
					     DCI_pdu->dci_alloc[i].format,
					     phy_vars_eNB->dlsch_eNB[(u8)UE_id],
					     &phy_vars_eNB->lte_frame_parms,
					     SI_RNTI,
					     0,
					     P_RNTI,
					     phy_vars_eNB->eNB_UE_stats[(u8)UE_id].DL_pmi_single);
#ifdef DEBUG_PHY_PROC      
	  LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d UE_id %d Generated DCI format %d, aggregation %d\n",
	      phy_vars_eNB->Mod_id, DCI_pdu->dci_alloc[i].rnti,
	      phy_vars_eNB->frame, next_slot>>1,UE_id,
	      DCI_pdu->dci_alloc[i].format,
	      DCI_pdu->dci_alloc[i].L);
#endif
	}
	else {
	 /* LOG_D(PHY,"[eNB %d][PDSCH] Frame %d : No UE_id with corresponding rnti %x, dropping DLSCH\n",
	      phy_vars_eNB->Mod_id,phy_vars_eNB->frame,(s16)DCI_pdu->dci_alloc[i].rnti);*/
	}
      }

    }

    // Apply physicalConfigDedicated if needed
    phy_config_dedicated_eNB_step2(phy_vars_eNB);

    for (i=0;i<DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci ; i++) {    
      if (DCI_pdu->dci_alloc[i].format == format0) {  // this is a ULSCH allocation

	harq_pid = subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,
				     pdcch_alloc2ul_frame(&phy_vars_eNB->lte_frame_parms,(((next_slot>>1)==0)?1:0)+phy_vars_eNB->frame,next_slot>>1),
				     pdcch_alloc2ul_subframe(&phy_vars_eNB->lte_frame_parms,next_slot>>1));
	if (harq_pid==255) {
	/*  LOG_E(PHY,"[eNB %d] Frame %d: Bad harq_pid for ULSCH allocation\n",phy_vars_eNB->Mod_id,phy_vars_eNB->frame);*/
#ifdef USER_MODE
	  exit(-1);
#else
	  exit_openair=1;
#endif
	}
#ifdef OPENAIR2
	UE_id = find_ue((s16)DCI_pdu->dci_alloc[i].rnti,phy_vars_eNB);
#else
	UE_id = i;
#endif
	if (UE_id<0) {
	//  LOG_E(PHY,"[eNB %d] Frame %d: Unknown UE_id for rnti %x\n",phy_vars_eNB->Mod_id,phy_vars_eNB->frame,(s16)DCI_pdu->dci_alloc[i].rnti);
#ifdef USER_MODE
	  exit(-1);
#else
	  exit_openair=1;
#endif
	}
#ifdef DEBUG_PHY_PROC
	LOG_D(PHY,"[eNB %d][PUSCH %d] Frame %d subframe %d Generated format0 DCI (rnti %x, dci %x) (DCI pos %d/%d), aggregation %d\n",
	    phy_vars_eNB->Mod_id, 
	    subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,
			      pdcch_alloc2ul_frame(&phy_vars_eNB->lte_frame_parms,(((next_slot>>1)==0)?1:0)+phy_vars_eNB->frame,next_slot>>1),
			      pdcch_alloc2ul_subframe(&phy_vars_eNB->lte_frame_parms,next_slot>>1)),
	    pdcch_alloc2ul_frame(&phy_vars_eNB->lte_frame_parms,(((next_slot>>1)==0)?1:0)+phy_vars_eNB->frame,next_slot>>1),
	    next_slot>>1,DCI_pdu->dci_alloc[i].rnti,
	    *(unsigned int *)&DCI_pdu->dci_alloc[i].dci_pdu[0],
	    i,DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci,
	    DCI_pdu->dci_alloc[i].L);
#endif
	
	generate_eNB_ulsch_params_from_dci(&DCI_pdu->dci_alloc[i].dci_pdu[0],
					   DCI_pdu->dci_alloc[i].rnti,
					   (next_slot>>1),
					   format0,
					   UE_id,
					   phy_vars_eNB,
					   SI_RNTI,
					   0,
					   P_RNTI,
					   0);  // do_srs
	
#ifdef DEBUG_PHY_PROC
	LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d Setting subframe_scheduling_flag for (ul subframe %d)\n",
	    phy_vars_eNB->Mod_id,harq_pid,
	    ((next_slot>>1)==0 ? 1 : 0) +phy_vars_eNB->frame,next_slot>>1,
	    pdcch_alloc2ul_subframe(&phy_vars_eNB->lte_frame_parms,next_slot>>1));
#endif
	phy_vars_eNB->ulsch_eNB[(u32)UE_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 1;
	
      }
    }

    




    // if we have DCI to generate do it now
    if ((DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci)>0) {

 
    }
    else {  // for emulation!!
      phy_vars_eNB->num_ue_spec_dci[(next_slot>>1)&1]=0;
      phy_vars_eNB->num_common_dci[(next_slot>>1)&1]=0;
    }
 
    if (abstraction_flag == 0) {
#ifdef DEBUG_PHY_PROC
      //LOG_D(PHY,"[eNB %d] Frame %d, subframe %d: Calling generate_dci_top\n",phy_vars_eNB->Mod_id,phy_vars_eNB->frame, next_slot>>1);
#endif
      for (sect_id=0;sect_id<number_of_cards;sect_id++) 
	num_pdcch_symbols = generate_dci_top(DCI_pdu->Num_ue_spec_dci,
					     DCI_pdu->Num_common_dci,
					     DCI_pdu->dci_alloc,
					     0,
					     AMP,
					     &phy_vars_eNB->lte_frame_parms,
					     phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
					     next_slot>>1);
    }
#ifdef PHY_ABSTRACTION
    else {
#ifdef PHY_ABSTRACTION
      LOG_D(PHY,"[eNB %d] Frame %d, subframe %d: Calling generate_dci_top_emul\n",phy_vars_eNB->Mod_id,phy_vars_eNB->frame, next_slot>>1);
      num_pdcch_symbols = generate_dci_top_emul(phy_vars_eNB,DCI_pdu->Num_ue_spec_dci,DCI_pdu->Num_common_dci,DCI_pdu->dci_alloc,next_slot>>1);
#endif
    }
#endif



#ifdef DEBUG_PHY_PROC
    //LOG_D(PHY,"[eNB %d] Frame %d, slot %d: num_pdcch_symbols=%d\n",phy_vars_eNB->Mod_id,phy_vars_eNB->frame, next_slot,num_pdcch_symbols);
#endif



    for (UE_id=0;UE_id<NUMBER_OF_UE_MAX;UE_id++) {
      if ((phy_vars_eNB->dlsch_eNB[(u8)UE_id][0])&&
	  (phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rnti>0)&&
	  (phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->active == 1)) {
	harq_pid = phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->current_harq_pid;
	input_buffer_length = phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->TBS/8;
      

#ifdef DEBUG_PHY_PROC
	LOG_D(PHY,"[eNB %d][PDSCH %x/%d] Frame %d, slot %d: Generating PDSCH/DLSCH with input size = %d, G %d, nb_rb %d, mcs %d, Ndi %d, rv %d \n",
	    phy_vars_eNB->Mod_id, phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rnti,harq_pid,
	    phy_vars_eNB->frame, next_slot, input_buffer_length,
	    get_G(&phy_vars_eNB->lte_frame_parms,
		  phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->nb_rb,
		  phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rb_alloc,
		  get_Qm(phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->mcs),
		  num_pdcch_symbols,next_slot>>1),
	    phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->nb_rb,
	    phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->mcs,
	    phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->Ndi,
	    phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->rvidx);
#endif

	phy_vars_eNB->eNB_UE_stats[(u8)UE_id].dlsch_sliding_cnt++;
	if (phy_vars_eNB->dlsch_eNB[(u32)UE_id][0]->harq_processes[harq_pid]->Ndi == 1) {

	  phy_vars_eNB->eNB_UE_stats[(u32)UE_id].dlsch_trials[0]++;
	  
#ifdef OPENAIR2
	  DLSCH_pdu = mac_xface->get_dlsch_sdu(phy_vars_eNB->Mod_id,
					       phy_vars_eNB->frame,
					       phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rnti,
					       0);
#else
	  DLSCH_pdu = DLSCH_pdu_tmp;
	  for (i=0;i<input_buffer_length;i++)
	    DLSCH_pdu[i] = (unsigned char)(taus()&0xff);
#endif      

#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_DLSCH
	  LOG_D(PHY,"eNB DLSCH SDU: \n");
	  for (i=0;i<phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->TBS>>3;i++)
	    LOG_D(PHY,"%x.",(u8)DLSCH_pdu[i]);
	  LOG_D(PHY,"\n");
#endif
#endif
	}
	else {
	  phy_vars_eNB->eNB_UE_stats[(u32)UE_id].dlsch_trials[0]++;	
#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_DLSCH  
	  LOG_D(PHY,"[eNB] This DLSCH is a retransmission\n");
#endif
#endif
	}
	if (abstraction_flag==0) {

	  // 36-212
	  dlsch_encoding(DLSCH_pdu,
			 &phy_vars_eNB->lte_frame_parms,
			 num_pdcch_symbols,
			 phy_vars_eNB->dlsch_eNB[(u8)UE_id][0],
			 next_slot>>1);
	  // 36-211
	  dlsch_scrambling(&phy_vars_eNB->lte_frame_parms,
			   num_pdcch_symbols,
			   phy_vars_eNB->dlsch_eNB[(u8)UE_id][0],
			   get_G(&phy_vars_eNB->lte_frame_parms,
				 phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->nb_rb,
				 phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rb_alloc,
				 get_Qm(phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->mcs),
				 num_pdcch_symbols,next_slot>>1),
			   0,
			   next_slot);      
	  for (sect_id=0;sect_id<number_of_cards;sect_id++) {
	    
	      if ((phy_vars_eNB->transmission_mode[(u8)UE_id] == 5) &&
		  (phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->dl_power_off == 0)) 
		amp = (s16)(((s32)AMP*(s32)ONE_OVER_SQRT2_Q15)>>15);
	      else
		amp = AMP;

	    re_allocated = dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
					    amp,
					    next_slot/2,
					    &phy_vars_eNB->lte_frame_parms,
					    num_pdcch_symbols,
					    phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]);
	  }
	}
#ifdef PHY_ABSTRACTION
	else {
#ifdef PHY_ABSTRACTION
	  dlsch_encoding_emul(phy_vars_eNB,
			      DLSCH_pdu,
			      phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]);
#endif
	}
#endif
	phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->active = 0;
	
	//mac_xface->macphy_exit("first dlsch transmitted\n");
      }

      else if ((phy_vars_eNB->dlsch_eNB[(u8)UE_id][0])&&
	       (phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rnti>0)&&
	       (phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->active == 0)) {
      
	// clear subframe TX flag since UE is not scheduled for PDSCH in this subframe (so that we don't look for PUCCH later)
	phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->subframe_tx[next_slot>>1]=0;
#ifdef DEBUG_PHY_PROC
	//LOG_D(PHY,"[eNB %d] DCI: Clearing subframe_tx for subframe %d, UE %d\n",phy_vars_eNB->Mod_id,next_slot>>1,UE_id);
#endif
      }
    } 
  
    if (phy_vars_eNB->dlsch_eNB_SI->active == 1) {
      input_buffer_length = phy_vars_eNB->dlsch_eNB_SI->harq_processes[0]->TBS/8;


#ifdef OPENAIR2
      DLSCH_pdu = mac_xface->get_dlsch_sdu(phy_vars_eNB->Mod_id,
					   phy_vars_eNB->frame,
					   SI_RNTI,
					   0);
#else
      DLSCH_pdu = DLSCH_pdu_tmp;
      for (i=0;i<input_buffer_length;i++)
	DLSCH_pdu[i] = (unsigned char)(taus()&0xff);
#endif      


      
#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_DLSCH
      LOG_D(PHY,"[eNB %d][SI] Frame %d, slot %d: Calling generate_dlsch (SI) with input size = %d, num_pdcch_symbols %d\n",
	  phy_vars_eNB->Mod_id,phy_vars_eNB->frame, next_slot, input_buffer_length,num_pdcch_symbols);
      for (i=0;i<input_buffer_length;i++)
	LOG_D(PHY,"%x.",i,DLSCH_pdu[i]);
      LOG_D(PHY,"\n");
#endif
#endif

      if (abstraction_flag == 0) {

	dlsch_encoding(DLSCH_pdu,
		       &phy_vars_eNB->lte_frame_parms,
		       num_pdcch_symbols,
		       phy_vars_eNB->dlsch_eNB_SI,
		       next_slot>>1);
	
	dlsch_scrambling(&phy_vars_eNB->lte_frame_parms,
			 num_pdcch_symbols,
			 phy_vars_eNB->dlsch_eNB_SI,
			 get_G(&phy_vars_eNB->lte_frame_parms,
			       phy_vars_eNB->dlsch_eNB_SI->nb_rb,
			       phy_vars_eNB->dlsch_eNB_SI->rb_alloc,
			       get_Qm(phy_vars_eNB->dlsch_eNB_SI->harq_processes[0]->mcs),
			       num_pdcch_symbols,next_slot>>1),
			 0,
			 next_slot);      
	
	for (sect_id=0;sect_id<number_of_cards;sect_id++) 
	  re_allocated = dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
					  AMP,
					  next_slot/2,
					  &phy_vars_eNB->lte_frame_parms,
					  num_pdcch_symbols,
					  phy_vars_eNB->dlsch_eNB_SI);
      } 
#ifdef PHY_ABSTRACTION
      else {
#ifdef PHY_ABSTRACTION
	dlsch_encoding_emul(phy_vars_eNB,
			    DLSCH_pdu,
			    phy_vars_eNB->dlsch_eNB_SI);
#endif
      }
#endif
      phy_vars_eNB->dlsch_eNB_SI->active = 0;
      
    }
  
    if (phy_vars_eNB->dlsch_eNB_ra->active == 1) {
#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d][RAPROC] Frame %d, slot %d, RA active, filling RAR:\n",
	  phy_vars_eNB->Mod_id,phy_vars_eNB->frame, next_slot);
#endif

      input_buffer_length = phy_vars_eNB->dlsch_eNB_ra->harq_processes[0]->TBS/8;

#ifdef OPENAIR2
      crnti = mac_xface->fill_rar(phy_vars_eNB->Mod_id,
				  phy_vars_eNB->frame,
				  dlsch_input_buffer,
				  phy_vars_eNB->lte_frame_parms.N_RB_UL,
				  input_buffer_length);
      /*
      for (i=0;i<input_buffer_length;i++)
	msg("%x.",dlsch_input_buffer[i]);
      msg("\n");
      */
      UE_id = add_ue(crnti,phy_vars_eNB);
      if (UE_id==-1) {
	mac_xface->macphy_exit("[PHY][eNB] Max user count reached.\n");
      }

      phy_vars_eNB->eNB_UE_stats[(u32)UE_id].mode = RA_RESPONSE;
      // Initialize indicator for first SR (to be cleared after ConnectionSetup is acknowledged)
      phy_vars_eNB->first_sr[(u32)UE_id] = 1;

      generate_eNB_ulsch_params_from_rar(dlsch_input_buffer,
					 phy_vars_eNB->frame,
					 (next_slot>>1),
					 phy_vars_eNB->ulsch_eNB[(u32)UE_id],
					 &phy_vars_eNB->lte_frame_parms);

      phy_vars_eNB->ulsch_eNB[(u32)UE_id]->Msg3_active = 1;

      get_Msg3_alloc(&phy_vars_eNB->lte_frame_parms,
		     next_slot>>1,
		     phy_vars_eNB->frame,
		     &phy_vars_eNB->ulsch_eNB[(u32)UE_id]->Msg3_frame,
		     &phy_vars_eNB->ulsch_eNB[(u32)UE_id]->Msg3_subframe);
#else
      for (i=0;i<input_buffer_length;i++)
      	dlsch_input_buffer[i]= (unsigned char)(taus()&0xff);
#endif

#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d][RAPROC] Frame %d, next slot %d: Calling generate_dlsch (RA) with input size = %d,Msg3 frame %d, Msg3 subframe %d\n",
	  phy_vars_eNB->Mod_id,
	  phy_vars_eNB->frame, next_slot,input_buffer_length, 
	  phy_vars_eNB->ulsch_eNB[(u32)UE_id]->Msg3_frame,
	  phy_vars_eNB->ulsch_eNB[(u32)UE_id]->Msg3_subframe);
#endif

      if (abstraction_flag == 0) {

	dlsch_encoding(dlsch_input_buffer,
		       &phy_vars_eNB->lte_frame_parms,
		       num_pdcch_symbols,
		       phy_vars_eNB->dlsch_eNB_ra,
		       next_slot>>1);

	//	phy_vars_eNB->dlsch_eNB_ra->rnti = RA_RNTI;
	dlsch_scrambling(&phy_vars_eNB->lte_frame_parms,
			 num_pdcch_symbols,
			 phy_vars_eNB->dlsch_eNB_ra,
			 get_G(&phy_vars_eNB->lte_frame_parms,
			       phy_vars_eNB->dlsch_eNB_ra->nb_rb,
			       phy_vars_eNB->dlsch_eNB_ra->rb_alloc,
			       get_Qm(phy_vars_eNB->dlsch_eNB_ra->harq_processes[0]->mcs),
			       num_pdcch_symbols,next_slot>>1),
			 0,
			 next_slot);
	for (sect_id=0;sect_id<number_of_cards;sect_id++) 
	  re_allocated = dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
					  AMP,
					  next_slot/2,
					  &phy_vars_eNB->lte_frame_parms,
					  num_pdcch_symbols,
					  phy_vars_eNB->dlsch_eNB_ra);
      }
#ifdef PHY_ABSTRACTION
      else {
#ifdef PHY_ABSTRACTION
	dlsch_encoding_emul(phy_vars_eNB,
			    dlsch_input_buffer,
			    phy_vars_eNB->dlsch_eNB_ra);
#endif
      }
#endif
      /*LOG_D(PHY,"[eNB %d][RAPROC] Frame %d subframe %d Deactivating DLSCH RA\n",phy_vars_eNB->Mod_id,
	  phy_vars_eNB->frame,next_slot>>1);*/
      phy_vars_eNB->dlsch_eNB_ra->active = 0;
	
#ifdef DEBUG_PHY_PROC    
      LOG_D(PHY,"[eNB %d] Frame %d, slot %d, DLSCH (RA) re_allocated = %d\n",phy_vars_eNB->Mod_id,
	  phy_vars_eNB->frame, next_slot, re_allocated);
#endif
    }

    // if we have PHICH to generate
    //    printf("[PHY][eNB] Frame %d subframe %d Checking for phich\n",phy_vars_eNB->frame,next_slot>>1); 
    if (is_phich_subframe(&phy_vars_eNB->lte_frame_parms,next_slot>>1)) {
#ifdef DEBUG_PHY_PROC
      //      LOG_D(PHY,"[eNB %d] Frame %d, subframe %d: Calling generate_phich_top\n",phy_vars_eNB->Mod_id,phy_vars_eNB->frame, next_slot>>1);
#endif
	for (sect_id=0;sect_id<number_of_cards;sect_id++) {
	  generate_phich_top(phy_vars_eNB,
			     next_slot>>1,
			     AMP,
			     sect_id,
			     abstraction_flag);
	}
    }
  }





#ifdef EMOS
  phy_procedures_emos_eNB_TX(next_slot);
#endif
}
void load_param_init(unsigned char N_tx, unsigned char N_rx,unsigned char transmission_mode,u8 extended_prefix_flag,u8 N_RB_DL,u8 frame_type,u8 tdd_config,u8 osf){
	LTE_DL_FRAME_PARMS *lte_frame_parms;

	printf("Start lte_param_init\n");
	PHY_vars_eNB = (PHY_VARS_eNB*)malloc(sizeof(PHY_VARS_eNB));
	memset(PHY_vars_eNB,0,sizeof(PHY_VARS_eNB));//..wanlin
	randominit(0);
	set_taus_seed(0);

	lte_frame_parms = &(PHY_vars_eNB->lte_frame_parms);

	lte_frame_parms->frame_type         = frame_type;
	lte_frame_parms->tdd_config         = tdd_config;
	lte_frame_parms->N_RB_DL            = N_RB_DL;   //50 for 10MHz and 25 for 5 MHz
	lte_frame_parms->N_RB_UL            = N_RB_DL;   
	lte_frame_parms->Ncp                = extended_prefix_flag;
	lte_frame_parms->Ncp_UL             = extended_prefix_flag;
	lte_frame_parms->Nid_cell           = 10;
	lte_frame_parms->nushift            = 0;
	lte_frame_parms->nb_antennas_tx     = N_tx;
	lte_frame_parms->nb_antennas_rx     = N_rx;
	//  lte_frame_parms->Csrs = 2;
	//  lte_frame_parms->Bsrs = 0;
	//  lte_frame_parms->kTC = 0;
	//  lte_frame_parms->n_RRC = 0;
	lte_frame_parms->mode1_flag = (transmission_mode == 1)? 1 : 0;
	lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = 0;//n_DMRS1 set to 0

	init_frame_parms(lte_frame_parms,osf);
	phy_init_top(lte_frame_parms); //allocation
	//
	lte_frame_parms->twiddle_fft      = twiddle_fft;
	lte_frame_parms->twiddle_ifft     = twiddle_ifft;
	lte_frame_parms->rev              = rev;

	phy_init_lte_top(lte_frame_parms);

	phy_init_lte_eNB(PHY_vars_eNB,0,0,0);

	printf("Done lte_param_init\n");
}
int main(){
	u8 extended_prefix_flag=0;
	u8 N_RB_DL=25,osf=1;
	u8 tdd_config=3,frame_type=0;
	load_param_init(1,1,1,extended_prefix_flag,N_RB_DL,frame_type,tdd_config,osf);  
	phy_procedures_eNB_TX(12,PHY_vars_eNB,0);
	system("PAUSE");
	return 0;
}

#endif
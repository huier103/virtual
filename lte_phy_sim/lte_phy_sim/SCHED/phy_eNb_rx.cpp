#ifdef PHY_eNb_RX
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/LTE_ESTIMATION/defs.h"
#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "SIMULATION/TOOLS/defs.h"
#include "PHY/vars.h"

#define BW 7.68
int number_of_cards=3;//..wanlin参照前面的内容，暂且设置等于3.此处的number_of_cards已经在init_top中定义。

static unsigned char I0_clear = 1;

PHY_VARS_eNB *PHY_vars_eNB;

// This function retrieves the harq_pid of the corresponding DLSCH process
// and updates the error statistics of the DLSCH based on the received ACK
// info from UE along with the round index.  It also performs the fine-grain
// rate-adaptation based on the error statistics derived from the ACK/NAK process



s32 remove_ue(u16 rnti, PHY_VARS_eNB *phy_vars_eNB, u8 abstraction_flag) {
	u8 i;

#ifdef DEBUG_PHY_PROC
	msg("[PHY] eNB %d removing UE with rnti %x\n",phy_vars_eNB->Mod_id,rnti);
#endif
	for (i=0;i<NUMBER_OF_UE_MAX;i++) {
		if ((phy_vars_eNB->dlsch_eNB[i]==NULL) || (phy_vars_eNB->ulsch_eNB[i]==NULL)) 
			msg("[PHY] Can't remove UE, not enough memory allocated\n");
		else {
			if (phy_vars_eNB->eNB_UE_stats[i].crnti==rnti) {
				//msg("[PHY] UE_id %d\n",i);
				clean_eNb_dlsch(phy_vars_eNB->dlsch_eNB[i][0], abstraction_flag);
				clean_eNb_ulsch(phy_vars_eNB->ulsch_eNB[i],abstraction_flag);
				phy_vars_eNB->eNB_UE_stats[i].crnti = 0;
				return(i);
			}
		}
	}

	return(-1);
}
void process_HARQ_feedback(u8 UE_id, 
	u8 subframe, 
	PHY_VARS_eNB *phy_vars_eNB,
	u8 pusch_flag, 
	u8 *pucch_payload, 
	u8 pucch_sel,
	u8 SR_payload) {

		u8 dl_harq_pid[8],dlsch_ACK[8],j,dl_subframe;
		LTE_eNB_DLSCH_t *dlsch             =  phy_vars_eNB->dlsch_eNB[(u32)UE_id][0];
		LTE_eNB_UE_stats *ue_stats         =  &phy_vars_eNB->eNB_UE_stats[(u32)UE_id];
		LTE_DL_eNB_HARQ_t *dlsch_harq_proc;
		u8 subframe_m4,M,m;

		if (phy_vars_eNB->lte_frame_parms.frame_type == 0){ //FDD
			subframe_m4 = (subframe<4) ? subframe+6 : subframe-4;

			dl_harq_pid[0] = dlsch->harq_ids[subframe_m4];
			M=1;
			if (pusch_flag == 1)
				dlsch_ACK[0] = phy_vars_eNB->ulsch_eNB[(u8)UE_id]->o_ACK[0];
			else
				dlsch_ACK[0] = pucch_payload[0];
			/*LOG_D(PHY,"[eNB %d] Frame %d: Received ACK/NAK %d for subframe %d\n",phy_vars_eNB->Mod_id,
			phy_vars_eNB->frame,dlsch_ACK[0],subframe_m4);*/
		}
		else {  // TDD Handle M=1,2 cases only

			M=ul_ACK_subframe2_M(&phy_vars_eNB->lte_frame_parms,
				subframe);
			// Now derive ACK information for TDD
			if (pusch_flag == 1) { // Do PUSCH ACK/NAK first
				// detect missing DAI
				//FK: this code is just a guess
				dlsch_ACK[0] = phy_vars_eNB->ulsch_eNB[(u8)UE_id]->o_ACK[0];
				dlsch_ACK[1] = phy_vars_eNB->ulsch_eNB[(u8)UE_id]->o_ACK[1];
			}

			else {
				if (pucch_sel == 2) {  // bundling
					dlsch_ACK[0] = pucch_payload[0];
					dlsch_ACK[1] = pucch_payload[0];
				}
				else {
					dlsch_ACK[0] = pucch_payload[0];
					dlsch_ACK[1] = pucch_payload[1];
				}
			}
		}

		for (m=0;m<M;m++) {

			dl_subframe = ul_ACK_subframe2_dl_subframe(&phy_vars_eNB->lte_frame_parms,
				subframe,
				m);

			if (dlsch->subframe_tx[dl_subframe]==1) {
				dl_harq_pid[m]     = dlsch->harq_ids[dl_subframe];

				if (dl_harq_pid[m]<dlsch->Mdlharq) {
					dlsch_harq_proc = dlsch->harq_processes[dl_harq_pid[m]];
#ifdef DEBUG_PHY_PROC	
					LOG_D(PHY,"[eNB %d][PDSCH %x/%d] status %d, round %d\n",phy_vars_eNB->Mod_id,
						dlsch->rnti,dl_harq_pid[m],
						dlsch_harq_proc->status,dlsch_harq_proc->round);
#endif
					if ((dl_harq_pid[m]<dlsch->Mdlharq) &&
						(dlsch_harq_proc->status == ACTIVE)) {
							// dl_harq_pid of DLSCH is still active

							//	  msg("[PHY] eNB %d Process %d is active (%d)\n",phy_vars_eNB->Mod_id,dl_harq_pid[m],dlsch_ACK[m]);
							if ( dlsch_ACK[m]== 0) {
								// Received NAK 

								//	    if (dlsch_harq_proc->round == 0)
								ue_stats->dlsch_NAK[dlsch_harq_proc->round]++;

								// then Increment DLSCH round index 
								dlsch_harq_proc->round++;

								if (dlsch_harq_proc->round == dlsch->Mdlharq) {
									// This was the last round for DLSCH so reset round and increment l2_error counter
									dlsch_harq_proc->round = 0;
									ue_stats->dlsch_l2_errors++;
									dlsch_harq_proc->status = SCH_IDLE;
									dlsch->harq_ids[dl_subframe] = dlsch->Mdlharq;
								}
							}
							else {
#ifdef DEBUG_PHY_PROC	
								LOG_D(PHY,"[eNB %d][PDSCH %x/%d] ACK Received in round %d, resetting process\n",phy_vars_eNB->Mod_id,
									dlsch->rnti,dl_harq_pid[m],dlsch_harq_proc->round);
#endif
								// Received ACK so set round to 0 and set dlsch_harq_pid IDLE
								dlsch_harq_proc->round  = 0;
								dlsch_harq_proc->status = SCH_IDLE; 
								dlsch->harq_ids[dl_subframe] = dlsch->Mdlharq;

								ue_stats->total_TBS = ue_stats->total_TBS + phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[dl_harq_pid[m]]->TBS;
								ue_stats->total_transmitted_bits = phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[dl_harq_pid[m]]->TBS + ue_stats->total_transmitted_bits;
							}

							// Do fine-grain rate-adaptation for DLSCH 
							if (ue_stats->dlsch_NAK[0] > dlsch->error_threshold) {
								if (ue_stats->dlsch_mcs_offset == 1)
									ue_stats->dlsch_mcs_offset=0;
								else
									ue_stats->dlsch_mcs_offset=-1;
							}
#ifdef DEBUG_PHY_PROC	  
							LOG_D(PHY,"[process_HARQ_feedback] Frame %d Setting round to %d for pid %d (subframe %d)\n",phy_vars_eNB->frame,
								dlsch_harq_proc->round,dl_harq_pid[m],subframe);
#endif

							// Clear NAK stats and adjust mcs offset
							// after measurement window timer expires
							if ((ue_stats->dlsch_sliding_cnt == dlsch->ra_window_size) ) {
								if ((ue_stats->dlsch_mcs_offset == 0) && (ue_stats->dlsch_NAK[0] < 2))
									ue_stats->dlsch_mcs_offset = 1;
								if ((ue_stats->dlsch_mcs_offset == 1) && (ue_stats->dlsch_NAK[0] > 2))
									ue_stats->dlsch_mcs_offset = 0;
								if ((ue_stats->dlsch_mcs_offset == 0) && (ue_stats->dlsch_NAK[0] > 2))
									ue_stats->dlsch_mcs_offset = -1;
								if ((ue_stats->dlsch_mcs_offset == -1) && (ue_stats->dlsch_NAK[0] < 2))
									ue_stats->dlsch_mcs_offset = 0;

								for (j=0;j<phy_vars_eNB->dlsch_eNB[j][0]->Mdlharq;j++)
									ue_stats->dlsch_NAK[j] = 0;
								ue_stats->dlsch_sliding_cnt = 0;
							}


					}
				}
			}
		}
}
void phy_procedures_eNB_RX(unsigned char last_slot,PHY_VARS_eNB *phy_vars_eNB,u8 abstraction_flag) {
	//RX processing
	u32 l, ret,i,j;
	u32 sect_id=0;
	u32 harq_pid, round;
	u8 SR_payload,*pucch_payload=NULL,pucch_payload0[2],pucch_payload1[2];
	s16 n1_pucch0,n1_pucch1,n1_pucch2,n1_pucch3;
	u8 do_SR=0,pucch_sel;
	s16 metric0,metric1;
	ANFBmode_t bundling_flag;
	PUCCH_FMT_t format;
	u8 nPRS;
	u8 two_ues_connected = 0;
	u8 pusch_active = 0;


	if (abstraction_flag == 0) {
		remove_7_5_kHz(phy_vars_eNB,last_slot);
	}
#ifdef OPENAIR2
	// check if we have to detect PRACH first
	if ((last_slot&1)==1){
		//    printf("Checking PRACH for eNB %d, subframe %d\n",phy_vars_eNB->Mod_id,last_slot>>1);
		if (is_prach_subframe(&phy_vars_eNB->lte_frame_parms,phy_vars_eNB->frame,last_slot>>1)>0) {
			//      printf("Running prach procedures\n");
			prach_procedures(phy_vars_eNB,last_slot>>1,abstraction_flag);
		}
	}
#endif
	if (abstraction_flag == 0) {
		for (l=0;l<phy_vars_eNB->lte_frame_parms.symbols_per_tti/2;l++) {

			for (sect_id=0;sect_id<number_of_cards;sect_id++) {
				slot_fep_ul(&phy_vars_eNB->lte_frame_parms,
					&phy_vars_eNB->lte_eNB_common_vars,
					l,
					last_slot,
					sect_id,
#ifdef HW_PREFIX_REMOVAL
					1
#else
					0
#endif
					);
			}
		}
	}
	sect_id = 0;

	/*
	for (UE_id=0;UE_id<NUMBER_OF_UE_MAX;UE_id++) {

	if ((phy_vars_eNB->eNB_UE_stats[(u32)UE_id].mode>PRACH) && (last_slot%2==1)) {
	#ifdef DEBUG_PHY_PROC	
	LOG_D(PHY,"[eNB %d] frame %d, slot %d: Doing SRS estimation and measurements for UE_id %d (UE_mode %d)\n",
	phy_vars_eNB->Mod_id,
	phy_vars_eNB->frame, last_slot, 
	UE_id,phy_vars_eNB->eNB_UE_stats[(u32)UE_id].mode);
	#endif
	for (sect_id=0;sect_id<number_of_cards;sect_id++) {

	lte_srs_channel_estimation(&phy_vars_eNB->lte_frame_parms,
	&phy_vars_eNB->lte_eNB_common_vars,
	&phy_vars_eNB->lte_eNB_srs_vars[(u32)UE_id],
	&phy_vars_eNB->soundingrs_ul_config_dedicated[(u32)UE_id],
	last_slot>>1,
	sect_id);
	lte_eNB_srs_measurements(phy_vars_eNB,
	sect_id,
	UE_id,
	1);
	#ifdef DEBUG_PHY_PROC
	LOG_D(PHY,"[eNB %d] frame %d, slot %d: UE_id %d, sect_id %d: RX RSSI %d (from SRS)\n",
	phy_vars_eNB->Mod_id,
	phy_vars_eNB->frame, last_slot, 
	UE_id,sect_id,
	phy_vars_eNB->PHY_measurements_eNB[sect_id].rx_rssi_dBm[(u32)UE_id]);
	#endif
	}

	sect_id=0;
	#ifdef USER_MODE
	//write_output("srs_est0.m","srsest0",phy_vars_eNB->lte_eNB_common_vars.srs_ch_estimates[0][0],512,1,1);
	//write_output("srs_est1.m","srsest1",phy_vars_eNB->lte_eNB_common_vars.srs_ch_estimates[0][1],512,1,1);
	#endif

	//msg("timing advance in\n");
	sync_pos = lte_est_timing_advance(&phy_vars_eNB->lte_frame_parms,
	&phy_vars_eNB->lte_eNB_srs_vars[(u32)UE_id],
	&sect_id,
	phy_vars_eNB->first_run_timing_advance[(u32)UE_id],
	number_of_cards,
	24576);

	//msg("timing advance out\n");

	//phy_vars_eNB->eNB_UE_stats[(u32)UE_id].UE_timing_offset = sync_pos - phy_vars_eNB->lte_frame_parms.nb_prefix_samples/8;
	phy_vars_eNB->eNB_UE_stats[(u32)UE_id].UE_timing_offset = 0;
	phy_vars_eNB->eNB_UE_stats[(u32)UE_id].sector = sect_id;
	#ifdef DEBUG_PHY_PROC	
	LOG_D(PHY,"[eNB %d] frame %d, slot %d: user %d in sector %d: timing_advance = %d\n",
	phy_vars_eNB->Mod_id,
	phy_vars_eNB->frame, last_slot, 
	UE_id, sect_id,
	phy_vars_eNB->eNB_UE_stats[(u32)UE_id].UE_timing_offset);
	#endif
	}
	}
	else {

	}
	*/
	// Check for active processes in current subframe
	harq_pid = subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,
		((last_slot>>1)==9 ? -1 : 0 )+ phy_vars_eNB->frame,last_slot>>1);
	//  printf("[eNB][PUSCH] subframe %d => harq_pid %d\n",last_slot>>1,harq_pid);

#ifdef OPENAIR2
	if ((phy_vars_eNB->eNB_UE_stats[0].mode == PUSCH) && 
		(phy_vars_eNB->eNB_UE_stats[1].mode == PUSCH))
		two_ues_connected = 1;
#else
	two_ues_connected = 1;
#endif

	pusch_active = 0;
	for (i=0;i<NUMBER_OF_UE_MAX;i++) { 

		/*
		if ((i == 1) && (phy_vars_eNB->cooperation_flag > 0) && (two_ues_connected == 1))
		break;
		*/
#ifdef OPENAIR2
		if (phy_vars_eNB->eNB_UE_stats[i].mode == RA_RESPONSE)
			process_Msg3(phy_vars_eNB,last_slot,i,harq_pid);
#endif

		/*
		#ifdef DEBUG_PHY_PROC
		if (phy_vars_eNB->ulsch_eNB[i]) {
		printf("[PHY][eNB %d][PUSCH %d] frame %d, subframe %d rnti %x, alloc %d\n",phy_vars_eNB->Mod_id,
		harq_pid,phy_vars_eNB->frame,last_slot>>1,
		(phy_vars_eNB->ulsch_eNB[i]->rnti),
		(phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->subframe_scheduling_flag) 
		);
		}
		#endif
		*/
		if ((phy_vars_eNB->ulsch_eNB[i]) &&
			(phy_vars_eNB->ulsch_eNB[i]->rnti>0) &&
			(phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->subframe_scheduling_flag==1) && 
			((last_slot%2)==1)) {
				pusch_active = 1;
				round = phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round;

#ifdef DEBUG_PHY_PROC
				LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d Scheduling PUSCH/ULSCH Reception for rnti %x\n",
					phy_vars_eNB->Mod_id,harq_pid,
					(((last_slot>>1)==9) ? -1 : 0)+phy_vars_eNB->frame,last_slot>>1,phy_vars_eNB->ulsch_eNB[i]->rnti);
#endif

#ifdef DEBUG_PHY_PROC
				if (phy_vars_eNB->ulsch_eNB[i]->Msg3_flag == 1){
					LOG_D(PHY,"[eNB %d] frame %d, slot %d, subframe %d: Scheduling ULSCH Reception for Msg3 in Sector %d\n",
						phy_vars_eNB->Mod_id,
						(((last_slot>>1)==9)?-1:0)+phy_vars_eNB->frame,
						last_slot,last_slot>>1,
						phy_vars_eNB->eNB_UE_stats[i].sector);
				} else {
					LOG_F(PHY,"[eNB %d] frame %d, slot %d, subframe %d: Scheduling ULSCH Reception for UE %d Mode %s sect_id %d\n",
						phy_vars_eNB->Mod_id,
						(((last_slot>>1)==9)?-1:0)+phy_vars_eNB->frame,
						last_slot,last_slot>>1,
						i,
						mode_string[phy_vars_eNB->eNB_UE_stats[i].mode],
						phy_vars_eNB->eNB_UE_stats[i].sector);
				}
#endif

				nPRS = 0;

				phy_vars_eNB->ulsch_eNB[i]->cyclicShift = (phy_vars_eNB->ulsch_eNB[i]->n_DMRS2 + phy_vars_eNB->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift + nPRS)%12;

#ifdef DEBUG_PHY_PROC
				LOG_D(PHY,"[eNB %d][PUSCH %d] Frame %d Subframe %d Demodulating PUSCH: dci_alloc %d, rar_alloc %d, round %d, Ndi %d, first_rb %d, nb_rb %d, cyclic_shift %d (n_DMRS2 %d, cyclicShift %d, nprs %d) \n",
					phy_vars_eNB->Mod_id,harq_pid,(((last_slot>>1)==9)?-1:0)+phy_vars_eNB->frame,last_slot>>1,
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->dci_alloc,
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->rar_alloc,
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round,
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->Ndi,
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->first_rb,
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->nb_rb,
					phy_vars_eNB->ulsch_eNB[i]->cyclicShift,
					phy_vars_eNB->ulsch_eNB[i]->n_DMRS2,
					phy_vars_eNB->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift,
					nPRS);
#endif
				if (abstraction_flag==0) {
					rx_ulsch(phy_vars_eNB,
						last_slot>>1,
						phy_vars_eNB->eNB_UE_stats[i].sector,  // this is the effective sector id
						i,
						phy_vars_eNB->ulsch_eNB,
						0);
				}
#ifdef PHY_ABSTRACTION
				else {
					rx_ulsch_emul(phy_vars_eNB,
						last_slot>>1,
						phy_vars_eNB->eNB_UE_stats[i].sector,  // this is the effective sector id
						i);
				}
#endif

				for (j=0;j<phy_vars_eNB->lte_frame_parms.nb_antennas_rx;j++)
					phy_vars_eNB->eNB_UE_stats[i].UL_rssi[j] = dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[j]) - phy_vars_eNB->rx_total_gain_eNB_dB;

				if (abstraction_flag == 0) {
					ret = ulsch_decoding(phy_vars_eNB,
						i,
						last_slot>>1,
						0, // control_only_flag
						1  //Nbundled
						);  
				}
#ifdef PHY_ABSTRACTION
				else {
					ret = ulsch_decoding_emul(phy_vars_eNB,
						last_slot>>1,
						i);
				}
#endif

#ifdef DEBUG_PHY_PROC
				LOG_D(PHY,"[eNB %d][PUSCH %d][RAPROC] frame %d subframe %d RX power (%d,%d) N0 (%d,%d) dB ACK (%d,%d), decoding iter %d\n",
					phy_vars_eNB->Mod_id,harq_pid,
					phy_vars_eNB->frame,last_slot>>1,
					dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[0]),
					dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[1]),
					phy_vars_eNB->PHY_measurements_eNB->n0_power_dB[0],
					phy_vars_eNB->PHY_measurements_eNB->n0_power_dB[1],
					phy_vars_eNB->ulsch_eNB[i]->o_ACK[0],
					phy_vars_eNB->ulsch_eNB[i]->o_ACK[1],
					ret);
#endif //DEBUG_PHY_PROC
				/*
				if ((two_ues_connected==1) && (phy_vars_eNB->cooperation_flag==2)) {
				for (j=0;j<phy_vars_eNB->lte_frame_parms.nb_antennas_rx;j++) {
				phy_vars_eNB->eNB_UE_stats[i].UL_rssi[j] = dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power_0[j]) 
				- phy_vars_eNB->rx_total_gain_eNB_dB;
				phy_vars_eNB->eNB_UE_stats[i+1].UL_rssi[j] = dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power_1[j]) 
				- phy_vars_eNB->rx_total_gain_eNB_dB;
				}
				#ifdef DEBUG_PHY_PROC
				LOG_D(PHY,"[eNB %d] Frame %d subframe %d: ULSCH %d RX power UE0 (%d,%d) dB RX power UE1 (%d,%d)\n",
				phy_vars_eNB->Mod_id,phy_vars_eNB->frame,last_slot>>1,i,
				dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power_0[0]),
				dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power_0[1]),
				dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power_1[0]),
				dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power_1[1]));
				#endif
				}
				else {
				*/
				for (j=0;j<phy_vars_eNB->lte_frame_parms.nb_antennas_rx;j++)
					phy_vars_eNB->eNB_UE_stats[i].UL_rssi[j] = dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[j]) 
					- phy_vars_eNB->rx_total_gain_eNB_dB;
#ifdef DEBUG_PHY_PROC
				LOG_D(PHY,"[eNB %d] Frame %d subframe %d: ULSCH %d RX power (%d,%d) dB\n",
					phy_vars_eNB->Mod_id,phy_vars_eNB->frame,last_slot>>1,i,
					dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[0]),
					dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[1]));
#endif

				//      }


				phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_pid][phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round]++;
#ifdef DEBUG_PHY_PROC
				LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d Clearing subframe_scheduling_flag\n",
					phy_vars_eNB->Mod_id,harq_pid,phy_vars_eNB->frame,last_slot>>1);
#endif
				phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->subframe_scheduling_flag=0;


				if (phy_vars_eNB->ulsch_eNB[i]->cqi_crc_status == 1) {
#ifdef DEBUG_PHY_PROC
					if (((phy_vars_eNB->frame%10) == 0) || (phy_vars_eNB->frame < 50)) 
						print_CQI(phy_vars_eNB->ulsch_eNB[i]->o,phy_vars_eNB->ulsch_eNB[i]->uci_format,0);
#endif
					extract_CQI(phy_vars_eNB->ulsch_eNB[i]->o,phy_vars_eNB->ulsch_eNB[i]->uci_format,&phy_vars_eNB->eNB_UE_stats[i]);
					phy_vars_eNB->eNB_UE_stats[i].rank = phy_vars_eNB->ulsch_eNB[i]->o_RI[0];
				}

				if (ret == (1+MAX_TURBO_ITERATIONS)) {
					phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors[harq_pid][phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round]++;
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_active = 1;
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_ACK = 0;
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round++;
					//	printf("[UE][PUSCH %d] Increasing to round %d\n",harq_pid,phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round);

					phy_vars_eNB->ulsch_eNB[i]->o_ACK[0] = 0;
					phy_vars_eNB->ulsch_eNB[i]->o_ACK[1] = 0;




					if (phy_vars_eNB->ulsch_eNB[i]->Msg3_flag == 1) {
						/* LOG_D(PHY,"[eNB %d][RAPROC] frame %d, slot %d, subframe %d, UE %d: Error receiving ULSCH (Msg3), round %d/%d\n",
						phy_vars_eNB->Mod_id,
						phy_vars_eNB->frame,last_slot,last_slot>>1, i,
						phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round-1,
						phy_vars_eNB->lte_frame_parms.maxHARQ_Msg3Tx-1);*/

						if (phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round == 
							phy_vars_eNB->lte_frame_parms.maxHARQ_Msg3Tx) {
								/*LOG_D(PHY,"[eNB %d][RAPROC] maxHARQ_Msg3Tx reached, abandoning RA procedure for UE\n",
								phy_vars_eNB->Mod_id);
								phy_vars_eNB->eNB_UE_stats[i].mode = PRACH;
								remove_ue(phy_vars_eNB->eNB_UE_stats[i].crnti,phy_vars_eNB,abstraction_flag);*/
#ifdef OPENAIR2
								mac_xface->cancel_ra_proc(phy_vars_eNB->Mod_id,
									phy_vars_eNB->frame,
									0);
#endif
								phy_vars_eNB->ulsch_eNB[(u32)i]->Msg3_active = 0;
								phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_active = 0;

								/*
								#ifdef USER_MODE
								if (abstraction_flag == 0)
								dump_ulsch(phy_vars_eNB);
								exit(-1);
								#endif
								*/
						}
						else {
							// activate retransmission for Msg3 (signalled to UE PHY by PHICH (not MAC/DCI)
							phy_vars_eNB->ulsch_eNB[(u32)i]->Msg3_active = 1;

							get_Msg3_alloc_ret(&phy_vars_eNB->lte_frame_parms,
								last_slot>>1,
								phy_vars_eNB->frame,
								&phy_vars_eNB->ulsch_eNB[i]->Msg3_frame,
								&phy_vars_eNB->ulsch_eNB[i]->Msg3_subframe);
						}
					} // This is Msg3 error
					else {
						/*LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d Error receiving ULSCH, round %d/%d\n",
						phy_vars_eNB->Mod_id,harq_pid,
						phy_vars_eNB->frame,last_slot>>1, i,
						phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round-1,
						phy_vars_eNB->ulsch_eNB[i]->Mdlharq);*/

						if (phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round==
							phy_vars_eNB->ulsch_eNB[i]->Mdlharq) {
								/*  LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d Mdlharq %d reached\n",
								phy_vars_eNB->Mod_id,harq_pid,
								phy_vars_eNB->frame,last_slot>>1, i,
								phy_vars_eNB->ulsch_eNB[i]->Mdlharq);*/

								phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round=0;
								phy_vars_eNB->eNB_UE_stats[i].ulsch_errors[harq_pid]++;
								phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors[harq_pid]++;
						}
					}
					// If we've dropped the UE, go back to PRACH mode for this UE
					if (phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors[harq_pid] == 20) {
						/*  LOG_D(PHY,"[eNB %d] frame %d, subframe %d, UE %d: ULSCH consecutive error count reached, removing UE\n",
						phy_vars_eNB->Mod_id,phy_vars_eNB->frame,last_slot>>1, i);*/
						phy_vars_eNB->eNB_UE_stats[i].mode = PRACH;
						remove_ue(phy_vars_eNB->eNB_UE_stats[i].crnti,phy_vars_eNB,abstraction_flag);
#ifdef OPENAIR2
						mac_xface->cancel_ra_proc(phy_vars_eNB->Mod_id,
							phy_vars_eNB->frame,
							0);
#endif
						phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors[harq_pid]=0;
					}
				}  // ulsch in error
				else {
					/*LOG_D(PHY,"[eNB %d][PUSCH %d] Frame %d subframe %d ULSCH received, setting round to 0, PHICH ACK\n",
					phy_vars_eNB->Mod_id,harq_pid,
					phy_vars_eNB->frame,last_slot>>1);*/	    

					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_active = 1;
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_ACK = 1;
					phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round = 0;
					phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors[harq_pid] = 0;

					if (phy_vars_eNB->ulsch_eNB[i]->Msg3_flag == 1) {
#ifdef OPENAIR2
#ifdef DEBUG_PHY_PROC
						LOG_D(PHY,"[eNB %d][RAPROC] Frame %d Terminating ra_proc for harq %d, UE %d\n",phy_vars_eNB->Mod_id,
							phy_vars_eNB->frame,harq_pid,i);
#endif
						mac_xface->terminate_ra_proc(phy_vars_eNB->Mod_id,
							phy_vars_eNB->frame,
							phy_vars_eNB->ulsch_eNB[i]->rnti,
							phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->b);
#endif

						phy_vars_eNB->eNB_UE_stats[i].mode = PUSCH;
						phy_vars_eNB->ulsch_eNB[i]->Msg3_flag = 0;

#ifdef DEBUG_PHY_PROC
						LOG_D(PHY,"[eNB %d][RAPROC] Frame %d : RX Subframe %d Setting UE %d mode to PUSCH\n",phy_vars_eNB->Mod_id,phy_vars_eNB->frame,last_slot>>1,i);
#endif //DEBUG_PHY_PROC

						for (j=0;j<phy_vars_eNB->dlsch_eNB[i][0]->Mdlharq;j++) {
							phy_vars_eNB->eNB_UE_stats[i].dlsch_NAK[j]=0;
							phy_vars_eNB->eNB_UE_stats[i].dlsch_sliding_cnt=0;
						}

						//mac_xface->macphy_exit("Mode PUSCH. Exiting.\n");
					}
					else {
#ifdef DEBUG_PHY_PROC
						LOG_D(PHY,"[eNB] Frame %d, Subframe %d : ULSCH SDU (RX) %d bytes:",phy_vars_eNB->frame,last_slot>>1,phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->TBS>>3);
						for (j=0;j<phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->TBS>>3;j++)
							LOG_D(PHY,"%x.",phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->b[j]);
						LOG_D(PHY,"\n");
#endif

#ifdef OPENAIR2
						mac_xface->rx_sdu(phy_vars_eNB->Mod_id,
							phy_vars_eNB->frame,
							phy_vars_eNB->ulsch_eNB[i]->rnti,
							phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->b);
#endif
					}
				}  // ulsch not in error

				// process HARQ feedback
#ifdef DEBUG_PHY_PROC
				LOG_D(PHY,"[eNB %d] Processing HARQ feedback for UE %d\n",phy_vars_eNB->Mod_id,i);
#endif
				process_HARQ_feedback(i,
					last_slot>>1,
					phy_vars_eNB,
					1, // pusch_flag
					0,
					0,
					0);


#ifdef DEBUG_PHY_PROC
				LOG_D(PHY,"[eNB %d] Frame %d subframe %d, sect %d: received ULSCH harq_pid %d for UE %d, ret = %d, CQI CRC Status %d, ACK %d,%d, ulsch_errors %d/%d\n", 
					phy_vars_eNB->Mod_id,
					phy_vars_eNB->frame, last_slot>>1, 
					phy_vars_eNB->eNB_UE_stats[i].sector, 
					harq_pid, 
					i, 
					ret, 
					phy_vars_eNB->ulsch_eNB[i]->cqi_crc_status, 
					phy_vars_eNB->ulsch_eNB[i]->o_ACK[0],	
					phy_vars_eNB->ulsch_eNB[i]->o_ACK[1], 
					phy_vars_eNB->eNB_UE_stats[i].ulsch_errors[harq_pid],
					phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_pid][0]);
#endif


				if (phy_vars_eNB->frame % 100 == 0) {
					if ((phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_pid][round] - 
						phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts_last[harq_pid][round]) != 0)
						phy_vars_eNB->eNB_UE_stats[i].ulsch_round_fer[harq_pid][round] = 
						(100*(phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors[harq_pid][round] - 
						phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors_last[harq_pid][round]))/
						(phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_pid][round] - 
						phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts_last[harq_pid][round]);

					//phy_vars_eNB->eNB_UE_stats[i].dlsch_bitrate = phy_vars_eNB->eNB_UE_stats[i].total_TBS - phy_vars_eNB->eNB_UE_stats[i].total_TBS_last;

					//phy_vars_eNB->eNB_UE_stats[i].total_TBS_last = phy_vars_eNB->eNB_UE_stats[i].total_TBS;
					phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts_last[harq_pid][round] = 
						phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_pid][round];
					phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors_last[harq_pid][round] = 
						phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors[harq_pid][round];
				}

				if(phy_vars_eNB->frame % 10 == 0) {
					phy_vars_eNB->eNB_UE_stats[i].dlsch_bitrate = (phy_vars_eNB->eNB_UE_stats[i].total_TBS - 
						phy_vars_eNB->eNB_UE_stats[i].total_TBS_last)*10;

					phy_vars_eNB->eNB_UE_stats[i].total_TBS_last = phy_vars_eNB->eNB_UE_stats[i].total_TBS;
				}
		}


#ifdef PUCCH
		else if ((phy_vars_eNB->dlsch_eNB[i][0]) &&
			(phy_vars_eNB->dlsch_eNB[i][0]->rnti>0) &&
			((last_slot%2)==1)){ // check for PUCCH

				// check SR availability
				do_SR = is_SR_subframe(phy_vars_eNB,i,last_slot>>1);
				//      do_SR = 0;

				// Now ACK/NAK
				// First check subframe_tx flag for earlier subframes
				get_n1_pucch_eNB(phy_vars_eNB,
					i,
					last_slot>>1,
					&n1_pucch0,
					&n1_pucch1,
					&n1_pucch2,
					&n1_pucch3);

				if ((n1_pucch0==-1) && (n1_pucch1==-1) && (do_SR==0)) {  // no TX PDSCH that have to be checked and no SR for this UE_id
				}
				else {
					// otherwise we have some PUCCH detection to do

					if (do_SR == 1) {

						if (abstraction_flag == 0)
							metric0 = rx_pucch(phy_vars_eNB,
							pucch_format1,
							i,
							phy_vars_eNB->scheduling_request_config[i].sr_PUCCH_ResourceIndex,
							0, // n2_pucch
							1, // shortened format
							&SR_payload,
							last_slot>>1,
							PUCCH1_THRES);
						else {
#ifdef PHY_ABSTRACTION

							metric0 = rx_pucch_emul(phy_vars_eNB,
								i,
								pucch_format1,
								&SR_payload,
								last_slot>>1);
							LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d Checking SR (UE SR %d)\n",phy_vars_eNB->Mod_id,
								phy_vars_eNB->ulsch_eNB[i]->rnti,phy_vars_eNB->frame,last_slot>>1,SR_payload);

#endif
						}
						if (SR_payload == 1) {
							LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d Got SR for PUSCH, transmitting to MAC\n",phy_vars_eNB->Mod_id,
								phy_vars_eNB->ulsch_eNB[i]->rnti,phy_vars_eNB->frame,last_slot>>1);
							if (phy_vars_eNB->first_sr[i] == 1) { // this is the first request for uplink after Connection Setup, so clear HARQ process 0 use for Msg4
								phy_vars_eNB->dlsch_eNB[i][0]->harq_processes[0]->round=0;
								phy_vars_eNB->dlsch_eNB[i][0]->harq_processes[0]->status=SCH_IDLE;
								LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d First SR\n",
									phy_vars_eNB->Mod_id,
									phy_vars_eNB->ulsch_eNB[i]->rnti,phy_vars_eNB->frame,last_slot>>1);
							}
							mac_xface->SR_indication(phy_vars_eNB->Mod_id,
								phy_vars_eNB->frame,
								phy_vars_eNB->dlsch_eNB[i][0]->rnti,last_slot>>1);

						} 
					}// do_SR==1
					if ((n1_pucch0==-1) && (n1_pucch1==-1)) { // just check for SR

					}
					else if (phy_vars_eNB->lte_frame_parms.frame_type==0) { // FDD
						// if SR was detected, use the n1_pucch from SR, else use n1_pucch0
						n1_pucch0 = (SR_payload==1) ? phy_vars_eNB->scheduling_request_config[i].sr_PUCCH_ResourceIndex:n1_pucch0;
						if (abstraction_flag == 0)
							metric0 = rx_pucch(phy_vars_eNB,
							pucch_format1a,
							i,
							(u16)n1_pucch0,
							0, //n2_pucch
							1, // shortened format
							pucch_payload0,
							last_slot>>1,
							PUCCH1_THRES);
						else {
#ifdef PHY_ABSTRACTION
							metric0 = rx_pucch_emul(phy_vars_eNB,i,
								pucch_format1a,
								pucch_payload0,
								last_slot>>1);
#endif
						}	
					} // FDD
					else {  //TDD

						bundling_flag = phy_vars_eNB->pucch_config_dedicated[i].tdd_AckNackFeedbackMode;

						// fix later for 2 TB case and format1b
						if (bundling_flag==bundling) {
							format = pucch_format1a;
							//	  msg("PUCCH 1a\n");
						}
						else {
							format = pucch_format1b;
							//	  msg("PUCCH 1b\n");
						}

						// if SR was detected, use the n1_pucch from SR, else use n1_pucch0
						if (SR_payload==1) {
							if (abstraction_flag == 0) 
								metric0 = rx_pucch(phy_vars_eNB,
								format,
								i,
								phy_vars_eNB->scheduling_request_config[i].sr_PUCCH_ResourceIndex,
								0, //n2_pucch
								1, // shortened format
								pucch_payload0,
								last_slot>>1,
								PUCCH1_THRES);
							else {
#ifdef PHY_ABSTRACTION
								metric0 = rx_pucch_emul(phy_vars_eNB,i,
									format,
									pucch_payload0,
									last_slot>>1);
#endif
							} 
						}
						else {  //using n1_pucch0/n1_pucch1 resources
#ifdef DEBUG_PHY_PROC	  
							LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d Checking ACK/NAK (%d,%d,%d,%d)\n",phy_vars_eNB->Mod_id,
								phy_vars_eNB->dlsch_eNB[i][0]->rnti,
								phy_vars_eNB->frame,last_slot>>1,
								n1_pucch0,n1_pucch1,n1_pucch2,n1_pucch3);
#endif
							metric0=0;
							metric1=0;

							// Check n1_pucch0 metric
							if (n1_pucch0 != -1) {
								if (abstraction_flag == 0) 
									metric0 = rx_pucch(phy_vars_eNB,
									format,
									i,
									(u16)n1_pucch0,
									0, // n2_pucch
									1, // shortened format
									pucch_payload0,
									last_slot>>1,
									PUCCH1_THRES);
								else {
#ifdef PHY_ABSTRACTION
									metric0 = rx_pucch_emul(phy_vars_eNB,i,
										format,
										pucch_payload0,
										last_slot>>1);
#endif
								}
							}

							// Check n1_pucch1 metric
							if (n1_pucch1 != -1) {
								if (abstraction_flag == 0)
									metric1 = rx_pucch(phy_vars_eNB,
									format,
									i,
									(u16)n1_pucch1,
									0, //n2_pucch
									1, // shortened format
									pucch_payload1,
									last_slot>>1,
									PUCCH1_THRES);
								else {
#ifdef PHY_ABSTRACTION
									metric1 = rx_pucch_emul(phy_vars_eNB,i,
										format,
										pucch_payload1,
										last_slot>>1);
#endif
								}
							}

							if (bundling_flag == multiplexing) {
								pucch_payload = (metric1>metric0) ? pucch_payload1 : pucch_payload0;
								pucch_sel     = (metric1>metric0) ? 1 : 0;
							}
							else {

								if (n1_pucch1 != -1)
									pucch_payload = pucch_payload1;
								else if (n1_pucch0 != -1)
									pucch_payload = pucch_payload0;

								pucch_sel = 2;  // indicate that this is a bundled ACK/NAK  
							}


							process_HARQ_feedback(i,last_slot>>1,phy_vars_eNB,
								0,// pusch_flag
								pucch_payload,
								pucch_sel,
								SR_payload);
						}
					}
				} // PUCCH processing
		}
#endif
	} // loop i=0 ... NUMBER_OF_UE_MAX-1

	if (((last_slot&1) == 1 ) &&
		(pusch_active == 0)){
			if (abstraction_flag == 0) {
				//      LOG_D(PHY,"[eNB] Frame %d, subframe %d Doing I0_measurements\n",
				//	  (((last_slot>>1)==9)?-1:0) + phy_vars_eNB->frame,last_slot>>1);
				lte_eNB_I0_measurements(phy_vars_eNB,
					0,
					phy_vars_eNB->first_run_I0_measurements);
			}
#ifdef PHY_ABSTRACTION
			else {
#ifdef PHY_ABSTRACTION
				lte_eNB_I0_measurements_emul(phy_vars_eNB,
					sect_id);
#endif
			}
#endif


			if (I0_clear == 1)
				I0_clear = 0;
	}

#ifdef EMOS
	phy_procedures_emos_eNB_RX(last_slot);
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
int load_data(PHY_VARS_eNB *PHY_vars_eNB0){

}
int main(){
	u8 extended_prefix_flag=0;
	u8 N_RB_DL=25,osf=1;
	u8 tdd_config=3,frame_type=0;
	load_param_init(1,1,1,extended_prefix_flag,N_RB_DL,frame_type,tdd_config,osf);
	load_data(PHY_vars_eNB);
	phy_procedures_eNB_RX(12,PHY_vars_eNB,0);
	system("PAUSE");
	return 0;
}
#endif
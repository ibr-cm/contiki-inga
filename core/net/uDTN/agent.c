/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 * \author Julian Heinbokel <j.heinbokel@tu-bs.de>
 */
 
#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "timer.h"
#include "net/netstack.h"
#include "net/rime/rimeaddr.h"
#include "mmem.h"
#include "lib/memb.h"
#include "logging.h"
#include "node-id.h"

#include "api.h"
#include "registration.h"
#include "bundle.h"
#include "storage.h"
#include "sdnv.h"
#include "redundancy.h"
#include "dispatching.h"
#include "routing.h"
#include "dtn_network.h"
#include "custody.h"
#include "discovery.h"
#include "statistics.h"
#include "convergence_layer.h"

#include "agent.h"

//FIXME das war nur für dtn_send_bundle_event?
//static struct mmem * bundleptr;
uint32_t dtn_node_id;
uint32_t dtn_seq_nr;
PROCESS(agent_process, "AGENT process");
AUTOSTART_PROCESSES(&agent_process);

void agent_init(void) {
	// if the agent process is already running, to nothing
	if( process_is_running(&agent_process) ) {
		return;
	}

	// Otherwise start the agent process
	process_start(&agent_process, NULL);
}

/*  Bundle Protocol Prozess */
PROCESS_THREAD(agent_process, ev, data)
{
	//uint32_t * bundle_number_ptr = NULL;
	struct registration_api * reg;

	PROCESS_BEGIN();
	
	/* We obtain our dtn_node_id from the RIME address of the node */
	dtn_node_id = convert_rime_to_eid(&rimeaddr_node_addr);
	dtn_seq_nr = 0;
	
	/* We are initialized quite early - give Contiki some time to do its stuff */
	process_poll(&agent_process);
	PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

	mmem_init();
	convergence_layer_init();
	BUNDLE_STORAGE.init();
	REDUNDANCE.init();
	CUSTODY.init();
	ROUTING.init();
	DISCOVERY.init();
	registration_init();

	dtn_application_remove_event  = process_alloc_event();
	dtn_application_registration_event = process_alloc_event();
	dtn_application_status_event = process_alloc_event();
	dtn_receive_bundle_event = process_alloc_event();
	dtn_send_bundle_event = process_alloc_event();
	submit_data_to_application_event = process_alloc_event();
	dtn_beacon_event = process_alloc_event();
	dtn_send_admin_record_event = process_alloc_event();
	dtn_bundle_in_storage_event = process_alloc_event();
	dtn_send_bundle_to_node_event = process_alloc_event();
	dtn_processing_finished = process_alloc_event();
	dtn_bundle_stored = process_alloc_event();
	
	// We use printf here, to make this message visible in every case!
	printf("Starting DTN Bundle Protocol Agent with EID ipn:%lu\n", dtn_node_id);
		
	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev);

		if(ev == dtn_application_registration_event) {
			reg = (struct registration_api *) data;

			registration_new_application(reg->app_id, reg->application_process, reg->node_id);
			LOG(LOGD_DTN, LOG_AGENT, LOGL_INF, "New Service registration for endpoint %lu", reg->app_id);
			continue;
		}
					
		if(ev == dtn_application_status_event) {
			int status;
			reg = (struct registration_api *) data;
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Service switching status to %i", reg->status);
			if(reg->status == APP_ACTIVE)
				status = registration_set_active(reg->app_id, reg->node_id);
			else if(reg->status == APP_PASSIVE)
				status = registration_set_passive(reg->app_id, reg->node_id);
			
			if(status == -1) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "no registration found to switch");
			}

			continue;
		}
		
		if(ev == dtn_application_remove_event) {
			reg = (struct registration_api *) data;
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Unregistering service for endpoint %lu", reg->app_id);
			registration_remove_application(reg->app_id, reg->node_id);
			continue;
		}
		
		if(ev == dtn_send_bundle_event) {
			//FIXME das event wird nicht mehr gebraucht, funktionalität wurde aufgeteilt unter storage_*.c und bundle.c
		    // momentan benutzt statusreport_basic_send das noch...
			//LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_send_bundle_event(%p) with seqNo %lu", bundleptr, dtn_seq_nr);
		}
		
		if(ev == dtn_send_admin_record_event) {
			LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "Send admin record currently not implemented");
			continue;
		}

		if(ev == dtn_beacon_event){
			rimeaddr_t* src =(rimeaddr_t*) data;
			ROUTING.new_neighbor(src);
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_beacon_event for %u.%u", src->u8[0], src->u8[1]);
			continue;
		}

		//FIXME warum nicht in storage aufrufen?
		if(ev == dtn_bundle_in_storage_event){
			uint32_t * bundle_number = (uint32_t *) data;

			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "bundle %lu in storage", *bundle_number);

			if(ROUTING.new_bundle(bundle_number) < 0){
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "routing reports error when announcing new bundle %lu", *bundle_number);
				continue;
			}

			continue;
		}
		
		//FIXME das schauen wir uns später nochmal an...
	    if(ev == dtn_processing_finished) {
	    	// data should contain the bundlemem ptr
	    	struct mmem * bundlemem = (struct mmem *) data;

	    	// Notify routing, that service has finished processing a bundle
	    	ROUTING.locally_delivered(bundlemem);
	    }
	}
	PROCESS_END();
}

void agent_delete_bundle(uint32_t bundle_number){
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Agent deleting bundle no %lu", bundle_number);

	convergence_layer_delete_bundle(bundle_number);
	ROUTING.del_bundle(bundle_number);
	CUSTODY.del_from_list(bundle_number);
}
/** @} */

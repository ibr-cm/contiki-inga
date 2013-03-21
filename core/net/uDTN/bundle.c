/**
 * \addtogroup bundle
 *
 * @{
 */

/**
 * \file
 * \brief this file implements the bundle memory representation
 *
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de)
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 * \author Julian Heinbokel <j.heinbokel@tu-bs.de>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmem.h"
#include "sys/process.h"
#include "clock.h"
#include "logging.h"

#include "sdnv.h"
#include "bundleslot.h"
#include "agent.h"
#include "hash.h"
#include "storage.h"
#include "registration.h"

#include "bundle.h"

/**
 * "Internal" functions
 */
static uint8_t bundle_decode_block(struct mmem *bundlemem, uint8_t *buffer, int max_len);
static int bundle_encode_block(struct bundle_block_t *block, uint8_t *buffer, uint8_t max_len);

uint32_t bundle_calculate_bundle_number(uint32_t tstamp_seq, uint32_t tstamp, uint32_t src_node, uint32_t frag_offs, uint32_t app_len){
    return HASH.hash_convenience(tstamp_seq, tstamp, src_node, frag_offs, app_len);
}

struct mmem * bundle_create_bundle()
{
	int ret;
	struct bundle_slot_t *bs;
	struct bundle_t *bundle;

	bs = bundleslot_get_free();

	if( bs == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Could not allocate slot for a bundle");
		return NULL;
	}

    //FIXME
	ret = mmem_alloc(&bs->bundle, sizeof(struct bundle_t));
    //ret = mmem_alloc(&bs->bundle, MIN_BUNDLESLOT_SIZE);
	if (!ret) {
		bundleslot_free(bs);
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Could not allocate memory for a bundle");
		return NULL;
	}

	bundle = (struct bundle_t *) MMEM_PTR(&bs->bundle);
	memset(bundle, 0, sizeof(struct bundle_t));
	bundle->rec_time=(uint32_t) clock_seconds();
	bundle->num_blocks = 0;  //FIXME das raus
	bundle->source_process = PROCESS_CURRENT(); //FIXME das raus

	return &bs->bundle;
}

struct mmem * bundle_new_bundle(uint32_t dest, uint32_t dst_srv, uint32_t src_srv, uint32_t lifetime, uint32_t bundle_flags){
    struct mmem *bundlemem;
    struct bundle_t *bundle;
    bundlemem = bundle_create_bundle();
    if (!bundlemem)
        return NULL;

    bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

//FIXME !!!
//check this elsewhere?
#ifdef TEST_NO_NETWORK
    printf("new_bundle: TEST_NO_NETWORK\n");
#else
    /* Go and find the process from which the bundle has been sent */ //FIXME wird evtl. noch an anderer stelle benötigt?
    if(registration_return_status(src_srv, dtn_node_id) == -1 && bundle->source_process != &agent_process) {
//    if( registration_get_application_id(src_srv) == 0xFFFF && bundle->source_process != &agent_process) {
        LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Unregistered process %d tries to send a bundle", src_srv);
        bundle_decrement(bundlemem);
        return NULL;
    }
#endif

    /* Set the source node, source service, destination node, destination service */
    bundle->src_node=dtn_node_id;
    bundle->src_srv=src_srv;
    bundle->dst_node=dest;
    bundle->dst_srv=dst_srv;

    /* Set lifetime */
    bundle->lifetime = lifetime;

    /* Set the outgoing sequence number */
    bundle->tstamp_seq=dtn_seq_nr;
    dtn_seq_nr++;

    /* FIXME needed for bundle identification but unused atm, are already zero */
    //bundle->tstamp = 0;
    //bundle->frag_offs = 0;
    //bundle->app_len = 0;

    //FIXME aus agent.c
    /* Check for report-to and set node and service accordingly */
    bundle_get_attr(bundlemem, FLAGS, &bundle_flags);
    if( bundle_flags & BUNDLE_FLAG_REPORT ) {
        uint32_t report_to_node = 0;
        bundle_get_attr(bundlemem, REP_NODE, &report_to_node);

        if( report_to_node == 0 ) {
            bundle_set_attr(bundlemem, REP_NODE, &dtn_node_id);
        }

        uint32_t report_to_service = 0;
        bundle_get_attr(bundlemem, REP_SERV, &report_to_service);

        if( report_to_service ) {
            bundle_set_attr(bundlemem, REP_SERV, &src_srv);
        }
    }

    /* calculate & set bundle_num*/
    bundle->bundle_num = bundle_calculate_bundle_number(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->frag_offs, bundle->app_len);

    //FIXME
    printf("bundle_new_bundle: RecTime: %lu , NumBlocks: %u , SrcNode: %lu , SrcSrv: %lu , DestNode: %lu , DestSrv: %lu , SeqNr: %lu , Lifetime: %lu, ID: %lu\n",
            bundle->rec_time, bundle->num_blocks, bundle->src_node, bundle->src_srv, bundle->dst_node, bundle->dst_srv, bundle->tstamp_seq, bundle->lifetime, bundle->bundle_num);

    return bundlemem;
}

//FIXME add_segm_to_block(start_offset, end_offset, *data)
int bundle_add_block(struct mmem *bundlemem, uint8_t type, uint8_t flags, uint8_t *data, uint32_t d_len)
{
       struct bundle_t *bundle;
       struct bundle_block_t *block;
       uint8_t i;
       int n;

//       if(bundlemem->size < d_len){ //FIXME das geht so nicht...
//           //FIXME offset!
//           //FIXME mmem ->used_size hinzufügen?
//           return 0;  //FIXME stattdessen else ...
//       }

       //FIXME je nach durchlauf muss an die passende stelle gesprungen werden, mehr platz alloziert werden

       n = mmem_realloc(bundlemem, bundlemem->size + d_len + sizeof(struct bundle_block_t));
       if( !n ) {
               return -1;
       }

       bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

       /* FIXME: Make sure we don't traverse outside of our allocated memory */

       /* Go through the blocks until we're behind the last one */
       block = (struct bundle_block_t *) bundle->block_data;
       for (i=0;i<bundle->num_blocks;i++) {
               /* None of these is the last block anymore */
               block->flags &= ~BUNDLE_BLOCK_FLAG_LAST;
               block = (struct bundle_block_t *) &block->payload[block->block_size];
       }

       block->type = type;
       block->flags = BUNDLE_BLOCK_FLAG_LAST | flags;
       block->block_size = d_len;

       bundle->num_blocks++;

       memcpy(block->payload, data, d_len);

       //FIXME
       // Save the bundle in storage
       n = BUNDLE_STORAGE.save_bundle(bundlemem, STORAGE_NO_SEGMENT);

       /* Saving the bundle failed... */
       if( !n ) {
           /* Decrement the sequence number */
           printf("b.c: saving failed\n");
           dtn_seq_nr--;
           return 0;
       }

       return d_len;
}

//FIXME Block größer als RAM?
//      Muss von segmentierung wissen und das an anwendung kommunizieren?
struct bundle_block_t *bundle_get_block(struct mmem *bundlemem, uint8_t i)
{
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	struct bundle_block_t *block = (struct bundle_block_t *) bundle->block_data;

	if (i >= bundle->num_blocks)
		return NULL;

	for (;i!=0;i--) {
		block = (struct bundle_block_t *) &block->payload[block->block_size];
	}

	return block;
}

struct bundle_block_t *bundle_get_block_by_type(struct mmem *bundlemem, uint8_t type)
{
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	struct bundle_block_t *block = (struct bundle_block_t *) bundle->block_data;
	int i = 0;

	for(i=0; i<bundle->num_blocks; i++) {
		if( block->type == type ) {
			return block;
		}

		block = (struct bundle_block_t *) &block->payload[block->block_size];
	}

	return NULL;
}

struct bundle_block_t * bundle_get_payload_block(struct mmem * bundlemem) {
	return bundle_get_block_by_type(bundlemem, BUNDLE_BLOCK_TYPE_PAYLOAD);
}

uint8_t bundle_set_attr(struct mmem *bundlemem, uint8_t attr, uint32_t *val)
{
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "set attr %lx",*val);
	switch (attr) {
		case FLAGS:
			bundle->flags = *val;
			bundle->custody = 0x08 &(uint8_t) *val;
			break;
		case DEST_NODE:
			bundle->dst_node = *val;
			break;
		case DEST_SERV:
			bundle->dst_srv = *val;
			break;
		case SRC_NODE:
			bundle->src_node = *val;
			break;
		case SRC_SERV:
			bundle->src_srv = *val;
			break;
		case REP_NODE:
			bundle->rep_node = *val;
			break;
		case REP_SERV:
			bundle->rep_srv = *val;
			break;
		case CUST_NODE:
			bundle->cust_node = *val;
			break;
		case CUST_SERV:
			bundle->cust_srv = *val;
			break;
		case TIME_STAMP:
			bundle->tstamp = *val;
			break;
		case TIME_STAMP_SEQ_NR:
			bundle->tstamp_seq = *val;
			break;
		case LIFE_TIME:
			bundle->lifetime = *val;
			break;
		case DIRECTORY_LEN:
			if (*val != 0)
				LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Dictionary length needs to be 0 for CBHE");
			break;
		case FRAG_OFFSET:
			bundle->frag_offs = *val;
			break;
		case LENGTH:
			/* FIXME */
		default:
			LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Unknown attribute");
			return 0;
	}
	return 1;
}

uint8_t bundle_get_attr(struct mmem *bundlemem, uint8_t attr, uint32_t *val)
{
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "get attr: %d in %lx", attr, *val);
	switch (attr) {
		case FLAGS:
			*val = bundle->flags;
			break;
		case DEST_NODE:
			*val = bundle->dst_node;
			break;
		case DEST_SERV:
			*val = bundle->dst_srv;
			break;
		case SRC_NODE:
			*val = bundle->src_node;
			break;
		case SRC_SERV:
			*val = bundle->src_srv;
			break;
		case REP_NODE:
			*val = bundle->rep_node;
			break;
		case REP_SERV:
			*val = bundle->rep_srv;
			break;
		case CUST_NODE:
			*val = bundle->cust_node;
			break;
		case CUST_SERV:
			*val = bundle->cust_srv;
			break;
		case TIME_STAMP:
			*val = bundle->tstamp;
			break;
		case TIME_STAMP_SEQ_NR:
			*val = bundle->tstamp_seq;
			break;
		case LIFE_TIME:
			*val = bundle->lifetime;
			break;
		case DIRECTORY_LEN:
			*val = 0;
			break;
		case FRAG_OFFSET:
			*val = bundle->frag_offs;
			break;
		case LENGTH:
			/* FIXME */
		default:
			LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Unknown attribute");
			return 0;
	}
	return 1;
}

struct mmem *bundle_recover_bundle(uint8_t *buffer, int size)
{
    //FIXME 2 weitere parameter: segm_flags, source (siehe convergence_layer.c
    //wenn 2. - n. segment, nur blöcke ersetzen oder anhängen
	uint32_t primary_size, value;
	uint8_t offs = 0;
	struct mmem *bundlemem;
	struct bundle_t *bundle;
	bundlemem = bundle_create_bundle();
	if (!bundlemem)
		return NULL;

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "rec bptr: %p  blptr:%p",bundle,buffer);

	/* Version 0x06 is the one described and supported in RFC5050 */
	if (buffer[0] != 0x06) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Version 0x%02x not supported", buffer[0]);
		goto err;
	}
	offs++;

	/* Flags */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->flags);

	/* Block Length - Number of bytes in this block following this
	 * field */
	offs += sdnv_decode(&buffer[offs], size-offs, &primary_size);
	primary_size += offs;

	/* Destination node + SSP */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->dst_node);
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->dst_srv);

	/* Source node + SSP */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->src_node);
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->src_srv);

	/* Report-to node + SSP */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->rep_node);
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->rep_srv);

	/* Custodian node + SSP */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->cust_node);
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->cust_srv);

	/* Creation Timestamp */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->tstamp);

	/* Creation Timestamp Sequence Number */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->tstamp_seq);

	/* Lifetime */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->lifetime);

	/* Directory Length */
	offs += sdnv_decode(&buffer[offs], size-offs, &value);
	if (value != 0) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Bundle does not use CBHE.");
		goto err;
	}

	if (bundle->flags & BUNDLE_FLAG_FRAGMENT) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_INF, "Bundle is a fragment");

		/* Fragment Offset */
		offs += sdnv_decode(&buffer[offs], size-offs, &bundle->frag_offs);

		/* Total App Data Unit Length */
		offs += sdnv_decode(&buffer[offs], size-offs, &bundle->app_len);
	}

	if (offs != primary_size) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Problem decoding the primary bundle block.");
		goto err;
	}

	/* calculate & set bundle_num*/
	bundle->bundle_num = bundle_calculate_bundle_number(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->frag_offs, bundle->app_len);

	//FIXME
	printf("bundle_recover_bundle: RecTime: %lu , NumBlocks: %u , SrcNode: %lu , SrcSrv: %lu , DestNode: %lu , DestSrv: %lu , SeqNr: %lu , Lifetime: %lu, ID: %lu\n",
            bundle->rec_time, bundle->num_blocks, bundle->src_node, bundle->src_srv, bundle->dst_node, bundle->dst_srv, bundle->tstamp_seq, bundle->lifetime, bundle->bundle_num);

	/* FIXME: Loop around and decode all blocks - does this work? */
	while (size-offs > 1) {
		offs += bundle_decode_block(bundlemem, &buffer[offs], size-offs);
	}

    //FIXME
    // Save the bundle in storage
	//BUNDLE_STORAGE.save_bundle(bundlemem, STORAGE_NO_SEGMENT);

	return bundlemem;

err:
	bundle_delete_bundle(bundlemem);
	return NULL;

}

//FIXME wenn segment, block ist größer als momentan vorhandene daten
//      oder beginnt nicht mit headerinfos, sondern mit rohen daten, headerinfos für weiteren block folgen evtl.
//      => es muss sich state gemerkt werden
static uint8_t bundle_decode_block(struct mmem *bundlemem, uint8_t *buffer, int max_len)
{
	uint8_t type, block_offs, offs = 0;
	uint32_t flags, size;
	struct bundle_t *bundle;
	struct bundle_block_t *block;
	int n;

	type = buffer[offs];
	offs++;

	/* Flags */
	offs += sdnv_decode(&buffer[offs], max_len-offs, &flags);

	/* Payload Size */
	offs += sdnv_decode(&buffer[offs], max_len-offs, &size);
	if (size > max_len-offs) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Bundle payload length too big.");
		return 0;
	}

	block_offs = bundlemem->size;

	n = mmem_realloc(bundlemem, bundlemem->size + sizeof(struct bundle_block_t) + size);
	if( !n ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Bundle payload length too big for MMEM.");
		return 0;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	bundle->num_blocks++;

	/* Add the block to the end of the bundle */
	block = (struct bundle_block_t *)((uint8_t *)bundle + block_offs);
	block->type = type;
	block->flags = flags;
	block->block_size = size;

	/* Copy the actual payload over */
	memcpy(block->payload, &buffer[offs], block->block_size);

	return offs + block->block_size;
}

int bundle_encode_bundle(struct mmem *bundlemem, uint8_t *buffer, int max_len)
{
	uint32_t value;
	uint8_t offs = 0, blklen_offs, i;
	int ret, blklen_size;
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	struct bundle_block_t *block;

	/* Hardcode the version to 0x06 */
	buffer[0] = 0x06;
	offs++;

	/* Flags */
	ret = sdnv_encode(bundle->flags, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Block length will be calculated later
	 * reserve one byte for now */
	blklen_offs = offs;
	offs++;

	/* Destination node + SSP */
	ret = sdnv_encode(bundle->dst_node, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	ret = sdnv_encode(bundle->dst_srv, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Source node + SSP */
	ret = sdnv_encode(bundle->src_node, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	ret = sdnv_encode(bundle->src_srv, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Report-to node + SSP */
	ret = sdnv_encode(bundle->rep_node, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	ret = sdnv_encode(bundle->rep_srv, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Custodian node + SSP */
	ret = sdnv_encode(bundle->cust_node, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	ret = sdnv_encode(bundle->cust_srv, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Creation Timestamp */
	ret = sdnv_encode(bundle->tstamp, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Creation Timestamp Sequence Number */
	ret = sdnv_encode(bundle->tstamp_seq, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Lifetime */
	ret = sdnv_encode(bundle->lifetime, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Directory Length */
	ret = sdnv_encode(0l, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	if (bundle->flags & BUNDLE_FLAG_FRAGMENT) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_INF, "Bundle is a fragment");

		/* Fragment Offset */
		ret = sdnv_encode(bundle->frag_offs, &buffer[offs], max_len - offs);
		if (ret < 0)
			return -1;
		offs += ret;

		/* Total App Data Unit Length */
		ret = sdnv_encode(bundle->app_len, &buffer[offs], max_len - offs);
		if (ret < 0)
			return -1;
		offs += ret;
	}

	/* Calculate block length value */
	value = offs - blklen_offs - 1;
	blklen_size = sdnv_encoding_len(value);
	/* Move the data around */
	if (blklen_size > 1) {
		memmove(&buffer[blklen_offs+blklen_size], &buffer[blklen_offs+1], value);
	}
	ret = sdnv_encode(value, &buffer[blklen_offs], blklen_size);

	offs += ret-1;

	block = (struct bundle_block_t *) bundle->block_data;
	for (i=0;i<bundle->num_blocks;i++) {
		offs += bundle_encode_block(block, &buffer[offs], max_len - offs);
		/* Reference the next block */
		block = (struct bundle_block_t *) &block->payload[block->block_size];
	}

	return offs;
}

static int bundle_encode_block(struct bundle_block_t *block, uint8_t *buffer, uint8_t max_len)
{
	uint8_t offs = 0;
	int ret;
	uint32_t value;

	/* Encode the next block */
	buffer[offs] = block->type;
	offs++;

	/* Flags */
	ret = sdnv_encode(block->flags, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Blocksize */
	value = block->block_size;
	ret = sdnv_encode(value, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Payload */
	memcpy(&buffer[offs], block->payload, block->block_size);
	offs += block->block_size;

	return offs;
}

int bundle_increment(struct mmem *bundlemem)
{
	struct bundle_slot_t *bs;
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "bundle_increment(%p) %u", bundle, bundle->rec_time);

	bs = container_of(bundlemem, struct bundle_slot_t, bundle);
	return bundleslot_increment(bs);
}

int bundle_decrement(struct mmem *bundlemem)
{
	struct bundle_slot_t *bs;
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "bundle_decrement(%p) %u", bundle, bundle->rec_time);

	bs = container_of(bundlemem, struct bundle_slot_t, bundle);
	return bundleslot_decrement(bs);
}

uint16_t bundle_delete_bundle(struct mmem *bundlemem)
{
	struct bundle_slot_t *bs;
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "delete %p %u", bundle,bundle->rec_time);

	bs = container_of(bundlemem, struct bundle_slot_t, bundle);
	bundleslot_free(bs);
	return 1;
}

rimeaddr_t convert_eid_to_rime(uint32_t eid) {
	rimeaddr_t dest;
	dest.u8[1] = (eid & 0x000000FF) >> 0;
	dest.u8[0] = (eid & 0x0000FF00) >> 8;
	return dest;
}

uint32_t convert_rime_to_eid(rimeaddr_t * dest) {
	uint32_t eid = 0;
	eid = (dest->u8[1] & 0xFF) + ((dest->u8[0] & 0xFF) << 8);

    LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "converting rimeaddr %u.%u to eid %lu", dest->u8[0], dest->u8[1], eid);
    printf("converting rimeaddr %u.%u to eid %lu\n", dest->u8[0], dest->u8[1], eid); //FIXME

	return eid;
}

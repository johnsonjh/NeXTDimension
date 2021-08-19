#include <sys/message.h>
#include <sys/types.h>
#include <vm/vm_param.h>
#include <vm/vm_prot.h>
/*
 * Make sure that this message is in a form we can support, and take care
 * of out of line data.  We can assume that the message structure is sane
 * at this point, so we don't look for bad lengths and whatnot.
 *
 * Out of band data is delivered by placing it in the backing store task on the host.
 * The data is demand paged over the backplane to get to the NextDimension board.
 */
 void
ipc_oob_receive( msg_header_t *msg )
{
	caddr_t saddr;
	caddr_t staddr;
	caddr_t endaddr;
	msg_type_long_t *tp;
	unsigned int tn;
	unsigned long ts;
	register long elts;
	vm_size_t numbytes;
	
	if ( msg->msg_simple == TRUE )	/* Safety check */
		return;

	saddr = (caddr_t)(msg + 1);	/* Start after header */
	endaddr = ((caddr_t)msg) + msg->msg_size;	/* And stop at the end... */
	
	while ( saddr < endaddr )
	{
		tp = (msg_type_long_t *)saddr;
		staddr = saddr;
		if (tp->msg_type_header.msg_type_longform) {
			tn = tp->msg_type_long_name;
			ts = tp->msg_type_long_size;
			elts = tp->msg_type_long_number;
			saddr += sizeof(msg_type_long_t);
		} else {
			tn = tp->msg_type_header.msg_type_name;
			ts = tp->msg_type_header.msg_type_size;
			elts = tp->msg_type_header.msg_type_number;
			saddr += sizeof(msg_type_t);
		}

		numbytes = ((elts * ts) + 7) >> 3;
		
		/*
		 * At this point, saddr points to the typed data, 
		 * and staddr points to the type info for the data.
		 * numbytes holds the lenght of the typed data in bytes.
		 *
		 * Next, we evaluate the type to see if the data is out of line.
		 * If so, the data is in our backing store at the indicated address.
		 * Set up the pmap to reflect the presence of the new data in our
		 * backing store.
		 */
		if ( tp->msg_type_header.msg_type_inline == FALSE )
		{
			vm_offset_t	vaddr;
			
			vaddr = *((vm_offset_t *)saddr);
			/* Set up pmap for new memory. */
			pmap_add(	current_pmap(),
					trunc_page(vaddr),
					round_page(vaddr + numbytes),
					VM_PROT_DEFAULT,
					TRUE ); 	/* Use backing store */
			saddr += sizeof (caddr_t);	/* Skip over pointer to data */ 
		}
		else
			saddr += ((numbytes+3) & (~0x3));/* Skip over the typed data */
	}
}

/*
 * Process a message sent to the host containing out of band data.
 *
 * Dirty pages in the OOB data are flushed to the host.  If the data is marked
 * to be deallocated, we destroy the pmap entries for the data.  The host takes
 * care of deleting data from the backing store.
 */
 void
ipc_oob_send( msg_header_t *msg )
{
	caddr_t saddr;
	caddr_t staddr;
	caddr_t endaddr;
	msg_type_long_t *tp;
	unsigned int tn;
	unsigned long ts;
	register long elts;
	vm_size_t numbytes;
	
	if ( msg->msg_simple == TRUE )	/* Safety check */
		return;

	saddr = (caddr_t)(msg + 1);	/* Start after header */
	endaddr = ((caddr_t)msg) + msg->msg_size;	/* And stop at the end... */
	
	while ( saddr < endaddr )
	{
		tp = (msg_type_long_t *)saddr;
		staddr = saddr;
		if (tp->msg_type_header.msg_type_longform) {
			tn = tp->msg_type_long_name;
			ts = tp->msg_type_long_size;
			elts = tp->msg_type_long_number;
			saddr += sizeof(msg_type_long_t);
		} else {
			tn = tp->msg_type_header.msg_type_name;
			ts = tp->msg_type_header.msg_type_size;
			elts = tp->msg_type_header.msg_type_number;
			saddr += sizeof(msg_type_t);
		}

		numbytes = ((elts * ts) + 7) >> 3;
		
		/*
		 * At this point, saddr points to the typed data, 
		 * and staddr points to the type info for the data.
		 * numbytes holds the lenght of the typed data in bytes.
		 *
		 * Next, we evaluate the type to see if the data is out of line.
		 * If so, we force dirty pages in the selected range out to the host.
		 * Finally, if the range is marked msg_type_deallocate, we remove that
		 * range from the pmap.  The host will remove it from the backing
		 * store if needed.
		 */
		if ( tp->msg_type_header.msg_type_inline == FALSE )
		{
			vm_offset_t	vaddr;
			
			vaddr = *((vm_offset_t *)saddr);
			/* Flush dirty pages to backing store. */
			pmap_clean(	current_pmap(),
					trunc_page(vaddr),
					round_page(vaddr + numbytes) );
			if ( tp->msg_type_header.msg_type_deallocate == TRUE )
			{
				pmap_remove(	current_pmap(),
						trunc_page(vaddr),
						round_page(vaddr + numbytes) ); 
			}
			saddr += sizeof (caddr_t);  /* Skip over pointer to data */ 
		}
		else
			saddr += ((numbytes+3) & (~0x3));/* Skip over typed data */
	}
}

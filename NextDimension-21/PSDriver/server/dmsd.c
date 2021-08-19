
#include <mach.h>
#include "video.h"

extern vm_offset_t dp_base_addr;


static inline SetDataPathReg(int offset,int val)
{
    vm_offset_t addr = dp_base_addr + offset;
    (*(v_int *)(dp_base_addr + offset)) = val;
}

static inline ReadDataPathReg(int offset)
{
    vm_offset_t addr = dp_base_addr + offset;
    return((*(v_int *)(dp_base_addr + offset)));
}

/*   IIC Bus transmission routines */


#define IIC_BUSY (1 << 31)
#define IIC_ERROR (1 << 30)
#define IIC_CONTINUE ( 1 << 29)



/* wait for iic transfer to finish */
static int iicwait()
{
    int stat,i,k;
    extern int lbolt;


    for(k = 0; k < 2000; k++) {
	if(!((stat = ReadDataPathReg(OFFSET_DP_IIC_1)) & IIC_BUSY)) {
	    return((stat & IIC_ERROR) != 0);
	}
    }
    printf("IIC Bus Timeout\n");
    return(1);
}
	
/* transmit a byte over the IIC bus.  Returns the state of the ACK bit. */
static inline int SendIIC(int reg_offset, int data)
{
    SetDataPathReg(reg_offset, data);
    return(iicwait());
}


/* send a complete packet over IIC bus, we take a port (which we ignore)
   so we can hook up to mig easily */

int ps_iicsendbytes(port_t port, int iicaddr, unsigned char *data, int dataCnt)
{
    int i, stat, continue_bit;
    int addr_bits, dp_reg, out_word;
    
    stat = 0; /* init status */
    iicwait();	/* wait for the iic bus to be free */
    addr_bits = (iicaddr & 0xfe) << 8;
    dp_reg = OFFSET_DP_IIC_1;

    for( i = 0; i < dataCnt; i++, addr_bits = 0, dp_reg = OFFSET_DP_IIC_2) {
	continue_bit = (dataCnt == 1) ? 0 : IIC_CONTINUE;
	out_word =  (*data++ & 0xff)| addr_bits | continue_bit | IIC_BUSY;
	stat |= SendIIC(dp_reg, out_word);
    }
    if(stat)
	printf("DMSD error on IIC bus\n");
    else
	printf("DMSD IIC transaction complete (no errors)\n");
    return(stat);
}








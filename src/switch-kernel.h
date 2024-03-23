#ifndef __SWITCH_KERNEL_
#define __SWITCH_KERNEL_

extern void switch_kernel(void);
extern uint8_t get_value_at_address (UINT24 address);
extern void set_value_at_address (UINT24 address,UINT8 value);

#endif //__SWITCH_KERNEL_
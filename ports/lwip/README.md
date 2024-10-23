## Port of BACnet/IP to LwIP 2.x

This LwIP BACnet/IP port uses a Makefile designed to be used from
the continuous integration pipeline. The container is configured with
Ubuntu Linux and uses APT to install liblwip-dev library and header files.
This build method ensures that the code is valid for the particular LwIP
library that is released with Ubuntu.

## Integration Hints

Developer must set the IP address, netmask, and UDP port into the BACnet/IP module.

Integration used a main loop bare metal design, shown here as an example.

    int main(void)
    {
        static bool valid_ip_address = false;
        static bool bacnet_ip_initialized = false;

        bacnet_init();
        LwIP_Init();
        /* Infinite loop */
        for (;;) {
            /* Periodic tasks */
            LwIP_Periodic_Handler();
            /* Run when there is a valid IP address */
            if (netif.ip_addr.addr != 0) {
                if (!valid_ip_address) {
                    valid_ip_address = true;
                    bip_set_addr(netif.ip_addr.addr);
                    bip_set_broadcast_addr(
                        (netif.ip_addr.addr & netif.netmask.addr) |
                        (~netif.netmask.addr));
                } else {
                    if (bacnet_ip_initialized) {
                        bacnet_task();
                    } else {
                        bip_init(NULL);
                        bacnet_ip_initialized = true;
                    }
                }
            } else {
                valid_ip_address = false;
            }
        }
    }

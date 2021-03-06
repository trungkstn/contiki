/*
 * Copyright (C) 2015-2016, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "eth-conf.h"
#include <stdio.h>
#include "net/eth-proc.h"
#include "contiki-net.h"
#include "net/ip/dhcpc.h"
#include "net/linkaddr.h"

#if NETSTACK_CONF_WITH_IPV6
const linkaddr_t linkaddr_null = { { 0, 0, 0, 0, 0, 0 } };
#else
/* 192.0.2.0/24 is a block reserved for documentation by RFC 5737. */
#define SUBNET_IP       192, 0, 2
#define NETMASK_IP      255, 255, 255, 0
#define HOST_IP         SUBNET_IP, 2
#define GATEWAY_IP      SUBNET_IP, 1
#define NAMESERVER_IP   GATEWAY_IP
PROCESS(dhcp_process, "DHCP");
#endif

/*---------------------------------------------------------------------------*/
void
eth_init(void)
{
#if !NETSTACK_CONF_WITH_IPV6
  uip_ipaddr_t ip_addr;

#define SET_IP_ADDR(x) \
  uip_ipaddr(&ip_addr, x)

  SET_IP_ADDR(HOST_IP);
  uip_sethostaddr(&ip_addr);

  SET_IP_ADDR(NETMASK_IP);
  uip_setnetmask(&ip_addr);

  SET_IP_ADDR(GATEWAY_IP);
  uip_setdraddr(&ip_addr);

#if WITH_DNS
  SET_IP_ADDR(NAMESERVER_IP);
  uip_nameserver_update(&ip_addr, UIP_NAMESERVER_INFINITE_LIFETIME);
#endif
#endif

  process_start(&eth_process, NULL);
#if !NETSTACK_CONF_WITH_IPV6
  /* Comment out the following line to disable DHCP and simply use the static
   * IP configuration setup above.
   */
  process_start(&dhcp_process, NULL);
#endif
}
#if !NETSTACK_CONF_WITH_IPV6
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dhcp_process, ev, data)
{
  PROCESS_BEGIN();

  dhcpc_init(uip_lladdr.addr, sizeof(uip_lladdr.addr));

  printf("Requesting DHCP configuration...\n");
  dhcpc_request();

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == tcpip_event || ev == PROCESS_EVENT_TIMER) {
      dhcpc_appcall(ev, data);
    } else if(ev == PROCESS_EVENT_EXIT) {
      process_exit(&dhcp_process);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
dhcpc_configured(const struct dhcpc_state *s)
{
  uip_sethostaddr(&s->ipaddr);
  uip_setnetmask(&s->netmask);
  uip_setdraddr(&s->default_router);
  uip_nameserver_update(&s->dnsaddr, UIP_NAMESERVER_INFINITE_LIFETIME);
  printf("DHCP configured:\n");
  printf(" - Host IP:        %d.%d.%d.%d\n", uip_ipaddr_to_quad(&s->ipaddr));
  printf(" - Netmask:        %d.%d.%d.%d\n", uip_ipaddr_to_quad(&s->netmask));
  printf(" - Default router: %d.%d.%d.%d\n",
         uip_ipaddr_to_quad(&s->default_router));
  printf(" - DNS server:     %d.%d.%d.%d\n", uip_ipaddr_to_quad(&s->dnsaddr));
}
/*---------------------------------------------------------------------------*/
void
dhcpc_unconfigured(const struct dhcpc_state *s)
{
  printf("DHCP unconfigured.\n");
}
/*---------------------------------------------------------------------------*/
#endif

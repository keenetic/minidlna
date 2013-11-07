/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 *
 * Copyright (c) 2006, Thomas Bernard
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#if defined(sun)
#include <sys/sockio.h>
#endif

#include "upnpglobalvars.h"
#include "getifaddr.h"
#include "log.h"
#include "minidlnatypes.h"

static int
getifaddr(const char *ifname, int notify)
{
#if HAVE_GETIFADDRS
	struct ifaddrs *ifap, *p;
	struct sockaddr_in *addr_in;

	if (getifaddrs(&ifap) != 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "getifaddrs(): %s\n", strerror(errno));
		return -1;
	}

	for (p = ifap; p != NULL; p = p->ifa_next)
	{
		if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET)
			continue;
		if (ifname && strcmp(p->ifa_name, ifname) != 0)
			continue;
		addr_in = (struct sockaddr_in *)p->ifa_addr;
		if (!ifname && (p->ifa_flags & (IFF_LOOPBACK | IFF_SLAVE)))
			continue;
		memcpy(&lan_addr[n_lan_addr].addr, &addr_in->sin_addr, sizeof(lan_addr[n_lan_addr].addr));
		if (!inet_ntop(AF_INET, &addr_in->sin_addr, lan_addr[n_lan_addr].str, sizeof(lan_addr[0].str)) )
		{
			DPRINTF(E_ERROR, L_GENERAL, "inet_ntop(): %s\n", strerror(errno));
			continue;
		}
		addr_in = (struct sockaddr_in *)p->ifa_netmask;
		memcpy(&lan_addr[n_lan_addr].mask, &addr_in->sin_addr, sizeof(lan_addr[n_lan_addr].mask));
		lan_addr[n_lan_addr].snotify = OpenAndConfSSDPNotifySocket(lan_addr[n_lan_addr].addr.s_addr);
		if (lan_addr[n_lan_addr].snotify >= 0)
		{
			if (notify)
				SendSSDPNotifies(lan_addr[n_lan_addr].snotify, lan_addr[n_lan_addr].str,
					runtime_vars.port, runtime_vars.notify_interval);
			n_lan_addr++;
		}
		if (ifname || n_lan_addr >= MAX_LAN_ADDR)
			break;
	}
	freeifaddrs(ifap);
	if (ifname && !p)
	{
		DPRINTF(E_ERROR, L_GENERAL, "Network interface %s not found\n", ifname);
		return -1;
	}
#else
	int s = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	struct ifconf ifc;
	struct ifreq *ifr;
	char buf[8192];
	int i, n;

	memset(&ifc, '\0', sizeof(ifc));
	ifc.ifc_buf = buf;
	ifc.ifc_len = sizeof(buf);

	if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "SIOCGIFCONF: %s\n", strerror(errno));
		close(s);
		return -1;
	}

	n = ifc.ifc_len / sizeof(struct ifreq);
	for (i = 0; i < n; i++)
	{
		ifr = &ifc.ifc_req[i];
		if (ifname && strcmp(ifr->ifr_name, ifname) != 0)
			continue;
		if (!ifname &&
		    (ioctl(s, SIOCGIFFLAGS, ifr) < 0 || ifr->ifr_ifru.ifru_flags & IFF_LOOPBACK))
			continue;
		if (ioctl(s, SIOCGIFADDR, ifr) < 0)
			continue;
		memcpy(&addr, &(ifr->ifr_addr), sizeof(addr));
		memcpy(&lan_addr[n_lan_addr].addr, &addr.sin_addr, sizeof(lan_addr[n_lan_addr].addr));
		if (!inet_ntop(AF_INET, &addr.sin_addr, lan_addr[n_lan_addr].str, sizeof(lan_addr[0].str)))
		{
			DPRINTF(E_ERROR, L_GENERAL, "inet_ntop(): %s\n", strerror(errno));
			close(s);
			continue;
		}
		if (ioctl(s, SIOCGIFNETMASK, ifr) < 0)
			continue;
		memcpy(&addr, &(ifr->ifr_addr), sizeof(addr));
		memcpy(&lan_addr[n_lan_addr].mask, &addr.sin_addr, sizeof(addr));
		lan_addr[n_lan_addr].snotify = OpenAndConfSSDPNotifySocket(lan_addr[i].addr.s_addr);
		if (lan_addr[n_lan_addr].snotify >= 0)
		{
			if (notify)
				SendSSDPNotifies(lan_addr[n_lan_addr].snotify, lan_addr[n_lan_addr].str,
					runtime_vars.port, runtime_vars.notify_interval);
			n_lan_addr++;
		}
		if (ifname || n_lan_addr >= MAX_LAN_ADDR)
			break;
	}
	close(s);
	if (ifname && i == n)
	{
		DPRINTF(E_ERROR, L_GENERAL, "Network interface %s not found\n", ifname);
		return -1;
	}
#endif
	return n_lan_addr;
}

int
getsysaddr(char * buf, int len)
{
	int i;
	int s = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	struct ifreq ifr;
	uint32_t mask;
	int ret = -1;

	for (i=1; i > 0; i++)
	{
		ifr.ifr_ifindex = i;
		if( ioctl(s, SIOCGIFNAME, &ifr) < 0 )
			break;
		if(ioctl(s, SIOCGIFADDR, &ifr, sizeof(struct ifreq)) < 0)
			continue;
		memcpy(&addr, &ifr.ifr_addr, sizeof(addr));
		if(strncmp(inet_ntoa(addr.sin_addr), "127.", 4) == 0)
			continue;
		if(ioctl(s, SIOCGIFNETMASK, &ifr, sizeof(struct ifreq)) < 0)
			continue;
		if(!inet_ntop(AF_INET, &addr.sin_addr, buf, len))
		{
			DPRINTF(E_ERROR, L_GENERAL, "inet_ntop(): %s\n", strerror(errno));
			close(s);
			break;
		}
		ret = 0;

		memcpy(&addr, &ifr.ifr_netmask, sizeof(addr));
		mask = ntohl(addr.sin_addr.s_addr);
		for (i = 0; i < 32; i++)
		{
			if ((mask >> i) & 1)
				break;
		}
		mask = 32 - i;
		if (mask)
		{
			i = strlen(buf);
			snprintf(buf+i, len-i, "/%u", mask);
		}
		break;
	}
	close(s);

	return(ret);
}

int
getsyshwaddr(char * buf, int len)
{
	struct if_nameindex *ifaces, *if_idx;
	unsigned char mac[6];
	struct ifreq ifr;
	int fd;
	int ret = -1;

	memset(&mac, '\0', sizeof(mac));
	/* Get the spatially unique node identifier */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 )
		return(ret);

	ifaces = if_nameindex();
	if(!ifaces)
		return(ret);

	for(if_idx = ifaces; if_idx->if_index; if_idx++)
	{
		strncpy(ifr.ifr_name, if_idx->if_name, IFNAMSIZ);
		if(ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
			continue;
		if(ifr.ifr_ifru.ifru_flags & IFF_LOOPBACK)
			continue;
		if( ioctl(fd, SIOCGIFHWADDR, &ifr) < 0 )
			continue;
		if( MACADDR_IS_ZERO(ifr.ifr_hwaddr.sa_data) )
			continue;
		ret = 0;
		break;
	}
	if_freenameindex(ifaces);
	close(fd);

	if(ret == 0)
	{
		if(len > 12)
		{
			memmove(mac, ifr.ifr_hwaddr.sa_data, 6);
			sprintf(buf, "%02x%02x%02x%02x%02x%02x",
			        mac[0]&0xFF, mac[1]&0xFF, mac[2]&0xFF,
			        mac[3]&0xFF, mac[4]&0xFF, mac[5]&0xFF);
		}
		else if(len == 6)
		{
			memmove(buf, ifr.ifr_hwaddr.sa_data, 6);
		}
	}
	return ret;
}

int
get_remote_mac(struct in_addr ip_addr, unsigned char * mac)
{
	struct in_addr arp_ent;
	FILE * arp;
	char remote_ip[16];
	int matches, hwtype, flags;
	memset(mac, 0xFF, 6);

 	arp = fopen("/proc/net/arp", "r");
	if( !arp )
		return 1;
	while( !feof(arp) )
	{
	        matches = fscanf(arp, "%15s 0x%8X 0x%8X %2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
		                      remote_ip, &hwtype, &flags,
		                      &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
		if( matches != 9 )
			continue;
		inet_pton(AF_INET, remote_ip, &arp_ent);
		if( ip_addr.s_addr == arp_ent.s_addr )
			break;
		mac[0] = 0xFF;
	}
	fclose(arp);

	if( mac[0] == 0xFF )
	{
		memset(mac, 0xFF, 6);
		return 1;
	}

	return 0;
}
void
reload_ifaces(int notify)
{
	int i;

	for (i = 0; i < n_lan_addr; i++)
	{
		close(lan_addr[i].snotify);
	}
	n_lan_addr = 0;

	if (runtime_vars.ifaces[0])
	{
		for (i = 0; runtime_vars.ifaces[i]; i++)
		{
			getifaddr(runtime_vars.ifaces[i], notify);
		}
	}
	else
		getifaddr(NULL, notify);

	for (i = 0; i < n_lan_addr; i++)
	{
		DPRINTF(E_INFO, L_GENERAL, "Enabled interface %s/%s\n",
			lan_addr[i].str, inet_ntoa(lan_addr[i].mask));
	}
}
/* parselanaddr()
 * parse address with mask
 * ex: 192.168.1.1/24
 * return value : 
 *    0 : ok
 *   -1 : error */
static int
parselanaddr(struct lan_addr_s * lan_addr, const char * str)
{
	const char * p;
	int nbits = 24;
	int n;
	p = str;
	while(*p && *p != '/' && !isspace(*p))
		p++;
	n = p - str;
	if(*p == '/')
	{
		nbits = atoi(++p);
		while(*p && !isspace(*p))
			p++;
	}
	if(n>15)
	{
		DPRINTF(E_OFF, L_GENERAL, "Error parsing address/mask: %s\n", str);
		return -1;
	}
	memcpy(lan_addr->str, str, n);
	lan_addr->str[n] = '\0';
	if(!inet_aton(lan_addr->str, &lan_addr->addr))
	{
		DPRINTF(E_OFF, L_GENERAL, "Error parsing address: %s\n", str);
		return -1;
	}
	lan_addr->mask.s_addr = htonl(nbits ? (0xffffffff << (32 - nbits)) : 0);
	return 0;
}

#if 0
void
get_lan_addresses(
		const char *const value,
		struct lan_addr_s *addresses,
		int *address_count)
{
	const char *string, *word;
	char ip_addr[INET_ADDRSTRLEN + 3] = {'\0'};

	*address_count = 0;

	for( string = value; (word = strtok((char *)string, ",")); string = NULL )
	{
		if(*address_count < MAX_LAN_ADDR)
		{
			if(getifaddr(word, ip_addr, sizeof(ip_addr)) >= 0)
			{
				if( *ip_addr && parselanaddr(&addresses[*address_count], ip_addr) == 0 )
				{
					strcpy(addresses[*address_count].if_name, word);
					(*address_count)++;
				}
			}
		}
		else
		{
			DPRINTF(E_ERROR, L_GENERAL, "Too many listening ips (max: %d), ignoring %s\n",
					MAX_LAN_ADDR, word);
		}
	}
}
#endif
int
OpenAndConfMonitorSocket(void)
{
#ifdef HAVE_NETLINK
	struct sockaddr_nl addr;
	int s;
	int ret;

	s = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (s < 0)
	{
		perror("couldn't open NETLINK_ROUTE socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = RTMGRP_IPV4_IFADDR;

	ret = bind(s, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0)
	{
		perror("couldn't bind");
		return -1;
	}

	return s;
#else
	return -1;
#endif
}

void
ProcessMonitorEvent(int s)
{
#ifdef HAVE_NETLINK
	int len;
	char buf[4096];
	struct nlmsghdr *nlh;
	int changed = 0;

	nlh = (struct nlmsghdr*)buf;

	len = recv(s, nlh, sizeof(buf), 0);
	if (len <= 0)
		return;
	while ((NLMSG_OK(nlh, len)) && (nlh->nlmsg_type != NLMSG_DONE))
	{
		if (nlh->nlmsg_type == RTM_NEWADDR ||
		    nlh->nlmsg_type == RTM_DELADDR)
		{
			changed = 1;
		}
		nlh = NLMSG_NEXT(nlh, len);
	}
	if (changed)
		reload_ifaces(1);
#endif
}


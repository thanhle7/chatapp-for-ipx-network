/*  Helper routines for accessing IPX services from Turbo C++ */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <string.h>
#include <ipx.h>
#include <bios.h>
const	char	test[8] = {'Ä',0x8,'\\',0x8,'³',0x8,'/',0x8};

typedef void far (*far_pf)();
typedef unsigned char str_48[48];

char	rdata[600], sdata[600], chain[5];
unsigned int	socket = 0x5555, wcount;
unsigned char	i, con_num, con_count, net_num[4], node_add[6], imme_add[6];

far_pf	ipx_spx;
ECB	Recb, Secb;
struct  IPXHEADER Ripx, Sipx;
str_48	user_id, id_list[100];

//=====================================

int     ipx_installed(void)
	{
	union  REGS     regs;
	struct SREGS    sregs;

	regs.x.ax = 0x7a00;
	int86x(0x2f, &regs, &regs, &sregs);
	if (regs.h.al != 0xff) return -1;	// Not installed

	ipx_spx = (far_pf)MK_FP(sregs.es, regs.x.di);
	_BX = 0x0010;
	_AL = 0x00;
	ipx_spx();
	return (_AL == 0x00? 0:1);		// wrong network
	}
//=====================================
int     open_socket(unsigned int socket)
	{
	if (ipx_installed() < 1) return -1;
	_DX = socket;
	_BX = 0x0000;
	_AL = 0xFF;
	ipx_spx();
	_AH = 0;
	return _AX;
	}
//=====================================
void    close_socket(unsigned int socket)
	{
	if (ipx_installed() < 1) return;
	_BX = 0x0001;
	_DX = socket;
	ipx_spx();
	}
//=====================================
void    ipx_listen_for_packet(struct ECB *ecb_ptr)
	{
	_ES = FP_SEG( (void far *) ecb_ptr);
	_SI = FP_OFF( (void far *) ecb_ptr);
	_BX = 0x0004;
	ipx_spx();
	}
//=====================================
void    ipx_send_packet(struct ECB *ecb_ptr)
	{
	_ES = FP_SEG( (void far *) ecb_ptr);
	_SI = FP_OFF( (void far *) ecb_ptr);
	_BX = 0x0003;
	ipx_spx();
	}

//=====================================
int     get_internet_address(unsigned char connection_number,
			    unsigned char *network_number,
			    unsigned char *physical_node)
	{
	union REGS      regs;
	struct SREGS    sregs;

	struct  {
		unsigned int    len;
		unsigned char   buffer_type;
		unsigned char   connection_number;
		} request_buffer;

	struct  {
		unsigned int    len;
		unsigned char   network_number [4];
		unsigned char   physical_node  [6];
		unsigned int    server_socket;
		} reply_buffer;

	regs.h.ah = 0xe3;
	request_buffer.len = 2;
	request_buffer.buffer_type = 0x13;
	request_buffer.connection_number = connection_number;


	reply_buffer.len = 12;

	regs.x.si = FP_OFF( (void far *) &request_buffer);
	sregs.ds  = FP_SEG( (void far *) &request_buffer);
	regs.x.di = FP_OFF( (void far *) &reply_buffer);
	sregs.es  = FP_SEG( (void far *) &reply_buffer);
	int86x(0x21, &regs, &regs, &sregs);

	memcpy(network_number, reply_buffer.network_number, 4);
	memcpy(physical_node,  reply_buffer.physical_node,  6);
	regs.h.ah = 0;
	return regs.x.ax;
	}
//=====================================
unsigned int    get_1st_connection_num (char *who)
	{
	union REGS      regs;
	struct SREGS    sregs;

	struct  {
		unsigned int    len;
		unsigned char   buffer_type;
		unsigned int    object_type;
		unsigned char   name_len;
		unsigned char   name [47];
		} request_buffer;

	struct  {
		unsigned int    len;
		unsigned char   number_connections;
		unsigned char   connection_num [100];
		} reply_buffer;

	regs.h.ah = 0xe3;

	request_buffer.len = 51;
	request_buffer.buffer_type = 0x15;
	request_buffer.object_type = 0x0100;
	request_buffer.name_len    = (unsigned char) strlen(who);
	strcpy(request_buffer.name, who);

	reply_buffer.len = 101;

	regs.x.si = FP_OFF( (void far *) &request_buffer);
	sregs.ds  = FP_SEG( (void far *) &request_buffer);
	regs.x.di = FP_OFF( (void far *) &reply_buffer);
	sregs.es  = FP_SEG( (void far *) &reply_buffer);

	int86x(0x21, &regs, &regs, &sregs);

	if (regs.h.al != 0) return 0;
	if (reply_buffer.number_connections == 0) return 0;

	regs.h.ah = 0;
	regs.h.al = reply_buffer.connection_num[0];
	return regs.x.ax;
	}
//=====================================
unsigned char get_connection_number(void)
	{
	_AH = 0xDC;
	geninterrupt(0x21);
	return _AL;
	}
//=====================================
void    get_user_id(unsigned char connection_number,
		    unsigned char *user_id)
	{
	union  REGS     regs;
	struct SREGS    sregs;

	struct  {
		unsigned int    len;
		unsigned char   buffer_type;
		unsigned char   connection_number;
		} request_buffer;

	struct  {
		unsigned int    len;
		unsigned char   object_id[4];
		unsigned char   object_type[2];
		char            object_name[48];
		char            login_time[7];
		} reply_buffer;

	regs.h.ah = 0xe3;
	request_buffer.len = 2;
	request_buffer.buffer_type = 0x16;
	request_buffer.connection_number = connection_number;

	reply_buffer.len = 61;

	regs.x.si = FP_OFF( (void far *) &request_buffer);
	sregs.ds  = FP_SEG( (void far *) &request_buffer);
	regs.x.di = FP_OFF( (void far *) &reply_buffer);
	sregs.es  = FP_SEG( (void far *) &reply_buffer);
	int86x(0x21, &regs, &regs, &sregs);

	strncpy(user_id, reply_buffer.object_name, 48);
	}
//=====================================
int     ipx_cancel_event(struct ECB *ecb_ptr)
	{
	_ES = FP_SEG( (void far *) ecb_ptr);
	_SI = FP_OFF( (void far *) ecb_ptr);
	_BX = 0x0006;
	ipx_spx();
	_AH = 0;
	return _AX;
	}
//=====================================
int     get_local_target(unsigned char *dest_network,
			 unsigned char *dest_node,
			 unsigned int   dest_socket,
			 unsigned char *bridge_address)
	{
	unsigned int    temp_ax;

	struct  {
		unsigned char   network_number [4];
		unsigned char   physical_node  [6];
		unsigned int    socket;
		} request_buffer;

	struct  {
		unsigned char   local_target [6];
		} reply_buffer;

	memcpy(request_buffer.network_number, dest_network, 4);
	memcpy(request_buffer.physical_node, dest_node, 6);
	request_buffer.socket = dest_socket;

	_ES = FP_SEG( (void far *) &request_buffer);
	_SI = FP_OFF( (void far *) &request_buffer);
	_DI = FP_OFF( (void far *) &reply_buffer);
	_BX = 0x0002;
	ipx_spx();
	_AH = 0;
	temp_ax = _AX;
	memcpy(bridge_address, reply_buffer.local_target, 6);
	return temp_ax;
	}
//=====================================
void    let_ipx_breath(void)
	{
	_BX = 0x000A;
	ipx_spx();
	}
//=====================================
unsigned char get_id_list()
{
	unsigned char gi;
	for (gi = 1 ; gi < 100 ; gi++)
	{
	    get_user_id(gi, id_list[gi]);
	    if (id_list[gi][0] == '\0') break;
	}
	return gi;
}
//=====================================

void main(void)
{
    clrscr();
    if (ipx_installed() > 0)
    {
      con_count = get_id_list();
      for (i = 1 ; i < con_count ; i++)
	  printf("Connection No: %2d   Obj Name: %s \n", i, id_list[i]);

      con_num = get_1st_connection_num ("AZ");
      get_user_id(con_num, user_id);
      get_internet_address(con_num, net_num, node_add);

      printf("\nConnection number: %2d  User Id: %s\n", con_num, user_id);
      printf("\nNetwork add: %12X  Network No: %8X\n", node_add, net_num);


      if (open_socket(socket)<1) exit(1);		// Receiving
      if (open_socket(socket+0x1010)<1) exit(1);	// Sending
      do
      {
	 clrscr();
	 printf("IPX Netware Protocol ... ");
	 printf("\n 1 .. Send\n 2 .. Receive\n");
	 printf("Enter your choice ? ");

	 gets(chain);
	 i = atoi(chain);
	 if (i == 0)
		{
		      printf("Normal shutdown . . .");
		      break;
		};
	 switch (i)
	   {
	      case 1:
		      // Send a message
		      get_local_target(net_num, node_add, socket, imme_add);
		      memset(&Secb, 0, sizeof(struct ECB));
		      memset(&Sipx, 0, sizeof(struct IPXHEADER));
		      clreol(); printf("Message to be sent:");
		      gets(sdata);

		      memcpy(Sipx.dest_network_number, net_num, 4);
		      memcpy(Sipx.dest_network_node, node_add, 6);
		      memcpy(Secb.immediate_address, imme_add, 6);
		      Secb.link_address = NULL;
		      Secb.event_service_routine = NULL;
		      Sipx.dest_network_socket = socket;
		      Sipx.packet_type  = 4;
		      Secb.packet_count = 2;
		      Secb.socket_number= socket+0x1010;

		      // first for ipx header buffer
		      Secb.packet[0].address = MK_FP(FP_SEG(&Sipx), FP_OFF(&Sipx));
		      Secb.packet[0].length  = sizeof(struct IPXHEADER);

		      // second for message buffer
		      Secb.packet[1].address = MK_FP(FP_SEG(sdata), FP_OFF(sdata));
		      Secb.packet[1].length  = 516;
		      ipx_send_packet(&Secb);

		      printf("Wait ");
		      for (i=0 ; i<225 ; i++)
		      {
		      wcount = 1;
		      putchar(test[i % 8]);
		      while ((Secb.in_use)&&(wcount++<2000)) let_ipx_breath();
		      if (Secb.in_use == 0) break;
		      };

		      if (Secb.in_use || Secb.completion_code)
			     printf("\rSending failed . . .");
		      else
			     printf("\rSending successed . . .");

		      break;
	      case 2:
		      // Receive a message
		      memset(&Recb, 0, sizeof(struct ECB));
		      memset(&Ripx, 0, sizeof(struct IPXHEADER));
		      Recb.event_service_routine = NULL;
		      Recb.socket_number= socket;
		      Sipx.packet_type  = 4;
		      Recb.packet_count = 2;

		      // first for ipx header buffer
		      Recb.packet[0].address = MK_FP(FP_SEG(&Ripx), FP_OFF(&Ripx));
		      Recb.packet[0].length  = sizeof(struct IPXHEADER);
		      // second for message buffer
		      Recb.packet[1].address = MK_FP(FP_SEG(rdata), FP_OFF(rdata));
		      Recb.packet[1].length  = 516;
		      ipx_listen_for_packet(&Recb);

		      printf("Wait ");
		      for (i=0 ; i<250 ; i++)
		      {
		      wcount = 1;
		      putchar(test[i % 8]);
		      while ((Recb.in_use)&&(wcount++<2000)) let_ipx_breath();
		      if (Recb.in_use == 0) break;
		      };

		      if (Recb.in_use || Recb.completion_code)
			    printf("\rReceiving failed . . .");
		      else
			    printf("\rReceiving message: %s through: %2d times", rdata, Ripx.transport_control);
		      break;
	      default:printf("No such function ...");
	   }
	 bioskey(0);
      } while (1);
      close_socket(socket);
      close_socket(socket+0x1010);
    };
}
//=====================================
/*  Helper routines for accessing IPX services from Turbo C++ */

#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>

const	char *type_list[15] = {"Who's ?","User","U_Group","PQueue",
	"Server","JServer","Gateway","Pserver","AQueue",
	"Aserver","JQueue","Adminis","RBridge",
	"Reserve","W_card"};
typedef void far (*far_pf)();
typedef unsigned char str_50[50];

typedef struct iph
    {
    unsigned int    checksum;
    unsigned int    length;
    unsigned char   transport_control;
    unsigned char   packet_type;
    unsigned char   dest_network_number [4];
    unsigned char   dest_network_node   [6];
    unsigned int    dest_network_socket;
    unsigned char   source_network_number [4];
    unsigned char   source_network_node   [6];
    unsigned int    source_network_socket;
    } IPXHEADER;

typedef struct ecb_
    {
    void far        *link_address;
    void far	    (*event_service_routine)(void);
    unsigned char   in_use;
    unsigned char   completion_code;
    unsigned int    socket_number;
    unsigned int    connection_id;      /* returned by Listen */
    unsigned int    rest_of_workspace;
    unsigned char   driver_workspace  [12];
    unsigned char   immediate_address [ 6];
    unsigned int    packet_count;
    struct {
	void far    *address;	// first for ipx header buffer
	unsigned int length;	// second for message buffer
	} packet [2];
    } ECB;

unsigned int	socket = 0x7777, wcount;
unsigned char	bpos, cpos, k, slaver_count;
str_50	 rdata, slaver[100];
char	 chain[5], sdata[5];

far_pf	ipx_spx;
ECB	Recb, Secb;
IPXHEADER Ripx, Sipx;

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
void    ipx_listen_for_packet(ECB *ecb_ptr)
	{
	_ES = FP_SEG( (void far *) ecb_ptr);
	_SI = FP_OFF( (void far *) ecb_ptr);
	_BX = 0x0004;
	ipx_spx();
	}
//=====================================
void    ipx_send_packet(ECB *ecb_ptr)
	{
	_ES = FP_SEG( (void far *) ecb_ptr);
	_SI = FP_OFF( (void far *) ecb_ptr);
	_BX = 0x0003;
	ipx_spx();
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

	memcpy(user_id, reply_buffer.object_name, 48);
	}
//=====================================
int     ipx_cancel_event(ECB *ecb_ptr)
	{
	_ES = FP_SEG( (void far *) ecb_ptr);
	_SI = FP_OFF( (void far *) ecb_ptr);
	_BX = 0x0006;
	ipx_spx();
	_AH = 0;
	return _AX;
	}
//=====================================
void    let_ipx_breath(void)
	{
	_BX = 0x000A;
	ipx_spx();
	}
//=====================================
void	instruct(unsigned char pt, unsigned char command)
{
    unsigned char ii;
    sdata[0] = 25;			// Master code to slaver
    sdata[1] = 25;
    sdata[2] = command;

    for (ii=0; ii < 4; ii++)					// IPX
	Sipx.dest_network_number[ii] = slaver[pt][ii];

    for (ii=0; ii < 6; ii++)
	Sipx.dest_network_node[ii] = slaver[pt][4+ii];

    for (ii=0; ii < 6; ii++)
	Secb.immediate_address[ii] = slaver[pt][10+ii];

    Sipx.dest_network_socket = 0x5555;
    Sipx.packet_type  = 4;

    Secb.event_service_routine = NULL;				// ECB
    Secb.packet_count = 2;
    Secb.socket_number= socket;

    // first for ipx header buffer
    Secb.packet[0].address = MK_FP(FP_SEG(&Sipx), FP_OFF(&Sipx));
    Secb.packet[0].length  = sizeof(IPXHEADER);

    // second for message buffer
    Secb.packet[1].address = MK_FP(FP_SEG(sdata), FP_OFF(sdata));
    Secb.packet[1].length  = 5;
    ipx_send_packet(&Secb);
};
//=====================================
void	hearing()
{
/*
       1  byte         Slaver code     ==   (24)
       4  bytes        Network number
       6  bytes        Node address
       1  byte         Object type
       38 bytes        Object name     ==> 50 bytes for dataportion.
*/
  unsigned char oldx, oldy, hi, hj, hk;
  oldx = wherex();
  oldy = wherey();
  if (Recb.in_use == 0)
     {
	// Get information from receiving.
	if (Recb.completion_code == 0)
	   {
	      if (rdata[0] == 24)
		 {
		   // One from slaver
		   for (hi=0; hi<slaver_count; )
			if (memcmp(slaver[hi], rdata+1, 10) != 0) hi++;
			else   break;	// already installed in memory.

		   for (hj=0; hj<39; hj++)
		       slaver[hi][16+hj] = rdata[hj+11]; //type & slavername

		   // get immediate address either alreay or not
		   for (hj=0; hj<6; hj++)
		       slaver[hi][10+hj] = Recb.immediate_address[hj];

		   if (hi == slaver_count)
		      {
			slaver_count += 1;

			// add to list of slaver. get its address
			for (hj=0; hj<10; hj++)
			    slaver[hi][hj] = rdata[hj+1]; //network & address
		      };
		   instruct(hi, 4);		// Resident yourself.

		   for (hj=0; hj<slaver_count; hj++)
		       {
			  gotoxy(7, hj+3);
			  printf("¯%-2d.   #%1X ", hj, slaver[hj][3]);
			  for (hk=4; hk<10; hk++)
			      printf("%2X", slaver[hj][hk]);
			  printf("    %-7s  ", type_list[slaver[hj][16]]);
			  printf("%-38s",slaver[hj]+17);
		       };
		 };
	   };

	// Try another Receiving.
	memset(&Recb, 0, sizeof(ECB));
	memset(&Ripx, 0, sizeof(IPXHEADER));

	Recb.event_service_routine = NULL;
	Recb.socket_number= socket;
	Sipx.packet_type  = 4;
	Recb.packet_count = 2;

	// first for ipx header buffer
	Recb.packet[0].address = MK_FP(FP_SEG(&Ripx), FP_OFF(&Ripx));
	Recb.packet[0].length  = sizeof(IPXHEADER);

	// second for message buffer
	Recb.packet[1].address = MK_FP(FP_SEG(rdata), FP_OFF(rdata));
	Recb.packet[1].length  = 50;
	ipx_listen_for_packet(&Recb);
     };
  let_ipx_breath();
  gotoxy(oldx, oldy);
};

//=====================================
void main(void)
{
    memset(&Secb, 0, sizeof(ECB));
    memset(&Sipx, 0, sizeof(IPXHEADER));
    Recb.in_use = 0; Recb.completion_code = 1; slaver_count = 0;
    clrscr();
    if (ipx_installed() > 0)
    {
      printf("              --- Network Management System by IPX Protocol ---");
      gotoxy(4, 23);
      printf("Instruction:  F1:  OK   *   F2:  Login   *   F3:  Logout   *   F4:  Quit");
      gotoxy(5, 24);
      printf("        Then press the number of slaver PC to execute command.");
      do ; while(open_socket(socket)<1);
      do
      {

	 while (!kbhit()) hearing();
	 k = getch();
	 if (k == 0) k = getch();
	 if (k>62 || k<59) k = 59;
	 k -= 59;
	 if (k == 3) break;

	 if (Secb.in_use != 0)
	    {
		printf("I'm busy now !");
		continue;
	    }
	 bpos  = getch();
	 bpos -= 48;
	 if (bpos >= slaver_count) continue;
	 instruct(bpos, k);
      } while(1);
      close_socket(socket);
    };
};
//=====================================
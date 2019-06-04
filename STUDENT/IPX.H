 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
; *   Remark Infos for this source code  ÄÄ  AZ-Group.                      *
; *   by:         Le Ngoc Thanh.                                            *
; *   from:       Informatics Department, Economics University. HCM City.   *
; *   Purpose:    IPX protocol programming in C++.                          *
; *   Product ID: EMP.1995                                                  *
; * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Network Object type.
const   char *type_list[15] = {"Who's ?","User","U_Group","PQueue",
        "Server","JServer","Gateway","Pserver","AQueue",
        "Aserver","JQueue","Adminis","RBridge",
        "Reserve","W_card"};

// a far function type to maintain ipx services function.
typedef void far (*far_pf)();

typedef struct ipx_
    {
    unsigned int    checksum;
    unsigned int    length;
    unsigned char   transport_control;
    unsigned char   packet_type;                // =4 for IPX packet
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
    void far        (*event_service_routine)(void);
    unsigned char   in_use;
    unsigned char   completion_code;
    unsigned int    socket_number;
    unsigned int    connection_id;      /* returned by Listen */
    unsigned int    rest_of_workspace;
    unsigned char   driver_workspace  [12];
    unsigned char   immediate_address [ 6];
    unsigned int    packet_count;
    struct {
        void far    *address;   // first for ipx header buffer
        unsigned int length;    // second for message buffer
        } packet [2];
    } ECB;

far_pf  ipx_spx;

//=====================================
// IPX protocol checking

int     ipx_installed(void)
        {
        union  REGS     regs;
        struct SREGS    sregs;

        regs.x.ax = 0x7a00;
        int86x(0x2f, &regs, &regs, &sregs);
        if (regs.h.al != 0xff) return -1;       // Not installed

        ipx_spx = (far_pf)MK_FP(sregs.es, regs.x.di);
        _BX = 0x0010;
        _AL = 0x00;
        ipx_spx();
        return (_AL == 0x00? 0:1);              // wrong network
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
/* an example of packet sending:
    Sipx.dest_network_number = ?; // IPX
    Sipx.dest_network_node   = ?;
    Sipx.dest_network_socket = ?;
    Sipx.packet_type         = 4;

    Secb.immediate_address   = ?; // optional
    Secb.event_service_routine = NULL;          // ECB
    Secb.packet_count = 2;
    Secb.socket_number= ?;

    // first for ipx header buffer
    Secb.packet[0].address = MK_FP(FP_SEG(&Sipx), FP_OFF(&Sipx));
    Secb.packet[0].length  = sizeof(IPXHEADER);

    // second for message buffer, sdata is buffer to tranfer
    Secb.packet[1].address = MK_FP(FP_SEG(sdata), FP_OFF(sdata));
    Secb.packet[1].length  = sizeof(sdata);
    ipx_send_packet(&Secb);
*/
//=====================================
/* an example of packet receiving:
        memset(&Recb, 0, sizeof(ECB));
        memset(&Ripx, 0, sizeof(IPXHEADER));

        Recb.event_service_routine = NULL;
        Recb.socket_number= ?;
        Sipx.packet_type  = 4;
        Recb.packet_count = 2;

        // first for ipx header buffer
        Recb.packet[0].address = MK_FP(FP_SEG(&Ripx), FP_OFF(&Ripx));
        Recb.packet[0].length  = sizeof(IPXHEADER);

        // second for message buffer, rdata is buffer to store data
        Recb.packet[1].address = MK_FP(FP_SEG(rdata), FP_OFF(rdata));
        Recb.packet[1].length  = sizeof(rdata);
        ipx_listen_for_packet(&Recb);
*/

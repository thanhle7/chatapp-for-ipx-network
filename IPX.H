
/*
 *  IPX.H -- see 'ipx.cpp'.
 *
*/

struct IPXHEADER
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
    };

struct ECB
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
    };

/*
int     ipx_spx_installed(void);
int     ipx_cancel_event(struct ECB *ecb_ptr);
void    close_socket(unsigned int socket);
int     open_socket(unsigned int socket);
int     get_local_target(unsigned char *dest_network,
			 unsigned char *dest_node,
			 unsigned int   dest_socket,
			 unsigned char *bridge_address);
void    let_ipx_breath(void);
void    ipx_listen_for_packet(struct ECB *ecb_ptr);
void    ipx_send_packet(struct ECB *ecb_ptr);
int     get_internet_address(unsigned char connection_number,
			    unsigned char *network_number,
			    unsigned char *physical_node);
unsigned int    get_1st_connection_num (char *who);
unsigned char get_connection_number(void);
void    get_user_id(unsigned char connection_number,
		    unsigned char *user_id);


*/
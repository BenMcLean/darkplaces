/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2003 Forest Hale

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef NET_H
#define NET_H

#include "lhnet.h"

#define NET_HEADERSIZE		(2 * sizeof(unsigned int))

// NetHeader flags
#define NETFLAG_LENGTH_MASK	0x0000ffff
#define NETFLAG_DATA		0x00010000
#define NETFLAG_ACK			0x00020000
#define NETFLAG_NAK			0x00040000
#define NETFLAG_EOM			0x00080000
#define NETFLAG_UNRELIABLE	0x00100000
#define NETFLAG_CTL			0x80000000


#define NET_PROTOCOL_VERSION	3

// This is the network info/connection protocol.  It is used to find Quake
// servers, get info about them, and connect to them.  Once connected, the
// Quake game protocol (documented elsewhere) is used.
//
//
// General notes:
//	game_name is currently always "QUAKE", but is there so this same protocol
//		can be used for future games as well; can you say Quake2?
//
// CCREQ_CONNECT
//		string	game_name				"QUAKE"
//		byte	net_protocol_version	NET_PROTOCOL_VERSION
//
// CCREQ_SERVER_INFO
//		string	game_name				"QUAKE"
//		byte	net_protocol_version	NET_PROTOCOL_VERSION
//
// CCREQ_PLAYER_INFO
//		byte	player_number
//
// CCREQ_RULE_INFO
//		string	rule
//
//
//
// CCREP_ACCEPT
//		long	port
//
// CCREP_REJECT
//		string	reason
//
// CCREP_SERVER_INFO
//		string	server_address
//		string	host_name
//		string	level_name
//		byte	current_players
//		byte	max_players
//		byte	protocol_version	NET_PROTOCOL_VERSION
//
// CCREP_PLAYER_INFO
//		byte	player_number
//		string	name
//		long	colors
//		long	frags
//		long	connect_time
//		string	address
//
// CCREP_RULE_INFO
//		string	rule
//		string	value

//	note:
//		There are two address forms used above.  The short form is just a
//		port number.  The address that goes along with the port is defined as
//		"whatever address you receive this reponse from".  This lets us use
//		the host OS to solve the problem of multiple host addresses (possibly
//		with no routing between them); the host will use the right address
//		when we reply to the inbound connection request.  The long from is
//		a full address and port in a string.  It is used for returning the
//		address of a server that is not running locally.

#define CCREQ_CONNECT		0x01
#define CCREQ_SERVER_INFO	0x02
#define CCREQ_PLAYER_INFO	0x03
#define CCREQ_RULE_INFO		0x04

#define CCREP_ACCEPT		0x81
#define CCREP_REJECT		0x82
#define CCREP_SERVER_INFO	0x83
#define CCREP_PLAYER_INFO	0x84
#define CCREP_RULE_INFO		0x85

typedef struct netconn_s
{
	struct netconn_s *next;

	lhnetsocket_t *mysocket;
	lhnetaddress_t peeraddress;

	// this is mostly identical to qsocket_t from quake

	// if this time is reached, kick off peer
	double connecttime;
	double timeout;
	double lastMessageTime;
	double lastSendTime;

	qboolean canSend;
	qboolean sendNext;

	unsigned int ackSequence;
	unsigned int sendSequence;
	unsigned int unreliableSendSequence;
	int sendMessageLength;
	qbyte sendMessage[NET_MAXMESSAGE];

	unsigned int receiveSequence;
	unsigned int unreliableReceiveSequence;
	int receiveMessageLength;
	qbyte receiveMessage[NET_MAXMESSAGE];

	char address[128];
} netconn_t;

extern netconn_t *netconn_list;
extern mempool_t *netconn_mempool;

extern cvar_t hostname;
extern cvar_t developer_networking;
extern char playername[];
extern int playercolor;

#define SERVERLIST_TOTALSIZE		2048
#define SERVERLIST_VIEWLISTSIZE		128
#define SERVERLIST_ANDMASKCOUNT		5
#define SERVERLIST_ORMASKCOUNT		5

typedef enum
{
	// SLMO_CONTAINS is the default for strings
	// SLMO_GREATEREQUAL is the default for numbers (also used when OP == CONTAINS or NOTCONTAINS
	SLMO_CONTAINS,
	SLMO_NOTCONTAIN,

	SLMO_LESSEQUAL,
	SLMO_LESS,
	SLMO_EQUAL,
	SLMO_GREATER,
	SLMO_GREATEREQUAL,
	SLMO_NOTEQUAL
} serverlist_maskop_t;

// struct with all fields that you can search for or sort by
typedef struct
{
	// address for connecting
	char cname[128];
	// ping time for sorting servers
	int ping;
	// name of the game
	char game[32];
	// name of the mod
	char mod[32];
	// name of the map
	char map[32];
	// name of the session
	char name[128];
	// max client number
	int maxplayers;
	// number of currently connected players
	int numplayers;
	// protocol version
	int protocol;
} serverlist_info_t;

typedef enum
{
	SLIF_CNAME,
	SLIF_PING,
	SLIF_GAME,
	SLIF_MOD,
	SLIF_MAP,
	SLIF_NAME,
	SLIF_MAXPLAYERS,
	SLIF_NUMPLAYERS,
	SLIF_PROTOCOL,
	SLIF_COUNT
} serverlist_infofield_t;

typedef enum
{
	SQS_NONE = 0,
	SQS_QUERYING,
	SQS_QUERIED,
	SQS_TIMEDOUT
} serverlist_query_state;

typedef struct
{
	// used to determine whether this entry should be included into the final view
	serverlist_query_state query;
	// used to count the number of times the host has tried to query this server already
	unsigned querycounter;
	// used to calculate ping when update comes in
	double querytime;

	serverlist_info_t info;

	// legacy stuff
	char line1[128];
	char line2[128];
} serverlist_entry_t;

typedef struct
{
	qboolean			active;
	serverlist_maskop_t  tests[SLIF_COUNT];
	serverlist_info_t info;
} serverlist_mask_t;

extern serverlist_mask_t serverlist_andmasks[SERVERLIST_ANDMASKCOUNT];
extern serverlist_mask_t serverlist_ormasks[SERVERLIST_ORMASKCOUNT];

extern serverlist_infofield_t serverlist_sortbyfield;
extern qboolean serverlist_sortdescending;

extern int serverlist_viewcount;
extern serverlist_entry_t *serverlist_viewlist[SERVERLIST_VIEWLISTSIZE];

extern int serverlist_cachecount;

extern qboolean serverlist_consoleoutput;

#if !defined(_WIN32 ) && !defined (__linux__) && !defined (__sun__)
#ifndef htonl
extern unsigned long htonl (unsigned long hostlong);
#endif
#ifndef htons
extern unsigned short htons (unsigned short hostshort);
#endif
#ifndef ntohl
extern unsigned long ntohl (unsigned long netlong);
#endif
#ifndef ntohs
extern unsigned short ntohs (unsigned short netshort);
#endif
#endif

//============================================================================
//
// public network functions
//
//============================================================================

extern double masterquerytime;
extern int masterquerycount;
extern int masterreplycount;
extern int serverquerycount;
extern int serverreplycount;

extern sizebuf_t net_message;

extern cvar_t cl_netlocalping;

int NetConn_SendReliableMessage(netconn_t *conn, sizebuf_t *data);
//void NetConn_SendMessageNext(netconn_t *conn);
//void NetConn_ReSendMessage(netconn_t *conn);
qboolean NetConn_CanSendMessage(netconn_t *conn);
int NetConn_SendUnreliableMessage(netconn_t *conn, sizebuf_t *data);
void NetConn_CloseClientPorts(void);
void NetConn_OpenClientPorts(void);
void NetConn_CloseServerPorts(void);
void NetConn_OpenServerPorts(int opennetports);
lhnetsocket_t *NetConn_ChooseClientSocketForAddress(lhnetaddress_t *address);
lhnetsocket_t *NetConn_ChooseServerSocketForAddress(lhnetaddress_t *address);
void NetConn_Init(void);
void NetConn_Shutdown(void);
netconn_t *NetConn_Open(lhnetsocket_t *mysocket, lhnetaddress_t *peeraddress);
void NetConn_Close(netconn_t *conn);
void NetConn_Listen(qboolean state);
int NetConn_IsLocalGame(void);
//int NetConn_ReceivedMessage(netconn_t *conn, qbyte *data, int length);
//int NetConn_ClientParsePacket(lhnetsocket_t *mysocket, qbyte *data, int length, lhnetaddress_t *peeraddress);
//int NetConn_ServerParsePacket(lhnetsocket_t *mysocket, qbyte *data, int length, lhnetaddress_t *peeraddress);
void NetConn_ClientFrame(void);
void NetConn_ServerFrame(void);
void NetConn_QueryMasters(void);
void NetConn_Heartbeat(int priority);
void NetConn_QueryQueueFrame(void);
int NetConn_SendToAll(sizebuf_t *data, double blocktime);
void Net_Stats_f(void);
void Net_Slist_f(void);

// ServerList interface (public)
// manually refresh the view set, do this after having changed the mask or any other flag
void ServerList_RebuildViewList(void);
void ServerList_ResetMasks(void);
void ServerList_QueryList(void);

#endif


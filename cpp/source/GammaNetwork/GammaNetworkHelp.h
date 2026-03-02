#ifndef _GAMMA_NETWORK_HELP_H_
#define _GAMMA_NETWORK_HELP_H_

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/TSmartPtr.h"
#include "GammaNetwork/CAddress.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#define MAX_SEND_BLOCK_COUNT		4096             // 网络发送Buffer单元最大个数
#define MAX_BLOCK_BUFFER_COUNT		65536            // 用于缓冲的Buffer单元最大个数
#define MAX_SENDBUFF_SIZE			64*1024*1024	 // 发送缓冲的最大bytes
#define MAX_UDP_SIZE				1400

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <MSWSock.h>

	#define SOCKET uint32
	#define GNWGetLastError		WSAGetLastError

	#define NE_EADDRNOTAVAIL	WSAEADDRNOTAVAIL
	#define NE_EWOULDBLOCK		WSAEWOULDBLOCK
	#define NE_EINPROGRESS		WSAEINPROGRESS
	#define NE_EADDRINUSE		WSAEADDRINUSE
	#define NE_ENETUNREACH		WSAENETUNREACH
	#define NE_ENETRESET		WSAENETRESET
	#define NE_ECONNRESET		WSAECONNRESET
	#define NE_ENOBUFS			WSAENOBUFS
	#define NE_ETIMEDOUT		WSAETIMEDOUT
	#define NE_ECONNREFUSED		WSAECONNREFUSED
	#define NE_EHOSTUNREACH		WSAEHOSTUNREACH
	#define NE_ECONNABORTED		WSAECONNABORTED
	#define NE_ENOTCONN			WSAENOTCONN
	#define NE_EOPERABORTED		WSA_OPERATION_ABORTED

#ifdef FORCE_SELECT_MODE
	enum ENetworkEvent
    {
        eNE_Listen			= ( 1 << 0 ),
        eNE_Connecting		= ( 1 << 1 ),
        eNE_ConnectedRead	= ( 1 << 0 ),
        eNE_ConnectedWrite	= ( 1 << 1 ),
        eNE_Error			= ( 1 << 2 ),
	};
#else
	enum ENetworkEvent
	{
		eNE_Listen           = FD_ACCEPT,
		eNE_Connecting       = FD_CONNECT,
		eNE_ConnectedRead    = FD_READ,
		eNE_ConnectedWrite   = FD_WRITE,
		eNE_Error            = FD_CLOSE,
	};
#endif

	typedef WSABUF  SDataBuffer;

#else

	/*
	0 --            Success
	1 EPERM        +Operation not permitted
	2 ENOENT       +No such file or directory
	3 ESRCH        +No such process
	4 EINTR        +Interrupted system call
	5 EIO          +Input/output error
	6 ENXIO        +No such device or address
	7 E2BIG        +Argument list too long
	8 ENOEXEC      +Exec format error
	9 EBADF        +Bad file descriptor
	10 ECHILD       +No child processes
	11 EAGAIN       +Resource temporarily unavailable
	12 ENOMEM       +Cannot allocate memory
	13 EACCES       +Permission denied
	14 EFAULT       +Bad address
	15 ENOTBLK       Block device required
	16 EBUSY        +Device or resource busy
	17 EEXIST       +File exists
	18 EXDEV        +Invalid cross-device link
	19 ENODEV       +No such device
	20 ENOTDIR      +Not a directory
	21 EISDIR       +Is a directory
	22 EINVAL       +Invalid argument
	23 ENFILE       +Too many open files in system
	24 EMFILE       +Too many open files
	25 ENOTTY       +Inappropriate ioctl for device
	26 ETXTBSY       Text file busy
	27 EFBIG        +File too large
	28 ENOSPC       +No space left on device
	29 ESPIPE       +Illegal seek
	30 EROFS        +Read-only file system
	31 EMLINK       +Too many links
	32 EPIPE        +Broken pipe
	33 EDOM         +Numerical argument out of domain
	34 ERANGE       +Numerical result out of range
	35 EDEADLK      +Resource deadlock avoided
	36 ENAMETOOLONG +File name too long
	37 ENOLCK       +No locks available
	38 ENOSYS       +Function not implemented
	39 ENOTEMPTY    +Directory not empty
	40 ELOOP         Too many levels of symbolic links
	42 ENOMSG        No message of desired type
	43 EIDRM         Identifier removed
	44 ECHRNG        Channel number out of range
	45 EL2NSYNC      Level 2 not synchronized
	46 EL3HLT        Level 3 halted
	47 EL3RST        Level 3 reset
	48 ELNRNG        Link number out of range
	49 EUNATCH       Protocol driver not attached
	50 ENOCSI        No CSI structure available
	51 EL2HLT        Level 2 halted
	52 EBADE         Invalid exchange
	53 EBADR         Invalid request descriptor
	54 EXFULL        Exchange full
	55 ENOANO        No anode
	56 EBADRQC       Invalid request code
	57 EBADSLT       Invalid slot
	59 EBFONT        Bad font file format
	60 ENOSTR        Device not a stream
	61 ENODATA       No data available
	62 ETIME         Timer expired
	63 ENOSR         Out of streams resources
	64 ENONET        Machine is not on the network
	65 ENOPKG        Package not installed
	66 EREMOTE       Object is remote
	67 ENOLINK       Link has been severed
	68 EADV          Advertise error
	69 ESRMNT        Srmount error
	70 ECOMM         Communication error on send
	71 EPROTO        Protocol error
	72 EMULTIHOP     Multihop attempted
	73 EDOTDOT       RFS specific error
	74 EBADMSG      +Bad message
	75 EOVERFLOW     Value too large for defined data type
	76 ENOTUNIQ      Name not unique on network
	77 EBADFD        File descriptor in bad state
	78 EREMCHG       Remote address changed
	79 ELIBACC       Can not access a needed shared library
	80 ELIBBAD       Accessing a corrupted shared library
	81 ELIBSCN       .lib section in a.out corrupted
	82 ELIBMAX       Attempting to link in too many shared libraries
	83 ELIBEXEC      Cannot exec a shared library directly
	84 EILSEQ        Invalid or incomplete multibyte or wide character
	85 ERESTART      Interrupted system call should be restarted
	86 ESTRPIPE      Streams pipe error
	87 EUSERS        Too many users
	88 ENOTSOCK      Socket operation on non-socket
	89 EDESTADDRREQ  Destination address required
	90 EMSGSIZE     +Message too long
	91 EPROTOTYPE    Protocol wrong type for socket
	92 ENOPROTOOPT   Protocol not available
	93 EPROTONOSUPPORT Protocol not supported
	94 ESOCKTNOSUPPORT Socket type not supported
	95 EOPNOTSUPP    Operation not supported
	96 EPFNOSUPPORT  Protocol family not supported
	97 EAFNOSUPPORT  Address family not supported by protocol
	98 EADDRINUSE    Address already in use
	99 EADDRNOTAVAIL Cannot assign requested address
	100 ENETDOWN      Network is down
	101 ENETUNREACH   Network is unreachable
	102 ENETRESET     Network dropped connection on reset
	103 ECONNABORTED  Software caused connection abort
	104 ECONNRESET    Connection reset by peer
	105 ENOBUFS       No buffer space available
	106 EISCONN       Transport endpoint is already connected
	107 ENOTCONN      Transport endpoint is not connected
	108 ESHUTDOWN     Cannot send after transport endpoint shutdown
	109 ETOOMANYREFS  Too many references: cannot splice
	110 ETIMEDOUT    +Connection timed out
	111 ECONNREFUSED  Connection refused
	112 EHOSTDOWN     Host is down
	113 EHOSTUNREACH  No route to host
	114 EALREADY      Operation already in progress
	115 EINPROGRESS  +Operation now in progress
	116 ESTALE        Stale NFS file handle
	117 EUCLEAN       Structure needs cleaning
	118 ENOTNAM       Not a XENIX named type file
	119 ENAVAIL       No XENIX semaphores available
	120 EISNAM        Is a named type file
	121 EREMOTEIO     Remote I/O error
	122 EDQUOT        Disk quota exceeded
	123 ENOMEDIUM     No medium found
	124 EMEDIUMTYPE   Wrong medium type
	*/

#ifdef _IOS
#define FORCE_SELECT_MODE
#endif

#ifdef FORCE_SELECT_MODE
#include <sys/select.h>
	enum ENetworkEvent
	{
		eNE_Listen			= ( 1 << 0 ),
		eNE_Connecting		= ( 1 << 1 ),
		eNE_ConnectedRead	= ( 1 << 0 ),
		eNE_ConnectedWrite	= ( 1 << 1 ),
		eNE_Error			= ( 1 << 2 ),
	};
#else
#include <sys/epoll.h>
	enum ENetworkEvent
	{
		eNE_Listen			= EPOLLIN,
		eNE_Connecting		= EPOLLOUT,
		eNE_ConnectedRead	= EPOLLIN,
		eNE_ConnectedWrite	= EPOLLOUT,
		eNE_Error			= EPOLLERR,
	};
#endif

	#include <netinet/tcp.h> 
    #include <sys/ioctl.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
    #include <unistd.h>
	#include <netdb.h>
	#include <cerrno>

	#define SOCKET uint32
	#define INVALID_SOCKET        -1

	#define GNWGetLastError	GetLastError
	#define closesocket		close
	#define ioctlsocket		ioctl
	//typedef int32              SOCKET; //re-defined against the line:222!
	typedef struct linger	LINGER;
	#define SOCKET_ERROR	-1
	#define SD_SEND			SHUT_WR

	#define NE_EADDRNOTAVAIL	EADDRNOTAVAIL
	#define NE_EWOULDBLOCK		EWOULDBLOCK	
	#define NE_EINPROGRESS		EINPROGRESS	
	#define NE_EADDRINUSE		EADDRINUSE	
	#define NE_ENETUNREACH		ENETUNREACH	
	#define NE_ENETRESET		ENETRESET	
	#define NE_ECONNRESET		ECONNRESET	
	#define NE_ENOBUFS			ENOBUFS		
	#define NE_ETIMEDOUT		ETIMEDOUT	
	#define NE_ECONNREFUSED		ECONNREFUSED	
	#define NE_EHOSTUNREACH		EHOSTUNREACH	
	#define NE_ECONNABORTED		ECONNABORTED	
	#define NE_ENOTCONN			ENOTCONN		
	#define NE_EOPERABORTED		EOPERABORTED	

	inline int32 GetLastError() { return errno; }
	struct SDataBuffer { char* buf; size_t len; };

#endif 

	enum EConnectState
	{
		eCS_Connecting,
		eCS_Connected,
		eCS_Disconnecting,
		eCS_Disconnected,
		eCS_Unknow,
	};
	
	inline Gamma::CAddress MakeAddress( const sockaddr* addr, uint32 nSize )
	{
		Gamma::CAddress Address;
		if( addr->sa_family == AF_INET )
		{
			const sockaddr_in* addrIpv4 = (const sockaddr_in*)addr;
			Address.SetAddress( inet_ntoa( addrIpv4->sin_addr ) );
			Address.SetPort( ntohs( addrIpv4->sin_port ) );
		}
		else if( addr->sa_family == AF_INET6 )
		{
			char szAdress[256];
			const sockaddr_in6* addrIpv6 = (const sockaddr_in6*)addr;
			const uint8* aryBuf = (const uint8*)&addrIpv6->sin6_addr;
			sprintf( szAdress, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
				aryBuf[0], aryBuf[1], aryBuf[2], aryBuf[3], aryBuf[4], aryBuf[5], aryBuf[6], aryBuf[7],
				aryBuf[8], aryBuf[9], aryBuf[10], aryBuf[11], aryBuf[12], aryBuf[13], aryBuf[14], aryBuf[15] );
			Address.SetAddress( szAdress );
			Address.SetPort( ntohs( addrIpv6->sin6_port ) );
		}
		return Address;
	}
#endif

#include "stdafx.h"
#include "JarvisSocket.h"
#include <assert.h>

using namespace JarvisSS;

/***************************/
/*** STATIC INITIALIZERS ***/
/***************************/

bool JarvisSocket::_fAllValid = true;
const struct addrinfo JarvisSocket::DEFAULT_HINTS =
{
	AI_PASSIVE,
	AF_INET,
	SOCK_STREAM,
	IPPROTO_TCP,
	0, 0, 0, 0
};

/**********************************/
/*** CONSTRUCTORS / DESTRUCTORS ***/
/**********************************/

JarvisSocket::JarvisSocket(std::string strAddress, int iPort, DisconnectFunctionPointer pfn)
{
	// init member
	_sock = INVALID_SOCKET;
	_strIp = strAddress;
	_iPort = iPort;
	_pfnOnDisconnect = pfn;
	_fConnected = false;

	InitMBufs();
	Setup();
}

JarvisSocket::JarvisSocket(SOCKET sockInit, sockaddr* paddr, DisconnectFunctionPointer pfn)
{
	_sock = sockInit;
	_strIp = StrAddrFromPsockaddr(paddr);
	_iPort = IPortFromPsockaddr(paddr);
	_pfnOnDisconnect = pfn;
	_fConnected = false;
	
	InitMBufs();
	Setup();
}

JarvisSocket::JarvisSocket(const JarvisSocket& other)
{
	_sock = other._sock;
	_strIp = other._strIp;
	_iPort = other._iPort;
	_pfnOnDisconnect = other._pfnOnDisconnect;
	_fConnected = other._fConnected;

	memcpy_s(_bRecieve, BUF_SIZE, other._bRecieve, BUF_SIZE);
}

JarvisSocket& JarvisSocket::operator=(const JarvisSocket& other)
{
	_sock = other._sock;
	_strIp = other._strIp;
	_iPort = other._iPort;
	_pfnOnDisconnect = other._pfnOnDisconnect;
	_fConnected = other._fConnected;

	memcpy_s((void*)_bRecieve[0], BUF_SIZE, other._bRecieve, BUF_SIZE);
	return *this;
}

JarvisSocket::~JarvisSocket()
{
	if(_sock != INVALID_SOCKET)
	{
		if(closesocket(_sock))
		{
			assert(false);
		}
		_sock = INVALID_SOCKET;
	}

	Teardown();
}

/*******************************/
/*** PUBLIC MEMBER FUNCTIONS ***/
/*******************************/

SOCKET JarvisSocket::get()
{
	return FValid() ? _sock : INVALID_SOCKET;
}

char* JarvisSocket::PbRecieve()
{
	ZeroMemory(&_bRecieve[0], BUF_SIZE);
	int cbRecieved = recv(_sock, _bRecieve, BUF_SIZE, 0);
	if (cbRecieved)
	{
		_fConnected = true;
	}
	return (FValid() && cbRecieved > 0) ? &_bRecieve[0] : NULL;
}

bool JarvisSocket::FConnect()
{
	int iResult = 0;
	int iError = 0;
	bool fStatus = false;
	addrinfo* paddrinfo;

	// resolve to server address and port
	if (0 != getaddrinfo(_strIp.c_str(), std::to_string(_iPort).c_str(), &DEFAULT_HINTS, &paddrinfo))
	{
		printf("%d\n", WSAGetLastError());
		NormalErr(L"Failed resolving server address and port");
	}
	// create socket
	_sock = socket(paddrinfo->ai_family, paddrinfo->ai_socktype, paddrinfo->ai_protocol);
	if (INVALID_SOCKET == _sock)
	{
		NormalErr(L"Failed creating socket");
	}
	// connect socket
	for (struct addrinfo *rp = paddrinfo; rp != NULL; rp = rp->ai_next)
	{
		if (!(iResult = connect(_sock, rp->ai_addr, rp->ai_addrlen)))
		{
			fStatus = true;
			_fConnected = true;
		}
		else if (WSAECONNREFUSED != (iError = WSAGetLastError()))
		{
			printf("Error: %d\n", iError);
		}
	}
	freeaddrinfo(paddrinfo);
	return fStatus;
}

bool JarvisSocket::FSend(const char* pbData, int iSize)
{
	bool fSucceded;
	if (!FIsConnected())
		return false;
	
	fSucceded= (SOCKET_ERROR != send(_sock, pbData, iSize, 0)) && FValid();

	if (!fSucceded)
	{
		switch (int iErr = WSAGetLastError())
		{
			// Connection was Lost cases
			case WSAENETDOWN:
			case WSAENETUNREACH:
			case WSAENETRESET:
			case WSAECONNABORTED:
			case WSAECONNRESET:
			case WSAENOTCONN:
			case WSAECONNREFUSED:
			case WSAEHOSTDOWN:
			case WSAEDISCON:
				OnDisconnect();
				break;
			//Fatal Error cases
			default:
				FatalErr(L"WSAGetLastError: " + iErr);			
		}
	}

	return fSucceded;
}

bool JarvisSocket::FValid()
{
	return _fAllValid;
}

bool JarvisSocket::FIsConnected()
{
	return _fConnected;
}

std::string JarvisSocket::getStrIp()
{
	return _strIp;
}

int JarvisSocket::getIPort()
{
	return _iPort;
}

/**********************************/
/*** PROTECTED MEMBER FUNCTIONS ***/
/**********************************/

void JarvisSocket::FatalErr(const wchar_t* xwszMsg)
{
#ifdef _DEBUG
	assert(false);
#endif

	MessageBox(NULL, xwszMsg, L"Fatal Socket Error", 0);
	_fAllValid = false;
}

void JarvisSocket::NormalErr(const wchar_t* xwszMsg, bool fSilent)
{
#ifdef _DEBUG
	assert(false);
	if (!fSilent)
	{
		MessageBox(NULL, std::to_wstring(WSAGetLastError()).c_str(), L"WSAGetLastError failure code", 0);
	}
#endif

	if (!fSilent)
	{
		MessageBox(NULL, xwszMsg, L"Socket Error", 0);
	}
	_fAllValid = false;
}

/********************************/
/*** PRIVATE MEMBER FUNCTIONS ***/
/********************************/

void JarvisSocket::Setup()
{
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		FatalErr(L"WSAStartup Failed");
	}
}

void JarvisSocket::Teardown()
{
	WSACleanup();
}

std::string JarvisSocket::StrAddrFromPsockaddr(sockaddr* client_addr)
{
	char str[INET_ADDRSTRLEN];

	sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_addr;
	int ipAddr = pV4Addr->sin_addr.s_addr;
	inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN);

	return std::string(str);
}

int JarvisSocket::IPortFromPsockaddr(sockaddr* client_addr)
{
	int cbBuf = (int)log10(MAXINT) + 2;
	sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_addr;
	return pV4Addr->sin_port;
}

void JarvisSocket::InitMBufs()
{
	ZeroMemory(&_bRecieve, BUF_SIZE);
}

void JarvisSocket::OnDisconnect()
{
	_fConnected = false;
	if (NULL != _pfnOnDisconnect)
	{
		(_pfnOnDisconnect)();
	}
}
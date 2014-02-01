#include "stdafx.h"
#include "JarvisSocket.h"

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

JarvisSocket::JarvisSocket(std::string strAddress, int iPort, bool fBlocking, bool fQuick)
{
	// init member
	_fBlockingIO = true; /*temp*/ if(!fBlocking){FatalErr(L"Non Blocking sockets not yet implemented");}
	_fQuick = false; /*temp*/ if(!fBlocking){FatalErr(L"Quick sockets not yet implemented");}
	_sock = INVALID_SOCKET;
	_strIp = strAddress;
	_iPort = iPort;
	_paddrinfoResult = NULL;

	InitMBufs();
	
	Setup();
	
	// resolve to server address and port
	if (0 != getaddrinfo(strAddress.c_str(), std::to_string(iPort).c_str(), &DEFAULT_HINTS, &_paddrinfoResult))
	{
		NormalErr(L"Failed resolving server address and port");
	}

	// create socket
	_sock = socket(_paddrinfoResult->ai_family, _paddrinfoResult->ai_socktype, _paddrinfoResult->ai_protocol);
	if (INVALID_SOCKET == _sock)
	{
		NormalErr(L"Failed creating socket");
	}
}

JarvisSocket::JarvisSocket(SOCKET sockInit, sockaddr* paddr, bool fBlocking, bool fQuick)
{
	_sock = sockInit;
	_strIp = StrAddrFromPsockaddr(paddr);
	_iPort = IPortFromPsockaddr(paddr);
	_fBlockingIO = true; /*temp*/ if (!fBlocking){ NormalErr(L"Non Blocking sockets not yet implemented"); }
	_fQuick = false; /*temp*/ if (!fBlocking){ NormalErr(L"Quick sockets not yet implemented"); }
	_paddrinfoResult = NULL;
	
	Setup();
	InitMBufs();
}

JarvisSocket::~JarvisSocket()
{
	if(_sock != INVALID_SOCKET)
	{
		if(closesocket(_sock))
		{
			FatalErr(L"Error closing socket");
		}
		_sock = INVALID_SOCKET;
	}

	free(_pbRecieve);
	freeaddrinfo(_paddrinfoResult);

	Teardown();
}

/*******************************/
/*** PUBLIC MEMBER FUNCTIONS ***/
/*******************************/

SOCKET JarvisSocket::get()
{
	return FValid() ? _sock : INVALID_SOCKET;
}
#include <string>
char* JarvisSocket::PbRecieve()
{
	ZeroMemory(_pbRecieve, BUF_SIZE);
	int cbRecieved = recv(_sock, _pbRecieve, BUF_SIZE, 0);
	std::string test = std::string(_pbRecieve);
	return (FValid() && cbRecieved > 0) ? _pbRecieve : NULL;
}

bool JarvisSocket::FConnect()
{
	int iResult = 0;
	bool fStatus = false;
	_sock = socket(_paddrinfoResult->ai_family, _paddrinfoResult->ai_socktype, _paddrinfoResult->ai_protocol);
	// connect socket
	for (struct addrinfo *rp = _paddrinfoResult; rp != NULL; rp = rp->ai_next)
	{
		if (!(iResult = connect(_sock, rp->ai_addr, rp->ai_addrlen)))
		{
			fStatus = true;
		}
		else if (WSAECONNREFUSED != iResult)
		{
			printf("Error: %d\n", WSAGetLastError());
		}
	}

	return fStatus;
}

bool JarvisSocket::FSend(const char* pbData, int iSize)
{
	return (SOCKET_ERROR != send(_sock, pbData, iSize, 0)) && FValid();
}

bool JarvisSocket::FValid()
{
	return _fAllValid;
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
	MessageBox(NULL, xwszMsg, L"Fatal Socket Error", 0);

#ifdef _DEBUG
	MessageBox(NULL, std::to_wstring(WSAGetLastError()).c_str(), L"WSAGetLastError failure code", 0);
#endif

	_fAllValid = false;
}

void JarvisSocket::NormalErr(const wchar_t* xwszMsg, bool fSilent)
{
	if (!fSilent)
	{
		MessageBox(NULL, xwszMsg, L"Socket Error", 0);
	}

#ifdef _DEBUG
	if (!fSilent)
	{
		MessageBox(NULL, std::to_wstring(WSAGetLastError()).c_str(), L"WSAGetLastError failure code", 0);
	}
#endif
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
	_pbRecieve = (char*)malloc(BUF_SIZE);
}

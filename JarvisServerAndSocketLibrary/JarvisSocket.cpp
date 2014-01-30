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

JarvisSocket::JarvisSocket(std::string strAddress, std::string strPort, bool fBlocking, bool fQuick)
{
	int iResult;
	addrinfo *result;
	
	// init member
	_fBlockingIO = true; /*temp*/ if(!fBlocking){FatalErr(L"Non Blocking sockets not yet implemented");}
	_fQuick = false; /*temp*/ if(!fBlocking){FatalErr(L"Quick sockets not yet implemented");}
	_sock = INVALID_SOCKET;
	_fValid = true;
	_strIp = strAddress;
	_strPort = strPort;

	InitMBufs();
	
	Setup();
	
	// resolve to server address and port
	if (0 != getaddrinfo(strAddress.c_str(), strPort.c_str(), &DEFAULT_HINTS, &result))
	{
		NormalErr(L"Failed resolving server address and port");
	}

	// create socket
	_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (INVALID_SOCKET == _sock)
	{
		NormalErr(L"Failed creating socket");
	}
		
	// connect socket
	for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
	{
		if(!(iResult = connect(_sock, rp->ai_addr, rp->ai_addrlen)))
		{
			break;
		}
	}
		
	if(iResult)
	{
		NormalErr(L"Failed connecting to server");
	}

	freeaddrinfo(result);
}

JarvisSocket::JarvisSocket(SOCKET sockInit, sockaddr* paddr, bool fBlocking, bool fQuick)
{
	_sock = sockInit;
	_fValid = true;
	_strIp = StrAddrFromPsockaddr(paddr);
	_strPort = StrPortFromPsockaddr(paddr);
	_fBlockingIO = true; /*temp*/ if (!fBlocking){ NormalErr(L"Non Blocking sockets not yet implemented"); }
	_fQuick = false; /*temp*/ if (!fBlocking){ NormalErr(L"Quick sockets not yet implemented"); }
	
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

	free(_pbSend);
	free(_pbRecieve);

	Teardown();
}

/*******************************/
/*** PUBLIC MEMBER FUNCTIONS ***/
/*******************************/

SOCKET JarvisSocket::get()
{
	return FValid() ? _sock : INVALID_SOCKET;
}

void* JarvisSocket::PbRecieve()
{
	ZeroMemory(_pbRecieve, BUF_SIZE);
	int cbRecieved = recv(_sock, (char*)_pbRecieve, BUF_SIZE, 0);
	return (FValid() && cbRecieved > 0) ? _pbRecieve : NULL;
}

bool JarvisSocket::FSend(void* pData, int iSize)
{
	return (SOCKET_ERROR != send(_sock, (const char*)_pbSend, iSize, 0)) && FValid();
}

bool JarvisSocket::FValid()
{
	return _fValid && _fAllValid;
}

std::string JarvisSocket::getStrIp()
{
	return _strIp;
}

std::string JarvisSocket::getStrPort()
{
	return _strPort;
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
	MessageBox(NULL, std::to_wstring(WSAGetLastError()).c_str(), L"WSAGetLastError failure code", 0);
#endif

	_fValid = false;
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

std::string JarvisSocket::StrPortFromPsockaddr(sockaddr* client_addr)
{
	int cbBuf = (int)log10(MAXINT) + 2;
	sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_addr;
	int ipPort = pV4Addr->sin_port;
	char* buf = (char*)malloc(cbBuf);
	_itoa_s(ipPort, buf, cbBuf, 10);
	std::string retval = std::string(buf);
	free(buf);
	return retval;
}

void JarvisSocket::InitMBufs()
{
	_pbRecieve = (char*)malloc(BUF_SIZE);
	_pbSend = (char*)malloc(BUF_SIZE);
}

#include "stdafx.h"
#include <assert.h>
#include "JarvisServer.h"
#include "JarvisSocket.h"
#include "IDataHandler.h"

#include <math.h>
using namespace JarvisSS;

/****************************/
/*** FORWARD DELCARATIONS ***/
/****************************/
typedef unsigned int uint;
uint DmsecSleepNowFromDmsecSleep(uint dmsecNow);

/**********************************/
/*** CONSTRUCTORS / DESTRUCTORS ***/
/**********************************/

JarvisServer::JarvisServer(int iPort, IDataHandler* pdh, DisconnectFunctionPointer dfnp)
{
	// init members
	_pdh = pdh; // converting pdh (type: pointer to data) to mpdh (type: pointer to a function returning void and taking DataHandlerParams* as an arg) 
	_fQuit = false;
	_fHasQuit = true;
	_iPort = iPort;
	_pfnOnDisconnect = dfnp;
		
	// do proper setup for windows sockets
	Setup();
}

JarvisServer::~JarvisServer()
{
	Stop();
	Teardown();
}

/*******************************/
/*** PUBLIC MEMBER FUNCTIONS ***/
/*******************************/

HANDLE JarvisServer::StartOnNewThread()
{
	return CreateThread(NULL, 0, JarvisServer::ServerThreadFunc, this, 0, NULL);
}

void JarvisServer::Start()
{
	SOCKET sockTemp;
	JarvisSocket* pjsock;
	sockaddr addr;
	SocketThreadParams* socktp;
	_fHasQuit = false;
	uint dmsecSleep = 0;

	// create socket to listen on and set it to listen
	_sockListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//JarvisServer::CreateListenSocket(&sockListen);

	/* should be taken care of by CreateSockListen. keeping around til I know that function works. */
	//= socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sockListen)
		JarvisSocket::FatalErr(L"Listen socket creation failed.");

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("0.0.0.0");
	service.sin_port = htons(_iPort);	// no idea if this is right. look here for bugs if the port isnt the right port

	if (SOCKET_ERROR == bind(_sockListen, (SOCKADDR *)& service, sizeof (service)))
	{
		if (WSAEADDRINUSE == WSAGetLastError())
		{
			JarvisSocket::FatalErr(L"Port already in use");
		}
		else
		{
			JarvisSocket::FatalErr(L"Listen socket binding failed.");
			int test = WSAGetLastError();
			MessageBox(NULL, std::to_wstring(test).c_str(), L"WSAGetLastError failure code", 0);
		}
	}

	if (0 != listen(_sockListen, SOMAXCONN))
		JarvisSocket::FatalErr(L"Listen socket listening failed.");

	// make non blocking
	u_long NonBlock = 0;
	if (ioctlsocket(_sockListen, FIONBIO, &NonBlock) == SOCKET_ERROR)
	{
		assert(false);
	}

	while (true)
	{
		// accept connections
		if (INVALID_SOCKET == (sockTemp = accept(_sockListen, &addr, NULL)))
		{
			return;
		}

		// create JarvisSocket from socket
		pjsock = new JarvisSocket(sockTemp, &addr);

		// send them to other threads to do work
		socktp = (SocketThreadParams*)malloc(sizeof(SocketThreadParams));
		socktp->pjsock = pjsock;
		socktp->pjserv = this;
		CreateThread(NULL, 0, JarvisServer::SocketThreadFunc, socktp, 0, NULL);
	}
}

void JarvisServer::Stop()
{
	closesocket(_sockListen);
}

IDataHandler* JarvisServer::PdhGet()
{
	return _pdh;
}

/********************************/
/*** PRIVATE MEMBER FUNCTIONS ***/
/********************************/

void JarvisServer::Setup()
{
	WSADATA wsadata;
	// i think all we need to do here is call WSASetup?
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		JarvisSocket::FatalErr(L"WSAStartup Failed");
	}
}

void JarvisServer::Teardown()
{
	WSACleanup();
}

DWORD WINAPI JarvisServer::ServerThreadFunc(void* pParams)
{
	JarvisServer* jserv = (JarvisServer*)pParams;
	JarvisServer::Setup();
	jserv->Start();
	Teardown();

	return 0;
}

DWORD WINAPI JarvisServer::SocketThreadFunc(void* pParams)
{
	SocketThreadParams socktp = *(SocketThreadParams*)pParams;
	DataHandlerParams dhp;
	free(pParams);

	dhp.pjsock = socktp.pjsock;
	dhp.pjserv = socktp.pjserv;
	
	while(true) // while connection is alive
	{
		// receive data on socket and handle the data
		dhp.pbBuf = socktp.pjsock->PbRecieve();
		if (!dhp.pjsock->_fConnected)
		{
			socktp.pjserv->OnDisconnect();
			break;
		}
		if (NULL != dhp.pbBuf)
			(socktp.pjserv->_pdh)->HandleData(&dhp);
	}	

	delete socktp.pjsock;
	return 0;
	// JarvisSocket going out of scope should take care of disposing of the socket correctly
}

void JarvisServer::OnDisconnect()
{
	if (NULL != _pfnOnDisconnect)
	{
		(*_pfnOnDisconnect)();
	}
}

uint DmsecSleepNowFromDmsecSleep(uint dmsecNow)
{
	const uint MAX_SLEEP = 100;
	return min(MAX_SLEEP, dmsecNow + 1);
}

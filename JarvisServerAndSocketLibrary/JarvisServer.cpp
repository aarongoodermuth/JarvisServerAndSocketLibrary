#include "stdafx.h"
//#include <string>
#include "JarvisServer.h"
#include "JarvisSocket.h"

using namespace JarvisSS;

/**********************************/
/*** CONSTRUCTORS / DESTRUCTORS ***/
/**********************************/

JarvisServer::JarvisServer(std::string strPort, void* pfdh)
{
	// init members
	_pfdh = (void (*)(DataHandlerParams*)) pfdh; // converting pdh (type: pointer to data) to mpdh (type: pointer to a function returning void and taking DataHandlerParams* as an arg) 
	_fQuit = false;
	_strPort = strPort;
		
	// do proper setup for windows sockets
	Setup();
	
}

JarvisServer::~JarvisServer()
{
	_fQuit = true;
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

	// create socket to listen on and set it to listen
	SOCKET sockListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//JarvisServer::CreateListenSocket(&sockListen);

	/* should be taken care of by CreateSockListen. keeping around til I know that function works. */
	//= socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sockListen)
		JarvisSocket::FatalErr(L"Listen socket creation failed.");

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("0.0.0.0");
	service.sin_port = htons(atoi(_strPort.c_str()));	// no idea if this is right. look here for bugs if the port isnt the right port

	if (SOCKET_ERROR == bind(sockListen, (SOCKADDR *)& service, sizeof (service)))
		JarvisSocket::FatalErr(L"Listen socket binding failed.");

	int test = WSAGetLastError();
	//MessageBox(NULL, std::to_wstring(test).c_str(), L"WSAGetLastError failure code", 0);

	if (0 != listen(sockListen, SOMAXCONN))
		JarvisSocket::FatalErr(L"Listen socket listening failed.");

	while (!_fQuit)
	{
		// release the thread if we should....i think?
		Sleep(0);

		// accept connections
		if (INVALID_SOCKET == (sockTemp = accept(sockListen, &addr, NULL)))
			continue;

		// create JarvisSocket from socket
		pjsock = new JarvisSocket(sockTemp, &addr);

		// send them to other threads to do work
		socktp = (SocketThreadParams*)malloc(sizeof(SocketThreadParams));
		socktp->paddr = &addr;
		socktp->psock = &sockTemp;
		socktp->pjserv = this;
		CreateThread(NULL, 0, JarvisServer::SocketThreadFunc, socktp, 0, NULL);
	}

	closesocket(sockListen);
}

void JarvisServer::Stop()
{
	_fQuit = true;
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
	delete jserv;

	return 0;
}

DWORD WINAPI JarvisServer::SocketThreadFunc(void* pParams)
{
	char* pbBbuf = (char*)malloc(M_BUF_SIZE);
	SocketThreadParams socktp = *(SocketThreadParams*)pParams;
	DataHandlerParams dhp;
	free(pParams);

	JarvisSocket jsock = JarvisSocket(*socktp.psock, socktp.paddr);
	dhp.pjsock = &jsock;
	
	while(true) // while connection is alive
	{
		// receive data on socket and handle the data
		dhp.pbBuf = jsock.PbRecieve();
		if (NULL == dhp.pbBuf)
			break;
		(socktp.pjserv->_pfdh)(&dhp);
	}	

	return 0;
	// JarvisSocket going out of scope should take care of disposing of the socket correctly
}
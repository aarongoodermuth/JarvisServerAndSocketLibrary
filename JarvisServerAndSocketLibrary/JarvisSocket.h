#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#define BUF_SIZE 1024

namespace JarvisSS
{
	class JarvisSocket
	{
	friend class JarvisServer;
	public:
		// typedefs
		typedef void(*DisconnectFunctionPointer)();

		// constructors/destructors/assignment operators
		JarvisSocket(std::string, int, DisconnectFunctionPointer pfn = NULL);
		JarvisSocket(SOCKET, sockaddr*, DisconnectFunctionPointer pfn = NULL);
		JarvisSocket(const JarvisSocket& other);
		JarvisSocket& operator=(const JarvisSocket& rhs);
		~JarvisSocket();
		
		// member functions
		SOCKET get();
		char* PbRecieve();
		bool FConnect();
		bool FSend(const char*, int);
		bool FValid();
		bool FIsConnected();
		std::string getStrIp();
		int getIPort();
	
	protected:
		
		static void FatalErr(const wchar_t*);
		static void NormalErr(const wchar_t* = L"", bool fSilent = false);

	private:
		// member variables
		static bool _fAllValid;
		static const addrinfo DEFAULT_HINTS;

		bool _fConnected;
		DisconnectFunctionPointer _pfnOnDisconnect;
		SOCKET _sock;
		char _bReceive[BUF_SIZE];
		int _cbReceive;
		std::string _strIp;
		int _iPort;

		// member functions
		static void Setup();
		static void Teardown();
		static std::string StrAddrFromPsockaddr(sockaddr*);
		static int IPortFromPsockaddr(sockaddr*);
		void InitMBufs();
		void OnDisconnect();
	};
}

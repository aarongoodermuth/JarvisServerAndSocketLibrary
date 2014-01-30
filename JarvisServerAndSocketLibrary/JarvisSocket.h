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
		// constructors/destructors
		JarvisSocket(std::string, std::string, bool fBlocking = true, bool fQuick = false);
		JarvisSocket(SOCKET, sockaddr*, bool fBlocking = true, bool fQuick = false);
		~JarvisSocket();
		
		// member functions
		void DestroySock();
		SOCKET get();
		void* PbRecieve();
		bool FSend(void*, int);
		bool FValid();
		std::string getStrIp();
		std::string getStrPort();
	
	protected:
		static bool _fValid;
		static void FatalErr(const wchar_t*);
		static void NormalErr(const wchar_t* = L"", bool fSilent = false);

	private:
		// member variables
		static bool _fAllValid;
		static const addrinfo DEFAULT_HINTS;

		bool _fQuick;
		bool _fBlockingIO;
		SOCKET _sock;
		void* _pbRecieve;
		void* _pbSend;
		std::string _strIp;
		std::string _strPort;

		// member functions
		static void Setup();
		static void Teardown();
		static std::string StrAddrFromPsockaddr(sockaddr*);
		static std::string StrPortFromPsockaddr(sockaddr*);

		void InitMBufs();
	};
}
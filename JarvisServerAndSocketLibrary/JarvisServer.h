#pragma once

#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "JarvisSocket.h"

namespace JarvisSS
{
	class JarvisServer
	{
	public:
		// constructors/destructors
		JarvisServer(std::string, void*);
		~JarvisServer();
		
		// structure definitions	
		struct DataHandlerParams
		{
			void* pbBuf;
			JarvisSocket* pjsock;
		};

		// member variables
		const static int M_BUF_SIZE = 1024;
		
		// member functions
		HANDLE StartOnNewThread();	
		void Start();
		void Stop();
		
	private:
		// structure definitions
		struct SocketThreadParams
		{
			SOCKET* psock;
			sockaddr* paddr;
			JarvisServer* pjserv;
		};
		
		// member variables
		void (*_pfdh)(DataHandlerParams*); // pointer to a function with return type: void that takes arguments of type DataHandlerParams*
		bool _fQuit;
		std::string _strPort;
	
		// member functions
		static void Setup();
		static void Teardown();
		static DWORD WINAPI ServerThreadFunc(void*);
		static DWORD WINAPI SocketThreadFunc(void*);
	};	
}


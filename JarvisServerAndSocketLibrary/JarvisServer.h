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
		// structure definitions	
		struct DataHandlerParams
		{
			char* pbBuf;
			JarvisSocket* pjsock;
		};
		typedef void(*DataHandlerFunctionPointer)(DataHandlerParams*);

		// constructors/destructors
		JarvisServer(int, DataHandlerFunctionPointer);
		~JarvisServer();
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
		DataHandlerFunctionPointer _dhfp; // pointer to a function with return type: void that takes arguments of type DataHandlerParams*
		bool _fQuit;
		int _iPort;
	
		// member functions
		static void Setup();
		static void Teardown();
		static DWORD WINAPI ServerThreadFunc(void*);
		static DWORD WINAPI SocketThreadFunc(void*);
	};
}


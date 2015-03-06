#pragma once

#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "JarvisSocket.h"

namespace JarvisSS
{
	class IDataHandler;
	class JarvisServer
	{
	public:
		// structure definitions	
		struct DataHandlerParams
		{
			char* pbBuf;
			JarvisSocket* pjsock;
			JarvisServer* pjserv;
			bool fBufTooSmall; // pbBuf will not hold all the data. You should either use pistm, or call pbBuf repeatedly until this flag is false
		};
		typedef void(*DataHandlerFunctionPointer)(DataHandlerParams*);
		typedef void(*DisconnectFunctionPointer)();

		// constructors/destructors
		JarvisServer(int, IDataHandler* pdh, DisconnectFunctionPointer dfnp = NULL);
		~JarvisServer();
		// member variables
		const static int M_BUF_SIZE = 1024;
		
		// member functions
		HANDLE StartOnNewThread();	
		void Start();
		void Stop();
		IDataHandler* PdhGet();
		
	private:
		// structure definitions
		struct SocketThreadParams
		{
			JarvisSocket* pjsock;
			JarvisServer* pjserv;
		};
		
		// member variables
		IDataHandler* _pdh; // pointer to a function with return type: void that takes arguments of type DataHandlerParams*
		DisconnectFunctionPointer _pfnOnDisconnect;
		bool _fQuit;
		bool _fHasQuit;
		int _iPort;
		SOCKET _sockListen;
	
		// member functions
		static void Setup();
		static void Teardown();
		static DWORD WINAPI ServerThreadFunc(void*);
		static DWORD WINAPI SocketThreadFunc(void*);

		void OnDisconnect();
	};
}


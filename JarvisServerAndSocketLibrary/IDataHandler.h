#pragma once
#include "JarvisServer.h"

namespace JarvisSS
{ 
	class IDataHandler
	{
	public:
		IDataHandler() {};
		virtual ~IDataHandler() {};

		virtual void HandleData(JarvisServer::DataHandlerParams* dhp) {};
	};

}

#include "utils.h"

#include <Windows.h>

std::string GenerateGUID()
{
	std::string result;
	UUID uuid = {};
	UuidCreate( &uuid );
	RPC_CSTR uuidStr;
	if ( RPC_S_OK == UuidToStringA( &uuid, &uuidStr ) ) {
		result = reinterpret_cast<char*>( uuidStr );
		RpcStringFreeA( &uuidStr );
	}
	return result;
}

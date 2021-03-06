/****************************
Copyright © 2006-2012 Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
****************************/
#include "engine.h"
#include "portal.h"
#include "core.h"
#include "config.h"

#include <dirent.h>
#include <AS3/AS3.h>

#define MODULE_EXPORT extern "C"
bool runningInBackground = false;

namespace luxtest
{
	void end();
	void loop();
	void start();
}

MODULE_EXPORT void update_engine()
{
	if ( lux::engine )
		lux::engine->Refresh();
}

MODULE_EXPORT void start_engine()
{
	lux::engine = new LuxEngine( "Preview_Quest.quest", "" );
	lux::engine->Start();
}

MODULE_EXPORT void close_engine()
{
	if ( lux::engine )
	lux::engine->Close();
}

MODULE_EXPORT int main( int argc, char *argv[] )
{
	AS3_GoAsync();
	return 0;
}






#include <default>
#include <graphics>
#include <string>

/* Main Entity has 3 more Public Functions */
forward public save();
forward public load();
forward public dialogbox(line);

new player = 1;

main()
{
	if (  InputButton(0,2) == 1)
	{
		player =InputSetDefault(2);
	}
	if (  InputButton(0,1) == 1)
	{
		player =InputSetDefault(1);
	}

	if (  InputButton(0,3) == 1)
	{
		player =InputSetDefault(3);
	}
	DebugText("default player: %d", player);
	DebugText("❶ ❷ ❸ ❹ ❺ ❻ ❼ ❽ ❾ ❿ ➀ ➁ ➂ ➃ ➄ ➅	▲	△	▼	▽	◀	◁	▶	▷	◐	◒	◑	◓ ");

}



public save()
{ 
}

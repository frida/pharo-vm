#include "pharovm/pharo.h"
#include "pharovm/staticPlugins.h"

extern void *vm_exports[][3];
extern void *os_exports[][3];

static StaticPlugin osStaticPlugin = { os_exports, NULL };
static StaticPlugin vmStaticPlugin = { vm_exports, &osStaticPlugin };

static StaticPlugin *firstPlugin = &vmStaticPlugin;

EXPORT(void)
registerStaticPlugin(StaticPlugin *plugin)
{
	plugin->next = firstPlugin;
	firstPlugin = plugin;
}

EXPORT(StaticPlugin *)
firstStaticPlugin(void)
{
	return firstPlugin;
}

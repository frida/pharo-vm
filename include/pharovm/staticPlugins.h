#pragma once

#ifndef __staticPlugins_h__
#define __staticPlugins_h__

typedef struct StaticPlugin StaticPlugin;

struct StaticPlugin {
	void *(*exports)[3];
	StaticPlugin *next;
};

VM_EXPORT void registerStaticPlugin(StaticPlugin *plugin);
VM_EXPORT StaticPlugin * firstStaticPlugin(void);

#if defined(_MSC_VER)
# define STATIC_PLUGIN_INITIALIZER(name) \
	static void name(void); \
	__pragma(section(".CRT$XCU", read)) \
	__declspec(allocate(".CRT$XCU")) \
		static void (*name##Entry)(void) = name; \
	static void name(void)
#else
# define STATIC_PLUGIN_INITIALIZER(name) \
	__attribute__((constructor)) static void name(void)
#endif

#define STATIC_PLUGIN(pluginName) \
	extern void *pluginName##_exports[][3]; \
	static StaticPlugin pluginName##StaticPlugin = { \
		pluginName##_exports, \
		NULL \
	}; \
	STATIC_PLUGIN_INITIALIZER(register##pluginName) \
	{ \
		registerStaticPlugin(&pluginName##StaticPlugin); \
	}

#endif

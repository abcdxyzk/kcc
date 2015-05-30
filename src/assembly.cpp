#include "kcc.h"

#if defined(__linux_32_system__) || defined(__windows_system__)
#include "./assembly_32.cpp"
#elif defined(__linux_64_system__)
#include "./assembly_64.cpp"
#endif

int assembly_main()
{
#if defined(__linux_32_system__) || defined(__windows_system__)
	return assembly_32_main();
#elif defined(__linux_64_system__)
	return assembly_64_main();
#endif
}


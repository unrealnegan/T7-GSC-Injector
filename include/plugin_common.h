#include <Common.h>

#define attr_module_hidden __attribute__((weak)) __attribute__((visibility("hidden")))
#define attr_public __attribute__((visibility("default")))

#define TEX_ICON_CHAMPIONS_LEAGUE "cxml://psnotification/tex_icon_champions_league"
#define MAX_PATH_ 260

#define final_printf(a, args...) klog("(%s:%d) " a, __FILE__, __LINE__, ##args)

void notif(const char* iconUri, const char* text);
void fnotif(const char* iconUri, const char *FMT, ...);

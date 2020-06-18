#ifndef _simple_h_

#include <syslog.h>

#if 0
#define loge(args...) syslog(LOG_LOCAL0|LOG_ERR, args)
#define logi(args...) syslog(LOG_LOCAL0|LOG_INFO, args)
#else
#define loge(args...) printf(args)
#define logi(args...) printf(args)
#endif

#endif


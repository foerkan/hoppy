#include <android_native_app_glue.h>
#include "mengine_platform.h"
#include "mengine_math.h"
#include "mengine_render.h"
#include "mengine_opengles.h"


/* Type Definitions */
typedef struct android_app android_app;
typedef struct android_poll_source android_poll_source;


/* Constants */
#define InitialGameMemorySize Megabytes(8)


/* Logger */
#include <android/log.h>

#define __LogTag "Mengine"
#define __LogBufferSize 256
#define __LogTempSize __LogBufferSize

static char __logBuffer[__LogBufferSize];
static char __logTemp[__LogTempSize];

#define __log(prio, ...) \
	if(snprintf(__logTemp, __LogTempSize, __VA_ARGS__) > 0){ \
		if(snprintf(__logBuffer, __LogBufferSize, "%s [%s:%d (%s)]", __logTemp, __FUNCTION__, __LINE__, __FILE__)){ \
			__android_log_print(prio, __LogTag, "%s", __logBuffer); \
		} \
	}

#define Debug(...) __log(ANDROID_LOG_DEBUG, __VA_ARGS__)
#define Error(...) __log(ANDROID_LOG_ERROR, __VA_ARGS__)
#define Warning(...) __log(ANDROID_LOG_WARN, __VA_ARGS__)


/* Other Util */
#define Assert(expr) \
	if(!(expr)){ \
		Error("Assertion failed!"); \
		*(int *) 0 = 0; \
	}
#define InvalidDefaultCase \
	default:{ \
		Error("An unknown case appeared in switch statement"); \
	} break; 

/* Platform structures */
typedef struct android_shared_data{
#if !SW_RENDERER
	opengles_manager GLESManager;
#endif

	b8 IsRunning;
	b8 IsEnabled;
	b8 RendererAvailable;
} android_shared_data;



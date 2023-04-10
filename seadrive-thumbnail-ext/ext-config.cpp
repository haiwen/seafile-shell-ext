#include "ext-config.h"

const char *log_file_name = "seadrive_thumbnail_ext.log";

// The timeout of pipe is 300s, which is same as the timeout for requesting thumbnails.
int kPipeWaitTimeMSec = 300000;

#ifndef SHELLEXT_FIX_LOG
#define SHELLEXT_FIX_LOG

int shellfix_log_start();
void shellfix_log_stop();

void shellfix_log_aux(const char *format, ...);

#define shellfix_log(format, ... )                                  \
    shellfix_log_aux("%s(line %d) %s: " format,                     \
                     __FILE__, __LINE__, __func__, ##__VA_ARGS__)   \


#endif // SHELLEXT_FIX_LOG


#ifndef LABWATCH_LOGFMT_H
#define LABWATCH_LOGFMT_H
int lw_printf(const char *, ...) __attribute__((format(printf,1,2)));
#endif

#include "logfmt.h"
#include "board.h"
#include <stdarg.h>
#include <stddef.h>
/* This product emits only strings and 32-bit decimal integers. A complete line
 * is queued atomically; overload drops a line rather than splicing its bytes. */
int lw_printf(const char *format,...) {
    char line[512];size_t used=0;va_list ap;va_start(ap,format);
    for(const char *p=format;*p&&used<sizeof line;++p) {
        if(*p!='%'){line[used++]=*p;continue;}
        ++p;bool wide=*p=='l';if(wide)++p;
        if(*p=='s') {
            const char *s=va_arg(ap,const char *);
            while(*s&&used<sizeof line)line[used++]=*s++;
        } else if(*p=='u'||*p=='d') {
            uint32_t value;
            if(*p=='d') {
                int32_t signed_value=wide?(int32_t)va_arg(ap,long):va_arg(ap,int);
                if(signed_value<0){if(used<sizeof line)line[used++]='-';value=0u-(uint32_t)signed_value;}
                else value=(uint32_t)signed_value;
            } else value=wide?(uint32_t)va_arg(ap,unsigned long):va_arg(ap,unsigned);
            char digits[10];unsigned count=0;
            do {digits[count++]=(char)('0'+value%10);value/=10;}while(value);
            while(count&&used<sizeof line)line[used++]=digits[--count];
        } else {va_end(ap);return -1;}
    }
    va_end(ap);
    if(used==sizeof line)return -1;
    return board_write_line(line,used)?(int)used:-1;
}

#ifndef _LOG_H_INCLUDED
#	define _LOG_H_INCLUDED

#	define LOG_CHUNK_SIZE (4)

void log_write(const char *format, ...);

#endif

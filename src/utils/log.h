#ifndef LOG_H
#define LOG_H

#define LOG_MESSAGE 1
#define LOG_WARNING 2
#define LOG_ERROR   3


#define log_message(msg) log_log(LOG_MESSAGE,msg)
#define log_warning(msg) log_log(LOG_WARNING,msg)
#define log_error(msg)   log_log(LOG_ERROR,msg)

#define LOG_FILE   "fileti.log" //temp, i guess

void log_log(int log_level,const char *message);


#endif

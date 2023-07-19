
#ifndef __HERA_LOGGER_H__
#define __HERA_LOGGER_H__

#define LOG_TRACE_LVL1(LINE) #LINE
#define LOG_TRACE_LVL2(LINE) LOG_TRACE_LVL1(LINE)
#define LOG_THROW_LOCATION "[" __FILE__ ":" LOG_TRACE_LVL2(__LINE__) "] "

typedef enum {
    SECTOR_MAIN                   = 000,
    SECTOR_MAIN_HERA              = 001,
    // SECTOR_USER                   = 100,
    // SECTOR_USER_AGENT             = 101,
    // SECTOR_USER_PHONE             = 102,
    // SECTOR_ADMIN                  = 200,
    // SECTOR_SERVER                 = 300,
    // SECTOR_EATERY                 = 400,
    // SECTOR_EATERY_DISH            = 401,
    // SECTOR_EATERY_REVIEW          = 402,
    // SECTOR_DETAIL                 = 500,
    SECTOR_LENGTH                 = 100,
} Sector; 


typedef enum {
    LF_VERB,
    LF_INFO,
    LF_WARN,
    LF_DBUG,
    LF_EROR,
    LF_BRAK,
} Flag;

#define log_verbose(...)  logger(LS, LF_VERB,  __VA_ARGS__)
#define log_info(...)     logger(LS, LF_INFO,  __VA_ARGS__)
#define log_warn(...)     logger(LS, LF_WARN,  __VA_ARGS__)
#define log_debug(...)    logger(LS, LF_DBUG,  __VA_ARGS__)
#define log_error(...)    logger(LS, LF_EROR,  __VA_ARGS__)
#define log_trace(...)    logger(LS, LF_EROR,  LOG_THROW_LOCATION __VA_ARGS__)
#define log_break()       logger(LS, LF_BRAK,  "")

void logger(const Sector index, const Flag flag, const char *format, ...);


#endif // __HERA_LOGGER_H__

#ifndef TinyLogger_H
#define	TinyLogger_H

#include <iomanip>
#include <string>
#include <sstream>
#include <assert.h>

#if defined(WIN32)
#include <windows.h>
#elif defined(__unix__)
#include <pthread.h>
#endif  //defined (WIN32)

namespace tinylogger
{
using uint32_t = unsigned int;

const unsigned int bufferSize = 10000000;

constexpr unsigned int tinyLoggerMaxFileSize = 2 * 1e7;

enum WriteState {
    InFile,
    InMemory,
    InOut,
    InFileAndOut
};
enum MessageType{
    Info,
    Warning,
    Error,
    Fatal
};

class TinyLogger;

class TinyLoggerDestroyer
{
private:
    TinyLogger *_tinyLogger;
public:
    virtual ~TinyLoggerDestroyer();
    void initDestoyer(TinyLogger *TinyLogger);
};

class TinyLogger
{
private:
    static TinyLogger *_tinyLogger;
    static TinyLoggerDestroyer _tinyLoggerDestroyer;

protected:
    TinyLogger(std::string LogFilename_str = "program_log.txt",
               WriteState WriteState_o = InFile,
               MessageType MinOutputMsgLvl_en = Info);
    virtual ~TinyLogger();

    friend class TinyLoggerDestroyer;

public:
    static TinyLogger *instance();

    TinyLogger &info(const char *str);
    TinyLogger &info(const std::string &str);
    TinyLogger &info(const bool flag);
    TinyLogger &info(const int value);
    TinyLogger &info(const unsigned int value);
    TinyLogger &info(const float value);

    TinyLogger &warning(const char *str);
    TinyLogger &warning(const std::string &str);

    TinyLogger &error(const char *str);
    TinyLogger &error(const std::string &str);

    TinyLogger &fatal(const char *str);
    TinyLogger &fatal(const std::string &str);

    void setAssert(bool);
    void setAssert(bool, const char *str);
    void setAssert(bool, const std::string &str);

    void setMinOutputMsgLvl(MessageType newMinOutputMsgLvl);

    template <class T>
    static std::string getNormalLine(T Variable){std::stringstream ss; ss<<Variable; return ss.str();}

private:
    std::string _logFilename;

    std::string _outBuffer;

    uint32_t _writePosition;

    WriteState _writeState;

    std::string _version;

    MessageType _minOutputMsgLvl;

#if defined(WIN32)
    CRITICAL_SECTION _mutex;
#elif defined(__unix__)
    pthread_mutex_t _mutex;
#endif  //WIN32

    void lock() {
#if defined(WIN32) && defined(__MINGW32__)
        return;
#elif defined(__unix__)
        pthread_mutex_lock(&_mutex);
#else
        return;
#endif  //defined (WIN32)
    }
    //---------------
    void unLock() {
#if defined(WIN32)
        return; //
#elif defined(__unix__)
        pthread_mutex_unlock(&_mutex);
#else
        return;
#endif  //defined (WIN32)
    }

    void writeString(const std::string & infoString, MessageType messageType);

    std::string getCurrentTime();

    std::string getCurrentState(MessageType messageType);

    void saveInformation(const std::string &str);

    void saveDataInFile(std::string str);

    std::string getBeginData();

    std::string getCurrentOS();

    std::string traceStack();
};

template<class T>
std::string valueToString(T Value, int precision = 4) {
    std::stringstream stringStream;
    stringStream<<std::fixed << std::setprecision(precision) << Value;
    return stringStream.str();
}

#ifndef __STRING
#define __STRING(x) #x
#endif
#define log_assert(expr) \
    tinylogger::TinyLogger::instance()->setAssert(expr, MORE_DATA(std::string("Assert failed: ").append(__STRING(expr))))

#define MORE_DATA(A) std::string(__FILE__).append(":").append(tinylogger::TinyLogger::getNormalLine(__LINE__)).append(" >> ").append(A)
}

#endif	/* TinyLogger_H */


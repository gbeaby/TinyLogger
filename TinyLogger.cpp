#include <fstream>
#include <iostream>
#include <ctime>
#include <ios>
#include "TinyLogger.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace tinylogger
{
TinyLogger * TinyLogger::_tinyLogger = 0;

TinyLoggerDestroyer TinyLogger::_tinyLoggerDestroyer;

TinyLoggerDestroyer::~TinyLoggerDestroyer() {
    delete TinyLogger::_tinyLogger;
}

void TinyLoggerDestroyer::initDestoyer(TinyLogger * tinyLogger) {
    TinyLogger::_tinyLogger = tinyLogger;
}

TinyLogger::TinyLogger(std::string logFilename, WriteState writeState, MessageType minOutputMsgLvl) :
    _minOutputMsgLvl(minOutputMsgLvl) {
#if defined(WIN32) && !defined(__MINGW32__)
    initializeCriticalSection(&_mutex);
#elif defined(__unix__)
    pthread_mutex_init(&_mutex, NULL);
#endif  //WIN32
    _logFilename = logFilename;
    _outBuffer.reserve(bufferSize);
    _writeState = writeState;
#ifdef WIN32
    _writeState = InFile;
#endif
    _writePosition = 0;
    if(_writeState == InMemory || _writeState == InFile){
        std::ofstream File_o;
        File_o.open(_logFilename.c_str(), std::ios::app);
        if(File_o.fail()) {
#ifdef __unix__
            _logFilename = std::string("/tmp/").append(_logFilename);
            File_o.open(_logFilename.c_str(), std::ios::app);
#endif
        }
        File_o.close();
        saveInformation(getBeginData());
    }
}

TinyLogger::~TinyLogger() {
    info("Success closing log file.");
    if(_writeState == InMemory){
        saveDataInFile(_outBuffer);
        _outBuffer.clear();
    }
#if defined(WIN32)
    DeleteCriticalSection(&_mutex);
#elif defined(__unix__)
    pthread_mutex_destroy(&_mutex);
#endif  //WIN32
}

TinyLogger *TinyLogger::instance() {
    if(!_tinyLogger){
        _tinyLogger = new TinyLogger();
        _tinyLoggerDestroyer.initDestoyer(_tinyLogger);
    }
    return _tinyLogger;
}

TinyLogger& TinyLogger::info(const char* str) {
    writeString(std::string(str), Info);
    return *this;
}

TinyLogger& TinyLogger::info(const std::string &str) {
    writeString(str, Info);
    return *this;
}

TinyLogger& TinyLogger::info(const bool Flag_b) {
    writeString(valueToString(Flag_b), Info);
    return *this;
}

TinyLogger& TinyLogger::info(const int Value_i) {
    writeString(valueToString(Value_i), Info);
    return *this;
}

TinyLogger& TinyLogger::info(const unsigned int Value_ui) {
    writeString(valueToString(Value_ui), Info);
    return *this;
}

TinyLogger& TinyLogger::info(const float Value_f) {
    writeString(valueToString(Value_f), Info);
    return *this;
}

TinyLogger& TinyLogger::warning(const char* str) {
    writeString(std::string(str), Warning);
    return *this;
}

TinyLogger& TinyLogger::warning(const std::string &str) {
    writeString(str, Warning);
    return *this;
}

TinyLogger& TinyLogger::error(const char* str) {
    writeString(std::string(str), Error);
    return *this;
}

TinyLogger& TinyLogger::error(const std::string &str) {
    writeString(str, Error);
    return *this;
}

TinyLogger& TinyLogger::fatal(const char* str) {
    writeString(std::string(str), Fatal);
    return *this;
}

TinyLogger& TinyLogger::fatal(const std::string &str) {
    writeString(str, Fatal);
    return *this;
}

void TinyLogger::setAssert(bool isPass) {
    if(!isPass) {
        info(traceStack());
        fatal(MORE_DATA(std::string("Assert failed: ")));
        assert(isPass);
    }
}

void TinyLogger::setAssert(bool isPass, const char *str) {
    if(!isPass) {
        info(traceStack());
        fatal(MORE_DATA(str));
        assert(isPass);
    }
}

void TinyLogger::setAssert(bool isPass, const std::string &str) {
    if(!isPass) {
        info(traceStack());
        fatal(MORE_DATA(str));
        assert(isPass);
    }
}

void TinyLogger::setMinOutputMsgLvl(MessageType newMinOutputMsgLvl) {
    _minOutputMsgLvl = newMinOutputMsgLvl;
}

void TinyLogger::writeString(const std::string & infoString, MessageType messageType) {
    if(_minOutputMsgLvl > messageType) {
        return;
    }
    std::string currentString = getCurrentTime();
    currentString.append(getCurrentState(messageType));
    currentString.append(infoString);
    saveInformation(currentString);
}

std::string TinyLogger::getCurrentTime() {
    char buffer[80];
    time_t seconds = time(NULL);
    tm* timeinfo = localtime(&seconds);
    strftime(buffer, 80, "[%d.%m.%Y %H:%M:%S] ", timeinfo);
    return std::string(buffer);
}

std::string TinyLogger::getCurrentState(MessageType MessageType) {
    std::string messageType;
    switch(MessageType){
    case Info:
        messageType.append("\t\t(II) ");
        break;
    case Warning:
        messageType.append("\t\t(WW) ");
        break;
    case Error:
        messageType.append("\t(EE) ");
        break;
    case Fatal:
        messageType.append("(FF) ");
        break;
    }
    return messageType;
}

void TinyLogger::saveInformation(const std::string & str) {
    switch(_writeState) {
    case InOut:{
        std::cout<<str<<std::endl;
    } break;
    case InMemory: {
        this->lock();
        if(_outBuffer.size() + str.size() >= bufferSize) {
#ifndef __NO_TinyLogger__
            std::cout<<str<<std::endl;
            saveDataInFile(_outBuffer);
#endif // __NO_TinyLogger__
            _outBuffer.clear();
        }
        _outBuffer.append(str);
        _outBuffer.append("\n");
        this->unLock();
    } break;
    case InFile:{
        this->lock();
#ifndef __NO_TinyLogger__
        saveDataInFile(str);
#endif // __NO_TinyLogger__
        this->unLock();
    } break;
    case InFileAndOut:{
        this->lock();
#ifndef __NO_TinyLogger__
        std::cout << str << std::endl;
        saveDataInFile(str);
#endif // __NO_TinyLogger__
        this->unLock();
    } break;
    }
}

void TinyLogger::saveDataInFile(std::string str) {
    std::ofstream File_o;
    File_o.open(_logFilename.c_str(), std::ios::app);
    if(File_o.tellp() >= tinyLoggerMaxFileSize) {
        File_o.close();
        File_o.open(_logFilename.c_str(), std::ios::trunc);
        File_o<<getBeginData()<<"\n";
    }
    File_o<<str.c_str()<<std::endl;
    File_o.close();
}

std::string TinyLogger::getBeginData() {
#if defined(__unix__) || defined(__gnu_linux__)
    char fullBinaryName[256];
    fullBinaryName[readlink("/proc/self/exe", fullBinaryName, sizeof(fullBinaryName))] = '\0';
    struct stat binaryParameters;
    stat(fullBinaryName, &binaryParameters);
    std::string str("\n\n\n\n\nName: ");
    str.append(fullBinaryName).append("\n");
    str.append("Version: ").append(_version).append("\n");
    tm* timeInfo = localtime(&binaryParameters.st_mtim.tv_sec);
    strftime(fullBinaryName, sizeof(fullBinaryName), "%d.%m.%Y %H:%M:%S ", timeInfo);
    str.append("Build on: ").append(fullBinaryName).append("\n");
    str.append("Based on: ").append(__VERSION__).append("\n\n");
    str.append("Markers: (II) informational, (WW) warning, (EE) error, (FF) fatal error.\n");
    str.append("Current Operating System: ").append(getCurrentOS()).append("\n");
    str.append("Runned at: ").append(getCurrentTime()).append("\n");
    return str;
#else
    char fullBinaryName[256];
    GetModuleFileNameA(NULL, fullBinaryName, sizeof(fullBinaryName));
    std::string str("Name: ");
    str.append(fullBinaryName).append("\n");
    str.append("Version: ").append(_version).append("\n");
    SYSTEMTIME SystemTime_o;
    GetLocalTime(&SystemTime_o);
    wsprintfA(fullBinaryName, "%d-%02d-%02d", SystemTime_o.wYear, SystemTime_o.wMonth, SystemTime_o.wDay);
    str.append("Build on: ").append(fullBinaryName).append("\n");
    str.append("Based on: ").append(__VERSION__).append("\n\n");
    str.append("Markers: (II) informational, (WW) warning, (EE) error, (FF) fatal error.\n");
    str.append("Current Operating System: ").append(getCurrentOS()).append("\n");
    str.append("Runned at: ").append(getCurrentTime()).append("\n");
    return str;
#endif
}

std::string TinyLogger::getCurrentOS() {
    std::string str;
#ifdef __gnu_linux__
    str = "Linux";
#elif __unix__
    str = "Unix";
#elif WIN32
    str = "Windows";
#endif
    return str;
}
std::string TinyLogger::traceStack() {
    std::stringstream outStream;
    return outStream.str();
}

std::string IntToStr_str(int value, int dev, int fill) {
    std::stringstream ss;
    switch (dev) {
    case 8:
        ss << std::oct << value;
        break;
    case 10:
        ss << std::setw(fill) << std::setfill('0') <<value;
        break;
    case 16:
        ss << std::hex << value;
        break;
    default:
        return "inf";
    }
    return ss.str();
}

std::string FloatToStr_str(float value, int right) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(right) << value;
    return ss.str();
}

}

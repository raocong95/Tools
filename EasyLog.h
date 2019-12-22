//
//  EasyLog.h
//
//  Created by sollyu on 14/11/20.
//  Copyright (c) 2014年 sollyu. All rights reserved.
//

/**
 事例代码: 
     // 设置onLogChange回调函数
     EasyLog::GetInstance()->onLogChange = [=](EasyLog::LOG_LEVEL level, std::string logText) -> void
     {
        std::cout << level << " " << logText;
     };
 
     LOGI("i'm %s", "sollyu");          // 输出 INFO (只有 LOGI 不会打印所在行)
     LOGE("I'm " << "sollyu");          // 输出 ERROR (会打印所在行)
     LOG_DEBUG("i'm %s", "sollyu");     // 输出 DEBUG (会打印所在行)
     EasyLog::GetInstance()->WriteLog(EasyLog::LOG_DEBUG, "i'm %s", "sollyu");
 
 // 上面代码的执行结果
 2 [2014-11-26 11:10:54 LOG_INFO ] i'm sollyu
 4 [2014-11-26 11:10:54 LOG_ERROR] I'm sollyu (/Projects/EasyLog/EasyLog/main.cpp : main : 20 )
 1 [2014-11-26 11:10:54 LOG_DEBUG] i'm sollyu (/Projects/EasyLog/EasyLog/main.cpp : main : 21 )
 1 [2014-11-26 11:10:54 LOG_DEBUG] i'm sollyu
 
 */

#ifndef ___EasyLog_h
#define ___EasyLog_h

#include <string>
#include <sstream>
#include <fstream>
#include <functional>
#include <codecvt>
#include <iomanip>

#ifndef EASY_LOG_FILE_NAME
#  define EASY_LOG_FILE_NAME			"EasyLog.log"   /** 日志的文件名 */
#endif

#ifndef EASY_LOG_LINE_BUFF_SIZE
#  define EASY_LOG_LINE_BUFF_SIZE		1024            /** 一行的最大缓冲 */
#endif

#ifndef EASY_LOG_DISABLE_LOG
#  define EASY_LOG_DISABLE_LOG          0               /** 非0表示禁用LOG - 非0情况下依然会触发onLogChange */
#endif

#ifdef WIN32
#else
#   define  sprintf_s   sprintf
#   define  vsnprintf_s vsnprintf
#endif

/** 写日志方法 */
#define WRITE_LOG(LEVEL, FMT, ...) \
{ \
    std::stringstream ss; \
    ss << FMT; \
    if (LEVEL != EasyLog::LOG_INFO) \
    { \
        ss << " (" << __FILE__ << " : " << __FUNCTION__ << " : " << __LINE__ << " )"; \
    } \
    EasyLog::GetInstance()->WriteLog(LEVEL, ss.str().c_str(), ##__VA_ARGS__); \
}

//! 快速宏
#define LOG_TRACE(FMT , ...) WRITE_LOG(EasyLog::LOG_TRACE, FMT, ##__VA_ARGS__)
#define LOG_DEBUG(FMT , ...) WRITE_LOG(EasyLog::LOG_DEBUG, FMT, ##__VA_ARGS__)
#define LOG_INFO(FMT  , ...) WRITE_LOG(EasyLog::LOG_INFO , FMT, ##__VA_ARGS__)
#define LOG_WARN(FMT  , ...) WRITE_LOG(EasyLog::LOG_WARN , FMT, ##__VA_ARGS__)
#define LOG_ERROR(FMT , ...) WRITE_LOG(EasyLog::LOG_ERROR, FMT, ##__VA_ARGS__)
#define LOG_ALARM(FMT , ...) WRITE_LOG(EasyLog::LOG_ALARM, FMT, ##__VA_ARGS__)
#define LOG_FATAL(FMT , ...) WRITE_LOG(EasyLog::LOG_FATAL, FMT, ##__VA_ARGS__)

#define LOGT( FMT , ... ) LOG_TRACE(FMT, ##__VA_ARGS__)
#define LOGD( FMT , ... ) LOG_DEBUG(FMT, ##__VA_ARGS__)
#define LOGI( FMT , ... ) LOG_INFO (FMT, ##__VA_ARGS__)
#define LOGW( FMT , ... ) LOG_WARN (FMT, ##__VA_ARGS__)
#define LOGE( FMT , ... ) LOG_ERROR(FMT, ##__VA_ARGS__)
#define LOGA( FMT , ... ) LOG_ALARM(FMT, ##__VA_ARGS__)
#define LOGF( FMT , ... ) LOG_FATAL(FMT, ##__VA_ARGS__)

/* wstring转string方法 */
#define WS2A(ws)		  EasyLog::WS2S(ws)

class EasyLog
{
public:
    /** 日志级别*/
	enum LOG_LEVEL { LOG_TRACE = 0, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_ALARM,  LOG_FATAL };
    
public:
    /** 单例模式 */
    static EasyLog * GetInstance() { static EasyLog* m_pInstance = new EasyLog(); return m_pInstance; }
    
public:
	std::function<void(LOG_LEVEL level, std::string logText)> onLogChange;
    
public:
	static std::string WS2S(const std::wstring& ws) { return WS2S(ws.c_str()); }
	static std::string WS2S(const wchar_t *pWcsStr)
	{
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        std::string narrowStr = conv.to_bytes(pWcsStr);
        return narrowStr;
	}

    /** 写日志操作 */
	void WriteLog(LOG_LEVEL level, const char *pLogText, ...)
	{
		va_list args;
		char logText[EASY_LOG_LINE_BUFF_SIZE] = { 0 };
		va_start(args, pLogText);
		vsnprintf_s(logText, EASY_LOG_LINE_BUFF_SIZE - 1, pLogText, args);
		WriteLog(logText, level);
	}

	void WriteLog(std::string logText, LOG_LEVEL level = LOG_ERROR)
	{
		static const char *const LOG_STRING[] =
		{
			"LOG_TRACE",
			"LOG_DEBUG",
			"LOG_INFO ",
			"LOG_WARN ",
			"LOG_ERROR",
			"LOG_ALARM",
			"LOG_FATAL",
		};

		// 添加时间信息
        std::time_t t  = std::time(NULL);
        std::tm     tm = *std::localtime(&t);
        
		// 生成一行LOG字符串
        std::stringstream szLogLine;
        szLogLine << "[" << std::put_time(&tm,"%Y-%m-%d %H:%M:%S") << " " << LOG_STRING[level] << "] " << logText;
		    
		#ifdef WIN32
        szLogLine << "\r\n";
		#else
        szLogLine << "\n";
		#endif
        
#if defined EASY_LOG_DISABLE_LOG && EASY_LOG_DISABLE_LOG == 0
		/* 输出LOG字符串 - 文件打开不成功的情况下按照标准输出 */
		if (m_fileOut.is_open())
		{
		    m_fileOut.write(szLogLine.str().c_str(), szLogLine.str().size());
		    m_fileOut.flush();
		}
		else
		{
		    std::cout << szLogLine.str() << std::endl;
		}
#endif
        
		/* 使用CallBack方式调用回显 */
		if (onLogChange)
		{
			onLogChange(level, szLogLine.str());
		}
	}

private:
    EasyLog(void)
	{
		m_fileOut.open(EASY_LOG_FILE_NAME, std::ofstream::out);
		WriteLog("------------------ LOG SYSTEM START ------------------ ", EasyLog::LOG_INFO);
	}
    virtual ~EasyLog(void) 
	{
		WriteLog("------------------ LOG SYSTEM END ------------------ ", EasyLog::LOG_INFO);
		if (m_fileOut.is_open()) m_fileOut.close();
	}
    
private:
    /** 写文件 */
    std::ofstream m_fileOut;
};

#endif  /** ___EasyLog_h */
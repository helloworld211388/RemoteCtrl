
// RemoteClient.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"		// 主符号


// CRemoteClientApp:
// 有关此类的实现，请参阅 RemoteClient.cpp
//

class CRemoteClientApp : public CWinApp//继承这个类以后，可以实现windows消息处理
{
public:
	CRemoteClientApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CRemoteClientApp theApp;//对于变量，只要有external就是声明，不会分配内存

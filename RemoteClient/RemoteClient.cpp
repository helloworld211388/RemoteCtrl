#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "ClientController.h"


//启用内存调试，以后可以报告，每次new发生在哪个文件的第几行
#ifdef _DEBUG
#define new DEBUG_NEW
#endif



//当点击帮助按钮时，会发送ID_HELP信号，从而出发OnHelp函数
BEGIN_MESSAGE_MAP(CRemoteClientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CRemoteClientApp 构造

CRemoteClientApp::CRemoteClientApp()
{
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

}


// 唯一的 CRemoteClientApp 对象
CRemoteClientApp theApp;//对于变量，只要没有extern 就是定义，会分配内存

//对于函数，主要没有{}就是声明，有就是定义


// CRemoteClientApp 初始化

BOOL CRemoteClientApp::InitInstance()
{
	//初始化通用控件
	INITCOMMONCONTROLSEX InitCtrls;//配置结构体
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;//启用win95风格的控件
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();//调用父类的初始化逻辑
	AfxEnableControlContainer();//允许在对话框中采用ActiveX控件

	// 创建 shell 管理器，为对话框中可能出现的文件浏览器相关控件（如树形视图、列表视图）提供支持
	CShellManager *pShellManager = new CShellManager;

	// 激活“Windows Native”视觉管理器，以便在 MFC 控件中启用主题
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	//设置Windows注册表的根键位置，用于保存应用程序设置
	//MFC会在注册表中自动保存用户偏好设置，这行代码指定保存位置。
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

	//1.初始化应用程序的业务逻辑控制器
	//2.调用控制器的 Invoke 方法显示主窗口 / 对话框
    //3.获取用户的响应（确定 / 取消 / 错误）
	CClientController::getInstance()->InitController();
	INT_PTR nResponse = CClientController::getInstance()->Invoke(m_pMainWnd);

	if (nResponse == IDOK)
	{
		// TODO: 在此放置处理何时用
		//  “确定”来关闭对话框的代码
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 在此放置处理何时用
		//  “取消”来关闭对话框的代码
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "警告: 对话框创建失败，应用程序将意外终止。\n");
		TRACE(traceAppMsg, 0, "警告: 如果您在对话框上使用 MFC 控件，则无法 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS。\n");
	}

	// 删除上面创建的 shell 管理器。
	if (pShellManager != nullptr)
	{
		delete pShellManager;
		TRACE("shell manager has deleted!");
	}
	//清理资源
#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}


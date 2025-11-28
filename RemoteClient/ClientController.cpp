#include "pch.h"
#include "ClientSocket.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC>
 CClientController::m_mapFunc;

CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	//初始化实例，并且手动构建命令映射表，而不是使用MFC的宏
	//灵活：可以在运行时动态添加/移除映射
	//非侵入式：不需要改变类的定义
	if (m_instance == NULL) {
		m_instance = new CClientController();
		TRACE("CClientController size is %d\r\n", sizeof(*m_instance));;
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = 
		{ 
			{WM_SHOW_STATUS,&CClientController::OnShowStatus},
			{WM_SHOW_WATCH,&CClientController::OnShowWatcher},
			{(UINT) - 1,NULL}
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs
				[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	//创建线程以监听处理消息
	m_hThread = (HANDLE)_beginthreadex(
		NULL, 0,
		&CClientController::threadEntry,
		this, 0, &m_nThreadID);//CreateThread
	//显示状态对话框
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

//打开并显示远程服务器对话框，然后让用户与它互动
int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

//LRESULT CClientController::SendMessage(MSG msg)
//{
//	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//	if (hEvent == NULL) return -2;
//	MSGINFO info(msg);
//	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,(WPARAM)&info,
//		(LPARAM) hEvent);
//	WaitForSingleObject(hEvent, INFINITE);
//	CloseHandle(hEvent);
//	return info.result;
//}

//hwnd是用来接受服务器响应的窗口句柄，bAutoClose用来控制服务器超时自动关闭
bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength,WPARAM wParam)
{
	TRACE("cmd:%d %s start %lld \r\n", nCmd,__FUNCTION__, GetTickCount64());
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->SendPacket(hWnd,CPacket(nCmd, pData, nLength),bAutoClose,wParam);
	return ret;
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);//隐藏状态对话框
	m_remoteDlg.EndWaitCursor();//结束等待光标，将沙漏光标改回正常鼠标
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));//弹出消息窗口
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);//创建一个远程对话框的子窗口
	if (dlg.DoModal() == IDOK) {//以模态对话框的形式展示此窗口，此时无法与其父窗口交互
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		//创建一个文件流
		FILE* pFile = _wfopen(m_strLocal, L"wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));
			return -1;
		}
	
        CT2A strRemoteA(m_strRemote); // 需要包含 <atlconv.h>
		//pFile将在客户端收到服务端发送过来的数据之后填充
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)strRemoteA, strlen(strRemoteA), (WPARAM)pFile);

		//调整窗口和指针
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

//开始监视远程服务器
void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
	m_watchDlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
	Sleep(50);//系统休息50毫秒
	ULONGLONG nTick = GetTickCount64();//记录当前系统的时间戳
	while (!m_isClosed) {//判断当前监视是否关闭
		if (m_watchDlg.isFull() == false) {//判断当前对话框缓冲区是不是满了
			if (GetTickCount64() - nTick < 200) {//检查当前时间距离上次获取屏幕截图过去了是否有200毫秒，如果没有，则将继续睡眠等待
				Sleep(200 - DWORD(GetTickCount64() - nTick));
			}
			nTick = GetTickCount64();//记录当前时间戳
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);
			//TODO:添加消息响应函数 WM_SEND_PACK_ACK
			//TODO:控制发送频率
			if (ret == 1) {
				//TRACE("成功发送请求图片命令\r\n");
			}
			else {
				TRACE("获取图片失败！ret = %d\r\n", ret);
			}
		}
		Sleep(1);//如果对话框满了，每间隔1ms检查一次是否变空
	}
	TRACE("thread end %d\r\n", m_isClosed);
}

void  CClientController::threadWatchScreen (void* arg)//中介函数，是一个静态成员函数，在.h文件中有声明
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {//取得消息 msg（消息结构体）、NULL（接收所有窗口的消息）、0, 0（接收所有消息类型）  当收到 WM_QUIT 消息时返回 FALSE，循环才会结束
		TranslateMessage(&msg);//翻译消息 将虚拟键码转换为字符消息（如按下 'A' 键转换为字符 WM_CHAR）
		DispatchMessage(&msg);//分发消息 将消息发送到对应窗口的消息处理函数
		if (msg.message == WM_SEND_MESSAGE) {//这个是客户端自己的其它窗口发来的消息，对方会等待结果。实际上，这个信号是被废弃了，没有人发过
			MSGINFO* pmsg = (MSGINFO*)msg.wParam; // 解析消息参数
			HANDLE hEvent = (HANDLE)msg.lParam; // 解析事件句柄，因为其它窗口要知道结果，所以msg.lParam被用来放事件句柄，所以和其它信号处理时的消息处理逻辑不一样
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam,pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);  // ⭐ 信号灯，通知原线程"我处理完了"
		}
		else {//这是服务端发来的信号，因为服务端是异步处理客户端请求的，不会等待客户端的处理结果
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);//wParam是 CPacket*，而lParam是文件句柄，不是用来获取完成通知的
			}
		}
		

	}
}


//中介函数
unsigned _stdcall
CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

//显示状态窗口
LRESULT CClientController::OnShowStatus(UINT nMsg, 
	WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

//显示监视窗口
LRESULT CClientController::OnShowWatcher(UINT nMsg, 
	WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}

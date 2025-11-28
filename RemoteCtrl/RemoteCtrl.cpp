// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "EdoyunTool.h"
#include <conio.h>
#include "CEdoyunQueue.h"
#include <MSWSock.h>
#include "EdoyunServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象
// define  INVOKE_PATH _T("C::\\Windows\\SysWOW64\\RemoteCtrl.exe")
#define  INVOKE_PATH _T("C:\\Users\\difeng\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

CWinApp theApp;
using namespace std;

//业务和通用

//用户判断是否需要使用此程序
bool ChooseAutoInvoke(const CString& strPath) {
    TCHAR wcsSystem[MAX_PATH] = _T("");
    if (PathFileExists(strPath)) {
        return true;
    }

    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    CString strInfo = _T("该程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if(ret == IDYES){
        if (!CEdoyunTool::WriteStartupDir(strPath)) 
        {
            MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}

void iocp();
void udp_server();
void udp_client(bool ishost = true);

int main(int argc, char* argv[])
{
	//初始化MFC框架以使用它的功能
    if (!CEdoyunTool::Init()) return 1;  

	//启动服务端
    iocp();
	clearsock();
    return 0;
}


void iocp()//启动服务端程序
{
    EdoyunServer server;
    server.StartService();
    getchar();
}



void clearsock() {
    WSACleanup();
}
void udp_server()
{
	//udp比tcp少了listen和accept，直接使用sendto发送数据，recvfrom接受数据
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return;
    }
    std::list<sockaddr_in>lstclients;
    sockaddr_in server, client;
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));
    server.sin_family = AF_INET;
    server.sin_port = htons(20000); //UDP端口尽量设置的大一些 10000以上  
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (-1 == bind(sock, (sockaddr*)&server, sizeof(server))) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        closesocket(sock);
        return;
    }
    char buf[4096] = "";
	int len = sizeof(client);
    int ret = 0;
    while (!_kbhit()) {
        ret = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&client, &len);
        if (ret > 0) {
            lstclients.push_back(client);
            CEdoyunTool::Dump((BYTE*)buf, ret);
            printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__ ,client.sin_addr.s_addr, client.sin_port);
            ret = sendto(sock, buf, ret, 0, (sockaddr*)&client, len);
            printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
        }
        else {
            printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(),ret);
        }
    }
    closesocket(sock);
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);

}
void udp_client(bool ishost) 
{
    Sleep(2000);
    sockaddr_in server, client;
    int len = sizeof(client);  // ✓ 正确初始化大小
    
    memset(&server, 0, sizeof(server));  // ✓ 清空
    memset(&client, 0, sizeof(client));  // ✓ 清空 client
    
    server.sin_family = AF_INET;
    server.sin_port = htons(20000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR!!!\r\n", __FILE__, __LINE__, __FUNCTION__);
        return;
    }
    
    if (ishost) {
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
        std::string msg = "hello world!\n";
        int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
        printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
        
        if (ret > 0) {
            char buf[4096] = {};  // ✓ 用单独的缓冲区接收
            ret = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&client, &len);
            printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
            
            if (ret > 0) {
                printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, 
                       client.sin_addr.s_addr, client.sin_port);
            }
        }
    }
    else {//从客户端代码
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    }
}

// LockDialog.cpp: 实现文件
//这个类目前没有增加任何实现，只是复用了cdialog，它的作用是在服务端进行锁屏操作以后，用这个窗口覆盖整个屏幕 
#include "pch.h"
#include "RemoteCtrl.h"
#include "afxdialogex.h"
#include "LockDialog.h"


// CLockDialog 对话框

IMPLEMENT_DYNAMIC(CLockDialog, CDialog)

CLockDialog::CLockDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_INFO, pParent)
{

}

CLockDialog::~CLockDialog()
{
}

void CLockDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLockDialog, CDialog)
END_MESSAGE_MAP()


// CLockDialog 消息处理程序

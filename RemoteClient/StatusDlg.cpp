// StatusDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "StatusDlg.h"


// CStatusDlg 对话框

IMPLEMENT_DYNAMIC(CStatusDlg, CDialog)

CStatusDlg::CStatusDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_STATUS, pParent)//IDD_DLG_STATUS定义对话框布局,其它ui操作在domodle中实现
{

}

CStatusDlg::~CStatusDlg()
{
}

void CStatusDlg::DoDataExchange(CDataExchange* pDX)//这个函数就是将ui和程序变量绑定，比如文本框的文字和变量m_info
{
	CDialog::DoDataExchange(pDX);//pdx保存了操作类型：是从ui读到程序变量，还是从程序变量写到ui上
	DDX_Control(pDX, IDC_EDIT_INFO, m_info);
		//IDC_EDIT_INFO - 对话框资源中定义的控件ID（在.rc 资源文件中）
		//m_info - 你类中的成员变量（CEdit 类型）
		//这条语句将两者"绑定"起来，建立对应关系
}


BEGIN_MESSAGE_MAP(CStatusDlg, CDialog)//这个函数的作用是开始创建消息映射表，传入第二个参数是：如果某个消息在这个表里没有找到，就去父类的消息映射表里查找
END_MESSAGE_MAP()


// CStatusDlg 消息处理程序

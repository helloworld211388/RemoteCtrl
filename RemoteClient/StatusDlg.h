#pragma once
#include "afxdialogex.h"


// CStatusDlg 对话框

class CStatusDlg : public CDialog
{
	DECLARE_DYNAMIC(CStatusDlg)// 允许运行时识别这是 CStatusDlg 对话框

public:
	CStatusDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CStatusDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_STATUS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()// 为消息处理做准备
public:
	CEdit m_info;
};

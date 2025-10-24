#include "pch.h"
#include "EdoyunServer.h"
#include "EdoyunTool.h"

#pragma warning(disable:4407)
template<EdoyunOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
        m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
        m_operator = EAccept;
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
        m_server = NULL;
}
template<EdoyunOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
    INT lLength = 0, rLength = 0;
    if (m_client->GetBufferSize() > 0) {
        sockaddr* plocal = NULL, *promote = NULL;
        GetAcceptExSockaddrs(*m_client, 0,
            sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            (sockaddr**)&plocal, &lLength, //本地地址
            (sockaddr**)&promote, &rLength//远程地址
        );
        memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));
        memcpy(m_client->GetRemoteAddr(),promote, sizeof(sockaddr_in));
        m_server->BindNewSocket(*m_client);
        int ret = WSARecv((SOCKET)*m_client,m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
        if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
            //TODO:报错
            TRACE("ret = %d error = %d\r\n", ret, WSAGetLastError());
        }
        if (!m_server->NewAccept())
        {
            return -2;
        }
    }
    return -1;
}
template<EdoyunOperator op>
inline SendOverlapped<op>::SendOverlapped(){
    m_operator = op;
    m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}
template<EdoyunOperator op>
inline RecvOverlapped<op>::RecvOverlapped(){
    m_operator = op;
    m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}
EdoyunClient::EdoyunClient() 
    :m_isbusy(false),  m_flags(0),
    m_accept(new LPACPOVERLAPPED()),
    m_recv(new RECVOVERLAPPED()),
    m_send(new SENDOVERLAPPED())
{
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof(m_laddr));
    memset(&m_raddr, 0, sizeof(m_raddr));
}

void EdoyunClient::SetOverlapped(EdoyunClient* ptr) {
    m_accept->m_client = ptr;
    m_recv->m_client = ptr;
    m_send->m_client = ptr;
}
EdoyunClient::operator LPOVERLAPPED() {

    return &m_accept->m_overlapped;
}

LPWSABUF EdoyunClient::RecvWSABuffer()
{
    return &m_recv->m_wsabuffer;
}

LPWSAOVERLAPPED EdoyunClient::RecvOverlapped()
{
    return &m_recv->m_overlapped;
}

LPWSABUF EdoyunClient::SendWSABuffer()
{
    return &m_send->m_wsabuffer;
}

LPWSAOVERLAPPED EdoyunClient::SendOverlapped()
{
    return &m_send->m_overlapped;
}

int EdoyunClient::Recv()
{
    int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
    if (ret <= 0) return -1;
    m_used += (size_t)ret;
    CEdoyunTool::Dump((BYTE*)m_buffer.data(), ret);

    return ret;
}

int EdoyunClient::Send(void* buffer, size_t nSize)//它的这个send操作看不出有任何作用
{

    return 0;
}

int EdoyunClient::SendData(std::vector<char>& data)
{

    return 0;
}
EdoyunServer::~EdoyunServer()
{
    closesocket(m_sock);
    std::map<SOCKET, PCLIENT>::iterator it = m_client.begin();
    for (; it != m_client.end(); it++) {
        it->second.reset();
    }
    m_client.clear();
    CloseHandle(m_hIOCP);
    m_pool.Stop();
}

bool EdoyunServer::StartService()
{
	//创建并绑定套接字
    CreateSocket();
    if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }
	//监听套接字
    if (listen(m_sock, 3) == -1) {//这里listen的第二个参数已经完成了三次握手，但是还没有被accept的连接的队列长度（全连接队列）
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }
	//创建iocp
    m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
    if (m_hIOCP == NULL) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        m_hIOCP = INVALID_HANDLE_VALUE;
        return false;
    }
	//将套接字注册到iocp中，将来它会监听sock所进行的动作
    CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
    //启动线程池，以监听iocp的连接
	m_pool.Invoke();
    m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
    //判断能否提交一个异步接受连接的请求
	if (!NewAccept()) return false;
    return true;
}


void EdoyunServer::CreateSocket()
{

	//创建异步套接字
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 2), &WSAData);
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);


	//允许套接字能快速绑定到处于timewait状态的端口，以实现快速重启
    int opt = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
}

int EdoyunServer::threadIocp()
{
    DWORD tranferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* lpOverlapped = NULL;
	//获取iocp通知
    if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, &lpOverlapped, INFINITE)) {
        if (CompletionKey != 0) {
			//解析通知中的异步请求
            EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
            TRACE("pOverlapped->m_operator %d \r\n",pOverlapped->m_operator);
            pOverlapped->m_server = this;
			//根据异步请求的具体类型来将其分配到具体的线程中
            switch (pOverlapped->m_operator) {
            case EAccept:
            {
                LPACPOVERLAPPED* pOver = (LPACPOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker);
            }
            break;
            case ERecv:
            {
                RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker);
            }
            break;
            case ESend:
            {
                TRACE("Send");
                SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker);
            }
            break;
            case EError:
            {
                ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker);
            }
            break;
            }
        }
        else {
            return -1;
        }
    }
    return 0;
}

//提交一个新的异步accept请求
bool EdoyunServer::NewAccept()
{
    EdoyunClient* pClient = new EdoyunClient();
    pClient->SetOverlapped(pClient);
    if (!AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient))
    {
        TRACE("%d\r\n", WSAGetLastError());
        if (WSAGetLastError() != WSA_IO_PENDING) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        
    }
    return true;
}

//将新的套接字注册
void EdoyunServer::BindNewSocket(SOCKET s)
{
    CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
}

#include "GatewaySvr.h"

//GatewaySvr* GatewaySvr::m_Instance = NULL;

GatewaySvr::GatewaySvr()
{
    m_Config.init("./config/gatewaySvr.xml");
    m_Config.parseXml();

    m_nInterval = m_Config.m_accConfigVec[0].updateFps; //loop per Xms default
    m_nCycleTick = acct_time::getCurTimeMs(); 
    MAXPKGLEN = m_Config.m_accConfigVec[0].sessionbuflen;
    SESSIONBUFLEN = m_Config.m_accConfigVec[0].maxpkglen;

    m_nStatisticTick = m_nCycleTick;
    m_nHandleCount = 0;

    m_nNextTick = m_nCycleTick + m_nInterval;
    m_ServerID = m_Config.m_accConfigVec[0].serverid;
    m_nIoThreadNum = m_Config.m_accConfigVec[0].recvThread;
    LOGI("=====e======m_nIoThreadNum:" <<  m_nIoThreadNum);
    m_svrType = eGateWay;
}

GatewaySvr::~GatewaySvr()
{

}

void GatewaySvr::start()
{
    EpollServer::start(); //create all epollfd

    m_acceptor.setSvrType(m_svrType);
    m_acceptor.setServer(this);
    m_acceptor.init(m_Config.m_accConfigVec[0].maxclient);
    m_acceptor.startListen(m_Config.m_accConfigVec[0].ip, m_Config.m_accConfigVec[0].port);
    m_acceptor.start();

    //getBestServerSession(eDBServer);

    for (int i = 0; i < m_nIoThreadNum;i++)
    {
        CIoThread *newThread = new CIoThread(this);
        newThread->setStartID(i);
        newThread->start();
    }

    //CSendThread *sendThread = new CSendThread(this);
    //sendThread->start();
    
    m_connector.start();
    
    for (uint32 i = 0; i < m_Config.m_dbConfigVec.size(); i++)
    {
        m_connector.connect(m_Config.m_dbConfigVec[i].ip, m_Config.m_dbConfigVec[i].port, m_Config.m_dbConfigVec[i].serverid);
    }
    int32 size = m_Config.m_logicConfigVec.size();
    for (int32 i = 0; i < size; i++)
    {
       m_connector.connect(m_Config.m_logicConfigVec[i].ip, m_Config.m_logicConfigVec[i].port, m_Config.m_logicConfigVec[i].serverid);
    }
    
}

void GatewaySvr::update()
{
    while (true)
    {
        while (acct_time::getCurTimeMs() >= m_nNextTick)
        {
            EpollServer::update();
            m_nNextTick = acct_time::getCurTimeMs() + m_nInterval;
            updateSessionList(); // handle new Session
            handleActiveSession();
            removeDeadSession();
        }
        acct_time::sleepMs(10); // sleep 1ms per loop
    }
}
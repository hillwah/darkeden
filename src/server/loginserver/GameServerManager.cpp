//////////////////////////////////////////////////////////////////////
//
// Filename    : GameServerManager.cpp
// Written By  : Reiot
// Description :
//
//////////////////////////////////////////////////////////////////////

#include "GameServerManager.h"
#include "Properties.h"
#include "Datagram.h"
#include "DatagramPacket.h"
////#include "LogClient.h"
#include <unistd.h>
#include "DB.h"
#include "Properties.h"

#include "Lpackets/LGKickCharacter.h"


GameServerManager::GameServerManager() throw(Error): m_pDatagramSocket(NULL) {
    __BEGIN_TRY

    // create datagram server socket
    while (true) {
        try {
            m_pDatagramSocket = new DatagramSocket(g_pConfig->getPropertyInt("LoginServerUDPPort"));
            break;
        }
        catch (BindException& be) {
            SAFE_DELETE(m_pDatagramSocket);
            cout << be.toString() << endl;
            sleep(1);
        }
    }

    __END_CATCH
}

GameServerManager::~GameServerManager() throw(Error) {
    __BEGIN_TRY

    if (m_pDatagramSocket != NULL) {
        delete m_pDatagramSocket;
        m_pDatagramSocket = NULL;
    }

    __END_CATCH
}

void GameServerManager::stop() throw(Error) {
    __BEGIN_TRY

    throw UnsupportedError();

    __END_CATCH
}

//////////////////////////////////////////////////////////////////////
// main method
//////////////////////////////////////////////////////////////////////
void GameServerManager::run() {
    try {
        string host = g_pConfig->getProperty("DB_HOST");
        string db = g_pConfig->getProperty("DB_DB");
        string user = g_pConfig->getProperty("DB_USER");
        string password = g_pConfig->getProperty("DB_PASSWORD");
        uint port = 0;
        if (g_pConfig->hasKey("DB_PORT"))
            port = g_pConfig->getPropertyInt("DB_PORT");

        Connection* pConnection = new Connection(host, db, user, password, port);
        g_pDatabaseManager->addConnection(Thread::self(), pConnection);

        while (true) {
            Datagram* pDatagram = NULL;
            DatagramPacket* pDatagramPacket = NULL;

            try {
                // 데이터그램 객체를 끄집어낸다.
                pDatagram = m_pDatagramSocket->receive();

                if (pDatagram != NULL) { // 일부 exception제거. by sigi. 2002.5.17
                    //cout << pDatagramPacket->toString() << endl;

                    /*
                    // UDP test
                    cout << "[DatagramPacket] " << pDatagram->getHost() << ":"
                                                << pDatagram->getPort() << endl;

                    if (pDatagram->getPort()!=9997)
                    {
                        LGKickCharacter lg;
                        lg.setPCName("111");
                        lg.setID(111);

                        sendPacket(pDatagram->getHost(), pDatagram->getPort(), &lg);
                    }
                    */

                    // 데이터그램 패킷 객체를 끄집어낸다.
                    pDatagram->read(pDatagramPacket);

                    if (pDatagramPacket != NULL) {

                        // 끄집어낸 데이터그램 패킷 객체를 실행한다.
                        pDatagramPacket->execute(NULL);

                        // 데이터그램 패킷 객체를 삭제한다.
                        delete pDatagramPacket;
                        pDatagramPacket = NULL;
                    }

                    // 데이터그램 객체를 삭제한다.
                    delete pDatagram;
                    pDatagram = NULL;
                }
            }
            catch (ProtocolException & pe) {
                cout << "GameServerManager::run Exception Check(ProtocolException)" << endl;
                cout << pe.toString() << endl;

                // 서버간 통신에서 프로토콜 에러가 발생하면,
                // 프로그래밍 오류이거나 해킹 시도이다.
                // 일단은 전자만이 해당되므로.. 에러로 간주한다.
                //throw Error(pe.toString());
                delete pDatagramPacket;
                delete pDatagram;
            }
            catch (ConnectException & ce) {
                cout << "GameServerManager::run Exception Check(ConnectException)" << endl;
                cout << ce.toString() << endl;

                // 으음.. 머지 이건..
                // 일단 에러당..
                //throw Error(ce.toString());
                delete pDatagramPacket;
                delete pDatagram;
            }
            catch (Throwable & t) {
                cout << "GameServerManager::run Exception Check(ConnectException)" << endl;
                cout << t.toString() << endl;
                delete pDatagramPacket;
                delete pDatagram;
            }
            usleep(100);
        }

        //cout << "GameServerManager thread exiting... " << endl;
        //::exit(1);
    }
    catch (Throwable & t) {
        //log(LOG_LOGINSERVER_ERROR, "", "", t.toString());
        cout << "GameServerManager thread exiting... : " << t.toString() << endl;
    }
}


void GameServerManager::sendDatagram(Datagram * pDatagram) throw(ConnectException, Error) {
    __BEGIN_TRY

    try {
        m_pDatagramSocket->send(pDatagram);
    }
    catch (ConnectException & t) {
        cout << "GameServerManager::sendDatagram Exception Check!!" << endl;
        cout << t.toString() << endl;
        throw ConnectException("GameServerManager::sendDatagram 상위로 던진다");
    }

    __END_CATCH
}


void GameServerManager::sendPacket(string host, uint port, DatagramPacket * pPacket) throw(ConnectException, Error) {
    __BEGIN_TRY
    __BEGIN_DEBUG

    try {
//    try
//    {
        // 데이터그램 객체를 하나 두고, 전송할 peer 의 호스트와 포트를 지정한다.
        Datagram datagram;

        datagram.setHost(host);
        datagram.setPort(port);

        // 데이터그램 패킷을 데이터그램에 집어넣는다.
        datagram.write(pPacket);

        // 데이터그램 소켓을 통해서 데이터그램을 전송한다.
        m_pDatagramSocket->send(&datagram);
//    }
//    catch (ConnectException & t )
//    {
//        cout << "GameServerManager::sendDatagram Exception Check!!" << endl;
//        cout << t.toString() << endl;
//        throw ConnectException("GameServerManager::sendDatagram 상위로 던진다");
//    }

    }
    catch (Throwable & t) {
        //cout << "====================================================================" << endl;
        cout << t.toString() << endl;
        //cout << "====================================================================" << endl;
    }

    __END_DEBUG
    __END_CATCH
}


GameServerManager * g_pGameServerManager = NULL;

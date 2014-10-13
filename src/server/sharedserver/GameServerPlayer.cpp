//////////////////////////////////////////////////////////////////////
// 
// Filename    : GameServerPlayer.cpp 
// Written By  : Reiot
// 
//////////////////////////////////////////////////////////////////////

#include "GameServerPlayer.h"
#include "Socket.h"
#include "SocketInputStream.h"
#include "SocketOutputStream.h"
#include "Assert1.h"
#include "Packet.h"
#include "PacketFactoryManager.h"

const int defaultGameServerPlayerInputStreamSize = 10240;
const int defaultGameServerPlayerOutputStreamSize = 163840;

GameServerPlayer::GameServerPlayer(Socket * pSocket) throw(Error) {//: Player(pSocket )//m_pSocket(pSocket), m_pInputStream(NULL), m_pOutputStream(NULL)
    __BEGIN_TRY

    Assert(pSocket != NULL);
    m_pSocket = pSocket;

    m_pInputStream = new SocketInputStream(m_pSocket, defaultGameServerPlayerInputStreamSize);

    Assert(m_pInputStream != NULL);
    
    m_pOutputStream = new SocketOutputStream(m_pSocket, defaultGameServerPlayerOutputStreamSize);

    Assert(m_pOutputStream != NULL);

    __END_CATCH
}

GameServerPlayer::~GameServerPlayer() throw(Error) {
    __BEGIN_TRY

  /*
    // delete socket input stream
    SAFE_DELETE(m_pInputStream);

    // delete socket output stream
    SAFE_DELETE(m_pOutputStream);

    // delete socket
    if (m_pSocket != NULL ) 
    {
        m_pSocket->close();
        delete m_pSocket;
        m_pSocket = NULL;
    }
    */

    __END_CATCH
}


void GameServerPlayer::processInput() throw(IOException, Error) {
    __BEGIN_TRY

    try {
        m_pInputStream->fill();
    }
    catch (NonBlockingIOException& nbie) {}

    __END_CATCH
}


void GameServerPlayer::processOutput() throw(IOException, Error) {
    __BEGIN_TRY

    try {
        m_pOutputStream->flush();
    }
    catch (InvalidProtocolException&) {
        throw DisconnectException("[GameServerPlayer] Problem with OutputStream->flush().");
    }

    __END_CATCH
}


void GameServerPlayer::processCommand() throw(IOException, Error) {
    __BEGIN_TRY

    try {
        // ����� �ӽ������� ���� ����
        char header[szPacketHeader];
        PacketID_t packetID;
        PacketSize_t packetSize;
        Packet * pPacket;

        // �Է¹��ۿ� ����ִ� ������ ��Ŷ���� ������ ó���Ѵ�.
        while (true) {
        
            // �Է½�Ʈ������ ��Ŷ���ũ�⸸ŭ �о��.
            // ���� ������ ũ�⸸ŭ ��Ʈ������ ���� �� ���ٸ�,
            // Insufficient ���ܰ� �߻��ϰ�, ������ ����������.
            if (!m_pInputStream->peek(header, szPacketHeader))
                break;

            // ��Ŷ���̵� �� ��Ŷũ�⸦ �˾Ƴ���.
            // �̶� ��Ŷũ��� ����� �����Ѵ�.
            memcpy(&packetID, &header[0], szPacketID);
            memcpy(&packetSize, &header[szPacketID], szPacketSize);

            // ��Ŷ ���̵� �̻��ϸ� �������� ������ �����Ѵ�.
            if (packetID >= Packet::PACKET_MAX)
                throw InvalidProtocolException("[GameServerPlayer] Invalid Packet ID.");
            
            // ��Ŷ ũ�Ⱑ �ʹ� ũ�� �������� ������ �����Ѵ�.
            if (packetSize > g_pPacketFactoryManager->getPacketMaxSize(packetID))
                throw InvalidProtocolException("[GameServerPlayer] Too Large Packet Size.");
            
            // �Է¹��۳��� ��Ŷũ�⸸ŭ�� ����Ÿ�� ����ִ��� Ȯ���Ѵ�.
            // ����ȭ�� break �� ����ϸ� �ȴ�. (���⼭�� �ϴ� exception�� �� ���̴�.)
            if (m_pInputStream->length() < szPacketHeader + packetSize)
                throw InsufficientDataException("[GameServerPlayer] Packet Size is too small");
            
            // ������� �Դٸ� �Է¹��ۿ��� ������ ��Ŷ �ϳ� �̻��� ����ִٴ� ���̴�.
            // ��Ŷ���丮�Ŵ����κ��� ��Ŷ���̵� ����ؼ� ��Ŷ ��Ʈ��ó�� �����ϸ� �ȴ�.
            // ��Ŷ���̵� �߸��� ���� ��Ŷ���丮�Ŵ������� ó���Ѵ�.
            pPacket = g_pPacketFactoryManager->createPacket(packetID);
            printf("get a packet: %d\n", packetID);

            // ���� �� ��Ŷ��Ʈ��ó�� �ʱ�ȭ�Ѵ�.
            // ��Ŷ����Ŭ������ ���ǵ� read()�� virtual ��Ŀ���� ���ؼ� ȣ��Ǿ�
            // �ڵ������� �ʱ�ȭ�ȴ�.
            m_pInputStream->readPacket(pPacket);
            
            // ���� �� ��Ŷ��Ʈ��ó�� ������ ��Ŷ�ڵ鷯�� �����ϸ� �ȴ�.
            // ��Ŷ���̵� �߸��� ���� ��Ŷ�ڵ鷯�Ŵ������� ó���Ѵ�.
            pPacket->execute(this);

            // ��Ŷ�� �����Ѵ�
            delete pPacket;
        }

    }
    catch (NoSuchElementException & nsee) {
        // PacketFactoryManager::createPacket(PacketID_t)
        // PacketFactoryManager::getPacketMaxSize(PacketID_t)
        // ���� ���� ���ɼ��� �ִ�.
        throw Error(nsee.toString());
    }
    catch (InsufficientDataException) {} 
    __END_CATCH
}


void GameServerPlayer::sendPacket(Packet * pPacket) throw(ProtocolException, Error) {
    __BEGIN_TRY

    m_pOutputStream->writePacket(pPacket);

  /*
    cout << endl;
    cout << "=== GameServerPlayer::sendPacket() ===" << endl;
    cout << pPacket->toString() << endl;
    cout << "============================" << endl;
    */

    __END_CATCH
}


void GameServerPlayer::disconnect(bool bDisconnected) throw(InvalidProtocolException, Error) {
    __BEGIN_TRY

    try {
        // �����ϰ� �α׾ƿ��� ��쿡�� ��� ���۸� �÷����� �� �ִ�.
        // �׷���, �ҹ����� �𽺸� �ɾ��ٸ� ������ �ݰ����Ƿ�
        // �÷����� ��� SIG_PIPE �� �ް� �ȴ�.
        if (bDisconnected == UNDISCONNECTED)
            m_pOutputStream->flush();

        m_pSocket->close();
    } 
    catch (InvalidProtocolException & t) {
        cerr << "[GameServerPlayer] Disconnect Exception." << endl;
        cerr << t.toString() << endl;
        m_pSocket->close();
        //throw Error("����...");
    }

    __END_CATCH
}

void GameServerPlayer::setSocket(Socket * pSocket) throw() {
    __BEGIN_TRY

    m_pSocket = pSocket;

    if (m_pInputStream != NULL) {
        delete m_pInputStream;
        m_pInputStream = new SocketInputStream(m_pSocket);
    }

    if (m_pOutputStream != NULL) {
        delete m_pOutputStream;
        m_pOutputStream = new SocketOutputStream(m_pSocket);
    }

    __END_CATCH
}


string GameServerPlayer::toString() const throw(Error) {
    __BEGIN_TRY
        
    StringStream msg;

    msg << "GameServerPlayer(SocketID: " << m_pSocket->getSOCKET() << ", Host:" << m_pSocket->getHost() << ", ID: " << m_ID << ")";

    return msg.toString();

    __END_CATCH
}

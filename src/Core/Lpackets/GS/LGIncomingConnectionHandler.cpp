//----------------------------------------------------------------------
//
// Filename    : LGIncomingConnectionHandler.cpp
// Written By  : Reiot
// Description :
//
//----------------------------------------------------------------------

// include files
#include "LGIncomingConnection.h"
#include "Properties.h"

#ifdef __GAME_SERVER__

	#include "ConnectionInfo.h"
	#include "ConnectionInfoManager.h"
	#include "LoginServerManager.h"
	#include "LogDef.h"

	#include "Gpackets/GLIncomingConnectionError.h"
	#include "Gpackets/GLIncomingConnectionOK.h"

#endif

//----------------------------------------------------------------------
// 
// LGIncomingConnectionHander::execute()
// 
// 게임 서버가 로그인 서버로부터 LGIncomingConnection 패킷을 받게 되면,
// ConnectionInfo를 새로 추가하게 된다.
// 
//----------------------------------------------------------------------
void LGIncomingConnectionHandler::execute (LGIncomingConnection * pPacket )
	 throw(ProtocolException , Error )
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

#ifdef __GAME_SERVER__

	//--------------------------------------------------------------------------------
	//
	// 인증키를 생성한다.
	//
	// *NOTE*
	//
	// 기존의 방식은 로그인 서버에서 인증키를 생성해서 게임서버에 보낸 후, 클라이언트로
	// 전송했다. 이렇게 할 경우 CLSelectPCHandler::execute()에서 인증키를 생성하고 
	// GLIncomingConnectionOKHandler::execute()에서 인증키를 클라이언트로 보내게 되는데,
	// 처리 메쏘드가 다르므로 키값이 어디에선가 유지되어야 한다. 가장 단순한 방법은 로그인
	// 플레이어 객체에 저장하면 되는데.. 뭔가 꾸지하다. 또다른 방법은 게임 서버에서 다시
	// 로그인 서버로 키값을 되돌려주는 것인데, 이는 네트워크상에 키값이 2회 왕복한다는
	// 점에서 불필요하다. 
	// 
	// 따라서, 게임 서버에서 생성해서 로그인서버에 보내주는 편이 훨씬 깔끔해지게 된다.
	//
	// *TODO*
	//
	// 최악의 경우, 로컬네트워크가 스니퍼링당해서 키값이 유출될 수 있다. (하긴 뭐 루트
	// 패스워드가 유출될 가능성도 있는데.. - -; 이런 거야 SSL을 써야 하는거고..)
	// 이를 대비해서 GLIncomingConnectionOK 패킷은 암호화되어야 한다.
	//
	// 또한 키값은 예측불가능해야 한다. (어차피 코드를 보면 예측가능해진다.)
	//
	//--------------------------------------------------------------------------------

	DWORD authKey = rand() << (time(0) % 10 ) + rand() >> (time(0)% 10);

	// CI 객체를 생성한다.
	ConnectionInfo * pConnectionInfo = new ConnectionInfo();
	pConnectionInfo->setClientIP(pPacket->getClientIP());
	pConnectionInfo->setKey(authKey);
	pConnectionInfo->setPlayerID(pPacket->getPlayerID());
	pConnectionInfo->setPCName(pPacket->getPCName());

	//--------------------------------------------------------------------------------
	//
	// 현재 시간 + 20 초 후를 expire time 으로 설정한다.
	//
	// *TODO*
	//
	// expire period 역시 Config 파일에서 지정해주면 좋겠다.
	//
	//--------------------------------------------------------------------------------
	Timeval currentTime;
	getCurrentTime(currentTime);	
	currentTime.tv_sec += 30;
	pConnectionInfo->setExpireTime(currentTime);

	// debug message
	/*
	cout << "+--------------------------------+" << endl
		 << "| Incoming Connection Infomation |" << endl
		 << "+--------------------------------+" << endl
		 << "ClientIP : " << pPacket->getClientIP() << endl
		 << "Auth Key : " << authKey << endl
		 << "P C Name : " << pPacket->getPCName() << endl;
	 */

	try {

		// CIM 에 추가한다.
		g_pConnectionInfoManager->addConnectionInfo(pConnectionInfo);

		// by sigi. 2002.12.7
		FILELOG_INCOMING_CONNECTION("connectionInfo.log", "Add [%s:%s] %s (%u)",
										pPacket->getPlayerID().c_str(),
										pPacket->getPCName().c_str(),
										pPacket->getClientIP().c_str(),
										authKey);


		// 로그인 서버에게 다시 알려준다.
		GLIncomingConnectionOK glIncomingConnectionOK;
		glIncomingConnectionOK.setPlayerID(pPacket->getPlayerID());
		glIncomingConnectionOK.setTCPPort(g_pConfig->getPropertyInt("TCPPort"));
		glIncomingConnectionOK.setKey(authKey);

		g_pLoginServerManager->sendPacket(pPacket->getHost() , pPacket->getPort() , &glIncomingConnectionOK);

		//cout << "LGIncomingConnectionHandler Send Packet to ServerIP : " << pPacket->getHost() << endl;
		//cout << "LGIncomingConnectionHandler Send Packet to ServerPort : " << pPacket->getPort() << endl;

	} catch (DuplicatedException & de ) {

		// 실패했을 경우 CI 를 삭제하고, 로그인 서버에게 GLIncomingConnectionError 패킷을 전송한다.
		SAFE_DELETE(pConnectionInfo);

//		GLIncomingConnectionError glIncomingConnectionError;
//		glIncomingConnectionError.setMessage(de.toString());
//		glIncomingConnectionError.setPlayerID(pPacket->getPlayerID());

//		cout << "Step 5" << endl;
//		g_pLoginServerManager->sendPacket(pPacket->getHost() , pPacket->getPort() , &glIncomingConnectionError);
//		cout << "LGIncomingConnectionHandler Send Packet to ServerIP : " << pPacket->getHost() << endl;
	}
	
#endif
		
	__END_DEBUG_EX __END_CATCH
}

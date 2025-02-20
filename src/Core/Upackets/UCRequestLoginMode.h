//--------------------------------------------------------------------------------
// 
// Filename    : UCRequestLoginMode.h 
// Written By  : Reiot
// 
//--------------------------------------------------------------------------------

#ifndef __UC_REQUEST_LOGIN_MODE_H__
#define __UC_REQUEST_LOGIN_MODE_H__

// include files
#include "Packet.h"
#include "PacketFactory.h"

//--------------------------------------------------------------------------------
//
// class UCRequestLoginMode;
//
//--------------------------------------------------------------------------------

class UCRequestLoginMode : public Packet {

public :
    UCRequestLoginMode() {};
    virtual ~UCRequestLoginMode() {};
	// 입력스트림(버퍼)으로부터 데이터를 읽어서 패킷을 초기화한다.
	void read (SocketInputStream & iStream ) throw(ProtocolException , Error ) { throw UnsupportedError(__PRETTY_FUNCTION__); }

    // 소켓으로부터 직접 데이터를 읽어서 패킷을 초기화한다.
    void read (Socket * pSocket ) throw(ProtocolException , Error);
		    
    // 출력스트림(버퍼)으로 패킷의 바이너리 이미지를 보낸다.
    void write (SocketOutputStream & oStream ) const throw(ProtocolException , Error ) { throw UnsupportedError(__PRETTY_FUNCTION__); }

    // 소켓으로 직접 패킷의 바이너리 이미지를 보낸다.
    void write (Socket * pSocket ) const throw(ProtocolException , Error);

	// execute packet's handler
	void execute (Player * pPlayer ) throw(ProtocolException , Error);

	// get packet id
	PacketID_t getPacketID () const throw() { return PACKET_UC_REQUEST_LOGIN_MODE; }
	
	// get packet body size
	// *OPTIMIZATION HINT*
	// const static UCRequestLoginModePacketSize 를 정의, 리턴하라.
	PacketSize_t getPacketSize () const throw() 
	{ 
		return szBYTE;
	}
	
	// 아무리 커도 백메가는 받지 못한다.
	static PacketSize_t getPacketMaxSize () throw() 
	{ 
		return szBYTE;
	}

	// get packet's name
	string getPacketName () const throw() { return "UCRequestLoginMode"; }
	
	// get packet's debug string
	string toString () const throw();


public :

	// get/set login mode
	BYTE getLoginMode() const { return m_LoginMode; }
	void setLoginMode(BYTE loginMode ) { m_LoginMode = loginMode; }

private :

	BYTE m_LoginMode;
	
};


//--------------------------------------------------------------------------------
//
// class UCRequestLoginModeFactory;
//
// Factory for UCRequestLoginMode
//
//--------------------------------------------------------------------------------

class UCRequestLoginModeFactory : public PacketFactory {

public :
	
	// create packet
	Packet * createPacket () throw() { return new UCRequestLoginMode(); }

	// get packet name
	string getPacketName () const throw() { return "UCRequestLoginMode"; }
	
	// get packet id
	PacketID_t getPacketID () const throw() { return Packet::PACKET_UC_REQUEST_LOGIN_MODE; }

	// get packet's max body size
	PacketSize_t getPacketMaxSize () const throw() { return szBYTE; }
	
};


//--------------------------------------------------------------------------------
//
// class UCRequestLoginModeHandler;
//
//--------------------------------------------------------------------------------

class UCRequestLoginModeHandler {

public :

	// execute packet's handler
	static void execute (UCRequestLoginMode * pPacket , Player * pPlayer ) throw(ProtocolException , Error);

};

#endif

//////////////////////////////////////////////////////////////////////
// 
// Filename    : GCCreatureDied.cpp 
// Written By  : Reiot
// Description : 
// 
//////////////////////////////////////////////////////////////////////

// include files
#include "GCCreatureDied.h"


//////////////////////////////////////////////////////////////////////
// 입력스트림(버퍼)으로부터 데이타를 읽어서 패킷을 초기화한다.
//////////////////////////////////////////////////////////////////////
void GCCreatureDied::read (SocketInputStream & iStream ) 
	 throw(ProtocolException , Error )
{
	__BEGIN_TRY
	BYTE flag;
    iStream.read(flag);
	iStream.read(m_ObjectID);

	__END_CATCH
}

		    
//////////////////////////////////////////////////////////////////////
// 출력스트림(버퍼)으로 패킷의 바이너리 이미지를 보낸다.
//////////////////////////////////////////////////////////////////////
void GCCreatureDied::write (SocketOutputStream & oStream ) const 
     throw(ProtocolException , Error )
{
	__BEGIN_TRY
	// oStream.write((BYTE)48);
	oStream.write(m_ObjectID);

	__END_CATCH
}


//////////////////////////////////////////////////////////////////////
// execute packet's handler
//////////////////////////////////////////////////////////////////////
void GCCreatureDied::execute (Player * pPlayer ) 
	 throw(ProtocolException , Error )
{
	__BEGIN_TRY
		
	GCCreatureDiedHandler::execute(this , pPlayer);

	__END_CATCH
}


//////////////////////////////////////////////////////////////////////
// get packet's debug string
//////////////////////////////////////////////////////////////////////
string GCCreatureDied::toString () const
       throw()
{
	__BEGIN_TRY
		
	StringStream msg;
	msg << "GCCreatureDied("
		<< "ObjectID:" << m_ObjectID 
		<< ")" ;
	return msg.toString();
		
	__END_CATCH
}



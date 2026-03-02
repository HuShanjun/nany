
#include "GammaCommon/CDomXml.h"
#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/CDictionary.h"
#include "GammaCommon/TGammaStrStream.h"

namespace Gamma
{
	inline bool IsXmlIdentity( int32 c )
	{
		return 
			( c >= 'a' && c <= 'z' ) ||
			( c >= 'A' && c <= 'Z' ) ||
			( c >= '0' && c <= '9' ) ||
			c == '_' || c == '-' || c == '.';
	}

	//=============================================
	// CDomXmlAttribute
	//=============================================
	CDomXmlAttribute::CDomXmlAttribute()
		: m_szName( NULL )
		, m_szContent( NULL )
	{
	}

	CDomXmlAttribute::~CDomXmlAttribute()
	{
	}

	bool CDomXmlAttribute::Parse( CRefStringPtr& DomXmlBuffer, size_t& nCurPos )
	{
		m_ptrBuffer = DomXmlBuffer;
		size_t nSize = m_ptrBuffer->size();
		char* szBuffer = &(*m_ptrBuffer)[0];

		m_szName = szBuffer + nCurPos;
		while( IsXmlIdentity( szBuffer[nCurPos] ) )
			if( ++nCurPos >= nSize )
				return false;

		while( IsBlank( szBuffer[nCurPos] ) )
		{
			szBuffer[nCurPos] = 0;
			if( ++nCurPos >= nSize )
				return false;
		}

		if( szBuffer[nCurPos] != '=' )
			return false;
		szBuffer[nCurPos++] = 0;

		while( IsBlank( szBuffer[nCurPos] ) )
			if( ++nCurPos >= nSize )
				return false;

		if( szBuffer[nCurPos] != '\'' && szBuffer[nCurPos] != '\"' )
			return false;

		if( ++nCurPos >= nSize )
			return false;

		while( IsBlank( szBuffer[nCurPos] ) )
			if( ++nCurPos >= nSize )
				return false;

		m_szContent = szBuffer + nCurPos;

		while( szBuffer[nCurPos] != '\'' && szBuffer[nCurPos] != '\"' )
			if( ++nCurPos >= nSize )
				return false;

		char* szEnd = szBuffer + nCurPos++;

		*szEnd = 0;
		while( szEnd != m_szContent && IsBlank( *szEnd ) )
			szEnd--;
		return true;
	}

	const char* CDomXmlAttribute::GetName() const
	{
		return m_szName;
	}

	const char* CDomXmlAttribute::GetValue() const
	{
		if( !m_szContent )
			return "";
        if( !CDictionary::HasTagHead( m_szContent ) )
			return m_szContent;
		const char* szText = CDictionary::Inst().GetValue( CDictionary::StrToKey( m_szContent + 3 ) );
		return szText ? szText : "";
	}

	void CDomXmlAttribute::SetValue( const char* szValue )
	{
		CRefStringPtr ptrBuffer = CRefStringPtr( new CRefString );
		ptrBuffer->append( m_szName );
		ptrBuffer->push_back( 0 );
		ptrBuffer->append( szValue );
		m_ptrBuffer = ptrBuffer;
		m_szName = m_ptrBuffer->c_str();
		m_szContent = m_szName + strlen( m_szName ) + 1;
	}

	CDomXmlAttribute* CDomXmlAttribute::GetNextSibling()
	{
		return GetNext();
	}

	const CDomXmlAttribute* CDomXmlAttribute::GetNextSibling() const
	{
		return GetNext();
	}

	//=============================================
	// CDomXmlDocument
	//=============================================
	CDomXmlDocument::CDomXmlDocument( const char* szName )
		: m_pParent( NULL )
		, m_szName( NULL )
		, m_szContent( NULL )
		, m_nLevel( 0 )
		, m_nChildCount( 0 )
	{
		if( szName )
		{
			m_ptrBuffer = CRefStringPtr( new CRefString );
			*m_ptrBuffer = szName;
			m_szName = m_ptrBuffer->c_str();
		}
	}

	CDomXmlDocument::CDomXmlDocument( const CDomXmlDocument& rhs )
		: m_pParent( NULL )
		, m_szName( NULL )
		, m_szContent( NULL )
		, m_nLevel( 0 )
		, m_nChildCount( 0 )
	{
		*this = rhs;
	}

	CDomXmlDocument::~CDomXmlDocument()
	{
		clear();
	}

	bool CDomXmlDocument::Load( const wchar_t* szFileName )
	{
		CPkgFile XmlFile;
		if ( !XmlFile.Open( szFileName ) )
			return false;
		if ( XmlFile.Size() == 0 )
			return false;
		return LoadFromBuffer( (const char*)XmlFile.GetFileBuffer(), XmlFile.Size() );
	}

	bool CDomXmlDocument::Load( const char* szFileName )
	{
		CPkgFile XmlFile;
		if ( !XmlFile.Open( szFileName ) )
			return false;
		if ( XmlFile.Size() == 0 )
			return false;
		return LoadFromBuffer( (const char*)XmlFile.GetFileBuffer(), XmlFile.Size() );
	}

	bool CDomXmlDocument::LoadFromBuffer( const char* pBuffer, size_t nCount )
	{
		clear();

		CRefStringPtr DomXmlBuffer( new CRefString );
		if( nCount == -1 )
			DomXmlBuffer->assign( pBuffer );
		else
			DomXmlBuffer->assign( pBuffer, nCount );

		size_t nCurPos = 0;
		if( !FindNextNode( DomXmlBuffer, nCurPos ) )
			return false;

		// 跳过XML 声明
		if( pBuffer[nCurPos + 1] == '?' )
		{
			while( true )
			{
				if( ++nCurPos >= nCount - 1 )
					return false;
				if( pBuffer[nCurPos] != '?' || pBuffer[nCurPos + 1] != '>' )
					continue;
				nCurPos = nCurPos + 2;
				break;
			}
			if( !FindNextNode( DomXmlBuffer, nCurPos ) )
				return false;
		}

		return Parse( DomXmlBuffer, nCurPos );
	}

	bool CDomXmlDocument::FindNextNode( CRefStringPtr& DomXmlBuffer, size_t& nCurPos )
	{
		m_ptrBuffer = DomXmlBuffer;
		size_t nSize = m_ptrBuffer->size();
		char* szBuffer = &( (*m_ptrBuffer)[0] );

		// 获取节点开始
		while( nCurPos < nSize )
		{
			while( szBuffer[nCurPos] != '<' )
				if( ++nCurPos >= nSize )
					return false;

			// 数据不够
			if( nCurPos + 1 >= nSize )
				return false;

			// 节点结束了
			if( szBuffer[ nCurPos + 1 ] == '/' )
				return false;

			// 不是注释则到达节点处
			if( szBuffer[ nCurPos + 1 ] != '!' )
				return true;

			// 至少应该有 <!----> 6 个字符
			if( ++nCurPos + 6 >= nSize || szBuffer[++nCurPos] != '-' || szBuffer[++nCurPos] != '-' )
				return false;

			char commentEnd[] = "-->";
			int failRet[3] = {-1, -1, 1};
			int j = 0;
			nCurPos++;
			while( nCurPos < nSize )
			{
				if( szBuffer[nCurPos] != commentEnd[j] )
				{
					j = failRet[j];
					if(j == -1) { j = 0; nCurPos++; }
				}
				else
				{
					j++;
					nCurPos++;
					if(j == 3) break;
				}
				if( nCurPos >= nSize )
					return false;
			}
		}

		return false;
	}

	bool CDomXmlDocument::Parse( CRefStringPtr& DomXmlBuffer, size_t& nCurPos )
	{
		m_ptrBuffer = DomXmlBuffer;
		size_t nSize = m_ptrBuffer->size();
		char* szBuffer = &( (*m_ptrBuffer)[0] );

		// 是否节点开始
		if( nCurPos >= nSize || szBuffer[nCurPos] != '<' )
			return false;

		// 终止m_szContent
		szBuffer[nCurPos++] = 0;

		// 跳过空格
		while( IsBlank( szBuffer[nCurPos] ) )
			if( ++nCurPos >= nSize )
				return false;

		// 名字
		m_szName = szBuffer + nCurPos;

		// 节点名字必须字母开头
		if( !IsXmlIdentity( szBuffer[nCurPos] ) )
			return false;

		while( IsXmlIdentity( szBuffer[nCurPos] ) )
			if( ++nCurPos >= nSize )
				return false;

		while( IsBlank( szBuffer[nCurPos] ) )
		{
			szBuffer[nCurPos] = 0;
			if( ++nCurPos >= nSize )
				return false;
		}

		// 节点属性
		while( FindNextAttribute( DomXmlBuffer, nCurPos ) )
			if( !AddAttribute( DomXmlBuffer, nCurPos ) )
				return false;

		// 空节点
		if( nCurPos < nSize && szBuffer[nCurPos] == '/' )
		{
			szBuffer[nCurPos++] = 0;
			while( szBuffer[nCurPos++] != '>' )
				if( nCurPos >= nSize )
					return false;
			return true;
		}

		if( szBuffer[nCurPos] != '>' )
			return false;

		// 终止m_szName
		szBuffer[nCurPos++] = 0;

		m_szContent = szBuffer + nCurPos;

		while( FindNextNode( DomXmlBuffer, nCurPos ) )
		{
			CDomXmlDocument* pChild = new CDomXmlDocument;
			TGammaList<CDomXmlDocument>::PushBack( *pChild );
			m_nChildCount++;
			pChild->m_pParent = this;
			pChild->m_nLevel = m_nLevel + 1;
			if( !pChild->Parse( DomXmlBuffer, nCurPos ) )
				return false;
		}

		if( nCurPos + 1 >= nSize || szBuffer[nCurPos] != '<' || szBuffer[nCurPos + 1] != '/' )
			return false;		
		while( szBuffer[nCurPos] != '>' )
			szBuffer[nCurPos++] = 0;
		return true;
	}

	bool CDomXmlDocument::FindNextAttribute( CRefStringPtr& DomXmlBuffer, size_t& nCurPos )
	{
		size_t nSize = m_ptrBuffer->size();
		char* szBuffer = &( (*m_ptrBuffer)[0] );
		while( IsBlank( szBuffer[nCurPos] ) )
			if( ++nCurPos >= nSize )
				return false;
		return IsXmlIdentity( szBuffer[nCurPos] );
	}

	bool CDomXmlDocument::AddAttribute( CRefStringPtr& DomXmlBuffer, size_t& nCurPos )
	{
		CDomXmlAttribute* pAttribute = new CDomXmlAttribute;
		TGammaList<CDomXmlAttribute>::PushBack( *pAttribute );
		return pAttribute->Parse( DomXmlBuffer, nCurPos );
	}

	bool CDomXmlDocument::Save( const wchar_t* szFileName ) const
	{
#ifdef _WIN32
		std::ofstream os( szFileName, std::ios::binary );
#else
		std::ofstream os( UcsToUtf8( szFileName ).c_str(), std::ios::binary );
#endif
		return Save( os, 0 );
	}

	bool CDomXmlDocument::Save( std::ostream& os, uint32 nStack ) const
	{
		char szBuff[1024];
		for( uint32 i = 0; i < nStack; ++i )
		{
			szBuff[i] = '\t';
		}
		szBuff[nStack] = 0;

		os << szBuff;

		os << '<';
		os << GetName();

		for( const CDomXmlAttribute* pAttribute = GetFirstAttribute(); 
			pAttribute; pAttribute = pAttribute->GetNextSibling() )
		{
			os << ' ';
			os << pAttribute->GetName();
			os << "=\'";
			os << pAttribute->GetValue();
			os << '\'';
		}
		
		uint32 nCount = 0;
		for( const CDomXmlDocument* pChild = GetFirstChild(); 
			pChild; pChild = pChild->GetNextSibling(), ++nCount )
		{
			if( nCount == 0 )
				os << ">\r\n";

			pChild->Save( os, nStack+1 );
		}
		if( nCount == 0 )
		{
			os << "/>\r\n";
			os.flush();
			return true;
		}

		os << szBuff;
		os << "</";
		os << GetName();
		os << ">\r\n";
		os.flush();

		return true;
	}

	void CDomXmlDocument::clear()
	{
		while( TGammaList<CDomXmlDocument>::GetFirst() )
			delete TGammaList<CDomXmlDocument>::GetFirst();
		while( TGammaList<CDomXmlAttribute>::GetFirst() )
			delete TGammaList<CDomXmlAttribute>::GetFirst();

		if( m_pParent )
		{
			--m_pParent->m_nChildCount;
			m_pParent = NULL;
		}

		m_ptrBuffer = NULL;
	}

	CDomXmlDocument* CDomXmlDocument::InsertNodeFirst( const char* szName )
	{
		CDomXmlDocument* pChild = new CDomXmlDocument;
		TGammaList<CDomXmlDocument>::PushFront( *pChild );
		m_nChildCount++;
		pChild->m_pParent = this;
		pChild->m_nLevel = m_nLevel + 1;
		pChild->m_ptrBuffer = CRefStringPtr( new CRefString );
		pChild->m_ptrBuffer->assign( szName );
		pChild->m_szName = pChild->m_ptrBuffer->c_str();
		return pChild;
	}

	CDomXmlDocument* CDomXmlDocument::InsertNodeLast( const char* szName )
	{
		CDomXmlDocument* pChild = new CDomXmlDocument;
		TGammaList<CDomXmlDocument>::PushBack( *pChild );
		m_nChildCount++;
		pChild->m_pParent = this;
		pChild->m_nLevel = m_nLevel + 1;
		pChild->m_ptrBuffer = CRefStringPtr( new CRefString );
		pChild->m_ptrBuffer->assign( szName );
		pChild->m_szName = pChild->m_ptrBuffer->c_str();
		return pChild;
	}

	void CDomXmlDocument::RemoveNode( const char* szName )
	{
		delete GetChild( szName );
	}

	CDomXmlAttribute* CDomXmlDocument::InsertAttributeFirst( const char* szName, const char* szValue )
	{
		CDomXmlAttribute* pAttribute = new CDomXmlAttribute;
		TGammaList<CDomXmlAttribute>::PushFront( *pAttribute );
		pAttribute->m_ptrBuffer = CRefStringPtr( new CRefString );
		pAttribute->m_ptrBuffer->assign( szName );
		pAttribute->m_ptrBuffer->push_back( 0 );
		size_t nSizeName = pAttribute->m_ptrBuffer->size();
		pAttribute->m_ptrBuffer->append( szValue );
		pAttribute->m_szName = pAttribute->m_ptrBuffer->c_str();
		pAttribute->m_szContent = pAttribute->m_ptrBuffer->c_str() + nSizeName;
		return pAttribute;
	}

	CDomXmlAttribute* CDomXmlDocument::InsertAttributeLast( const char* szName, const char* szValue )
	{
		CDomXmlAttribute* pAttribute = new CDomXmlAttribute;
		TGammaList<CDomXmlAttribute>::PushBack( *pAttribute );
		pAttribute->m_ptrBuffer = CRefStringPtr( new CRefString );
		pAttribute->m_ptrBuffer->assign( szName );
		pAttribute->m_ptrBuffer->push_back( 0 );
		size_t nSizeName = pAttribute->m_ptrBuffer->size();
		pAttribute->m_ptrBuffer->append( szValue );
		pAttribute->m_szName = pAttribute->m_ptrBuffer->c_str();
		pAttribute->m_szContent = pAttribute->m_ptrBuffer->c_str() + nSizeName;
		return pAttribute;
	}

	CDomXmlAttribute* CDomXmlDocument::InsertAttributeFirst( const char* szName, bool bValue )
	{
		return InsertAttributeFirst( szName, bValue ? "true" : "false" );
	}

	CDomXmlAttribute* CDomXmlDocument::InsertAttributeLast( const char* szName, bool bValue )
	{
		return InsertAttributeLast( szName, bValue ? "true" : "false" );
	}

	void CDomXmlDocument::RemoveAttribute( const char* szName )
	{
		delete GetAttribute( szName );
	}

	CDomXmlDocument& CDomXmlDocument::operator()( size_t nChildIndex )
	{
		CDomXmlDocument* pChild = TGammaList<CDomXmlDocument>::GetFirst();
		while( pChild && nChildIndex-- )
			pChild = pChild->GetNext();
		if( !pChild )
			GammaThrow( "not find child!!" );
		return *pChild;
	}

	CDomXmlDocument& CDomXmlDocument::operator()( const char* szChildName )
	{
		CDomXmlDocument* pChild = TGammaList<CDomXmlDocument>::GetFirst();
		while( pChild && strcmp( pChild->m_szName, szChildName ) )
			pChild = pChild->GetNext();
		if( !pChild )
			GammaThrow( "not find child!!" );
		return *pChild;
	}

	const CDomXmlDocument& CDomXmlDocument::operator()( size_t nChildIndex ) const
	{
		CDomXmlDocument* pChild = TGammaList<CDomXmlDocument>::GetFirst();
		while( pChild && nChildIndex-- )
			pChild = pChild->GetNext();
		if( !pChild )
			GammaThrow( "not find child!!" );
		return *pChild;
	}

	const CDomXmlDocument& CDomXmlDocument::operator()( const char* szChildName ) const
	{
		CDomXmlDocument* pChild = TGammaList<CDomXmlDocument>::GetFirst();
		while( pChild && strcmp( pChild->m_szName, szChildName ) )
			pChild = pChild->GetNext();
		if( !pChild )
			GammaThrow( "not find child!!" );
		return *pChild;
	}

	CDomXmlAttribute& CDomXmlDocument::operator[]( size_t nAttributeIndex )
	{
		CDomXmlAttribute* pAttribute = GetAttribute( nAttributeIndex );
		if( !pAttribute )
			GammaThrow( "not find child!!" );
		return *pAttribute;
	}

	CDomXmlAttribute& CDomXmlDocument::operator[]( const char* szName )
	{
		CDomXmlAttribute* pAttribute = GetAttribute( szName );
		if( !pAttribute )
			GammaThrow( "not find child!!" );
		return *pAttribute;
	}

	const CDomXmlAttribute& CDomXmlDocument::operator[]( size_t nAttributeIndex ) const
	{
		const CDomXmlAttribute* pAttribute = GetAttribute( nAttributeIndex );
		if( !pAttribute )
			GammaThrow( "not find child!!" );
		return *pAttribute;
	}

	const CDomXmlAttribute& CDomXmlDocument::operator[]( const char* szAttributeName ) const
	{
		const CDomXmlAttribute* pAttribute = GetAttribute( szAttributeName );
		if( !pAttribute )
			GammaThrow( "not find child!!" );
		return *pAttribute;
	}

	CDomXmlDocument* CDomXmlDocument::GetChild( const char* szName )
	{
		CDomXmlDocument* pChild = TGammaList<CDomXmlDocument>::GetFirst();
		while( pChild && strcmp( pChild->m_szName, szName ) )
			pChild = pChild->GetNext();
		return pChild;
	}

	const CDomXmlDocument* CDomXmlDocument::GetChild( const char* szName ) const
	{
		CDomXmlDocument* pChild = TGammaList<CDomXmlDocument>::GetFirst();
		while( pChild && strcmp( pChild->m_szName, szName ) )
			pChild = pChild->GetNext();
		return pChild;
	}

	CDomXmlDocument* CDomXmlDocument::GetFirstChild()
	{
		return TGammaList<CDomXmlDocument>::GetFirst();
	}

	CDomXmlDocument* CDomXmlDocument::GetNextSibling()
	{
		return TGammaList<CDomXmlDocument>::CGammaListNode::GetNext();
	}

	const CDomXmlDocument* CDomXmlDocument::GetFirstChild() const
	{
		return TGammaList<CDomXmlDocument>::GetFirst();
	}

	const CDomXmlDocument* CDomXmlDocument::GetNextSibling() const
	{
		return TGammaList<CDomXmlDocument>::CGammaListNode::GetNext();
	}

	const char* CDomXmlDocument::GetText() const
	{
		if( !m_szContent )
			return "";
        if( ( (uint8)m_szContent[0] ) != CDictionary::eUtf8HeadTag0 ||
            ( (uint8)m_szContent[1] ) != CDictionary::eUtf8HeadTag1 ||
            ( (uint8)m_szContent[2] ) != CDictionary::eUtf8HeadTag2 )
			return m_szContent;

		const char* szText = CDictionary::Inst().GetValue( CDictionary::StrToKey( m_szContent + 3 ) );
		return szText ? szText : "";
	}

	CDomXmlAttribute* CDomXmlDocument::GetFirstAttribute()
	{
		return TGammaList<CDomXmlAttribute>::GetFirst();
	}

	const CDomXmlAttribute* CDomXmlDocument::GetFirstAttribute() const
	{
		return TGammaList<CDomXmlAttribute>::GetFirst();
	}

	CDomXmlAttribute* CDomXmlDocument::GetAttribute( const char* szName )
	{
		CDomXmlAttribute* pAttribute = TGammaList<CDomXmlAttribute>::GetFirst();
		while( pAttribute && strcmp( pAttribute->m_szName, szName ) )
			pAttribute = pAttribute->GetNext();
		return pAttribute;
	}
	
	CDomXmlAttribute* CDomXmlDocument::GetAttribute( size_t nAttributeIndex )
	{
		CDomXmlAttribute* pAttribute = TGammaList<CDomXmlAttribute>::GetFirst();
		while( pAttribute && nAttributeIndex-- )
			pAttribute = pAttribute->GetNext();
		return pAttribute;
	}

	const CDomXmlAttribute* CDomXmlDocument::GetAttribute( const char* szName ) const
	{
		CDomXmlAttribute* pAttribute = TGammaList<CDomXmlAttribute>::GetFirst();
		while( pAttribute && strcmp( pAttribute->m_szName, szName ) )
			pAttribute = pAttribute->GetNext();
		return pAttribute;
	}

	const CDomXmlAttribute* CDomXmlDocument::GetAttribute( size_t nAttributeIndex ) const
	{
		CDomXmlAttribute* pAttribute = TGammaList<CDomXmlAttribute>::GetFirst();
		while( pAttribute && nAttributeIndex-- )
			pAttribute = pAttribute->GetNext();
		return pAttribute;
	}

	uint32 CDomXmlDocument::Count() const
	{
		return m_nChildCount;
	}

	const char* CDomXmlDocument::GetName() const
	{
		return m_szName;
	}

	const CDomXmlDocument& CDomXmlDocument::operator=( const CDomXmlDocument& rhs )
	{
		std::string strBuffer;
		gammasstream ssOut( strBuffer );
		rhs.Save( ssOut, 0 );
		LoadFromBuffer( strBuffer.c_str(), strBuffer.size() );
		return *this;
	}
}

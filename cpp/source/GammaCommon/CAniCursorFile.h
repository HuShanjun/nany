#pragma once
#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/TGammaRBTree.h"
#include "GammaCommon/TConstString.h"
#include <vector>
#include <map>
#include <iosfwd>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN	
#endif
#include <Windows.h>
#endif

namespace Gamma
{
	class CBufFile;
	class CStringFile;

    /*
    它一般由五部分构成：标志区、文字说明区、信息区、时间控制区和数据区
    ，即RIFF—ACON，LIST—INFO，anih，rate，LIST—fram。
    */
	class CAniCursorFile 
		: public IResListener
		, public TGammaRBTree<CAniCursorFile>::CGammaRBTreeNode
    {
        enum EAniFlag
        {
            eAniFlag_Icon = 1,
            eAniFlag_Sequence = 2,		
        };

        enum Const 
        { 
            MillisecondsPerJIF = 1000/60, /// rate
        };

#pragma pack(push,1)
        /// anih块的固定内容
        struct SAniHeader
        {
            uint32_t m_nHeaderSize;
            uint32_t m_nFrameCount;
            uint32_t m_nStepOfAni;	/// 完成一次动画过程要显示的图象数
            uint32_t m_nImgWidth;
            uint32_t m_nImgHeight;
            uint32_t m_nImgBitCount;  /// 颜色位数
            uint32_t m_nImgPlanes;	/// 颜色位面数
            uint32_t m_nJIFRate;		/// JIF速率（1JIF = 1/60秒）
            uint32_t m_nFlag;			/// AF_ICON/AF_SEQUENCE设置标记
            /*
            如果fl中AF_ICON被设置，
            则cFrame=cSteps,cx,cy以及cBitCount和cPlanes将被忽略,
            每一桢都是一个ICON，它包含了自己的尺寸信息。
            如果fl中AF_SEQUENCE被设置，则cFrame<=cSteps，
            ANI文件中存在seq块
            */

            bool HasSequence() const { return (m_nFlag & eAniFlag_Sequence) != 0; }
            bool IsFrameIconOrCursor() const { return (m_nFlag & eAniFlag_Icon) != 0; }
		};

		union ChunkID
		{
			int m_nID;
			char m_szID[4];
		};

		struct SFileInfo
		{
			ChunkID m_nRiff;
			int  m_nFileLen; /// 排除前8个字节的文件长度
			ChunkID m_nACON;
		};

		struct SChunckHeader
		{
			ChunkID m_nID;
			uint32_t m_nSize;
		};
#pragma pack(pop)

        struct SCursorData
        {
#ifdef _WIN32
			HCURSOR	m_hCursor;
#endif
		};

		enum EChunkIDType
		{
			eHeaderID_LIST = MAKE_DWORD( 'L', 'I', 'S', 'T' ),
			eHeaderID_RIFF = MAKE_DWORD( 'R', 'I', 'F', 'F' ),
			eHeaderID_ACON = MAKE_DWORD( 'A', 'C', 'O', 'N' ),
			eHeaderID_INFO = MAKE_DWORD( 'I', 'N', 'F', 'O' ),
			eHeaderID_FRAM = MAKE_DWORD( 'f', 'r', 'a', 'm' ),
			eHeaderID_ICON = MAKE_DWORD( 'i', 'c', 'o', 'n' ),
			eHeaderID_ANIH = MAKE_DWORD( 'a', 'n', 'i', 'h' ),
			eHeaderID_RATE = MAKE_DWORD( 'r', 'a', 't', 'e' ),
			eHeaderID_SEQ = MAKE_DWORD( 's', 'e', 'q', ' ' ),
			eHeaderID_INAM = MAKE_DWORD( 'I', 'N', 'A', 'M' ),
			eHeaderID_IART = MAKE_DWORD( 'I', 'A', 'R', 'T' ),

			eHeaderID_Unknown = 0,
			eHeaderID_ChuckCount = 11
		};

		typedef void ( CAniCursorFile::*ReadFunc )( CBufFile&, const SChunckHeader& chunkHeader );
		typedef std::map<EChunkIDType, ReadFunc> CChunckReadMap;

		struct CAllCursorMap : public TGammaRBTree<CAniCursorFile>
		{
			CAllCursorMap();
			~CAllCursorMap();
			CChunckReadMap			m_chuckReaders;
		};
		
		static CAllCursorMap		s_mapAllCursor;

		gammacstring				m_strFileName;
		SAniHeader					m_aniHeader;
		std::vector<uint32_t>			m_vecSequence;
		std::vector<uint32_t>			m_vecFrameDurations;
		std::vector<SCursorData>	m_vecFrameDatas;	/// 每帧数据
		mutable uint32_t				m_nTotalTime;

    public:
        CAniCursorFile( const char* szFileName );
		~CAniCursorFile();
		operator const gammacstring&() { return m_strFileName; }
		bool operator < ( const gammacstring& strKey ) { return (const gammacstring&)*this < strKey; }

		static CAniCursorFile*		GetCursor( const char* szCursorName );
		const SAniHeader&			GetAniHeader() const { return m_aniHeader; }
        uint32_t						GetFrameCount() const;
        uint32_t						GetFrameDuration( uint32_t nFrame ) const;
		uint32_t						GetTotalTime() const;
		void						Update();


    private:
		template< EChunkIDType nChunkID >
		void						ReadChunk( CBufFile& BufFile, const SChunckHeader& chunkHeader );
		void						SetCursor( const SCursorData* pCursorData );

        void						TryReadChunk( CBufFile& BufFile );
		void						ReadStaticCursor( CBufFile& BufFile, SCursorData& cursor, size_t nReadStartPos, size_t nDataSize );

		void						ClearData();
		bool						Load( const char* szCursorFile );
		void						OnLoadedEnd( const char* szFileName, const tbyte* pBuffer, uint32_t nSize );
    };
}

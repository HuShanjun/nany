//=====================================================================
// CGamma3DViewOperator.h
// 定义标准的3D视图操作
// 柯达昭
// 2007-09-06
//=======================================================================

#ifndef _GAMMA_VIEW_OPERATOR_H_
#define _GAMMA_VIEW_OPERATOR_H_

#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TVector3.h"
#include "GammaCommon/TVector2.h"
#include "GammaCommon/CMatrix.h"
#include <algorithm>

using namespace std;

namespace Gamma
{
	class CGamma3DViewOperator
	{
	protected:
		CVector3f	m_vCameraUp;
		CVector3f	m_vCameraPos;
		CVector3f	m_vCameraDir;
		int32		m_nNormalViewIndex;

	public:
		CGamma3DViewOperator();

		void		ResetView( const CVector3f* vInitPos = NULL, const CVector3f* vInitViewPos = NULL );
		void		Move( CPos moveDelta ); 
		void		Rotate( CPos vDelta ); 
		void		Zoom( CPos point ); 
		void		Zoom( float fDelta ); 
		void		MoveOnXZPlane( CPos moveDelta );
	};

	inline CGamma3DViewOperator::CGamma3DViewOperator()
		: m_nNormalViewIndex( 0 )
	{
		ResetView();
	}

	inline void CGamma3DViewOperator::ResetView( const CVector3f* vInitPos, const CVector3f* vInitViewPos )
	{    
		if ( vInitViewPos && vInitPos )
		{
			m_vCameraPos    = *vInitPos;
			m_vCameraDir    = *vInitViewPos - *vInitPos;
			m_vCameraUp = CVector3f( 0, 1, 0 );

			CVector3f xAxis = m_vCameraUp.Cross( m_vCameraDir );
			m_vCameraUp = m_vCameraDir.Cross( xAxis );
			m_vCameraUp.Normalize();
		}
		else
		{
			CVector3f CamerPos[3] =
			{
				Gamma::CVector3f( 428.65f, 350, 428.65f ),
				Gamma::CVector3f( 857.3f,  700, 857.3f ),
				Gamma::CVector3f( 0, 100, -500 )  
			};

			m_vCameraUp = Gamma::CVector3f( -0.7071f, 1.732f, -0.7071f );
			m_vCameraPos = CamerPos[m_nNormalViewIndex];
			m_vCameraDir = -CamerPos[m_nNormalViewIndex];
			m_nNormalViewIndex = ( m_nNormalViewIndex + 1 )%ELEM_COUNT( CamerPos );
		}
	}

	inline void CGamma3DViewOperator::Move( CPos moveDelta )
	{
		Gamma::CVector3f X = m_vCameraDir.Cross( m_vCameraUp );
		Gamma::CVector3f Y = m_vCameraUp;
		Y.Normalize();
		X.Normalize();
		m_vCameraPos += Y*((float)moveDelta.y);
		m_vCameraPos += X*((float)moveDelta.x);
	}

	inline void CGamma3DViewOperator::MoveOnXZPlane( CPos moveDelta )
	{
		Gamma::CVector3f X = m_vCameraDir.Cross( m_vCameraUp );
		Gamma::CVector3f Z = m_vCameraDir;
		Z.Normalize();
		X.Normalize();
		X.y = Z.y = 0;
		m_vCameraPos += Z*((float)moveDelta.y);
		m_vCameraPos += X*((float)moveDelta.x);
	}

	inline void CGamma3DViewOperator::Rotate( CPos point )
	{
		Gamma::CMatrix mat;

		Gamma::CVector3f Pos = m_vCameraPos + m_vCameraDir;
		mat.SetRotateY(  point.x/50.0f );
		m_vCameraDir = m_vCameraDir.Rotate( mat );
		m_vCameraUp = m_vCameraUp.Rotate( mat );

		Gamma::CVector3f X = m_vCameraDir.Cross( m_vCameraUp );
		mat.SetRotation( X, -point.y/50.0f );
		m_vCameraDir = m_vCameraDir.Rotate( mat );
		m_vCameraUp = m_vCameraUp.Rotate( mat );
		m_vCameraPos = Pos - m_vCameraDir;
	}

	inline void CGamma3DViewOperator::Zoom( CPos point )
	{
		Gamma::CVector3f Pos = m_vCameraPos + m_vCameraDir;
		float Size = ( -point.y )/500.0f;
		if( Size > 0.5f )
			Size = 0.5f;
		m_vCameraDir *= 1.0f - Size;
		m_vCameraPos = Pos - m_vCameraDir;
	}

	inline void CGamma3DViewOperator::Zoom( float fDelta )
	{		
		float fDirLen = m_vCameraDir.Len();
		if ( fDelta < 0 && fDirLen + fDelta < 5.0 )
			return;

		Gamma::CVector3f Pos = m_vCameraPos + m_vCameraDir;	
		m_vCameraDir /= fDirLen;
		m_vCameraDir *= fDirLen + fDelta;
		m_vCameraPos = Pos - m_vCameraDir;
	}
};

#endif
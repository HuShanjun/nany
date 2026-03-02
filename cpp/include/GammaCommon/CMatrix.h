//=====================================================================
// CMatrix.h
// 矩阵类的定义// 
// 定义各种可能的构造矩阵的方法,和相关的矩阵操作
// 柯达昭
// 2007-08-31
//=====================================================================

#ifndef __GAMMA_MATH_MATRIX_H__
#define __GAMMA_MATH_MATRIX_H__
#include "TVector3.h"
#include "TVector4.h"

namespace Gamma
{
    class CMatrix
    {
    public:

        union 
        {
            struct 
            {
                float    _11, _12, _13, _14;
                float    _21, _22, _23, _24;
                float    _31, _32, _33, _34;
                float    _41, _42, _43, _44;
            };
            float    m[4][4];
        };

        //! 缺省构造函数
        CMatrix();
        //! 拷贝构造函数
        CMatrix(const CMatrix&    mat);
        //! 用Euler角构造矩阵
        CMatrix(float fPitch, float fYaw, float fRoll);
        //! 构造绕某个轴旋转某个角度的变换矩阵
        CMatrix(const CVector3f& vDir, float fRads);
        //! 根据三个矢量和原点(坐标轴)(都是相对于世界坐标系)，构造转换矩阵
        CMatrix(const CVector3f& vX, const CVector3f& vY, 
            const CVector3f& vZ, const CVector3f& vOrg);
        //! 根据起始，终止矢量和向上的矢量，构造视矩阵
        CMatrix(const CVector3f& vFrom, const CVector3f& vAt, 
            const CVector3f& vUp);
        //! 直接构造矩阵
        CMatrix( 
            float _11, float _12, float _13, float _14,
            float _21, float _22, float _23, float _24,
            float _31, float _32, float _33, float _34,
            float _41, float _42, float _43, float _44 );

        ~CMatrix();
		
		//!矩阵赋值
		const CMatrix& Assign(const CMatrix& mat){ return *this = mat; };
        //!按行设置矩阵
        void SetWithRow(const CVector3f& r0,const CVector3f& r1,const CVector3f& r2);
        //!按列设置矩阵
        void SetWithColumn(const CVector3f& c0,const CVector3f& c1,const CVector3f& c2);
        //!  求同一矩阵
		void Identity();
		//!  是否同一矩阵
		bool IsIdentity() const;
        //! 求逆矩阵
        bool Invert();
        //! 求逆矩阵
        void InvertFast();
        //! 求转置矩阵
        void Transpose();    
        //! 矩阵相乘
        CMatrix operator * (const CMatrix& mat) const;
        //! 矩阵自乘
        CMatrix& operator *= (const CMatrix& mat);
        //! 矩阵相等
        bool operator == ( const CMatrix& mat ) const;
        //! 矩阵不等
		bool operator != ( const CMatrix& mat ) const;
		//! 下标访问
		const CVector4f& operator[]( size_t nIndex ) const;
		//! 下标访问
		CVector4f& operator[]( size_t nIndex );
        //! 矩阵和矢量相乘
        const CVector3f operator*( const CVector3f &b );
        //! 设置偏移矩阵
        const CMatrix& SetTranslate( float tx, float ty, float tz );
        //! 设置偏移矩阵
        const CMatrix& SetTranslate( const CVector3f& v );
        //! 设置缩放矩阵
        const CMatrix& SetScale( float sx, float sy, float sz );
        //! 设置缩放矩阵
		const CMatrix& SetScale( const CVector3f& v );
		//! 矩阵乘
		CMatrix Multiply( const CMatrix& Mat ) const;
		//! 矩阵乘，忽略第四列
		CMatrix FastMultiply( const CMatrix& Mat ) const;

        //! 根据一个传入的坐标系(相对于世界坐标系),构造一个转换矩阵
        //void SetTransform(const CCoord& crd, bool IsWorldToObj = true);

        //! 根据起始，终止矢量和向上的矢量(都是相对于世界坐标系)，构造转换矩阵
        // 因该去掉，只使用第一个
        bool SetTransform( const CVector3f& vFrom, 
            const CVector3f& vAt, 
            const CVector3f& vUp );

        //! 根据三个矢量(坐标轴)和原点(都是相对于世界坐标系)，构造转换矩阵
        void SetTransform( const CVector3f& vX, 
            const CVector3f& vY, 
            const CVector3f& vZ, 
            const CVector3f& vOrg);

        //! 构造从一个坐标系到另一个坐标系的转换矩阵
        //void SetTransform(const CCoord& crdSrc, const CCoord& crdDes);

        //! 根据传入的参数构造投影矩阵
        bool SetProjection( float fFOV = 1.570795f, 
            float fAspect = 1.0f,
            float fNearPlane = 1.0f, 
            float fFarPlane = 1000.0f );

        //! 设置绕X轴旋转fRads弧度的变换矩阵
        const CMatrix&	SetRotateX( float fRads );
        //! 设置绕Y轴旋转fRads弧度的变换矩阵
        const CMatrix&	SetRotateY( float fRads );
        //! 设置绕Z轴旋转fRads弧度的变换矩阵
        const CMatrix&	SetRotateZ( float fRads );
        //! 构造Euler旋转矩阵
        const CMatrix& SetRotation(float fPitch, float fYaw, float fRoll);
        //! 设置绕某个轴旋转某个弧度的变换矩阵
        const CMatrix& SetRotation( const CVector3f& vDir, float fRads );
		//! 设置左手坐标系正交投影矩阵
		const CMatrix& OrthoLH( float fWidth, float fHeight, float fNearPlane = 1.0f, float fFarPlane = 1000.0f ); 
        //! 检查矩阵奇偶性，也就是左右手坐标系				 
		bool Parity() const;
		//! 矩阵组合	 
		void Compose( const CVector3f& vScale, const CVector3f& vEulerRotate, const CVector3f& vTranslation );
		//! 获取矩阵位置
		const CVector3f GetLocation() const;
		//! 获取矩阵行
        CVector4f GetAxis( int axis ) const;
		//! 获取矩阵列
		CVector4f GetCol( int nCol ) const;
		//! 获取行放大系数
		CVector3f GetAxisScale() const;
		//! 获取列放大系数
		CVector3f GetColScale() const;

		static const CMatrix IDENTITY;
    };

    //! 缺省构造函数
    inline CMatrix::CMatrix()
        : _11(1.0f), _12(0.0f), _13(0.0f), _14(0.0f)
        , _21(0.0f), _22(1.0f), _23(0.0f), _24(0.0f)
        , _31(0.0f), _32(0.0f), _33(1.0f), _34(0.0f)
        , _41(0.0f), _42(0.0f), _43(0.0f), _44(1.0f)
    {
        //Identity();
    }

    //! 析购函数
    inline CMatrix::~CMatrix()
    {

    }

    //! 拷贝构造函数
    inline CMatrix::CMatrix(const CMatrix&    mat)
    {
        memcpy(&m, &mat, sizeof(CMatrix));
    }

    //=====================================================================
    //   \fn     CMatrix::CMatrix(float fPitch, float fYaw, float fRoll)
    //   \brief  用Euler角构造矩阵，参考SetRotation
    //   \param  float fPitch    相对于x轴旋转的弧度
    //   \param  float fYaw        相对于y轴旋转的弧度
    //   \param  float fRoll        相对于z轴旋转的弧度
    //   \sa     SetRotation(float fPitch, float fYaw, float fRoll)
    //=====================================================================

    inline CMatrix::CMatrix(float fPitch, float fYaw, float fRoll)
    {
        SetRotation(fPitch, fYaw, fRoll);
    }

    //====================================================================
    // \fn     CMatrix::CMatrix(const CVector3f& vDir, float fRads)
    // \brief   构造绕某个轴旋转某个角度的变换矩阵,参考SetRotation
    //
    // \param  const CVector3f&    vDir    表示空间中的一个轴
    // \param  float                fRads    旋转的弧度值
    // \sa     SetRotion(const CVector3f& vDir, float fRads)
    //====================================================================
    inline CMatrix::CMatrix(const CVector3f& vDir, float fRads)
    {
        SetRotation(vDir, fRads);
    }

    //====================================================================
    //  \fn     CMatrix::CMatrix(const CVector3f& vX, const CVector3f& vY, 
    //                const CVector3f& vZ, const CVector3f& vOrg)
    //  \brief  根据三个矢量和原点(坐标轴)构造视矩阵, 参考SetView
    //
    //        根据视空间的三个坐标轴和原点形成世界空间到此视空间的4*4变换矩阵
    //  \param  const CVector3f& vX    视空间的x轴
    //  \param  const CVector3f& vY    视空间的y轴
    //  \param  const CVector3f& vZ    视空间的z轴
    //    \param  const CVector3f& vOrg    视空间的原点
    //    \sa     SetView
    //    ====================================================================
    inline CMatrix::CMatrix(const CVector3f& vX, const CVector3f& vY, 
        const CVector3f& vZ, const CVector3f& vOrg)
    {
        SetTransform(vX, vY, vZ, vOrg);
    }

    //=====================================================================
    //  \fn     CMatrix::CMatrix(const CVector3f& vFrom, const CVector3f& vAt, 
    //                                    const CVector3f& vUp)
    //  \brief  根据起始，终止矢量和向上的矢量，构造视矩阵, 参考SetView
    //
    //  \param  const CVector3f& vFrom        起始矢量
    //  \param  const CVector3f& vAt        终止矢量
    //  \param  const CVector3f& vUp        向上的矢量
    //  \sa     SetView
    //=====================================================================
    inline CMatrix::CMatrix(const CVector3f& vFrom, const CVector3f& vAt, 
        const CVector3f& vUp)
    {
        SetTransform(vFrom, vAt, vUp);
    }

    //=====================================================================
    //  \fn     CMatrix::CMatrix( 
    //                float _11, float _12, float _13, float _14,
    //                float _21, float _22, float _23, float _24,
    //                float _31, float _32, float _33, float _34,
    //                float _41, float _42, float _43, float _44 )
    //  \brief  直接构造矩阵
    //=====================================================================
    inline CMatrix::CMatrix( 
        float _11, float _12, float _13, float _14,
        float _21, float _22, float _23, float _24,
        float _31, float _32, float _33, float _34,
        float _41, float _42, float _43, float _44 )
        : _11(_11), _12(_12), _13(_13), _14(_14)
        , _21(_21), _22(_22), _23(_23), _24(_24)
        , _31(_31), _32(_32), _33(_33), _34(_34)
        , _41(_41), _42(_42), _43(_43), _44(_44)
    {}

    //=====================================================================
    //  \fn     void CMatrix::SetWithRow(const CVector3f& r0,const CVector3f& r1,const CVector3f& r2)
    //  \brief  按行设置矩阵
    //
    //  \param  const CVector3f& r0 第一行
    //  \param  const CVector3f& r1 第二行
    //  \param  const CVector3f& r2 第三行
    //  \return 无
    //=====================================================================
    inline void CMatrix::SetWithRow(const CVector3f& r0,const CVector3f& r1,const CVector3f& r2)
    {
		_11 = r0.x;_12 = r0.y;_13 = r0.z;_14 = 0;
		_21 = r1.x;_22 = r1.y;_23 = r1.z;_24 = 0;
		_31 = r2.x;_32 = r2.y;_33 = r2.z;_34 = 0;
		_41 = 0;   _42 = 0;   _43 = 0;   _44 = 1;
    }

    //=====================================================================
    //  \fn     void CMatrix::SetWithColumn(const CVector3f& c0,const CVector3f& c1,const CVector3f& c2);
    //  \brief  按行设置矩阵
    //
    //  \param  const CVector3f& c0 第一列
    //  \param  const CVector3f& c1 第二列
    //  \param  const CVector3f& c2 第三列
    //  \return 无
    //=====================================================================
    inline void CMatrix::SetWithColumn(const CVector3f& c0,const CVector3f& c1,const CVector3f& c2)
    {
		_11 = c0.x;_12 = c1.x;_13 = c2.x;_14 = 0;
		_21 = c0.y;_22 = c1.y;_23 = c2.y;_24 = 0;
		_31 = c0.z;_32 = c1.z;_33 = c2.z;_34 = 0;
		_41 = 0;   _42 = 0;   _43 = 0;   _44 = 1;
    }

    //=====================================================================
    //  \fn     void CMatrix::Identity()
    //  \brief  设置同一矩阵
    //
    //          同一矩阵对角为一,矩阵与同一矩阵相乘后不会改变
    //  \return void 
    //=====================================================================
    inline void CMatrix::Identity()
    {
        _12 = _13 = _14 = _21 = _23 = _24 = 0.0f;
        _31 = _32 = _34 = _41 = _42 = _43 = 0.0f;
        _11 = _22 = _33 = _44 = 1.0f;
	}

	//=====================================================================
	//  \fn     bool CMatrix::IsIdentity()
	//  \brief  判断同一矩阵
	//
	//          同一矩阵对角为一,矩阵与同一矩阵相乘后不会改变
	//  \return bool 
	//=====================================================================
	inline bool CMatrix::IsIdentity() const
	{
		static CMatrix s_Identity( 
			1, 0, 0, 0, 
			0, 1, 0, 0, 
			0, 0, 1, 0, 
			0, 0, 0, 1 );
		return !memcmp( this, &s_Identity, sizeof(CMatrix) );
	}

    //=====================================================================
    //  \fn     void CMatrix::SetTranslate( float tx, float ty, float tz )
    //  \brief  设置偏移矩阵
    //  \param        float tx x方向上的偏移
    //  \param        float ty y方向上的偏移
    //  \param        float tz z方向上的偏移
    //  \return void 
    //=====================================================================
    inline const CMatrix& CMatrix::SetTranslate( float tx, float ty, float tz )
    { 
        Identity(); 
        _41 = tx; 
        _42 = ty; 
        _43 = tz;
		return *this;
    }

    //=====================================================================
    //  \fn     void CMatrix::SetTranslate( const CVector3f& v )
    //  \brief  设置偏移矩阵
    //  \param  const CVector3f& v 代表3个方向上的偏移
    //  \return void 
    //=====================================================================
    inline const CMatrix& CMatrix::SetTranslate( const CVector3f& v )
    { 
		SetTranslate( v.x, v.y, v.z );
		return *this;
    }

    //=====================================================================
    //  \fn     void CMatrix::SetScale( float sx, float sy, float sz )
    //  \brief  设置缩放矩阵
    //  \param   float sx    x方向上的缩放比例
    //  \param   float sy    y方向上的缩放比例
    //  \param   float sz    z方向上的缩放比例
    //  \return void 
    //=====================================================================
    inline const CMatrix& CMatrix::SetScale( float sx, float sy, float sz )
    { 
        Identity( ); 
        _11 = sx; 
        _22 = sy;
		_33 = sz;
		return *this;
	}

	//=====================================================================
	//  \fn     void CMatrix::SetScale( const CVector3f& v )
	//  \brief  设置缩放矩阵
	//          详细描述
	//  \param  const CVector3f& v 代表3个方向上的缩放比例
	//  \return void 
	//=====================================================================
	inline const CMatrix&  CMatrix::SetScale( const CVector3f& v )
	{ 
		SetScale( v.x, v.y, v.z );
		return *this;
	}

	//=====================================================================
	//  \fn     const CMatrix& CMatrix::Multiply( const CMatrix& Mat )
	//  \brief  矩阵乘
	//          详细描述
	//  \param  const CMatrix& Mat 
	//  \return void 
	//=====================================================================
	inline CMatrix CMatrix::Multiply( const CMatrix& Mat ) const
	{
		return (*this)*Mat;
	}

	//=====================================================================
	//  \fn     const CMatrix& CMatrix::FastMultiply( const CMatrix& Mat )
	//  \brief  快速矩阵乘，忽略第四列
	//          详细描述
	//  \param  const CMatrix& Mat 
	//  \return void 
	//=====================================================================
	inline CMatrix CMatrix::FastMultiply( const CMatrix& Mat ) const
	{
		return CMatrix
			( 
			_11*Mat._11 + _12*Mat._21 + _13*Mat._31,
			_11*Mat._12 + _12*Mat._22 + _13*Mat._32,
			_11*Mat._13 + _12*Mat._23 + _13*Mat._33,
			0,
			_21*Mat._11 + _22*Mat._21 + _23*Mat._31,
			_21*Mat._12 + _22*Mat._22 + _23*Mat._32,
			_21*Mat._13 + _22*Mat._23 + _23*Mat._33,
			0,
			_31*Mat._11 + _32*Mat._21 + _33*Mat._31,
			_31*Mat._12 + _32*Mat._22 + _33*Mat._32,
			_31*Mat._13 + _32*Mat._23 + _33*Mat._33,
			0,
			_41*Mat._11 + _42*Mat._21 + _43*Mat._31 + Mat._41,
			_41*Mat._12 + _42*Mat._22 + _43*Mat._32 + Mat._42,
			_41*Mat._13 + _42*Mat._23 + _43*Mat._33 + Mat._43,
			1
			);
	}

    //=====================================================================
    //  \fn     void CMatrix::SetView( const CVector3f& vX, const CVector3f& vY, 
    //            const CVector3f& vZ, const CVector3f& vOrg)
    //  \brief  根据三个矢量(坐标轴)和原点(都是相对于世界坐标系)，构造转换矩阵
    //
    //            根据三个矢量(坐标轴)和原点(都是相对于世界坐标系)，构造一个由世界坐标
    //            系到此坐标系的转换矩阵.一个常用的应用是根据视空间的三个坐标轴和原点
    //            形成世界空间到此视空间的4*4变换矩阵,            
    //  \param   const CVector3f& vX    坐标轴的x轴
    //  \param   const CVector3f& vY    坐标轴的y轴
    //  \param   const CVector3f& vZ    坐标轴的z轴
    //  \param   const CVector3f& vOrg 坐标轴的原点
    //  \return void 
    //    \todo    没有错误检测，这三个轴矢量是否要单位化
    //=====================================================================
    inline void CMatrix::SetTransform( const CVector3f& vX, const CVector3f& vY, 
        const CVector3f& vZ, const CVector3f& vOrg)
    {
        // Start building the matrix. The first three rows contains the basis
        // vectors used to rotate the view to point at the lookat point
        _11 = vX.x;    _12 = vY.x;    _13 = vZ.x;
        _21 = vX.y;    _22 = vY.y;    _23 = vZ.y;
        _31 = vX.z;    _32 = vY.z;    _33 = vZ.z;

        // Do the translation values (rotations are still about the eyepoint)
        _41 = - vOrg.Dot( vX );
        _42 = - vOrg.Dot( vY );
        _43 = - vOrg.Dot( vZ );

        _14 = 0;
        _24 = 0;
        _34 = 0;
        _44 = 1;
    }

    //=====================================================================
    //  \fn     bool CMatrix::SetView( const CVector3f& vFrom, const CVector3f& vAt, 
    //            const CVector3f& vUpAssumptive )
    //  \brief  根据三个矢量和原点(坐标轴)构造视矩阵
    //
    //          根据给出的Eye point, LooKAt point, 和一个向上的矢量确定一个4*4的视矩阵
    //            一个世界空间的矢量乘以这个矩阵就会变换到这个视空间
    //  \param  const CVector3f& vFrom            Camera 的起始点
    //  \param  const CVector3f& vAt            Camera 的终止点
    //  \param  const CVector3f& vUpAssumptive 向上的坐标
    //  \return bool true 成功， false 失败
    //  \sa     参考内容
    //=====================================================================
    inline bool CMatrix::SetTransform( const CVector3f& vFrom, const CVector3f& vAt, 
        const CVector3f& vUpAssumptive )
    {
        float    fLen;

        // 得到z轴
        CVector3f vView = vAt - vFrom;
        vView.Normalize();

        // 通过点积计算Up(y)矢量到 View(z)矢量的投影,再计算正真与(View)z相垂直
        // 的Up(y)矢量.画一个图想象点积的几何意义
        float fDotProduct = vUpAssumptive.Dot(vView);
        CVector3f vUp = vUpAssumptive - vView * fDotProduct;
        fLen = vUp.Len();

        // 如果这个Up(y)矢量太短,则使用缺省矢量尝试    
        // 因为 vView, (0.0f, 1.0f, 0.0f)都是单位矢量,可以证明(0.0f, 1.0f, 0.0f)
        // 到 vView的投影就等于vView.y(即 vView到(0.0f, 1.0f, 0.0f)的投影),几何意义
        // 与上面的相同.
        if( fabs( fLen ) <= 0.000001f )
        {
            vUp = CVector3f( 0.0f, 1.0f, 0.0f ) - vView * vView.y;
            fLen = vUp.Len();
            if( fabs( fLen ) <= 0.000001f )
            {
                vUp = CVector3f( 0.0f, 0.0f, 1.0f ) - vView * vView.z;
                fLen = vUp.Len();
                if( fabs( fLen ) <= 0.000001f )
                    return false;
            }
        }

        // 将Up(y)矢量单位化
        vUp /= fLen;

        // 通过 vUp(Y)矢量 插积 vView(Z)矢量得到 vRight(X)矢量,(注意是Up在前)
        // 两个单位矢量的插积还是单位矢量
        CVector3f vRight = vUp.Cross( vView );

        SetTransform(vRight, vUp, vView, vFrom);

        return true;
    }

    //=====================================================================
    //  \fn     bool CMatrix::SetProjection(float fFOV,float fAspect, float fNearPlane,float fFarPlane)
    //  \brief  设置投影矩阵 
    //  \param  float fFOV            = 1.570795f        Y轴上的视域 field-of-view,弧度
    //  \param  float fAspect        = 1.3333f        x/y 间的比率
    //  \param  float fNearPlane    = 1.0f            近平面
    //  \param  float fFarPlane        = 1000.0f        远平面
    //  \return bool 
    //=====================================================================
    inline bool CMatrix::SetProjection(float fFOV, float fAspect, float fNearPlane, float fFarPlane)
	{
		GammaAst( fNearPlane != fFarPlane );
		GammaAst( fFOV != 0 );

		float fctg	= tanf( fFOV/2 );
		float h		= 1.0f/fctg;
		float w		= h/fAspect;
		float Q		= fFarPlane / ( fFarPlane - fNearPlane );

		memset( this, 0, sizeof(CMatrix) );
		_11 = w;
		_22 = h;
		_33 = Q;
		_34 = 1.0f;
		_43 = -Q*fNearPlane;

        return true;
    }

    //=====================================================================
    //  \fn     void CMatrix::SetRotateX( float fRads )
    //  \brief  设置绕X轴旋转fRads弧度的变换矩阵
    //  \param  float fRads        旋转的弧度值
    //=====================================================================
    inline const CMatrix& CMatrix::SetRotateX( float fRads )
    {
        Identity( );

        float c = cosf(fRads);
        float s = sinf( fRads );

        _22 =  c;
        _23 =  s;
        _32 = -s;
		_33 =  c;

		return *this;
    }

    //=====================================================================
    //  \fn     void CMatrix::SetRotateY( float fRads )
    //  \brief  设置绕Y轴旋转fRads弧度的变换矩阵
    //  \param  float fRads 旋转的弧度值
    //=====================================================================
    inline const CMatrix& CMatrix::SetRotateY( float fRads )
    {
        Identity( );

        float c = cosf(fRads);
        float s = sinf( fRads );

        _11 =  c;
        _13 = -s;
        _31 =  s;
		_33 =  c;

		return *this;
    }

    //=====================================================================
    //  \fn     void CMatrix::SetRotateZ( float fRads )
    //  \brief  设置绕Z轴旋转fRads弧度的变换矩阵
    //  \param  float fRads 旋转的弧度值
    //=====================================================================
    inline const CMatrix& CMatrix::SetRotateZ( float fRads )
    {
        Identity( );

        float c = cosf(fRads);
        float s = sinf( fRads );

        _11  =  c;
        _12  =  s;
        _21  = -s;
        _22  =  c;

		return *this;
    }

    //=====================================================================
    //  \fn     void CMatrix::SetRotation(float fPitch, float fYaw, float fRoll)
    //  \brief  构造Euler旋转矩阵
    //          旋转的顺序是 x, y z,应该可以直接构造
    //  \param  float fPitch    相对于x轴旋转的弧度
    //  \param  float fYaw        相对于y轴旋转的弧度
    //  \param  float fRoll        相对于z轴旋转的弧度
    //=====================================================================
    inline const CMatrix& CMatrix::SetRotation(float fYaw, float fPitch, float fRoll)
    {
        CMatrix    matTemp;

        Identity();

        matTemp.SetRotateZ(fRoll);
        *this *= matTemp;
        matTemp.SetRotateX(fPitch);
        *this *= matTemp;
        matTemp.SetRotateY(fYaw);
        *this *= matTemp;

		return *this;
        /*
        int i;
        float sinangx,cosangx,sinangy,cosangy,sinangz,cosangz;
        float xold,yold,zold,xnew,ynew,znew;//temporary rotation results to prevent round off error
        sinangx=(float)sin(yaw);
        cosangx=(float)cos(yaw);
        sinangy=(float)sin(pitch);
        cosangy=(float)cos(pitch);
        sinangz=(float)sin(roll);
        cosangz=(float)cos(roll);
        for(i=0; i<3; i++)
        {
        //init points
        xold=m[i][0];
        yold=m[i][1];
        zold=m[i][2];
        //rotate point locally  
        //xrot
        xnew=xold;
        ynew=(yold*cosangx)-(zold*sinangx);
        znew=(yold*sinangx)+(zold*cosangx);
        xold=xnew;
        yold=ynew;
        zold=znew;
        //yrot            
        xnew=((xold*cosangy)+(zold*sinangy));
        ynew=yold;
        znew=((-1*(xold*sinangy))+(zold*cosangy));
        xold=xnew;
        yold=ynew;
        zold=znew;
        //zrot
        xnew=((xold*cosangz)-(yold*sinangz));
        ynew=((yold*cosangz)+(xold*sinangz));
        znew=zold;
        //change object location
        m[i][0]=xnew;
        m[i][1]=ynew;
        m[i][2]=znew;
        }
        */
    }

    //=====================================================================
    //  \fn     void CMatrix::SetRotation( const CVector3f& vDir, float fRads )
    //  \brief  设置绕某个轴旋转某个弧度的变换矩阵
    //  \param   const CVector3f& vDir 表示空间中的一个轴
    //  \param  float fRads                旋转的弧度值(由轴的正向向原点看去，逆时针为正)
    //  \return void 
    //=====================================================================
    inline const CMatrix&  CMatrix::SetRotation( const CVector3f& vDir, float fRads )
    {
        float        fCos    = cosf( fRads );
        float        fSin    = sinf( fRads );
        CVector3f    v        = vDir;
        v.Normalize();

        _11 = ( v.x * v.x ) * ( 1.0f - fCos ) + fCos;
        _12 = ( v.x * v.y ) * ( 1.0f - fCos ) + (v.z * fSin);
        _13 = ( v.x * v.z ) * ( 1.0f - fCos ) - (v.y * fSin);

        _21 = ( v.y * v.x ) * ( 1.0f - fCos ) - (v.z * fSin);
        _22 = ( v.y * v.y ) * ( 1.0f - fCos ) + fCos ;
        _23 = ( v.y * v.z ) * ( 1.0f - fCos ) + (v.x * fSin);

        _31 = ( v.z * v.x ) * ( 1.0f - fCos ) + (v.y * fSin);
        _32 = ( v.z * v.y ) * ( 1.0f - fCos ) - (v.x * fSin);
        _33 = ( v.z * v.z ) * ( 1.0f - fCos ) + fCos;

        _14 = _24 = _34 = 0.0f;
        _41 = _42 = _43 = 0.0f;
        _44 = 1.0f;

		return *this;
    }

	//=====================================================================
	//  \fn     const CMatrix& CMatrix::OrthoLH( float fWidth, float fHeight, float fNearPlane, float fFarPlane )
	//  \brief  设置左手坐标系正交投影矩阵
	//  \sa     参考内容
	//=====================================================================
	inline const CMatrix& CMatrix::OrthoLH( float fWidth, float fHeight, float fNearPlane, float fFarPlane )
	{
		Identity();
		_11 = 2.0f/fWidth;
		_22 = 2.0f/fHeight;
		_33 = 1.0f/( fFarPlane - fNearPlane );
		_43 = -fNearPlane/( fFarPlane - fNearPlane );

		return *this;
	}

	inline void CMatrix::Compose( const CVector3f& vScale, const CVector3f& vEulerRotate, const CVector3f& vTranslation )
	{
		CVector3f vHalf = vEulerRotate*0.5f;
		float cosX = cosf( vHalf.x ), sinX = sinf( vHalf.x );
		float cosY = cosf( vHalf.y ), sinY = sinf( vHalf.y );
		float cosZ = cosf( vHalf.z ), sinZ = sinf( vHalf.z );

		float w = cosX*cosY*cosZ + sinX*sinY*sinZ;
		float x = sinX*cosY*cosZ - cosX*sinY*sinZ;
		float y = cosX*sinY*cosZ + sinX*cosY*sinZ;
		float z = cosX*cosY*sinZ - sinX*sinY*cosZ;

		float x2 = x + x;
		float y2 = y + y;
		float z2 = z + z;
		float xx = x * x2;
		float xy = x * y2;
		float xz = x * z2;
		float yy = y * y2;
		float yz = y * z2;
		float zz = z * z2;
		float wx = w * x2;
		float wy = w * y2;
		float wz = w * z2;

		*this = CMatrix(
			1.0f - ( yy + zz ), xy + wz,            xz - wy,            0,
			xy - wz,            1.0f - ( xx + zz ), yz + wx,            0,
			xz + wy,            yz - wx,            1.0f - ( xx + yy ), 0,
			vTranslation.x,     vTranslation.y,     vTranslation.z,     1 );

		( *(CVector3f*)m[0] ) *= vScale.x;
		( *(CVector3f*)m[1] ) *= vScale.y;
		( *(CVector3f*)m[2] ) *= vScale.z;
	}
	
    //=====================================================================
    //  \fn     bool CMatrix::Invert()
    //  \brief  求逆矩阵
    //          只有在矩阵的第四列为[0 0 0 1] 这个函数才能正常工作，自身改变
    //  \return bool true 求得逆矩阵， false 求不出逆矩阵
    //  \sa     参考内容
    //=====================================================================
    inline bool CMatrix::Invert()
	{
		#define GAMMA_MATRIX_SWAP( a, b ) { float t = a; a = b; b = t; }

		CMatrix InvertMat( *this );
		int32 is[4];
		int32 js[4];
		float fDet = 1.0f;
		int f = 1;

		for( int32 k = 0; k < 4; k++ )
		{
			// 第一步，全选主元
			float fMax = 0.0f;
			for( int32 i = k; i < 4; i++ )
			{
				for( int32 j = k; j < 4; j++ )
				{
					//const float f = abs( InvertMat.m[i][j] );
					const float f = InvertMat.m[i][j] < 0 ?  -InvertMat.m[i][j] : InvertMat.m[i][j];
					if( f > fMax )
					{
						fMax	= f;
						is[k]	= i;
						js[k]	= j;
					}
				}
			}

			if( fMax < 1.0e-19F )
				return false;

			if( is[k] != k )
			{
				f = -f;
				GAMMA_MATRIX_SWAP( InvertMat.m[k][0], InvertMat.m[is[k]][0] );
				GAMMA_MATRIX_SWAP( InvertMat.m[k][1], InvertMat.m[is[k]][1] );
				GAMMA_MATRIX_SWAP( InvertMat.m[k][2], InvertMat.m[is[k]][2] );
				GAMMA_MATRIX_SWAP( InvertMat.m[k][3], InvertMat.m[is[k]][3] );
			}

			if( js[k] != k )
			{
				f = -f;
				GAMMA_MATRIX_SWAP( InvertMat.m[0][k], InvertMat.m[0][js[k]] );
				GAMMA_MATRIX_SWAP( InvertMat.m[1][k], InvertMat.m[1][js[k]] );
				GAMMA_MATRIX_SWAP( InvertMat.m[2][k], InvertMat.m[2][js[k]] );
				GAMMA_MATRIX_SWAP( InvertMat.m[3][k], InvertMat.m[3][js[k]] );
			}

			// 计算行列值
			fDet *= InvertMat.m[k][k];

			// 计算逆矩阵

			// 第二步
			InvertMat.m[k][k] = 1.0f / InvertMat.m[k][k];	
			// 第三步
			for( int32 j = 0; j < 4; j++ )
			{
				if( j != k )
					InvertMat.m[k][j] *= InvertMat.m[k][k];
			}

			// 第四步
			for( int32 i = 0; i < 4; i++ )
			{
				if( i != k )
				{
					for( int32 j = 0; j < 4; j++ )
					{
						if( j != k )
							InvertMat.m[i][j] = InvertMat.m[i][j] - InvertMat.m[i][k] * InvertMat.m[k][j];
					}
				}
			}

			// 第五步
			for( int32 i = 0; i < 4; i++ )
			{
				if( i != k )
					InvertMat.m[i][k] *= -InvertMat.m[k][k];
			}
		}

		for( int32 k = 3; k >= 0; k-- )
		{
			if( js[k] != k )
			{
				GAMMA_MATRIX_SWAP( InvertMat.m[k][0], InvertMat.m[js[k]][0] );
				GAMMA_MATRIX_SWAP( InvertMat.m[k][1], InvertMat.m[js[k]][1] );
				GAMMA_MATRIX_SWAP( InvertMat.m[k][2], InvertMat.m[js[k]][2] );
				GAMMA_MATRIX_SWAP( InvertMat.m[k][3], InvertMat.m[js[k]][3] );
			}

			if( is[k] != k )
			{
				GAMMA_MATRIX_SWAP( InvertMat.m[0][k], InvertMat.m[0][is[k]] );
				GAMMA_MATRIX_SWAP( InvertMat.m[1][k], InvertMat.m[1][is[k]] );
				GAMMA_MATRIX_SWAP( InvertMat.m[2][k], InvertMat.m[2][is[k]] );
				GAMMA_MATRIX_SWAP( InvertMat.m[3][k], InvertMat.m[3][is[k]] );
			}
		}

		*this = InvertMat;
        return true;
    }

    //=====================================================================
    //  \fn     void CMatrix::InvertFast()
    //  \brief  求逆矩阵
    //          只有在矩阵的第四列为[0 0 0 1]并且三根轴缩放大小一致这个函数才能正常工作，自身改变
    //  \return 
    //  \sa     参考内容
    //=====================================================================
    inline void CMatrix::InvertFast()
    {
        /*
        // 摘自Advanced 3D Game Programming with DirectX7
        // first transpose the rotation matrix
        _11 = in._11;
        _12 = in._21;
        _13 = in._31;
        _21 = in._12;
        _22 = in._22;
        _23 = in._32;
        _31 = in._13;
        _32 = in._23;
        _33 = in._33;

        // fix right column
        _14 = 0;
        _24 = 0;
        _34 = 0;
        _44 = 1;

        // now get the new translation vector
        point3 temp = in.GetLoc();

        _41 = -(temp.x * in._11 + temp.y * in._12 + temp.z * in._13);
        _42 = -(temp.x * in._21 + temp.y * in._22 + temp.z * in._23);
        _43 = -(temp.x * in._31 + temp.y * in._32 + temp.z * in._33);
        */

        // first transpose the rotation matrix
        CMatrix    InMat( *this );
        CVector3f v( _11, _12, _13 );
        float fScale = _11*_11 + _12*_12 + _13*_13;

        if( fabs( fScale - 1.0f ) <= 0.000001f )
        {
            _11 = InMat._11;
            _12 = InMat._21;
            _13 = InMat._31;
            _21 = InMat._12;
            _22 = InMat._22;
            _23 = InMat._32;
            _31 = InMat._13;
            _32 = InMat._23;
            _33 = InMat._33;
        }
        else
        {
            fScale = 1.0f/fScale;
            _11 = InMat._11*fScale;
            _12 = InMat._21*fScale;
            _13 = InMat._31*fScale;
            _21 = InMat._12*fScale;
            _22 = InMat._22*fScale;
            _23 = InMat._32*fScale;
            _31 = InMat._13*fScale;
            _32 = InMat._23*fScale;
            _33 = InMat._33*fScale;
        }

        // fix right column
        _14 = 0;
        _24 = 0;
        _34 = 0;
        _44 = 1;

        // now get the new translation vector
        _41 = -( InMat._41 * _11 + InMat._42 * _21 + InMat._43 * _31 );
        _42 = -( InMat._41 * _12 + InMat._42 * _22 + InMat._43 * _32 );
        _43 = -( InMat._41 * _13 + InMat._42 * _23 + InMat._43 * _33 );
    }

    //=====================================================================
    //  \fn     void CMatrix::Transpose()
    //  \brief  求转置矩阵
    //          转置矩阵既是矩阵的行列元素互换，自身改变
    //  \return void 
    //=====================================================================
    inline void CMatrix::Transpose()
    {
        for(int i = 0; i < 3; i++)
        {
            for(int j = i + 1; j < 4; j++)
            {
                float fTemp = m[i][j];
                m[i][j] = m[j][i];
                m[j][i] = fTemp;
            }
        }
    }

    inline const CVector3f CMatrix::GetLocation() const
    {
        return CVector3f( _41, _42, _43 );
    }

    inline CVector4f CMatrix::GetAxis( int axis ) const
    {
        return *(const CVector4f*)m[axis];
    }
    
    inline CVector4f CMatrix::GetCol( int nCol ) const
    {
        return
            CVector4f
            (
                m[0][nCol],
				m[1][nCol],
				m[2][nCol],
				m[3][nCol]
            );
	}

	inline CVector3f CMatrix::GetAxisScale() const
	{
		return CVector3f( 
			( (const CVector3f*)m[0] )->Len(), 
			( (const CVector3f*)m[1] )->Len(), 
			( (const CVector3f*)m[2] )->Len() ); 
	}

	inline CVector3f CMatrix::GetColScale() const
	{
		return CVector3f( 
			CVector3f( m[0][0], m[1][0], m[2][0] ).Len(), 
			CVector3f( m[0][1], m[1][1], m[2][1] ).Len(), 
			CVector3f( m[0][2], m[1][2], m[2][2] ).Len() ); 
	}

    //===========================================================================
    // \fn     bool CMatrix::Parity()
    // \brief  检查矩阵奇偶性
    //
    // \return CMatrix 返回矩阵奇偶性，返回false为左手坐标系（通常），true为右手
    //===========================================================================
    inline bool CMatrix::Parity() const
    {
        if ( *this == CMatrix() ) 
            return false;

        return ( *(const CVector3f*)m[0] ).
			Cross( *(const CVector3f*)m[1] ).
			Dot( *(const CVector3f*)m[2]  ) < 0;
    }

    //=====================================================================
    //  \fn     CMatrix CMatrix::operator * (const CMatrix& mat) const
    //  \brief  矩阵相乘
    //          重载了乘号,完成与传入的矩阵相乘的操作，自身不改变
    //  \param  const CMatrix& mat 为第二(右)操作数
    //  \return CMatrix CMatrix::operator 返回相乘后的结果矩阵
    //=====================================================================
    inline CMatrix CMatrix::operator * (const CMatrix& mat) const
    {
        CMatrix    MatRet;

        float* pA = (float*)this;
        float* pB = (float*)&mat;
        float* pM = (float*)&MatRet;

        memset( pM, 0, sizeof(CMatrix) );

        for( unsigned int i = 0; i < 4; i++ ) 
            for( unsigned int j = 0; j < 4; j++ ) 
                for( unsigned int k = 0; k < 4; k++ ) 
                    pM[4*i+j] +=  pA[4*i+k] * pB[4*k+j];

        return MatRet;
    }

    //=====================================================================
    //  \fn     CMatrix& CMatrix::operator *= (const CMatrix& mat)
    //    \brief  矩阵自乘
    //
    //          重载了*=号,完成与传入的矩阵相乘的操作，自身改变
    //  \param  const CMatrix& mat 为第二(右)操作数
    //  \return CMatrix& CMatrix::operator 返回的是相乘后的结果也是自身的引用
    //=====================================================================
    inline CMatrix& CMatrix::operator *= (const CMatrix& mat)
    {
        CMatrix    MatRet;

        float* pA = (float*)this;
        float* pB = (float*)&mat;
        float* pM = (float*)&MatRet;

        memset( pM, 0, sizeof(CMatrix) );

        for( unsigned int i = 0; i < 4; i++ ) 
            for( unsigned int j = 0; j < 4; j++ ) 
                for( unsigned int k = 0; k < 4; k++ ) 
                    pM[4*i+j] +=  pA[4*i+k] * pB[4*k+j];

        memcpy( this, pM, sizeof(CMatrix) );

        return *this;
    }

    inline const CVector3f CMatrix::operator*( const CVector3f& b )
    {
        return CVector3f(
            b.x*_11 + b.y*_21 + b.z*_31 + _41,
            b.x*_12 + b.y*_22 + b.z*_32 + _42,
            b.x*_13 + b.y*_23 + b.z*_33 + _43
            );
    };

    inline bool CMatrix::operator == ( const CMatrix& mat ) const
    {

        return 
            _11 == mat._11 && _12 == mat._12 && _13 == mat._13 && _14 == mat._14 && 
            _21 == mat._21 && _22 == mat._22 && _23 == mat._23 && _24 == mat._24 && 
            _31 == mat._31 && _32 == mat._32 && _33 == mat._33 && _34 == mat._34 && 
            _41 == mat._41 && _42 == mat._42 && _43 == mat._43 && _44 == mat._44 ;
    }

    inline bool CMatrix::operator != ( const CMatrix& mat ) const
    {
        return !( *this == mat );
    }

	//! 下标访问
	inline const CVector4f& CMatrix::operator[]( size_t nIndex ) const
	{
		return ( (const CVector4f*)m )[nIndex];
	}

	//! 下标访问
	inline CVector4f& CMatrix::operator[]( size_t nIndex )
	{
		return ( (CVector4f*)m )[nIndex];
	}

    //=====================================================================
    //! \fn     TVector3<T>::Rotate(const TMATRIX& Mat)
    //  \brief  仅仅旋转矢量
    //          不考虑矩阵中的偏移参数，仅仅利用矩阵中的旋转信息完成矢量的旋转，
    //        内部去掉了矩阵的第四行，第四列。矢量自身改变
    //  \sa     需要做传入的矩阵的校验
    //=====================================================================
    template<class T> 
    inline const TVector3<T>& TVector3<T>::Rotate( const CMatrix& Mat )
	{
		T fX = (T)( x*Mat._11 + y*Mat._21 + z*Mat._31 );
		T fY = (T)( x*Mat._12 + y*Mat._22 + z*Mat._32 );
		T fZ = (T)( x*Mat._13 + y*Mat._23 + z*Mat._33 );

        x = fX;
        y = fY;
        z = fZ;
        return *this;    
    }

    //=====================================================================
    //! \fn     TVector3<T>::FastMultiply(const TMATRIX& Mat)
    //  \brief  矢量与快速矩阵相乘, 返回值为结果矢量,不同于重载的乘操作，最后一列不乘, 速度由所提高
    //  \sa     需要做传入的矩阵的校验
    //=====================================================================    
	template<class T> 
	inline const TVector3<T>& TVector3<T>::FastTransform( const CMatrix& Mat )
	{
		T fX = (T)( x*Mat._11 + y*Mat._21 + z*Mat._31 + Mat._41 );
		T fY = (T)( x*Mat._12 + y*Mat._22 + z*Mat._32 + Mat._42 );
		T fZ = (T)( x*Mat._13 + y*Mat._23 + z*Mat._33 + Mat._43 );

		x = fX;
		y = fY;
		z = fZ;
		return *this;    
	}

	//=====================================================================
	//! \fn     TVector3<T>::FastMultiply(const TMATRIX& Mat)
	//  \brief  矢量与快速矩阵相乘, 返回值为结果矢量,不同于重载的乘操作，最后一列不乘, 速度由所提高
	//  \sa     需要做传入的矩阵的校验
	//=====================================================================    
	template<class T> 
	inline const TVector3<T>& TVector3<T>::Transform( const CMatrix& Mat )
	{
		T fX = (T)( x*Mat._11 + y*Mat._21 + z*Mat._31 + Mat._41 );
		T fY = (T)( x*Mat._12 + y*Mat._22 + z*Mat._32 + Mat._42 );
		T fZ = (T)( x*Mat._13 + y*Mat._23 + z*Mat._33 + Mat._43 );
		T fW = (T)( x*Mat._14 + y*Mat._24 + z*Mat._34 + Mat._44 );

		if( fW == 0.0f )
		{
			x = 0;
			y = 0;
			z = 0;
		}
		else
		{
			x = fX/fW;
			y = fY/fW;
			z = fZ/fW;
		}

		return *this;    
	}

    //=====================================================================
    //! \fn     CVector3f operator* ( const CVector3f& vCoord, const CMatrix& Mat )
    //  \brief    矢量与矩阵相乘, (矢量在左,矩阵在右,行矢量)
    //=====================================================================
    inline CVector3f operator* ( const CVector3f& vCoord, const CMatrix& Mat )
    {
        float fX = vCoord.x*Mat._11 + vCoord.y*Mat._21 + vCoord.z*Mat._31 + Mat._41;
        float fY = vCoord.x*Mat._12 + vCoord.y*Mat._22 + vCoord.z*Mat._32 + Mat._42;
        float fZ = vCoord.x*Mat._13 + vCoord.y*Mat._23 + vCoord.z*Mat._33 + Mat._43;
        float fW = vCoord.x*Mat._14 + vCoord.y*Mat._24 + vCoord.z*Mat._34 + Mat._44;

        if( fW == 0.0f )
            return CVector3f(0.0f, 0.0f, 0.0f);

        return CVector3f( fX/fW,  fY/fW,  fZ/fW );
    }

    //=====================================================================
    //! \fn     CVector3f operator*= ( CVector3f& vCoord, const CMatrix& Mat )
    //  \brief    矢量与矩阵相乘, (矢量在左,矩阵在右,行矢量)
    //=====================================================================
    inline CVector3f operator*= ( CVector3f& vCoord, const CMatrix& Mat )
    {        
        return vCoord = vCoord*Mat;
	}


	//=====================================================================
	//! \fn     CVector3f operator* ( const CVector3f& vCoord, const CMatrix& Mat )
	//  \brief    矢量与矩阵相乘, (矢量在左,矩阵在右,行矢量)
	//=====================================================================
	inline CVector4f operator* ( const CVector4f& vCoord, const CMatrix& Mat )
	{
		float fX = vCoord.x*Mat._11 + vCoord.y*Mat._21 + vCoord.z*Mat._31 + Mat._41;
		float fY = vCoord.x*Mat._12 + vCoord.y*Mat._22 + vCoord.z*Mat._32 + Mat._42;
		float fZ = vCoord.x*Mat._13 + vCoord.y*Mat._23 + vCoord.z*Mat._33 + Mat._43;
		float fW = vCoord.x*Mat._14 + vCoord.y*Mat._24 + vCoord.z*Mat._34 + Mat._44;

		return CVector4f( fX, fY, fZ, fW );
	}

	//=====================================================================
	//! \fn     CVector3f operator*= ( CVector3f& vCoord, const CMatrix& Mat )
	//  \brief    矢量与矩阵相乘, (矢量在左,矩阵在右,行矢量)
	//=====================================================================
	inline CVector4f operator*= ( CVector4f& vCoord, const CMatrix& Mat )
	{        
		return vCoord = vCoord*Mat;
	}
}

#endif 

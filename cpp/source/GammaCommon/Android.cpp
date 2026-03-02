

#ifdef _ANDROID

#include "Android.h"
#include "GammaCommon/GammaMd5.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/CGammaWindow.h"
#include "GammaCommon/CPathMgr.h"
#include "GammaCommon/CVersion.h"
#include "GammaCommon/GammaCodeCvs.h"
#include <android/log.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <stdlib.h>
#include <poll.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <alloca.h>

#undef main
extern int32_t main( int nArg, const char* szArg[] );
#define SYSTEM_FILE_ID	0x7b56b800

namespace Gamma
{
	CAndroidApp::CAndroidApp()
		: m_pApplication( NULL )
		, m_pActivity( NULL )
		, m_pNativeWindow( NULL )
		, m_pConfig( NULL )
		, m_pSensorMgr( NULL )
		, m_pSensorEventQueue( NULL )
		, m_bMainThreadRunning( false )
		, m_bActivityRunning( false )
		, m_hInputLock( GammaCreateLock() )
		, m_pSavedState( NULL )
		, m_nSavedStateSize( 0 )
		, m_pInputQueue( NULL )
		, m_bDestroyed( false )
		, m_bDestroyRequested( false )
		, m_bInputVisible( false )
		, m_pPendingInputQueue( NULL )
		, m_pPendingWindow( NULL )
		, m_nMainStackSize( 0 )
		, m_bFullScreenInput( false )
		, m_nSysFileID( 0 )
	{
		memset( &m_HardwareDesc, 0, sizeof(m_HardwareDesc) );
		memset( m_szExternalPath, 0, sizeof( m_szExternalPath ) );
		memset( m_szPackagePath, 0, sizeof( m_szPackagePath ) );
		pthread_mutex_init( &m_Mutex, NULL );
		pthread_cond_init( &m_Condiction, NULL );
	}

	CAndroidApp::~CAndroidApp()
	{
		pthread_cond_destroy( &m_Condiction );
		pthread_mutex_destroy( &m_Mutex );
		GammaDestroyLock( m_hInputLock );
	}

	//===================================================================
	// Interface
	//===================================================================
	CAndroidApp& CAndroidApp::GetInstance()
	{
		static CAndroidApp s_Instance;
		return s_Instance;
	}

	const char* CAndroidApp::GetExternalPath()
	{
		return m_szExternalPath;
	}

	const char* CAndroidApp::GetPackagePath()
	{
		return m_szPackagePath;
	}

	uint64_t CAndroidApp::GetVersion() const
	{
		return m_nVersion;
	}

	void CAndroidApp::GetApplicationSignature( char szMd5[33] )
	{
		memcpy( szMd5, m_arySignatureMd5, sizeof(m_arySignatureMd5) );
	}

	//===================================================================
	// jni adapter
	//===================================================================
	void CAndroidApp::SetClipboardContent( int32_t nType, const void* pContent, uint32_t nSize )
	{
		if( nType != CONTENT_TYPE_TEXT )
			return;
		if( !m_pActivity || !m_pActivity->clazz )
			return;
		JNIEnv* pJniEnv = NULL;
		JavaVMAttachArgs Args = { JNI_VERSION_1_6, "NativeThread", NULL };
		jint lResult = m_pJavaVM->AttachCurrentThread( &pJniEnv, &Args );
		GammaAst( lResult != JNI_ERR );

		jobject lNativeActivity = m_pActivity->clazz;
		jclass ClassNativeActivity = pJniEnv->GetObjectClass(lNativeActivity);

		jmethodID nMethodID = pJniEnv->GetMethodID
			(ClassNativeActivity,  "SetClipboard", "(Ljava/lang/String;)V");
		if( nMethodID )
		{
			string strText( (const char*)pContent, nSize );
			jstring pStringText = pJniEnv->NewStringUTF( strText.c_str() );
			pJniEnv->CallVoidMethod( lNativeActivity, nMethodID, pStringText );
			pJniEnv->DeleteLocalRef( pStringText );
		}

		pJniEnv->DeleteLocalRef( ClassNativeActivity );
	}

	void CAndroidApp::GetClipboardContent( int32_t nType, const void*& pContent, uint32_t& nSize )
	{
		if( nType != CONTENT_TYPE_TEXT )
			return;
		if( !m_pActivity || !m_pActivity->clazz )
			return;

		JNIEnv* pJniEnv = NULL;
		JavaVMAttachArgs Args = { JNI_VERSION_1_6, "NativeThread", NULL };
		jint lResult = m_pJavaVM->AttachCurrentThread( &pJniEnv, &Args );
		GammaAst( lResult != JNI_ERR );

		jobject lNativeActivity = m_pActivity->clazz;
		jclass ClassNativeActivity = pJniEnv->GetObjectClass(lNativeActivity);

		jmethodID nMethodID = pJniEnv->GetMethodID
			(ClassNativeActivity,  "GetClipboard", "()Ljava/lang/String;");

		pContent = "";
		nSize = 0;
		if( nMethodID )
		{
			jstring jstrText = (jstring)pJniEnv->CallObjectMethod( lNativeActivity, nMethodID );
			static string s_ClipString;
			s_ClipString = pJniEnv->GetStringUTFChars( jstrText, NULL );
			pContent = s_ClipString.c_str();
			nSize = s_ClipString.size();
			pJniEnv->DeleteLocalRef( jstrText );
		}

		pJniEnv->DeleteLocalRef( ClassNativeActivity );
	}

	void CAndroidApp::OpenURL( const char* szUrl )
	{
		if( !szUrl || !szUrl[0] )
			return;
		if( !m_pActivity || !m_pActivity->clazz )
			return;
		JNIEnv* pJniEnv = NULL;
		JavaVMAttachArgs Args = { JNI_VERSION_1_6, "NativeThread", NULL };
		jint lResult = m_pJavaVM->AttachCurrentThread( &pJniEnv, &Args );
		GammaAst( lResult != JNI_ERR );

		jclass ClassNativeApplication = pJniEnv->GetObjectClass( m_pApplication );
		jclass IntentClass = pJniEnv->FindClass( "android/content/Intent" );
		jclass UriClass = pJniEnv->FindClass( "android/net/Uri" );
		jmethodID MethodStartActive = pJniEnv->GetMethodID
			( ClassNativeApplication,  "startActivity", "(Landroid/content/Intent;)V");
		jmethodID MethIntentInit = pJniEnv->GetMethodID
			( IntentClass, "<init>", "(Ljava/lang/String;)V");
		jmethodID MethIntentSetData = pJniEnv->GetMethodID
			( IntentClass, "setData", "(Landroid/net/Uri;)Landroid/content/Intent;");
		jmethodID MethodParse = pJniEnv->GetStaticMethodID
			( UriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");

		jstring pAction = pJniEnv->NewStringUTF( "android.intent.action.VIEW" );
		jstring pUrl = pJniEnv->NewStringUTF( szUrl );

		jobject jIntent = pJniEnv->NewObject( IntentClass, MethIntentInit, pAction );
		jobject contentURL = pJniEnv->CallStaticObjectMethod( UriClass, MethodParse, pUrl );
		pJniEnv->CallObjectMethod( jIntent, MethIntentSetData, contentURL );
		pJniEnv->CallVoidMethod( m_pApplication, MethodStartActive, jIntent );

		pJniEnv->DeleteLocalRef( pUrl );
		pJniEnv->DeleteLocalRef( pAction );
		pJniEnv->DeleteLocalRef( ClassNativeApplication );
		pJniEnv->DeleteLocalRef( jIntent );
		pJniEnv->DeleteLocalRef( contentURL );
	}

	void CAndroidApp::EnableInput( bool bEnable, const wchar_t* szFullScreenInitText )
	{
		if( !m_pActivity || !m_pActivity->clazz )
			return;
		JNIEnv* pJniEnv = NULL;
		JavaVMAttachArgs Args = { JNI_VERSION_1_6, "NativeThread", NULL };
		jint lResult = m_pJavaVM->AttachCurrentThread( &pJniEnv, &Args );
		GammaAst( lResult != JNI_ERR );

		m_bFullScreenInput = !!szFullScreenInitText;
		GammaLock( m_hInputLock );
		m_vecInputBuffer.clear();
		GammaUnlock( m_hInputLock );

		jobject jFullScreenInitText = NULL;
		if( szFullScreenInitText )
		{
			uint32_t l = wcslen( szFullScreenInitText );
			jchar* szBuffer = new jchar[l];
			for( uint32_t i = 0; i < l; i++ )
				szBuffer[i] = szFullScreenInitText[i];
			jFullScreenInitText = pJniEnv->NewString( szBuffer, l );
			delete[] szBuffer;
		}

		// Retrieves NativeActivity.
		jobject lNativeActivity = m_pActivity->clazz;
		jclass ClassNativeActivity = pJniEnv->GetObjectClass(lNativeActivity);
		jmethodID MethodEnableInput = pJniEnv->GetMethodID(
			ClassNativeActivity, "enableInput", "(ZLjava/lang/String;)V");
		if( MethodEnableInput )
			pJniEnv->CallVoidMethod( lNativeActivity,
			MethodEnableInput, bEnable, jFullScreenInitText );
		pJniEnv->DeleteLocalRef(ClassNativeActivity);
		pJniEnv->DeleteLocalRef(jFullScreenInitText);
	}

	bool CAndroidApp::GetSystemFile( bool bList, int32_t nType, void* pContext, void* funCallback )
	{
		if( !funCallback || !m_pActivity || !m_pActivity->clazz )
			return false;

		SFileOpenContext* pFileContext = new SFileOpenContext;
		pFileContext->m_nType = nType;
		pFileContext->m_pContext = pContext;
		pFileContext->m_funCallback = funCallback;

		JNIEnv* pJniEnv = NULL;
		JavaVMAttachArgs Args = { JNI_VERSION_1_6, "NativeThread", NULL };
		jint lResult = m_pJavaVM->AttachCurrentThread( &pJniEnv, &Args );
		GammaAst( lResult != JNI_ERR );

		jobject lNativeActivity = m_pActivity->clazz;
		jclass ClassNativeActivity = pJniEnv->GetObjectClass(lNativeActivity);

		if( bList )
		{
			pFileContext->m_nID = 0;
			const char* aryMedia[CONTENT_TYPE_COUNT] =
			{
				"android/provider/MediaStore$Files",
				"android/provider/MediaStore$Images$Media",
				"android/provider/MediaStore$Audio$Media",
				"android/provider/MediaStore$Video$Media",
			};
			
			jmethodID nMethodGetResolver = pJniEnv->GetMethodID( ClassNativeActivity,
				"getContentResolver", "()Landroid/content/ContentResolver;");
			jclass ContentResolverClass = pJniEnv->FindClass( "android/content/ContentResolver" );
			jmethodID MethQuery = pJniEnv->GetMethodID( ContentResolverClass, "query", 
				"(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;"
				"[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;");
			
			jclass MediaClass = pJniEnv->FindClass( aryMedia[nType] );
			jobject Uri;
			if( !nType )
			{
				jmethodID MethodGetContentUri = pJniEnv->GetStaticMethodID( MediaClass,
					"getContentUri", "(Ljava/lang/String;)Landroid/net/Uri;" );
				jstring jstrExernal = pJniEnv->NewStringUTF( "external" );
				Uri = pJniEnv->CallStaticObjectMethod( MediaClass,
					MethodGetContentUri, jstrExernal );
				pJniEnv->DeleteLocalRef( jstrExernal );
			}
			else
			{
				jfieldID UriField = pJniEnv->GetStaticFieldID(
						MediaClass, "EXTERNAL_CONTENT_URI", "Landroid/net/Uri;" );
				Uri = pJniEnv->GetStaticObjectField( MediaClass, UriField );
			}
			
			jclass CursorClass = pJniEnv->FindClass( "android/database/Cursor" );
			jmethodID nMethodMove = pJniEnv->GetMethodID( CursorClass, "moveToNext", "()Z");
			jmethodID nMethodClose = pJniEnv->GetMethodID( CursorClass, "close", "()V");
			jmethodID nMethodGetIndex = pJniEnv->GetMethodID( CursorClass,
					"getColumnIndexOrThrow", "(Ljava/lang/String;)I");
			jmethodID nMethodGetValue = pJniEnv->GetMethodID( CursorClass,
					"getString", "(I)Ljava/lang/String;");
			jobject Resolver = pJniEnv->CallObjectMethod( lNativeActivity, nMethodGetResolver );
			jobject Cursor = pJniEnv->CallObjectMethod(
					Resolver, MethQuery, Uri, NULL, NULL, NULL, NULL );
			if( Cursor )
			{
				jobject Path = pJniEnv->NewStringUTF( "_data" );
				while( pJniEnv->CallBooleanMethod( Cursor, nMethodMove ) )
				{
					int32_t nIndex = pJniEnv->CallIntMethod( Cursor, nMethodGetIndex, Path );
					jobject jPath = pJniEnv->CallObjectMethod( Cursor, nMethodGetValue, nIndex );
					jstring strPath = jPath ? (jstring)jPath : NULL;
					const char* szPath = jPath ? pJniEnv->GetStringUTFChars( strPath, NULL ) : "";
					pFileContext->m_vecFileName.push_back( szPath );
					pJniEnv->DeleteLocalRef( jPath );
				}
				pJniEnv->DeleteLocalRef( Path );
				pJniEnv->CallVoidMethod( Cursor, nMethodClose );
			}
			pJniEnv->DeleteLocalRef( ContentResolverClass );
			pJniEnv->DeleteLocalRef( MediaClass );
			pJniEnv->DeleteLocalRef( CursorClass );
			pJniEnv->DeleteLocalRef( Resolver );
			pJniEnv->DeleteLocalRef( Uri );
			pJniEnv->DeleteLocalRef( Cursor );
			m_listFileContext.PushBack( *pFileContext );
		}
		else
		{
			int32_t nID = ( ++m_nSysFileID )|SYSTEM_FILE_ID;
			pFileContext->m_nID = nID;
			m_Lock.Lock();
			m_mapFileContext.Insert( *pFileContext );
			m_Lock.Unlock();

			jmethodID nMethodID = pJniEnv->GetMethodID( ClassNativeActivity,
				"StartActivityForResultSynchronized", "(ILandroid/content/Intent;)V");
			if( nMethodID )
			{
				jclass IntentClass = pJniEnv->FindClass( "android/content/Intent" );
				jmethodID MethIntentInit = pJniEnv->GetMethodID
					( IntentClass, "<init>", "(Ljava/lang/String;)V");
				jmethodID MethIntentSetType = pJniEnv->GetMethodID
					( IntentClass, "setType", "(Ljava/lang/String;)Landroid/content/Intent;");

				jstring pAction = pJniEnv->NewStringUTF( "android.intent.action.GET_CONTENT" );
				jobject jIntent = pJniEnv->NewObject( IntentClass, MethIntentInit, pAction );

				const char* szType = "*";
				if( pFileContext->m_nType == CONTENT_TYPE_IMAGE )
					szType = "image/*";
				else if( pFileContext->m_nType == CONTENT_TYPE_MUSIC )
					szType = "audio/*";
				else if( pFileContext->m_nType == CONTENT_TYPE_VIDEO )
					szType = "video/*";

				jstring pStringText = pJniEnv->NewStringUTF( szType );
				pJniEnv->CallObjectMethod( jIntent, MethIntentSetType, pStringText );

				pJniEnv->CallVoidMethod( lNativeActivity, nMethodID, nID, jIntent );
				pJniEnv->DeleteLocalRef( pAction );
				pJniEnv->DeleteLocalRef( jIntent );
				pJniEnv->DeleteLocalRef( pStringText );
			}
		}

		pJniEnv->DeleteLocalRef( ClassNativeActivity );
		return true;
	}

	void* CAndroidApp::LoadDynamicLib( const char* szName )
	{
		string strName = szName;
		if( strName.find( ".so" ) == wstring::npos )
			strName += ".so";
		return dlopen( strName.c_str(), RTLD_NOW|RTLD_GLOBAL ); // 在全局空间加载
	}

	void* CAndroidApp::GetFunctionAddress( void* pLibContext, const char* szName )
	{
		return dlsym( RTLD_DEFAULT, szName );
	}

	void CAndroidApp::OnActivityResult( JNIEnv* env, int32_t nID, int32_t nResult, jobject Intent )
	{
		if( SYSTEM_FILE_ID == ( nID&0xffffff00 ) )
			return OnSystemFileCallback( env, nID, nResult, Intent );
	}
	
	void CAndroidApp::FinishedSystemFileCallback( SFileOpenContext* pFileContext )
	{
		m_Lock.Lock();
		m_listFileContext.PushBack( *pFileContext );
		m_Lock.Unlock();
	}
	
	void CAndroidApp::OnSystemFileCallback( JNIEnv* env, int32_t nID, int32_t nResult, jobject Intent )
	{
		m_Lock.Lock();
		SFileOpenContext* pFileContext = m_mapFileContext.Find( nID );
		if( !pFileContext )
			return m_Lock.Unlock();
		pFileContext->CFileOpenContextMap::CGammaRBTreeNode::Remove();
		m_Lock.Unlock();
		
		if( nResult != -1/*RESULT_OK*/ )
			return FinishedSystemFileCallback( pFileContext );
		
		jclass StringClass = env->FindClass( "java/lang/String" );
		jclass IntentClass = env->FindClass( "android/content/Intent" );
		jclass UriClass = env->FindClass( "android/net/Uri" );
		jmethodID MethodGetData = env->GetMethodID
				( IntentClass,  "getData", "()Landroid/net/Uri;");
		jobject Uri = env->CallObjectMethod( Intent, MethodGetData );
		if( Uri == NULL )
			return FinishedSystemFileCallback( pFileContext );
		
		jmethodID MethodGetAuthority = env->GetMethodID(
				UriClass, "getAuthority", "()Ljava/lang/String;" );
		jmethodID MethodGetScheme = env->GetMethodID(
				UriClass, "getScheme", "()Ljava/lang/String;" );
		jstring jstrAuthor = (jstring )env->CallObjectMethod( Uri, MethodGetAuthority );
		jstring jstrScheme = (jstring )env->CallObjectMethod( Uri, MethodGetScheme );
		const char* szAuthority = jstrAuthor ? env->GetStringUTFChars( jstrAuthor, NULL ) : "";
		const char* szScheme = jstrScheme ? env->GetStringUTFChars( jstrScheme, NULL ) : "";
		string strAuthority = szAuthority ? szAuthority : "";
		string strScheme = szScheme ? szScheme : "";
		jobject contentUri = NULL;
		jobject selection = NULL;
		jobject selectionArgs = NULL;
		
		jclass DocumentsContractClass = env->FindClass( "android/provider/DocumentsContract" );
		jmethodID MethodIsDocumentUri = env->GetStaticMethodID( DocumentsContractClass,
			"isDocumentUri", "(Landroid/content/Context;Landroid/net/Uri;)Z");
		jmethodID MethodGetDocumentId = env->GetStaticMethodID( DocumentsContractClass,
			"getDocumentId", "(Landroid/net/Uri;)Ljava/lang/String;");
		
		// isKitKat
		if( m_nSDKInit >= 19 && env->CallStaticBooleanMethod(
			DocumentsContractClass, MethodIsDocumentUri, m_pActivity->clazz, Uri ) )
		{
			jstring docId = (jstring)env->CallStaticObjectMethod(
					DocumentsContractClass, MethodGetDocumentId, Uri );
			const char* szDocID = docId ? env->GetStringUTFChars( docId, NULL ) : "";
			if( strAuthority == "com.android.externalstorage.documents" )
			{
				vector<string> split = SeparateString( szDocID, ':' );
				if( !stricmp( split[0].c_str(), "primary" ) )
				{
					jclass EnvironmentClass = env->FindClass( "android/os/Environment" );
					jmethodID MethodGetExternalStorageDirectory = env->GetStaticMethodID(
						EnvironmentClass, "getExternalStorageDirectory", "()Ljava/io/File;");
					jobject directory = env->CallStaticObjectMethod(
							EnvironmentClass, MethodGetExternalStorageDirectory, Uri );
					jclass FileClass = env->FindClass( "java/io/File" );
					jmethodID MethodToString = env->GetMethodID(
							FileClass, "toString", "()Ljava/lang/String;");
					jstring jPath = (jstring)env->CallObjectMethod( directory, MethodToString );
					const char* szDir = jPath ? env->GetStringUTFChars( jPath, NULL ) : "";
					pFileContext->m_vecFileName.push_back( string( szDir ) + "/" + split[1] );
					env->DeleteLocalRef( EnvironmentClass );
					env->DeleteLocalRef( directory );
				}
				else
				{
					string strPath = string( "/storage/" ) + split[0] + "/" + split[1];
					pFileContext->m_vecFileName.push_back( strPath );
				}
				return FinishedSystemFileCallback( pFileContext );
			}
			else if( strAuthority == "com.android.providers.downloads.documents" )
			{
				jmethodID MethodParse = env->GetStaticMethodID(
						UriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;" );
				jclass ContentUris = env->FindClass( "android/content/ContentUris" );
				jmethodID MethodWithAppendedId = env->GetStaticMethodID( ContentUris,
					"withAppendedId", "(Landroid/net/Uri;J)Landroid/net/Uri;");
				jobject TempUri = env->CallStaticObjectMethod( UriClass, MethodParse,
					env->NewStringUTF( "content://downloads/public_downloads" ) );
				contentUri = env->CallStaticObjectMethod( ContentUris,
					MethodWithAppendedId, TempUri, (jlong)GammaA2I64( szDocID ) );
				env->DeleteLocalRef( ContentUris );
				env->DeleteLocalRef( TempUri );
			}
			else if( strAuthority == "com.android.providers.media.documents" )
			{
				vector<string> split = SeparateString( szDocID, ':' );
				string strType;
				if( split[0] == "image" )
					strType = "android/provider/MediaStore$Images$Media";
				else if( split[0] == "audio" )
					strType = "android/provider/MediaStore$Audio$Media";
				else if( split[0] == "video" )
					strType = "android/provider/MediaStore$Video$Media";
				
				if( !strType.empty() )
				{
					jclass MediaClass = env->FindClass( strType.c_str() );
					jfieldID UriField = env->GetStaticFieldID(
							MediaClass, "EXTERNAL_CONTENT_URI", "Landroid/net/Uri;" );
					contentUri = env->GetStaticObjectField( MediaClass, UriField );
					env->DeleteLocalRef( MediaClass );
				}
				selection = env->NewStringUTF( "_id=?" );
				jstring arg = env->NewStringUTF( split[1].c_str() );
				selectionArgs = env->NewObjectArray( 1, StringClass, arg );
				env->DeleteLocalRef( arg );
			}
			env->DeleteLocalRef( docId );
		}
		else
		{
			if( !stricmp( strScheme.c_str(), "file" ) )
			{
				jmethodID MethodGetPath = env->GetMethodID(
					UriClass, "getPath", "()Ljava/lang/String;" );
				jstring path = (jstring)env->CallObjectMethod( Uri, MethodGetPath );
				const char* szPath = path ? env->GetStringUTFChars( path, NULL ) : "";
				pFileContext->m_vecFileName.push_back( szPath );
				env->DeleteLocalRef( path );
				return FinishedSystemFileCallback( pFileContext );
			}
			
			if( !stricmp( strScheme.c_str(), "content" ) )
				contentUri = Uri;
		}
		
		if( contentUri == NULL && selection == NULL && selectionArgs == NULL )
			return FinishedSystemFileCallback( pFileContext );
		
		jstring column = env->NewStringUTF( "_data" );
		jobject projection = env->NewObjectArray( 1, StringClass, column );
				
		jclass ClassNativeActivity = env->GetObjectClass( m_pActivity->clazz );
		jmethodID nMethodGetResolver = env->GetMethodID( ClassNativeActivity,
			"getContentResolver", "()Landroid/content/ContentResolver;");
		jclass ContentResolverClass = env->FindClass( "android/content/ContentResolver" );
		jmethodID MethQuery = env->GetMethodID( ContentResolverClass, "query",
			"(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;"
			"[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;");
		jclass CursorClass = env->FindClass( "android/database/Cursor" );
		jmethodID MethodMove = env->GetMethodID( CursorClass, "moveToFirst", "()Z");
		jmethodID MethodClose = env->GetMethodID( CursorClass, "close", "()V");
		jmethodID MethodGetIndex = env->GetMethodID( CursorClass,
			"getColumnIndexOrThrow", "(Ljava/lang/String;)I");
		jmethodID MethodGetValue = env->GetMethodID( CursorClass,
			"getString", "(I)Ljava/lang/String;");
		jobject Context = m_pActivity->clazz;
		jobject Resolver = env->CallObjectMethod( Context, nMethodGetResolver );
		jobject Cursor = env->CallObjectMethod( Resolver, MethQuery,
			contentUri, projection, selection, selectionArgs, NULL );
		if( Cursor )
		{
			if( env->CallBooleanMethod( Cursor, MethodMove ) )
			{
				int32_t nIndex = env->CallIntMethod(Cursor, MethodGetIndex, column);
				jobject jPath = env->CallObjectMethod(Cursor, MethodGetValue, nIndex);
				jstring strPath = jPath ? (jstring) jPath : NULL;
				const char *szPath = jPath ? env->GetStringUTFChars(strPath, NULL ) : "";
				pFileContext->m_vecFileName.push_back(szPath);
				env->DeleteLocalRef(jPath);
			}
			env->CallVoidMethod( Cursor, MethodClose );
			env->DeleteLocalRef( Cursor );
		}
		
		env->DeleteLocalRef( StringClass );
		env->DeleteLocalRef( IntentClass );
		env->DeleteLocalRef( UriClass );
		env->DeleteLocalRef( Uri );
		env->DeleteLocalRef( jstrAuthor );
		env->DeleteLocalRef( jstrScheme );
		env->DeleteLocalRef( DocumentsContractClass );
		env->DeleteLocalRef( column );
		env->DeleteLocalRef( projection );
		env->DeleteLocalRef( ContentResolverClass );
		env->DeleteLocalRef( CursorClass );
		env->DeleteLocalRef( Resolver );
		FinishedSystemFileCallback( pFileContext );
	}

	void CAndroidApp::AddCharMsgFromJava( const jchar* szBuffer, uint32_t nSize )
	{
		GammaLock( m_hInputLock );
		if( m_bFullScreenInput )
			m_vecInputBuffer.clear();
		for( uint32_t i = 0; i < nSize; i++ )
			m_vecInputBuffer.push_back( szBuffer[i] );
		GammaUnlock( m_hInputLock );
	}

	void CAndroidApp::OnInputMgrShowFromJava( bool bShow )
	{
		m_bInputVisible = bShow;
	}

	bool CAndroidApp::IsWifiConnect()
	{
		if( !m_pApplication )
			return false;

		try
		{
			JNIEnv* pJniEnv = NULL;
			JavaVMAttachArgs Args = { JNI_VERSION_1_6, "NativeThread", NULL };
			jint lResult = m_pJavaVM->AttachCurrentThread( &pJniEnv, &Args );
			GammaAst( lResult != JNI_ERR );

			jclass ClassApplication = pJniEnv->GetObjectClass( m_pApplication );
			jmethodID MethodGetSystemService = pJniEnv->GetMethodID( ClassApplication,
				"getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;" );

			jclass ConnectManagerClass = pJniEnv->FindClass( "android/net/ConnectivityManager" );
			jmethodID MethodGetNetworkInfo = pJniEnv->GetMethodID( ConnectManagerClass,
				"getNetworkInfo", "(I)Landroid/net/NetworkInfo;" );
			jclass NetworkInfoClass = pJniEnv->FindClass( "android/net/NetworkInfo" );
			jmethodID MethodIsConnected = pJniEnv->GetMethodID( NetworkInfoClass, "isConnected", "()Z" );

			bool bIsWifiConnect = false;
			jstring jstrConnective = pJniEnv->NewStringUTF( "connectivity" );
			jobject jobjConnectManager = pJniEnv->
				CallObjectMethod( m_pApplication, MethodGetSystemService, jstrConnective );
			if( jobjConnectManager )
			{
				jobject jobjNetworkInfo = pJniEnv->CallObjectMethod( jobjConnectManager, MethodGetNetworkInfo, 1 );
				bIsWifiConnect = pJniEnv->CallBooleanMethod( jobjNetworkInfo, MethodIsConnected );
				pJniEnv->DeleteLocalRef( jobjNetworkInfo );
				pJniEnv->DeleteLocalRef( jobjConnectManager );
			}

			pJniEnv->DeleteLocalRef( ClassApplication );
			pJniEnv->DeleteLocalRef( ConnectManagerClass );
			pJniEnv->DeleteLocalRef( NetworkInfoClass );
			pJniEnv->DeleteLocalRef( jstrConnective );
			return bIsWifiConnect;
		}
		catch(...)
		{
			return false;
		}
	}

	bool CAndroidApp::GetLocation( double& fLongitude, double& fLatitude, double& fAltitude )
	{
		if( !m_pApplication )
			return false;

		try
		{
			JNIEnv* pJniEnv = NULL;
			JavaVMAttachArgs Args = { JNI_VERSION_1_6, "NativeThread", NULL };
			jint lResult = m_pJavaVM->AttachCurrentThread( &pJniEnv, &Args );
			GammaAst( lResult != JNI_ERR );

			jclass ClassApplication = pJniEnv->GetObjectClass( m_pApplication );
			jmethodID MethodGetSystemService = pJniEnv->GetMethodID( ClassApplication,
				"getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;" );
			jmethodID getClassLoader = pJniEnv->GetMethodID(
				ClassApplication, "getClassLoader", "()Ljava/lang/ClassLoader;" );
			jobject loader = pJniEnv->CallObjectMethod( m_pApplication, getClassLoader );

			jclass classLoader = pJniEnv->FindClass( "java/lang/ClassLoader" );
			jmethodID loadClass = pJniEnv->GetMethodID(
				classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;" );

			jclass LocationManagerClass = pJniEnv->FindClass( "android/location/LocationManager" );
			jmethodID MethodGetProviders = pJniEnv->GetMethodID( LocationManagerClass,
				"getProviders", "(Z)Ljava/util/List;" );
			jmethodID MethodGetLocation = pJniEnv->GetMethodID( LocationManagerClass,
				"getLastKnownLocation", "(Ljava/lang/String;)Landroid/location/Location;" );

			jclass ListClass = pJniEnv->FindClass( "java/util/List" );
			jmethodID MethodContains = pJniEnv->GetMethodID( ListClass,
				"contains", "(Ljava/lang/Object;)Z" );

			jstring strCompatName = pJniEnv->
				NewStringUTF( "android/support/v4/app/ActivityCompat" );
			jclass CompatClass = (jclass)pJniEnv->
				CallObjectMethod( loader, loadClass, strCompatName );
			jmethodID MethodCheckSelfPermission = pJniEnv->GetStaticMethodID( CompatClass,
				"checkSelfPermission", "(Landroid/content/Context;Ljava/lang/String;)I" );

			jclass LocationClass = pJniEnv->FindClass( "android/location/Location" );
			jmethodID MethodGetLatitude = pJniEnv->
				GetMethodID( LocationClass,	"getLatitude", "()D" );
			jmethodID MethodGetLongitude = pJniEnv->
				GetMethodID( LocationClass,	"getLongitude", "()D" );
			jmethodID MethodGetAltitude = pJniEnv->
				GetMethodID( LocationClass,	"getAltitude", "()D" );

			jstring jstrLocation = pJniEnv->NewStringUTF( "location" );
			jstring jstrGps = pJniEnv->NewStringUTF( "gps" );
			jstring jstrNetwork = pJniEnv->NewStringUTF( "network" );

			const char* szAccessLocation = "android.permission.ACCESS_COARSE_LOCATION";
			const char* szAccessFine = "android.permission.ACCESS_FINE_LOCATION";
			jstring jstrAccessLocation = pJniEnv->NewStringUTF( szAccessLocation );
			jstring jstrAccessFine = pJniEnv->NewStringUTF( szAccessFine );

			jobject jobjLocationManager = pJniEnv->
				CallObjectMethod( m_pApplication, MethodGetSystemService, jstrLocation );

			jobject jobjLocation = NULL;
			if( jobjLocationManager )
			{
				jobject jobjList = pJniEnv->
					CallObjectMethod( jobjLocationManager, MethodGetProviders, true );
				bool bHasGps = pJniEnv->
					CallBooleanMethod( jobjList, MethodContains, jstrGps );
				bool bHasNetwork = pJniEnv->
					CallBooleanMethod( jobjList, MethodContains, jstrNetwork );
				if( pJniEnv->CallStaticIntMethod( CompatClass,
					MethodCheckSelfPermission, m_pApplication, jstrAccessFine ) == 0 ||
					pJniEnv->CallStaticIntMethod( CompatClass,
					MethodCheckSelfPermission, m_pApplication, jstrAccessLocation ) == 0 )
				{
					if( !jobjLocation && bHasGps )
						jobjLocation = pJniEnv->CallObjectMethod
						( jobjLocationManager, MethodGetLocation, jstrGps );
					if( !jobjLocation && bHasNetwork )
						jobjLocation = pJniEnv->CallObjectMethod
						( jobjLocationManager, MethodGetLocation, jstrNetwork );
				}
				pJniEnv->DeleteLocalRef( jobjList );
				pJniEnv->DeleteLocalRef( jobjLocationManager );
			}

			if( jobjLocation )
			{
				fLatitude = pJniEnv->CallDoubleMethod( jobjLocation, MethodGetLatitude );
				fLongitude = pJniEnv->CallDoubleMethod( jobjLocation, MethodGetLongitude );
				fAltitude = pJniEnv->CallDoubleMethod( jobjLocation, MethodGetAltitude );
				pJniEnv->DeleteLocalRef( jobjLocation );
			}

			pJniEnv->DeleteLocalRef( ClassApplication );
			pJniEnv->DeleteLocalRef( loader );
			pJniEnv->DeleteLocalRef( classLoader );
			pJniEnv->DeleteLocalRef( LocationManagerClass );
			pJniEnv->DeleteLocalRef( ListClass );
			pJniEnv->DeleteLocalRef( strCompatName );
			pJniEnv->DeleteLocalRef( CompatClass );
			pJniEnv->DeleteLocalRef( LocationClass );
			pJniEnv->DeleteLocalRef( jstrLocation );
			pJniEnv->DeleteLocalRef( jstrGps );
			pJniEnv->DeleteLocalRef( jstrNetwork );
			pJniEnv->DeleteLocalRef( jstrAccessLocation );
			pJniEnv->DeleteLocalRef( jstrAccessFine );
			return jobjLocation != NULL;
		}
		catch(...)
		{
			return false;
		}
	}

	//===================================================================
	// framework
	//===================================================================
	int32_t CAndroidApp::AndroidMessagePump()
	{
		STATCK_LOG( this );
		if( m_bDestroyed || m_pActivity == NULL )
			return 0;
		PROCESS_LOG;

		int32_t nCommandCount = ProcessCommand();
		if( nCommandCount < 0 )
			return -1;

		PROCESS_LOG;
		int32_t nInputCount = ProcessInput();
		PROCESS_LOG;
		if( nInputCount < 0 )
			return nCommandCount;

		while( m_listFileContext.GetFirst() )
		{
			SFileOpenContext* pContext = m_listFileContext.GetFirst();
			m_Lock.Lock();
			pContext->CFileOpenContextList::CGammaListNode::Remove();
			m_Lock.Unlock();
			if( pContext->m_funCallback )
			{
				if( pContext->m_nID == 0 )
				{
					vector<const char*> vecPath;
					for( uint32_t i = 0; i < pContext->m_vecFileName.size(); i++ )
						vecPath.push_back( pContext->m_vecFileName[i].c_str() );
					const char** aryPath = vecPath.empty() ? NULL : &vecPath[0];
					( (SystemFileListCallback)pContext->m_funCallback )(
							pContext->m_pContext, aryPath, vecPath.size() );
				}
				else
				{
					const char* szPath = pContext->m_vecFileName.size() ?
										 pContext->m_vecFileName[0].c_str() : NULL;
					( (SystemFileCallback)pContext->m_funCallback )
							( pContext->m_pContext, szPath );
				}
			}
			delete pContext;
		}
		return nCommandCount + nInputCount;
	}

	int32_t CAndroidApp::DispatchMessage( uint32_t nInputID, uint32_t nMsg, uint32_t wParam, uint32_t lParam )
	{
		for( map<void*, InputHandler>::iterator it = m_mapHandlers.begin();
			it != m_mapHandlers.end(); ++it )
		{
			if( !(*it->second)( it->first, nInputID, nMsg, wParam, lParam ) )
				continue;
			return 1;
		}
		return 0;
	}

	void CAndroidApp::RegisterMsgHandler( void* pContext, InputHandler handler )
	{
		if( handler )
			m_mapHandlers[pContext] = handler;
		else
			m_mapHandlers.erase( pContext );
	}

	int32_t CAndroidApp::AndroidInputHandler( AInputEvent* event )
	{
		uint32_t nMsg = 0, wParam = 0, lParam = 0, nInputID = 0;
		uint32_t nInputType = AInputEvent_getType( event );

		//TODO:处理各种Input事件
		if( nInputType == AINPUT_EVENT_TYPE_MOTION ) 
		{
			int32_t nActionCode = AMotionEvent_getAction( event );
			int32_t nAction = nActionCode & AMOTION_EVENT_ACTION_MASK;
			int32_t nActionPointerIndex = nActionCode >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
			nInputID = AMotionEvent_getPointerId( event, nActionPointerIndex );

			switch( nAction )
			{
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
			case AMOTION_EVENT_ACTION_DOWN:
				nMsg = WM_LBUTTONDOWN;
				wParam = MK_LBUTTON;
				break;
			case AMOTION_EVENT_ACTION_POINTER_UP:
			case AMOTION_EVENT_ACTION_UP:
			case AMOTION_EVENT_ACTION_CANCEL:
				nMsg = WM_LBUTTONUP;
				break;
			case AMOTION_EVENT_ACTION_MOVE:
				{
					wParam = MK_LBUTTON;
					for( int nPoint = 0; nPoint < AMotionEvent_getPointerCount(event); nPoint++ )
					{
						lParam = (uint32_t)AMotionEvent_getX( event, nPoint ) |
							( (uint32_t)AMotionEvent_getY( event, nPoint ) << 16 );
						DispatchMessage( AMotionEvent_getPointerId( event, nPoint ), WM_MOUSEMOVE, MK_LBUTTON, lParam );
					}
				}
				return 1;
			}

			lParam = (uint32_t)AMotionEvent_getX( event, nActionPointerIndex ) |
				( (uint32_t)AMotionEvent_getY( event, nActionPointerIndex ) << 16 );
		}
		else if( nInputType == AINPUT_EVENT_TYPE_KEY ) 
		{
			int32_t nAction = AKeyEvent_getAction( event );
			if( nAction == AKEY_EVENT_ACTION_DOWN )
				nMsg = WM_KEYDOWN;
			else if( nAction == AKEY_EVENT_ACTION_UP )
				nMsg = WM_KEYUP;
			else
				return 1;

			int32_t nKeyCode = AKeyEvent_getKeyCode( event );
			switch( nKeyCode )
			{
			case AKEYCODE_BACK:
				wParam = VK_ESCAPE;
				break;
			case AKEYCODE_DEL:
				wParam = VK_BACK;
				break;
			case AKEYCODE_ENTER:
				wParam = VK_RETURN;
				break;
			case AKEYCODE_CTRL_LEFT:
				if( DispatchMessage( nInputID, nMsg, VK_CONTROL, lParam ) )
					return 1;
				wParam = VK_LCONTROL;
				break;
			case AKEYCODE_CTRL_RIGHT:
				if( DispatchMessage( nInputID, nMsg, VK_CONTROL, lParam ) )
					return 1;
				wParam = VK_RCONTROL;
				break;
			case AKEYCODE_ALT_LEFT:
				if( DispatchMessage( nInputID, nMsg, VK_MENU, lParam ) )
					return 1;
				wParam = VK_LMENU;
				break;
			case AKEYCODE_ALT_RIGHT:
				if( DispatchMessage( nInputID, nMsg, VK_MENU, lParam ) )
					return 1;
				wParam = VK_RMENU;
				break;
			case AKEYCODE_SHIFT_LEFT:
				if( DispatchMessage( nInputID, nMsg, VK_SHIFT, lParam ) )
					return 1;
				wParam = VK_LSHIFT;
				break;
			case AKEYCODE_SHIFT_RIGHT:
				if( DispatchMessage( nInputID, nMsg, VK_SHIFT, lParam ) )
					return 1;
				wParam = VK_RSHIFT;
				break;
			case AKEYCODE_TAB:
				wParam = VK_TAB;
				break;
			case AKEYCODE_SPACE:
				wParam = VK_SPACE;
				break;
			case AKEYCODE_VOLUME_UP:
				wParam = VK_XBUTTON1;
				break;
			case AKEYCODE_VOLUME_DOWN:
				wParam = VK_XBUTTON2;
				break;
			default:
				if( nKeyCode >= AKEYCODE_0 && nKeyCode <= AKEYCODE_9 )
					wParam = '0' + nKeyCode - AKEYCODE_0;
				else if( nKeyCode >= AKEYCODE_A && nKeyCode <= AKEYCODE_Z )
					wParam = 'A' + nKeyCode - AKEYCODE_A;
				else if( nKeyCode >= AKEYCODE_F1 && nKeyCode <= AKEYCODE_F12 )
					wParam = VK_F1 + nKeyCode - AKEYCODE_F1;
				else //[todo]
					return 1;
				break;
			}
		}

		if( nMsg && DispatchMessage( nInputID, nMsg, wParam, lParam ) )
			return 1;

		if( WM_KEYDOWN == nMsg || WM_KEYUP == nMsg ) 
		{
			if( wParam == VK_ESCAPE )
				return 1;
			if( wParam == VK_BACK && !m_bFullScreenInput && m_bInputVisible )
				return 1;
		}

		return 0;
	}

	void CAndroidApp::AndroidCmdHandler( int32_t cmd )
	{
		GammaLog << "AndroidCmdHandler begin:" << cmd << endl;

		switch( cmd ) 
		{
		case APP_CMD_INPUT_CHANGED:			// 0
			break;
		case APP_CMD_INIT_WINDOW:			// 1
			DispatchMessage( 0, WM_CREATE, 0, 0 );
			break;
		case APP_CMD_TERM_WINDOW:			// 2
			DispatchMessage( 0, WM_DESTROY, 0, 0 );
			break;
		case APP_CMD_WINDOW_RESIZED:		// 3
			DispatchMessage( 0, WM_SIZE, SIZE_RESTORED, 0 );
			break;
		case APP_CMD_WINDOW_REDRAW_NEEDED:	// 4
			break;
		case APP_CMD_CONTENT_RECT_CHANGED:	// 5
			break;
		case APP_CMD_GAINED_FOCUS:			// 6
			DispatchMessage( 0, WM_SETFOCUS, 0, 0 );
			break;
		case APP_CMD_LOST_FOCUS:	
			DispatchMessage( 0, WM_KILLFOCUS, 0, 0 );
			break;
		case APP_CMD_CONFIG_CHANGED:		// 8
			break;
		case APP_CMD_LOW_MEMORY:			// 9
			DispatchMessage( 0, WM_LOW_MEMORY, 0, 0 );
			break;
		case APP_CMD_START:					// 10
			DispatchMessage( 0, WM_ACTIVATEAPP, 1, 0 );
			break;
		case APP_CMD_RESUME:				// 11
			DispatchMessage( 0, WM_ACTIVATE, 1, 0 );
			break;
		case APP_CMD_SAVE_STATE:			// 12
			break;
		case APP_CMD_PAUSE:					// 13
			DispatchMessage( 0, WM_ACTIVATE, 0, 0 );
			break;
		case APP_CMD_STOP:					// 14
			DispatchMessage( 0, WM_ACTIVATEAPP, 0, 0 );
			break;
		case APP_CMD_DESTROY:				// 15
			break;
		}
		GammaLog << "AndroidCmdHandler end:" << cmd << endl;
	}

	void CAndroidApp::Run( ANativeActivity* pActivity, void* pSavedState, size_t nSavedStateSize )
	{
		// 获取全局相关信息
		if( !m_pJavaVM )
		{
			m_pJavaVM = pActivity->vm;
			JNIEnv* pJniEnv = pActivity->env;

			jobject lNativeActivity = pActivity->clazz;
			jclass ClassActivity = pJniEnv->GetObjectClass( lNativeActivity );

			jmethodID MethodGetApplication = pJniEnv->
				GetMethodID( ClassActivity, "getApplication", "()Landroid/app/Application;" );
			m_pApplication = pJniEnv->CallObjectMethod( lNativeActivity, MethodGetApplication );
			m_pApplication = pJniEnv->NewGlobalRef( m_pApplication );

			jmethodID MethodGetExtStorage = pJniEnv->GetMethodID(
				ClassActivity, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;" );
			jmethodID MethodGetFilesDir = pJniEnv->
				GetMethodID( ClassActivity, "getFilesDir", "()Ljava/io/File;" );
			jobject pExtStorage = pJniEnv->
				CallObjectMethod( lNativeActivity, MethodGetExtStorage, NULL );
			if( !pExtStorage )
				pExtStorage = pJniEnv->CallObjectMethod( lNativeActivity, MethodGetFilesDir );
			jclass ClassExtStorage = pJniEnv->GetObjectClass( pExtStorage );
			jmethodID MethodGetPath = pJniEnv->
				GetMethodID( ClassExtStorage, "getPath", "()Ljava/lang/String;" );
			jstring jstrExternalPath = (jstring)pJniEnv->
				CallObjectMethod( pExtStorage, MethodGetPath );
			const char* szExternalPath = pJniEnv->GetStringUTFChars( jstrExternalPath, NULL );
			strcpy2array_safe( m_szExternalPath, szExternalPath );
			if( m_szExternalPath[strlen( m_szExternalPath ) - 1 ] != '/' )
				strcat( m_szExternalPath, "/" );
			pJniEnv->ReleaseStringUTFChars( jstrExternalPath, szExternalPath );
			pJniEnv->DeleteLocalRef( pExtStorage );
			pJniEnv->DeleteLocalRef( ClassExtStorage );
			pJniEnv->DeleteLocalRef( jstrExternalPath );
			GammaLog << "getExternalFilesDir" << endl;

			jmethodID MethodGetPackagePath = pJniEnv->
				GetMethodID( ClassActivity, "getPackageResourcePath", "()Ljava/lang/String;" );
			jstring jstrPackagePath = (jstring)pJniEnv->
				CallObjectMethod( lNativeActivity, MethodGetPackagePath, NULL );
			const char* szPackagePath = pJniEnv->GetStringUTFChars( jstrPackagePath, NULL );
			strcpy2array_safe( m_szPackagePath, szPackagePath );
			pJniEnv->ReleaseStringUTFChars( jstrPackagePath, szPackagePath );
			GammaLog << "getPackageResourcePath" << endl;

			// 获取程序版本
			jclass PackageManagerClass = pJniEnv->FindClass( "android/content/pm/PackageManager" );
			jclass PackageInfoClass = pJniEnv->FindClass( "android/content/pm/PackageInfo" );
			jclass SignatureClass = pJniEnv->FindClass( "android/content/pm/Signature" );
			jmethodID MethodGetPackageMgr = pJniEnv->GetMethodID
				( ClassActivity, "getPackageManager", "()Landroid/content/pm/PackageManager;" );
			jmethodID MethodGetPackageName = pJniEnv->GetMethodID
				( ClassActivity, "getPackageName", "()Ljava/lang/String;" );
			jmethodID MethodGetPackageInfo = pJniEnv->
				GetMethodID( PackageManagerClass, "getPackageInfo",
				"(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;" );
			jfieldID FieldVersionName = pJniEnv->
				GetFieldID( PackageInfoClass, "versionName", "Ljava/lang/String;" );
			jfieldID FieldSignatureArray = pJniEnv->
				GetFieldID( PackageInfoClass, "signatures", "[Landroid/content/pm/Signature;" );
			jmethodID MethodToByteArray = pJniEnv->
				GetMethodID( SignatureClass, "toByteArray", "()[B" );

			jstring jstrPackageName = (jstring)pJniEnv->
				CallObjectMethod( lNativeActivity, MethodGetPackageName );
			jobject jPackageManager = pJniEnv->
				CallObjectMethod( lNativeActivity, MethodGetPackageMgr );
			jobject jPackageInfo = pJniEnv->
				CallObjectMethod( jPackageManager, MethodGetPackageInfo, jstrPackageName, 64 );
			jstring jstrVersionName = (jstring)pJniEnv->
				GetObjectField( jPackageInfo, FieldVersionName );
			const char* szVersionName = pJniEnv->
				GetStringUTFChars( jstrVersionName, NULL );
			m_nVersion = CVersion( szVersionName );
			pJniEnv->ReleaseStringUTFChars( jstrVersionName, szVersionName );
			GammaLog << "getVersionName" << endl;

			jobjectArray jarySignature = (jobjectArray)pJniEnv->
				GetObjectField( jPackageInfo, FieldSignatureArray );
			jobject jSignature = pJniEnv->GetObjectArrayElement( jarySignature, 0 );
			jbyteArray jSignatureByteArray = (jbyteArray)pJniEnv->
				CallObjectMethod( jSignature, MethodToByteArray );

			jbyte* pByteArray = pJniEnv->GetByteArrayElements( jSignatureByteArray, 0 );
			int nArrayLen = pJniEnv->GetArrayLength( jSignatureByteArray );
			memset( m_arySignatureMd5, 0, sizeof(m_arySignatureMd5) );
			MD5Ex( m_arySignatureMd5, pByteArray, nArrayLen );
			pJniEnv->ReleaseByteArrayElements( jSignatureByteArray, pByteArray, 0 );

			CPathMgr::SetCurPath( "external:/" );
		}

		pActivity->callbacks->onDestroy = OnDestroy;
		pActivity->callbacks->onStart = OnStart;
		pActivity->callbacks->onResume = OnResume;
		pActivity->callbacks->onSaveInstanceState = OnSaveInstanceState;
		pActivity->callbacks->onPause = OnPause;
		pActivity->callbacks->onStop = OnStop;
		pActivity->callbacks->onConfigurationChanged = OnConfigurationChanged;
		pActivity->callbacks->onLowMemory = OnLowMemory;
		pActivity->callbacks->onWindowFocusChanged = OnWindowFocusChanged;
		pActivity->callbacks->onNativeWindowCreated = OnNativeWindowCreated;
		pActivity->callbacks->onNativeWindowDestroyed = OnNativeWindowDestroyed;
		pActivity->callbacks->onNativeWindowResized = OnNativeWindowResized;
		pActivity->callbacks->onInputQueueCreated = OnInputQueueCreated;
		pActivity->callbacks->onInputQueueDestroyed = OnInputQueueDestroyed;
		pActivity->instance = this;

		m_pActivity = pActivity;
		if( pSavedState != NULL )
		{
			m_pSavedState = malloc( nSavedStateSize );
			m_nSavedStateSize = nSavedStateSize;
			memcpy( m_pSavedState, pSavedState, m_nSavedStateSize );
		}

		int msgpipe[2];
		if( pipe( msgpipe ) )
			return;

		m_nCommandRead = msgpipe[0];
		m_nCommandWrite = msgpipe[1];

		if( !m_bMainThreadRunning )
		{
			struct SStartUp
			{
				static void* Run( void* pContext )
				{
					( (CAndroidApp*)pContext )->m_pMaxMainStack = &pContext;
					GammaLog << "Start Main Thread -> Stack:" 
						<< (uint64_t)(ptrdiff_t)&pContext << "," 
						<< ( (CAndroidApp*)pContext )->m_nMainStackSize << endl;
					( (CAndroidApp*)pContext )->MainThread();
					return NULL;
				}
			};

			pthread_attr_t attr; 
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
			pthread_attr_getstacksize( &attr, &m_nMainStackSize );
			pthread_create( &m_threadMain, &attr, &SStartUp::Run, this );
			m_bMainThreadRunning = true;
		}

		m_bDestroyed = false;
		m_bDestroyRequested = false;
	}

	int32_t CAndroidApp::MainThread()
	{
		//int64_t nPreTime = 0;
		//int64_t nCurTime = 0;
		//nPreTime = GetGammaTime();
		//nCurTime = GetGammaTime();
		//while( nCurTime < nPreTime + 500 )
		//{
		//	nPreTime = nCurTime;
		//	nCurTime = GetGammaTime();
		//}

		m_pConfig = AConfiguration_new();
		AConfiguration_fromAssetManager( m_pConfig, m_pActivity->assetManager );

		try
		{
			GammaLog << "main( 1, \"\" )" << endl;
			const char* szArg = "";
			main( 1, &szArg );
		}
		catch( std::exception& e )
		{
			GammaErr << e.what() << endl;
		}
		catch( const std::string& e )
		{
			GammaErr << e.c_str() << endl;
		}
		catch( const char* e )
		{
			GammaErr << e << endl;
		}
		catch( ... )
		{
			GammaErr << "UnknowError" << endl;
		}

		AConfiguration_delete( m_pConfig );

		if( m_pActivity )
		{
			// 结束进程
			JNIEnv* pJniEnv = NULL;
			JavaVMAttachArgs Args = { JNI_VERSION_1_6, "NativeThread", NULL };
			jint lResult = m_pJavaVM->AttachCurrentThread( &pJniEnv, &Args );
			GammaAst( lResult != JNI_ERR );
			// Retrieves NativeActivity.
			jobject lNativeActivity = m_pActivity->clazz;
			jclass ClassNativeActivity = pJniEnv->GetObjectClass( lNativeActivity );
			jmethodID MethodExit = pJniEnv->GetMethodID( ClassNativeActivity, "exit", "()V" );
			pJniEnv->CallVoidMethod( lNativeActivity, MethodExit );
			pJniEnv->DeleteLocalRef( ClassNativeActivity );
		}

		while( !m_bDestroyed )
		{
			AndroidMessagePump();
			GammaSleep( 10 );
		}
		// Can't touch android_app object after this.
		return 0;
	}

	void* CAndroidApp::SaveInstanceState( size_t* &outLen )
	{
		void* pSavedState = NULL;
		pthread_mutex_lock( &m_Mutex );
		m_bStateSaved = false;
		WriteCommand( APP_CMD_SAVE_STATE );
		while ( !m_bStateSaved )
			pthread_cond_wait( &m_Condiction, &m_Mutex );

		if( m_pSavedState != NULL )
		{
			pSavedState = m_pSavedState;
			*outLen = m_nSavedStateSize;
			m_pSavedState = NULL;
			m_nSavedStateSize = 0;
		}
		pthread_mutex_unlock( &m_Mutex );
		return pSavedState;
	}

	void CAndroidApp::FreeSavedState()
	{
		pthread_mutex_lock( &m_Mutex );
		if( m_pSavedState != NULL ) 
		{
			free( m_pSavedState );
			m_pSavedState = NULL;
			m_nSavedStateSize = 0;
		}
		pthread_mutex_unlock( &m_Mutex );
	}

	int8_t CAndroidApp::ReadCommand()
	{
		int8_t cmd;
		if( read( m_nCommandRead, &cmd, sizeof(cmd) ) == sizeof(cmd) )
			return cmd;
		return -1;
	}

	void CAndroidApp::PreExecCommand( int8_t nCmd )
	{
		switch( nCmd )
		{
		case APP_CMD_INPUT_CHANGED:
			pthread_mutex_lock( &m_Mutex );
			m_pInputQueue = m_pPendingInputQueue;
			pthread_cond_broadcast( &m_Condiction );
			pthread_mutex_unlock( &m_Mutex );
			break;

		case APP_CMD_INIT_WINDOW:
			pthread_mutex_lock( &m_Mutex );
			m_pNativeWindow = m_pPendingWindow;
			pthread_cond_broadcast( &m_Condiction );
			pthread_mutex_unlock( &m_Mutex );
			FetchHardwareInfo();
			break;

		case APP_CMD_TERM_WINDOW:
			pthread_cond_broadcast( &m_Condiction );
			break;

		case APP_CMD_RESUME:
		case APP_CMD_START:
		case APP_CMD_PAUSE:
		case APP_CMD_STOP:
			pthread_mutex_lock( &m_Mutex );
			m_eActivityState = (EActivityState)nCmd;
			pthread_cond_broadcast( &m_Condiction );
			pthread_mutex_unlock( &m_Mutex );
			break;

		case APP_CMD_SAVE_STATE:
			FreeSavedState();
			break;

		case APP_CMD_CONFIG_CHANGED:
			AConfiguration_fromAssetManager( m_pConfig,
				m_pActivity->assetManager );
			break;

		case APP_CMD_DESTROY:
			m_bDestroyRequested = true;
			break;
		}
	}

	void CAndroidApp::PostExecCommand( int8_t nCmd )
	{
		switch( nCmd )
		{
		case APP_CMD_TERM_WINDOW:
			pthread_mutex_lock( &m_Mutex );
			m_pNativeWindow = NULL;
			pthread_cond_broadcast( &m_Condiction );
			pthread_mutex_unlock( &m_Mutex );
			break;

		case APP_CMD_SAVE_STATE:
			pthread_mutex_lock( &m_Mutex );
			m_bStateSaved = true;
			pthread_cond_broadcast( &m_Condiction );
			pthread_mutex_unlock( &m_Mutex );
			break;

		case APP_CMD_RESUME:
			FreeSavedState();
			m_bActivityRunning = true;
			break;

		case APP_CMD_PAUSE:
			m_bActivityRunning = false;
			break;
		}
	}

	void CAndroidApp::WriteCommand( int8_t nCmd )
	{
		GammaLog << "WriteCommand:" << (int32_t)nCmd << endl;
		write( m_nCommandWrite, &nCmd, sizeof(nCmd) );
	}

	void CAndroidApp::SetInput( AInputQueue* inputQueue )
	{
		pthread_mutex_lock( &m_Mutex );
		m_pPendingInputQueue = inputQueue;
		WriteCommand( APP_CMD_INPUT_CHANGED );
		while( inputQueue != m_pPendingInputQueue )
			pthread_cond_wait( &m_Condiction, &m_Mutex );
		pthread_mutex_unlock( &m_Mutex );
	}

	void CAndroidApp::SetWindow( ANativeWindow* window )
	{
		pthread_mutex_lock( &m_Mutex );
		if( m_pPendingWindow != NULL )
			WriteCommand( APP_CMD_TERM_WINDOW );
		m_pPendingWindow = window;
		if( window != NULL ) 
			WriteCommand( APP_CMD_INIT_WINDOW );
		while( m_pNativeWindow != m_pPendingWindow )
			pthread_cond_wait( &m_Condiction, &m_Mutex );
		pthread_mutex_unlock( &m_Mutex );
	}

	void CAndroidApp::SetActivityState( int8_t cmd )
	{
		pthread_mutex_lock( &m_Mutex );
		WriteCommand( cmd );
		while( m_eActivityState != cmd )
			pthread_cond_wait( &m_Condiction, &m_Mutex );
		pthread_mutex_unlock( &m_Mutex );
	}

	void CAndroidApp::Free()
	{
		pthread_mutex_lock( &m_Mutex );
		WriteCommand( APP_CMD_DESTROY );
		while( !m_bDestroyed )
			pthread_cond_wait( &m_Condiction, &m_Mutex );
		pthread_mutex_unlock( &m_Mutex );

		close( m_nCommandRead );
		close( m_nCommandWrite );
		m_nCommandRead = 0;
		m_nCommandWrite = 0;
		m_pActivity = NULL;
		//free(android_app);
	}

	int32_t CAndroidApp::ProcessCommand()
	{
		STATCK_LOG( this );
		struct pollfd pfd = { m_nCommandRead, POLLIN, 0 };
		if( poll( &pfd, 1, 0 ) < 0 || pfd.revents != POLLIN )
			return 0;

		PROCESS_LOG;
		int8_t cmd = ReadCommand();
		PreExecCommand( cmd );
		PROCESS_LOG;
		AndroidCmdHandler( cmd );
		PROCESS_LOG;
		PostExecCommand( cmd );

		PROCESS_LOG;
		// Check if we are exiting.
		if ( !m_bDestroyRequested )
			return 1;

		FreeSavedState();
		pthread_mutex_lock( &m_Mutex );
		m_bDestroyed = true;
		pthread_cond_broadcast( &m_Condiction );
		pthread_mutex_unlock( &m_Mutex );
		return -1;
	}

	int32_t CAndroidApp::ProcessInput()
	{
		STATCK_LOG( this );
		if( !m_pInputQueue || !m_bActivityRunning )
			return -1;

		int32_t nCount = 0;
		AInputEvent* pEvent = NULL;
		PROCESS_LOG;
		while( AInputQueue_hasEvents( m_pInputQueue ) > 0 )
		{
			PROCESS_LOG;
			if( AInputQueue_getEvent( m_pInputQueue, &pEvent ) < 0 )
				break;

			PROCESS_LOG;

			// This is for a bug in 4.2, which make the keyboard unable to be closed with the back key.
			bool bSkipPreDispatch = AInputEvent_getType( pEvent ) == AINPUT_EVENT_TYPE_KEY
				&& AKeyEvent_getKeyCode( pEvent ) == AKEYCODE_BACK;

			// TODO: Not sure if we should skip the predispatch all together
			//       or run it but not return afterwards. The main thing
			//       is that the code below this 'if' block will be called.
			if( !bSkipPreDispatch && AInputQueue_preDispatchEvent( m_pInputQueue, pEvent ) )
			{
				// NOTE: This is for a bug in 2.3.x, which make the even do not be consumed 
				//       if no AInputQueue_finishEvent calling.
				//       But after 2.3, even is consumed automatically by AInputQueue_preDispatchEvent,
				//		 so can not call AInputQueue_finishEvent after preDispatchEvent
				if( !memcmp( "Android 2.3", m_HardwareDesc.m_szOSDesc, strlen( "Android 2.3" ) ) )
					AInputQueue_finishEvent( m_pInputQueue, pEvent, 0 );
				continue;
			}

			PROCESS_LOG;
			nCount++;

			int32_t nHandledCount = 1;
			if( bSkipPreDispatch && m_bInputVisible )
				EnableInput( false, NULL );
			else
				nHandledCount = AndroidInputHandler( pEvent );

			AInputQueue_finishEvent( m_pInputQueue, pEvent, nHandledCount );
		}

		PROCESS_LOG;
		GammaLock( m_hInputLock );
		uint32_t nCharCount = (uint32_t)m_vecInputBuffer.size();
		uint32_t nSize = Max<uint32_t>( nCharCount*sizeof(uint16_t), 1 );
		uint16_t* aryBuffer = (uint16_t*)alloca( nSize );
		if( nCharCount )
			memcpy( aryBuffer, &m_vecInputBuffer[0], nSize );
		m_vecInputBuffer.clear();
		PROCESS_LOG;
		GammaUnlock( m_hInputLock );
		PROCESS_LOG;

		if( nCharCount && m_bFullScreenInput )
			DispatchMessage( 0, WM_CHAR, 0, 0 );

		for( uint32_t i = 0; i < nCharCount; i++ )
		{
			if( aryBuffer[i] )
			{
				DispatchMessage( 0, WM_CHAR, aryBuffer[i], 0 );
				continue;
			}

			DispatchMessage( 0, WM_KEYDOWN, VK_BACK, 0 );
			DispatchMessage( 0, WM_KEYUP, VK_BACK, 0 );
		}
		return nCount + ( nCharCount ? 1 : 0 );
	}

	void CAndroidApp::FetchHardwareInfo()
	{
		if( m_HardwareDesc.m_szOSDesc[0] )
			return;

		try
		{
			/*
			char	m_szDeviceDesc[DEVICEDESC_LEN]*;
			char	m_szCpuType[CPUTYPE_LEN]*;
			char	m_szOSDesc[OSDESC_LEN]*;
			char	m_szLanguage[LANGUAGE_LEN]*;
			uint64_t	m_nMac*;
			uint32_t	m_nCpuFrequery*;
			uint32_t	m_nCpuCount*;
			uint32_t	m_nMemSize*;
			uint16_t  m_nScreen_X*;
			uint16_t	m_nScreen_Y*;
			uint32_t	m_nVideoMemSize;
			*/

			JNIEnv* pJniEnv = NULL;
			JavaVMAttachArgs Args = { JNI_VERSION_1_6, "NativeThread", NULL };
			jint lResult = m_pJavaVM->AttachCurrentThread( &pJniEnv, &Args );
			GammaAst( lResult != JNI_ERR );

			FILE* fp;
			char szTempBuffer[2048];

			jclass BuildClass = pJniEnv->FindClass( "android/os/Build" );
			jfieldID Brand = pJniEnv->GetStaticFieldID( BuildClass, "BRAND", "Ljava/lang/String;" );
			jfieldID Model = pJniEnv->GetStaticFieldID( BuildClass, "MODEL", "Ljava/lang/String;" );
			jfieldID Hardware = pJniEnv->GetStaticFieldID( BuildClass, "HARDWARE", "Ljava/lang/String;" );
			jfieldID Board = pJniEnv->GetStaticFieldID( BuildClass, "BOARD", "Ljava/lang/String;" );
			jfieldID Serial = pJniEnv->GetStaticFieldID( BuildClass, "SERIAL", "Ljava/lang/String;" );
			jstring jstrDeviceBrand = (jstring)pJniEnv->GetStaticObjectField( BuildClass, Brand );
			jstring jstrDeviceMode = (jstring)pJniEnv->GetStaticObjectField( BuildClass, Model );
			const char* szDeviceBrand = jstrDeviceBrand ? pJniEnv->GetStringUTFChars( jstrDeviceBrand, NULL ) : NULL;
			const char* szDeviceMode = jstrDeviceMode ? pJniEnv->GetStringUTFChars( jstrDeviceMode, NULL ) : NULL;
			gammasstream( m_HardwareDesc.m_szDeviceDesc, sizeof(m_HardwareDesc.m_szDeviceDesc) )
				<< ( szDeviceBrand ? szDeviceBrand : "" ) << " " << ( szDeviceMode ? szDeviceMode : "" );

			jclass VersionClass = pJniEnv->FindClass( "android/os/Build$VERSION" );
			jfieldID Release = pJniEnv->GetStaticFieldID( VersionClass, "RELEASE", "Ljava/lang/String;" );
			jfieldID SDKInit = pJniEnv->GetStaticFieldID( VersionClass, "SDK_INT", "I" );
			jstring jstrOSVersion = (jstring)pJniEnv->GetStaticObjectField( VersionClass, Release );
			m_nSDKInit = pJniEnv->GetStaticIntField( VersionClass, SDKInit );
			const char* szOSVersion = jstrOSVersion ? pJniEnv->GetStringUTFChars( jstrOSVersion, NULL ) : NULL;
			gammasstream( m_HardwareDesc.m_szOSDesc, sizeof( m_HardwareDesc.m_szOSDesc ) )
				<< "Android " << ( szOSVersion ? szOSVersion : "" );

			jclass ClassNativeActivity = pJniEnv->GetObjectClass( m_pActivity->clazz );
			jmethodID MethodGetSystemService = pJniEnv->GetMethodID( ClassNativeActivity,
				"getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;" );
			jclass WifiManagerClass = pJniEnv->FindClass( "android/net/wifi/WifiManager" );
			jmethodID MethodGetConnectionInfo = pJniEnv->GetMethodID( WifiManagerClass,
				"getConnectionInfo", "()Landroid/net/wifi/WifiInfo;" );
			jclass WifiInfoClass = pJniEnv->FindClass( "android/net/wifi/WifiInfo" );
			jmethodID MethodGetMacAddress = pJniEnv->GetMethodID( WifiInfoClass,
				"getMacAddress", "()Ljava/lang/String;" );
			jmethodID MethodGetContentResolver = pJniEnv->GetMethodID( ClassNativeActivity,
				"getContentResolver", "()Landroid/content/ContentResolver;" );

			jclass SecureClass = pJniEnv->FindClass( "android/provider/Settings$Secure" );
			jmethodID MethodGetString = pJniEnv->GetStaticMethodID( SecureClass,
				"getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;" );
			jfieldID AndroidID = pJniEnv->GetStaticFieldID( SecureClass, "ANDROID_ID", "Ljava/lang/String;" );
			jstring jstrAndroidID = (jstring)pJniEnv->GetStaticObjectField( SecureClass, AndroidID );
			jobject jobjContentResolver = pJniEnv->CallObjectMethod( m_pActivity->clazz, MethodGetContentResolver );
			jstring jstrAndroidIDValue = (jstring)pJniEnv->CallStaticObjectMethod(
				SecureClass, MethodGetString, jobjContentResolver, jstrAndroidID );

			jstring jstrWifi = pJniEnv->NewStringUTF( "wifi" );
			jobject jobjWifiManager = pJniEnv->CallObjectMethod( m_pActivity->clazz, MethodGetSystemService, jstrWifi );
			jobject jobjWifiInfo = pJniEnv->CallObjectMethod( jobjWifiManager, MethodGetConnectionInfo );
			jstring jstrMac = (jstring)pJniEnv->CallObjectMethod( jobjWifiInfo, MethodGetMacAddress );
			const char* szMac = jstrMac ? pJniEnv->GetStringUTFChars( jstrMac, NULL ) : NULL;
			for( uint32_t i = 0, j = 0; szMac && szMac[i]; i++ )
			{
				if( !IsHexNumber( szMac[i] ) )
					continue;
				int32_t nHigh = ValueFromHexNumber( szMac[i++] ) << 4;
				int32_t nLow = ValueFromHexNumber( szMac[i] );
				( (uint8_t*)&m_HardwareDesc.m_nMac )[j++] = nHigh|nLow;
			}

			uint8_t* nMac = (uint8_t*)&m_HardwareDesc.m_nMac;
			sprintf( m_HardwareDesc.m_szUUID, "%02x%02x%02x%02x%02x%02x-",
				nMac[0], nMac[1], nMac[2], nMac[3], nMac[4], nMac[5] );
			jstring jstrDeviceSerial = (jstring)pJniEnv->GetStaticObjectField( BuildClass, Serial );
			const char* szDeviceSerial = jstrDeviceSerial ? pJniEnv->GetStringUTFChars( jstrDeviceSerial, NULL ) : "";
			strcat2array_safe( m_HardwareDesc.m_szUUID, szDeviceSerial );
			strcat2array_safe( m_HardwareDesc.m_szUUID, "-" );
			const char* szAndroidID = jstrAndroidID ? pJniEnv->GetStringUTFChars( jstrAndroidIDValue, NULL ) : "";
			strcat2array_safe( m_HardwareDesc.m_szUUID, szAndroidID );

			jclass WindowManagerClass = pJniEnv->FindClass( "android/view/WindowManager" );
			jmethodID MethodGetDefaultDisplay = pJniEnv->GetMethodID( WindowManagerClass,
				"getDefaultDisplay", "()Landroid/view/Display;" );
			jclass DisplayClass = pJniEnv->FindClass( "android/view/Display" );
			jmethodID MethodGetMetrics = pJniEnv->GetMethodID( DisplayClass,
				"getMetrics", "(Landroid/util/DisplayMetrics;)V" );
			jclass DisplayMetricsClass = pJniEnv->FindClass( "android/util/DisplayMetrics" );
			jmethodID MethodDisplayMetrics = pJniEnv->GetMethodID( DisplayMetricsClass, "<init>", "()V" );
			jfieldID WidthPixels = pJniEnv->GetFieldID( DisplayMetricsClass, "widthPixels", "I" );
			jfieldID HeightPixels = pJniEnv->GetFieldID( DisplayMetricsClass, "heightPixels", "I" );

			jstring jstrWindow = pJniEnv->NewStringUTF( "window" );
			jobject jobjWindowManager = pJniEnv->CallObjectMethod( m_pActivity->clazz, MethodGetSystemService, jstrWindow );
			jobject jobjDisplay = pJniEnv->CallObjectMethod( jobjWindowManager, MethodGetDefaultDisplay );
			jobject jobjMetrics = pJniEnv->NewObject( DisplayMetricsClass, MethodDisplayMetrics );
			pJniEnv->CallVoidMethod( jobjDisplay, MethodGetMetrics, jobjMetrics );
			m_HardwareDesc.m_nScreen_X = (uint16_t)( pJniEnv->GetIntField( jobjMetrics, WidthPixels ) );
			m_HardwareDesc.m_nScreen_Y = (uint16_t)( pJniEnv->GetIntField( jobjMetrics, HeightPixels ) );

			jstring jstrDeviceHardWare = (jstring)pJniEnv->GetStaticObjectField( BuildClass, Hardware );
			jstring jstrDeviceBoard = (jstring)pJniEnv->GetStaticObjectField( BuildClass, Board );
			const char* szDeviceHardware = jstrDeviceHardWare ? pJniEnv->GetStringUTFChars( jstrDeviceHardWare, NULL ) : NULL;
			const char* szDeviceBoard = jstrDeviceBoard ? pJniEnv->GetStringUTFChars( jstrDeviceBoard, NULL ) : NULL;
			gammasstream ssCpuType( m_HardwareDesc.m_szCpuType, sizeof(m_HardwareDesc.m_szCpuType) );
			ssCpuType << ( szDeviceHardware ? szDeviceHardware : "" ) << " " << ( szDeviceBoard ? szDeviceBoard : "" );

			jclass ClassLocale = pJniEnv->FindClass( "java/util/Locale" );
			jmethodID MethodGetDefault = pJniEnv->GetStaticMethodID( ClassLocale,
				"getDefault", "()Ljava/util/Locale;" );
			jmethodID MethodGetLanguage = pJniEnv->GetMethodID( ClassLocale,
				"getLanguage", "()Ljava/lang/String;" );
			jmethodID MethodGetCountry = pJniEnv->GetMethodID( ClassLocale,
				"getCountry", "()Ljava/lang/String;" );
			jobject jobjLocale = pJniEnv->CallStaticObjectMethod( ClassLocale, MethodGetDefault );
			jstring jstrLanguage = (jstring)pJniEnv->CallObjectMethod( jobjLocale, MethodGetLanguage );
			jstring jstrCountry = (jstring)pJniEnv->CallObjectMethod( jobjLocale, MethodGetCountry );
			const char* szLanguage = jstrLanguage ? pJniEnv->GetStringUTFChars( jstrLanguage, NULL ) : "";
			const char* szCountry = jstrCountry ? pJniEnv->GetStringUTFChars( jstrCountry, NULL ) : "";
			gammasstream( m_HardwareDesc.m_szLanguage, sizeof(m_HardwareDesc.m_szLanguage) )
				<< ( szLanguage ? szLanguage : "" ) << "-" << ( szCountry ? szCountry : "" );

			#define RELEASE_STR( a, b ) \
				if( a ){ pJniEnv->ReleaseStringUTFChars( a, b ); pJniEnv->DeleteLocalRef( a ); }

			pJniEnv->DeleteLocalRef( ClassLocale );
			pJniEnv->DeleteLocalRef( jobjLocale );
			RELEASE_STR( jstrLanguage, szLanguage );
			RELEASE_STR( jstrCountry, szCountry );
			pJniEnv->DeleteLocalRef( WindowManagerClass );
			pJniEnv->DeleteLocalRef( DisplayClass );
			pJniEnv->DeleteLocalRef( DisplayMetricsClass );
			pJniEnv->DeleteLocalRef( jstrWindow );
			pJniEnv->DeleteLocalRef( jobjWindowManager );
			pJniEnv->DeleteLocalRef( jobjDisplay );
			pJniEnv->DeleteLocalRef( jobjMetrics );
			pJniEnv->DeleteLocalRef( BuildClass );
			pJniEnv->DeleteLocalRef( SecureClass );
			pJniEnv->DeleteLocalRef( jstrAndroidID );
			pJniEnv->DeleteLocalRef( jobjContentResolver );
			RELEASE_STR( jstrDeviceMode, szDeviceMode );
			RELEASE_STR( jstrDeviceBrand, szDeviceBrand );
			RELEASE_STR( jstrDeviceSerial, szDeviceSerial );
			RELEASE_STR(  jstrAndroidIDValue, szAndroidID );
			RELEASE_STR( jstrDeviceHardWare, szDeviceHardware );
			RELEASE_STR( jstrDeviceBoard, szDeviceBoard );
			RELEASE_STR( jstrOSVersion, szOSVersion );
			pJniEnv->DeleteLocalRef( VersionClass );
			pJniEnv->DeleteLocalRef( jstrWifi );
			pJniEnv->DeleteLocalRef( jobjWifiManager );
			pJniEnv->DeleteLocalRef( jobjWifiInfo );
			RELEASE_STR( jstrMac, szMac );
			pJniEnv->DeleteLocalRef( ClassNativeActivity );
			pJniEnv->DeleteLocalRef( WifiManagerClass );
			pJniEnv->DeleteLocalRef( WifiInfoClass );

			if( NULL != ( fp = fopen( "/proc/cpuinfo", "r" ) ) )
			{
				fgets( szTempBuffer, 2048, fp );
				size_t n = strlen( szTempBuffer );
				while( IsBlank( szTempBuffer[n-1] ) )
					szTempBuffer[--n] = 0;
				const char* szFind = strchr( szTempBuffer, ':' );
				if( szFind )
					ssCpuType << szFind + 1;
				while( !feof( fp ) )
				{
					fgets( szTempBuffer, 2048, fp );
					if( memcmp( szTempBuffer, "processor", strlen( "processor" ) ) )
						continue;
					m_HardwareDesc.m_nCpuCount++;
				}
				fclose( fp );
			}

			if( NULL != ( fp = fopen( "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r" ) ) )
			{
				fgets( szTempBuffer, 2048, fp );
				fclose( fp );
				m_HardwareDesc.m_nCpuFrequery = (uint32_t)( atoll( szTempBuffer )/( 1000 ) );
			}

			if( NULL != ( fp = fopen( "/proc/meminfo", "r" ) ) )
			{
				fgets( szTempBuffer, 2048, fp );
				fclose( fp );
				const char* szStart = szTempBuffer;
				while( *szStart && !IsNumber( *szStart ) )
					szStart++;
				m_HardwareDesc.m_nMemSize = (uint32_t)( atoll( szStart )/( 1024 ) );
			}
		}
		catch(...)
		{
		}
	}

	//==========================================================================================
	// 以下为连接系统的静态函数
	//==========================================================================================
	void CAndroidApp::OnDestroy( ANativeActivity* pActivity )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnDestroy begin" );
		( (CAndroidApp*)pActivity->instance )->Free();
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnDestroy end" );
	}

	void CAndroidApp::OnStart( ANativeActivity* pActivity )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnStart begin" );
		( (CAndroidApp*)pActivity->instance )->SetActivityState( APP_CMD_START );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnStart end" );
	}

	void CAndroidApp::OnResume( ANativeActivity* pActivity )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnResume begin" );
		( (CAndroidApp*)pActivity->instance )->SetActivityState( APP_CMD_RESUME );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnResume end" );
	}

	void* CAndroidApp::OnSaveInstanceState( ANativeActivity* pActivity, size_t* outLen )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnSaveInstanceState begin" );
		return ( (CAndroidApp*)pActivity->instance )->SaveInstanceState( outLen );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnSaveInstanceState end" );
	}

	void CAndroidApp::OnPause( ANativeActivity* pActivity )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnPause begin" );
		( (CAndroidApp*)pActivity->instance )->SetActivityState( APP_CMD_PAUSE );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnPause end" );
	}

	void CAndroidApp::OnStop( ANativeActivity* pActivity )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnStop begin" );
		( (CAndroidApp*)pActivity->instance )->SetActivityState( APP_CMD_STOP );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnStop end" );
	}

	void CAndroidApp::OnConfigurationChanged( ANativeActivity* pActivity )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnConfigurationChanged begin" );
		( (CAndroidApp*)pActivity->instance )->WriteCommand( APP_CMD_CONFIG_CHANGED );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnConfigurationChanged end" );
	}

	void CAndroidApp::OnLowMemory( ANativeActivity* pActivity )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnLowMemory begin" );
		( (CAndroidApp*)pActivity->instance )->WriteCommand( APP_CMD_LOW_MEMORY );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnLowMemory end" );
	}

	void CAndroidApp::OnWindowFocusChanged( ANativeActivity* pActivity, int bFocused )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnWindowFocusChanged begin" );
		( (CAndroidApp*)pActivity->instance )->WriteCommand( bFocused ? APP_CMD_GAINED_FOCUS : APP_CMD_LOST_FOCUS);
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnWindowFocusChanged end" );
	}

	void CAndroidApp::OnNativeWindowCreated( ANativeActivity* pActivity, ANativeWindow* pWindow )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnNativeWindowCreated begin" );
		( (CAndroidApp*)pActivity->instance )->SetWindow( pWindow );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnNativeWindowCreated end" );
	}

	void CAndroidApp::OnNativeWindowDestroyed( ANativeActivity* pActivity, ANativeWindow* pWindow )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnNativeWindowDestroyed begin" );
		( (CAndroidApp*)pActivity->instance )->SetWindow( NULL );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnNativeWindowDestroyed end" );
	}

	void CAndroidApp::OnNativeWindowResized( ANativeActivity* pActivity, ANativeWindow* pWindow )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnNativeWindowResized begin" );
		( (CAndroidApp*)pActivity->instance )->WriteCommand( APP_CMD_WINDOW_RESIZED );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnNativeWindowResized end" );
	}

	void CAndroidApp::OnInputQueueCreated( ANativeActivity* pActivity, AInputQueue* pQueue )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnInputQueueCreated begin" );
		( (CAndroidApp*)pActivity->instance )->SetInput( pQueue );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnInputQueueCreated end" );
	}

	void CAndroidApp::OnInputQueueDestroyed( ANativeActivity* pActivity, AInputQueue* pQueue )
	{
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnInputQueueDestroyed begin" );
		( (CAndroidApp*)pActivity->instance )->SetInput( NULL );
		__android_log_print( ANDROID_LOG_INFO, "NDK_LOG", "OnInputQueueDestroyed end" );
	}

}

extern "C"
{
	JNIEXPORT void JNICALL Java_com_joyegame_gammacommon_GammaActivity_OnInputText
		( JNIEnv* env, jobject obj, jcharArray text, jint nSize )
	{
		jchar* szBuffer = env->GetCharArrayElements( text, NULL );
		Gamma::CAndroidApp::GetInstance().AddCharMsgFromJava( szBuffer, nSize );
		env->ReleaseCharArrayElements( text, szBuffer, 0 );
	}

	JNIEXPORT void JNICALL Java_com_joyegame_gammacommon_GammaActivity_OnInputMgrShow
		( JNIEnv* env, jobject obj, bool bShow )
	{
		Gamma::CAndroidApp::GetInstance().OnInputMgrShowFromJava( bShow );
	};

	JNIEXPORT void JNICALL Java_com_joyegame_gammacommon_GammaActivity_OnActivityResult
		( JNIEnv* env, jobject obj, jint nID, jint nResult, jobject Intent )
	{
		Gamma::CAndroidApp::GetInstance().OnActivityResult( env, nID, nResult, Intent );
	};
}

void ANativeActivity_onCreate( ANativeActivity* pActivity, void* savedState, size_t savedStateSize ) 
{
	GammaLog << "ANativeActivity_onCreate begin" << endl;
	Gamma::CAndroidApp::GetInstance().Run( pActivity, savedState, savedStateSize );
	GammaLog << "ANativeActivity_onCreate end" << endl;
}

#endif

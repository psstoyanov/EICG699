/*

Book:      	Game and Graphics Programming for iOS and Android with OpenGL(R) ES 2.0
Author:    	Romain Marucchi-Foino
ISBN-10: 	1119975913
ISBN-13: 	978-1119975915
Publisher: 	John Wiley & Sons	

Copyright (C) 2011 Romain Marucchi-Foino

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of
this software. Permission is granted to anyone who either own or purchase a copy of
the book specified above, to use this software for any purpose, including commercial
applications subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that
you wrote the original software. If you use this software in a product, an acknowledgment
in the product would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be misrepresented
as being the original software.

3. This notice may not be removed or altered from any source distribution.

*/

#ifndef TEMPLATEAPP_H
#define TEMPLATEAPP_H

#ifdef __IPHONE_4_0

	#include "gfx.h"
#else

	#include "../../../common/gfx.h"
#endif



typedef struct
{
void ( *Init			)( int width, int height );
void ( *Draw			)( void );
void ( *ToucheBegan	    )( float x, float y, unsigned int tap_count );
void ( *ToucheMoved	    )( float x, float y, unsigned int tap_count );
void ( *ToucheMoved2    )( float x, float y, unsigned int tap_count, unsigned int id );
void ( *ToucheEnded	    )( float x, float y, unsigned int tap_count );
void ( *Accelerometer   )( float x, float y, float z );

} TEMPLATEAPP;

extern TEMPLATEAPP templateApp;

void templateAppInit( int width, int height );

void templateAppDraw( void );

//The following code is used for gluing the Dalvik-Java and C/C++
//commands commands together.

void templateAppToucheBegan( float x, float y, unsigned int tap_count );



void templateAppToucheMoved( float x, float y, unsigned int tap_count );

void templateAppToucheMoved2( float x, float y, unsigned int tap_count, unsigned int id );

void templateAppToucheEnded( float x, float y, unsigned int tap_count );



void templateAppToucheCancelled( float x, float y, unsigned int tap_count );

void templateAppAccelerometer( float x, float y, float z );

void templateAppExit( void );


//In case the application is made for iOS devices.
	#ifndef __IPHONE_4_0

        //The difference in compilers requires extern "C" code
        //for compatibility.
		extern "C"
		{
		//The actual gluing code
		//Every command is exported from Java to C/C++
		//This includes the initialization, drawing commands and so forth.
		//Commands for input (multitouch/controller support) is added here.
			JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_Init( JNIEnv *env, jobject obj, jint width, jint height, jstring apkFilePath );

			JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_Draw( JNIEnv *env, jobject obj );

			JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_ToucheBegan( JNIEnv *env, jobject obj, jfloat x, jfloat y, jint tap_count );

			JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_ToucheMoved( JNIEnv *env, jobject obj, jfloat x, jfloat y, jint tap_count );

			JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_ToucheMoved2( JNIEnv *env, jobject obj, jfloat x, jfloat y, jint tap_count, jint id );

			JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_ToucheEnded( JNIEnv *env, jobject obj, jfloat x, jfloat y, jint tap_count );

			JNIEXPORT void JNICALL Java_com_android_templateApp_templateApp_Accelerometer( JNIEnv *env, jobject obj, jfloat x, jfloat y, jfloat z );
		};


		//Since the check for iOS devices returns negative
		//the following commands will be executed instead.
		//Hence the same gluing function has to be glued.
		//Gluing the initialization function.
		JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_Init( JNIEnv *env, jobject obj, jint width, jint height, jstring apkFilePath )
		{
			setenv( "FILESYSTEM", env->GetStringUTFChars( apkFilePath, NULL ), 1 );

			if( templateApp.Init ) templateApp.Init( width, height );
		}


		JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_Draw( JNIEnv *env, jobject obj )
		{ if( templateApp.Draw ) templateApp.Draw(); }

		JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_ToucheBegan( JNIEnv *env, jobject obj, jfloat x, jfloat y, jint tap_count )
		{ if( templateApp.ToucheBegan ) templateApp.ToucheBegan( x, y, tap_count ); }

		JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_ToucheMoved( JNIEnv *env, jobject obj, jfloat x, jfloat y, jint tap_count )
		{ if( templateApp.ToucheMoved ) templateApp.ToucheMoved( x, y, tap_count ); }

		JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_ToucheMoved2( JNIEnv *env, jobject obj, jfloat x, jfloat y, jint tap_count, jint id )
				{ if( templateApp.ToucheMoved2 ) templateApp.ToucheMoved2( x, y, tap_count, id ); }

		JNIEXPORT void JNICALL Java_com_android_templateApp_GL2View_ToucheEnded( JNIEnv *env, jobject obj, jfloat x, jfloat y, jint tap_count )
		{ if( templateApp.ToucheEnded ) templateApp.ToucheEnded( x, y, tap_count ); }

		JNIEXPORT void JNICALL Java_com_android_templateApp_templateApp_Accelerometer( JNIEnv *env, jobject obj, jfloat x, jfloat y, jfloat z )
		{ if( templateApp.Accelerometer ) templateApp.Accelerometer( x, y, z ); }

	#endif

#endif

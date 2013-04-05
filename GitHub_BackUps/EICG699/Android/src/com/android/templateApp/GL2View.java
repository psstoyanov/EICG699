package com.android.templateApp;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.support.v4.view.MotionEventCompat;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import com.android.chapter7_6.R;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

class GL2View extends GLSurfaceView implements SurfaceHolder.Callback
{
	public static final String DEBUG_TAG = "MyLoggingTag";
	public Renderer r;
	
	public GL2View( Context context )
	{
        super( context );

        setEGLContextFactory( new ContextFactory() );

        				  // new ConfigChooser( 8, 8, 8, 8, 1, 1 )
        setEGLConfigChooser( new ConfigChooser( 5, 6, 5, 0, 1, 1 ) );

        r = new Renderer();
        
        setRenderer( r );
        
        r.apkFilePath = context.getPackageResourcePath();
	}

    private static class ContextFactory implements GLSurfaceView.EGLContextFactory
    {
    	public EGLContext createContext( EGL10 egl, EGLDisplay display, EGLConfig eglConfig )
    	{
    		int[] attrib_list = { 0x3098,
    							  2,
    							  EGL10.EGL_NONE };
            
    		EGLContext context = egl.eglCreateContext( display,
    												   eglConfig,
    												   EGL10.EGL_NO_CONTEXT,
    												   attrib_list );
    		return context;
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context)
        { egl.eglDestroyContext( display, context ); }
    }
       
    //Declare native functions for multitouch
    
    public static native void ToucheBegan( float x, float y, int tap_count );
    public static native void ToucheBegan2( float x, float y, int tap_count, int id );

    public static native void ToucheMoved( float x, float y, int tap_count );
    public static native void ToucheMoved2( float x, float y, int tap_count , int id);
    
    
    public static native void ToucheEnded(  int tap_count );
    public static native void ToucheEnded2( int tap_count, int id );
    
    
    
    private long last_tap = 0;
    
    private int tap_count = 1;
    
    // Handling multitouch events
    // Passing the ID and action from Android
    // to be translated into C/C++ code
    // through the glue code in "templateApp.h
    
    
    // The maximum number of supported multitouch points
    // Most current devices support up-to 10 simultaneous points
    // Nexus 7 supports up-to 10
    final int MAX_NUMBER_OF_POINT = 20;
    
    // The coordinates of each touch pointer
	float[] x = new float[MAX_NUMBER_OF_POINT];
	float[] y = new float[MAX_NUMBER_OF_POINT];
	
	// Two boolean variables for each touch
	// Upon releasing a touch (one fingers is no longer
	// touching the screen), all touch points will
	// be registered as released for a short time.
	// The second boolean is used to distinguish the false
	// positives.
	boolean[] touching = new boolean[MAX_NUMBER_OF_POINT];
	boolean[] false_positive = new boolean[MAX_NUMBER_OF_POINT];
	

	public boolean onTouchEvent(MotionEvent event) 
	{
		
		int action = (event.getAction() & MotionEvent.ACTION_MASK);
		// Get the number of touches
		int pointCount = event.getPointerCount();
		// Loop through all touch events
		for (int i = 0; i < pointCount; i++) 
		{
			// Get the ID number of the touch event.
			// It is unique and does not change upon release.
			int id = event.getPointerId(i);
			 
			//Ignore pointer higher than our max.
			if(id < MAX_NUMBER_OF_POINT)
			{
				// Set the coordinates for each touch pointer
				x[id] = (int)event.getX(i);
				y[id] = (int)event.getY(i);
				
				// If the event is one of the following:
				// A finger is put down, more than one finger is down,
				// a finger has moved;
				// apply the appropriate action.
				if((action == MotionEvent.ACTION_DOWN)
						 ||(action == MotionEvent.ACTION_POINTER_DOWN)
						 ||(action == MotionEvent.ACTION_MOVE))
				{
					// Only if a pointer ID has both flags negative,
					// the pointer will be counted as a new one.
					// If the pointer has an active flag, but the false positive
					// flag is false, the pointer is not new.
					if(touching[id]!=true&&false_positive[id]!=true)
					{
						// Send the touch begin event (coordinates and id) to
						// the C/C++ engine
						ToucheBegan2( x[id], y[id], tap_count, id );
					}
					else
					{
						// Send the touch motion event (coordinates and id) to
						// the C/C++ engine
						ToucheMoved2( x[id], y[id], tap_count ,  id);
					}
					
					// Set both flags as true.
					touching[id] = true;
					false_positive[id]=true;
					
				}
				// In every other case (a touch has been canceled, finger has been
				// lifted, etc.)
				else
				{
					// Set the false_positive ID as true.
					false_positive[id]=false;
					// In case it is a single touch, there will be no false
					// positives for releasing a finger. Set both flags as false;
					if(pointCount==1)
					{
						// Since there are no false positives, send the release command
						// to the C/C++ engine.
						touching[id]=false;
						ToucheEnded2( tap_count, id);
					}
				}
			 } 
		}
		
		
		// A check for false positives.
		// Loop through the maximum supported touches.
		for( int i=0; i < MAX_NUMBER_OF_POINT; i++)
		{
			// In case the pointer does not exist and the false positive
			// flag is false, set the pointer as inactive.
			// This way only the released fingers will be set as such.
			if(event.findPointerIndex(i)==-1&&false_positive[i]!=true)
			{
				// Send the release command to the C/C++ engine for the
				// pointer ID.
				touching[i]=false;
				ToucheEnded2( tap_count, i);
			}
		}
		
		
		
		invalidate();	
		return true;
		
	}
	
	// The original touchEvent handler.
    /*
    public boolean onTouchEvent( final MotionEvent event )
    {
        switch( event.getAction() )
        {
	        case MotionEvent.ACTION_DOWN:
	        {
	        	if( event.getEventTime() - last_tap < 333 ) tap_count = tap_count + 1;
	        	else tap_count = 1;

	        	last_tap = event.getEventTime();

	        	ToucheBegan( event.getX(0), event.getY(0), tap_count );
	        	break;
	        }
	        
	        case MotionEvent.ACTION_MOVE:
	        {
	        	ToucheMoved( event.getX(0), event.getY(0), tap_count );
	        	break;
	        }
	        
	        case MotionEvent.ACTION_UP:
	        {
	        	ToucheEnded( event.getX(0), event.getY(0), tap_count );
	        	break;
	        }
        }

        return true;
    } */
   
    private static class ConfigChooser implements GLSurfaceView.EGLConfigChooser {

        protected int mRedSize,
        			  mGreenSize,
        			  mBlueSize,
        			  mAlphaSize,
        			  mDepthSize,
        			  mStencilSize;
        
        public ConfigChooser( int r, int g, int b, int a, int depth, int stencil )
        {
            mRedSize     = r;
            mGreenSize   = g;
            mBlueSize    = b;
            mAlphaSize   = a;
            mDepthSize   = depth;
            mStencilSize = stencil;
        }

        
        private static int[] s_configAttribs =
        {
            EGL10.EGL_RED_SIZE		 , 5,
            EGL10.EGL_GREEN_SIZE	 , 6,
            EGL10.EGL_BLUE_SIZE		 , 5,
            EGL10.EGL_ALPHA_SIZE	 , 0,
            EGL10.EGL_RENDERABLE_TYPE, 4,
            EGL10.EGL_NONE
        };

        
        public EGLConfig chooseConfig( EGL10 egl, EGLDisplay display )
        {
            int[] num_config = new int[ 1 ];

            egl.eglChooseConfig( display,
            					 s_configAttribs,
            					 null,
            					 0,
            					 num_config );

            int numConfigs = num_config[ 0 ];

            EGLConfig[] configs = new EGLConfig[ numConfigs ];
            
            egl.eglChooseConfig( display,
            					 s_configAttribs,
            					 configs,
            					 numConfigs,
            					 num_config );
       
            return chooseConfig( egl, display, configs );
        }
        
        
        public EGLConfig chooseConfig( EGL10 egl, EGLDisplay display, EGLConfig[] configs )
        {
            for( EGLConfig config : configs )
            {
                int depth   = findConfigAttrib( egl, display, config, EGL10.EGL_DEPTH_SIZE  , 0 ),
                    stencil = findConfigAttrib( egl, display, config, EGL10.EGL_STENCIL_SIZE, 0 );

                if( depth < mDepthSize || stencil < mStencilSize ) continue;

                int r = findConfigAttrib( egl, display, config, EGL10.EGL_RED_SIZE  , 0 ),
                   	g =	findConfigAttrib( egl, display, config, EGL10.EGL_GREEN_SIZE, 0 ),
                   	b =	findConfigAttrib( egl, display, config, EGL10.EGL_BLUE_SIZE , 0 ),
                    a =	findConfigAttrib( egl, display, config, EGL10.EGL_ALPHA_SIZE, 0 );

                if( r == mRedSize   && 
                	g == mGreenSize && 
                	b == mBlueSize  && 
                	a == mAlphaSize ) return config;
            }
            
            return null;
        }
        
        
        private int findConfigAttrib( EGL10 egl, EGLDisplay display, EGLConfig config, int attribute, int defaultValue )
        {
        	int[] mValue = new int[ 1 ];
        	
            if( egl.eglGetConfigAttrib( display,
            							config,
            							attribute,
            							mValue ) ) return mValue[ 0 ];
            return defaultValue;
        }
     } 
    
    public static native void Init( int w, int h, String apkFilePath );
     
    public static native void Draw();
     
    public static class Renderer implements GLSurfaceView.Renderer
    {
    	public String apkFilePath;
    	public int	  width;
    	public int	  height;
    	
    	public void onDrawFrame( GL10 gl ){ Draw(); }

    	private char init_once = 0;
    	
        public void onSurfaceChanged( GL10 gl, int width, int height )
        {
        	if( init_once == 0 )
        	{
        		this.width  = width;
        		this.height = height;
        		
         		Init( width, height, apkFilePath );
	        	init_once = 1;
        	}
        }
       
        public void onSurfaceCreated( GL10 gl, EGLConfig config ) { }
    }
}

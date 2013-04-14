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

#include "templateApp.h"

#define OBJ_FILE ( char * )"Scene.obj"

#define PHYSIC_FILE ( char * )"Scene.bullet"


bool openBrowser=false;

// Maximum number ot supported touches
const int MAX_NUMBER_OF_POINT=2;

/*
 * Each touch pointer is assigned a touch type
 * Type "0" - inactive, Type "1" - movement, Type "2" - view, Type "3" - extra
 */
int touch_type[2]={0};
/*
 * Is the pointer active
 */
bool touching[2]={false};

/*
 * Check for pointer with a movement flag
 */
bool movement_touch()
{
	for (int i=0;i<MAX_NUMBER_OF_POINT;i++)
			{
			if(touch_type[i]==1)
			return true;
			}
	return false;
}
/*
 * Check for pointer with a view flag
 */
bool view_touch()
{
	for (int i=0;i<MAX_NUMBER_OF_POINT;i++)
			{
			if(touch_type[i]==2)
			return true;
			}
	return false;
}

/*
 *
 */

OBJ *obj = NULL;
/*
 * added for multitouch camera controls
 */
float screen_size;

vec2 view_location,
 	 view_delta = { 0.0f, 0.0f };

vec3 move_location = { 0.0f, 0.0f, 0.0f },
	 move_delta;

vec3 eye,
     up = { 0.0f, 0.0f, 1.0f };

vec3 next_eye;

float roty		= -165.0f,
      next_roty = roty,
      rotz		= 0.0f,
      next_rotz	= rotz,
      distance	= -5.0f;


OBJMESH *player = NULL;

THREAD *thread = NULL;

vec4 frustum[ 6 ];

int viewport_matrix[ 4 ];

FONT *font_small = NULL,
	 *font_big   = NULL;

vec2 accelerometer = { 0.0f, 0.0f },
     next_accelerometer = { 0.0f, 0.0f };


unsigned char game_state = 0;


float game_time  = 0.0f,
      gem_factor = 20.0f;

unsigned int gem_points = 0;

	  
SOUNDBUFFER *background_soundbuffer = NULL;
SOUND *background_sound = NULL;

SOUNDBUFFER *gems_soundbuffer[ 4 ];
SOUND *gems_sound[ 4 ];

SOUNDBUFFER *water_soundbuffer = NULL;
SOUND *water_sound = NULL;

SOUNDBUFFER *lava_soundbuffer = NULL;
SOUND *lava_sound = NULL;

SOUNDBUFFER *toxic_soundbuffer = NULL;
SOUND *toxic_sound = NULL;


TEMPLATEAPP templateApp = { templateAppInit,
							templateAppDraw,
							templateAppToucheBegan,
							templateAppToucheMoved,
							templateAppToucheEnded,
							Pause,
							templateAppAccelerometer,
							templateAppExit,
							templateAppCallTheBrowser};


btSoftBodyRigidBodyCollisionConfiguration *collisionconfiguration = NULL;

btCollisionDispatcher *dispatcher = NULL;

btBroadphaseInterface *broadphase = NULL;

btConstraintSolver *solver = NULL;

btSoftRigidDynamicsWorld *dynamicsworld = NULL;


void init_physic_world( void )
{
	collisionconfiguration = new btSoftBodyRigidBodyCollisionConfiguration();

	dispatcher = new btCollisionDispatcher( collisionconfiguration );

	broadphase = new btDbvtBroadphase();

	solver = new btSequentialImpulseConstraintSolver();

	dynamicsworld = new btSoftRigidDynamicsWorld( dispatcher,	
												  broadphase,
												  solver,
												  collisionconfiguration );

	dynamicsworld->setGravity( btVector3( 0.0f, 0.0f, -9.8f ) );
}


void load_physic_world( void )
{
	btBulletWorldImporter *btbulletworldimporter = new btBulletWorldImporter( dynamicsworld );

	MEMORY *memory = mopen( PHYSIC_FILE, 1 );

	btbulletworldimporter->loadFileFromMemory( ( char * )memory->buffer, memory->size );

	mclose( memory );

	unsigned int i = 0;

	while( i != btbulletworldimporter->getNumRigidBodies() ) { 

		OBJMESH *objmesh = OBJ_get_mesh( obj,
										 btbulletworldimporter->getNameForPointer(
										 btbulletworldimporter->getRigidBodyByIndex( i ) ), 0 );
										 
		if( objmesh ) { 

			objmesh->btrigidbody = ( btRigidBody * )btbulletworldimporter->getRigidBodyByIndex( i );
			
			objmesh->btrigidbody->setUserPointer( objmesh );
		} 

		++i; 
	} 

	delete btbulletworldimporter;
}


void free_physic_world( void )
{
	while( dynamicsworld->getNumCollisionObjects() )
	{
		btCollisionObject *btcollisionobject = dynamicsworld->getCollisionObjectArray()[ 0 ];
		
		btRigidBody *btrigidbody = btRigidBody::upcast( btcollisionobject );

		if( btrigidbody )
		{
			delete btrigidbody->getCollisionShape();
			
			delete btrigidbody->getMotionState();
			
			dynamicsworld->removeRigidBody( btrigidbody );
			
			dynamicsworld->removeCollisionObject( btcollisionobject );
			
			delete btrigidbody;
		}
	}
	
	delete collisionconfiguration; collisionconfiguration = NULL;

	delete dispatcher; dispatcher = NULL;

	delete broadphase; broadphase = NULL;

	delete solver; solver = NULL;
	
	delete dynamicsworld; dynamicsworld = NULL;	
}


void program_bind_attrib_location( void *ptr ) {

	PROGRAM *program = ( PROGRAM * )ptr;

	glBindAttribLocation( program->pid, 0, "POSITION"  );
	glBindAttribLocation( program->pid, 2, "TEXCOORD0" );
}


void program_draw( void *ptr )
{
	PROGRAM *program = ( PROGRAM * )ptr;
	
	unsigned int i = 0;
		
	while( i != program->uniform_count )
	{
		if( !program->uniform_array[ i ].constant &&
			!strcmp( program->uniform_array[ i ].name, "DIFFUSE" ) )
		{
			glUniform1i( program->uniform_array[ i ].location, 1 );
		
			program->uniform_array[ i ].constant = 1;
		}

		else if( !strcmp( program->uniform_array[ i ].name, "MODELVIEWPROJECTIONMATRIX" ) )
		{
			glUniformMatrix4fv( program->uniform_array[ i ].location,
								1,
								GL_FALSE,
								( float * )GFX_get_modelview_projection_matrix() );		
		}
		
		else if( !strcmp( program->uniform_array[ i ].name, "TEXTUREMATRIX" ) ) { 

		   static vec2 scroll = { 0.0f, 0.0f };

		   GFX_set_matrix_mode( TEXTURE_MATRIX );

		   GFX_push_matrix();

		   scroll.x += 0.0025f;
		   scroll.y += 0.0025f;

		   GFX_translate( scroll.x, scroll.y, 0.0f );

		   glUniformMatrix4fv( program->uniform_array[ i ].location,
							   1,
							   GL_FALSE,
							   ( float * )GFX_get_texture_matrix() );

		   GFX_pop_matrix();

		   GFX_set_matrix_mode( MODELVIEW_MATRIX ); 
		}		

		++i;
	}
}


class ClosestNotMeRayResultCallback:public btCollisionWorld::ClosestRayResultCallback { 

	public:
		ClosestNotMeRayResultCallback( btRigidBody *rb,
									   const btVector3 &p1,
									   const btVector3 &p2 ) :
		btCollisionWorld::ClosestRayResultCallback( p1, p2 )
		{ m_btRigidBody = rb; }

	virtual btScalar addSingleResult( btCollisionWorld::LocalRayResult &localray,
									  bool normalinworldspace )
	{ 
		if( localray.m_collisionObject == m_btRigidBody )
		{ return 1.0f; }
		
		return ClosestRayResultCallback::addSingleResult( localray, normalinworldspace );
	}

	protected:
		btRigidBody *m_btRigidBody;
};


bool contact_added_callback( btManifoldPoint &btmanifoldpoint,
							 const btCollisionObject *btcollisionobject0,
							 int part_0, int index_0,
							 const btCollisionObject *btcollisionobject1,
							 int part_1, int index_1 ) {

	OBJMESH *objmesh0 = ( OBJMESH * )( ( btRigidBody * )btcollisionobject0 )->getUserPointer();

	OBJMESH *objmesh1 = ( OBJMESH * )( ( btRigidBody * )btcollisionobject1 )->getUserPointer();
	
	
	if( ( strstr( objmesh0->name, "level_clear" ) || 
		  strstr( objmesh1->name, "level_clear" ) ) )

		game_state = 1;


	else if( ( strstr( objmesh0->name, "player" ) || 
			   strstr( objmesh1->name, "player" ) )
			&&
			 ( strstr( objmesh0->name, "gem" ) || 
			   strstr( objmesh1->name, "gem" ) ) ) { 

		OBJMESH *objmesh = NULL;

		btCollisionObject *btcollisionobject = NULL;

		if( strstr( objmesh0->name, "gem" ) ) {

			objmesh = objmesh0;
			btcollisionobject = ( btCollisionObject * )btcollisionobject0;
		}
		else { 
		
			objmesh = objmesh1;
			btcollisionobject = ( btCollisionObject * )btcollisionobject1;
		}

		unsigned char index = 0;

		if( strstr( objmesh->name, "red" ) ) { 
			
			gem_points += 1;
			index = 0;
		}
		else if( strstr( objmesh->name, "green" ) ) { 

			gem_points += 2;
			index = 1;
		}
		
		else if( strstr( objmesh->name, "blue" ) ) { 
			
			gem_points += 3;
			index = 2;
		}
		
		else if( strstr( objmesh->name, "yellow" ) ) { 
		
			gem_points += 4;
			index = 3;
		}

		SOUND_set_location( gems_sound[ index ],
							&objmesh->location,
							objmesh->radius * gem_factor );


		SOUND_play( gems_sound[ index ], 0 );

		objmesh->visible = 0;

		delete objmesh->btrigidbody->getCollisionShape();

		delete objmesh->btrigidbody->getMotionState();

		dynamicsworld->removeRigidBody( objmesh->btrigidbody );

		dynamicsworld->removeCollisionObject( btcollisionobject );

		delete objmesh->btrigidbody;

		objmesh->btrigidbody = NULL;
	}		 

	return false;
}


void decompress_stream( void *ptr )
{
	SOUND_update_queue( background_sound );	
}


void load_level( void )
{
	obj = OBJ_load( OBJ_FILE, 1 );

	unsigned int i = 0;

	while( i != obj->n_objmesh ) {
	
		OBJ_optimize_mesh( obj, i, 128 );

		OBJ_build_mesh( obj, i );
		
		OBJ_free_mesh_vertex_data( obj, i );

		++i;
	}
	
	
	init_physic_world();
	
	load_physic_world();
	
	gContactAddedCallback = contact_added_callback;
	
	
   OBJMESH *level_clear = OBJ_get_mesh( obj, "level_clear", 0 );

   level_clear->btrigidbody->setCollisionFlags( level_clear->btrigidbody->getCollisionFlags()  |
												btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK |
												btCollisionObject::CF_NO_CONTACT_RESPONSE );
   level_clear->visible = 0;

   i = 0;
   while( i != obj->n_objmesh ) { 

      OBJMESH *objmesh = &obj->objmesh[ i ];

      if( strstr( objmesh->name, "gem" ) ) { 

         objmesh->rotation.z = ( float )( random() % 360 );

         objmesh->btrigidbody->setCollisionFlags( objmesh->btrigidbody->getCollisionFlags() |
												  btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK );
      }
	  
	  ++i;
   }	
	
	
	player = OBJ_get_mesh( obj, "player", 0 );
	
	player->btrigidbody->setFriction( 10.0f );
	
	memcpy( &eye, &player->location, sizeof( vec3 ) );
	
	
	i = 0;
	while( i != obj->n_texture ) { 

		OBJ_build_texture( obj,
						   i,
						   obj->texture_path,
						   TEXTURE_MIPMAP ,
						   TEXTURE_FILTER_2X,
						   0.0f );
		++i;
	}


	i = 0;
	while( i != obj->n_program ) { 

		OBJ_build_program( obj,
						   i,
						   program_bind_attrib_location,
						   program_draw,
						   1,
						   obj->program_path );
		++i;
	}


	i = 0;
	while( i != obj->n_objmaterial ) { 

		OBJ_build_material( obj, i, NULL );
		
		++i;
	}	

	

	font_small = FONT_init( ( char * )"foo.ttf" );

	FONT_load( font_small,
			   font_small->name,
			   1,
			   24.0f,
			   512,
			   512,
			   32,
			   96 );
			   

	font_big = FONT_init( ( char * )"foo.ttf" );

	FONT_load( font_big,
			   font_big->name,
			   1,
			   48.0f,
			   512,
			   512,
			   32,
			   96 );
			   

	MEMORY *memory = NULL;

	i = 0;
	while( i != 4 ) {
	
		switch( i ) { 
		
			case 0: {

				memory = mopen( ( char * )"red.ogg", 1 );
				break;
			}
			case 1: {

				memory = mopen( ( char * )"green.ogg", 1 );
				break;
			}
			case 2: {

				memory = mopen( ( char * )"blue.ogg", 1 );
				break;
			}
			case 3: {

				memory = mopen( ( char * )"yellow.ogg", 1 );
				break;
			}
		}

		gems_soundbuffer[ i ] = SOUNDBUFFER_load( ( char * )"gem", memory );

		mclose( memory );
		
		gems_sound[ i ] =  SOUND_add( ( char * )"gem", gems_soundbuffer[ i ] );

		SOUND_set_volume( gems_sound[ i ], 1.0f );

		++i;
	}
	

	OBJMESH *objmesh = NULL;
	
	memory = mopen( ( char * )"water.ogg", 1 );

	water_soundbuffer = 
	SOUNDBUFFER_load( ( char * )"water", memory );

	mclose( memory );

	water_sound = SOUND_add( ( char * )"water", water_soundbuffer );

	objmesh = OBJ_get_mesh( obj, "water", 0 );

	SOUND_set_location( water_sound,
	&objmesh->location, 
	objmesh->radius );

	SOUND_set_volume( water_sound, 0.5f );

	SOUND_play( water_sound, 1 );


	memory = mopen( ( char * )"lava.ogg", 1 );
	
	lava_soundbuffer = SOUNDBUFFER_load( ( char * )"lava", memory );
	
	mclose( memory );
	
	lava_sound = SOUND_add( ( char * )"lava", lava_soundbuffer );

	objmesh = OBJ_get_mesh( obj, "lava", 0 );

	SOUND_set_location( lava_sound,
						&objmesh->location, 
						objmesh->radius );
						
	SOUND_set_volume( lava_sound, 0.5f );

	SOUND_play( lava_sound, 1 );


	memory = mopen( ( char * )"toxic.ogg", 1 );
	
	toxic_soundbuffer = SOUNDBUFFER_load( ( char * )"toxic", memory );

	mclose( memory );

	toxic_sound = SOUND_add( ( char * )"toxic", toxic_soundbuffer );
	
	objmesh = OBJ_get_mesh( obj, "toxic", 0 );

	SOUND_set_location( toxic_sound,
						&objmesh->location, 
						objmesh->radius );
	
	SOUND_set_volume( toxic_sound, 0.5f );
	
	SOUND_play( toxic_sound, 1 );


	memory = mopen( ( char * )"background.ogg", 1 );

	background_soundbuffer = SOUNDBUFFER_load_stream( ( char * )"background", memory );

	background_sound = SOUND_add( ( char * )"background", background_soundbuffer );
	
	SOUND_set_volume( background_sound, 0.5f );

	SOUND_play( background_sound, 1 );

	THREAD_play( thread );
}


void free_level( void )
{
	gem_points =
	game_state = 0;
	game_time  = 0.0f;


	unsigned int i = 0;

	THREAD_pause( thread );   

	background_sound = SOUND_free( background_sound );

	background_soundbuffer->memory = mclose( background_soundbuffer->memory );

	background_soundbuffer = SOUNDBUFFER_free( background_soundbuffer );

	while( i != 4 ) {
	 
		gems_sound[ i ] = SOUND_free( gems_sound[ i ] );
		gems_soundbuffer[ i ] = SOUNDBUFFER_free( gems_soundbuffer[ i ] );
	
		++i;
	}

	water_sound = SOUND_free( water_sound );
	water_soundbuffer = SOUNDBUFFER_free( water_soundbuffer );

	lava_sound = SOUND_free( lava_sound );
	lava_soundbuffer = SOUNDBUFFER_free( lava_soundbuffer );

	toxic_sound = SOUND_free( toxic_sound );
	toxic_soundbuffer = SOUNDBUFFER_free( toxic_soundbuffer );
   
	player = NULL;

	THREAD_pause( thread );	

	font_small = FONT_free( font_small );
	
	font_big = FONT_free( font_big );
	
	free_physic_world();

	obj = OBJ_free( obj );
}


void templateAppInit( int width, int height ) {

	screen_size = width;



	atexit( templateAppExit );

	GFX_start();
	
	AUDIO_start();

	thread = THREAD_create( decompress_stream, NULL, THREAD_PRIORITY_NORMAL, 1 );

	glViewport( 0.0f, 0.0f, width, height );
	
	glGetIntegerv( GL_VIEWPORT, viewport_matrix );

	srandom( get_milli_time() );

	load_level();

}


void templateAppDraw( void ) {

	if( game_state == 2 ) {

		free_level();
		load_level();
	}
   
	glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

	GFX_set_matrix_mode( PROJECTION_MATRIX );
	GFX_load_identity();
	
	GFX_set_perspective( 80.0f,
						 ( float )viewport_matrix[ 2 ] / ( float )viewport_matrix[ 3 ],
						 0.1f,
						 50.0f,
						 0.0f );

	GFX_set_matrix_mode( MODELVIEW_MATRIX );
	GFX_load_identity();



	if( view_delta.x || view_delta.y )
	{

		if( view_delta.x ) next_rotz -= view_delta.x;

		if( view_delta.y )
		{
			next_roty += view_delta.y;
			next_roty = CLAMP( next_roty, -180.0f, -90.0f );
		}

		view_delta.x =
		view_delta.y = 0.0f;
	}


	if( move_delta.z )
	{
		vec3 direction;

		float r = rotz * DEG_TO_RAD,
				      c = cosf( r ),
				      s = sinf( r );

		direction.x =  s * move_delta.y + c * move_delta.x;
		direction.y =  c * move_delta.y - s * move_delta.x;

		player->btrigidbody->setAngularVelocity( 2*btVector3(  -direction.y * ( move_delta.z * 6.7f ),
															 -direction.x * ( move_delta.z * 6.7f ),
															  0.0f ) );


		player->btrigidbody->setActivationState( ACTIVE_TAG );
	}

		//console_print("player_x: %3.f player_y: %3.f\n",player->location.x,player->location.y);

	next_eye.x = player->location.x +
					 distance *
					 cosf( roty * DEG_TO_RAD ) *
					 sinf( rotz * DEG_TO_RAD );

	next_eye.y = player->location.y -
					 distance *
					 cosf( roty * DEG_TO_RAD ) *
					 cosf( rotz * DEG_TO_RAD );


	next_eye.z = player->location.z +
					 distance *
					 sinf( roty * DEG_TO_RAD );
		

	btVector3 p1( player->location.x,
					 player->location.y,
					 player->location.z ),

			 p2( next_eye.x,
					 next_eye.y,
					 next_eye.z );

	ClosestNotMeRayResultCallback back_ray( player->btrigidbody,
											   p1,
											   p2 );
	dynamicsworld->rayTest( p1,
							   p2,
							   back_ray );

	if( back_ray.hasHit() )
	{

		back_ray.m_hitNormalWorld.normalize();

		next_eye.x =   back_ray.m_hitPointWorld.x() +
					   ( back_ray.m_hitNormalWorld.x() * 0.1f );

		next_eye.y =   back_ray.m_hitPointWorld.y() +
					   ( back_ray.m_hitNormalWorld.y()* 0.1f );

		next_eye.z =   back_ray.m_hitPointWorld.z() +
					   ( back_ray.m_hitNormalWorld.z()* 0.1f );
	}


	roty = roty * 0.9f + next_roty * 0.1f;
	rotz = rotz * 0.9f + next_rotz * 0.1f;

	eye.x = eye.x * 0.95f + next_eye.x * 0.05f;
	eye.y = eye.y * 0.95f + next_eye.y * 0.05f;
	eye.z = eye.z * 0.95f + next_eye.z * 0.05f;

	player->location.z += player->dimension.z * 0.5f;

	/*
	 * needed for directional audio
	 */
	vec3 player_location={player->location.x,player->location.y,player->location.z};
	AUDIO_set_listener( &eye, &player_location, &up );


	GFX_look_at( &eye,
				 &player->location,
				 &up );
	
	build_frustum( frustum,
				  GFX_get_modelview_matrix(),
				  GFX_get_projection_matrix() );	


	unsigned int i = 0;

	while( i != obj->n_objmesh ) {

		OBJMESH *objmesh = &obj->objmesh[ i ];

		objmesh->distance = sphere_distance_in_frustum( frustum, 
														&objmesh->location,
														objmesh->radius );

		if( objmesh->distance && objmesh->visible )
		{
			GFX_push_matrix();

			if( strstr( objmesh->name, "gem" ) ) {

				GFX_translate( objmesh->location.x, 
							   objmesh->location.y, 
							   objmesh->location.z );
				
				objmesh->rotation.z += 1.0f;

				GFX_rotate( objmesh->rotation.z, 0.0f, 0.0f, 1.0f );
			}
			
			else if( objmesh->btrigidbody )
			{
				mat4 mat;

				objmesh->btrigidbody->getWorldTransform().getOpenGLMatrix( ( float * )&mat );
				
				memcpy( &objmesh->location, ( vec3 * )&mat.m[ 3 ], sizeof( vec3 ) );

				GFX_multiply_matrix( &mat );				
			}
			else
			{
				GFX_translate( objmesh->location.x, 
							   objmesh->location.y, 
							   objmesh->location.z );
			}

			OBJ_draw_mesh( obj, i );

			GFX_pop_matrix();
		}
		
		++i;
	}
	if(!game_state )
	{
	dynamicsworld->stepSimulation( 1.0f / 60.0f );
	}
	
	GFX_set_matrix_mode( PROJECTION_MATRIX );
	GFX_load_identity();

	float half_width  = ( float )viewport_matrix[ 2 ] * 0.5f,
		  half_height = ( float )viewport_matrix[ 3 ] * 0.5f;
	GFX_set_orthographic_2d( -half_width,
			                  half_width,
						     -half_height,
						      half_height );

	GFX_rotate( 0.0f, 0.0f, 0.0f, 1.0f );

	GFX_translate( -half_width, -half_height, 0.0f );



	GFX_set_matrix_mode( MODELVIEW_MATRIX );

	GFX_load_identity();
	
	vec4 font_color = { 0.0f, 0.0f, 0.0f, 1.0f };

	char gem_str  [ MAX_CHAR ] = {""},
		 time_str [ MAX_CHAR ] = {""},
		 level_str[ MAX_CHAR ] = {""};

	if( !game_state )
	{
	
		sprintf( level_str, "Level Clear!" );

		FONT_print( font_big,
					viewport_matrix[ 2 ] * 0.5f - FONT_length( font_big, level_str ) * 0.5f + 4.0f,
					viewport_matrix[ 3 ] - font_big->font_size * 1.5f - 4.0f,
					level_str,
					&font_color );

		/* Yellow. */
		font_color.x = 1.0f;
		font_color.y = 1.0f;
		font_color.z = 0.0f;

		FONT_print( font_big,
					viewport_matrix[ 2 ] * 0.5f - FONT_length( font_big, level_str ) * 0.5f,
					viewport_matrix[ 3 ] - font_big->font_size * 1.5f,
					level_str,
					&font_color );

	}

	font_color.x = 0.0f;
	font_color.y = 0.0f;
	font_color.z = 0.0f;

	sprintf( gem_str, "Gem Points:%02d", gem_points );
	sprintf( time_str, "Game Time:%02.2f", game_time * 0.1f );

	FONT_print( font_small,
				viewport_matrix[ 2 ] - FONT_length( font_small, gem_str ) - 6.0f,
				( font_small->font_size * 0.5f ),
				gem_str,
				&font_color );

	FONT_print( font_small,
				8.0f,
				( font_small->font_size * 0.5f ),
				time_str,
				&font_color );

	font_color.x = 1.0f;
	font_color.y = 1.0f;
	font_color.z = 0.0f;

	FONT_print( font_small,
				viewport_matrix[ 2 ] - FONT_length( font_small, gem_str ) - 8.0f,
				( font_small->font_size * 0.5f ),
				gem_str,
				&font_color );
	
	FONT_print( font_small,
				6.0f,
				( font_small->font_size * 0.5f ),
				time_str,
				&font_color );

	if( !game_state ) game_time += SOUND_get_time( background_sound );

}


void templateAppToucheBegan( float x, float y, unsigned int tap_count, unsigned int id )
{
	openBrowser=false;
	if( game_state == 1 && tap_count >= 2 ) game_state = 2;


	if( x < ( screen_size * 0.5f ) )
	{
		if(!movement_touch()&&!game_state)
		{

			touch_type[id]=1;
			console_print("move_change");
			move_location.x = x;
			move_location.y = y;
		}

	}
	else
	{
		if(!view_touch()&&!game_state)
		{
			openBrowser=true;
			touch_type[id]=2;
			console_print("view_change");
			view_location.x = x;
			view_location.y = y;
		}
	}
}
void templateAppToucheMoved( float x, float y, unsigned int tap_count, unsigned int id )
{


	/* First create a "dead zone" that occupies 10% of the screen size,
	 * located at the center of the screen. This way, if the user is on one side
	 * of the screen and swipes all the way to the other side, you can then stop
	 * the movement.
	 */

	if( x > ( ( screen_size * 0.5f ) - ( screen_size * 0.05f ) ) &&
		x < ( ( screen_size * 0.5f ) + ( screen_size * 0.05f ) ) )
	{

		/* Stop the current movement for the view or if the camera is
		 * on the move
		 */
		move_delta.z =
		view_delta.x =
		view_delta.y = 0.0f;

		/* In order to make things easier for the user, assign the current
		 * location of the touch to be either the starting point of the view or the
		 * movement, since you never now in which direction the user will move the
		 * touch.
		 */
		if(touch_type[id]==1)
		{

			move_location.x =x;
			move_location.y =y;
		}
		if(touch_type[id]==2)
		{
			view_location.x =x;
			view_location.y =y;
		}


	}

	/* If the touch start is on the left side of the screen, deal with it
	 * as a camera movement.
	 */
	else if( x < ( screen_size * 0.5f ) )
	{
		if(touch_type[id]==1)
		{
			//console_print("move touch");

			/* Store the current touch as a 3D vector.
			 */
			vec3 touche = { x,
					        y,
					        0.0f };

			/* Calculate the delta to determine which direction the touch is
			 * going.
			*/
			vec3_diff( &move_delta,
					   &move_location,
					   &touche );

			/* Normalize the delta to have a direction vector in the range of
			 * -1 to 1.
			 */
			vec3_normalize( &move_delta,
					        &move_delta );

			/* Calculate the force (basically the distance from the starting
			 * movement location to the current touch location) and divide it by a factor
			 * in pixels. This way, the closer to the starting point, the slower the
			 * movement will be, and as the touch distance increases, the movement speed
			 * will increase up to its maximum.
			 */
			move_delta.z = CLAMP( vec3_dist( &move_location, &touche ) / 128.0f,
																		   0.0f,
																		   1.0f );
		}
	}

	/* Since the touch is on the right side of the screen, simply calculate
	 * the delta for the view so you can then use it to manipulate the X and Z
	 * rotation of the camera.
	 */
	else
	{
		if(touch_type[id]==2)
		{
			/* Calculate the view delta and linearly interpolate the values to
			 * smooth things out a bit.
			 */


			//console_print("view touch");
			view_delta.x = view_delta.x * 0.75f + ( x - view_location.x ) * 0.25f;
			view_delta.y = view_delta.y * 0.75f + ( y - view_location.y ) * 0.25f;

			/* Remember the current location as the starting point for the next
			 * movement (if any).
			*/
			view_location.x = x;
			view_location.y = y;
		}
	}
}
void templateAppToucheEnded( unsigned int tap_count, unsigned int id )
{

	touch_type[id]=0;
	if(!movement_touch())
	{
		move_delta.x =
		move_delta.y =
		move_delta.z = 0.0f;
	}
}


void Pause(bool pause)
{
	if(pause)
	{
		game_state=3;
		console_print("paused");
		SOUND_stop( water_sound );
		SOUND_stop( lava_sound );
		SOUND_stop( toxic_sound );
		SOUND_stop( background_sound );


	}
	else
	{
		game_state=0;
		console_print("unpaused");
		SOUND_play( water_sound, 1 );
		SOUND_play( lava_sound, 1 );
		SOUND_play( toxic_sound, 1 );
		MEMORY *memory;
		memory = mopen( ( char * )"background.ogg", 1 );

		background_soundbuffer = SOUNDBUFFER_load_stream( ( char * )"background", memory );

		background_sound = SOUND_add( ( char * )"background", background_soundbuffer );

		SOUND_set_volume( background_sound, 0.5f );

		SOUND_play( background_sound, 1 );


	}

}

void templateAppAccelerometer( float x, float y, float z )
{
	vec3 tmp = { x, y, z };

	vec3_normalize( &tmp, &tmp );

	accelerometer.x = tmp.x + 0.35f;

	#ifndef __IPHONE_4_0

	  accelerometer.y = tmp.y + 0.35f;
	#else

	  accelerometer.y = tmp.y;
	#endif
}


void templateAppExit( void )
{
	
	free_level();

	thread = THREAD_free( thread );
	
	AUDIO_stop();
}

bool templateAppCallTheBrowser()
{

	return openBrowser;

}



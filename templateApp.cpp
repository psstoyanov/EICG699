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

#define VERTEX_SHADER ( char * )"vertex.glsl"

#define FRAGMENT_SHADER ( char * )"fragment.glsl"

// Used for touch handling.
bool TOUCH_MAXIMUM[20];



OBJ *obj = NULL;

PROGRAM *program = NULL;

float screen_size;

vec2 view_location,
 	 view_delta = { 0.0f, 0.0f };

vec3 move_location = { 0.0f, 0.0f, 0.0f },
	 move_delta;

vec3 eye,
     up = { 0.0f, 0.0f, 1.0f };

OBJMESH *player = NULL;


vec3 next_eye;

float roty		= -165.0f,
      next_roty = roty,
      rotz		= 180.0f,
      next_rotz	= rotz,
      distance	= -5.0f;



// All functions defined through glue code
// must be added in the same order
// to the structure of the application.

TEMPLATEAPP templateApp = { templateAppInit,
							templateAppDraw,
							templateAppToucheBegan,
							templateAppToucheMoved,
							templateAppToucheMoved2,
							templateAppToucheEnded };


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

	glBindAttribLocation( program->pid, 0, "POSITION" );
	glBindAttribLocation( program->pid, 2, "TEXCOORD0" );
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


void templateAppInit( int width, int height ) {

	screen_size = width;

	atexit( templateAppExit );

	GFX_start();

	glViewport( 0.0f, 0.0f, width, height );

	GFX_set_matrix_mode( PROJECTION_MATRIX );
	GFX_load_identity();
	
	GFX_set_perspective( 80.0f,
						 ( float )width / ( float )height,
						 0.1f,
						 100.0f,
						 0.0f );

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
	

	player = OBJ_get_mesh( obj, "player", 0 );
	
	player->btrigidbody->setFriction( 10.0f );
	
	memcpy( &eye, &player->location, sizeof( vec3 ) );
	
	

	i = 0;
	while( i != obj->n_texture ) { 

		OBJ_build_texture( obj,
						   i,
						   obj->texture_path,
						   TEXTURE_MIPMAP,
						   TEXTURE_FILTER_2X,
						   0.0f );
		++i;
	}


	i = 0;
	while( i != obj->n_objmaterial ) { 

		OBJ_build_material( obj, i, NULL );
		
		++i;
	}	
	
	program = PROGRAM_create( ( char * )"default",
							  VERTEX_SHADER,
							  FRAGMENT_SHADER,
							  1,
							  0,
							  program_bind_attrib_location,
							  NULL );

	PROGRAM_draw( program );
	
	glUniform1i( PROGRAM_get_uniform_location( program, ( char * )"DIFFUSE" ), 1 );
}


void templateAppDraw( void ) {

	glClearColor( 0.5f, 0.5f, 0.5f, 1.0f );
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );


	GFX_set_matrix_mode( MODELVIEW_MATRIX );
	GFX_load_identity();


	if( view_delta.x || view_delta.y ) { 

		if( view_delta.x ) next_rotz -= view_delta.x;

		if( view_delta.y ) {
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

		player->btrigidbody->setAngularVelocity( btVector3(  -direction.y * ( move_delta.z * 6.7f ),
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

	if( back_ray.hasHit() ) { 

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


	GFX_look_at( &eye,
				 &player->location,
				 &up );
	

	unsigned int i = 0;

	while( i != obj->n_objmesh ) {

		OBJMESH *objmesh = &obj->objmesh[ i ];

		GFX_push_matrix();

		mat4 mat;
		
		objmesh->btrigidbody->getWorldTransform().getOpenGLMatrix( ( float * )&mat );
		
		memcpy( &objmesh->location, ( vec3 * )&mat.m[ 3 ], sizeof( vec3 ) );

		GFX_multiply_matrix( &mat );		

		glUniformMatrix4fv( program->uniform_array[ 0 ].location,
							1,
							GL_FALSE,
							( float * )GFX_get_modelview_projection_matrix() );

		OBJ_draw_mesh( obj, i );
		
		GFX_pop_matrix();
		
		++i;
	}
	
	dynamicsworld->stepSimulation( 1.0f / 60.0f );
}


void templateAppToucheBegan( float x, float y, unsigned int tap_count )
{
	if( x < ( screen_size * 0.5f ) )
	{
		move_location.x = x;
		move_location.y = y;
	}
	else
	{
		view_location.x = x;
		view_location.y = y;
	}
}

void templateAppToucheMoved2( float x, float y, unsigned int tap_count, unsigned int id )
{


	//console_print("touch id: %3.f\n",id);
	if(id==3)
		console_print("nai-setne");
	if( x > ( ( screen_size * 0.5f ) - ( screen_size * 0.05f ) ) &&
		x < ( ( screen_size * 0.5f ) + ( screen_size * 0.05f ) ) ) {

		move_delta.z =
		view_delta.x =
		view_delta.y = 0.0f;

		move_location.x = x;
		move_location.y = y;

		view_location.x = x;
		view_location.y = y;

	}

	else if( x < ( screen_size * 0.5f ) ) {

		vec3 touche = { x,
						y,
						0.0f };

		vec3_diff( &move_delta,
				   &move_location,
				   &touche );

		vec3_normalize( &move_delta,
						&move_delta );

		move_delta.z = CLAMP( vec3_dist( &move_location, &touche ) / 128.0f,
							  0.0f,
							  1.0f );
	}

	else {

		view_delta.x = view_delta.x * 0.75f + ( x - view_location.x ) * 0.25f;
		view_delta.y = view_delta.y * 0.75f + ( y - view_location.y ) * 0.25f;

		view_location.x = x;
		view_location.y = y;
	}
}


void templateAppToucheMoved( float x, float y, unsigned int tap_count )
{
	if( x > ( ( screen_size * 0.5f ) - ( screen_size * 0.05f ) ) &&
		x < ( ( screen_size * 0.5f ) + ( screen_size * 0.05f ) ) ) {
		
		move_delta.z =
		view_delta.x =
		view_delta.y = 0.0f;
		
		move_location.x = x;
		move_location.y = y;
		
		view_location.x = x;
		view_location.y = y;
	}
	
	else if( x < ( screen_size * 0.5f ) ) {

		vec3 touche = { x, 
						y,
						0.0f };

		vec3_diff( &move_delta, 
				   &move_location,
				   &touche );

		vec3_normalize( &move_delta, 
						&move_delta );
		
		move_delta.z = CLAMP( vec3_dist( &move_location, &touche ) / 128.0f,
							  0.0f,
							  1.0f );
	}
	
	else {

		view_delta.x = view_delta.x * 0.75f + ( x - view_location.x ) * 0.25f;
		view_delta.y = view_delta.y * 0.75f + ( y - view_location.y ) * 0.25f;

		view_location.x = x;
		view_location.y = y;
	}
}


void templateAppToucheEnded( float x, float y, unsigned int tap_count )
{
	move_delta.z = 0.0f;
}


void templateAppExit( void ) {

	free_physic_world();
	
	SHADER_free( program->vertex_shader );

	SHADER_free( program->fragment_shader );

	PROGRAM_free( program );

	OBJ_free( obj );
}

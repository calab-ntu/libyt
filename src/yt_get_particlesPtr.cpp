#include "yt_combo.h"
#include "libyt.h"

//-------------------------------------------------------------------------------------------------------
// Function    :  yt_get_particlesPtr
// Description :  Get pointer of the array of struct yt_particle with length num_particles.
//
// Note        :  1. User should call this function after yt_set_parameter(), since we initialize 
//                   particle_list then.
//
// Parameter   :  yt_particle **particle_list  : Store the particle list array pointer.
//
// Return      :  YT_SUCCESS or YT_FAIL
//-------------------------------------------------------------------------------------------------------
//
int yt_get_particlesPtr( yt_particle **particle_list )
{
	// check if libyt has been initialized
   	if ( !g_param_libyt.libyt_initialized ){
    	YT_ABORT( "Please invoke yt_init() before calling %s()!\n", __FUNCTION__ );     	
   	}

	// check if yt_set_parameter() have been called
   	if ( !g_param_libyt.param_yt_set ) {
    	YT_ABORT( "Please invoke yt_set_parameter() before calling %s()!\n", __FUNCTION__ );
    }

    // check if num_species > 0, if not, particle_list won't be initialized
    if ( g_param_yt.num_species <= 0 ){
    	YT_ABORT( "num_species == %d <= 0, you don't need to input particle_list, and it is also not initialized!\n",
    	           g_param_yt.num_species);
    }

   	log_info( "Getting pointer to particle list information ...\n" );

   	*particle_list = g_param_yt.particle_list;

   	g_param_libyt.get_particlesPtr = true;
	log_info( "Getting pointer to particle list information  ... done.\n" );
	
	return YT_SUCCESS;
}
#include "yt_combo.h"
#include "string.h"

//-------------------------------------------------------------------------------------------------------
// Description :  List of libyt C extension python methods
//
// Note        :  1. List of python C extension methods functions.
//                2. These function will be called in python, so the parameters indicate python 
//                   input type.
// 
// Lists       :       Python Method         C Extension Function         
//              .............................................................
//                     derived_func          libyt_field_derived_func
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// Function    :  libyt_field_derived_func
// Description :  Use the derived_func defined inside yt_field struct to derived the field according to 
//                this function.
//
// Note        :  1. Support only grid dimension = 3 for now.
//                2. We assume that parallelism in yt will make each rank only has to deal with the local
//                   grids. So we can always find one grid with id = gid inside grids_local.
//                   (Maybe we can add feature get grids data from other rank in the future!)
//                3. The returned numpy array data type is numpy.double.
//                
// Parameter   :  int : GID of the grid
//                str : field name
//
// Return      :  numpy.ndarray
//-------------------------------------------------------------------------------------------------------
static PyObject* libyt_field_derived_func(PyObject *self, PyObject *args){

    // Parse the input arguments input by python.
    // If not in the format libyt.derived_func( int , str ), raise an error
    long  gid;
    char *field_name;

    if ( !PyArg_ParseTuple(args, "ls", &gid, &field_name) ){
        PyErr_SetString(PyExc_ValueError, "Wrong input type, expect to be libyt.derived_func(int, str).");
        return NULL;
    }

    // Get the derived_func define in field_list according to field_name.
    // If cannot find field_name inside field_list, raise an error.
    // If we successfully find the field_name, but the derived_func is not assigned (is NULL), raise an error.
    void (*derived_func) (long, double*);
    bool have_FieldName = false;

    for (int v = 0; v < g_param_yt.num_fields; v++){
        if ( strcmp(g_param_yt.field_list[v].field_name, field_name) == 0 ){
            have_FieldName = true;
            if ( g_param_yt.field_list[v].derived_func != NULL ){
                derived_func = g_param_yt.field_list[v].derived_func;
            }
            else {
                PyErr_Format(PyExc_AttributeError, "In field_list, field_name [ %s ], derived_func does not set properly.\n", 
                             g_param_yt.field_list[v].field_name);
                return NULL;
            }
            break;
        }
    }

    if ( !have_FieldName ) {
        PyErr_Format(PyExc_AttributeError, "Cannot find field_name [ %s ] in field_list.\n", field_name);
        return NULL;
    }

    // Get the grid's dimension[3] according to the gid.
    // We assume that parallelism in yt will make each rank only has to deal with the local grids.
    // We can always find grid with id = gid inside grids_local.
    int  grid_dimensions[3];
    bool have_Grid = false;

    for (int lid = 0; lid < g_param_yt.num_grids_local; lid++){
        if ( g_param_yt.grids_local[lid].id == gid ){
            have_Grid = true;
            grid_dimensions[0] = g_param_yt.grids_local[lid].dimensions[0];
            grid_dimensions[1] = g_param_yt.grids_local[lid].dimensions[1];
            grid_dimensions[2] = g_param_yt.grids_local[lid].dimensions[2];
            break;
        }
    }

    if ( !have_Grid ){
        int MyRank;
        MPI_Comm_rank(MPI_COMM_WORLD, &MyRank);
        PyErr_Format(PyExc_AttributeError, "Cannot find grid with GID [ %ld ] on MPI rank [%d].\n", gid, MyRank);
        return NULL;
    }

    // Allocate 1D array with size of grid dimension, initialized with 0.
    // derived_func will make changes to this array.
    // This array will be wrapped by Numpy API and will be return. 
    // The called object will then OWN this numpy array, so that we don't have to free it.
    long gridTotalSize = grid_dimensions[0] * grid_dimensions[1] * grid_dimensions[2];
    double *output = (double *) malloc( gridTotalSize * sizeof(double) );
    for (long i = 0; i < gridTotalSize; i++) {
        output[i] = (double) 0;
    }

    // Call the derived_func, result will be made inside output 1D array.
    (*derived_func) (gid, output);

    // Wrapping the C allocated 1D array into 3D numpy array.
    int      nd = 3;
    npy_intp dims[3] = {grid_dimensions[0], grid_dimensions[1], grid_dimensions[2]};
    int      typenum = NPY_DOUBLE;
    PyObject *derived_NpArray = PyArray_SimpleNewFromData(nd, dims, typenum, output);
    PyArray_ENABLEFLAGS( (PyArrayObject*) derived_NpArray, NPY_ARRAY_OWNDATA);

    return derived_NpArray;
}

//-------------------------------------------------------------------------------------------------------
// Description :  Preparation for creating libyt python module
//
// Note        :  1. Contains data blocks for creating libyt python module.
//                2. Only initialize libyt python module, not import to system yet.
// 
// Lists:      :  libyt_method_list       : Declare libyt C extension python methods.
//                libyt_module_definition : Definition to libyt python module.
//                PyInit_libyt            : Create libyt python module, and append python objects, 
//                                          ex: dictionary.
//-------------------------------------------------------------------------------------------------------

// Define functions in module, list all libyt module methods here
static PyMethodDef libyt_method_list[] =
{
// { "method_name", c_function_name, METH_VARARGS, "Description"},
   {"derived_func", libyt_field_derived_func, METH_VARARGS, 
    "Input GID and field name, and get the field data derived by derived_func."},
   { NULL, NULL, 0, NULL } // sentinel
};

// Declare the definition of libyt_module
static struct PyModuleDef libyt_module_definition = 
{
    PyModuleDef_HEAD_INIT,
    "libyt",
    "libyt documentation",
    -1,
    libyt_method_list
};

// Create libyt python module
static PyObject* PyInit_libyt(void)
{
  // Create libyt module
  PyObject *libyt_module = PyModule_Create( &libyt_module_definition );
  if ( libyt_module != NULL ){
    log_debug( "Creating libyt module ... done\n" );
  }
  else {
    YT_ABORT(  "Creating libyt module ... failed!\n");
  }

  // Add objects dictionary
  g_py_grid_data  = PyDict_New();
  g_py_hierarchy  = PyDict_New();
  g_py_param_yt   = PyDict_New();
  g_py_param_user = PyDict_New();

  PyModule_AddObject(libyt_module, "grid_data",  g_py_grid_data );
  PyModule_AddObject(libyt_module, "hierarchy",  g_py_hierarchy );
  PyModule_AddObject(libyt_module, "param_yt",   g_py_param_yt  );
  PyModule_AddObject(libyt_module, "param_user", g_py_param_user);

  log_debug( "Attaching empty dictionaries to libyt module ... done\n" );

  return libyt_module;
}

//-------------------------------------------------------------------------------------------------------
// Function    :  create_libyt_module
// Description :  Create the libyt module
//
// Note        :  1. Create libyt module, should be called before Py_Initialize().
//                2. It is used for sharing data between simulation code and YT.
//
// Parameter   :  None
//
// Return      :  YT_SUCCESS or YT_FAIL
//-------------------------------------------------------------------------------------------------------
int create_libyt_module()
{
  PyImport_AppendInittab("libyt", &PyInit_libyt);

  return YT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------------
// Function    :  init_libyt_module
// Description :  Initialize the libyt module
//
// Note        :  1. Import newly created libyt module.
//                2. Load user script to python.
//                
// Parameter   :  None
//
// Return      :  YT_SUCCESS or YT_FAIL
//-------------------------------------------------------------------------------------------------------
int init_libyt_module()
{

// import newly created libyt module
   if ( PyRun_SimpleString("import libyt\n") == 0 )
      log_debug( "Import libyt module ... done\n" );
   else
      YT_ABORT(  "Import libyt module ... failed!\n" );


// import YT inline analysis script
   const int CallYT_CommandWidth = 8 + strlen( g_param_libyt.script );   // 8 = "import " + '\0'
   char *CallYT = (char*) malloc( CallYT_CommandWidth*sizeof(char) );
   sprintf( CallYT, "import %s", g_param_libyt.script );

   if ( PyRun_SimpleString( CallYT ) == 0 )
      log_debug( "Importing YT inline analysis script \"%s\" ... done\n", g_param_libyt.script );
   else
      YT_ABORT(  "Importing YT inline analysis script \"%s\" ... failed (please do not include the \".py\" extension)!\n",
                g_param_libyt.script );

   free( CallYT );

   return YT_SUCCESS;

} // FUNCTION : init_libyt_module


# compilation options (NOT USED YET)
#######################################################################################################
# double precision
#SIMU_OPTION += -DFLOAT8

# debug mode
#SIMU_OPTION += -DYT_DEBUG


# source files
#######################################################################################################
CC_FILE := yt_init.cpp  yt_finalize.cpp  yt_set_parameter.cpp  yt_inline.cpp  yt_add_user_parameter.cpp \
           yt_commit_grids.cpp yt_get_gridsPtr.cpp yt_get_fieldsPtr.cpp yt_get_particlesPtr.cpp         \
           yt_free_gridsPtr.cpp yt_getGridInfo.cpp
CC_FILE += logging.cpp  init_python.cpp  init_libyt_module.cpp  add_dict.cpp  allocate_hierarchy.cpp    \
		   check_hierarchy.cpp append_grid.cpp


# library name
#######################################################################################################
LIB_REALNAME := libyt.so.1.0.0
LIB_SONAME   := libyt.so.1


# different paths
#######################################################################################################
PYTHON_PATH := $(YOUR_PYTHON_PATH)
OBJ_PATH := ./obj


# flags
#######################################################################################################
#CXX := icpc
CXX := mpic++

LIB := -L$(PYTHON_PATH)/lib -lpython3.8

INCLUDE := -I../include -I$(PYTHON_PATH)/include/python3.8 \
           -I$(PYTHON_PATH)/lib/python3.8/site-packages/numpy/core/include

#CXXWARN_FLAG := -w1
CXXWARN_FLAG := -Wall -Wno-write-strings
CXXFLAG := $(CXXWARN_FLAG) $(INCLUDE) $(SIMU_OPTION) -O2 -fPIC


# rules and targets
#######################################################################################################
OBJ := $(patsubst %.cpp, $(OBJ_PATH)/%.o, $(CC_FILE))

$(OBJ_PATH)/%.o : %.cpp
	 $(CXX) $(CXXFLAG) -o $@ -c $<


# link
#######################################################################################################
$(LIB_REALNAME) : $(OBJ)
	 $(CXX) -shared -Wl,-soname,$(LIB_SONAME) -o $@ $^ $(LIB)


# miscellaneous
#######################################################################################################
clean :
	 rm -f $(OBJ)
	 rm -f $(LIB_REALNAME)




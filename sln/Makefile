# arguments
runlink                 = static
platform                = linux/x64



# paths home
project_home            = ..
build_dir               = $(project_home)/sln
bin_dir                 = $(project_home)/lib/$(platform)
object_dir              = $(project_home)/.objs



# includes of cm256_codec headers
cm256_codec_inc_path    = $(project_home)/inc
cm256_codec_includes    = -I$(cm256_codec_inc_path)



# all includes that cm256_codec solution needs
includes                = $(cm256_codec_includes)



# source files of cm256_codec solution
cm256_codec_src_path    = $(project_home)/src
cm256_codec_source      = $(filter %.cpp, $(shell find $(cm256_codec_src_path) -depth -name "*.cpp"))



# objects of cm256_codec solution
cm256_codec_objects     = $(cm256_codec_source:$(project_home)%.cpp=$(object_dir)%.o)



# cm256_codec depends libraries
cm256_codec_depends     =



# output libraries
ifeq ($(runlink), static)
	cm256_codec_outputs = $(bin_dir)/libcm256_codec.a
else ifeq ($(platform), mac)
	cm256_codec_outputs = $(bin_dir)/libcm256_codec.dylib
else
	cm256_codec_outputs = $(bin_dir)/libcm256_codec.so
endif



# build output command line
ifeq ($(runlink), static)
	build_command       = ar -rv $(cm256_codec_outputs) $^
else
	build_command       = g++ -std=c++11 -shared -o $(cm256_codec_outputs) $^ $(cm256_codec_depends)
endif



# build targets
targets                 = cm256_codec

# let 'build' be default target, build all targets
build    : $(targets)

cm256_codec      : $(cm256_codec_objects)
	@echo
	@echo "@@@@@  start making cm256_codec  @@@@@"
	$(build_command)
	@echo "@@@@@  make cm256_codec success  @@@@@"
	@echo

# build all objects
$(object_dir)/%.o:$(project_home)/%.cpp
	@dir=`dirname $@`;		\
	if [ ! -d $$dir ]; then	\
		mkdir -p $$dir;		\
	fi
	g++ -c -std=c++11 -g -Wall -O1 -pipe -fPIC -mssse3 -DUSE_SSSE3 $(includes) -o $@ $<

clean            :
	rm -rf $(object_dir) $(bin_dir)/libcm256_codec.*

rebuild          : clean build

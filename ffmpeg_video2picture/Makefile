#----------------------------------------------------------------------------
#       Macros
#----------------------------------------------------------------------------
PROJECT       = video2picture
BIN           = $(BTARGETDIR)video2picture
OBJS          = $(patsubst %.cpp, $(OBJSDIR)%.o, $(wildcard *.cpp)) \
				$(patsubst %.c, $(OBJSDIR)%.o, $(wildcard *.c))
BTARGETDIR    = ./bin/
OBJSDIR       = ../objs/
INCLUDEDIR    = -I"$(ACE_ROOT)" -I"./" -I"/usr/local/ffmpeg/include"
LIBDIR        = -L"/usr/local/ffmpeg/lib" 
CPPFLAGS      = -fPIC -O -D_REENTRANT $(INCLUDEDIR) -g -std=gnu++11
LDFLAGS       = $(LIBDIR)
LDLIBS        = -lavcodec -lavformat -lavutil -lswscale -ldl -lpthread
COMPILE.cc    = g++ $(CPPFLAGS) -c -fpermissive
LINK.cc       = g++ $(CPPFLAGS) $(LDFLAGS)


#----------------------------------------------------------------------------
#       Local targets
#----------------------------------------------------------------------------

$(BIN): $(OBJS)
	@test -d "$(BTARGETDIR)" || mkdir -p "$(BTARGETDIR)"
	$(LINK.cc) $(OBJS) $(LDLIBS) -o $@
	
$(OBJSDIR)%.o: %.cpp
	@test -d "$(OBJSDIR)" || mkdir -p "$(OBJSDIR)"
	$(COMPILE.cc) -o $@ $<

$(OBJSDIR)%.o: %.c
	@test -d "$(OBJSDIR)" || mkdir -p "$(OBJSDIR)"
	$(COMPILE.cc) -o $@ $<
	
clean:
	-rm -rf $(OBJSDIR)

realclean: clean
	-rm -rf $(BIN)

include build.conf

INCLUDE = -I/usr/include \
					-I./utils \
					-I/usr/local/include \
					-I/home/http/FCGI_ADSERVER/Build/glog/include \
					-I/home/http/FCGI_ADSERVER/Build/json-c-0.9

#-I/home/http/FCGI_ADSERVER/Build/gtest-1.7.0/include \

SRC = mbf.cc \
			db_mbf.cc \
			mbftable.cc \
			mbftargeting.cc \
			platformfeetable.cc \
			utils/targeting.cc \
			utils/util.cc \
			utils/db.c

#SRC_TEST = utils/util_test.cc


OBJ_DIR = obj

OUTLIB = $(OBJ_DIR)/libmbf.a

# TEST = $(OBJ_DIR)/unit_test

OBJ = $(OBJ_DIR)/mbf.o \
			$(OBJ_DIR)/db_mbf.o \
			$(OBJ_DIR)/mbftable.o \
			$(OBJ_DIR)/mbftargeting.o \
			$(OBJ_DIR)/platformfeetable.o \
			$(OBJ_DIR)/db.o \
			$(OBJ_DIR)/targeting.o \
			$(OBJ_DIR)/util.o


#OBJ_TEST = $(OBJ_DIR)/util_test.o

#LDFLAGS = -L/usr/local/lib -lcmph -lstdc++

LDFLAGS = -L. \
					-L/usr/local/lib \
					-L/home/http/FCGI_ADSERVER/Build/json-c-0.9/.libs \
					-L/home/http/FCGI_ADSERVER/Build/glog/lib \
					-lstdc++ \
					-ljson \
					-lglog \
					-lgtest \
					-lodbc \
					-lpthread \
					-lrt \
					-lm

# -L/home/http/FCGI_ADSERVER/Build/gtest-1.7.0 \

all : $(OUTLIB)

make_dir:
	@echo "-------------------------------"
	@echo "Creating Obj"
	@echo "-------------------------------"
	mkdir -p $(OBJ_DIR)

$(OUTLIB) : make_dir $(OBJ)
	ar -r $(OUTLIB) $(OBJ)
	@echo "-------------------------------"
	@echo "Library Build Complete"
	@echo "-------------------------------"

#$(TEST) : $(OBJ_TEST)
#$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE) $(OUTLIB) $(OBJ_TEST) $(LDFLAGS) -o $@

clean : 
	\rm -rf $(OBJ_DIR)

# This can be used to make multiline comments
#ifeq ("x", "y")
#endif

$(OBJ_DIR)/mbf.o : db_mbf.h db_mbf.cc mbf.cc mbf.h mbftable.cc mbftable.h mbftargeting.cc mbftargeting.h utils/util.h utils/util.cc utils/targeting.h utils/targeting.cc utils/db.c utils/db.h platformfeetable.h

	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE) -c mbf.cc -o $@


$(OBJ_DIR)/targeting.o : db_mbf.h db_mbf.cc mbf.cc mbf.h mbftable.cc mbftable.h mbftargeting.cc mbftargeting.h utils/util.h utils/util.cc utils/targeting.h utils/targeting.cc utils/db.c utils/db.h platformfeetable.h

	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE) -c utils/targeting.cc -o $@

$(OBJ_DIR)/util.o : db_mbf.h db_mbf.cc mbf.cc mbf.h mbftable.cc mbftable.h mbftargeting.cc mbftargeting.h utils/util.h utils/util.cc utils/targeting.h utils/targeting.cc utils/db.c utils/db.h platformfeetable.h

	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE) -c utils/util.cc -o $@

$(OBJ_DIR)/mbftargeting.o : db_mbf.h db_mbf.cc mbf.cc mbf.h mbftable.cc mbftable.h mbftargeting.cc mbftargeting.h utils/util.h utils/util.cc utils/targeting.h utils/targeting.cc utils/db.c utils/db.h platformfeetable.h

	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE) -c mbftargeting.cc -o $@

$(OBJ_DIR)/platformfeetable.o : db_mbf.h db_mbf.cc mbf.cc mbf.h mbftable.cc mbftable.h mbftargeting.cc mbftargeting.h utils/util.h utils/util.cc utils/targeting.h utils/targeting.cc utils/db.c utils/db.h platformfeetable.h

	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE) -c platformfeetable.cc -o $@

$(OBJ_DIR)/mbftable.o : db_mbf.h db_mbf.cc mbf.cc mbf.h mbftable.cc mbftable.h mbftargeting.cc mbftargeting.h utils/util.h utils/util.cc utils/targeting.h utils/targeting.cc utils/db.c utils/db.h platformfeetable.h

	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE) -c mbftable.cc -o $@

$(OBJ_DIR)/db_mbf.o : db_mbf.h db_mbf.cc mbf.cc mbf.h mbftable.cc mbftable.h mbftargeting.cc mbftargeting.h utils/util.h utils/util.cc utils/targeting.h utils/targeting.cc utils/db.c utils/db.h platformfeetable.h

	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE) -c db_mbf.cc -o $@

$(OBJ_DIR)/db.o : utils/db.c utils/db.h
	$(CC) $(CFLAGS) $(INCLUDE) -c utils/db.c -o $@

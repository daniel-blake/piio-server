AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = $(DEPS_CFLAGS)

DISTCLEANFILES  =   src/pi-io-server-glue.hpp \
                    script/init.d/piio-server \
					version.py

I2C_SRC =           src/i2c/i2c.h \
					src/i2c/i2c.c


EXCEPTION_SRC = 	src/exception/baseexceptions.hpp \
                    src/exception/baseexceptions.cpp

LOG_SRC =			src/log/log.hpp \
                    src/log/log.cpp 

THREAD_SRC = 		src/thread/thread.hpp \
                    src/thread/thread.cpp 

BUTTONTIMER_SRC = 	src/buttontimer/buttontimer.hpp \
                    src/buttontimer/buttontimer.cpp 

MCP_GPIO_SRC =      src/gpio/gpio.hpp \
                    src/gpio/gpio.cpp \
                    src/gpio/c_gpio.h \
                    src/gpio/c_gpio.c \
                    src/mcp23017/mcp23017.hpp \
                    src/mcp23017/mcp23017.cpp \
                    $(I2C_SRC) \
                    $(EXCEPTION_SRC)
                    
PCA9685_SRC =       src/pca9685/pca9685.cpp \
					src/pca9685/pca9685.hpp 

IOGROUP_SRC =       src/gpioregistry.hpp \
                    src/gpioregistry.cpp \
                    src/iogroup-base.hpp \
                    src/iogroup-base.cpp \
                    src/iogroup-digital.hpp \
                    src/iogroup-digital.cpp \
                    src/iogroup-softpwm.hpp \
                    src/iogroup-softpwm.cpp \
                    src/iogroup-hwpwm.hpp \
                    src/iogroup-hwpwm.cpp \
                    src/iogroup-gpio.hpp \
                    src/iogroup-gpio.cpp \
                    src/iogroup-mcp23017.hpp \
                    src/iogroup-mcp23017.cpp \
                    src/iogroup-pca9685.hpp \
                    src/iogroup-pca9685.cpp 
                    

## Programs to install

sbin_PROGRAMS   =   piio-server
check_PROGRAMS  =   mcp23017-i2ctest configtest c_gpiotest pca9685-test

## Configuration files to install

EXTRA_DIST =    cfg/piio.conf \
                cfg/dbus/nl.miqra.piio.conf \
                cfg/init.d/piio-server.in \
                cfg/rsyslog/syslog.piio.conf \
                src/pi-io-introspect.xml

init_d_dirdir = $(sysconfdir)/init.d
init_d_dir_SCRIPTS = cfg/init.d/piio-server

dbus_confdir = $(sysconfdir)/dbus-1/system.d
dbus_conf_DATA = cfg/dbus/nl.miqra.piio.conf

rsyslog_confdir = $(sysconfdir)/rsyslog.d
rsyslog_conf_DATA = cfg/rsyslog/syslog.piio.conf

etcdir = $(sysconfdir)
etc_DATA = cfg/piio.conf

## Sources lists

BUILT_SOURCES       =   src/pi-io-server-glue.hpp 

piio_server_SOURCES =   src/pi-io-server.cpp \
                        src/pi-io-server.hpp \
                        src/pi-io-server-glue.hpp \
                        $(MCP_GPIO_SRC) \
                        $(LOG_SRC) \
                        $(THREAD_SRC) \
                        $(BUTTONTIMER_SRC) \
                        $(IOGROUP_SRC) \
                        $(PCA9685_SRC)
                        
piio_server_LDADD   =  $(DEPS_LIBS) -lpthread -lboost_program_options -lrt

                    
src/pi-io-server-glue.hpp: src/pi-io-introspect.xml
	dbusxx-xml2cpp $^ --adaptor=$@
    
    
mcp23017_i2ctest_SOURCES    =   src/test/mcp23017-i2ctest.c \
                                $(I2C_SRC)


mcp23017_i2ctest_LDADD      =   -lpthread

configtest_SOURCES          =   src/test/configtest.cpp 

configtest_LDADD            =   -lpthread -lboost_program_options $(DEPS_LIBS)

c_gpiotest_SOURCES          =   src/test/c_gpiotest.cpp \
                                src/gpio/c_gpio.h \
                                src/gpio/c_gpio.c 

c_gpiotest_LDADD            =   -lpthread

pca9685_test_SOURCES		=   src/test/pca9685-test.cpp \
								src/pca9685/pca9685.cpp \
								src/pca9685/pca9685.hpp \
								$(I2C_SRC) \
		                        $(LOG_SRC) \
		                        $(THREAD_SRC) \
		                        $(EXCEPTION_SRC)
								

pca9685_test_LDADD          =   -lpthread


cfg/init.d/piio-server: cfg/init.d/piio-server.in
	cat $^ > $@
	sed -i "s#@BIN_DIR@#$(bindir)#" $@
	sed -i "s#@ETC_DIR@#$(sysconfdir)#" $@
	sed -i "s#@SBIN_DIR@#$(sbindir)#" $@

version.py: version.py.in 
	cat $^ > $@
	sed -i "s#@V_PACKAGE@#$(PACKAGE)#" $@
	sed -i "s#@V_VERSION\@#$(VERSION)#" $@
	
	
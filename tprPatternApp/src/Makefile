TOP=../..

include $(TOP)/configure/CONFIG

USR_CXXFLAGS += -std=c++0x
USR_CXXFLAGS += -DHAVE_YAML
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#========================================
INCLUDES += $(addprefix -I,$(HPSTPR_INCLUDE))
INCLUDES += $(addprefix -I,$(YAML_INCLUDE))
INCLUDES += $(addprefix -I,$(BOOST_INCLUDE))

## this part is just for yaml loader driver
INCLUDES += $(addprefix -I,$(CPSW_FRAMEWORK_INCLUDE))



INC += tprPatternAsynDriver.h
INC += tprPatternCommon.h
INC += evrTime.h
DBD += tprPatternAsynDriver.dbd

LIBRARY_IOC = tprPattern

tprPattern_DBD  += tprPatternAsynDriver.dbd
tprPattern_SRCS += tprPatternAsynDriver.cpp
tprPattern_SRCS += tprEvent.cc

yaml-cpp_DIR += $(YAML_LIB)
tprPattern_LIBS += yaml-cpp

cpsw_DIR = $(CPSW_FRAMEWORK_LIB)
tprPattern_LIBS += cpsw


hpstpr_DIR = $(HPSTPR_LIB)
tprPattern_LIBS += hpstpr

tprPattern_LIBS += asyn
tprPattern_LIBS += yamlLoader

tprPattern_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


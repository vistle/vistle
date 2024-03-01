set(HEADERS ReadSoundPlan.h)
set(SOURCES ReadSoundPlan.cpp)
set(MODULE_CAN_CREATE_HASH True)
add_module(ReadSoundPlan "Read SoundPLAN files." ${SOURCES} ${HEADERS})

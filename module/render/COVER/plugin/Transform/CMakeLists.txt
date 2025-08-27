set(HEADERS TransformInteraction.h TransformPlugin.h)
set(SOURCES TransformInteraction.cpp TransformPlugin.cpp)
cover_add_plugin(TransformPlugin ${HEADERS} ${SOURCES})
set_target_properties(TransformPlugin PROPERTIES OUTPUT_NAME Transform)

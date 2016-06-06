aux_source_directory(. SRC)
add_library(mitosha STATIC ${SRC})
install(TARGETS mitosha ARCHIVE DESTINATION lib)
install(FILES ../include/mitosha.h DESTINATION include)

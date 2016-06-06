aux_source_directory(. SRC)
foreach(FILO ${SRC})
  add_mutest(FILE ${FILO} DEPENDS mitosha rt pthread boost_system stdc++)
endforeach(FILO)

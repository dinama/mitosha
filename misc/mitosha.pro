CONFIG -= qt
CONFIG += exceptions c++11 c11

INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/include/c++/4.7
INCLUDEPATH += ../include
INCLUDEPATH += ../src
INCLUDEPATH += ../deps/mutest

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CFLAGS += -std=c11
QMAKE_LFLAGS += -std=c++11 -std=c11

HEADERS += \
    ../include/mitosha.h

SOURCES += \
    ../src/avl.c \
    ../src/btree.c \
    ../src/list.c \
    ../src/test/test_avl.c \
    ../src/test/test_avl_perf.cpp \
    ../src/test/test_page.c \
    ../src/test/test_list.c \
    ../src/pool.c \
    ../tests/test_btree.c \
    ../tests/test_list.c \
    ../src/shma.c \
    ../tests/test_uni.c \
    ../tests/test_shm.c \
    ../tests/test_pool.cc \
    ../tests/test_perf.cpp \
    ../tests/test_avltree.c

OTHER_FILES += \
    ../CMakeLists.txt \
    ../src/CMakeLists.txt \
    ../test/CMakeLists.txt

DISTFILES += \
    ../.clang-format \
    ../tests/CMakeLists.txt \
    ../.gitmodules \
    ../.gitignore \
    ../README.md

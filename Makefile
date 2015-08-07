OBJECTS = Async.o\
          FIFO.o\
          Heap.o\
          IOPoller.o\
          List.o\
          Logging.o\
          Main.o\
          MemoryPool.o\
          RBTree.o\
          Runtime.o\
          Scheduler.o\
          ThreadPool.o\
          Timer.o
CPPFLAGS = -iquote Include -MMD -MT $@ -MF Build/$*.d -D_GNU_SOURCE
#CPPFLAGS += -DNDEBUG
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Werror
#CFLAGS += -O2
ARFLAGS = rc

all: Build/Library.a

Build/Library.a: $(addprefix Build/, $(OBJECTS))
	$(AR) $(ARFLAGS) $@ $^

ifneq ($(MAKECMDGOALS), clean)
-include $(patsubst %.o, Build/%.d, $(OBJECTS))
endif

Build/%.o: Source/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f Build/*

install: all
	mkdir -p /usr/local/lib
	cp -T Build/Library.a /usr/local/lib/libpixy.a
	mkdir -p /usr/local/include
	cp -r -T Include /usr/local/include/Pixy

uninstall:
	rm /usr/local/lib/libpixy.a
	rmdir -p --ignore-fail-on-non-empty /usr/local/lib
	rm -r /usr/local/include/Pixy
	rmdir -p --ignore-fail-on-non-empty /usr/local/include

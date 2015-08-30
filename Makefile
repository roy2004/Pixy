PREFIX = /usr/local/
OBJECTS = Async.o\
          Event.o\
          Heap.o\
          IO.o\
          IOPoller.o\
          List.o\
          Logging.o\
          MemoryPool.o\
          Runtime.o\
          Scheduler.o\
          ThreadPool.o\
          Timer.o\
          Vector.o
CPPFLAGS = -iquote Include -MMD -MT $@ -MF Build/$*.d -D_GNU_SOURCE
#CPPFLAGS += -DNDEBUG
#CPPFLAGS += -DUSE_VALGRIND
CFLAGS = -std=c99 -Wall -Wextra -Werror
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
	mkdir -p $(PREFIX)lib
	cp -T Build/Library.a $(PREFIX)lib/libpixy.a
	mkdir -p $(PREFIX)include
	cp -r -T Include $(PREFIX)include/Pixy

uninstall:
	rm $(PREFIX)lib/libpixy.a
	rmdir -p --ignore-fail-on-non-empty $(PREFIX)lib
	rm -r $(PREFIX)include/Pixy
	rmdir -p --ignore-fail-on-non-empty $(PREFIX)include

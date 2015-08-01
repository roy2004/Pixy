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
          Timer.o
CPPFLAGS = -iquote Include -MMD -MT $@ -MF Build/$*.d -D_GNU_SOURCE
CFLAGS = -std=c99 -Wall -Wextra
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
	cp -T Build/Library.a /usr/local/lib/libpixy.a
	cp -r -T Include /usr/local/include/Pixy

uninstall:
	rm -f /usr/local/lib/libpixy.a
	rm -r -f /usr/local/include/Pixy

OBJECTS = Async.o\
          FIFO.o\
          Heap.o\
          IOPoller.o\
          List.o\
          Logging.o\
          MemoryPool.o\
          RBTree.o\
          Runtime.o\
          Scheduler.o\
          Timer.o
CPPFLAGS = -iquote Include -MMD -MT $@ -MF Build/$*.d
CFLAGS = -Wall -Wextra -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Werror
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

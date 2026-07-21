CC = gcc

SUITE ?= all

CFLAGS = -Wall -Wextra -g -Iallocator -Ireference_counting -Idynamic_array -Imark_and_sweep -Imark_and_compact

COMMON_SRCS = allocator/allocator.c dynamic_array/dynamic_array.c

RC_SRCS = main.c $(COMMON_SRCS) reference_counting/reference_counting.c
SWEEP_SRCS = main.c $(COMMON_SRCS) mark_and_sweep/mark_and_sweep.c
COMPACT_SRCS = main.c $(COMMON_SRCS) mark_and_compact/mark_and_compact.c

RC_TARGET = gc_app_rc
SWEEP_TARGET = gc_app_sweep
COMPACT_TARGET = gc_app_compact

all: $(RC_TARGET) $(SWEEP_TARGET) $(COMPACT_TARGET)

$(RC_TARGET):
	$(CC) $(CFLAGS) -DTEST_REFERENCE_COUNTING -o $@ $(RC_SRCS)
	@echo "Build complete! Run with: make run SUITE=rc"

$(SWEEP_TARGET):
	$(CC) $(CFLAGS) -DTEST_MARK_SWEEP -o $@ $(SWEEP_SRCS)
	@echo "Build complete! Run with: make run SUITE=sweep"

$(COMPACT_TARGET):
	$(CC) $(CFLAGS) -DTEST_MARK_COMPACT -o $@ $(COMPACT_SRCS)
	@echo "Build complete! Run with: make run SUITE=compact"

run: all
	@case "$(SUITE)" in \
		rc) ./$(RC_TARGET) ;; \
		sweep) ./$(SWEEP_TARGET) ;; \
		compact) ./$(COMPACT_TARGET) ;; \
		all) ./$(RC_TARGET) && ./$(SWEEP_TARGET) && ./$(COMPACT_TARGET) ;; \
		*) echo "Unknown SUITE '$(SUITE)'. Use rc, sweep, compact, or all."; exit 1 ;; \
	esac

clean:
	rm -f $(RC_TARGET) $(SWEEP_TARGET) $(COMPACT_TARGET)
	@echo "Workspace cleaned!"
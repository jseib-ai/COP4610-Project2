
.PHONY: all part1 part2 part3 trace clean

# Build the entire project
all: part1 part2 part3

# Build individual project parts
part1:
	$(MAKE) -C part1

part2:
	$(MAKE) -C part2

part3:
	$(MAKE) -C part3

# Run the trace target from part 1
trace:
	$(MAKE) -C part1 trace

# Clean all project parts
clean:
	$(MAKE) -C part1 clean
	$(MAKE) -C part2 clean
	$(MAKE) -C part3 clean

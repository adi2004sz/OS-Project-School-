CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
DEBUG_FLAGS = -g -O0 -DDEBUG
EXEC = treasure_manager treasure_hub score_calculator
TEST_HUNT = test_hunt

all: $(EXEC)

treasure_manager: treasure_manager.c
	$(CC) $(CFLAGS) -o $@ $<

treasure_hub: treasure_hub.c
	$(CC) $(CFLAGS) -o $@ $<

score_calculator: score_calculator.c
	$(CC) $(CFLAGS) -o $@ $<

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

clean:
	rm -f $(EXEC)
	rm -f *~ *.o
	rm -f logged_hunt-*

test_setup: all
	@echo "Setting up test data..."
	@mkdir -p $(TEST_HUNT)
	./treasure_manager add $(TEST_HUNT) 1 alice 37.7749 -122.4194 "Under the red bridge" 100
	./treasure_manager add $(TEST_HUNT) 2 bob 40.7128 -74.0060 "Near the tall tower" 200
	./treasure_manager add $(TEST_HUNT) 3 alice 34.0522 -118.2437 "Behind the palm tree" 150
	@echo "Test data created successfully!"

test: test_setup
	@echo "\n--- Testing treasure_manager ---"
	./treasure_manager list $(TEST_HUNT)
	./treasure_manager view $(TEST_HUNT) 2
	
	@echo "\n--- Testing score_calculator ---"
	./score_calculator $(TEST_HUNT)
	
	@echo "\n--- Testing treasure_hub (manual test) ---"
	@echo "Run './treasure_hub' and use the following commands:"
	@echo "1. start_monitor"
	@echo "2. list_hunts"
	@echo "3. list_treasures $(TEST_HUNT)"
	@echo "4. view_treasure $(TEST_HUNT) 1"
	@echo "5. calculate_score"
	@echo "6. stop_monitor"
	@echo "7. exit"

run: all
	./treasure_hub

test_advanced: all clean_test_data
	@echo "Setting up advanced test data..."
	@mkdir -p hunt1
	@mkdir -p hunt2
	./treasure_manager add hunt1 1 charlie 51.5074 -0.1278 "Under the clock tower" 300
	./treasure_manager add hunt1 2 diana 48.8566 3.3522 "By the river" 250
	./treasure_manager add hunt2 1 edward 35.6762 139.6503 "Behind the cherry tree" 400
	./treasure_manager add hunt2 2 frank 22.3193 114.1694 "Under the bridge" 350
	./treasure_manager add hunt2 3 diana 1.3521 103.8198 "Near the fountain" 450
	@echo "Advanced test data created successfully!"
	@echo "\n--- Testing score calculation for multiple hunts ---"
	./score_calculator hunt1
	./score_calculator hunt2

clean_test_data:
	rm -rf $(TEST_HUNT) hunt1 hunt2

# Memory leak check using valgrind (if available)
memcheck: debug
	@command -v valgrind >/dev/null 2>&1 || { echo "Valgrind not found. Please install it first."; exit 1; }
	@echo "Testing treasure_manager for memory leaks..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./treasure_manager list $(TEST_HUNT)
	@echo "Testing score_calculator for memory leaks..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./score_calculator $(TEST_HUNT)

install: all
	@mkdir -p ~/bin
	cp $(EXEC) ~/bin/

.PHONY: all debug clean test test_setup test_advanced clean_test_data memcheck install run
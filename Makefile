SKETCH_DIR = sketch
SKETCH_FILE = $(SKETCH_DIR)/sketch.ino
FQBN = esp32:esp32:esp32
BUILD_DIR = build

ARDUINO_CLI = ./arduino-cli

RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
NC = \033[0m # No Color

all: compile

check-cli:
	@command -v $(ARDUINO_CLI) >/dev/null 2>&1 || { \
		echo "$(RED)Error: arduino-cli not found!$(NC)"; \
		echo "Please install arduino-cli or set ARDUINO_CLI variable correctly."; \
		exit 1; \
	}

# Check if ESP32 core is installed
check-core: check-cli
	@$(ARDUINO_CLI) core list | grep -q "esp32:esp32" || { \
		echo "$(YELLOW)ESP32 core not found. Installing...$(NC)"; \
		$(ARDUINO_CLI) core update-index; \
		$(ARDUINO_CLI) core install esp32:esp32; \
	}

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile the sketch
compile: check-core $(BUILD_DIR)
	@echo "$(GREEN)Compiling $(SKETCH_FILE) for $(FQBN)...$(NC)"
	$(ARDUINO_CLI) compile \
		--fqbn $(FQBN) \
		--build-path $(BUILD_DIR) \
		$(SKETCH_DIR)
	@echo "$(GREEN)Compilation successful!$(NC)"
	@echo "Output files in: $(BUILD_DIR)/"

verbose: check-core
	$(ARDUINO_CLI) compile \
		--fqbn $(FQBN) \
		--verbose \
		--build-path $(BUILD_DIR) \
		$(SKETCH_DIR)

clean:
	@echo "$(YELLOW)Cleaning build directory...$(NC)"
	rm -rf $(BUILD_DIR)
	@echo "$(GREEN)Clean complete!$(NC)"

# Show information about the board
info: check-cli
	$(ARDUINO_CLI) board details $(FQBN)

# List all connected boards
list-ports:
	$(ARDUINO_CLI) board list

# Upload to connected board (assumes board is connected)
upload: compile
	@echo "$(GREEN)Uploading to board...$(NC)"
	$(ARDUINO_CLI) upload \
		--fqbn $(FQBN) \
		--port $(PORT) \
		--input-dir $(BUILD_DIR) \
		$(SKETCH_DIR)

# Upload with specific port (usage: make upload-port PORT=/dev/ttyUSB0)
upload-port:
	@if [ -z "$(PORT)" ]; then \
		echo "$(RED)Error: Please specify PORT. Example: make upload-port PORT=/dev/ttyUSB0$(NC)"; \
		exit 1; \
	fi
	$(MAKE) upload PORT=$(PORT)

# Monitor serial output (usage: make monitor PORT=/dev/ttyUSB0)
monitor:
	@if [ -z "$(PORT)" ]; then \
		echo "$(RED)Error: Please specify PORT. Example: make monitor PORT=/dev/ttyUSB0$(NC)"; \
		exit 1; \
	fi
	$(ARDUINO_CLI) monitor -p $(PORT)

# Quick compile and upload with auto-detected port
run: compile
	@PORT=$$($(ARDUINO_CLI) board list | grep "esp32" | awk '{print $$1}'); \
	if [ -n "$$PORT" ]; then \
		echo "$(GREEN)Found ESP32 on $$PORT. Uploading...$(NC)"; \
		$(ARDUINO_CLI) upload --fqbn $(FQBN) --port $$PORT --input-dir $(BUILD_DIR) $(SKETCH_DIR); \
	else \
		echo "$(RED)No ESP32 board found. Please connect your board.$(NC)"; \
		exit 1; \
	fi

# Install arduino-cli (if not installed)
install-cli:
	@echo "$(YELLOW)Installing arduino-cli...$(NC)"
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
	@echo "$(GREEN)arduino-cli installed!$(NC)"
	@echo "Make sure to add it to your PATH or use ./arduino-cli"

# Update core indices
update:
	$(ARDUINO_CLI) core update-index

# List installed cores
list-cores:
	$(ARDUINO_CLI) core list

# Help target
help:
	@echo "$(GREEN)Available targets:$(NC)"
	@echo "  make          - Compile the sketch"
	@echo "  make compile  - Compile the sketch"
	@echo "  make verbose  - Compile with verbose output"
	@echo "  make clean    - Remove build directory"
	@echo "  make info     - Show board information"
	@echo "  make list-ports - List connected boards"
	@echo "  make upload PORT=/dev/ttyUSB0 - Upload to specific port"
	@echo "  make monitor PORT=/dev/ttyUSB0 - Monitor serial output"
	@echo "  make run      - Compile and upload to first found ESP32"
	@echo "  make update   - Update core indices"
	@echo "  make list-cores - List installed cores"
	@echo "  make install-cli - Install arduino-cli"

.PHONY: all compile verbose clean info list-ports upload upload-port monitor run update list-cores install-cli help check-cli check-core
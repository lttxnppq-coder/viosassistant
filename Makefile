# ============================================================
# ViosAssistant - ESP32-S3 N16R8 Makefile (Windows)
# Uses makeEspArduino (https://github.com/plerup/makeEspArduino)
# Board: ESP32S3 Dev Module (esp32s3)
# ============================================================

PROJECT_NAME = ViosAssistant
SKETCH = ViosAssistant.ino

# --- ESP32 Arduino Core Path (REQUIRED - user must set) ---
# Default: Arduino CLI package location
# Example: make ESP_ROOT="C:/Users/hi/AppData/Local/Arduino15/packages/esp32/hardware/esp32/3.3.11"
ESP_ROOT ?= 

# Normalize path
ESP_ROOT := $(subst \,/,$(ESP_ROOT))

# --- Board Configuration (ESP32-S3 N16R8) ---
# Based on ESP32S3 Dev Module (esp32s3)
# Note: ESP32 Arduino core uses 'esp32' as core directory for all variants
CHIP = esp32
BOARD = esp32s3

# Flash configuration for ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM OPI)
FLASH_SIZE = 16MB
FLASH_MODE = qio
FLASH_FREQ = 80m

# PSRAM configuration for ESP32-S3 with 8MB OPI PSRAM
PSRAM_TYPE = opi

# Partition scheme for 16MB flash
PARTITIONS = default_16MB

# PSRAM enable
EXTRA_CXXFLAGS += -DBOARD_HAS_PSRAM

# CH343P is the primary upload/debug interface (GPIO43/44)
# Do NOT enable ARDUINO_USB_CDC_ON_BOOT - CH343P is the primary interface

# --- Local Libraries (REQUIRED in lib/) ---
LIBS = lib/Adafruit_GFX lib/Adafruit_SSD1306 lib/Adafruit_BusIO

# --- Project Include Directories ---
INCLUDE_DIRS = config model drivers services application rtos utils

# --- Build Output ---
BUILD_DIR = build

# --- Upload Port (REQUIRED for flash/monitor) ---
# Example: make flash UPLOAD_PORT=COM5
UPLOAD_PORT ?= 

# --- makeEspArduino (REQUIRED) ---
MAKE_ESP_ARDUINO = makeEspArduino/makeEspArduino.mk

# ============================================================
# Validation (fail fast with clear instructions)
# ============================================================

ifeq ($(ESP_ROOT),)
$(error ESP_ROOT not set. Run: make ESP_ROOT="C:/path/to/esp32-arduino")
endif

ifeq ($(wildcard $(MAKE_ESP_ARDUINO)),)
$(error makeEspArduino not found. Run: git clone https://github.com/plerup/makeEspArduino.git makeEspArduino)
endif

MISSING_LIBS = $(filter-out $(wildcard $(addprefix lib/,Adafruit_GFX Adafruit_SSD1306 Adafruit_BusIO)),$(LIBS))
ifneq ($(MISSING_LIBS),)
$(error Missing libraries in lib/: $(MISSING_LIBS). Download pinned versions and extract to lib/)
endif

include $(MAKE_ESP_ARDUINO)

# ============================================================
# Custom Targets (Windows/PowerShell compatible)
# ============================================================

.PHONY: all clean flash monitor help deps-check

all: $(BUILD_DIR)/$(PROJECT_NAME).bin

deps-check:
	@echo ESP_ROOT: $(ESP_ROOT)
	@echo makeEspArduino: $(MAKE_ESP_ARDUINO)
	@echo Libraries: $(LIBS)
	@echo UPLOAD_PORT: $(UPLOAD_PORT)
	@echo CHIP: $(CHIP)
	@echo BOARD: $(BOARD)
	@echo FLASH_SIZE: $(FLASH_SIZE)
	@echo FLASH_MODE: $(FLASH_MODE)
	@echo PSRAM_TYPE: $(PSRAM_TYPE)
	@echo PARTITIONS: $(PARTITIONS)
	@echo All dependencies OK

clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR) 2>nul || echo Build dir already clean

flash: all
	@if "$(UPLOAD_PORT)"=="" (echo Error: UPLOAD_PORT required. Use: make flash UPLOAD_PORT=COM5 && exit /b 1)
	esptool.py --chip esp32s3 --port $(UPLOAD_PORT) --baud 921600 write_flash -z 0x0 $(BUILD_DIR)/$(PROJECT_NAME).bin

monitor:
	@if "$(UPLOAD_PORT)"=="" (echo Error: UPLOAD_PORT required. Use: make monitor UPLOAD_PORT=COM5 && exit /b 1)
	@where arduino-cli >nul 2>nul && (arduino-cli monitor -p $(UPLOAD_PORT) -c baudrate=115200) || (echo Arduino CLI not in PATH. Use PuTTY/TeraTerm on $(UPLOAD_PORT) @ 115200)

help:
	@echo Targets:
	@echo   make                            - Build project (requires ESP_ROOT)
	@echo   make flash UPLOAD_PORT=COM5     - Build and flash
	@echo   make monitor UPLOAD_PORT=COM5   - Serial monitor
	@echo   make clean                      - Clean build directory
	@echo   make deps-check                 - Verify all dependencies
	@echo   make help                       - Show this help
	@echo.
	@echo Required variables:
	@echo   ESP_ROOT="C:/path/to/esp32-arduino"
	@echo   UPLOAD_PORT=COM5      (for flash/monitor)
	@echo.
	@echo Board: ESP32-S3 Dev Module (esp32s3)
	@echo Flash: 16MB QIO 80MHz
	@echo PSRAM: 8MB OPI enabled
	@echo Partition: default_16MB.csv
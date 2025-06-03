#################################################
# Project Setup
#################################################
TARGET = SmartHome
MCU = atmega328p
F_CPU = 16000000UL
BAUD = 9600UL

SOURCE_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

SOURCES= $(wildcard $(SOURCE_DIR)/*.c)
HEADERS= $(addprefix $(INCLUDE_DIR)/,$(notdir $(SOURCES:.c=.h)))
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SOURCES:.c=.o)))

TARGET_ARCH = -mmcu=$(MCU)


#################################################
# Programmer Setup
#################################################
PROGRAMMER_TYPE = arduino
PROGRAMMER_ARGS = -P /dev/ttyACM0 # passed to avrdude


#################################################
# Toolchain Setup
#################################################
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AVRSIZE = avr-size
AVRDUDE = avrdude


#################################################
# Compiler & Linker Options
#################################################
CPPFLAGS = -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -I.
CFLAGS = -Os -g -std=gnu99 -Wall
# Use short (8-bit) data types 
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums 
# Splits up object files per function
CFLAGS += -ffunction-sections -fdata-sections 
LDFLAGS = -Wl,-Map,$(BUILD_DIR)/$(TARGET).map 
# Optional, but often ends up with smaller code
LDFLAGS += -Wl,--gc-sections 
# Relax shrinks code even more, but makes disassembly messy
# LDFLAGS += -Wl,--relax
# LDFLAGS += -Wl,-u,vfprintf -lprintf_flt -lm  # for floating-point printf
# LDFLAGS += -Wl,-u,vfprintf -lprintf_min      # for smaller printf


#################################################
# Rules
#################################################
# Make .o files from .c files 
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS) Makefile
	 mkdir -p $(BUILD_DIR)
	 $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	 $(OBJCOPY) -j .text -j .data -O ihex $< $@

$(BUILD_DIR)/%.eeprom: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ 

$(BUILD_DIR)/%.lst: $(BUILD_DIR)/%.elf
	$(OBJDUMP) -S $< > $@abi


#################################################
# Make Commands
#################################################
# These targets don't have files named after them
.PHONY: all disassemble disasm eeprom size clean squeaky_clean flash fuses

all: $(BUILD_DIR)/$(TARGET).hex 

debug:
	@echo
	@echo "Source files:"   $(SOURCES)
	@echo "Header files:"   $(HEADERS)
	@echo "Object files:"   $(OBJECTS)
	@echo "MCU, F_CPU, BAUD:"  $(MCU), $(F_CPU), $(BAUD)
	@echo	

# Optionally create listing file from .elf
# This creates approximate assembly-language equivalent of your code.
# Useful for debugging time-sensitive bits, 
# or making sure the compiler does what you want.
disassemble: $(BUILD_DIR)/$(TARGET).lst

disasm: disassemble

# Optionally show how big the resulting program is 
size:  $(BUILD_DIR)/$(TARGET).elf
	$(AVRSIZE) -C --mcu=$(MCU) $(BUILD_DIR)/$(TARGET).elf

clean:
	rm -rf $(BUILD_DIR)

flash: $(BUILD_DIR)/$(TARGET).hex 
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) $(PROGRAMMER_ARGS) -U flash:w:$<

# An alias
program: flash

flash_eeprom: $(BUILD_DIR)/$(TARGET).eeprom
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) $(PROGRAMMER_ARGS) -U eeprom:w:$<

avrdude_terminal:
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) $(PROGRAMMER_ARGS) -nt


#################################################
# Fuse Settings
#################################################
## Mega 48, 88, 168, 328 default values
LFUSE = 0x62
HFUSE = 0xdf
EFUSE = 0x00

## Generic 
FUSE_STRING = -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m 

fuses: 
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) \
	           $(PROGRAMMER_ARGS) $(FUSE_STRING)
show_fuses:
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) $(PROGRAMMER_ARGS) -nv	

## Called with no extra definitions, sets to defaults
set_default_fuses:  FUSE_STRING = -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m 
set_default_fuses:  fuses

## Set the fuse byte for full-speed mode
## Note: can also be set in firmware for modern chips
set_fast_fuse: LFUSE = 0xE2
set_fast_fuse: FUSE_STRING = -U lfuse:w:$(LFUSE):m 
set_fast_fuse: fuses

## Set the EESAVE fuse byte to preserve EEPROM across flashes
set_eeprom_save_fuse: HFUSE = 0xD7
set_eeprom_save_fuse: FUSE_STRING = -U hfuse:w:$(HFUSE):m
set_eeprom_save_fuse: fuses

## Clear the EESAVE fuse byte
clear_eeprom_save_fuse: FUSE_STRING = -U hfuse:w:$(HFUSE):m
clear_eeprom_save_fuse: fuses

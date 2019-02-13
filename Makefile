###############################################################################
# Makefile for the project real_canopen
###############################################################################

## General Flags
PROJECT = real_canopen
MCU = at90can128
TARGET = AVR
CC = avr-gcc
SRC = src
CAN_SRC = canfestival/src
DRV = canfestival/drivers/AVR
ARDUINO_ROOT = C:/arduino
COM_PORT = com6

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -Os -fsigned-char -fpack-struct
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=$(PROJECT).map

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Include Directories
INCLUDES = -I"canfestival/include" -I"canfestival/include/AVR" -I"src"

## Objects that must be built in order to link
OBJECTS = 	$(DRV)/can_AVR.o\
		$(DRV)/timer_AVR.o\
		$(CAN_SRC)/dcf.o\
		$(CAN_SRC)/timer.o\
		$(CAN_SRC)/emcy.o\
		$(CAN_SRC)/lifegrd.o\
		$(CAN_SRC)/lss.o\
		$(CAN_SRC)/nmtMaster.o\
		$(CAN_SRC)/nmtSlave.o\
		$(CAN_SRC)/objacces.o\
		$(CAN_SRC)/pdo.o\
		$(CAN_SRC)/sdo.o\
		$(CAN_SRC)/states.o\
		$(CAN_SRC)/sync.o\
		$(SRC)/real_objdict.o\
		$(SRC)/uart.o\
		$(SRC)/twi.o\
		$(SRC)/pwm.o\
		$(SRC)/main.o

## Build
all: $(PROJECT).elf $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss size

## Compile
%.o: %.c
#	@echo " "
	@echo "---------------------------------------------------------------------------"
	@echo "**Compiling $< -> $@"
#	@echo "*********************************************"
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<


##Link
$(PROJECT).elf: $(OBJECTS)
#	@echo " "
	@echo "---------------------------------------------------------------------------"
	@echo "**Linking :  $@"
#	@echo "*********************************************"
	$(CC) $(LDFLAGS) $(LIBDIRS) $(LIBS) $(OBJECTS) -o $@

%.hex: $(PROJECT).elf
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(PROJECT).elf
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(PROJECT).elf
	avr-objdump -h -S $< > $@

size: $(PROJECT).elf
	@echo
	@avr-size -C --mcu=${MCU} $(PROJECT).elf

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) $(PROJECT).elf dep/* $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss $(PROJECT).map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

upload:
	$(ARDUINO_ROOT)\hardware\tools\avr\bin\avrdude.exe -p c128 -C $(ARDUINO_ROOT)\hardware\tools\avr\etc\avrdude.conf -c arduino -P $(COM_PORT) -Uflash:w:"$(PROJECT).hex":i

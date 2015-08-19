CROSS_COMPILE?=
OBJC:=video_file.c collect_camera_data.c trans_data.c main.c
LDS:=
TARGET:=target

AS	= $(CROSS_COMPILE)as
LD	= $(CROSS_COMPILE)ld
CC	= $(CROSS_COMPILE)gcc
CPP = $(CC) -E
AR	= $(CROSS_COMPILE)ar
NM	= $(CROSS_COMPILE)nm
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY	= $(CROSS_COMPILE)objcopy
OBJDUMP	= $(CROSS_COMPILE)objdump
a_flags =
dump_file=dump.txt

all:$(TARGET)
$(TARGET):$(OBJC)
	$(CC) $(a_flags) $^ -o $@ 
	#$(LD) -Tcwm.lds $(OBJC:.c=.o) -o $@
	
	echo $(CURDIR)

dump:
	$(OBJDUMP) -Dxz $(TARGET) > $(dump_file) 
clean:
	rm -rf $(OBJC:.c=.o) $(TARGET) $(TARGET:=.dump) *.swp $(dump_file)

.PHONY:clean

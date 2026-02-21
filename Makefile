# Makefile — MSYS2 MinGW64 için
# Kullanım: cd /c/RtlSdr/radar && make

CC      = gcc
TARGET  = radar.exe

SRCDIR  = src
INCDIR  = include

SRCS    = $(SRCDIR)/main.c     \
          $(SRCDIR)/fft.c      \
          $(SRCDIR)/sdr.c      \
          $(SRCDIR)/recorder.c \
          $(SRCDIR)/render.c   \
          $(SRCDIR)/widgets.c  \
          $(SRCDIR)/panel.c

OBJS    = $(SRCS:.c=.o)

CFLAGS  = -Wall -Wextra -O2 -I$(INCDIR)
LIBS    = -lrtlsdr -lSDL2 -lSDL2_ttf -lm

# Windows: console penceresi açık kalsın (hata mesajları için)
# -mwindows eklerseniz konsol gizlenir (release için uygundur)
# LIBS += -mwindows

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)
	@echo ">>> Derleme tamamlandi: $(TARGET)"

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)

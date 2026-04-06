CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
SRCDIR  = src
SRCS    = $(SRCDIR)/main.c \
          $(SRCDIR)/csv.c \
          $(SRCDIR)/tabela_hash.c \
          $(SRCDIR)/busca.c \
          $(SRCDIR)/busca_sequencial.c \
          $(SRCDIR)/busca_interpolacao.c \
          $(SRCDIR)/servidor.c

# ---------------------------------------------------------------
# Detecta o ambiente e define compilador/flags adequados
# ---------------------------------------------------------------

# MinGW no Windows (via PATH do Windows)
MINGW_GCC ?= gcc.exe

ifeq ($(OS),Windows_NT)
    # Compilando no Windows (MinGW / cmd / PowerShell)
    TARGET  = busca_combustivel.exe
    LDFLAGS = -lws2_32
    COMPILER = $(MINGW_GCC)
else
    # Linux / macOS / WSL com gcc nativo
    TARGET   = busca_combustivel
    LDFLAGS  =
    COMPILER = $(CC)
endif

# ---------------------------------------------------------------
# Regras
# ---------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(SRCS)
	$(COMPILER) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

# Atalho para compilar com MinGW dentro do WSL
mingw: $(SRCS)
	/mnt/c/Users/VICTO/AppData/Local/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin/gcc.exe \
	    $(CFLAGS) -o busca_combustivel.exe $(SRCS) -lws2_32

clean:
	rm -f busca_combustivel busca_combustivel.exe

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run mingw

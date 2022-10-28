
include =	-I./inc

src = 	$(shell find . -name "*.c")	\
		$(shell find . -name "*.cpp")
		
BIN = app
all: $(BIN)

INSTALL_BIN_PATH = ./output/

CC = mips-linux-gnu-gcc
CXX = mips-linux-gnu-g++

all = $(BIN)
obj = $(src:.c=.o)
objs = $(obj:.cpp=.o)

$(BIN): $(objs)
	$(CXX) -o $@ $^ -lpthread
	install -D -m 0755 $(BIN) $(INSTALL_BIN_PATH)

%.o:%.c
	$(CC) -c -o $@ $< $(include)
%.o:%.cpp
	$(CXX) -c -o $@ $< $(include)

clean:
	rm $(BIN) $(objs) -vf

prin:
	@echo $(objs)


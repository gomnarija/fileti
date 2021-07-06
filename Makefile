CC =gcc
CFLAGS=-Wall -g -pthread 

BIN=fileti



#parent dirs
SRC_DIR=src
OBJ_DIR=obj

#sub dirs
SRC_DIRS = $(SRC_DIR) src/utils src/ftp 
OBJ_DIRS = $(patsubst $(SRC_DIR)%,$(OBJ_DIR)%,$(SRC_DIRS))

SRC  = $(shell find $(SRC_DIRS) -maxdepth 1 -name '*.c')
OBJ  = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))


all: obj_dir $(BIN)

$(BIN):$(OBJ)
	$(CC) $(CFLAGS) $^ -o $(BIN) 

$(OBJ):$(OBJ_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


obj_dir: $(OBJ_DIRS)



$(OBJ_DIRS):
	mkdir -p $(OBJ_DIRS)

clean:clean_obj
	rm $(BIN)

clean_obj:
	rm -r $(OBJ_DIR)




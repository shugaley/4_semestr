CC = gcc
LD = gcc

DIR_LIB   = lib
DIR_EXEC  = out
DIR_BUILD = build
DIR_BUILD_WITH_TEST = $(DIR_BUILD)/test


LIB              = $(addprefix $(DIR_LIB)/,libRbTree.a)
FLAGS            = -MD -g -Wall -fprofile-arcs -ftest-coverage
FLAGS_WITH_TEST  = $(FLAGS)
CFLAGS           = $(FLAGS)
CFLAGS_WITH_TEST = $(FLAGS_WITH_TEST)
INC              = -I$(DIR_INC)
DEF 			 = -DNDEBUG
DEF_WITH_TEST    = -DTEST
DIR_INC          = include
DIR_SRC          = src
OBJ              = $(addprefix $(DIR_BUILD)/,$(OBJNAME))
OBJ_WITH_TEST    = $(addprefix $(DIR_BUILD_WITH_TEST)/,$(OBJNAME))
OBJNAME          = RbTree.o


EXEC_TEST    = $(addprefix $(DIR_EXEC)/,test.out)
LIB_TEST     = $(addprefix $(DIR_LIB)/,libRbTreeTest.a)
FLAGS_TEST   = $(FLAGS)
CFLAGS_TEST  = $(FLAGS_TEST)
LFLAGS_TEST  = -lgcov --coverage
INC_TEST     = $(INC)
DIR_SRC_TEST = test
OBJ_TEST     = $(addprefix $(DIR_BUILD)/,main.o RbTreeUnitTest.o)



GCOV_FLAGS = -o $(DIR_BUILD_WITH_TEST) -n

# ------------------------------------------------------------------------------

.DELETE_ON_ERROR:

all: lib

# ------------------------------------------------------------------------------

.PHONY: lib
lib: $(LIB)

$(LIB): $(OBJ)
	ar rcs $@ $^

$(OBJ): | dirs

# ------------------------------------------------------------------------------

.PHONY: test
test: $(EXEC_TEST)

$(EXEC_TEST) : $(OBJ_TEST) $(LIB_TEST)
	$(CC) $(LFLAGS_TEST) -o $@ $^

$(LIB_TEST): $(OBJ_WITH_TEST)
	ar rcs $@ $^

$(OBJ_WITH_TEST) : | dirs
$(OBJ_TEST) : | dirs

# ------------------------------------------------------------------------------

$(DIR_BUILD)/%.o : $(DIR_SRC)/%.c
	$(CC) -c $(CFLAGS) $(INC) $(DEF) -o $@ $<

$(DIR_BUILD_WITH_TEST)/%.o : $(DIR_SRC)/%.c
	$(CC) -c $(CFLAGS_WITH_TEST) $(INC) $(DEF_WITH_TEST) -o $@ $<

$(DIR_BUILD)/%.o : $(DIR_SRC_TEST)/%.c
	$(CC) -c $(CFLAGS_TEST) $(INC_TEST) -o $@ $<

-include $(OBJ:.o=.d) $(OBJ_WITH_TEST:.o=.d) $(OBJ_TEST:.o=.d)

# ------------------------------------------------------------------------------

.PHONY: dirs
dirs:
	mkdir -p $(DIR_BUILD) $(DIR_BUILD_WITH_TEST) $(DIR_LIB) $(DIR_EXEC)

# ------------------------------------------------------------------------------

.PHONY: coverage
coverage : $(DIR_SRC)/*.c run_test
	 gcov $< $(GCOV_FLAGS)

.PHONY: run_test
run_test: $(EXEC_TEST)
	./$(EXEC_TEST)

# ------------------------------------------------------------------------------

.PHONY: clean
clean:
	rm -rf $(DIR_BUILD) $(DIR_BUILD_WITH_TEST) $(DIR_LIB) $(DIR_EXEC)
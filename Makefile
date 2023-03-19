CC   := g++
NAME := simd
ARGS :=

LOGFILE := compileLog

AVX2CFLAGS := -mavx2

CFLAGS :=  $(AVX2CFLAGS) -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie -fPIE
SANITIZERS := #-fsanitize=address,leak #,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr -Wstack-protector  -Wuseless-cast-Wpedantic
LFLAGS_SANITIZER := -lpthread -lm -lsfml-system -lsfml-window -lsfml-graphics #-lasan
LFLAGS           := -lpthread -lm -lsfml-system -lsfml-window -lsfml-graphics

SRCDIR := src
OBJDIR := objects
INCDIR := include
DEPDIR := dependences

SOURCES     := $(wildcard $(addsuffix /*.cpp, $(if $(SRCDIR), $(SRCDIR), .)) )
OBJECTS     := $(patsubst %.cpp, $(if $(OBJDIR), $(OBJDIR)/%.o, ./%.o), $(notdir $(SOURCES)) )
DEPENDENCES := $(patsubst %.cpp, $(if $(DEPDIR), $(DEPDIR)/%.d, ./%.d), $(notdir $(SOURCES)) )

VPATH := $(SRCDIR)

.PHONY: clean run  dependences cleanDependences makeDependencesDi1r objects turnOffSanitizer

$(NAME):  dependences objects $(OBJECTS) cleanDependences
	@$(if $(OBJECTS), $(CC) $(OBJECTS) $(LFLAGS_SANITIZER) -o $@ #2>>$(LOGFILE))

clean:
	@rm -rf $(OBJECTS) $(DEPENDENCES) $(DEPDIR) $(NAME)

turnOffSanitizer: SANITIZERS :=
turnOffSanitizer: LFLAGS_SANITIZER := $(LFLAGS)

check: clean turnOffSanitizer $(NAME)
	@$(if $(NAME), valgrind --tool=callgrind --callgrind-out-file=$(NAME).log ./$(NAME) $(ARGS))
	@kcachegrind $(NAME).log
# --leak-check=full --show-leak-kinds=all

run: clean $(NAME)
	@$(if $(NAME), ./$(NAME) $(ARGS))

dependences: makeDependencesDir $(DEPENDENCES)

makeDependencesDir:
	@$(if $(DEPDIR), mkdir -p $(DEPDIR))

$(if $(DEPDIR), $(DEPDIR)/%.d, %.d): %.cpp
	@$(CC) -M $(addprefix -I, $(INCDIR)) $< -o $@ #2>>$(LOGFILE)

cleanDependences:
	@rm -rf $(DEPENDENCES) $(DEPDIR)

objects:
	@$(if $(OBJDIR), mkdir -p $(OBJDIR))

$(if $(OBJDIR), $(OBJDIR)/%.o, %.o): %.cpp
	@$(CC) -c $(addprefix -I, $(INCDIR)) -save-temps $(CFLAGS) $(SANITIZERS) $< -o $@ #2>>$(LOGFILE)

include $(wildcard $(DEPDIR)/*.d)

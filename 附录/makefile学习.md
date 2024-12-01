# Makefile学习笔记
Makefile 是用于控制编译流程的文件，特别适合大型项目的自动化管理。通过 Makefile，可以指定文件依赖关系和构建规则，使得编译更加高效和清晰。  
## 一、基本语法
```makefile
target: prerequisites
    command
```  
- target: 目标文件：可执行文件、目标文件或者伪目标(clean)。  
- prerequisites: 依赖文件列表。  
- command: 生成目标文件的命令(前面要以TAB开头)。  
示例：  
```makefile
# Makefile 示例
hello: hello.o
    gcc -o hello hello.o

hello.o: hello.c
    gcc -c hello.c
```  
## 二、特性详解
### 1、变量
- 变量定义  
```makefile
CC = gcc
CFLAGS = -Wall -g
```  
- 变量使用  
```makefile
all: program.c
    $(CC) $(CFLAGS) -o program program.c
```  
### 2、自动化变量
- $@: 当前目标文件的名称；  
- $^: 所有的依赖文件；  
- $<: 第一个依赖文件；  
- $?: 相较于目标文件更新的文件；  
示例：  
```makefile
program: program.o
    gcc -o $@ $^
```   
### 3、模式规则
用于处理大量相似文件的规则：   
```makefile
%.o: %.c
    gcc -c $< -o $@
```   
- 规则表示所有 .c 文件编译为对应的 .o 文件。  
- $< 是当前规则的依赖文件（即 .c 文件）。  
- $@ 是当前规则的目标文件（即 .o 文件）。  
### 4、函数
makefile内置函数：  
- wildcard：匹配文件：  
```makefile
SRC = $(wildcard *.c)
```   
- patsubst：模式替换：  
```makefile
OBJ = $(patsubst %.c, %.o, $(SRC))
```   
- addprefix 和 addsuffix：添加前缀和后缀：  
```makefile
DIRS = src include
INCLUDES = $(addprefix -I, $(DIRS))
```   
### 5、伪目标
用于定义不生成目标文件的目标，如：清理临时文件等。常用 .PHONEY 进行指定。  
```makefile
.PHONY: clean
clean:
    rm -f *.o program
```   
### 6、多目标规则
```makefile
all: program1 program2

program1: program1.o
    gcc -o program1 program1.o

program2: program2.o
    gcc -o program2 program2.o
    rm -f *.o program
```   
执行 make all 时，会同时编译 program1 和 program2。  
## 三、进阶用法
### 1、自动化依赖管理
通过 gcc -M 生成依赖文件：
```makefile
%.d: %.c
    gcc -M $< > $@
```   
包含依赖文件：  
```makefile
-include $(wildcard *.d)
```   
### 2、内置规则和隐式变量
- 内置规则：  
.c -> .o:  $(CC) -c $(CFLAGS)  

- 隐式变量（可以覆盖这些变量）：  
CC: 编译器（默认是CC）    
CFLAGS: 编译选项。  
### 3、环境变量
Makefile 可以使用shell环境变量  
```makefile
all:
    echo $(HOME)
```     
### 4、条件语句
根据条件切换规则或变量：  
```makefile
ifeq ($(DEBUG), 1)
CFLAGS += -g
else
CFLAGS += -O2
endif
```  
## 四、常用命令
- make：根据默认目标执行规则。  
- make <target>：指定目标。  
- make -n：显示规则执行的命令，但不实际执行。  
- make -j N：并行执行规则，N 是并发任务数。  

## 五、完整示例
```makefile
CC = gcc
CFLAGS = -Wall -g
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
TARGET = program

all: $(TARGET)

$(TARGET): $(OBJ)
    $(CC) -o $@ $^

%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
    rm -f $(OBJ) $(TARGET)
```    

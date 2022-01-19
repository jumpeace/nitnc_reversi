#
#	Makefile
#

#	最終目的のファイル
TARGET = main.exe

#	ソースファイル(*.c)の一覧
SRCS = main.c

#	オブジェクトファイル(*.o)の一覧
OBJS = $(SRCS:.c=.o)

#	ヘッダファイルの一覧
HEADERS =

#	コンパイラ・リンカの指定
CC = gcc
CCFLAGS = -Wall -I/usr/include/opengl
LD = gcc
LDFLAGS =
LIBS = -lglpng -lglut -lGLU -lGL -lm

#	OBJSからTARGETを作る方法
$(TARGET) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

.c.o :
	$(CC) $(CCFLAGS) -c $(LIBS) $<

#	*.o は HEADERS と Makefile に依存(これらが書き換わったときにも*.oを再構築)
$(OBJS) : $(HEADERS) Makefile

#	make cleanとしたときに実行されるコマンド
clean :
	rm -f $(OBJS) $(ICONOBJ)

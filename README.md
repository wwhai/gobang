# C写的五子棋
计划写一个能实现落子评估打分的工具，用来和别人下棋的时候作弊。但是因为时间问题搁置了，就烂了吧。

# 注意事项
Windows下SDL2有个坑，就是`WinMain`函数入口的问题，需要在链接器参数里面加上这行：
```bash
LINKER_FLAGS = -lSDL2main -lSDL2
```

同时在main函数的第一行加上这个宏:
```c
#define SDL_MAIN_HANDLED
```
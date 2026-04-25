#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include <stddef.h>
#include <stdint.h>

// 注册一个类型为 type 的对象池需求
// 每个调用该宏的 .c 文件会增加一个占位符，链接器将统计总数
//#define REGISTER_OBJECT_POOL(type) \
    static char __obj_pool_##type##_##__LINE__ __attribute__((used, section(".object_pool." #type)))
#define REGISTER_OBJECT_POOL(type) \
    static char __obj_pool_##type##_##__LINE__[sizeof(type)] __attribute__((used, section(".object_pool_" #type)))
// 获取类型 type 的对象池中对象的最大数量（编译时确定）
#define GET_OBJECT_POOL_SIZE(type) \
    ( (size_t)( (uintptr_t)&__stop_object_pool_##type - (uintptr_t)&__start_object_pool_##type ) )

// 声明外部链接器符号（由链接器自动提供）
#define DECLARE_OBJECT_POOL_SYMBOLS(type) \
    extern char __start_object_pool_##type[]; \
    extern char __stop_object_pool_##type[]

// 简单的静态内存池实现（固定块大小）
typedef struct pool_block {
    struct pool_block *next;
} pool_block_t;
// 初始化对象池：需要外部提供 mem_pool_t 结构和内存池管理函数
typedef struct mem_pool {
    //void *next;
    pool_block_t *free_list;
    size_t block_size;
} mem_pool_t;

void mem_pool_init(mem_pool_t *pool, void *buffer, size_t buffer_size, size_t block_size);
void *mem_pool_alloc(mem_pool_t *pool);
void mem_pool_free(mem_pool_t *pool, void *block);

// 初始化类型 type 的对象池
#define INIT_OBJECT_POOL(type, pool_var) \
    do { \
        DECLARE_OBJECT_POOL_SYMBOLS(type); \
        size_t pool_size = GET_OBJECT_POOL_SIZE(type); \
        mem_pool_init(&(pool_var), __start_object_pool_##type, pool_size, sizeof(type)); \
    } while(0)
#define DEC_INIT_POOL(type) \
void pool_##type##_init(void)

#define DEF_INIT_POOL(type) \
static mem_pool_t type##_pool; \
void pool_##type##_init(void) { \
        INIT_OBJECT_POOL(type, type##_pool); \
}

#define INV_INIT_POOL(type) \
pool_##type##_init()


#endif // OBJECT_POOL_H
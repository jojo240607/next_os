#include "linear_pool.h"
#include "util.h"

static void noram_error();

static linear_allocator_t gloable_alloc = {0};  //普通对象内存池
static linear_allocator_t gloable_ccm_alloc = {0};//ccm内存池

linear_allocator_t *linear_pool_get() {
    return &gloable_alloc;
}
// 初始化：buffer 可以是静态数组或 malloc 的动态内存
static void linear_allocator_init(linear_allocator_t *alloc) {
    static uint8_t buffer[DEFAULT_OBJBUF_SIZE];
    alloc->start = (uint8_t*)buffer;
    alloc->next = alloc->start;
    alloc->size = DEFAULT_OBJBUF_SIZE;
}

static void linear_ccm_allocator_init(linear_allocator_t *alloc) {
    CCMRAM static uint8_t buffer[DEFAULT_CCMRAM_SIZE];
    alloc->start = (uint8_t*)buffer;
    alloc->next = alloc->start;
    alloc->size = DEFAULT_CCMRAM_SIZE;
}

// 分配一块内存（不对齐，简单版）
static void* linear_alloc(linear_allocator_t *alloc, size_t num_bytes) {
    if (num_bytes > alloc->size) {
        noram_error();//没有内存里进入到死循环
        return NULL;   // 内存不足
    }
    void *ptr = alloc->next;
    alloc->next += num_bytes;
    alloc->size -= num_bytes;
    return ptr;
}

// 整体重置（释放所有分配的对象）
static void linear_allocator_reset(linear_allocator_t *alloc) {
    alloc->next = alloc->start;
}

// 获取已用空间大小（可选）
static size_t linear_allocator_used(linear_allocator_t *alloc) {
    return alloc->next - alloc->start;
}
static void noram_error() {
    while (1);
}
void os_pool_init() {
    linear_allocator_init(&gloable_alloc);
    linear_ccm_allocator_init(&gloable_ccm_alloc);
}
void *os_malloc(size_t num_bytes) {
    return linear_alloc(&gloable_alloc, num_bytes);
}

void *os_ccm_malloc(size_t num_bytes) {
    return linear_alloc(&gloable_ccm_alloc, num_bytes);
}

void os_free (void *p) {

}
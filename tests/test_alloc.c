#include "alloc.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <err.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <unistd.h>
#include <stdint.h>

// Internal declarations for White-Box testing
struct block {
    size_t  size;
    struct block *next;
    struct block *prev;
    int free;
    int padding;
    char anchor[1];
};
typedef struct block *meta_block;
extern meta_block base;
size_t align_64b(ssize_t x);
meta_block find_block(meta_block *last, size_t size);
meta_block extend_heap(meta_block last, size_t new_size);
void split_block(meta_block b, size_t new_size);
meta_block fusion(meta_block block, int ok);
meta_block get_pointer_to_meta_block(void *ptr);
int valid_addr(void *p);
void reset_heap();


void test_align_zero(void) {
    CU_ASSERT_EQUAL(align_64b(0), 0);

}

void test_align_prime(void) {
    CU_ASSERT_EQUAL(align_64b(19), 24);
}

void test_align_negative(void) {
    CU_ASSERT_EQUAL(align_64b(-31), 0);
}

void test_align_big_range(void) {
    CU_ASSERT_EQUAL(align_64b(-65535), 0);
    CU_ASSERT_EQUAL(align_64b(65535), 65536);
}

void test_find_block_base(void) {

    base = extend_heap(NULL, 16);
    base->free = 1;
    meta_block last = NULL;
    CU_ASSERT_EQUAL(find_block(&last, 8), base);
}

void test_find_second_block(void) {
    base = extend_heap(NULL, 8);
    base->free = 1;
    meta_block second_block = extend_heap(base, 27);
    second_block->free = 1;
    meta_block last = base;
    CU_ASSERT_EQUAL(find_block(&last, 24), second_block);
    CU_ASSERT_EQUAL(last, base);
}

void test_find_block_NULL(void) {
    meta_block last;
    base = NULL;
    CU_ASSERT_EQUAL(find_block(&last, 10), NULL);
}

void test_extend_heap_base(void) {
    base = extend_heap(NULL, 24);
    CU_ASSERT_NOT_EQUAL(base, NULL);
    CU_ASSERT_EQUAL(base->size, 24);
}

void test_extend_heap_large_size(void) {
    meta_block b1, b2, b3, b4;
    
    base = extend_heap(NULL, 1e10);
    b1 = extend_heap(base, 1e10);
    b2 = extend_heap(b1, 1e10);
    b3 = extend_heap(b2, 1e10);
    b4 = extend_heap(b3, 1e10);
    CU_ASSERT_NOT_EQUAL(b4, NULL);
}

void test_split_block_size(void) {
    base = extend_heap(NULL, 8);
    meta_block b1 = extend_heap(base, 64);
    split_block(b1, 16);
    CU_ASSERT_EQUAL(base->next->size, 16);
    CU_ASSERT_EQUAL(base->next->next->size, 16);    
}

void test_split_block_pointer(void) {
    base = extend_heap(NULL, 8);
    meta_block b1 = extend_heap(base, 64);
    split_block(b1, 16);
    CU_ASSERT_PTR_NOT_NULL(base->next);
    CU_ASSERT_PTR_NOT_NULL(base->next->next);
}

void test_fusion_2_blocks_fwd(void) {
    meta_block b1;
    base = extend_heap(NULL, 16);
    b1 = extend_heap(base, 16);
    base->free = 1;
    b1->free = 1;
    CU_ASSERT_EQUAL(fusion(base, 1), base);
    CU_ASSERT_EQUAL(base->size, 64);
}

void test_fusion_2_blocks_bck(void) {
    meta_block b1;
    base = extend_heap(NULL, 16);
    b1 = extend_heap(base, 16);
    base->free = 1;
    b1->free = 1;
    CU_ASSERT_EQUAL(fusion(b1, 1), base);
    CU_ASSERT_EQUAL(base->size, 64);
}

void test_fusion_4_blocks(void) {
    meta_block b1, b2, b3, b4, b5;
    base = extend_heap(NULL, 16);
    b1 = extend_heap(base, 16);
    b2 = extend_heap(b1, 16);
    b3 = extend_heap(b2, 16);
    b4 = extend_heap(b3, 16);
    b5 = extend_heap(b4, 16);
    b1->free = 1;
    b2->free = 1;
    b3->free = 1;
    b4->free = 1;
    CU_ASSERT_EQUAL(fusion(b3, 1), b1);
    CU_ASSERT_EQUAL(b1->size, 16*4 + offsetof(struct block, anchor)*3);
}

void test_get_pointer_to_meta_block(void) {
    base = extend_heap(NULL, 16);
    void *p = (char*)base + offsetof(struct block, anchor);
    CU_ASSERT_EQUAL(get_pointer_to_meta_block(p), base);
}

void test_valid_addr_yes(void) {
    base = extend_heap(NULL, 16);
    void *p = (char*)base + offsetof(struct block, anchor);
    CU_ASSERT_TRUE(valid_addr(p));
}

void test_valid_addr_no(void) {
    base = extend_heap(NULL, 16);
    void *p = (char*)base+3190;
    CU_ASSERT_FALSE(valid_addr(p));
    p = (char*)base-300;
    CU_ASSERT_FALSE(valid_addr(p));
}

void test_valid_addr_misalign(void) {
    base = extend_heap(NULL, 16);
    void *p = (char*)base + offsetof(struct block, anchor) + 1;
    CU_ASSERT_FALSE(valid_addr(p));
}

void test_my_free_mid(void) {
    void *a = my_malloc(10);
    void *b = my_malloc(12);
    void *c = my_malloc(18);
    my_free(b);
    meta_block b1 = get_pointer_to_meta_block(b);
    CU_ASSERT_TRUE(b1->free);
}

void test_my_free_end(void) {
    base = extend_heap(NULL, 16);
    meta_block b1 = extend_heap(base, 16);
    my_free((void*)b1->anchor);
    CU_ASSERT_EQUAL((void*)b1, sbrk(0));
}


void test_my_malloc_array(void) {
    int *ptr = (int *)my_malloc(20);
    for (int i = 0; i < 5; i++){
         ptr[i] = i + 1;
         CU_ASSERT_EQUAL(ptr[i], i + 1);
    }
}

// this works for numbers bigger than 1e4, but it takes a long time (e.g under 611.396 seconds for 1e5);
void test_my_malloc_volume(void) {
    int *ptr;
    for(int i = 0; i < 1e4; i++) { 
        ptr = (int *)my_malloc(i);
    }
}

void test_my_malloc_coalesce(void) {
    void *a = my_malloc(100);
    void *b = my_malloc(100);
    void *c = my_malloc(100);
    my_free(a);
    my_free(b);
    void *d = my_malloc(180); 
    CU_ASSERT_PTR_NOT_NULL(d);
    CU_ASSERT_TRUE(d < c); 
}

void test_my_malloc_split(void) {
    void *large = my_malloc(2000);
    void *end = my_malloc(1);
    my_free(large);  
    void *small = my_malloc(100);
    meta_block meta = get_pointer_to_meta_block(small);
    CU_ASSERT_EQUAL(meta->size, 104);
    CU_ASSERT_PTR_NOT_NULL(meta->next); 
}

void test_my_malloc_integrity(void) {
    char *str1 = my_malloc(16);
    strcpy(str1, "Hello");   
    void *garbage = my_malloc(1024); 
    CU_ASSERT_STRING_EQUAL(str1, "Hello");
}

void test_my_malloc_boundaries(void) {
    void *p1 = my_malloc(0);
    CU_ASSERT_PTR_NULL(p1); 
    void *p2 = my_malloc(SIZE_MAX); 
    CU_ASSERT_PTR_NULL(p2);
}

void test_my_calloc_array(void) {
    int *ptr = (int *)my_calloc(20, 4);
    for (int i = 0; i < 20; i++){
         ptr[i] = i + 1;
         CU_ASSERT_EQUAL(ptr[i], i + 1);
    }
}

// this works for numbers bigger than 1e4, but it takes a long time (e.g under 692.396 seconds for 1e5);
void test_my_calloc_volume(void) {
    int *ptr;
    for(int i = 0; i < 1e4; i++) { 
        ptr = (int *)my_calloc(i, 4);
    }
}

void test_my_calloc_coalesce(void) {
    void *a = my_calloc(100, 4);
    void *b = my_calloc(100, 4);
    void *c = my_calloc(100, 4);
    my_free(a);
    my_free(b);
    void *d = my_calloc(180, 4); 
    CU_ASSERT_PTR_NOT_NULL(d);
    CU_ASSERT_TRUE(d < c); 
}

void test_my_calloc_split(void) {
    void *large = my_calloc(2000, 4);
    void *end = my_calloc(1, 8);
    my_free(large);  
    void *small = my_calloc(100, 2);
    meta_block meta = get_pointer_to_meta_block(small);
    CU_ASSERT_EQUAL(meta->size, 200);
    CU_ASSERT_PTR_NOT_NULL(meta->next); 
}

void test_my_calloc_integrity(void) {
    char *str1 = my_calloc(16, 1);
    strcpy(str1, "Hello");   
    void *garbage = my_malloc(1024);        // test for alternating malloc with calloc
    CU_ASSERT_STRING_EQUAL(str1, "Hello");
}

void test_my_calloc_boundaries(void) {
    void *p1 = my_calloc(0, 0);
    CU_ASSERT_PTR_NULL(p1); 
    void *p2 = my_calloc(SIZE_MAX, SIZE_MAX); 
    CU_ASSERT_PTR_NOT_NULL(p2);
}

void test_my_calloc_size(void) {
    void *p = my_calloc(39, 71);\
    CU_ASSERT_EQUAL(get_pointer_to_meta_block(p)->size, 39 * 71 + 8 -((39 * 71) % 8));
}

/*
Helper method to create a suite
@param name Pointer to the name of the suite
@return CUnit suite object
*/
static CU_pSuite create_suite(const char* name) {
    CU_pSuite suite = CU_add_suite_with_setup_and_teardown(name, NULL, NULL, NULL, reset_heap); 
    if (CU_get_error() != CUE_SUCCESS)
        errx(EXIT_FAILURE, "%s", CU_get_error_msg());
    return suite;
}


/*
Tear down method to reset the heap after every test
*/
void reset_heap() {
    if (base != NULL) {
        brk(base); 
        base = NULL;
    }
}

int main(void) {
    // initialize registry
    if (CU_initialize_registry() != CUE_SUCCESS)
        errx(EXIT_FAILURE, "can't initialize test registry");

    // initialize test suites

    // align suite
    CU_pSuite align_suite = create_suite("align_suite");
    
    CU_add_test(align_suite, "align_zero", test_align_zero);
    CU_add_test(align_suite, "align_prime", test_align_prime);
    CU_add_test(align_suite, "align_negative", test_align_negative);
    CU_add_test(align_suite, "align_big_range", test_align_big_range);

    // find_block suite
    CU_pSuite find_block_suite = create_suite("find_block_suite");

    CU_add_test(find_block_suite, "find_block_base", test_find_block_base);
    CU_add_test(find_block_suite, "find_second_block", test_find_second_block);
    CU_add_test(find_block_suite, "find_block_NULL", test_find_block_NULL);

    // extend_heap suite
    CU_pSuite extend_heap_suite = create_suite("extend_heap_suite");

    CU_add_test(extend_heap_suite, "extend_heap_base", test_extend_heap_base);
    CU_add_test(extend_heap_suite, "extend_heap_large_size", test_extend_heap_large_size);

    // split_block suite
    CU_pSuite split_block_suite = create_suite("split_block_suite");

    CU_add_test(split_block_suite, "split_block_size", test_split_block_size);
    CU_add_test(split_block_suite, "split_block_pointer", test_split_block_pointer);

    // fusion suite
    CU_pSuite fusion_suite = create_suite("fusion_suite");

    CU_add_test(fusion_suite, "fusion_2_blocks_fwd", test_fusion_2_blocks_fwd);
    CU_add_test(fusion_suite, "fusion_2_blocks_bck", test_fusion_2_blocks_bck);
    CU_add_test(fusion_suite, "fusion_4_blocks", test_fusion_4_blocks);

    // get_pointer_to_meta_block suite
    CU_pSuite get_pointer_to_meta_block_suite = create_suite("get_pointer_to_meta_block suite");

    CU_add_test(get_pointer_to_meta_block_suite, "get_pointer_to_meta_block", test_get_pointer_to_meta_block);

    // valid_addr suite
    CU_pSuite valid_addr_suite = create_suite("valid_addr suite");

    CU_add_test(valid_addr_suite, "valid_addr_yes", test_valid_addr_yes);
    CU_add_test(valid_addr_suite, "valid_addr_no", test_valid_addr_no);
    CU_add_test(valid_addr_suite, "valid_addr_misalign", test_valid_addr_misalign);

    // my_free suite
    CU_pSuite my_free_suite = create_suite("my_free suite");

    CU_add_test(my_free_suite, "my_free_mid", test_my_free_mid);
    CU_add_test(my_free_suite, "my_free_end", test_my_free_end);

    // my_malloc suite
    CU_pSuite my_malloc_suite = create_suite("my_malloc suite");

    CU_add_test(my_malloc_suite, "my_malloc_array", test_my_malloc_array);
    CU_add_test(my_malloc_suite, "my_malloc_volume", test_my_malloc_volume);
    CU_add_test(my_malloc_suite, "my_malloc_coalesce", test_my_malloc_coalesce);
    CU_add_test(my_malloc_suite, "my_malloc_split", test_my_malloc_split);
    CU_add_test(my_malloc_suite, "my_malloc_integrity", test_my_malloc_integrity);
    CU_add_test(my_malloc_suite, "my_malloc_boundaries", test_my_malloc_boundaries);

    // my_calloc suite
    CU_pSuite my_calloc_suite = create_suite("my_calloc suite");

    CU_add_test(my_calloc_suite, "my_calloc_array", test_my_calloc_array);
    CU_add_test(my_calloc_suite, "my_calloc_volume", test_my_calloc_volume);
    CU_add_test(my_calloc_suite, "my_calloc_coalesce", test_my_calloc_coalesce);
    CU_add_test(my_calloc_suite, "my_calloc_split", test_my_calloc_split);
    CU_add_test(my_calloc_suite, "my_calloc_integrity", test_my_calloc_integrity);
    CU_add_test(my_calloc_suite, "my_calloc_boundaries", test_my_calloc_boundaries);
    CU_add_test(my_calloc_suite, "my_calloc_size", test_my_calloc_size);


    // run the tests
    CU_basic_run_tests();

    // clean the registry
    CU_cleanup_registry();
    return 0;
}
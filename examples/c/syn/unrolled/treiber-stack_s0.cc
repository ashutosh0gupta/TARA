// Examples borrowed from http://www.practicalsynthesis.org/PSynSyn.html

#include <threads.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdio.h>

#include "librace.h"
#include "model-assert.h"
#include "mem_op_macros.h"

#define MAX_FREELIST 4 /* Each thread can own up to MAX_FREELIST free nodes */
#define INITIAL_FREE 2 /* Each thread starts with INITIAL_FREE free nodes */

#define POISON_IDX 0x666

static unsigned int (*fr_l)[MAX_FREELIST];


#define release memory_order_release
#define acquire memory_order_acquire
#define relaxed memory_order_relaxed

#define MAX_NODES			0xf

typedef unsigned long long pointer;
typedef atomic_ullong pointer_t;

#define MAKE_POINTER(ptr, count)	((((pointer)count) << 32) | ptr)
#define PTR_MASK 0xffffffffLL
#define COUNT_MASK (0xffffffffLL << 32)

static inline void set_count(pointer *p, unsigned int val) { *p = (*p & ~COUNT_MASK) | ((pointer)val << 32); }
static inline void set_ptr(pointer *p, unsigned int val) { *p = (*p & ~PTR_MASK) | val; }
static inline unsigned int get_count(pointer p) { return (p & COUNT_MASK) >> 32; }
static inline unsigned int get_ptr(pointer p) { return p & PTR_MASK; }

typedef struct node {
	unsigned int value;
	pointer_t next;

} node_t;

typedef struct {
	pointer_t top;
	node_t nodes[MAX_NODES + 1];
} stack_t;

void init_stack(stack_t *s, int num_threads);
void push(stack_t *s, unsigned int val);
unsigned int pop(stack_t *s);
int get_thread_num();


/* Search this thread's free list for a "new" node */
static unsigned int new_node()
{
	int i;
	int t = get_thread_num();
	for (i = 0; i < MAX_FREELIST; i++) {
		//unsigned int node = load_32(&free_lists[t][i]);
		unsigned int node = fr_l[t][i];
		if (node) {
			//store_32(&free_lists[t][i], 0);
			fr_l[t][i] = 0;
			return node;
		}
	}
	/* free_list is empty? */
	MODEL_ASSERT(0);
	return 0;
}

/* Place this node index back on this thread's free list */
static void reclaim(unsigned int node)
{
	int i;
	int t = get_thread_num();

	/* Don't reclaim NULL node */
	//MODEL_ASSERT(node);

	for (i = 0; i < MAX_FREELIST; i++) {
		/* Should never race with our own thread here */
		//unsigned int idx = load_32(&free_lists[t][i]);
		unsigned int idx = fr_l[t][i];

		/* Found empty spot in free list */
		if (idx == 0) {
			//store_32(&free_lists[t][i], node);
			fr_l[t][i] = node;
			return;
		}
	}
	/* free list is full? */
	MODEL_ASSERT(0);
}

void init_stack(stack_t *s, int num_threads)
{
	int i, j;

	/* Initialize each thread's free list with INITIAL_FREE pointers */
	/* The actual nodes are initialized with poison indexes */
	fr_l = (unsigned int (*)[4]) malloc(num_threads * sizeof(*fr_l));
	for (i = 0; i < num_threads; i++) {
		for (j = 0; j < INITIAL_FREE; j++) {
			fr_l[i][j] = 1 + i * MAX_FREELIST + j;
			char name[std::has_name::NUM_ELEMS];
			sprintf(name, "fr_l[%d][%d].n", i, j);
			s->nodes[fr_l[i][j]].next.set_name(name);
			init(&s->nodes[fr_l[i][j]].next, MAKE_POINTER(POISON_IDX, 0));
		}
	}

	/* initialize stack */
	s->top.set_name("s->top");
	init(&s->top, MAKE_POINTER(0, 0));

}

void push(stack_t *s, unsigned int val) {
	unsigned int nodeIdx = new_node();
	node_t *node = &s->nodes[nodeIdx];
	node->value = val;
	pointer oldTop, newTop;
	bool success;
	while (true) {
		// acquire
		oldTop = load(&s->top, memory_order_relaxed);
		newTop = MAKE_POINTER(nodeIdx, get_count(oldTop) + 1);
		// relaxed
		store(&node->next, oldTop, memory_order_relaxed);

		// release & relaxed
		success = compare_exchange_strong(&s->top, &oldTop,
			newTop, memory_order_relaxed, memory_order_relaxed);
		if (success)
			break;
	} 
}

unsigned int pop(stack_t *s) 
{
	pointer oldTop, newTop, next;
	node_t *node;
	bool success;
	int val;
	while (true) {
		// acquire
		oldTop = load(&s->top, memory_order_relaxed);
		if (get_ptr(oldTop) == 0)
			return 0;
		node = &s->nodes[get_ptr(oldTop)];
		// relaxed
		next = load(&node->next, memory_order_relaxed);
		newTop = MAKE_POINTER(get_ptr(next), get_count(oldTop) + 1);
		// release & relaxed
		success = compare_exchange_strong(&s->top, &oldTop, newTop, memory_order_relaxed, memory_order_relaxed);
		if (success)
			break;
	}
	val = node->value;
	/* Reclaim the used slot */
	reclaim(get_ptr(oldTop));
	return val;
}



static int procs = 4;
static stack_t *stack;
static thrd_t *threads;
static int num_threads;

unsigned int idx1, idx2;
unsigned int a, b;


atomic<int> x[3] = {atomic<int>("x1"), atomic<int>("x2"), atomic<int>("x3")};

int get_thread_num()
{
	thrd_t curr = thrd_current();
	int i;
	for (i = 0; i < num_threads; i++)
		if (curr.priv == threads[i].priv)
			return i;
	MODEL_ASSERT(0);
	return -1;
}

static void main_task(void *param)
{
	unsigned int val;
	int pid = *((int *)param);

	if (pid % 4 == 0) {
		store(&x[1], 17, memory_order_relaxed);
		push(stack, 1);
	} else if (pid % 4 == 1) {
		store(&x[2], 37, memory_order_relaxed);
		push(stack, 2);
	} else if (pid % 4 == 2) {
		idx1 = pop(stack);
		if (idx1 != 0) {
			a = load(&x[idx1], memory_order_relaxed);
			printf("a: %d\n", a);
		}
	} else {
		idx2 = pop(stack);
		if (idx2 != 0) {
			b = load(&x[idx2], memory_order_relaxed);
			printf("b: %d\n", b);
		}
	}
}

int user_main(int argc, char **argv)
{
	int i;
	int *param;
	unsigned int in_sum = 0, out_sum = 0;

	atomic_init(&x[1], 0);
	atomic_init(&x[2], 0);

	stack = (stack_t*)calloc(1, sizeof(*stack));

	num_threads = procs;
	threads = (thrd_t*)malloc(num_threads * sizeof(thrd_t));
	param = (int*)malloc(num_threads * sizeof(*param));

	init_stack(stack, num_threads);

	for (i = 0; i < num_threads; i++) {
		param[i] = i;
		thrd_create(&threads[i], main_task, &param[i]);
	}
	for (i = 0; i < num_threads; i++)
		thrd_join(threads[i]);

	bool correct = true;
	correct |= (a == 17 || a == 37 || a == 0);
	MODEL_ASSERT(correct);

	free(param);
	free(threads);
	free(stack);

	return 0;
}


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

#define HASHTABLE_SIZE 16

static uint64_t hash_key(const char *);

struct elem {
	struct elem *next;
	const char *val;
};

int main() {
	struct elem *hashtable =  malloc(sizeof *hashtable * HASHTABLE_SIZE);
	struct elem empty = {
		.next = NULL,
		.val = NULL,
	};
	for (int i = 0; i < HASHTABLE_SIZE; i++) {
		hashtable[i] = empty;
	}

	struct elem el = {
		.next = NULL,
		.val = "/homepage",
	};

	int index;

	index = (int)hash_key(el.val) % HASHTABLE_SIZE;

	hashtable[index] = el;

	for (int i = 0; i < HASHTABLE_SIZE; i++) {
		if (hashtable[i].val != NULL) {
			printf("%s\n", hashtable[i].val);
		}
	}

	free(hashtable);
}

static uint64_t hash_key(const char *key) {
	uint64_t hash = FNV_OFFSET;
	for (const char * p = key; *p; p++) {
		hash ^= (uint64_t)(unsigned char)(*p);
		hash *= FNV_PRIME;
	}
	return hash;
}

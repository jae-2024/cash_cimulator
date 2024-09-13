#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

struct d_cache { /* 데이터 캐시 구조체 */
    int tag;       /* 태그 */
    int valid;     /* valid bit */
    int time;      /* LRU를 구현하기 위한 마지막 접근 시간*/
    int dirty;     /* write back 시 dirty bit */
};

/* 전역 변수 */
int l_total = 0, s_total = 0;
int l_hits = 0, l_misses = 0; /*load 히트 수와 미스 수*/
int s_hits = 0, s_misses = 0; /*store 히트 수와 미스 수*/
int t_cycles = 0;             /*토탈 사이클*/
int time_count = 0;           /* LRU를 구현하기 위한 시간 */

struct d_cache* dp; /* 데이터 캐시를 가리키는 포인터 */
/* 함수 목록 */
void read_data(int addr, int c_size, int b_size, int assoc);
void write_data(int addr, int c_size, int b_size, int assoc);
int evict(int set, int assoc);

int main() {
    FILE* fp;
    char mode;
    unsigned int address;
    int k;

    fp = fopen("gcc.trace.txt", "r");

    if (fp == NULL) {
        printf("파일을 열 수 없습니다.\n");
        return 1;
    }

    dp = (struct d_cache*)malloc(1024 * sizeof(struct d_cache)); /*캐시 사이즈*/

    if (dp == NULL) {
        printf("메모리 할당에 실패했습니다.\n");
        return 1;
    }

    printf("associative: ");
    scanf("%d", &k);

    while (fscanf(fp, "%c %x\n", &mode, &address) != EOF) {
        switch (mode) {
        case 'l':
            read_data(address, 16384, 16, k);
            l_total++;
            break;
        case 's':
            write_data(address, 16384, 16, k);
            s_total++;
            break;
        }
        time_count++; /* 하나의 명령을 수행할 때마다 시간을 늘린다. */
        t_cycles++;
    }

    printf("Total loads: %d\nTotal stores: %d\nLoad hits: %d\nLoad misses: %d\nStore hits: %d\nStore misses: %d\nTotal cycles: %d",
        l_total, s_total, l_hits, l_misses, s_hits, s_misses, t_cycles);

    fclose(fp);
    free(dp);

    return 0;
}

void read_data(int addr, int c_size, int b_size, int assoc) {
    int num_of_sets, set; /* set의 개수와 입력받은 주소의 set을 저장하는 변수 */
    int i, ev = 0, avail = 0; /* 반복문의 인덱스와 victim의 인덱스, 그리고
                                     새로 넣을 블럭의 인덱스 */
    struct d_cache* p;            /* 캐시를 가리키는 포인터 */

    num_of_sets = c_size / (b_size * assoc); /* set의 개수를 구한다. */
    set = (addr / b_size) %
        num_of_sets; /* 입력받은 인자로부터 해당 주소의 set을 구한다. */

    /* 캐시에서 해당 set을 검색하여 HIT/MISS를 결정 */
    for (i = 0; i < assoc; i++) {
        p = &dp[set * assoc + i];
        /* valid bit이 1이고 tag값이 일치하면 접근 시간을 바꾸고 HIT */
        if (p->valid == 1 && p->tag == (addr / b_size) / num_of_sets) {
            l_hits++;
            return;
        }
    }
    l_misses++;
}

void write_data(int addr, int c_size, int b_size, int assoc) {
    int num_of_sets, set; /* set의 개수와 입력받은 주소의 set을 저장하는 변수 */
    int i, ev = 0, avail = 0; /* 반복문의 인덱스와 victim의 인덱스, 그리고
                                     새로 넣을 블럭의 인덱스 */
    struct d_cache* p;            /* 캐시를 가리키는 포인터 */

    num_of_sets = c_size / (b_size * assoc); /* set의 개수를 구한다. */
    set = (addr / b_size) %
        num_of_sets; /* 입력받은 인자로부터 해당 주소의 set을 구한다. */

    /* 캐시에서 해당 set을 검색하여 HIT/MISS를 결정 */
    for (i = 0; i < assoc; i++) {
        p = &dp[set * assoc + i];
        /* valid bit이 1이고 tag값이 일치하면 접근 시간을 바꾸고, dirty bit을 1로
         * 변경하고 HIT */
        if (p->valid == 1 && p->tag == (addr / b_size) / num_of_sets) {
            p->time = time_count;
            p->dirty = 1;
            s_hits++;
            return;
        }
        else if (p->valid == 0) {
            if (i < assoc) {
                avail = i;
            }
        }
    }
    s_misses++;
    /* 캐시의 set이 가득찬 경우 */
    if (avail == assoc) {
        ev = evict(set, assoc);
        p = &dp[set * assoc + ev];

        p->valid = 1;
        p->time = time_count;
        p->tag = (addr / b_size) / num_of_sets;
        p->dirty = 1; /* 새로 올린 블럭도 수정했으므로 dirty bit은 1 */
    }
    /* 캐시의 set에 자리가 있는 경우 */
    else {
        p = &dp[set * assoc + avail];
        p->valid = 1;
        p->time = time_count;
        p->tag = (addr / b_size) / num_of_sets;
        p->dirty = 1;
    }
}

/* LRU기법에 따라서 victim을 정하는 함수 */
int evict(int set, int assoc) {
    int i, tmp_time = 0; /* 반복문의 인덱스와 시간을 저장하는 변수 */
    int min = time_count + 1,
        min_i = 0; /* 최소값을 찾기 위한 변수와 인덱스 변수 */

    /* set에서 time값이 가장 작은 블럭의 인덱스를 찾아 return한다. */
    for (i = 0; i < assoc; i++) {
        tmp_time = dp[set * assoc + i].time;
        if (min > tmp_time) {
            min = tmp_time;
            min_i = i;
        }
    }
    return min_i;
}
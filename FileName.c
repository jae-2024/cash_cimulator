#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

struct d_cache { /* ������ ĳ�� ����ü */
    int tag;       /* �±� */
    int valid;     /* valid bit */
    int time;      /* LRU�� �����ϱ� ���� ������ ���� �ð�*/
    int dirty;     /* write back �� dirty bit */
};

/* ���� ���� */
int l_total = 0, s_total = 0;
int l_hits = 0, l_misses = 0; /*load ��Ʈ ���� �̽� ��*/
int s_hits = 0, s_misses = 0; /*store ��Ʈ ���� �̽� ��*/
int t_cycles = 0;             /*��Ż ����Ŭ*/
int time_count = 0;           /* LRU�� �����ϱ� ���� �ð� */

struct d_cache* dp; /* ������ ĳ�ø� ����Ű�� ������ */
/* �Լ� ��� */
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
        printf("������ �� �� �����ϴ�.\n");
        return 1;
    }

    dp = (struct d_cache*)malloc(1024 * sizeof(struct d_cache)); /*ĳ�� ������*/

    if (dp == NULL) {
        printf("�޸� �Ҵ翡 �����߽��ϴ�.\n");
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
        time_count++; /* �ϳ��� ����� ������ ������ �ð��� �ø���. */
        t_cycles++;
    }

    printf("Total loads: %d\nTotal stores: %d\nLoad hits: %d\nLoad misses: %d\nStore hits: %d\nStore misses: %d\nTotal cycles: %d",
        l_total, s_total, l_hits, l_misses, s_hits, s_misses, t_cycles);

    fclose(fp);
    free(dp);

    return 0;
}

void read_data(int addr, int c_size, int b_size, int assoc) {
    int num_of_sets, set; /* set�� ������ �Է¹��� �ּ��� set�� �����ϴ� ���� */
    int i, ev = 0, avail = 0; /* �ݺ����� �ε����� victim�� �ε���, �׸���
                                     ���� ���� ���� �ε��� */
    struct d_cache* p;            /* ĳ�ø� ����Ű�� ������ */

    num_of_sets = c_size / (b_size * assoc); /* set�� ������ ���Ѵ�. */
    set = (addr / b_size) %
        num_of_sets; /* �Է¹��� ���ڷκ��� �ش� �ּ��� set�� ���Ѵ�. */

    /* ĳ�ÿ��� �ش� set�� �˻��Ͽ� HIT/MISS�� ���� */
    for (i = 0; i < assoc; i++) {
        p = &dp[set * assoc + i];
        /* valid bit�� 1�̰� tag���� ��ġ�ϸ� ���� �ð��� �ٲٰ� HIT */
        if (p->valid == 1 && p->tag == (addr / b_size) / num_of_sets) {
            l_hits++;
            return;
        }
    }
    l_misses++;
}

void write_data(int addr, int c_size, int b_size, int assoc) {
    int num_of_sets, set; /* set�� ������ �Է¹��� �ּ��� set�� �����ϴ� ���� */
    int i, ev = 0, avail = 0; /* �ݺ����� �ε����� victim�� �ε���, �׸���
                                     ���� ���� ���� �ε��� */
    struct d_cache* p;            /* ĳ�ø� ����Ű�� ������ */

    num_of_sets = c_size / (b_size * assoc); /* set�� ������ ���Ѵ�. */
    set = (addr / b_size) %
        num_of_sets; /* �Է¹��� ���ڷκ��� �ش� �ּ��� set�� ���Ѵ�. */

    /* ĳ�ÿ��� �ش� set�� �˻��Ͽ� HIT/MISS�� ���� */
    for (i = 0; i < assoc; i++) {
        p = &dp[set * assoc + i];
        /* valid bit�� 1�̰� tag���� ��ġ�ϸ� ���� �ð��� �ٲٰ�, dirty bit�� 1��
         * �����ϰ� HIT */
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
    /* ĳ���� set�� ������ ��� */
    if (avail == assoc) {
        ev = evict(set, assoc);
        p = &dp[set * assoc + ev];

        p->valid = 1;
        p->time = time_count;
        p->tag = (addr / b_size) / num_of_sets;
        p->dirty = 1; /* ���� �ø� ���� ���������Ƿ� dirty bit�� 1 */
    }
    /* ĳ���� set�� �ڸ��� �ִ� ��� */
    else {
        p = &dp[set * assoc + avail];
        p->valid = 1;
        p->time = time_count;
        p->tag = (addr / b_size) / num_of_sets;
        p->dirty = 1;
    }
}

/* LRU����� ���� victim�� ���ϴ� �Լ� */
int evict(int set, int assoc) {
    int i, tmp_time = 0; /* �ݺ����� �ε����� �ð��� �����ϴ� ���� */
    int min = time_count + 1,
        min_i = 0; /* �ּҰ��� ã�� ���� ������ �ε��� ���� */

    /* set���� time���� ���� ���� ���� �ε����� ã�� return�Ѵ�. */
    for (i = 0; i < assoc; i++) {
        tmp_time = dp[set * assoc + i].time;
        if (min > tmp_time) {
            min = tmp_time;
            min_i = i;
        }
    }
    return min_i;
}
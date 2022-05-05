#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Khaja Zayn Zuhuruddin (65582663), zuhurudk
// Kashan Hassan Saeed

int count = 1;
struct Memory
{
    int address, vmAddress, data;
};

typedef struct
{
    int v_page_num;
    int time_stamp;
    unsigned long long lastAccessed;
} tTuple;

struct PageTable
{
    int v_page_num, valid_bit, dirty_bit, page_num, time_stamp;
    unsigned long long lastAccessed;
};

struct Memory main_memory[32];
struct Memory virtual_memory[128];

struct PageTable p_table[16];

int fifo = 0, lru = 0;

int LRU()
{

    tTuple memory_pages[4];
    int i;
    int i2 = 0;

    for (i = 0; i < 16; i++)
    {
        if (p_table[i].valid_bit == 1) // if valid bit is 1 it means it is in main memory
        {
            memory_pages[i2].v_page_num = p_table[i].v_page_num;
            memory_pages[i2].lastAccessed = p_table[i].lastAccessed;
            i2++;
        }
    }

    // find the oldest ("first in") page
    unsigned long long min = memory_pages[0].lastAccessed;

    int where = 0;
    int oldest_page_index_in_memory_pages = 0;

    for (i = 0; i < 4; i++)
    {

        if ((memory_pages[i].lastAccessed) < min)
        {
            min = memory_pages[i].lastAccessed;
            where = i;
        }
    }

    oldest_page_index_in_memory_pages = where;

    int oldest_v_page_num = memory_pages[oldest_page_index_in_memory_pages].v_page_num; // the page in the page table that will be sent back to disk

    if (p_table[oldest_v_page_num].dirty_bit == 1) // means that some edits were made that need to be saved
    {
        // copy and paste all the changes to the virtual memory
        int virtual_memory_starting_index = oldest_v_page_num * 8;
        int main_memory_starting_index = oldest_page_index_in_memory_pages * 8;
        int i;
        for (i = 0; i < 8; i++)
        {
            virtual_memory[virtual_memory_starting_index + i].data = main_memory[main_memory_starting_index + i].data;
            main_memory[main_memory_starting_index + i].address = 0;
        }
    }

    // update page table
    p_table[oldest_v_page_num].page_num = p_table[oldest_v_page_num].v_page_num;
    p_table[oldest_v_page_num].valid_bit = 0; // disk page
    p_table[oldest_v_page_num].dirty_bit = 0;

    return oldest_page_index_in_memory_pages;
}

int FIFO()
{
    // gather info (v_page_num, time_stamp) on all the pages in main memory
    tTuple memory_pages[4];
    int i;
    int i2 = 0;
    for (i = 0; i < 16; i++)
    {
        if (p_table[i].valid_bit == 1) // if valid bit is 1 it means it is in main memory
        {
            memory_pages[i2].v_page_num = p_table[i].v_page_num;
            memory_pages[i2].time_stamp = p_table[i].time_stamp;
            i2++;
        }
    }

    // find the oldest ("first in") page
    int oldest_page_index_in_memory_pages = 0;
    for (i = 0; i < 4; i++)
    {
        if (memory_pages[i].time_stamp < memory_pages[oldest_page_index_in_memory_pages].time_stamp)
        {
            oldest_page_index_in_memory_pages = i;
        }
    }

    int oldest_v_page_num = memory_pages[oldest_page_index_in_memory_pages].v_page_num; // the page in the page table that will be sent back to disk

    if (p_table[oldest_v_page_num].dirty_bit == 1) // means that some edits were made that need to be saved
    {
        // copy and paste all the changes to the virtual memory
        int virtual_memory_starting_index = oldest_v_page_num * 8;
        int main_memory_starting_index = oldest_page_index_in_memory_pages * 8;
        int i;
        for (i = 0; i < 8; i++)
        {
            virtual_memory[virtual_memory_starting_index + i].data = main_memory[main_memory_starting_index + i].data;
            main_memory[main_memory_starting_index + i].address = 0;
        }
    }

    // update page table
    p_table[oldest_v_page_num].page_num = p_table[oldest_v_page_num].v_page_num;
    p_table[oldest_v_page_num].valid_bit = 0; // disk page
    p_table[oldest_v_page_num].dirty_bit = 0;

    return oldest_page_index_in_memory_pages;
}

void init()
{
    int i;
    for (i = 0; i < sizeof(main_memory) / sizeof(main_memory[0]); i++)
    {
        main_memory[i].data = -1;
        main_memory[i].address = 0;
    }
    for (i = 0; i < sizeof(virtual_memory) / sizeof(virtual_memory[0]); i++)
    {
        virtual_memory[i].data = -1;
        virtual_memory[i].address = i;
    }
    for (i = 0; i < sizeof(p_table) / sizeof(p_table[0]); i++)
    {
        p_table[i].v_page_num = p_table[i].page_num = i;
        p_table[i].valid_bit = p_table[i].dirty_bit = 0;
        p_table[i].time_stamp = 0;
    }
}

// struct timeval
// {
//     time_t tv_sec;  /* seconds */
//     time_t tv_usec; /* microseconds */
// };

unsigned long long getMS()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    unsigned long long millisecondsSinceEpoch =
        (unsigned long long)(tv.tv_sec) * 1000 +
        (unsigned long long)(tv.tv_usec) / 1000;
    return millisecondsSinceEpoch;
}

int firstAvailableMM()
{
    int i;
    for (i = 0; i < 32; i += 8)
        if (main_memory[i].address == 0)
            return i / 8;
    return -1;
}
void read(int virtual_addr)
{
    int i;
    int where;

    if (p_table[virtual_addr / 8].valid_bit == 0)
    {
        printf("A Page Fault Has Occurred\n");

        where = firstAvailableMM();

        if (where == -1)
        {
            if (lru == 1)
                where = LRU();
            else
                where = FIFO();
        }

        p_table[virtual_addr / 8].valid_bit = 1;
        p_table[virtual_addr / 8].time_stamp = count++;
        p_table[virtual_addr / 8].page_num = where;
        p_table[virtual_addr / 8].lastAccessed = getMS();

        for (i = 0; i < 8; i++)
        {
            main_memory[where * 8 + i].address = 1;
            main_memory[where * 8 + i].vmAddress = (virtual_addr / 8) * 8 + i;
        }

        main_memory[where * 8 + (virtual_addr % 8)].data = virtual_memory[virtual_addr].data;

        printf("%d\n", main_memory[p_table[virtual_addr / 8].page_num + (virtual_addr % 8)].data);
    }

    else
    {
        p_table[virtual_addr / 8].lastAccessed = getMS();
        printf("%d\n", main_memory[p_table[virtual_addr / 8].page_num + (virtual_addr % 8)].data);
    }
}

void write(int virtual_addr, int data)
{
    int i;
    int where;

    if (p_table[virtual_memory[virtual_addr].address / 8].valid_bit == 0)
    {
        printf("A Page Fault Has Occurred\n");

        where = firstAvailableMM();

        if (where == -1)
        {
            if (lru == 1)
                where = LRU();
            else
                where = FIFO();
        }

        p_table[virtual_addr / 8].valid_bit = 1;
        p_table[virtual_addr / 8].time_stamp = count++;
        p_table[virtual_addr / 8].page_num = where;
        p_table[virtual_addr / 8].lastAccessed = getMS();

        for (i = 0; i < 8; i++)
            main_memory[where * 8 + i].address = 1;

        main_memory[p_table[virtual_addr / 8].page_num + (virtual_addr % 8)].data = data;
    }

    else
    {
        p_table[virtual_addr / 8].lastAccessed = getMS();

        main_memory[p_table[virtual_addr / 8].page_num + (virtual_addr % 8)].data = data;
        p_table[virtual_addr / 8].dirty_bit = 1;
    }
}

void showmain(int ppn)
{
    int i;
    for (i = 0; i < 8; i++)
        printf("%d:%d\n", main_memory[ppn + i].vmAddress, main_memory[ppn + i].data);
}

void showptable()
{
    int i;
    for (i = 0; i < sizeof(p_table) / sizeof(p_table[0]); i++)
        printf("%d:%d:%d:%d\n", p_table[i].v_page_num, p_table[i].valid_bit, p_table[i].dirty_bit, p_table[i].page_num);
}

void run(char args[3][10])
{

    args[2][strcspn(args[2], "\n")] = 0;
    args[1][strcspn(args[2], "\n")] = 0;

    if (strcmp(args[0], "read") == 0)
        read(atoi(args[1]));

    else if (strcmp(args[0], "write") == 0)
        write(atoi(args[1]), atoi(args[2]));

    else if (strcmp(args[0], "showmain") == 0)
        showmain(atoi(args[1]));

    else if (strcmp(args[0], "showptable\n") == 0)
        showptable();
}

void parse(char args[3][10], char input[100])
{
    char *token = strtok(input, " ");

    int i = 0;
    strcpy(args[i], token);
    i++;

    while (token != NULL)
    {
        token = strtok(NULL, " ");
        if (token == NULL)
            return;
        strcpy(args[i], token);
        i++;
    }
}

void loop()
{
    char args[3][10];
    char input[100];

    do
    {
        printf("> ");

        fgets(input, 80, stdin);

        parse(args, input);

        if (strcmp(args[0], "quit\n") == 0)
            exit(0);

        run(args);

    } while (1);
}

int main(int argc, char **argv)
{
    if (argv[1] == NULL || strcmp(argv[1], "FIFO") == 0)
        fifo = 1;
    else if (strcmp(argv[1], "LRU") == 0)
        lru = 1;
    init();
    loop();
    return 0;
}

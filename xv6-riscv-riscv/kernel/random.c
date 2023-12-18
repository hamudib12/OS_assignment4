#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

// Linear feedback shift register
// Returns the next pseudo-random number
// The seed is updated with the returned value
uint8 lfsr_char(uint8 lfsr)
{
    uint8 bit;
    bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 4)) & 0x01; lfsr = (lfsr >> 1) | (bit << 7);
    return lfsr;
}

struct {
    struct spinlock lock; // for safe concurrency.
    uint8 random_generator;
} random;


int randomwrite(int fd, const uint64 src, int n){
    if(n != 1){
        return -1;
    }
    uint8 seed;
    acquire(&random.lock);
    if(copyin(myproc()->pagetable, (char*)&seed, src, 1) == -1) {
        return -1;
    }
    random.random_generator = seed;
    release(&random.lock);
    return 1;
}
int randomread(int fd, uint64 dst, int n){
    int target = n;
    acquire(&random.lock);
    while (target > 0){
        uint8 rand = lfsr_char(random.random_generator); // generate random.
        if(copyout(myproc()->pagetable, dst, (char*)&rand, 1) < 0){
            break;
        }
        target --;
        random.random_generator = rand; // update the generator.
    }
    release(&random.lock);
    return n - target;
}
void
randominit(void)
{
    initlock(&random.lock, "random");
    random.random_generator = 0x2A;
    devsw[RANDOM].read = randomread;
    devsw[RANDOM].write = randomwrite;
}
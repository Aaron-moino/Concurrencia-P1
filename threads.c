#include "threads.h"

void lock(int *v) {
    while(test_and_set(v)==1);
}
void unlock(int *v) {
    *v=0;
}
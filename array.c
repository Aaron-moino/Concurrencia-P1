#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "options.h"
#include <threads.h>

#define DELAY_SCALE 1000

struct array {
    int size;
    int *arr;
};

struct thr_args {
    int id;
    int iterations;
    int delay;
    struct array *arr;
    mtx_t *mutex;
    mtx_t mutex_incre;
    mtx_t mutex_change;
    int *incre_cont;
    int *change_cont;
};

void apply_delay(int delay) {
    for (int i = 0; i < delay * DELAY_SCALE; i++); // waste time
}

int increment(void *arg) {
    struct thr_args *args = arg;
    int pos, val;

    while (*args->incre_cont < args->iterations ) {
        pos = rand() % args->arr->size;

        mtx_lock(&args->mutex_incre);
        *args->incre_cont = *args->incre_cont + 1;
        mtx_unlock(&args->mutex_incre);

        printf("%d increasing position %d\n", args->id, pos);

        while(1) {
            if(mtx_trylock(&args->mutex[pos])==thrd_success) break;
        }
        args->arr->arr[pos]++;
        apply_delay(args->delay);

        mtx_unlock(&args->mutex[pos]);

    }
    return 0;
}

int change(void *arg) {
    struct thr_args *args = arg;
    int pos1, pos2;

    while (*args->change_cont <args->iterations) {
        pos1 = rand() % args->arr->size;
        pos2 = rand() % args->arr->size;

        printf("%d decreasing position %d "
               "& increasing position %d\n", args->id, pos1, pos2);

        mtx_lock(&args->mutex_change);

        *args->change_cont = *args->change_cont + 1;
        mtx_unlock(&args->mutex_change);

        while(1) {
            if(mtx_trylock(&args->mutex[pos1]) == thrd_success) break;
        }
        args->arr->arr[pos1]--;
        apply_delay(args->delay);

        mtx_unlock(&args->mutex[pos1]);
        apply_delay(args->delay);

        while(1) {
            if(mtx_trylock(&args->mutex[pos2]) == thrd_success) break;
        }
        args->arr->arr[pos2]++;
        apply_delay(args->delay);

        mtx_unlock(&args->mutex[pos2]);


    }
    return 0;
}

void print_array(struct array arr) {
    int total = 0;

    for (int i = 0; i < arr.size; i++) {
        total += arr.arr[i];
        printf("%d ", arr.arr[i]);

    }

    printf("\nTotal: %d\n", total);
}


int main(int argc, char **argv) {
    struct options opt;
    struct array arr;
    srand(time(NULL));
    // Default values for the options 5 10 100 1000
    opt.num_threads = 5;
    opt.size = 10;
    opt.iterations = 100;
    opt.delay = 1000;

    read_options(argc, argv, &opt);

    thrd_t *thrd_increment = malloc(sizeof(thrd_t) * opt.num_threads);
    thrd_t *thrd_change = malloc(sizeof(thrd_t) * opt.num_threads);

    int *cont1 = malloc(sizeof(int));
    int *cont2 = malloc(sizeof(int));
    *cont1 = 0;
    *cont2 = 0;

    arr.size = opt.size;

    struct thr_args *arg1 = malloc(sizeof(struct thr_args) * opt.num_threads);
    struct thr_args *arg2 = malloc(sizeof(struct thr_args) * opt.num_threads);
    arr.arr = malloc(sizeof(int) * arr.size);

    mtx_t *mtx = malloc(sizeof(mtx_t) * opt.size);
    mtx_t mtx_cont1;
    mtx_t mtx_cont2;

    memset(arr.arr, 0, arr.size * sizeof(int));


    for (int i = 0; i < opt.size; i++) {
        mtx_init(&mtx[i], mtx_plain);
    }

    for (int i = 0; i < opt.num_threads; i++) {
        arg1[i].id = i;
        arg1[i].delay = opt.delay;
        arg1[i].arr = &arr;
        arg1[i].iterations = opt.iterations;
        arg1[i].mutex = mtx;
        arg1[i].mutex_incre = mtx_cont1;
        arg1[i].incre_cont = cont1;
        thrd_create(&thrd_increment[i], increment, &arg1[i]);
    }
    for (int i = 0; i < opt.num_threads; i++) {
        arg2[i].id = i + opt.num_threads;
        arg2[i].delay = opt.delay;
        arg2[i].arr = &arr;
        arg2[i].iterations = opt.iterations;
        arg2[i].mutex = mtx;
        arg2[i].mutex_change = mtx_cont2;
        arg2[i].change_cont = cont2;
        thrd_create(&thrd_change[i], change, &arg2[i]);
    }

    for (int j = 0; j < opt.num_threads; j++) {
        thrd_join(thrd_increment[j], NULL);
    }
    for (int j = 0; j < opt.num_threads; j++) {
        thrd_join(thrd_change[j], NULL);
    }

    for (int j = 0; j < opt.size; j++) {
        mtx_destroy(&mtx[j]);
    }

    mtx_destroy(&arg2->mutex_change);
    mtx_destroy(&arg1->mutex_incre);

    print_array(arr);

    free(thrd_increment);
    free(thrd_change);
    free(mtx);
    free(arr.arr);
    free(cont1);
    free(cont2);
    free(arg1);
    free(arg2);

    return 0;
}

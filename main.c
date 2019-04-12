#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

struct Param {
    double result, *v, *v1, *v2, **mat;
    int k, *index;
};

struct Sem {
    sem_t sem;
    int value;
};

void init (double **v1, double **v2, double ***mat, int k);
void mFree (double** v1, double** v2, double*** mat, int k);

double* productMatV2 (double* v2, double** mat, int k);
double productV1V (double* v1, double* v, int k);

void tidV_create(pthread_t** tidV, int k);
void* thread_func(struct Param* param);
void param_init(struct Param* param, double* v1, double* v2, double** mat, int k);

struct Sem sem;
sem_t semV;

int main(int argc, char* argv[]){
    double resultS, *v, *v1, *v2, **mat;
    int k;
    pthread_t* tidV;
    struct Param param;

    if (argc != 2) {
        fprintf(stderr, "Expected: %s <int>\n", argv[0]);
        return -10;
    }

    k = atoi(argv[1]);
    init(&v1, &v2, &mat, k);
    sem_init(&(sem.sem), 0, 1);
    sem.value = 0;
    sem_init(&semV, 0, 1);

    param_init(&param, v1, v2, mat, k);

    tidV_create(&tidV, k);

    for (int i=0; i<k; i++) {
        pthread_create(&tidV[i], NULL, (void*) &thread_func, &param);
    }

    for (int i=0; i<k; i++)
        pthread_join(tidV[i], NULL);


    v = productMatV2(v2, mat, k);
    resultS = productV1V(v1, v, k);

    printf("Result: %f %f\n", param.result, resultS);

    mFree(&v1, &v2, &mat, k);
    free(tidV);
    sem_destroy(&sem.sem);

    return 0;
}

void init (double **v1, double **v2, double ***mat, int k) {
    double r;

    *v1 = (double*)malloc(sizeof(double[k]));
    *v2 = (double*)malloc(sizeof(double[k]));
    *mat = (double**)malloc(sizeof(double[k][k]));

    for (int i=0; i<k; i++)
        (*mat)[i] = (double*)malloc(sizeof(double[k]));

    srandom((unsigned int) time(NULL));

    for (int i=0; i<k; i++) {
        r = (double)(random()%1000000)/1000000-0.5;
        (*v1)[i] = r;
    }

    for (int i=0; i<k; i++) {
        r = (double)(random()%1000000)/1000000-0.5;
        (*v2)[i] = r;
    }

    for (int i=0; i<k; i++) {
        for (int j=0; j<k; j++) {
            r = (double)(random()%1000000)/1000000-0.5;
            (*mat)[i][j] = r;
        }
    }
}

void mFree (double** v1, double** v2, double*** mat, int k) {

    free(*v1);
    free(*v2);

//    for (int i=0; i<k; i++)
//        free((*mat)[i]);

    free(*mat);
}

double* productMatV2 (double* v2, double** mat, int k) {
    double* result;

    result = (double*)malloc(sizeof(double)*k);

    for (int i=0; i<k; i++)
        for (int j=0; j<k; j++)
            result[i] += v2[j]*mat[i][j];

    return result;
}

double productV1V (double* v1, double* v2, int k) {
    double final = 0;

    for (int i=0; i<k; i++)
        final += v1[i]*v2[i];

    return final;
}

void tidV_create(pthread_t** tidV, int k) {
    *tidV = (pthread_t*)malloc(sizeof(pthread_t[k]));
}

void param_init(struct Param* param, double* v1, double* v2, double** mat, int k) {

    param->index = (int*)malloc(sizeof(int[k]));
    param->v = (double*)malloc(sizeof(double[k]));

    param->v1 = v1;
    param->v2 = v2;
    param->mat = mat;
    param->k = k;
    param->result = 0;

    for (int i=0; i<param->k; i++){
        param->v[i] = 0;
        param->index[i] = 0;
    }
}

void* thread_func (struct Param* param) {
    int ind = -1;

    sem_wait(&semV);
    for (int i=0; i<param->k; i++) {
        if (param->index[i] == 0) {
            ind = i;
            param->index[i] = 1;
            break;
        }
    }
    sem_post(&semV);

    for (int i=0; i<param->k; i++)
        param->v[ind] += param->v2[i] * param->mat[ind][i];

    sem_wait(&(sem.sem));
    if (++sem.value == param->k) {
        for (int i=0; i<param->k; i++)
            param->result += param->v[i]*param->v1[i];
    }
    sem_post(&(sem.sem));

    pthread_exit(NULL);
}
/* 
Author Name: Josh Morris
Professor: Dr. Pettey
Class: 4330 
Lab 3 

This lab simulates a small bank with 4 customers and 3 atms.
The program begins by reading cusdtomers account number and 
balances from corresponding files. It then tracks atm transactions
in threads for each atm and updates customer acounts using main
after three transactions have taken place. Main records each transaction
to the customer's account files 

input:  Customer files
        atm transaction files

output: customer files
        customer final balance
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef struct jb {
    int atmNum;
    int accNum;
    char trans;
    float transAmt;
    float curBalance;
} job;

typedef struct acc {
    int  num;
    float curBalance;
    FILE *record;
} account;

typedef struct thread_data {
    int id;
    FILE * atmFile;
}   atmType;

#define NUMATM 3
#define NUMCUSTOMER 4
#define MAXDONEJOBS 3
int doneThreads = 0;
int doneJobs = 0;
job workQueue[MAXDONEJOBS];
account customer[NUMCUSTOMER];
pthread_mutex_t mv = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t maincv = PTHREAD_COND_INITIALIZER;
pthread_cond_t threadcv = PTHREAD_COND_INITIALIZER;

void * processATM(void * my_atm);

int main() {
    int id[NUMATM];
    int i, j;
    atmType atm[NUMATM];
    const char * cstFileName[NUMCUSTOMER] = {"cust0.dat",
                                             "cust1.dat",
                                             "cust2.dat",
                                             "cust3.dat"};
    const char * atmFileName[NUMATM] = {"atm0.dat",
                                        "atm1.dat",
                                        "atm2.dat"};
    pthread_t thread[NUMATM];

    //read in customer account numbers and balances
    for (i = 0; i < NUMCUSTOMER; ++i) {
        customer[i].record = fopen(cstFileName[i], "r");
        fscanf(customer[i].record, "%d", &customer[i].num);
        fscanf(customer[i].record, "%f", &customer[i].curBalance);
        //printf("%d %f\n", customer[i].num, customer[i].curBalance);
        fclose(customer[i].record);
    }

    //reopen customer files for writing
    // write account number and beginning balance to each customer file
    for (i = 0; i < NUMCUSTOMER; ++i) {
        if ((customer[i].record = fopen(cstFileName[i], "w")) == NULL){
            printf("error opening atm %i\n", i);
            exit(1);
        }
        fprintf(customer[i].record, "%08i %.2f \n", customer[i].num, customer[i].curBalance);
    }

    //open transaction files (atm files)
    // create threads for each atm
    for (i = 0; i < NUMATM; ++i) {
        atm[i].id = i;
        atm[i].atmFile = fopen(atmFileName[i], "r");

        pthread_create(&thread[i], NULL, processATM, (void*)&atm[i]);
    }

    pthread_mutex_lock(&mv);
    // while there are still transactions (numFinAtm < 3)
    while(doneThreads < NUMATM) {
        // wait for three jobs to be processed
        while (doneJobs < MAXDONEJOBS) {
            while(pthread_cond_wait(&maincv, &mv) != 0);
        }


        // process jobs and print transaction
        for (i = 0; i < MAXDONEJOBS; ++i) {
            for (j = 0; j < NUMCUSTOMER; ++j) {
                if(workQueue[i].accNum == customer[j].num){
                    fprintf(customer[j].record, "%i %c %.2f %.2f\n", workQueue[i].atmNum, workQueue[i].trans, workQueue[i].transAmt, workQueue[i].curBalance);
                    break;
                }
            }
        }

        // clear work queue (set done jobs to 0)
        doneJobs = 0;

        // wake threads
        pthread_cond_broadcast(&threadcv);
        while(pthread_cond_wait(&maincv, &mv) != 0);
    }

    pthread_mutex_unlock(&mv);

    // process last three jobs and print transactions
    for (i = 0; i < MAXDONEJOBS; ++i) {
        for (j = 0; j < NUMCUSTOMER; ++j) {
            if(workQueue[i].accNum == customer[j].num){
                fprintf(customer[j].record, "%i %c %.2f %.2f\n", workQueue[i].atmNum, workQueue[i].trans, workQueue[i].transAmt, workQueue[i].curBalance);
                break;
            }
        }
    }


    // join all threads
    for (i = 0; i < NUMATM; ++i) {
        pthread_join(thread[i], NULL);
    }

    // print all balances
    // close all files
    for(i = 0; i < NUMCUSTOMER; ++i) {
        printf("the ending balance for %08i is $%.2f\n", customer[i].num, customer[i].curBalance);


        if(i < NUMATM) {
            fclose(atm[i].atmFile);
        }
        fclose(customer[i].record);
    }

    return 0;
}

void * processATM(void * my_atm) {
    int lineRead;
    int sleepTime;
    int i;
    job curJob;
    atmType me = *(atmType *) my_atm;

    curJob.atmNum = me.id;

    // while there are still transactions
    while ((lineRead = fscanf(me.atmFile, "%i %c %f %i", &curJob.accNum, &curJob.trans, &curJob.transAmt, &sleepTime)) != EOF) {
        pthread_mutex_lock(&mv);
        //identify correct account and update balance
        for(i = 0; i < NUMCUSTOMER; ++i) {
            if (customer[i].num == curJob.accNum) {
                if (curJob.trans == 'w') {
                    customer[i].curBalance -= curJob.transAmt;
                    printf("acc: %08i bal: %.2f \n", curJob.accNum, customer[i].curBalance);
                    if (customer[i].curBalance < 0) {
                        customer[i].curBalance -= 10;
                        printf("%08i is overdrawn and incurred a $10 fee\n", curJob.accNum);
                        printf("acc: %08i bal: %.2f \n", curJob.accNum, customer[i].curBalance);

                    }
                }
                else if (curJob.trans == 'd'){
                    customer[i].curBalance += curJob.transAmt;
                    printf("acc: %08i bal: %.2f \n", curJob.accNum, customer[i].curBalance);                    
                }

                curJob.curBalance = customer[i].curBalance;
                break;
            }
        } 

        // wait for work queue to open up
        while (doneJobs == 3) {
            while(pthread_cond_wait(&threadcv, &mv) != 0);
        }

        // add job to work queue and wake up main if need be
        if (doneJobs < (MAXDONEJOBS-1)) {
            workQueue[doneJobs] = curJob;
            ++doneJobs;
        }
        else {
            workQueue[doneJobs] = curJob;
            ++doneJobs;
            //fprintf(stderr, "thread %i is broadcasting\n", me.id);
            pthread_cond_broadcast(&maincv);
        }
        pthread_mutex_unlock(&mv);

        sleep(sleepTime);
    }

    pthread_mutex_lock(&mv);
    //fprintf(stderr, "thread %d is done processing\n", me.id);
    ++doneThreads;
    if (doneThreads == 3) {
        //fprintf(stderr, "thread %d is broadcasting\n", me.id);
        pthread_cond_broadcast(&maincv);
    }
    pthread_mutex_unlock(&mv);

    return;
}

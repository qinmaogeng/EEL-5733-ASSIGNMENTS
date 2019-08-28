#include <pthread.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

sem_t lock;
sem_t empty;
sem_t full;
int TupleProd = 0;
int ListNum = 0;
int done = 0;
int BufNum;
int BufSize;
int Buffer[100];
FILE *fp;
int empty_t;
int fullflag[5] = {0,0,0,0,0};

struct tuple{
    char Uid[5];
    char Act;
    char Topic[16];
    int score;
    int Rid;
};

struct idlist{
    char Uid[5];
    int Rid;
};

struct tuple tuples[100];
struct idlist list[100];

void *mapper(void *t)
{
    int i = 0;
    int Mid = (long)t;
    int re;
    int n = 0;
    int flag = 0;
    int fla = 0;
    char c;
    int emptyt;
    while(1)
    {
        sem_wait(&lock);
        re = scanf("(%[^,],%c,%[^)])%c", tuples[TupleProd].Uid, &tuples[TupleProd].Act, tuples[TupleProd].Topic, &c);
        if (re != 4)
        {
             done = 1;
             emptyt = empty_t;
             for(i = 0; i <= 5; i++)
             {
                 sem_post(&empty);
             }            
             printf("mapper signalled done!\n");
             sem_post(&lock);
             break;
        } //read the tuple from file
        switch(tuples[TupleProd].Act){
             case 'P':
		tuples[TupleProd].score = 50;
		break;
	     case 'L':
		tuples[TupleProd].score = 20;
		break;
	     case 'D':
		tuples[TupleProd].score = -10;
		break;
	     case 'C':
		tuples[TupleProd].score = 30;
		break;
	     case 'S':
		tuples[TupleProd].score = 40;
		break;
         }    //get the score according to the Action
         if(TupleProd == 0)
         {
             strcpy(list[ListNum].Uid, tuples[TupleProd].Uid);
             list[ListNum].Rid = 0;
             tuples[TupleProd].Rid = list[ListNum].Rid;               
         }
         else
         {
             for(n = 0; n <= ListNum; n++)
             {
                 if(strcmp(list[n].Uid, tuples[TupleProd].Uid) == 0)
                 {
                     tuples[TupleProd].Rid = list[n].Rid;
                     flag = 1;          
                 }
             }
             if(flag == 0)
             {
                 ListNum++;
                 strcpy(list[ListNum].Uid, tuples[TupleProd].Uid);
                 list[ListNum].Rid = list[ListNum-1].Rid + 1;
                 tuples[TupleProd].Rid = list[ListNum].Rid;
             }
                 flag = 0;
             
         }  //insert the Uid and Rid to the id list which is used for reuducer and set the Rid of every tuple
         while (Buffer[tuples[TupleProd].Rid] == BufSize){
             printf("mapper waiting on full\n");
             fullflag[tuples[TupleProd].Rid] = 1;
             sem_post(&lock);
             sem_wait(&full);
             sem_wait(&lock);
         }
         fullflag[tuples[TupleProd].Rid] = 0;
         fla = 0;
         printf("mapper insert tuple\n");
         printf("(%s,%s,%d),%d\n",tuples[TupleProd].Uid, tuples[TupleProd].Topic, tuples[TupleProd].score, tuples[TupleProd].Rid);
         Buffer[tuples[TupleProd].Rid]++;
         TupleProd++;
         printf("Rid: %d, TupleProd: %d, BUffer: %d\n", tuples[TupleProd-1].Rid, TupleProd, Buffer[tuples[TupleProd-1].Rid]);
         sem_post(&empty);
         if(empty_t != 0)
         {
             empty_t--;
         }       
         sem_post(&lock);
    }
    return t;
}

void *reducer(void *t)
{
    long Rid = (long)t;
    int n = 0;
    int s = 0;
    struct tuple consumer[100]; //array of consumed tuple
    int TupleCond = 0;
    int j = 0;
    int k = 0;
    int kk = 0;
    int flag = 0;
    int fla = 0;
    while(1)
    {
        sem_wait(&lock);
        while(Buffer[Rid] == 0 && !done)
        {
            printf("reducer %ld waiting on empty\n", Rid);
            sem_post(&lock);
            sem_wait(&empty);
            if(fla == 0)
            {
                 empty_t++;
                 fla = 1;  
            }
            sem_wait(&lock);
        }
        fla = 0;
        
        if(Buffer[Rid] == 0 && done)
        {
            struct tuple reducer[100];
            printf("reducer %ld is exiting\n", Rid);
            printf("\n");
            for(j = 0; j <= TupleCond; j++)
            {
                if(j == 0)
                {
                    strcpy(reducer[k].Uid, consumer[j].Uid);
                    reducer[k].score = consumer[j].score;
                    strcpy(reducer[k].Topic, consumer[j].Topic);
                }
                else
                {
                    for(kk = 0; kk <= k; kk++)
                    {
                        if(strcmp(consumer[j].Topic,reducer[kk].Topic) == 0)
                        {
                            reducer[kk].score += consumer[j].score;
                            flag = 1;
                        }
                    }
                    if(flag == 0)
                    {
                        k++;
                        strcpy(reducer[k].Uid, consumer[j].Uid);
                        reducer[k].score = consumer[j].score;
                        strcpy(reducer[k].Topic, consumer[j].Topic);
                    }
                    flag = 0;
                }
            }
            for(j = 0; j < k; j++)
            {
                printf("(%s,%s,%d)\n", reducer[j].Uid, reducer[j].Topic, reducer[j].score);
                fprintf(fp, "(%s,%s,%d)\n", reducer[j].Uid, reducer[j].Topic, reducer[j].score);
            }
            printf("\n");
            sem_post(&lock);
            break;  
        }  //the process of exiting a reducer and print its output
         
        for(n = s; n <= TupleProd; n++)
        {
            if(tuples[n].Rid == Rid)
            {
                s = n+1;
                strcpy(consumer[TupleCond].Uid, tuples[n].Uid);
                consumer[TupleCond].score = tuples[n].score;
                strcpy(consumer[TupleCond].Topic, tuples[n].Topic);
                printf("(%s,%s,%d)\n",consumer[TupleCond].Uid, consumer[TupleCond].Topic, consumer[TupleCond].score);
                TupleCond++;
                Buffer[Rid]--;
                printf("reducer: %ld TupleCond: %d Buffer: %d\n", Rid,TupleCond,Buffer[Rid] );
                break;
            }   
        } //consume the tuple by linearly searching the tuples produced by mapper
        if(fullflag[Rid] == 1)
        {
            sem_post(&full);   
        }
        sem_post(&lock);
    }
    return t;
}

int main(int argc, char *argv[])
{
    long i = 0;
    BufSize = atoi(argv[1]);
    BufNum = atoi(argv[2]);
    for(i = 0; i < BufNum; i++)
    {
        Buffer[i] = 0;
    } //initialize the buffer
    fp = fopen("output.txt","w+");
    
    pthread_t Thid[BufNum+1];
    void *status;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    sem_init(&lock,0,1);
    sem_init(&empty,0,0);
    sem_init(&full,0,1);
    
    pthread_create(&Thid[0],&attr,mapper,(void *)i); /*create the mapper thread*/
    
    for(i = 0; i < BufNum; i++)
    {
        pthread_create(&Thid[i+1],&attr,reducer,(void *)i);
    } /*create the reducer thread*/

    pthread_join(Thid[0], &status);
    printf("main joined mapper\n");
    for(i = 0; i < BufNum; i++)
    {
        pthread_join(Thid[i+1], &status);
        printf("main joined reducer %ld\n", i);
    }
    
    fclose(fp);

    pthread_exit(NULL);
    return 0;
}

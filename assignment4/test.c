#include <pthread.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>

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

struct prim{
    pthread_mutex_t lock;
    pthread_mutexattr_t mattr;
    pthread_cond_t full;
    pthread_cond_t empty;
    pthread_condattr_t cattr;
    int NumItems;
};

int main(int argc, char *argv[])
{
    FILE *fp;
    int BufSize = atoi(argv[1]);
    int BufNum = atoi(argv[2]);
    pid_t reducers[BufNum];
    int i = 0;
    int ii = 0;
    int out = 0;
    int j = 0;
    int k = 0;
    int kk = 0;
    int flag = 0;
    int TupleCond = 0;
    int *done;
    struct tuple consumer[100];
    struct tuple reducer[100];
    struct tuple **buffer;
    struct prim *prims = mmap(NULL, sizeof(struct prim)*BufNum, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);;
    struct prim *tempointer;
    done = mmap(NULL,sizeof(int),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *done = 0;
    fp = mmap(NULL, sizeof(FILE), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fp = fopen("output.txt","w+");
    buffer = mmap(NULL,BufNum,PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    while(i < BufNum && !(*done))
    {
        printf("reducer %d creating\n",i);
        pthread_mutexattr_init(&prims[i].mattr);
        pthread_mutexattr_setpshared(&prims[i].mattr,PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&prims[i].lock,&prims[i].mattr);
        pthread_condattr_init(&prims[i].cattr);
        pthread_condattr_setpshared(&prims[i].cattr,PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&prims[i].full,&prims[i].cattr);
        pthread_cond_init(&prims[i].empty,&prims[i].cattr);
        buffer[i] = mmap(NULL,sizeof(struct tuple)*BufSize,PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        printf("reducer %d created\n",i);
        reducers[i] = fork();
        if(reducers[i] == 0)
        {
             out = 0;
             j = 0;
             k = 0;
             kk = 0;
             flag = 0;
             TupleCond = 0;
             prims[i].NumItems = 0;
             while(1)
             {
                 pthread_mutex_lock(&prims[i].lock);
                 printf("Number of items is %d\n", prims[i].NumItems);               
                 while(prims[i].NumItems == 0 && !(*done))
                 {
                    printf("reducer %d waiting on empty\n", i);
                    pthread_cond_wait(&prims[i].empty, &prims[i].lock);
                 }
             
                 if(prims[i].NumItems == 0 && (*done))
                 {
                    printf("reducer %d is exiting\n", i);
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
                    
                    pthread_mutex_unlock(&prims[i].lock);
                    break;  
                }

                printf("consuming tuple from buffer %d,number of items is %d\n", i, prims[i].NumItems);
                printf("(%s,%s,%d)\n", buffer[i][out].Uid, buffer[i][out].Topic, buffer[i][out].score);
                strcpy(consumer[TupleCond].Uid, buffer[i][out].Uid);
                consumer[TupleCond].score = buffer[i][out].score;
                strcpy(consumer[TupleCond].Topic, buffer[i][out].Topic);
                TupleCond++;
                prims[i].NumItems--;
                out = (out + 1) % BufSize;
                 
                pthread_cond_signal(&prims[i].full);
                pthread_mutex_unlock(&prims[i].lock);
             }             
        } 
        else
        {
             i++;
        }
    }
    int listNum;
    int re;
    int first = 0;
    flag = 0;
    char c;
    int in[BufNum];
    for(j = 0; j < BufNum; j++)
    {
        in[j] = 0;
    }
    struct idlist list[BufNum];
    struct tuple temp;
    while(1)
    {
        re = scanf("(%[^,],%c,%[^)])%c", temp.Uid, &temp.Act, temp.Topic, &c);
        if(re != 4)
        {
             *done = 1;
             for(ii = 0; ii < BufNum; ii++)
             {
                 pthread_cond_signal(&prims[ii].empty);
             }
             break;
        }
        switch(temp.Act){
             case 'P':
		temp.score = 50;
		break;
	     case 'L':
		temp.score = 20;
		break;
	     case 'D':
		temp.score = -10;
		break;
	     case 'C':
		temp.score = 30;
		break;
	     case 'S':
		temp.score = 40;
		break;
        }
        if(listNum == 0 && first == 0)
        {
             strcpy(list[listNum].Uid, temp.Uid);
             list[listNum].Rid = 0;
             temp.Rid = list[listNum].Rid;
             first = 1;
        }
        else
        {
             for(ii = 0; ii <= listNum; ii++)
             {
                if(strcmp(list[ii].Uid, temp.Uid) == 0)
                 {
                     temp.Rid = list[ii].Rid;
                     flag = 1;          
                 }
             }
             if(flag == 0)
             {
                 listNum++;
                 strcpy(list[listNum].Uid, temp.Uid);
                 list[listNum].Rid = list[listNum-1].Rid + 1;
                 temp.Rid = list[listNum].Rid;
             }
                 flag = 0;

        }
        printf("RId is %d , number of items is %d\n", temp.Rid, prims[temp.Rid].NumItems);
        pthread_mutex_lock(&prims[temp.Rid].lock);
        while(prims[temp.Rid].NumItems == BufSize)
        {
             printf("buffer %d full\n", temp.Rid);
             pthread_cond_signal(&prims[temp.Rid].empty);
             pthread_cond_wait(&prims[temp.Rid].full, &prims[temp.Rid].lock);
        }
        printf("insert tuple to buffer %d\n", temp.Rid);
        strcpy(buffer[temp.Rid][in[temp.Rid]].Uid, temp.Uid);
        buffer[temp.Rid][in[temp.Rid]].score = temp.score;
        strcpy(buffer[temp.Rid][in[temp.Rid]].Topic, temp.Topic);
        printf("(%s,%s,%d)\n", buffer[temp.Rid][in[temp.Rid]].Uid, buffer[temp.Rid][in[temp.Rid]].Topic, buffer[temp.Rid][in[temp.Rid]].score);
        in[temp.Rid] = (in[temp.Rid] + 1) % BufSize;
        prims[temp.Rid].NumItems++; 
        printf("current id is %d\n", temp.Rid);      
        pthread_mutex_unlock(&prims[temp.Rid].lock);
        pthread_cond_signal(&prims[temp.Rid].empty);
    }
    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);
    exit(0);  
}

#include <stdio.h>
#include <string.h>

struct tuple{
     char Uid[5];
     char Act;
     char Topic[16];
     int Score;
};

int main()
{
     int i = 0;
     int j = 0;
     int re = 3;
     int n = 0;
     int nn = 0;
     int s = 0;
     int flag = 0;
     char c;
     struct tuple tuples[100];
     while(1)
     {
         re = scanf("(%[^,],%[^,],%d)%c", tuples[i].Uid, tuples[i].Topic, &tuples[i].Score,&c);
         if(re != 4)
         {
               break;
         }
         i++;
     }
     struct tuple reducer[100];
     for(j = 0; j < i; j++)
     {
         if(j == 0)
         {
            strcpy(reducer[n].Uid, tuples[j].Uid);
            reducer[n].Score = tuples[j].Score;
            strcpy(reducer[n].Topic, tuples[j].Topic);
         }
         else if(strcmp(tuples[j].Uid, tuples[j-1].Uid) == 0)
         {
            for(nn = s; nn <= n; nn++)
            {
               if(strcmp(tuples[j].Topic,reducer[nn].Topic) == 0)
               {
                  reducer[nn].Score += tuples[j].Score;
                  flag = 1;
                  
               }
            }
            if(flag == 0)
            {
               n++;
               strcpy(reducer[n].Uid, tuples[j].Uid);
               reducer[n].Score = tuples[j].Score;
               strcpy(reducer[n].Topic, tuples[j].Topic);
            }
            flag = 0;
            if(j == i-1)
            {
               for(nn = s; nn <= n; nn++)
               {
                  printf("(%s,%s,%d)\n", reducer[nn].Uid, reducer[nn].Topic, reducer[nn].Score);
               }
            }           
         }
         else if(strcmp(tuples[j].Uid, tuples[j-1].Uid) != 0)
         {
            for(nn = s; nn<=n; nn++)
            {
               printf("(%s,%s,%d)\n", reducer[nn].Uid, reducer[nn].Topic, reducer[nn].Score);
            }
            n++;
            s = n;
            strcpy(reducer[n].Uid, tuples[j].Uid);
            reducer[n].Score = tuples[j].Score;
            strcpy(reducer[n].Topic, tuples[j].Topic);
            if (j == i - 1)
	    {
	       printf("(%s,%s,%d)\n", reducer[n].Uid, reducer[n].Topic, reducer[n].Score);
	    }
         }
     }
}

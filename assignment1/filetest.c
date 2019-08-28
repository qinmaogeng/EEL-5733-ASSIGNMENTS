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
     char comma = ',';
     struct tuple tuples[100];
     FILE *fp = fopen("input.txt","r");
     while(comma == ',')
     {
           fscanf(fp,"(%[^,],%c,%[^)])%c", tuples[i].Uid, &tuples[i].Act, tuples[i].Topic, &comma);
           switch(tuples[i].Act){
                case 'P':
			tuples[i].Score = 50;
			break;
		case 'L':
			tuples[i].Score = 20;
			break;
		case 'D':
			tuples[i].Score = -10;
			break;
		case 'C':
			tuples[i].Score = 30;
			break;
		case 'S':
			tuples[i].Score = 40;
			break;
           }
           printf("(%s,%s,%d)\n", tuples[i].Uid, tuples[i].Topic, tuples[i].Score);
           i++;
     }
     fclose(fp);
     return 0;
}

#include <stdio.h>
static int hit(int x,int y){int dx=x-48,dy=y-32;return dx*dx+dy*dy<576;}
int main(void){FILE *f=fopen("render.ppm","w");if(!f)return 2;fprintf(f,"P3\n96 64\n255\n");for(int y=0;y<64;++y)for(int x=0;x<96;++x){int v=hit(x,y)?255:0;fprintf(f,"%d %d %d ",v,v,v);}fclose(f);return 0;}

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define CLOCKS_PER_SECOND 10

long gettime(struct timeval tv)
{
        return tv.tv_sec*1000+tv.tv_usec/1000;
}

void write_freq(FILE *);
void write_util(FILE *);
void write_temp(FILE *);
void write_gpu(FILE *);
void write_battery(FILE *);
void write_bus(FILE *);

int main(int argc, char* argv[])
{
	int sampling_period=10, sampling_time=1000000000;

        struct timeval tv;
        long btime, ctime, atime;
        long freq;
        int temp;
        int value=100;
        FILE *fp;
        pid_t pid;
	char fname[100];
	char strbuf[100];
	char tmp[100];
	FILE *buf=NULL;
	int debug=1;

	gettimeofday(&tv,NULL);
        btime=gettime(tv);
        atime=btime;

        printf("Also checking %d\n",value);
	sprintf(fname,"log/%s.txt",argv[1]);
        fp=fopen(fname,"w");

        
        fprintf(fp,"Time\tCPU0_freq\tCPU4_freq\tCPU7_freq\t");
        fprintf(fp, "CPU0_util\tCPU1_util\tCPU2_util\tCPU3_util\tCPU4_util\tCPU5_util\tCPU6_util\tCPU7_util\t");
        fprintf(fp, "MIF_Freq\tGPU_Freq\tGPU_Load\t");
        for(int i = 0; i < 6; i++){
        	sprintf(strbuf, "/sys/class/thermal/thermal_zone%d/type", i);
		buf=fopen(strbuf, "r");
		fscanf(buf, "%s", tmp);
		fprintf(fp, "%s\t", tmp);
        }
        fprintf(fp, "CPU0_state\tCPU4_state\tCPU7_state\tGPU_state\tTPU_state\tbatt_volt\tbatt_curr\t");
        fprintf(fp, "EOL\n");

        while(1){
//              if(i>=value) break;
                usleep(1000);
                gettimeofday(&tv,NULL);
                ctime=gettime(tv);


                if(ctime-btime<sampling_period)
                        continue;

		fprintf(fp,"[%ld]\t",ctime-atime);

		write_freq(fp);

		write_util(fp);
	
		write_bus(fp);

		write_gpu(fp);

		write_temp(fp);

		write_battery(fp);

		fprintf(fp,"EOL\n");

                btime+=ctime;
                if(ctime-atime>sampling_time)
                         break;
        }
        fclose(fp);
        return 0;
 
	
}

void write_freq(FILE *fp)
{
	FILE *buf=NULL;
	char strbuf[200];
	long big_freq, mid_freq, little_freq;
	int i;

	for(i=0;i<1;i++){
		sprintf(strbuf, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
		buf=fopen(strbuf, "r");
		if(buf==NULL){
			fprintf(fp,"OFF\t");
		}
		else{
			fscanf(buf,"%ld",&little_freq);
			fprintf(fp,"%ld\t", little_freq);
			fclose(buf);
		}
	}

	for(i=4;i<5;i++){
		sprintf(strbuf, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
		buf=fopen(strbuf, "r");
		if(buf==NULL){
			fprintf(fp,"OFF\t");
		}
		else{
			fscanf(buf,"%ld",&mid_freq);
			fprintf(fp,"%ld\t", mid_freq);
			fclose(buf);
		}
	}	
	
	for(i=7;i<8;i++){
		sprintf(strbuf, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
		buf=fopen(strbuf, "r");
		if(buf==NULL){
			fprintf(fp,"OFF\t");
		}
		else{
			fscanf(buf,"%ld",&big_freq);
			fprintf(fp,"%ld\t", big_freq);
			fclose(buf);
		}
	}
	
	/*
	buf=fopen("/sys/devices/system/cpu/cpu4/cpufreq/scaling_cur_freq","r");

	if(buf==NULL){
		fprintf(fp,"big_FREQ: OFF\t");
	}
	else{
		fscanf(buf,"%ld",&big_freq);
		fprintf(fp,"big_FREQ: %ld\t", big_freq);
		fclose(buf);
	}

	buf=fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq","r");

	if(buf==NULL){
		fprintf(fp,"LITTLE_FREQ: OFF\t");
	}
	else{
		fscanf(buf,"%ld",&little_freq);	
		fprintf(fp,"LITTLE_FREQ: %ld\t",little_freq);
       	 	fclose(buf);
        }*/

}


void write_util(FILE *fp)
{
	FILE *stat, *buf;
	int i,j,sum;
	char strbuf[200];
	static int last[8][10]={0,};
	int current[10],util[10];
	stat=fopen("/proc/stat","r");
	fgets(strbuf,200,stat);
	for(i=0;i<8;i++){
		sprintf(strbuf,"/sys/devices/system/cpu/cpu%d/online",i);
		buf=NULL;
		buf=fopen(strbuf,"r");
		if(buf==NULL){
                        fprintf(fp,"OFF\t");
                        continue;
		}
		fscanf(buf,"%d",&j);
		fclose(buf);
		if(j==0){
			fprintf(fp,"OFF\t");
			continue;
		}

		fscanf(stat,"%s",strbuf);
		sum=0;
		for(j=0;j<10;j++){
			fscanf(stat,"%d",&current[j]);
			util[j]=current[j]-last[i][j];
			last[i][j]=current[j];
			sum+=util[j];
		}

		if(sum==0)
			fprintf(fp,"0\t");
		else
			fprintf(fp,"%d\t",(util[0]+util[1]+util[2]+util[5]+util[6]+util[7])*100/sum);

	}

	fclose(stat);
}


void write_temp(FILE *fp)
{
	FILE *buf;
	char strbuf[200];
	int temperature[10], i;
	int temp;

	for(i = 0; i < 6; i++){
		sprintf(strbuf, "/sys/class/thermal/thermal_zone%d/temp", i);
		buf=fopen(strbuf, "r");
		fscanf(buf, "%d", &temp);
		fprintf(fp, "%d\t", temp);
	}

	/*
	BIG - 0, MID - 1, LITTLE - 2, G3D - 3, ISP - 4, TPU - 5, skin_temp2 - 35, skin_temp1 - 39
	*/
	sprintf(strbuf, "/sys/class/thermal/cooling_device8/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);
	
	sprintf(strbuf, "/sys/class/thermal/cooling_device9/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);

	sprintf(strbuf, "/sys/class/thermal/cooling_device10/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);

	sprintf(strbuf, "/sys/class/thermal/cooling_device16/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);

	sprintf(strbuf, "/sys/class/thermal/cooling_device11/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);

/*big:
	buf=fopen("/sys/class/thermal/thermal_zone0/temp","r");
	if(buf==NULL){
                fprintf(fp,"OFF\t");
		fclose(buf);
                goto mid;
	}
	fscanf(buf,"%d", &temperature[0]);
	fprintf(fp,"%d\t",temperature[0]);
	fclose(buf);
mid:
	buf=fopen("/sys/class/thermal/thermal_zone1/temp","r");
	if(buf==NULL){
                fprintf(fp,"OFF\t");
		fclose(buf);
                goto little;
	}
	fscanf(buf,"%d", &temperature[1]);
	fprintf(fp,"%d\t",temperature[1]);
	fclose(buf);
little:
	buf=fopen("/sys/class/thermal/thermal_zone2/temp","r");
	if(buf==NULL){
                fprintf(fp,"OFF\t");
		fclose(buf);
                goto gpu1;
	}
	fscanf(buf,"%d", &temperature[2]);
	fprintf(fp,"%d\t",temperature[2]);
	fclose(buf);
gpu1:
	buf=fopen("/sys/class/thermal/thermal_zone3/temp","r");
	if(buf==NULL){
                fprintf(fp,"OFF\t");
		fclose(buf);
                goto gpu2;
	}
	fscanf(buf,"%d", &temperature[8]);
	fprintf(fp,"%d\t",temperature[8]);
	fclose(buf);
gpu2:
	buf=fopen("/sys/class/thermal/thermal_zone9/temp","r");
	if(buf==NULL){
                fprintf(fp,"OFF\t");
                goto end;
	}
	fscanf(buf,"%d", &temperature[9]);
	fprintf(fp,"%d\t",temperature[9]);
end:
*/
        fclose(buf);
}

void write_gpu(FILE *fp)
{
	FILE *buf;
	char util[200];
	char norm_util[200];
	char norm_freq[200];
	
	buf=fopen("/sys/devices/platform/28000000.mali/cur_freq","r");
	if(buf==NULL){
		fprintf(fp,"OFF\t");
	}
	else{
		fscanf(buf,"%s",norm_freq);
		fprintf(fp,"%d\t", atoi(norm_freq)/1000);
		fclose(buf);
	}

	//fprintf(fp, "GPU_UTIL: %s\t", norm_util);
	//fprintf(fp, "GPU_FREQ: %s\t", norm_freq);	
	
	
	buf=fopen("/sys/devices/platform/28000000.mali/utilization","r");
	if(buf==NULL){
		fprintf(fp,"OFF\t");
	}
	else{
		fscanf(buf,"%s",norm_util);
		fprintf(fp,"%d\t", atoi(norm_util));
		fclose(buf);
	}

	//fprintf(fp, "GPU_UTIL: %s\t", norm_util);
	//fprintf(fp, "GPU_FREQ: %s\t", norm_freq);	
	
}

void write_battery(FILE *fp)
{
	FILE *buf;
	char voltage[200];
	char current[200];

	buf=fopen("/sys/class/power_supply/battery/voltage_now","r");
	if(buf==NULL){
		fprintf(fp,"OFF\t");
	}
	else{	
		fscanf(buf,"%s",voltage);
		fprintf(fp,"%d\t", atoi(voltage));
	}
	fclose(buf);

	buf=fopen("/sys/class/power_supply/battery/current_now","r");
	if(buf==NULL){
		fprintf(fp,"OFF\t");
	}
	else{
		fscanf(buf,"%s",current);
		fprintf(fp,"%d\t", atoi(current));
	}
	fclose(buf);
}

void write_bus(FILE *fp)
{
	FILE *buf;
	char cci_bus_freq[200];
	char cpu_bus_freq[200];
	char gpu_bus_freq[200];

	buf=fopen("/sys/class/devfreq/17000010.devfreq_mif/cur_freq","r");
	if(buf==NULL){
		fprintf(fp,"OFF\t");
	}
	else{
		fscanf(buf,"%s",cpu_bus_freq);
		fprintf(fp,"%d\t", atoi(cpu_bus_freq));
	}
	fclose(buf);
	/*
	buf=fopen("/sys/class/devfreq/17000020.devfreq_int/cur_freq","r");
	if(buf==NULL){
		fprintf(fp,"OFF\t");
	}
	else{
		fscanf(buf,"%s",gpu_bus_freq);
		fprintf(fp,"%d\t", atoi(gpu_bus_freq));
	}
	fclose(buf);

	buf=fopen("/sys/class/devfreq/170000a0.devfreq_npu/cur_freq","r");
	if(buf==NULL){
		fprintf(fp,"OFF\t");
	}
	else{
		fscanf(buf,"%s",cci_bus_freq);
		fprintf(fp,"%d\t", atoi(cci_bus_freq));
	}
	fclose(buf);		
	*/
}
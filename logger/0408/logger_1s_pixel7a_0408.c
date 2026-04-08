#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define SAMPLING_TIME 1000

typedef unsigned long long u64;
long gettime(struct timeval tv)
{
        return tv.tv_sec*1000+tv.tv_usec/1000;
}
// current time starts at 1ms, so we have to calculate with elapsed time

void write_freq(FILE *);
void write_util(FILE *);
void write_temp(FILE *);
void write_gpu(FILE *);
void write_battery(FILE *);
void write_bus(FILE *);
void write_governer(FILE *);

int main(int argc, char* argv[])
{
	int op_period=1000000000;

        struct timeval tv;
        long btime, ctime, atime;
        long freq;
        int temp;
		long count = 0;
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
	sprintf(fname,"%s.txt",argv[1]);
        fp=fopen(fname,"w");

        
        fprintf(fp,"Time\tLIT0_freq\tLIT1_freq\tLIT2_freq\tLIT3_freq\tMID4_freq\tMID5_freq\tBIG6_freq\tBIG7_freq\t");
        fprintf(fp, "LIT0_util\tLIT1_util\tLIT2_util\tLIT3_util\tMID4_util\tMID5_util\tBIG6_util\tBIG7_util\t");
        fprintf(fp, "MIF_Freq\tGPU_Freq\tGPU_Load\t"); // 17000010.devfreq_mif/cur_freq + /28000000.mali/cur_freq
        for(int i = 9; i < 12; i++){
        	sprintf(strbuf, "/sys/class/thermal/thermal_zone%d/type", i);
		buf=fopen(strbuf, "r");
		fscanf(buf, "%s", tmp);
		fprintf(fp, "%s\t", tmp);
        }
        fprintf(fp, "LIT_state\tMID_state\tBIG_state\t");
		
		fprintf(fp, "GPU_state\tTPU_state\tbatt_volt\tbatt_curr\t");
		fprintf(fp, "LIT0_governer\tLIT1_governer\tLIT2_governer\tLIT3_governer\tMID4_governer\tMID5_governer\tBIG6_governer\tBIG7_governer\t");
        fprintf(fp, "EOL\n"); // state = cooling_device8/9/10/16/11/cur_state + EOL 종료

        while(1){
                usleep(SAMPLING_TIME * 1000);
                gettimeofday(&tv,NULL);
                ctime=gettime(tv);


                if (ctime - btime >= SAMPLING_TIME) {

					fprintf(fp, "%ld\t", count * SAMPLING_TIME);

					write_freq(fp);
					write_util(fp);
					write_bus(fp);
					write_gpu(fp);
					write_temp(fp);
					write_battery(fp);
					write_governer(fp);
					fprintf(fp,"EOL\n");
             		fflush(fp); 
					btime += SAMPLING_TIME; 
					count++;
        	}
                if(ctime-atime>op_period)
                         break;
        }
        fclose(fp);
        return 0;
 
	
}

void write_freq(FILE *fp)
{
	FILE *buf=NULL;
	char strbuf[200];
	long freq;
	int i;


	for(i=0;i<8;i++){
		sprintf(strbuf, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
		buf=fopen(strbuf, "r");
		if(buf==NULL){
			fprintf(fp,"OFF\t");
		}
		else{
			fscanf(buf,"%ld",&freq);
			fprintf(fp,"%ld\t", freq);
			fclose(buf);
		}
	}
	


}




void write_governer(FILE *fp)
{
	FILE *buf=NULL;
	char strbuf[200];
	char governor[200];
	int i;

	for(i=0;i<8;i++){
		sprintf(strbuf, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);
		buf=fopen(strbuf, "r");
		if(buf==NULL){
			fprintf(fp,"OFF\t");
		}
		else{
			fscanf(buf,"%s",governor);
			fprintf(fp,"%s\t", governor);
			fclose(buf);
		}
	}
}


void write_util(FILE *fp) {
    FILE *stat_fp;
    char line[256];
    static u64 last[8][10] = {0,};
    u64 current[8][10] = {0,};
    int online[8] = {0,};
    

    stat_fp = fopen("/proc/stat", "r");
    if (!stat_fp) return;

    fgets(line, sizeof(line), stat_fp); 

    for (int i = 0; i < 8; i++) {
        if (fgets(line, sizeof(line), stat_fp)) {
            
            sscanf(line, "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &current[i][0], &current[i][1], &current[i][2], &current[i][3],
                   &current[i][4], &current[i][5], &current[i][6], &current[i][7],
                   &current[i][8], &current[i][9]);
        }
    }
    fclose(stat_fp);

    
    for (int i = 0; i < 8; i++) {
        char path[64];
        int is_online = 0;
        sprintf(path, "/sys/devices/system/cpu/cpu%d/online", i);
        
        FILE *online_fp = fopen(path, "r");
        if (online_fp) {
            fscanf(online_fp, "%d", &is_online);
            fclose(online_fp);
        } else {
            
            is_online = 1; 
        }

        if (is_online == 0) {
            fprintf(fp, "OFF\t");
            continue;
        }

        
        u64 diff[10];
        u64 total_diff = 0;
        for (int j = 0; j < 10; j++) {
            if (current[i][j] < last[i][j]) {
                diff[j] = 0; 
            } else {
                diff[j] = current[i][j] - last[i][j];
            }
            last[i][j] = current[i][j];
            total_diff += diff[j];
        }

        
        if (total_diff == 0) {
            fprintf(fp, "0\t");
        } else {
            u64 active_diff = diff[0] + diff[1] + diff[2] + diff[5] + diff[6] + diff[7];
            fprintf(fp, "%llu\t", (active_diff * 100) / total_diff);
        }
    }
}


void write_temp(FILE *fp)
{
	FILE *buf;
	char strbuf[200];
	int temperature[10], i;
	int temp;

	for(i = 9; i < 12; i++){
		sprintf(strbuf, "/sys/class/thermal/thermal_zone%d/temp", i);
		buf=fopen(strbuf, "r");
		fscanf(buf, "%d", &temp);
		fprintf(fp, "%d\t", temp);
		fclose(buf);
	}

	/*
	BIG - 0, MID - 1, LITTLE - 2, G3D - 3, ISP - 4, TPU - 5, skin_temp2 - 35, skin_temp1 - 39
	*/
	sprintf(strbuf, "/sys/class/thermal/cooling_device10/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);
	fclose(buf);
	
	sprintf(strbuf, "/sys/class/thermal/cooling_device12/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);
	fclose(buf);

	sprintf(strbuf, "/sys/class/thermal/cooling_device14/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);
	fclose(buf);

	sprintf(strbuf, "/sys/class/thermal/cooling_device22/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);
	fclose(buf);

	sprintf(strbuf, "/sys/class/thermal/cooling_device21/cur_state");
	buf=fopen(strbuf, "r");
	fscanf(buf, "%d", &temp);
	fprintf(fp, "%d\t", temp);
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